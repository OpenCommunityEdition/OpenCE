/*
MODS-WORKER.JS

Copies a mod into the site's Origin Private File System, where the game
finds it (/data/mods/<its folder>: port/linux/src/port_mods.c, docs/mods.md),
from the folder the player chose: its mod.toml and its tags folder of Halo PC
tag files (the rest of the folder is not the game's). It runs in a Worker,
which can write with synchronous OPFS access handles, and replaces a mod of
the same folder's name. The folder is the player's, so its paths are
checked: no part of one empty, hidden, a parent's or with a separator in it,
and no more files than the game lists; a folder without mod.toml at its top
is not a mod.

Messages from the page: { type: 'add', files } (the chosen folder's File
objects, each with its webkitRelativePath) or { type: 'remove', name }. To
the page: { type: 'progress', file, done, total }, then { type: 'done',
name, files, bytes } or { type: 'error', message }.
*/

'use strict';

// (port_mods.c's MAXIMUM_MOD_FOLDER_LENGTH and MAXIMUM_MOD_FILES)
const MAXIMUM_FOLDER_LENGTH = 63;
const MAXIMUM_FILES = 262144;
const MAXIMUM_DEPTH = 24;
const COPY_CHUNK = 4 * 1024 * 1024;

// a part of a path a mod's file may have: a name of a folder or file, not
// hidden (as port_mods.c skips those), no separator
function plainPart(part) {
  return part.length > 0 && part.length <= 255 && !part.startsWith('.') && !/[\\/:\u0000-\u001f]/.test(part);
}

// a mod's folder name, as load_order.toml names it (its length in UTF-8's
// bytes, as port_mods.c counts it)
function plainFolder(name) {
  return plainPart(name) && new TextEncoder().encode(name).length <= MAXIMUM_FOLDER_LENGTH;
}

// The mod in a chosen folder's files: { name, files: [{ parts, file }] }
// of its mod.toml and its tags folder's files; throws why it is not one.
function modFiles(list) {
  let name = null;
  const files = [];
  let toml = false;
  for (const file of list) {
    const parts = (file.webkitRelativePath || file.name).split('/');
    if (!parts.every(plainPart)) continue;
    if (name === null) name = parts[0];
    if (parts[0] !== name) throw new Error('Choose one mod\'s folder.');
    if (parts.length === 2 && parts[1] === 'mod.toml') {
      toml = true;
      files.push({ parts: parts.slice(1), file });
    } else if (parts.length >= 3 && parts.length <= MAXIMUM_DEPTH + 3 && parts[1] === 'tags') {
      files.push({ parts: parts.slice(1), file });
    }
  }
  if (name === null) throw new Error('The folder is empty.');
  if (!plainFolder(name)) throw new Error(`"${name}" cannot be a mod's folder name: it must not be hidden, and be ${MAXIMUM_FOLDER_LENGTH} bytes at most.`);
  if (!toml) throw new Error(`${name} has no mod.toml: it is not a mod.`);
  if (files.length > MAXIMUM_FILES) throw new Error(`${name} has more files than the game reads.`);
  return { name, files };
}

async function directoryOf(root, parts) {
  let directory = root;
  for (const part of parts) directory = await directory.getDirectoryHandle(part, { create: true });
  return directory;
}

async function writeFile(directory, name, file, onChunk) {
  const handle = await directory.getFileHandle(name, { create: true });
  const access = await handle.createSyncAccessHandle();
  try {
    access.truncate(0);
    let position = 0;
    while (position < file.size) {
      const count = Math.min(COPY_CHUNK, file.size - position);
      const bytes = new Uint8Array(await file.slice(position, position + count).arrayBuffer());
      if (bytes.length !== count) throw new Error(`Could not read ${file.name}.`);
      let written = 0;
      while (written < count) {
        const result = access.write(bytes.subarray(written), { at: position + written });
        if (!result) throw new Error(`Could not write ${file.name}: is the device full?`);
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

async function add(list) {
  const mod = modFiles(list);
  const total = mod.files.reduce((sum, entry) => sum + entry.file.size, 0);
  const storage = await navigator.storage.getDirectory();
  if (navigator.storage.estimate) {
    const { quota, usage } = await navigator.storage.estimate();
    if (quota && quota - usage < total) {
      throw new Error(`The browser lets this site keep ${Math.floor((quota - usage) / 1e6)} MB more, ` +
        `and the mod takes ${Math.ceil(total / 1e6)} MB.`);
    }
  }
  const mods = await storage.getDirectoryHandle('mods', { create: true });
  // (a mod of the same name is replaced whole)
  await mods.removeEntry(mod.name, { recursive: true }).catch(() => {});
  const root = await mods.getDirectoryHandle(mod.name, { create: true });
  let done = 0;
  let lastReport = 0;
  try {
    for (const entry of mod.files) {
      const directory = await directoryOf(root, entry.parts.slice(0, -1));
      await writeFile(directory, entry.parts[entry.parts.length - 1], entry.file, (count) => {
        done += count;
        const now = Date.now();
        if (now - lastReport > 100 || done === total) {
          lastReport = now;
          postMessage({ type: 'progress', file: entry.parts.join('/'), done, total });
        }
      });
    }
  } catch (error) {
    // (no half of a mod: the game would play what was copied)
    await mods.removeEntry(mod.name, { recursive: true }).catch(() => {});
    throw error;
  }
  return { name: mod.name, files: mod.files.length, bytes: total };
}

async function remove(name) {
  if (!plainFolder(name)) throw new Error('No such mod.');
  const storage = await navigator.storage.getDirectory();
  const mods = await storage.getDirectoryHandle('mods', { create: true });
  await mods.removeEntry(name, { recursive: true });
  return { name, files: 0, bytes: 0 };
}

if (typeof onmessage !== 'undefined' || typeof WorkerGlobalScope !== 'undefined') {
  onmessage = async (event) => {
    try {
      const result = event.data.type === 'remove' ? await remove(event.data.name) : await add(event.data.files);
      postMessage({ type: 'done', ...result });
    } catch (error) {
      postMessage({ type: 'error', message: error && error.message ? error.message : String(error) });
    }
  };
}

// (for the tests: port/web/tests)
if (typeof module !== 'undefined') {
  module.exports = { modFiles, plainPart, plainFolder };
}
