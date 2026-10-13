/*
XISO-WORKER.JS

Copies the game's maps folder into the site's Origin Private File System,
where the game finds it (/data/maps: port/web/src/web_main.c), from the
player's own Xbox disc image of Halo: Combat Evolved (an "xiso", .iso), as
port/linux/src/xiso.c does for the desktop ports, or from a maps folder
already copied out of one. It runs in a Worker, which can write with
synchronous OPFS access handles (Safari has no OPFS writable streams), and
reads the image a slice at a time: an image is 4 to 7 GB, and nothing of it
is kept but the maps.

The image's file system is XDVDFS, read as extract-xiso does
(port/third_party/extract-xiso): 2048-byte sectors; a volume descriptor at
0x10000 that starts and ends with "MICROSOFT*XBOX*MEDIA" and gives the root
directory's sector and size; directories whose entries form a binary tree
(left and right subtree offsets in 4-byte units, the start sector, the size,
attributes, the name's length and the name). Images of a whole disc put the
game partition further in, at one of PARTITION_OFFSETS. The image is the
player's file, so it is untrusted: every offset and size is checked against
the image and the directory it is read from, and a directory's walk is
bounded.

Each map is checked as the game checks it when it loads it (cache_files.c):
"head" and "foot" signatures, the Xbox's cache version, and the build of a
retail disc (01.10.12.2276) or of this decompilation's (01.01.14.2342).

The loading screen's picture, which the retail disc has no file of, is
taken out of the image's default.xbe and written beside the maps as the
loading.tga the game reads (loadingPicture; xiso.c's write_loading_picture
does the same). Without it the loading screen makes clouds of its own.

Messages from the page: { type: 'image', file } or { type: 'folder', files }
(a maps folder's File objects). To the page: { type: 'progress', file, done,
total }, then { type: 'done', files, bytes } or { type: 'error', message }.
*/

'use strict';

const SECTOR_SIZE = 2048;
const VOLUME_DESCRIPTOR_OFFSET = 0x10000;
const PARTITION_OFFSETS = [0, 0x0FD90000, 0x02080000, 0x18300000];
const MAGIC = 'MICROSOFT*XBOX*MEDIA';
const ENTRY_HEADER_SIZE = 14;
const ATTRIBUTE_DIRECTORY = 0x10;
const MAXIMUM_DIRECTORY_SIZE = 1024 * 1024;
const MAXIMUM_ENTRIES = 4096;
const MAXIMUM_DEPTH = 64;
const COPY_CHUNK = 4 * 1024 * 1024;
const MAP_HEADER_SIZE = 0x800;
const CACHE_VERSION = 5;
const BUILDS = ['01.10.12.2276', '01.01.14.2342'];
// the loading screen's picture (source/interface/progress_bar.c)
const LOADING_WIDTH = 320;
const LOADING_HEIGHT = 240;
const TGA_HEADER_SIZE = 18;
const MAXIMUM_XBE_SIZE = 64 * 1024 * 1024;
// written last: the page starts the game only when it is there
const COMPLETE_MARKER = '.complete';

async function readAt(file, offset, size) {
  const end = Math.min(offset + size, file.size);
  if (offset < 0 || offset >= end) return new Uint8Array(0);
  return new Uint8Array(await file.slice(offset, end).arrayBuffer());
}

function text(bytes, start, length) {
  let result = '';
  for (let i = 0; i < length; i++) result += String.fromCharCode(bytes[start + i]);
  return result;
}

function u16(bytes, at) {
  return bytes[at] | bytes[at + 1] << 8;
}

function u32(bytes, at) {
  return (bytes[at] | bytes[at + 1] << 8 | bytes[at + 2] << 16 | bytes[at + 3] << 24) >>> 0;
}

async function findVolume(file) {
  for (const partition of PARTITION_OFFSETS) {
    const descriptor = await readAt(file, partition + VOLUME_DESCRIPTOR_OFFSET, SECTOR_SIZE);
    if (descriptor.length < SECTOR_SIZE) continue;
    if (text(descriptor, 0, 20) === MAGIC && text(descriptor, 0x7EC, 20) === MAGIC) {
      return { partition, rootSector: u32(descriptor, 20), rootSize: u32(descriptor, 24) };
    }
  }
  return null;
}

// a name a file of the maps folder may have: no path, nothing hidden
function plainName(name) {
  return name.length > 0 && name.length <= 64 && /^[A-Za-z0-9_.\- ]+$/.test(name) && !name.startsWith('.');
}

