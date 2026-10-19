/*
XISO-WORKER.JS

Copies the game data into the site's Origin Private File System, where the
game reads it (/data/maps: port/web/src/web_main.c): the maps folder of the
player's Xbox disc image of Halo: Combat Evolved, or the files of a maps
folder the player chose. It runs in a Worker, which can write with
synchronous OPFS access handles (Safari has no OPFS writable streams). Only
the maps and loading.tga are kept, and each map's header is checked as the
game checks it, so that a disc the game cannot play is refused here.

Written from a description of the Xbox's disc file system, XDVDFS
(xboxdevwiki's XDVDFS page, https://xboxdevwiki.net/XDVDFS, and the format
facts OpenCE's readers rely on). What this file relies on:

- Sectors are 2048 bytes and integers little-endian. Sector numbers count
  from the start of the game partition: an image of the partition alone has
  it at 0, an image of a whole disc further in (0x0FD90000, 0x02080000 or
  0x18300000 for XGD2, XGD3 and XGD1 discs). They are tried in that order.
- The volume descriptor is the partition's sector 32 (at 0x10000). It holds
  "MICROSOFT*XBOX*MEDIA" at its start and again at 0x7EC, and the root
  directory table's start sector (u32 at 0x14) and size in bytes (u32 at
  0x18).
- A directory table is a binary tree of entries whose root is the entry at
  offset 0. An entry is a left and a right subtree link (u16 each, in 4-byte
  units from the table's start, 0 for none), the start sector and the size
  of the file or of the subdirectory's table (u32 each), the attributes (u8,
  0x10 for a directory), the name's length (u8) and the name. Unused bytes of
  a table are 0xFF, so a left link of 0xFFFF is padding, not an entry.
- A file is one contiguous run of bytes from the start of its sector.

The image is the player's file (4 to 7 GB) and nothing read from it is
trusted: it is read a slice at a time, only the root's table and the maps
folder's (each at most 1 MB), and the walk of a table visits each offset
once, goes at most 64 links deep and looks at most 4096 entries.

The loading screen's picture, which the retail disc has no file of, is
taken out of the image's default.xbe (read whole, at most 64 MB) and kept
with the maps as the loading.tga the game reads, when the maps folder has
none (loadingPicture; xiso.c's xiso_write_loading_picture does the same).
Without it the light sweeps over black, as the game's own does.

Messages from the page: { type: 'image', file } (the disc image) or
{ type: 'folder', files } (a chosen folder's File objects). To the page:
{ type: 'progress', file, done, total }, then { type: 'done', files, bytes }
or { type: 'error', message }. A new copy first removes the old maps folder
and writes maps/.complete ({"files":[...],"bytes":n}) last: the page takes
the data as ready only once that marker is there.
*/

'use strict';

const SECTOR_SIZE = 2048;
const DESCRIPTOR_OFFSET = 0x10000;
const MAGIC = 'MICROSOFT*XBOX*MEDIA';
const SECOND_MAGIC_OFFSET = 0x7EC;
const ENTRY_FIXED_SIZE = 14;
const ATTRIBUTE_DIRECTORY = 0x10;
const PADDING_LINK = 0xFFFF;
// where the game partition may start, in the order they are tried: an image
// of the partition alone, then whole XGD2, XGD3 and XGD1 discs
const PARTITION_STARTS = [0x00000000, 0x0FD90000, 0x02080000, 0x18300000];
// directory tables are a few sectors; anything much larger is not one
const MAXIMUM_TABLE_SIZE = 1024 * 1024;
const MAXIMUM_DEPTH = 64;
const MAXIMUM_ENTRIES = 4096;
const MAP_HEADER_SIZE = 0x800;
const MAP_BUILDS = ['01.10.12.2276', '01.01.14.2342'];
const COPY_CHUNK = 4 * 1024 * 1024;
// the loading screen's picture (source/interface/progress_bar.c), out of
// default.xbe; the executable is a few MB
const LOADING_WIDTH = 320;
const LOADING_HEIGHT = 240;
const TGA_HEADER_SIZE = 18;
const MAXIMUM_XBE_SIZE = 64 * 1024 * 1024;

// ---------- names

function asciiLower(text) {
  return text.replace(/[A-Z]/g, (letter) => String.fromCharCode(letter.charCodeAt(0) + 32));
}

