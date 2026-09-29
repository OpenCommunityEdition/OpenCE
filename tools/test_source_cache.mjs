import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { webcrypto } from 'node:crypto';
import vm from 'node:vm';

const source = readFileSync(new URL('../port/web/site/cache.js', import.meta.url), 'utf8');
const bytes = new Uint8Array(2048).fill(7);
bytes.set(new TextEncoder().encode('daeh'), 0);
bytes.set(new TextEncoder().encode('toof'), 2044);
const sha256 = Buffer.from(await webcrypto.subtle.digest('SHA-256', bytes)).toString('hex');

async function scenario(options = {}) {
  const files = new Map(), directories = new Set(['']), writes = [], requests = [], progress = [];
  const abort = new AbortController();
  const absent = () => new DOMException('missing', 'NotFoundError');
  const put = (path, data) => {
    const pieces = path.split('/');
    for (let n = 1; n < pieces.length; n++) directories.add(pieces.slice(0, n).join('/'));
    files.set(path, typeof data === 'string' ? new TextEncoder().encode(data) : data.slice());
  };
  const fileHandle = path => ({
    kind: 'file',
    async getFile() { const blob = new Blob([files.get(path)]); blob.name = path.split('/').at(-1); return blob; },
    async createSyncAccessHandle() {
      let data = files.get(path);
      return {
        truncate(size) {
          if (path.endsWith('space-check')) {
            if (options.quota === false) throw new DOMException('full', 'QuotaExceededError');
            return;
          }
          data = data.slice(0, size);
          writes.push(path);
        },
        write(value, { at }) {
          const next = new Uint8Array(Math.max(data.length, at + value.length));
          next.set(data); next.set(value, at); data = next;
          return value.length;
        },
        flush() { files.set(path, data); },
        close() { files.set(path, data); },
      };
    },
  });
  const directoryHandle = path => ({
    kind: 'directory',
    async getDirectoryHandle(name, { create = false } = {}) {
      const child = path ? path + '/' + name : name;
      if (!directories.has(child) && !create) throw absent();
      directories.add(child); return directoryHandle(child);
    },
    async getFileHandle(name, { create = false } = {}) {
      const child = path ? path + '/' + name : name;
      if (!files.has(child)) { if (!create) throw absent(); files.set(child, new Uint8Array()); }
      return fileHandle(child);
    },
    async removeEntry(name) {
      const child = path ? path + '/' + name : name;
      files.delete(child);
    },
    async *entries() {
      const prefix = path ? path + '/' : '';
      for (const dir of directories) {
        if (dir.startsWith(prefix) && dir !== path && !dir.slice(prefix.length).includes('/')) {
          yield [dir.slice(prefix.length), directoryHandle(dir)];
        }
      }
      for (const file of files.keys()) {
        if (file.startsWith(prefix) && !file.slice(prefix.length).includes('/')) yield [file.slice(prefix.length), fileHandle(file)];
      }
    },
  });
  const context = {
    Uint8Array, URL, Blob, TextEncoder, AbortController, DOMException, crypto: webcrypto,
    setTimeout: (callback, milliseconds) => setTimeout(callback, milliseconds >= 600000 ? milliseconds : 0), clearTimeout,
    navigator: {
      storage: { getDirectory: async () => directoryHandle(''), persist: async () => true },
      locks: { request: async (name, settings, fn) => fn(options.locked ? null : {}) },
    },
    fetch: async url => {
      requests.push(String(url));
      if (String(url).endsWith('manifest.json')) {
        const manifest = { version: 2, files: context.HaloCache.expected.map(name => ({
          name: 'maps/' + name, size: bytes.length, sha256,
          chunks: [0, 1].map(index => ({ path: `chunks/${name}.part00${index}`, size: bytes.length / 2 })),
        })) };
        if (options.badManifest) manifest.files[0].chunks[0].path = 'https://unexpected.example/steal';
        return new Response(JSON.stringify(manifest));
      }
      if (options.cancel) abort.abort();
      const index = String(url).endsWith('000') ? 0 : 1;
      const block = bytes.slice(index * 1024, (index + 1) * 1024);
      if (options.corrupt) block.fill(0);
      return new Response(options.truncated ? block.slice(0, 512) : block);
    },
  };
  vm.runInNewContext(source, context);
  const names = context.HaloCache.expected;
  const cached = options.cached === 'partial' ? names.slice(1) : options.cached ? names : [];
  const root = options.native ? 'maps' : 'halo/data/maps';
  for (const name of cached) put(root + '/' + name, bytes);
  if (cached.length && !options.native && !options.noMarker) {
    put('halo/data/maps.json', JSON.stringify(Object.fromEntries(cached.map(name => [name, bytes.length]))));
  }
  if (cached.length && options.native) {
    put('maps/.complete', JSON.stringify({ files: cached, bytes: cached.length * bytes.length }));
  }
  if (options.corruptCached) put(root + '/ui.map', new Uint8Array(2048));
  let result, error;
  try { result = await context.HaloCache.ensure({ signal: abort.signal, onProgress: entry => progress.push(entry) }); }
  catch (caught) { error = caught; }
  return { result, error, requests, writes, files, progress, names };
}