// the entries of a directory table: { name, sector, size, directory }
function walkDirectory(table) {
  const entries = [];
  const seen = new Set();
  const walk = (offset, depth) => {
    offset *= 4;
    // (a cycle, a tree too deep or too wide, or an entry past the table:
    // what is left of it is ignored)
    if (depth > MAXIMUM_DEPTH || seen.has(offset) || seen.size >= MAXIMUM_ENTRIES ||
        offset + ENTRY_HEADER_SIZE > table.length) {
      return;
    }
    seen.add(offset);
    const left = u16(table, offset);
    const right = u16(table, offset + 2);
    // (padding at the end of a sector)
    if (left === 0xFFFF) return;
    const nameLength = table[offset + 13];
    if (left) walk(left, depth + 1);
    if (nameLength > 0 && offset + ENTRY_HEADER_SIZE + nameLength <= table.length) {
      entries.push({
        name: text(table, offset + ENTRY_HEADER_SIZE, nameLength),
        sector: u32(table, offset + 4),
        size: u32(table, offset + 8),
        directory: (table[offset + 12] & ATTRIBUTE_DIRECTORY) !== 0,
      });
    }
    if (right) walk(right, depth + 1);
  };
  walk(0, 0);
  return entries;
}

async function readDirectory(file, volume, sector, size) {
  if (!size || size > MAXIMUM_DIRECTORY_SIZE) return null;
  const offset = volume.partition + sector * SECTOR_SIZE;
  if (offset + size > file.size) return null;
  const table = await readAt(file, offset, size);
  return table.length === size ? table : null;
}

// why a map's header is not one the game loads, or null
function mapProblem(header) {
  if (header.length < MAP_HEADER_SIZE) return 'it is too short';
  if (text(header, 0, 4) !== 'daeh' || text(header, 0x7FC, 4) !== 'toof') return 'it is not a map';
  if (u32(header, 4) !== CACHE_VERSION) return 'it is not an Xbox map (its cache version is ' + u32(header, 4) + ')';
  const build = text(header, 0x40, 32).replace(/\0.*$/s, '');
  if (!BUILDS.includes(build)) return 'its build, ' + JSON.stringify(build) + ', is not one the game loads';
  return null;
}

// the files to copy, each { name, size, read(offset, size) }; the maps are
// checked, other files (loading.tga) copied as they are
async function checkedFiles(candidates) {
  const files = [];
  for (const candidate of candidates) {
    const lower = candidate.name.toLowerCase();
    if (!plainName(candidate.name)) continue;
    if (lower.endsWith('.map')) {
      const problem = mapProblem(await candidate.read(0, MAP_HEADER_SIZE));
      if (problem) throw new Error(`${candidate.name} cannot be used: ${problem}.`);
      files.push(candidate);
    } else if (lower === 'loading.tga') {
      files.push(candidate);
    }
  }
  if (!files.some((file) => file.name.toLowerCase() === 'ui.map')) {
    throw new Error('There is no ui.map among the maps: this is not Halo: Combat Evolved for the Xbox.');
  }
  return files;
}

// The loading screen's picture out of a default.xbe, as the TGA the game
// reads (uncompressed, 24 bits, rows from the bottom), or null. The
// executable holds it, 320 by 240, as runs of 4-bit greys (a byte each: the
// grey's high four bits in its low nibble, the run's length in its high one)
// after the width and height as 32-bit numbers: found by those two numbers
// followed by runs that fill the picture exactly.
function loadingPicture(xbe) {
  const pixels = LOADING_WIDTH * LOADING_HEIGHT;
  const greys = new Uint8Array(pixels);
  let found = false;
  for (let at = 0; at + 8 <= xbe.length && !found; at++) {
    if (u32(xbe, at) !== LOADING_WIDTH || u32(xbe, at + 4) !== LOADING_HEIGHT) continue;
    let pixel = 0;
    for (let next = at + 8; next < xbe.length && pixel < pixels; next++) {
      const run = xbe[next] >> 4;
      if (!run || pixel + run > pixels) break;
      greys.fill((xbe[next] & 0x0F) << 4, pixel, pixel + run);
      pixel += run;
    }
    found = pixel === pixels;
  }
  if (!found) return null;
  const tga = new Uint8Array(TGA_HEADER_SIZE + pixels * 3);
  tga[2] = 2;
  tga[12] = LOADING_WIDTH & 0xFF;
  tga[13] = LOADING_WIDTH >> 8;
  tga[14] = LOADING_HEIGHT & 0xFF;
  tga[15] = LOADING_HEIGHT >> 8;
  tga[16] = 24;
  for (let y = 0; y < LOADING_HEIGHT; y++) {
    const row = greys.subarray((LOADING_HEIGHT - 1 - y) * LOADING_WIDTH, (LOADING_HEIGHT - y) * LOADING_WIDTH);
    for (let x = 0; x < LOADING_WIDTH; x++) {
      const out = TGA_HEADER_SIZE + (y * LOADING_WIDTH + x) * 3;
      tga[out] = tga[out + 1] = tga[out + 2] = row[x];
    }
  }
  return tga;
}