function sameName(a, b) {
  return asciiLower(a) === asciiLower(b);
}

// A name a file of the maps folder may have: no path, nothing hidden.
function plainName(name) {
  return typeof name === 'string' && /^[A-Za-z0-9_. -]{1,64}$/.test(name) && !name.startsWith('.');
}

function latin1(bytes) {
  let text = '';
  for (const byte of bytes) text += String.fromCharCode(byte);
  return text;
}

// ---------- the image

async function readBytes(file, start, end) {
  return new Uint8Array(await file.slice(start, end).arrayBuffer());
}

// The game partition: { partition, rootSector, rootSize }, from the first
// start whose volume descriptor has both magics; null if none has.
async function findVolume(file) {
  for (const partition of PARTITION_STARTS) {
    const start = partition + DESCRIPTOR_OFFSET;
    if (start + SECTOR_SIZE > file.size) continue;
    const descriptor = await readBytes(file, start, start + SECTOR_SIZE);
    if (descriptor.length !== SECTOR_SIZE) continue;
    if (latin1(descriptor.subarray(0, MAGIC.length)) !== MAGIC ||
      latin1(descriptor.subarray(SECOND_MAGIC_OFFSET, SECOND_MAGIC_OFFSET + MAGIC.length)) !== MAGIC) {
      continue;
    }
    const view = new DataView(descriptor.buffer, descriptor.byteOffset, descriptor.byteLength);
    return { partition, rootSector: view.getUint32(0x14, true), rootSize: view.getUint32(0x18, true) };
  }
  return null;
}

// A directory table's entries, { name, sector, size, directory }, for every
// entry that has a name, in the tree's order (left subtree, the entry, right
// subtree). A link to padding, past the table or to an offset already
// visited ends that branch.
function walkDirectory(table) {
  const view = new DataView(table.buffer, table.byteOffset, table.byteLength);
  const visited = new Set();
  const entries = [];
  const walk = (offset, depth) => {
    if (depth > MAXIMUM_DEPTH || offset + ENTRY_FIXED_SIZE > table.length) return;
    const left = view.getUint16(offset, true);
    const right = view.getUint16(offset + 2, true);
    if (left === PADDING_LINK || visited.has(offset) || visited.size >= MAXIMUM_ENTRIES) return;
    visited.add(offset);
    if (left) walk(left * 4, depth + 1);
    const nameLength = table[offset + 13];
    const nameStart = offset + ENTRY_FIXED_SIZE;
    if (nameLength && nameStart + nameLength <= table.length) {
      entries.push({
        name: latin1(table.subarray(nameStart, nameStart + nameLength)),
        sector: view.getUint32(offset + 4, true),
        size: view.getUint32(offset + 8, true),
        directory: (table[offset + 12] & ATTRIBUTE_DIRECTORY) !== 0,
      });
    }
    if (right) walk(right * 4, depth + 1);
  };
  walk(0, 0);
  return entries;
}

async function readTable(file, partition, sector, size, message) {
  const start = partition + sector * SECTOR_SIZE;
  if (size === 0 || size > MAXIMUM_TABLE_SIZE || start + size > file.size) throw new Error(message);
  const table = await readBytes(file, start, start + size);
  if (table.length !== size) throw new Error(message);
  return table;
}

// ---------- the maps

// Why the game would not load a map whose first bytes are header, or null.
function mapProblem(header) {
  if (header.length < MAP_HEADER_SIZE) return 'it is too short';
  if (latin1(header.subarray(0, 4)) !== 'daeh' || latin1(header.subarray(0x7FC, 0x800)) !== 'toof') {
    return 'it is not a map';
  }
  const view = new DataView(header.buffer, header.byteOffset, header.byteLength);
  const version = view.getUint32(4, true);
  if (version !== 5) return `it is not an Xbox map (its cache version is ${version})`;
  const field = header.subarray(0x40, 0x60);
  const end = field.indexOf(0);
  const build = latin1(end >= 0 ? field.subarray(0, end) : field);
  if (!MAP_BUILDS.includes(build)) return `its build, ${JSON.stringify(build)}, is not one the game loads`;
  return null;
}

