/* Bounded browser file reads and portable names for the separate CE/MCC
 * importers. Format policy stays in its owning importer; no Xbox import use. */
'use strict';

(function (root) {
  const MAX_FILES = 4096;
  const MAX_TOTAL = 16 * 1024 * 1024 * 1024;
  const MAP_LIMIT = 0x7fffffff;

  function path(value, limit = 512) {
    if (typeof value !== 'string' || !value.length || value.length >= limit) throw new Error('Invalid import path.');
    const parts = value.replace(/\\/g, '/').split('/');
    if (parts.length > 8) throw new Error('Import path is too deep.');
    for (const part of parts) {
      if (!part || /^[. ]|[. ]$/.test(part) || /[^\x20-\x7e]|[:*?"<>|]/.test(part) ||
          /^(con|prn|aux|nul|conin\$|conout\$|com[0-9]|lpt[0-9])(?:[ .]|$)/i.test(part)) {
        throw new Error('Unsafe import path: ' + value);
      }
    }
    return parts.join('/').toLowerCase();
  }

  function chosen(list, accept = () => true, optional = () => false) {
    if (!Array.isArray(list) || !list.length || list.length > MAX_FILES) throw new Error('Choose 1–4096 files.');
    let total = 0;
    const seen = new Set();
    return list.map(file => {
      if (!Number.isSafeInteger(file.size) || file.size < 0 || file.size > MAP_LIMIT) throw new Error('File exceeds the import size limit.');
      total += file.size;
      if (total > MAX_TOTAL) throw new Error('The selection exceeds 16 GiB.');
      const source = file.webkitRelativePath || file.name;
      if (!accept(source)) return null;
      let relative;
      try { relative = path(source); }
      catch (error) { if (optional(source)) return null; throw error; }
      if (seen.has(relative)) throw new Error('Duplicate or case-conflicting file: ' + relative);
      seen.add(relative);
      return { path: relative, file };
    }).filter(Boolean);
  }

  async function bytes(file, offset, count) {
    if (!Number.isSafeInteger(offset) || !Number.isSafeInteger(count) || offset < 0 || count < 0 || offset + count > file.size) {
      throw new Error('Truncated file: ' + file.name);
    }
    const data = new Uint8Array(await file.slice(offset, offset + count).arrayBuffer());
    if (data.length !== count) throw new Error('Could not read ' + file.name);
    return data;
  }

  function cacheHeader(data, size, version, tagLimit, fileLimit) {
    if (data.length !== 2048) throw new Error('Truncated map header.');
    const h = new DataView(data.buffer, data.byteOffset, data.byteLength);
    const length = h.getUint32(8, true) || size;
    const offset = h.getUint32(16, true), tags = h.getUint32(20, true), type = h.getUint16(0x60, true);
    if (h.getUint32(0, true) !== 0x68656164 || h.getUint32(0x7fc, true) !== 0x666f6f74 ||
        h.getUint32(4, true) !== version || type > 1 ||
        !data.subarray(0x20, 0x40).includes(0) || !data.subarray(0x40, 0x60).includes(0) ||
        size > fileLimit || length < 2048 || length > size || offset < 2048 ||
        tags < 40 || tags > tagLimit || offset > length || tags > length - offset) {
      throw new Error('Unsupported or malformed map header.');
    }
    return type;
  }

  function resourceHeader(data, size, expected) {
    if (data.length < 16) throw new Error('Truncated resource map.');
    const h = new DataView(data.buffer, data.byteOffset, data.byteLength);
    const names = h.getUint32(4, true), index = h.getUint32(8, true), count = h.getInt32(12, true);
    if (h.getUint32(0, true) !== expected || names < 16 || index < names || index > size ||
        count < 0 || count > Math.floor((size - index) / 12)) throw new Error('Wrong or malformed resource map.');
  }

  function entry(relative, file) { return { path: relative, size: file.size, file }; }
  const api = { MAX_FILES, MAX_TOTAL, MAP_LIMIT, path, chosen, bytes, cacheHeader, resourceHeader, entry };
  root.HaloImportFiles = api;
  if (typeof module !== 'undefined') module.exports = api;
})(globalThis);
