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
  const files = new Map(), directories = new Set(['']), writes = [], requests = [], progress = [], reservations = [], workerMessages = [];
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
            reservations.push(size);
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
    Worker: class {
      constructor() { workerMessages.push('created'); }
      postMessage(message) {
        workerMessages.push(message);
        if (message.type === 'cache') queueMicrotask(() => this.onmessage({ data: { type: 'cache-done', maps: { required: message.required } } }));
      }
      terminate() { workerMessages.push('terminated'); }
    },
    fetch: async url => {
      requests.push(String(url));
      if (String(url).endsWith('manifest.json')) {
        const manifest = { version: 2, files: context.HaloCache.expected.map(name => ({
          name: 'maps/' + name, size: bytes.length, sha256,
          chunks: [0, 1].map(index => ({ path: `chunks/${name}.part00${index}`, size: bytes.length / 2 })),
        })) };
        if (options.badManifest) manifest.files[1].chunks[0].path = 'https://unexpected.example/steal';
        return new Response(JSON.stringify(manifest));
      }
      if (options.cancel || (options.cancelMap && String(url).includes(options.cancelMap))) abort.abort();
      const index = String(url).endsWith('000') ? 0 : 1;
      const block = bytes.slice(index * 1024, (index + 1) * 1024);
      if (options.corrupt) block.fill(0);
      return new Response(options.truncated ? block.slice(0, 512) : block);
    },
  };
  vm.runInNewContext(source, context);
  const names = context.HaloCache.expected;
  const cached = Array.isArray(options.cached) ? options.cached : options.cached === 'partial' ? names.slice(1) : options.cached ? names : [];
  const root = options.native ? 'maps' : 'halo/data/maps';
  for (const name of cached) put(root + '/' + name, bytes);
  if (cached.length && !options.native && !options.noMarker) {
    put('halo/data/maps.json', JSON.stringify(Object.fromEntries(cached.map(name => [name, bytes.length]))));
  }
  if (cached.length && options.native && !options.noMarker) {
    put('maps/.complete', JSON.stringify({ files: cached, bytes: cached.length * bytes.length }));
  }
  if (options.corruptCached) put(root + '/ui.map', new Uint8Array(2048));
  if (options.badMarker && options.native) put('maps/.complete', JSON.stringify({ files: cached, bytes: 1 }));
  for (const [path, data] of Object.entries(options.extraFiles || {})) put(path, data);
  const initialState = options.inspectInitial ? await context.HaloCache.mapsState({ required: options.required }) : undefined;
  let result, error;
  try { result = await context.HaloCache.ensure({ required: options.required, signal: abort.signal, onProgress: entry => progress.push(entry) }); }
  catch (caught) { error = caught; }
  return { result, error, requests, writes, files, progress, names, reservations, context, workerMessages, initialState };
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

const roomMaps = ['ui.map', 'bloodgulch.map'];
const runtimeBytes = 2 * 0x11600000 + 0x02300000 + 3 * 0x02f00000 + (64 << 20);
r = await scenario({ required: roomMaps });
assert.equal(r.error, undefined);
assert.deepEqual(Array.from(r.result.files), roomMaps);
assert.equal(r.result.requiredBytes, bytes.length * 2);
assert.equal(r.requests.length, 5);
assert.deepEqual(r.requests.slice(1).map(url => url.split('/').at(-1)), [
  'ui.map.part000', 'ui.map.part001', 'bloodgulch.map.part000', 'bloodgulch.map.part001',
]);
assert.equal(r.reservations[0], runtimeBytes + bytes.length * 2);
assert.equal(r.progress.at(-1).total, bytes.length * 2);
assert.equal(r.progress.at(-1).done, bytes.length * 2);
assert.equal(r.progress.at(-1).fraction, 1);
console.log('PASS cold room download fetches only manifest plus four chunks, UI first, reserves and reports required bytes');

const unrelated = await scenario({ required: roomMaps, cached: ['a10.map', 'a30.map', 'a50.map'],
  extraFiles: { 'halo/save/profile.dat': new Uint8Array(32768), 'halo/data/notes.txt': new Uint8Array(8192) } });
assert.equal(unrelated.reservations[0], runtimeBytes + bytes.length * 2);
assert.equal(unrelated.requests.length, 5);
const withRuntimeCache = await scenario({ required: roomMaps, cached: ['a10.map'],
  extraFiles: { 'halo/save/z/cache000.map': new Uint8Array(8192), 'halo/save/z/notes.txt': new Uint8Array(32768) } });