// The files to copy, of those given ({ name, size, read }): the maps (each
// checked) and loading.tga, under plain names, the first of names that
// differ only in case; throws why they cannot be used.
async function checkedFiles(candidates) {
  const kept = [];
  for (const candidate of candidates) {
    if (!plainName(candidate.name)) continue;
    if (kept.some((file) => sameName(file.name, candidate.name))) continue;
    const lower = asciiLower(candidate.name);
    if (lower.endsWith('.map')) {
      const problem = mapProblem(await candidate.read(0, MAP_HEADER_SIZE));
      if (problem) throw new Error(`${candidate.name} cannot be used: ${problem}.`);
      kept.push(candidate);
    } else if (lower === 'loading.tga') {
      kept.push(candidate);
    }
  }
  if (!kept.some((file) => sameName(file.name, 'ui.map'))) {
    throw new Error('There is no ui.map among the maps: this is not Halo: Combat Evolved for the Xbox.');
  }
  return kept;
}

// The maps folder among the root's folders: "maps", or on a multi-language
// European disc, which has a "maps_xx" for each language instead, the one
// for the browser's language, else English's, else the first.
function mapsFolder(directories, language) {
  const plain = directories.find((entry) => sameName(entry.name, 'maps'));
  if (plain) return plain;
  const languages = directories.filter((entry) => entry.name.length === 7 && sameName(entry.name.slice(0, 5), 'maps_'));
  const wanted = 'maps_' + asciiLower(language).slice(0, 2);
  return languages.find((entry) => sameName(entry.name, wanted)) ||
    languages.find((entry) => sameName(entry.name, 'maps_en')) || languages[0];
}

// The maps folder's files in a disc image, as { name, size, read(offset,
// size) }; throws why it cannot be used.
async function imageFiles(file) {
  const volume = await findVolume(file);
  if (!volume) throw new Error('This file is not an Xbox disc image (no XDVDFS volume was found).');
  const { partition } = volume;
  const root = await readTable(file, partition, volume.rootSector, volume.rootSize,
    'The disc image\'s file system is damaged.');
  const rootEntries = walkDirectory(root);
  const maps = mapsFolder(rootEntries.filter((entry) => entry.directory),
    (typeof navigator !== 'undefined' && navigator.language) || '');
  if (!maps) throw new Error('The disc image has no maps folder: it is not a Halo disc.');
  const table = await readTable(file, partition, maps.sector, maps.size, 'The disc image\'s maps folder is damaged.');
  const candidates = [];
  for (const entry of walkDirectory(table)) {
    if (entry.directory) continue;
    const start = partition + entry.sector * SECTOR_SIZE;
    // (an empty file's sector need not be anywhere)
    if (entry.size && start + entry.size > file.size) {
      throw new Error(`The disc image ends before ${entry.name} does: is it complete?`);
    }
    candidates.push({
      name: entry.name,
      size: entry.size,
      read: (offset, size) => {
        const count = Math.max(0, Math.min(size, entry.size - offset));
        return readBytes(file, Math.min(start + offset, file.size), Math.min(start + offset + count, file.size));
      },
    });
  }
  const files = await checkedFiles(candidates);
  if (!files.some((entry) => sameName(entry.name, 'loading.tga'))) {
    const picture = await loadingPictureFile(file, partition, rootEntries);
    if (picture) files.push(picture);
  }
  return files;
}

// The same, from the files of a chosen folder.
async function folderFiles(list) {
  return checkedFiles(list.map((file) => ({
    name: file.name,
    size: file.size,
    read: (offset, size) => readBytes(file, offset, offset + size),
  })));
}

// ---------- the loading screen's picture

