/* Halo 1 MCC packages imported independently of Xbox and CE. Schema and
 * bounds follow MccPackageImporter/MccJson on Android and the native MCC
 * metadata readers. Original JSON bytes remain the content identity. */
'use strict';
if (typeof importScripts === 'function') importScripts('map-import-files.js', 'map-import-storage.js');

(function (root) {
  const files = root.HaloImportFiles || require('./map-import-files.js');
  const JSON_LIMIT = 256 * 1024, PNG_LIMIT = 8 * 1024 * 1024;
  class JsonNumber { constructor(text) { this.text = text; } }

  // JSON.parse alone loses long Workshop identifiers and duplicate keys.
  function parseJson(text) {
    if (text.charCodeAt(0) === 0xfeff) text = text.slice(1);
    let at = 0, tokens = 0;
    function space() { while (at < text.length && /[ \t\r\n]/.test(text[at])) at++; }
    function string() {
      const start = at++;
      while (at < text.length) {
        const c = text[at++];
        if (c === '\\') { at++; continue; }
        if (c === '"') {
          const value = JSON.parse(text.slice(start, at));
          if (value.includes('\0')) throw new Error('NUL in metadata.');
          return value;
        }
      }
      throw new Error('Unterminated metadata string.');
    }
    function value(depth) {
      if (depth > 16 || ++tokens > 32768) throw new Error('Metadata nesting or token limit exceeded.');
      space();
      const c = text[at];
      if (c === '"') return string();
      if (c === '{' || c === '[') {
        at++;
        const object = c === '{', end = object ? '}' : ']';
        const result = object ? Object.create(null) : [];
        space();
        if (text[at] === end) { at++; return result; }
        for (;;) {
          space();
          if (object) {
            if (text[at] !== '"') throw new Error('Invalid metadata key.');
            if (++tokens > 32768) throw new Error('Metadata token limit exceeded.');
            const key = string(); space();
            if (Object.hasOwn(result, key) || text[at++] !== ':') throw new Error('Duplicate or invalid metadata key: ' + key);
            result[key] = value(depth + 1);
          } else result.push(value(depth + 1));
          space();
          if (text[at] === end) { at++; return result; }
          if (text[at++] !== ',') throw new Error('Invalid metadata separator.');
        }
      }
      const match = /^(?:true|false|null|-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)/.exec(text.slice(at));
      if (!match) throw new Error('Invalid metadata value.');
      at += match[0].length;
      return match[0] === 'true' ? true : match[0] === 'false' ? false : match[0] === 'null' ? null : new JsonNumber(match[0]);
    }
    const result = value(0); space();
    if (at !== text.length || !result || Array.isArray(result) || typeof result !== 'object' || result instanceof JsonNumber) {
      throw new Error('Metadata must contain one JSON object.');
    }
    return result;
  }

  async function json(file) {
    if (!file || file.size > JSON_LIMIT) throw new Error('Missing or oversized required JSON metadata.');
    return parseJson(new TextDecoder('utf-8', { fatal: true }).decode(await files.bytes(file, 0, file.size)));
  }

  function packageName(value) {
    const name = String(value || '').trim().toLowerCase();
    if (!/^[a-z0-9][a-z0-9 _-]{0,39}$/.test(name) || /^dl_[0-9a-f]{20}$/.test(name)) {
      throw new Error('Choose a package name of 1–40 letters, digits, spaces, underscores or hyphens. Download names are reserved.');
    }
    files.path(name);
    return name;
  }

  function crc(bytes) {
    let value = 0xffffffff;
    for (const byte of bytes) {
      value ^= byte;
      for (let bit = 0; bit < 8; bit++) value = value & 1 ? 0xedb88320 ^ (value >>> 1) : value >>> 1;
    }
    return (value ^ 0xffffffff) >>> 0;
  }

  // Optional previews must fit the native decoder's supported PNG subset.
  // Scan the inflated row bounds/filters without allocating an image canvas.
  async function png(file) {
    if (!file || file.size < 57 || file.size > PNG_LIMIT) return false;
    try {
      const b = await files.bytes(file, 0, file.size), h = new DataView(b.buffer);
      if (!b.subarray(0, 8).every((v, i) => v === [137,80,78,71,13,10,26,10][i])) return false;
      let at = 8, row = 0, height = 0, ended = false, idatEnded = false;
      const compressed = [];
      while (at + 12 <= b.length) {
        const length = h.getUint32(at), type = String.fromCharCode(...b.subarray(at + 4, at + 8));
        if (length > b.length - at - 12 || crc(b.subarray(at + 4, at + 8 + length)) !== h.getUint32(at + 8 + length)) return false;
        if (at === 8) {
          if (type !== 'IHDR' || length !== 13) return false;
          const width = h.getUint32(at + 8); height = h.getUint32(at + 12);
          if (!width || width > 2048 || !height || height > 2048 || b[at + 16] !== 8 ||
              ![2,6].includes(b[at + 17]) || b[at + 18] || b[at + 19] || b[at + 20]) return false;
          row = width * (b[at + 17] === 2 ? 3 : 4) + 1;
        } else if (type === 'IDAT') {
          if (idatEnded) return false;
          compressed.push(b.subarray(at + 8, at + 8 + length));
        } else if (type === 'IEND') {
          if (length || at + 12 !== b.length) return false;
          ended = true; break;
        } else {
          if (compressed.length) idatEnded = true;
          if (!(b[at + 4] & 32)) return false;
        }
        at += length + 12;
      }
      if (!ended || !compressed.length || typeof DecompressionStream === 'undefined') return false;
      const reader = new Blob(compressed).stream().pipeThrough(new DecompressionStream('deflate')).getReader();
      let offset = 0;
      try {
        for (;;) {
          const chunk = await reader.read();
          if (chunk.done) break;
          if (chunk.value.length > row * height - offset) { await reader.cancel(); return false; }
          for (let i = 0; i < chunk.value.length; i++) if ((offset + i) % row === 0 && chunk.value[i] > 4) { await reader.cancel(); return false; }
          offset += chunk.value.length;
        }
      } finally { reader.releaseLock(); }
      return offset === row * height;
    } catch { return false; }
  }

  async function plan(list, options = {}) {
    const inputs = files.chosen(list, name => /\.(map|json|png)$/i.test(name), name => /\.png$/i.test(name));
    if (!inputs.length) throw new Error('The folder contains no MCC package files.');
    const manifests = inputs.filter(e => /(^|\/)modinfo\.json$/.test(e.path));
    if (manifests.length > 1) throw new Error('Choose one MCC package at a time.');
    let prefix;
    if (manifests.length) prefix = manifests[0].path.slice(0, -'modinfo.json'.length);
    else {
      const roots = new Set(inputs.map(e => e.path.split('/')[0]));
      prefix = inputs.every(e => e.file.webkitRelativePath) && roots.size === 1 ? [...roots][0] + '/' : '';
    }
    const available = new Map();
    for (const input of inputs) if (input.path.startsWith(prefix)) available.set(input.path.slice(prefix.length), input.file);
    const info = available.has('modinfo.json') ? await json(available.get('modinfo.json')) : null;
    if (info && info.Engine !== 'Halo1') throw new Error('Only Halo 1 MCC packages are supported.');
    if (!info && available.has('campaigninfo.json')) throw new Error('CampaignInfo.json requires ModInfo.json.');
    const rawId = info?.ModIdentifier?.HostedModIds?.SteamWorkshopId;
    const id = rawId instanceof JsonNumber ? rawId.text : typeof rawId === 'string' ? rawId : '';
    const workshopId = /^[0-9]{1,20}$/.test(id) ? id : '';
    const folder = packageName(options.name || workshopId || prefix.split('/').filter(Boolean).pop());
    const output = new Map(), maps = new Map();
    function add(path, file) {
      path = files.path(path, 128);
      if (output.has(path)) throw new Error('Conflicting flattened package files: ' + path);
      output.set(path, files.entry(path, file));
    }
    for (const [relative, file] of available) if (relative.endsWith('.map')) {
      const name = relative.replace(/^(?:halo1\/maps\/|maps\/)/, '');
      if (name.includes('/') || !/^[a-z0-9][a-z0-9 _-]*\.map$/.test(name)) throw new Error('Unsupported nested map path: ' + relative);
      if (['bitmaps.map', 'sounds.map', 'loc.map'].includes(name)) {
        files.resourceHeader(await files.bytes(file, 0, 16), file.size, { 'bitmaps.map': 1, 'sounds.map': 2, 'loc.map': 3 }[name]);
      } else {
        const leaf = name.slice(0, -4);
        if (9 + folder.length + 1 + leaf.length > 63) throw new Error('Unlaunchable MCC map name: ' + name);
        maps.set(leaf, files.cacheHeader(await files.bytes(file, 0, 2048), file.size, 13, 0x4000000, files.MAP_LIMIT));
      }
      add(name, file);
    }
    if (!maps.size || maps.size > 1024) throw new Error('Package must contain 1–1024 supported MCC maps.');
    const previews = new Set(), records = new Set();
    function images(object) {
      for (const field of ['Thumbnail', 'Large']) {
        try {
          const path = files.path(object?.Images?.[field], 128);
          if (path.endsWith('.png')) previews.add(path);
        } catch { /* Unsupported optional paths retain the game's placeholder. */ }
      }
    }
    async function record(relative, campaign) {
      relative = files.path(relative, 128);
      if (!relative.endsWith('.json') || records.has(relative)) throw new Error('Invalid or duplicate map metadata reference.');
      records.add(relative);
      const file = available.get(relative), metadata = await json(file);
      const scenario = files.path(metadata.ScenarioFile, 256).split('/').pop().replace(/\.map$/, '');
      if (maps.get(scenario) !== (campaign ? 0 : 1)) throw new Error('Missing map or wrong mission type: ' + scenario);
      if (campaign && Object.hasOwn(metadata, 'CampaignMapKind') &&
          (typeof metadata.CampaignMapKind !== 'string' || metadata.CampaignMapKind.toLowerCase() !== 'default')) throw new Error('Unsupported campaign mission kind.');
      add(relative, file); images(metadata);
      return scenario;
    }
    if (info) {
      add('modinfo.json', available.get('modinfo.json')); images(info);
      const content = info.GameModContents;
      if (Object.hasOwn(info, 'GameModContents') && (!content || typeof content !== 'object' ||
          Array.isArray(content) || content instanceof JsonNumber)) throw new Error('Invalid GameModContents object.');
      if (content && Object.hasOwn(content, 'HasCampaign') && typeof content.HasCampaign !== 'boolean') {
        throw new Error('Invalid HasCampaign flag.');
      }
      if (content?.HasCampaign === true) {
        const campaign = await json(available.get('campaigninfo.json'));
        if (!Array.isArray(campaign.CampaignMaps) || !campaign.CampaignMaps.length || campaign.CampaignMaps.length > 64) throw new Error('Campaign must declare 1–64 missions.');
        add('campaigninfo.json', available.get('campaigninfo.json')); images(campaign);
        const seen = new Set();
        for (const relative of campaign.CampaignMaps) {
          const scenario = await record(relative, true);
          if (seen.has(scenario)) throw new Error('Duplicate campaign mission: ' + scenario);
          seen.add(scenario);
        }
      } else if (available.has('campaigninfo.json')) throw new Error('Campaign metadata does not declare HasCampaign.');
      if (content && Object.hasOwn(content, 'MultiplayerMaps')) {
        if (!Array.isArray(content.MultiplayerMaps) || content.MultiplayerMaps.length > 1024) throw new Error('Invalid MultiplayerMaps list.');
        for (const relative of content.MultiplayerMaps) await record(relative, false);
      }
    }
    for (const path of previews) if (await png(available.get(path))) add(path, available.get(path));
    const title = typeof info?.Title?.Neutral === 'string' ? info.Title.Neutral : folder;
    if (workshopId) add('workshop_id.txt', new Blob([workshopId + '\n' + title.replace(/[^ -~]/g, '').slice(0, 40) + '\n']));
    return { family: 'mcc', folder, title, entries: [...output.values()] };
  }

  root.HaloMccImport = { plan, parseJson, png, packageName };
  if (typeof module !== 'undefined') module.exports = root.HaloMccImport;
  if (typeof WorkerGlobalScope !== 'undefined' && root instanceof WorkerGlobalScope) root.HaloMapStorage.worker(plan);
})(globalThis);
