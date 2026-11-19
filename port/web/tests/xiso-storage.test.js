'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync(require.resolve('../site/xiso-worker.js'), 'utf8');

class File {
  constructor(size) { this.kind = 'file'; this.size = size; }
  async getFile() { return { size: this.size }; }
  async createSyncAccessHandle() {
    return {
      truncate: (size) => { this.size = size; },
      write: (bytes, { at }) => { this.size = Math.max(this.size, at + bytes.length); return bytes.length; },
      flush() {}, close() {},
    };
  }
}

class Directory {
  constructor(entries = {}) { this.kind = 'directory'; this.entries = new Map(Object.entries(entries)); this.removed = []; }
  async *values() { yield* this.entries.values(); }
  async getDirectoryHandle(name, options = {}) {
    if (!this.entries.has(name) && options.create) this.entries.set(name, new Directory());
    if (!this.entries.has(name)) throw Object.assign(new Error('missing'), { name: 'NotFoundError' });
    assert.equal(this.entries.get(name).kind, 'directory');
    return this.entries.get(name);
  }
  async getFileHandle(name) {
    if (!this.entries.has(name)) this.entries.set(name, new File(0));
    return this.entries.get(name);
  }
  async removeEntry(name, options) {
    assert.equal(options.recursive, true);
    this.removed.push(name);
    this.entries.delete(name);
  }
}

function harness(estimate) {
  const old = new Directory({ 'ui.map': new File(4096), nested: new Directory({ leftover: new File(4096) }) });
  const saves = new Directory({ save: new File(1000000) });
  const root = new Directory({ maps: old, saves, cache: new File(1000000) });
  const messages = [];
  const context = vm.createContext({
    navigator: { storage: { getDirectory: async () => root, estimate } },
    TextEncoder, Uint8Array, DataView, Date, onmessage: null,
    postMessage: (message) => messages.push(message),
  });
  vm.runInContext(source, context);
  return { root, old, saves, messages, run: (files) => context.onmessage({ data: { type: 'folder', files } }) };
}

function map(valid = true) {
  const bytes = new Uint8Array(4096);
  bytes.set(Buffer.from(valid ? 'daeh' : 'nope'), 0);
  new DataView(bytes.buffer).setUint32(4, 5, true);
  bytes.set(Buffer.from('01.10.12.2276'), 0x40);
  bytes.set(Buffer.from('toof'), 0x7FC);
  return { name: 'ui.map', size: bytes.length, slice: (start, end) => new Blob([bytes.slice(start, end)]) };
}

function preserved(h) {
  assert.equal(h.root.entries.get('maps'), h.old);
  assert.equal(h.root.entries.get('saves'), h.saves);
  assert.deepEqual(h.root.removed, []);
}

function error(h) {
  const result = h.messages.at(-1);
  assert.equal(result.type, 'error');
  return result.message;
}

test('insufficient final capacity preserves old maps and does not count saves/cache as reclaimable', async () => {
  const h = harness(async () => ({ quota: 100000, usage: 110000 }));
  await h.run([map()]);
  assert.match(error(h), /After replacing the existing maps.*existing maps have not been changed/);
  preserved(h);
});

test('replacement fits when old maps, including nested files, are reclaimed', async () => {
  const h = harness(async () => ({ quota: 100000, usage: 103000 }));
  await h.run([map()]);
  assert.equal(h.messages.at(-1).type, 'done');
  assert.deepEqual(h.root.removed, ['maps']);
  assert.equal(h.root.entries.get('saves'), h.saves);
  assert.equal(h.root.entries.get('cache').size, 1000000);
  const maps = h.root.entries.get('maps');
  assert.equal(maps.entries.get('ui.map').size, 4096);
  assert.ok(maps.entries.get('.complete').size > 0);
});

test('unknown or nonfinite storage estimates preserve maps', async () => {
  for (const estimate of [undefined, async () => undefined, async () => ({}),
    async () => ({ quota: Infinity, usage: 0 }), async () => ({ quota: 100000, usage: NaN }),
    async () => ({ quota: -1, usage: 0 }), async () => { throw new Error('unavailable'); }]) {
    const h = harness(estimate);
    await h.run([map()]);
    assert.match(error(h), /could not estimate available storage.*existing maps have not been changed/);
    preserved(h);
  }
});

test('negative available estimate displays zero, not negative MB', async () => {
  const h = harness(async () => ({ quota: 1000000, usage: 525000000 }));
  await h.run([map()]);
  assert.match(error(h), /keep 0 MB more/);
  assert.doesNotMatch(error(h), /-\d+ MB/);
  preserved(h);
});

test('invalid input is rejected before deleting existing maps', async () => {
  const h = harness(async () => ({ quota: 1000000, usage: 0 }));
  await h.run([map(false)]);
  assert.match(error(h), /cannot be used/);
  preserved(h);
});

test('preflight includes the completion marker bytes', async () => {
  const h = harness(async () => ({ quota: 100000, usage: 104096 }));
  await h.run([map()]); // 8192 reclaimed leaves exactly 4096 bytes, not room for the marker.
  assert.match(error(h), /completion marker need/);
  preserved(h);
});