// The loading screen's picture out of a default.xbe, as the TGA the game
// reads (uncompressed, 24 bits, rows from the bottom), or null. The
// executable holds it, 320 by 240, as runs of 4-bit greys (a byte each: the
// grey's high four bits in its low nibble, the run's length in its high one)
// after the width and height as u32s: found by those two numbers followed by
// runs that fill the picture exactly (a run of none, or one past the
// picture's end, is not it).
function loadingPicture(xbe) {
  const pixels = LOADING_WIDTH * LOADING_HEIGHT;
  const greys = new Uint8Array(pixels);
  const view = new DataView(xbe.buffer, xbe.byteOffset, xbe.byteLength);
  let found = false;
  for (let at = 0; at + 8 <= xbe.length && !found; at++) {
    if (view.getUint32(at, true) !== LOADING_WIDTH || view.getUint32(at + 4, true) !== LOADING_HEIGHT) continue;
    let pixel = 0;
    for (let next = at + 8; next < xbe.length && pixel < pixels; next++) {
      const run = xbe[next] >> 4;
      if (!run || run > pixels - pixel) break;
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
    const row = (LOADING_HEIGHT - 1 - y) * LOADING_WIDTH;
    for (let x = 0; x < LOADING_WIDTH; x++) {
      const out = TGA_HEADER_SIZE + (y * LOADING_WIDTH + x) * 3;
      tga[out] = tga[out + 1] = tga[out + 2] = greys[row + x];
    }
  }
  return tga;
}

// The root's default.xbe's picture, as a file to copy ({ name, size,
// read }), or null.
async function loadingPictureFile(file, partition, rootEntries) {
  const xbe = rootEntries.find((entry) => !entry.directory && sameName(entry.name, 'default.xbe'));
  if (!xbe || !xbe.size || xbe.size > MAXIMUM_XBE_SIZE) return null;
  const start = partition + xbe.sector * SECTOR_SIZE;
  if (start + xbe.size > file.size) return null;
  const bytes = await readBytes(file, start, start + xbe.size);
  if (bytes.length !== xbe.size) return null;
  const tga = loadingPicture(bytes);
  return tga && { name: 'loading.tga', size: tga.length, read: async (offset, size) => tga.subarray(offset, offset + size) };
}

// ---------- the copy

function writeAll(access, bytes, position, name) {
  let written = 0;
  while (written < bytes.length) {
    const result = access.write(bytes.subarray(written), { at: position + written });
    if (!result) throw new Error(`Could not write ${name}: is the device full?`);
    written += result;
  }
}

async function copy(files) {
  const total = files.reduce((sum, file) => sum + file.size, 0);
  if (navigator.storage.estimate) {
    const { quota, usage } = await navigator.storage.estimate();
    if (quota && quota - usage < total) {
      throw new Error(`The browser lets this site keep ${Math.floor((quota - usage) / 1e6)} MB more, ` +
        `and the maps take ${Math.ceil(total / 1e6)} MB.`);
    }
  }
  const storage = await navigator.storage.getDirectory();
  // (a new copy replaces whatever an earlier, perhaps interrupted, one left)
  await storage.removeEntry('maps', { recursive: true }).catch(() => {});
  const maps = await storage.getDirectoryHandle('maps', { create: true });
  let done = 0;
  let lastReport = 0;
  for (const file of files) {
    const handle = await maps.getFileHandle(file.name, { create: true });
    const access = await handle.createSyncAccessHandle();
    try {
      access.truncate(0);
      let position = 0;
      while (position < file.size) {
        const count = Math.min(COPY_CHUNK, file.size - position);
        const bytes = await file.read(position, count);
        if (bytes.length < count) throw new Error(`Could not read ${file.name} (is the file complete?).`);
        writeAll(access, bytes.subarray(0, count), position, file.name);
        position += count;
        done += count;
        const now = Date.now();
        if (now - lastReport > 100 || done === total) {
          lastReport = now;
          postMessage({ type: 'progress', file: file.name, done, total });
        }
      }
      access.flush();
    } finally {
      access.close();
    }
  }
  // last: the sign that the copy is complete
  const marker = await (await maps.getFileHandle('.complete', { create: true })).createSyncAccessHandle();
  try {
    marker.truncate(0);
    const text = new TextEncoder().encode(JSON.stringify({ files: files.map((file) => file.name), bytes: total }));
    writeAll(marker, text, 0, '.complete');
    marker.flush();
  } finally {
    marker.close();
  }
  return { files: files.length, bytes: total };
}

if (typeof onmessage !== 'undefined' || typeof WorkerGlobalScope !== 'undefined') {
  onmessage = async (event) => {
    try {
      const message = event.data;
      const files = message.type === 'folder' ? await folderFiles(message.files) : await imageFiles(message.file);
      postMessage({ type: 'done', ...(await copy(files)) });
    } catch (error) {
      postMessage({ type: 'error', message: error && error.message ? error.message : String(error) });
    }
  };
}

// (for the tests: port/web/tests)
if (typeof module !== 'undefined') {
  module.exports = { findVolume, walkDirectory, mapProblem, imageFiles, folderFiles, plainName, mapsFolder, loadingPicture };
}
