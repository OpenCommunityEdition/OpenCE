// The site's service worker (port/web/site/sw.js) with a fake network and
// cache storage: what it isolates, how it keeps a build's files together, and
// how a new build replaces them. node --test "port/web/tests/*.test.js"

'use strict';

const test = require('node:test');
const assert = require('node:assert');

const ORIGIN = 'https://halocombatevolved.com';

// a build served by the fake network: version.json and its files
let served = {};

function serve(version, files) {
  served = { 'version.json': JSON.stringify({ version, files: Object.keys(files) }) };
  for (const [name, text] of Object.entries(files)) served[name] = text;
  served[''] = files['index.html'] || '<page>';
}

class FakeCache {
  constructor() {
    this.entries = new Map();
  }

  key(request) {
    const url = new URL(typeof request === 'string' ? request : request.url, ORIGIN + '/');
    return url.origin + url.pathname;
  }

  async addAll(requests) {
    for (const request of requests) {
      const response = await globalThis.fetch(request);
      if (!response.ok) throw new Error('fetch failed');
      this.entries.set(this.key(request), await response.text());
    }
  }

  async put(request, response) {
    this.entries.set(this.key(request), await response.text());
  }

  async match(request) {
    const text = this.entries.get(this.key(request));
    return text === undefined ? undefined : new Response(text);
  }
}

class FakeCacheStorage {
  constructor() {
    this.caches = new Map();
  }

  async open(name) {
    if (!this.caches.has(name)) this.caches.set(name, new FakeCache());
    return this.caches.get(name);
  }

  async keys() {
    return [...this.caches.keys()];
  }

  async delete(name) {
    return this.caches.delete(name);
  }
}

globalThis.fetch = async (request) => {
  const url = new URL(typeof request === 'string' ? request : request.url, ORIGIN + '/');
  const name = url.pathname.replace(/^\//, '');
  if (!(name in served)) return new Response('missing', { status: 404 });
  return new Response(served[name], { status: 200, headers: { 'Content-Type': 'text/plain' } });
};
globalThis.caches = new FakeCacheStorage();
globalThis.self = {
  location: new URL(ORIGIN + '/'),
  addEventListener() {},
  skipWaiting() {},
  clients: { claim() {} },
};
// (relative requests, as the worker makes them, against the site)
const NativeRequest = Request;
globalThis.Request = class extends NativeRequest {
  constructor(input, init) {
    super(typeof input === 'string' ? new URL(input, ORIGIN + '/').href : input, init);
  }
};

const sw = require('../site/sw.js');

test('every response is made cross-origin isolated', async () => {
  const response = sw.isolated(new Response('x', { status: 200, headers: { 'Content-Type': 'text/html' } }));
  assert.strictEqual(response.headers.get('Cross-Origin-Opener-Policy'), 'same-origin');
  assert.strictEqual(response.headers.get('Cross-Origin-Embedder-Policy'), 'require-corp');
  assert.strictEqual(response.headers.get('Cross-Origin-Resource-Policy'), 'same-origin');
  assert.strictEqual(response.headers.get('Content-Type'), 'text/html');
  assert.strictEqual(await response.text(), 'x');
});

test("a build's files are kept together and served from the cache", async () => {
  serve('aaaa', { 'index.html': '<page a>', 'halo.js': 'js a', 'halo.wasm': 'wasm a' });
  await sw.installVersion(JSON.parse(served['version.json']));
  assert.strictEqual(await sw.activeVersion(), 'aaaa');
  // the network moves on; the cached build is still what is served
  serve('bbbb', { 'index.html': '<page b>', 'halo.js': 'js b', 'halo.wasm': 'wasm b' });
  const js = await sw.respond(new Request(ORIGIN + '/halo.js'));
  assert.strictEqual(await js.text(), 'js a');
  assert.strictEqual(js.headers.get('Cross-Origin-Embedder-Policy'), 'require-corp');
  const page = await sw.respond(new Request(ORIGIN + '/?map=a10'));
  assert.strictEqual(await page.text(), '<page a>');
});

test('version.json?latest asks the network, so a new build is seen', async () => {
  serve('cccc', { 'index.html': '<page c>', 'halo.js': 'js c' });
  const latest = await sw.respond(new Request(ORIGIN + '/version.json?latest'));
  assert.strictEqual(JSON.parse(await latest.text()).version, 'cccc');
});

test('a new build replaces the old one whole', async () => {
  serve('dddd', { 'index.html': '<page d>', 'halo.js': 'js d', 'halo.wasm': 'wasm d' });
  await sw.installVersion(JSON.parse(served['version.json']));
  assert.strictEqual(await sw.activeVersion(), 'dddd');
  const names = await caches.keys();
  assert.ok(names.includes(sw.CACHE_PREFIX + 'dddd'));
  assert.ok(!names.includes(sw.CACHE_PREFIX + 'aaaa'));
  assert.strictEqual(await (await sw.respond(new Request(ORIGIN + '/halo.wasm'))).text(), 'wasm d');
});

test('a build whose files cannot all be fetched is not made the one served', async () => {
  serve('eeee', { 'index.html': '<page e>', 'halo.js': 'js e' });
  served['version.json'] = JSON.stringify({ version: 'eeee', files: ['index.html', 'halo.js', 'halo.wasm'] });
  await assert.rejects(sw.installVersion(JSON.parse(served['version.json'])));
  assert.strictEqual(await sw.activeVersion(), 'dddd');
});