assert.equal(withRuntimeCache.reservations[0], runtimeBytes + bytes.length * 2 - 8192);
console.log('PASS unrelated maps, saves, and notes cannot reduce required storage; only existing runtime cache allocations count');

const requestCount = r.requests.length, writeCount = r.writes.length;
const cachedRoom = await r.context.HaloCache.mapsState({ required: roomMaps });
assert.equal(cachedRoom.requiredBytes, bytes.length * 2);
assert.equal(await r.context.HaloCache.mapsState(), null);
await r.context.HaloCache.ensure({ required: roomMaps });
assert.equal(r.requests.length, requestCount);
assert.equal(r.writes.length, writeCount);
console.log('PASS room subset is launch-ready on reload with zero network or writes, full game still needs remaining maps');

const full = await r.context.HaloCache.ensure();
assert.equal(full.files.length, 24);
assert.equal(r.requests.length - requestCount, 1 + 22 * 2);
assert.equal(r.writes.filter(path => path === 'halo/data/maps/ui.map').length, 1);
assert.equal(r.writes.filter(path => path === 'halo/data/maps/bloodgulch.map').length, 1);
assert.equal((await r.context.HaloCache.mapsState()).files.length, 24);
console.log('PASS upgrading a room subset to the full game downloads remaining 22 maps without rewriting completed room maps');

for (const native of [false, true]) {
  r = await scenario({ cached: 'all', native, required: roomMaps });
  assert.equal(r.error, undefined);
  assert.equal(r.result.files.length, 24);
  assert.equal(r.result.bytes, bytes.length * 24);
  assert.equal(r.result.requiredBytes, bytes.length * 2);
  assert.equal(r.progress.at(-1).total, bytes.length * 2);
  assert.equal(r.requests.length, 0); assert.equal(r.writes.length, 0);
}
console.log('PASS full production and native .complete caches satisfy room subsets with no fetch or writes');

r = await scenario({ cached: roomMaps, required: roomMaps, noMarker: true, inspectInitial: true });
assert.equal(r.initialState, null);
assert.equal(r.requests.length, 5);
assert.equal(r.result.requiredBytes, bytes.length * 2);
assert.equal(r.writes.includes('halo/data/maps.json'), true);
assert.equal((await r.context.HaloCache.mapsState({ required: roomMaps })).requiredBytes, bytes.length * 2);
console.log('PASS unmarked legacy subset becomes ready only after redownloading and SHA-verifying both requested maps');

const oversizedHeaderMap = new Uint8Array(bytes.length + 1024);
oversizedHeaderMap.set(bytes);
r = await scenario({ cached: roomMaps, required: roomMaps, noMarker: true, inspectInitial: true,
  extraFiles: { 'halo/data/maps/ui.map': oversizedHeaderMap } });
assert.equal(r.initialState, null);
assert.equal(r.requests.length, 5);
assert.equal(r.result.requiredBytes, bytes.length * 2);
assert.equal(r.files.get('halo/data/maps/ui.map').length, bytes.length);
assert.equal(r.writes.includes('halo/data/maps/bloodgulch.map'), true);
console.log('PASS unmarked legacy partial with valid header and wrong size is repaired through verified downloads');

const corruptUnmarked = bytes.slice();
corruptUnmarked[100] ^= 1;
r = await scenario({ cached: roomMaps, required: roomMaps, noMarker: true, inspectInitial: true,
  extraFiles: { 'halo/data/maps/ui.map': corruptUnmarked } });
assert.equal(r.initialState, null);
assert.equal(r.requests.length, 5);
assert.deepEqual([...r.files.get('halo/data/maps/ui.map')], [...bytes]);
console.log('PASS same-size unmarked map with valid header and corrupted body is replaced before completion metadata');

r = await scenario({ cached: roomMaps, required: roomMaps, native: true });
assert.equal(r.result.dataRoot, '/data');
assert.equal(r.requests.length, 0); assert.equal(r.writes.length, 0);
const nativeFull = await r.context.HaloCache.ensure();
assert.equal(nativeFull.dataRoot, '/data');
assert.equal(nativeFull.files.length, 24);
assert.equal(r.requests.length, 1 + 22 * 2);
assert.equal(r.writes.includes('maps/ui.map'), false);
assert.equal(r.writes.includes('maps/bloodgulch.map'), false);
assert.equal(JSON.parse(new TextDecoder().decode(r.files.get('maps/.complete'))).files.length, 24);
console.log('PASS completed native subset grows in place and preserves original maps and native completion marker');

