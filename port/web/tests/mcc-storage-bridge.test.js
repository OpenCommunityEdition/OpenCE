'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

function bridge(service) {
  const words = new Int32Array(new SharedArrayBuffer(16));
  let library;
  const context = {
    addToLibrary(value) { library = value; },
    UTF8ToString(value) { return value; },
    HEAP32: words, Atomics, Promise, Math, Number, JSON,
    Module: { haloMccStorage: service },
  };
  vm.runInNewContext(fs.readFileSync(path.join(__dirname, '../src/web_mcc_library.js'), 'utf8'), context);
  async function call(action, payload = {}) {
    words[0]++;
    words[1] = -1;
    library.web_js_mcc_storage(action, JSON.stringify(payload), 0, words[0]);
    await new Promise(resolve => setImmediate(resolve));
    return { result: words[1], available: (words[2] >>> 0) + (words[3] >>> 0) * 4294967296 };
  }
  return { words, call, library, context };
}

test('quota keeps unknown, exhausted and greater-than-32-bit values distinct', async () => {
  for (const [value, expected] of [
    [{ known: false, available: 0 }, { result: 0, available: 0 }],
    [{ known: true, available: 0 }, { result: 1, available: 0 }],
    [{ known: true, available: 10737418240 }, { result: 1, available: 10737418240 }],
    [{ known: true, available: -1 }, { result: 0, available: 0 }],
    [{ known: true, available: Infinity }, { result: 0, available: 0 }],
    [{ known: true, available: 1.5 }, { result: 0, available: 0 }],
  ]) assert.deepEqual(await bridge(async () => value).call('space'), expected);
});

test('publication and visibility accept only explicit success', async () => {
  assert.equal((await bridge(async () => ({ ok: true })).call('commit')).result, 1);
  assert.equal((await bridge(async () => ({ ok: false })).call('commit')).result, 0);
  assert.equal((await bridge(async () => ({ visible: true })).call('visible')).result, 1);
  assert.equal((await bridge(async () => ({ visible: false })).call('visible')).result, 0);
  assert.equal((await bridge(async () => ({ ok: true })).call('visible')).result, 0);
});

test('missing service and rejected storage operations fail closed', async () => {
  assert.equal((await bridge(undefined).call('begin')).result, 0);
  assert.equal((await bridge(async () => { throw Error('quota exceeded'); }).call('commit')).result, 0);
});

test('a stale asynchronous completion cannot overwrite the current mailbox', async () => {
  let resolve;
  const b = bridge(() => new Promise(done => { resolve = done; }));
  b.words[0] = 4;
  b.words[1] = -1;
  b.library.web_js_mcc_storage('commit', '{}', 0, 4);
  await Promise.resolve();
  b.words[0] = 5;
  resolve({ ok: true });
  await new Promise(done => setImmediate(done));
  assert.equal(b.words[1], -1);
});

test('a delayed proxy cannot decode or dispatch an expired request', async () => {
  let calls = 0;
  const b = bridge(async () => { calls++; return { ok: true }; });
  b.context.UTF8ToString = () => { throw Error('expired pointer decoded'); };
  b.words[0] = 5;
  b.words[1] = -1;
  b.library.web_js_mcc_storage('begin', '{}', 0, 4);
  await new Promise(done => setImmediate(done));
  assert.equal(calls, 0);
  assert.equal(b.words[1], -1);
});

test('expiry between proxy decoding and promise dispatch prevents mutation', async () => {
  let calls = 0;
  const b = bridge(async () => { calls++; return { ok: true }; });
  b.words[0] = 4;
  b.words[1] = -1;
  b.library.web_js_mcc_storage('begin', '{}', 0, 4);
  b.words[0] = 5;
  await new Promise(done => setImmediate(done));
  assert.equal(calls, 0);
  assert.equal(b.words[1], -1);
});
