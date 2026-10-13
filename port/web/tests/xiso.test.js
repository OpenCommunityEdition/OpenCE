// The disc image importer (port/web/site/xiso-worker.js) on images made here:
// a valid one, and ones with damaged directories and maps the game would not
// load. node --test "port/web/tests/*.test.js"

'use strict';

const test = require('node:test');
const assert = require('node:assert');
const xiso = require('../site/xiso-worker.js');

const SECTOR = 2048;
const MAGIC = 'MICROSOFT*XBOX*MEDIA';

// a map file's first 2048 bytes as cache_files.c checks them
function mapHeader({ head = 'daeh', foot = 'toof', version = 5, build = '01.10.12.2276', size = 4096 } = {}) {
  const bytes = new Uint8Array(size);
  bytes.set(Buffer.from(head, 'latin1'), 0);
  new DataView(bytes.buffer).setUint32(4, version, true);
  bytes.set(Buffer.from(build, 'latin1'), 0x40);
  bytes.set(Buffer.from(foot, 'latin1'), 0x7FC);
  return bytes;
}

// a directory table of the files, as a balanced XDVDFS tree (entries are
// 4-byte aligned; left and right subtree offsets in 4-byte units)
function directoryTable(entries) {
  const sorted = [...entries].sort((a, b) => a.name.toLowerCase().localeCompare(b.name.toLowerCase()));
  let offset = sorted.reduce((sum, entry) => sum + ((14 + entry.name.length + 3) & ~3), 0);
  const table = new Uint8Array(Math.max(SECTOR, (offset + SECTOR - 1) & ~(SECTOR - 1)));
  table.fill(0xFF, offset);
  // the root of the tree at offset 0: rebuild the order so that the middle
  // entry comes first
  const order = [];
  const place = (low, high) => {
    if (low > high) return -1;
    const middle = (low + high) >> 1;
    const index = order.length;
    order.push({ entry: sorted[middle], left: -1, right: -1 });
    order[index].left = place(low, middle - 1);
    order[index].right = place(middle + 1, high);
    return index;
  };
  place(0, sorted.length - 1);
  const at = [];
  offset = 0;
  for (const node of order) {
    at.push(offset);
    offset += (14 + node.entry.name.length + 3) & ~3;
  }
  const view = new DataView(table.buffer);
  order.forEach((node, index) => {
    const base = at[index];
    view.setUint16(base, node.left >= 0 ? at[node.left] / 4 : 0, true);
    view.setUint16(base + 2, node.right >= 0 ? at[node.right] / 4 : 0, true);
    view.setUint32(base + 4, node.entry.sector, true);
    view.setUint32(base + 8, node.entry.size, true);
    table[base + 12] = node.entry.directory ? 0x10 : 0x20;
    table[base + 13] = node.entry.name.length;
    table.set(Buffer.from(node.entry.name, 'latin1'), base + 14);
  });
  return table;
}

// an image: its maps folder's files, where its game partition starts, its
// default.xbe and other entries of its root
function image({ maps, partition = 0, rootEntries = [], xbe = new Uint8Array(100) }) {
  const chunks = new Map();
  let nextSector = 0x30;
  const allocate = (bytes) => {
    const sector = nextSector;
    nextSector += Math.ceil(bytes.length / SECTOR) + 1;
    chunks.set(sector, bytes);
    return sector;
  };
  const mapEntries = maps.map((map) => ({ name: map.name, size: map.data.length, sector: allocate(map.data) }));
  const mapsTable = directoryTable(mapEntries);
  const mapsSector = allocate(mapsTable);
  const root = directoryTable([{ name: 'maps', directory: true, sector: mapsSector, size: mapsTable.length },
    { name: 'default.xbe', sector: allocate(xbe), size: xbe.length }, ...rootEntries]);
  const rootSector = allocate(root);
  const size = partition + nextSector * SECTOR;
  const bytes = new Uint8Array(size);
  const descriptor = partition + 0x10000;
  bytes.set(Buffer.from(MAGIC, 'latin1'), descriptor);
  bytes.set(Buffer.from(MAGIC, 'latin1'), descriptor + 0x7EC);
  new DataView(bytes.buffer).setUint32(descriptor + 20, rootSector, true);
  new DataView(bytes.buffer).setUint32(descriptor + 24, root.length, true);
  for (const [sector, chunk] of chunks) bytes.set(chunk, partition + sector * SECTOR);
  return new File([bytes], 'halo.iso');
}