for (const options of [{ cached: roomMaps, noMarker: true }, { cached: 'all', noMarker: true }, { cached: roomMaps, badMarker: true }]) {
  r = await scenario({ ...options, native: true, required: roomMaps, inspectInitial: true });
  assert.equal(r.initialState, null);
  assert.equal(r.result.dataRoot, '/data/halo/data');
  assert.equal(r.requests.length, 5);
}
console.log('PASS interrupted or malformed native imports cannot satisfy a requested subset without a valid completion marker');

r = await scenario({ required: roomMaps, badManifest: true });
assert.match(r.error.message, /manifest/);
assert.equal(r.requests.length, 1); assert.equal(r.writes.length, 0);
console.log('PASS subset requests validate the entire pinned manifest including unrequested maps');

r = await scenario({ required: roomMaps, cached: roomMaps, corruptCached: true });
assert.equal(r.error, undefined);
assert.equal(r.requests.length, 3);
assert.equal(r.writes.includes('halo/data/maps/bloodgulch.map'), false);
console.log('PASS damaged requested map is repaired without fetching or rewriting the other completed room map');

r = await scenario({ required: roomMaps, corrupt: true });
assert.match(r.error.message, /integrity/);
assert.equal(r.requests.length, 7);
assert.equal(r.files.has('halo/data/maps/ui.map'), false);
assert.equal(await r.context.HaloCache.mapsState({ required: roomMaps }), null);
console.log('PASS subset integrity failures retry twice and never become launch-ready');

r = await scenario({ required: roomMaps, cancelMap: 'bloodgulch.map' });
assert.equal(r.error.name, 'AbortError');
assert.equal(r.progress.at(-1).state, 'paused');
assert.equal(r.files.has('halo/data/maps/ui.map'), true);
assert.equal(r.files.has('halo/data/maps/bloodgulch.map'), false);
const cancelledRequests = r.requests.length;
const resumed = await r.context.HaloCache.ensure({ required: roomMaps });
assert.equal(resumed.requiredBytes, bytes.length * 2);
assert.equal(r.requests.length - cancelledRequests, 3);
console.log('PASS cancelling the second room map preserves completed UI and resumes with only the missing map');

const invalidRequired = [[], ['ui.map', 'ui.map'], ['ui'], ['UI.map'], ['../ui.map'], ['maps/ui.map'], ['nope.map'], null, 'ui.map', {}];
for (const required of invalidRequired) {
  r = await scenario({ required });
  assert.equal(r.error.name, 'TypeError');
  assert.equal(r.requests.length, 0); assert.equal(r.writes.length, 0);
  await assert.rejects(r.context.HaloCache.mapsState({ required }), { name: 'TypeError' });
  assert.throws(() => r.context.HaloCache.download({ required }), { name: 'TypeError' });
  assert.equal(r.workerMessages.length, 0);
}
console.log('PASS every public map API rejects invalid required names before network, writes, or worker creation');

r = await scenario({ cached: 'all' });
const requested = roomMaps.slice();
const workerDownload = r.context.HaloCache.download({ required: requested });
requested.pop();
const workerResult = await workerDownload;
assert.deepEqual(Array.from(workerResult.required), roomMaps);
assert.deepEqual(Array.from(r.workerMessages.find(message => message.type === 'cache').required), roomMaps);
console.log('PASS download worker receives an independent copy of the validated required map list');

const workerSource = readFileSync(new URL('../port/web/site/xiso-worker.js', import.meta.url), 'utf8');
const workerOptions = [], workerOutput = [];
const workerContext = {
  AbortController, onmessage: null, importScripts() {},
  HaloCache: { async ensure(options) { workerOptions.push(options); return { files: options.required }; } },
  postMessage(message) { workerOutput.push(message); },
};
vm.runInNewContext(workerSource, workerContext);
await workerContext.onmessage({ data: { type: 'cache', required: roomMaps } });
assert.deepEqual(workerOptions[0].required, roomMaps);
assert.equal(workerOutput.at(-1).type, 'cache-done');
console.log('PASS cache worker forwards requested subset to the verified downloader');
