/*
SW.JS

The site's service worker:

- it serves every response with the headers that make the page
  cross-origin isolated (Cross-Origin-Opener-Policy and
  Cross-Origin-Embedder-Policy), which SharedArrayBuffer, and so the game's
  threads, need; GitHub Pages cannot send them. The first visit loads once
  without them, then again under this worker (app.js);
- it keeps a build's files, so that the site starts without the network
  and a build is never mixed with another: version.json names the build and
  its files; they are fetched together into a cache of their own, and a new
  build replaces them as a whole only when the page asks (once the player
  agrees). version.json?latest always asks the network, so that the page can
  tell a new build is out.
*/

'use strict';

const CACHE_PREFIX = 'opence-web-';
const META_CACHE = 'opence-web-meta';

async function networkVersion() {
  const response = await fetch('version.json', { cache: 'no-store' });
  if (!response.ok) throw new Error('version.json: HTTP ' + response.status);
  return response.json();
}

async function activeVersion() {
  const meta = await caches.open(META_CACHE);
  const response = await meta.match('active');
  return response ? (await response.json()).version : null;
}

async function setActiveVersion(version) {
  const meta = await caches.open(META_CACHE);
  await meta.put('active', new Response(JSON.stringify({ version })));
}

// a build's files into a cache of their own; then it is the one served, and
// the others go
async function installVersion(stamp) {
  const name = CACHE_PREFIX + stamp.version;
  const cache = await caches.open(name);
  const files = ['./', 'version.json', ...stamp.files.filter((file) => file !== 'version.json')];
  await cache.addAll(files.map((path) => new Request(path, { cache: 'no-store' })));
  await setActiveVersion(stamp.version);
  for (const other of await caches.keys()) {
    if (other.startsWith(CACHE_PREFIX) && other !== name && other !== META_CACHE) await caches.delete(other);
  }
}

self.addEventListener('install', (event) => {
  event.waitUntil((async () => {
    try {
      if (!(await activeVersion())) await installVersion(await networkVersion());
    } catch {
      // offline, or a deployment under way: the network serves until a later
      // visit caches a build
    }
    await self.skipWaiting();
  })());
});

self.addEventListener('activate', (event) => {
  event.waitUntil(self.clients.claim());
});

function isolated(response) {
  if (!response || response.status === 0 || response.type === 'opaque') return response;
  const headers = new Headers(response.headers);
  headers.set('Cross-Origin-Opener-Policy', 'same-origin');
  headers.set('Cross-Origin-Embedder-Policy', 'require-corp');
  headers.set('Cross-Origin-Resource-Policy', 'same-origin');
  return new Response(response.body, { status: response.status, statusText: response.statusText, headers });
}

async function cachedResponse(request) {
  const version = await activeVersion();
  if (!version) return null;
  const cache = await caches.open(CACHE_PREFIX + version);
  const url = new URL(request.url);
  let response = await cache.match(request, { ignoreSearch: true });
  if (!response && (request.mode === 'navigate' || url.pathname.endsWith('/'))) {
    response = await cache.match('./', { ignoreSearch: true });
  }
  return response;
}

async function respond(request) {
  const url = new URL(request.url);
  if (url.pathname.endsWith('/version.json') && url.searchParams.has('latest')) {
    return isolated(await fetch('version.json', { cache: 'no-store' }));
  }
  const cached = await cachedResponse(request);
  if (cached) return isolated(cached);
  try {
    return isolated(await fetch(request));
  } catch (error) {
    if (request.mode === 'navigate') {
      const page = await cachedResponse(new Request('./'));
      if (page) return isolated(page);
    }
    throw error;
  }
}

self.addEventListener('fetch', (event) => {
  const request = event.request;
  if (request.method !== 'GET') return;
  if (new URL(request.url).origin !== self.location.origin) return;
  event.respondWith(respond(request));
});

self.addEventListener('message', (event) => {
  if (event.data === 'update') {
    event.waitUntil((async () => {
      let message = 'updated';
      try {
        await installVersion(await networkVersion());
      } catch {
        message = 'update-failed';
      }
      if (event.source) event.source.postMessage(message);
    })());
  }
});

// (for the tests: port/web/tests)
if (typeof module !== 'undefined') {
  module.exports = { isolated, installVersion, activeVersion, cachedResponse, respond, CACHE_PREFIX };
}