const goodMaps = [
  { name: 'ui.map', data: mapHeader() },
  { name: 'a10.map', data: mapHeader({ size: 9000 }) },
  { name: 'bloodgulch.map', data: mapHeader({ build: '01.01.14.2342' }) },
  { name: 'loading.tga', data: new Uint8Array(50) },
];

test('a map header the game loads passes; others say why not', () => {
  assert.strictEqual(xiso.mapProblem(mapHeader()), null);
  assert.strictEqual(xiso.mapProblem(mapHeader({ build: '01.01.14.2342' })), null);
  assert.match(xiso.mapProblem(mapHeader({ head: 'xxxx' })), /not a map/);
  assert.match(xiso.mapProblem(mapHeader({ foot: 'xxxx' })), /not a map/);
  assert.match(xiso.mapProblem(mapHeader({ version: 7 })), /cache version is 7/);
  assert.match(xiso.mapProblem(mapHeader({ build: '01.00.00.0564' })), /build/);
  assert.match(xiso.mapProblem(new Uint8Array(100)), /too short/);
});

test('the maps folder comes out of an image, at either partition offset', async () => {
  for (const partition of [0, 0x02080000]) {
    const files = await xiso.imageFiles(image({ maps: goodMaps, partition }));
    assert.deepStrictEqual(files.map((file) => file.name).sort(), ['a10.map', 'bloodgulch.map', 'loading.tga', 'ui.map']);
    const a10 = files.find((file) => file.name === 'a10.map');
    assert.strictEqual(a10.size, 9000);
    const start = await a10.read(0, 4);
    assert.strictEqual(Buffer.from(start).toString('latin1'), 'daeh');
  }
});

test('an image without ui.map, or with a map the game would not load, is refused', async () => {
  await assert.rejects(xiso.imageFiles(image({ maps: goodMaps.filter((map) => map.name !== 'ui.map') })), /no ui\.map/);
  await assert.rejects(xiso.imageFiles(image({ maps: [...goodMaps, { name: 'b30.map', data: mapHeader({ version: 609 }) }] })),
    /b30\.map cannot be used/);
});

test('a file that is not an image is refused', async () => {
  await assert.rejects(xiso.imageFiles(new File([new Uint8Array(0x20000)], 'x.iso')), /not an Xbox disc image/);
});

test('a directory that points at itself, or past its table, ends the walk', () => {
  // one entry whose left subtree is itself and whose right is past the end
  const table = new Uint8Array(64);
  const view = new DataView(table.buffer);
  view.setUint16(0, 0, true);
  view.setUint16(2, 100, true);
  table[13] = 3;
  table.set(Buffer.from('a.b', 'latin1'), 14);
  assert.deepStrictEqual(xiso.walkDirectory(table).map((entry) => entry.name), ['a.b']);
  // two entries, each the other's left subtree
  const cycle = new Uint8Array(64);
  const cycleView = new DataView(cycle.buffer);
  cycleView.setUint16(0, 5, true);
  cycle[13] = 1;
  cycle.set(Buffer.from('x', 'latin1'), 14);
  cycleView.setUint16(20, 0, true);
  cycle[33] = 1;
  cycle.set(Buffer.from('y', 'latin1'), 34);
  assert.deepStrictEqual(xiso.walkDirectory(cycle).map((entry) => entry.name).sort(), ['x', 'y']);
});

test("a file whose extent runs past the image's end is refused", async () => {
  const good = image({ maps: goodMaps });
  const truncated = new File([good.slice(0, good.size - SECTOR * 4)], 'cut.iso');
  await assert.rejects(xiso.imageFiles(truncated), /ends before|damaged|complete/);
});

test('names with paths or hidden files are not copied', () => {
  assert.ok(xiso.plainName('a10.map'));
  assert.ok(!xiso.plainName('../a10.map'));
  assert.ok(!xiso.plainName('maps/a10.map'));
  assert.ok(!xiso.plainName('.complete'));
  assert.ok(!xiso.plainName(''));
});

test('a maps folder chosen as files is checked as an image is', async () => {
  const files = goodMaps.map((map) => new File([map.data], map.name));
  const result = await xiso.folderFiles([...files, new File([new Uint8Array(10)], 'notes.txt')]);
  assert.deepStrictEqual(result.map((file) => file.name).sort(), ['a10.map', 'bloodgulch.map', 'loading.tga', 'ui.map']);
  await assert.rejects(xiso.folderFiles(files.filter((file) => file.name !== 'ui.map')), /no ui\.map/);
});

