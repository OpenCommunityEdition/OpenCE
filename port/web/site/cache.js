/* The source-built launcher shares the production launcher's OPFS map cache.
   Downloads run in xiso-worker.js so Safari can use synchronous OPFS writes. */
'use strict';

globalThis.HaloCache = (() => {
  const BASE = 'https://raw.githubusercontent.com/fqlx/halo-ce-universal/fc2a0301d8ddd48f8585d3dd53a680964bde0f63/';
  const EXPECTED = [
    'ui', 'a10', 'a30', 'a50', 'b30', 'b40', 'c10', 'c20', 'c40', 'd20', 'd40',
    'beavercreek', 'bloodgulch', 'boardingaction', 'carousel', 'chillout', 'damnation',
    'hangemhigh', 'longest', 'prisoner', 'putput', 'ratrace', 'sidewinder', 'wizard',
  ].map(name => name + '.map');
  const CACHE_BYTES = 2 * 0x11600000 + 0x02300000 + 3 * 0x02f00000 + (64 << 20);
  const DATA_PATH = ['halo', 'data'];

  async function directory(path, create = false) {
    let folder = await navigator.storage.getDirectory();
    for (const name of path) folder = await folder.getDirectoryHandle(name, { create });
    return folder;
  }

  async function readJSON(path, name) {
    try {
      return JSON.parse(await (await (await (await directory(path)).getFileHandle(name)).getFile()).text());
    } catch (error) {
      if (error.name === 'NotFoundError' || error instanceof SyntaxError) return null;
      throw error;
    }
  }

  async function headerValid(file) {
    if (file.size < 2048) return false;
    const data = new Uint8Array(await file.slice(0, 2048).arrayBuffer());
    const text = offset => String.fromCharCode(...data.subarray(offset, offset + 4));
    return text(0) === 'daeh' && text(2044) === 'toof';
  }

  async function inspect(path, manifest = null) {
    const maps = new Map();
    let folder;
    try { folder = await directory(path); }
    catch (error) { if (error.name === 'NotFoundError') return maps; throw error; }
    for (const name of EXPECTED) {
      try {
        const file = await (await folder.getFileHandle(name)).getFile();
        if ((!manifest || manifest[name] === file.size) && await headerValid(file)) maps.set(name, file.size);
      } catch (error) {
        if (error.name !== 'NotFoundError') throw error;
      }
    }
    return maps;
  }

  function mapState(maps, legacy) {
    if (maps.size !== EXPECTED.length) return null;
    return {
      files: [...maps.keys()], bytes: [...maps.values()].reduce((sum, size) => sum + size, 0),
      dataRoot: legacy ? '/data/halo/data' : '/data',
      saveRoot: legacy ? '/data/halo/save' : '/data/save', cached: true,
    };
  }

  async function mapsState() {
    // The production launcher wrote maps.json only after each copy completed.
    // Its oldest imports predate that marker and were checked by map header.
    const legacy = mapState(await inspect([...DATA_PATH, 'maps'], await readJSON(DATA_PATH, 'maps.json')), true);
    if (legacy) return legacy;
    const marker = await readJSON(['maps'], '.complete');
    if (!marker || !Array.isArray(marker.files) || !EXPECTED.every(name => marker.files.includes(name))) return null;
    const native = mapState(await inspect(['maps']), false);
    return native && native.bytes === marker.bytes ? native : null;
  }

  async function withLock(task, signal) {
    signal?.throwIfAborted();
    if (!navigator.locks) return task();
    return navigator.locks.request('halo-game', { ifAvailable: true }, lock => {
      if (!lock) throw new Error('Halo is open in another tab. Close that tab, then retry.');
      return task();
    });
  }

  async function writeJSON(path, name, value) {
    const folder = await directory(path, true);
    const handle = await folder.getFileHandle(name, { create: true });
    const access = await handle.createSyncAccessHandle();
    try {
      const bytes = new TextEncoder().encode(JSON.stringify(value));
      access.truncate(0);
      let position = 0;
      while (position < bytes.length) {
        const written = access.write(bytes.subarray(position), { at: position });
        if (!written) throw new Error('Could not save the map manifest.');
        position += written;
      }
      access.flush();
    } finally { access.close(); }
  }

  async function folderBytes(path) {
    let bytes = 0;
    const walk = async folder => {
      for await (const [, handle] of folder.entries()) {
        if (handle.kind === 'file') bytes += (await handle.getFile()).size;
        else await walk(handle);
      }
    };
    try { await walk(await directory(path)); }
    catch (error) { if (error.name !== 'NotFoundError') throw error; }
    return bytes;
  }

  async function reserve(bytes) {
    if (bytes <= 0) return;
    const folder = await directory(['halo'], true);
    let access;
    try {
      access = await (await folder.getFileHandle('space-check', { create: true })).createSyncAccessHandle();
      access.truncate(bytes);
      access.flush();
    } catch (error) {
      if (error.name === 'QuotaExceededError') {
        throw new DOMException('Not enough browser storage. Free disk space and retry in a normal browser window.', 'QuotaExceededError');
      }
      throw error;
    } finally {
      if (access) access.close();
      await folder.removeEntry('space-check').catch(() => {});
    }
  }

  function validateManifest(manifest) {
    const names = new Set(EXPECTED.map(name => 'maps/' + name));
    if (manifest.version !== 2 || !Array.isArray(manifest.files) || manifest.files.length !== names.size) {
      throw new Error('Incomplete game download manifest');
    }
    for (const file of manifest.files) {
      if (!names.delete(file.name) || !Number.isSafeInteger(file.size) || file.size < 2048 || file.size > 512 * 1024 * 1024 ||
          !/^[a-f0-9]{64}$/.test(file.sha256) || !Array.isArray(file.chunks) || !file.chunks.length ||
          file.chunks.length > 32 || file.chunks.some(chunk =>
            !/^chunks\/[a-z0-9_]+\.map\.part[0-9]{3}$/.test(chunk.path) ||
            !Number.isSafeInteger(chunk.size) || chunk.size < 1 || chunk.size > 48 * 1024 * 1024) ||
          file.chunks.reduce((sum, chunk) => sum + chunk.size, 0) !== file.size) {
        throw new Error('Invalid game download manifest');
      }
    }
    return manifest.files;
  }

  function delay(milliseconds, signal) {
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => { signal.removeEventListener('abort', abort); resolve(); }, milliseconds);
      function abort() { clearTimeout(timer); reject(signal.reason); }
      signal.addEventListener('abort', abort, { once: true });
      if (signal.aborted) abort();
    });
  }

  async function withResponse(url, signal, use) {
    const abort = new AbortController();
    const stop = () => abort.abort(signal.reason);
    signal.addEventListener('abort', stop, { once: true });
    const timer = setTimeout(() => abort.abort(new Error('Download timed out')), 600000);
    try {
      signal.throwIfAborted();
      const response = await fetch(url, { signal: abort.signal, cache: 'no-store', credentials: 'omit' });
      if (!response.ok) throw new Error(`Download returned HTTP ${response.status}`);
      return await use(response);
    } finally {
      clearTimeout(timer);
      signal.removeEventListener('abort', stop);
    }
  }

  async function ensure({ signal = new AbortController().signal, onProgress = () => {} } = {}) {
    let done = 0, total = 0;
    const report = (state, title, detail, count = done) => onProgress({ state, title, detail, done: count, total,
      fraction: total ? Math.min(state === 'ready' ? 1 : 0.99, count / total) : 0 });
    const ready = state => {
      total = done = state.bytes;
      report('ready', 'Already downloaded', `${(total / 1e9).toFixed(2)} GB saved in this browser. Ready to play.`);
      return state;
    };
    report('checking', 'Checking game download', 'Looking for game data saved in this browser…');
    const existing = await mapsState();
    if (existing) return ready(existing); // Never request or copy an existing complete cache.
    const retry = async task => {
      for (let attempt = 0; ; attempt++) {
        signal.throwIfAborted();
        try { return await task(); }
        catch (error) {
          if (signal.aborted || attempt >= 2 || error.name === 'QuotaExceededError') throw error;
          report('downloading', 'Retrying download', `Retrying in ${(attempt + 1) * 3} seconds. Completed maps are safe.`);
          await delay((attempt + 1) * 3000, signal);
        }
      }
    };
    try {
      return await withLock(async () => {
        const state = await mapsState();
        if (state) return ready(state);
        const remote = await retry(() => withResponse(new URL('manifest.json', BASE), signal, response => response.json()));
        const files = validateManifest(remote);
        const manifest = await readJSON(DATA_PATH, 'maps.json') || Object.fromEntries(await inspect([...DATA_PATH, 'maps']));
        const stored = await inspect([...DATA_PATH, 'maps'], manifest);
        const wanted = files.filter(file => stored.get(file.name.slice(5)) !== file.size);
        total = files.reduce((sum, file) => sum + file.size, 0);
        done = total - wanted.reduce((sum, file) => sum + file.size, 0);
        await reserve(total + CACHE_BYTES - await folderBytes(['halo']));
        navigator.storage.persist?.().catch(() => {});
        const folder = await directory([...DATA_PATH, 'maps'], true);
        for (const file of wanted) {
          const name = file.name.slice(5);
          await retry(async () => {
            delete manifest[name];
            await writeJSON(DATA_PATH, 'maps.json', manifest);
            const handle = await folder.getFileHandle(name, { create: true });
            let access, written = 0, lastReport = 0;
            try {
              access = await handle.createSyncAccessHandle();
              access.truncate(0);
              for (const chunk of file.chunks) {
                await withResponse(new URL(chunk.path, BASE), signal, async response => {
                  const reader = response.body.getReader();
                  let received = 0;
                  try {
                    for (;;) {
                      signal.throwIfAborted();
                      const { done: finished, value } = await reader.read();
                      if (finished) break;
                      received += value.byteLength;
                      if (received > chunk.size) throw new Error(`Oversized download: ${name}`);
                      let offset = 0;
                      while (offset < value.length) {
                        const count = access.write(value.subarray(offset), { at: written });
                        if (!count) throw new Error(`Could not save ${name}`);
                        written += count;
                        offset += count;
                      }
                      const now = Date.now();
                      if (now - lastReport > 100 || written === file.size) {
                        report('downloading', 'Downloading game data',
                          `${((done + written) / 1e9).toFixed(2)} of ${(total / 1e9).toFixed(2)} GB · Keep this tab open.`, done + written);
                        lastReport = now;
                      }
                    }
                    if (received !== chunk.size) throw new Error(`Incomplete download: ${name}`);
                  } finally { await reader.cancel().catch(() => {}); reader.releaseLock(); }
                });
              }
              access.flush();
              access.close();
              access = null;
              signal.throwIfAborted();
              report('checking', 'Checking downloaded data', `Verifying ${name} before saving it.`, done + written);
              const cached = await handle.getFile();
              const digest = new Uint8Array(await crypto.subtle.digest('SHA-256', await cached.arrayBuffer()));
              const hash = Array.from(digest, byte => byte.toString(16).padStart(2, '0')).join('');
              if (cached.size !== file.size || hash !== file.sha256 || !await headerValid(cached)) {
                throw new Error(`${name} failed its integrity check`);
              }
              signal.throwIfAborted();
              manifest[name] = file.size;
              await writeJSON(DATA_PATH, 'maps.json', manifest);
            } catch (error) {
              if (access) access.close();
              delete manifest[name];
              await folder.removeEntry(name).catch(() => {});
              throw error;
            }
          });
          done += file.size;
        }
        const complete = await mapsState();
        if (!complete) throw new Error('The downloaded maps are incomplete');
        return ready(complete);
      }, signal);
    } catch (error) {
      report(signal.aborted ? 'paused' : 'error', signal.aborted ? 'Download paused' : 'Download interrupted',
        signal.aborted ? 'Completed maps are saved. Resume when you are ready.' : `${error.message}. Retry or choose your disc image.`);
      throw error;
    }
  }

  function download({ signal, onProgress = () => {} } = {}) {
    return new Promise((resolve, reject) => {
      const worker = new Worker('xiso-worker.js');
      const stop = () => worker.postMessage({ type: 'cancel-cache' });
      const finish = () => { worker.terminate(); signal?.removeEventListener('abort', stop); };
      signal?.addEventListener('abort', stop, { once: true });
      worker.onmessage = ({ data }) => {
        if (data.type === 'cache-progress') onProgress(data.progress);
        else if (data.type === 'cache-done') { finish(); resolve(data.maps); }
        else if (data.type === 'error') {
          finish();
          reject(new DOMException(data.message, data.name || 'Error'));
        }
      };
      worker.onerror = event => { finish(); reject(new Error(event.message || 'The download stopped.')); };
      worker.postMessage({ type: 'cache' });
      if (signal?.aborted) stop();
    });
  }

  return { mapsState, download, ensure, withLock, directory, expected: EXPECTED.slice(), validateManifest };
})();