// the root's default.xbe's loading picture, as a file to copy, or null
async function loadingPictureFile(file, volume, rootEntries) {
  const xbe = rootEntries.find((entry) => !entry.directory && entry.name.toLowerCase() === 'default.xbe');
  if (!xbe || !xbe.size || xbe.size > MAXIMUM_XBE_SIZE) return null;
  const offset = volume.partition + xbe.sector * SECTOR_SIZE;
  if (offset + xbe.size > file.size) return null;
  const tga = loadingPicture(await readAt(file, offset, xbe.size));
  return tga && { name: 'loading.tga', size: tga.length, read: async (at, size) => tga.subarray(at, at + size) };
}

async function imageFiles(file) {
  const volume = await findVolume(file);
  if (!volume) throw new Error('This file is not an Xbox disc image (no XDVDFS volume was found).');
  const root = await readDirectory(file, volume, volume.rootSector, volume.rootSize);
  if (!root) throw new Error("The disc image's file system is damaged.");
  const rootEntries = walkDirectory(root);
  const maps = rootEntries.find((entry) => entry.directory && entry.name.toLowerCase() === 'maps');
  if (!maps) throw new Error('The disc image has no maps folder: it is not a Halo disc.');
  const table = await readDirectory(file, volume, maps.sector, maps.size);
  if (!table) throw new Error("The disc image's maps folder is damaged.");
  const files = await checkedFiles(walkDirectory(table).filter((entry) => !entry.directory).map((entry) => {
    const offset = volume.partition + entry.sector * SECTOR_SIZE;
    if (offset + entry.size > file.size) {
      throw new Error(`The disc image ends before ${entry.name} does: is it complete?`);
    }
    return { name: entry.name, size: entry.size, read: (at, size) => readAt(file, offset + at, Math.min(size, entry.size - at)) };
  }));
  if (!files.some((entry) => entry.name.toLowerCase() === 'loading.tga')) {
    const picture = await loadingPictureFile(file, volume, rootEntries);
    if (picture) files.push(picture);
  }
  return files;
}

async function folderFiles(list) {
  return checkedFiles(list.map((file) => ({ name: file.name, size: file.size, read: (at, size) => readAt(file, at, size) })));
}

async function writeFile(directory, source, onChunk) {
  const handle = await directory.getFileHandle(source.name, { create: true });
  const access = await handle.createSyncAccessHandle();
  try {
    access.truncate(0);
    let position = 0;
    while (position < source.size) {
      const count = Math.min(COPY_CHUNK, source.size - position);
      const bytes = await source.read(position, count);
      if (bytes.length !== count) throw new Error(`Could not read ${source.name} (is the file complete?).`);
      let written = 0;
      while (written < count) {
        const result = access.write(bytes.subarray(written), { at: position + written });
        if (!result) throw new Error(`Could not write ${source.name}: is the device full?`);
        written += result;
      }
      position += count;
      onChunk(count);
    }
    access.flush();
  } finally {
    access.close();
  }
}

async function copy(files) {
  const total = files.reduce((sum, entry) => sum + entry.size, 0);
  const storage = await navigator.storage.getDirectory();
  if (navigator.storage.estimate) {
    const { quota, usage } = await navigator.storage.estimate();
    if (quota && quota - usage < total) {
      throw new Error(`The browser lets this site keep ${Math.floor((quota - usage) / 1e6)} MB more, ` +
        `and the maps take ${Math.ceil(total / 1e6)} MB.`);
    }
  }
  // a new copy replaces whatever an earlier, perhaps interrupted, one left
  await storage.removeEntry('maps', { recursive: true }).catch(() => {});
  const directory = await storage.getDirectoryHandle('maps', { create: true });
  let done = 0;
  let lastReport = 0;
  for (const entry of files) {
    await writeFile(directory, entry, (count) => {
      done += count;
      const now = Date.now();
      if (now - lastReport > 100 || done === total) {
        lastReport = now;
        postMessage({ type: 'progress', file: entry.name, done, total });
      }
    });
  }
  const marker = await directory.getFileHandle(COMPLETE_MARKER, { create: true });
  const access = await marker.createSyncAccessHandle();
  access.truncate(0);
  access.write(new TextEncoder().encode(JSON.stringify({ files: files.map((f) => f.name), bytes: total })), { at: 0 });
  access.flush();
  access.close();
  return { files: files.length, bytes: total };
}

if (typeof onmessage !== 'undefined' || typeof WorkerGlobalScope !== 'undefined') {
  onmessage = async (event) => {
    try {
      const files = event.data.type === 'folder' ? await folderFiles(event.data.files) : await imageFiles(event.data.file);
      postMessage({ type: 'done', ...(await copy(files)) });
    } catch (error) {
      postMessage({ type: 'error', message: error && error.message ? error.message : String(error) });
    }
  };
}

// (for the tests: port/web/tests)
if (typeof module !== 'undefined') {
  module.exports = { findVolume, walkDirectory, mapProblem, imageFiles, folderFiles, plainName, loadingPicture };
}
