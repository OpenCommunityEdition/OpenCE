/* Custom Edition browser imports retain the existing flat custom_maps layout.
 * Header checks follow cache_file_formats.c; the engine remains responsible
 * for validating tags and dependencies before a map can play. */
'use strict';
if (typeof importScripts === 'function') importScripts('map-import-files.js', 'map-import-storage.js');

(function (root) {
  const files = root.HaloImportFiles || require('./map-import-files.js');
  const resources = { 'bitmaps.map': 1, 'sounds.map': 2, 'loc.map': 3 };

  async function plan(list) {
    const entries = [], names = new Set();
    for (const input of files.chosen(list, name => /\.map$/i.test(name))) {
      const name = input.path.split('/').pop();
      if (!name.endsWith('.map')) continue;
      if (!/^[a-z0-9][a-z0-9 _-]*\.map$/.test(name) || name.length - 4 + 'custom_maps/'.length > 63) throw new Error('Unlaunchable CE map name: ' + name);
      if (names.has(name)) throw new Error('Conflicting CE filenames: ' + name);
      names.add(name);
      if (Object.hasOwn(resources, name)) {
        files.resourceHeader(await files.bytes(input.file, 0, 16), input.file.size, resources[name]);
      } else {
        const header = await files.bytes(input.file, 0, 2048);
        files.cacheHeader(header, input.file.size, 609, 0x01700000, 0x30000000);
        const h = new DataView(header.buffer, header.byteOffset, header.byteLength);
        if (h.getUint32(12, true)) throw new Error('Compressed CE maps are unsupported: ' + name);
        if (h.getUint32(0x70, true) === 0x79656c6f && h.getUint16(0x76, true)) {
          throw new Error('This map requires unsupported OpenSauce features: ' + name);
        }
      }
      entries.push(files.entry(name, input.file));
    }
    if (!entries.length) throw new Error('No Custom Edition maps or resources were selected.');
    return { family: 'ce', folder: '', title: 'Custom Edition', entries };
  }

  root.HaloCeImport = { plan };
  if (typeof module !== 'undefined') module.exports = { plan };
  if (typeof WorkerGlobalScope !== 'undefined' && root instanceof WorkerGlobalScope) {
    root.HaloMapStorage.worker(plan);
  }
})(globalThis);