let r = await scenario({ cached: 'all' });
assert.equal(r.result.dataRoot, '/data/halo/data');
assert.equal(r.result.saveRoot, '/data/halo/save');
assert.equal(r.requests.length, 0); assert.equal(r.writes.length, 0);
assert.equal(r.progress.at(-1).fraction, 1);
console.log('PASS production maps and saves reused in place, zero network or writes, progress 100%');
r = await scenario({ cached: 'all', noMarker: true });
assert.equal(r.requests.length, 0); assert.equal(r.result.files.length, 24);
console.log('PASS older production cache without maps.json remains compatible');
r = await scenario({ cached: 'all', native: true });
assert.equal(r.result.dataRoot, '/data'); assert.equal(r.requests.length, 0);
console.log('PASS new source-port disc import remains compatible');
r = await scenario({ cached: 'partial' });
assert.equal(r.result.files.length, 24); assert.equal(r.requests.length, 3);
assert.deepEqual([...r.files.get('halo/data/maps/ui.map')], [...bytes]);
assert.equal(JSON.parse(new TextDecoder().decode(r.files.get('halo/data/maps.json')))['ui.map'], bytes.length);
console.log('PASS resume downloads only missing map, concatenates chunks and commits after SHA256');
r = await scenario({ cached: 'all', corruptCached: true });
assert.equal(r.result.files.length, 24); assert.equal(r.requests.length, 3);
console.log('PASS damaged cache header triggers repair of only that map');
r = await scenario({ cached: 'partial', corrupt: true });
assert.match(r.error.message, /integrity/); assert.equal(r.requests.length, 7);
assert.equal(r.files.has('halo/data/maps/ui.map'), false);
assert.equal(Object.keys(JSON.parse(new TextDecoder().decode(r.files.get('halo/data/maps.json')))).length, 23);
console.log('PASS corrupt download retries twice and never becomes a completed map');
r = await scenario({ cached: 'partial', truncated: true });
assert.match(r.error.message, /Incomplete/); assert.equal(r.files.has('halo/data/maps/ui.map'), false);
console.log('PASS truncated chunks are removed and uncommitted');
r = await scenario({ quota: false });
assert.equal(r.error.name, 'QuotaExceededError'); assert.equal(r.requests.length, 1);
console.log('PASS quota is checked before game-data requests');
r = await scenario({ badManifest: true });
assert.match(r.error.message, /manifest/); assert.equal(r.requests.length, 1);
console.log('PASS off-site chunk URL rejected before downloads');
r = await scenario({ cached: 'partial', cancel: true });
assert.equal(r.error.name, 'AbortError'); assert.equal(r.progress.at(-1).state, 'paused');
assert.equal(r.files.has('halo/data/maps/ui.map'), false);
assert.equal(r.files.has('halo/data/maps/a10.map'), true);
console.log('PASS cancellation keeps previously completed maps and removes partial map');
r = await scenario({ cached: 'partial', locked: true });
assert.match(r.error.message, /another tab/); assert.equal(r.requests.length, 0); assert.equal(r.writes.length, 0);
console.log('PASS running production game prevents competing writes');