// the loading screen's picture (xiso-worker.js's loadingPicture): made-up
// executables holding a picture of grey bands as the retail one holds its own
const LOADING_WIDTH = 320;
const LOADING_HEIGHT = 240;
const loadingSizes = new Uint8Array([64, 1, 0, 0, 240, 0, 0, 0]);
const loadingGreys = Uint8Array.from({ length: LOADING_WIDTH * LOADING_HEIGHT },
  (_, pixel) => ((Math.floor(pixel / LOADING_WIDTH / 15) % 16) << 4));

// greys (multiples of 16) as the executable's runs: a byte each, the run's
// length in the high nibble, the grey's high bits in the low one
function loadingRuns(greys) {
  const out = [];
  for (let at = 0; at < greys.length;) {
    let length = 1;
    while (at + length < greys.length && greys[at + length] === greys[at] && length < 15) length++;
    out.push(length << 4 | greys[at] >> 4);
    at += length;
  }
  return Uint8Array.from(out);
}

function concatenate(...parts) {
  const out = new Uint8Array(parts.reduce((sum, part) => sum + part.length, 0));
  let at = 0;
  for (const part of parts) {
    out.set(part, at);
    at += part.length;
  }
  return out;
}

// (another 320, 240 before it, whose runs do not fill a picture)
const loadingXbe = concatenate(new Uint8Array(1000).fill(0xCC), loadingSizes, new Uint8Array([0x10, 0x00]),
  new Uint8Array(64).fill(0x90), loadingSizes, loadingRuns(loadingGreys), new Uint8Array(1000).fill(0xCC));

test("the loading screen's picture is found in default.xbe and written as the TGA the game reads", () => {
  const tga = xiso.loadingPicture(loadingXbe);
  assert.ok(tga);
  // (progress_bar.c: an uncompressed 24-bit picture of 320 by 240)
  assert.strictEqual(tga[2], 2);
  assert.strictEqual(tga[16], 24);
  assert.strictEqual(tga[12] | tga[13] << 8, LOADING_WIDTH);
  assert.strictEqual(tga[14] | tga[15] << 8, LOADING_HEIGHT);
  assert.strictEqual(tga.length, 18 + LOADING_WIDTH * LOADING_HEIGHT * 3);
  // (rows from the bottom, the grey in each channel)
  for (const y of [0, 100, LOADING_HEIGHT - 1]) {
    const row = tga.subarray(18 + (LOADING_HEIGHT - 1 - y) * LOADING_WIDTH * 3, 18 + (LOADING_HEIGHT - y) * LOADING_WIDTH * 3);
    for (let x = 0; x < LOADING_WIDTH; x += 37) {
      const grey = loadingGreys[y * LOADING_WIDTH + x];
      assert.deepStrictEqual([...row.subarray(x * 3, x * 3 + 3)], [grey, grey, grey]);
    }
  }
});

test('runs that do not fill the picture exactly are not it', () => {
  const runs = loadingRuns(loadingGreys);
  assert.strictEqual(xiso.loadingPicture(new Uint8Array(100).fill(0xCC)), null);
  assert.strictEqual(xiso.loadingPicture(concatenate(loadingSizes, runs.subarray(0, -1))), null);
  // (a run past the picture's end: its last 15 pixels as 14 and then 2)
  assert.strictEqual(xiso.loadingPicture(concatenate(loadingSizes, runs.subarray(0, -1), new Uint8Array([0xE0, 0x20]))), null);
  // (a run of none)
  assert.strictEqual(xiso.loadingPicture(concatenate(loadingSizes, runs.subarray(0, -1), new Uint8Array([0x00, 0xF0]))), null);
});

test("an image whose maps folder has no loading.tga gets one from its default.xbe; one that has one keeps it", async () => {
  const maps = goodMaps.filter((map) => map.name !== 'loading.tga');
  const files = await xiso.imageFiles(image({ maps, xbe: loadingXbe }));
  const picture = files.find((file) => file.name === 'loading.tga');
  assert.ok(picture);
  assert.strictEqual(picture.size, 18 + LOADING_WIDTH * LOADING_HEIGHT * 3);
  assert.deepStrictEqual([...(await picture.read(0, 18))], [...xiso.loadingPicture(loadingXbe).subarray(0, 18)]);
  const kept = await xiso.imageFiles(image({ maps: goodMaps, xbe: loadingXbe }));
  assert.strictEqual(kept.filter((file) => file.name === 'loading.tga').length, 1);
  assert.strictEqual(kept.find((file) => file.name === 'loading.tga').size, 50);
  // (no picture in the executable: none, and the game makes clouds)
  const none = await xiso.imageFiles(image({ maps }));
  assert.ok(!none.some((file) => file.name === 'loading.tga'));
});
