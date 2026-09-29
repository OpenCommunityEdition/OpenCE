import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {webcrypto} from "node:crypto";
import vm from "node:vm";

const source = readFileSync(new URL("../port/web/auto-cache.js", import.meta.url), "utf8");
const bytes = new Uint8Array(2048).fill(7);
const hash = Buffer.from(await webcrypto.subtle.digest("SHA-256", bytes)).toString("hex");
const names = ["ui", ...Array.from({length: 23}, (_, i) => `map${i}`)];
const files = names.map(name => ({name: `maps/${name}.map`, size: bytes.length, sha256: hash,
    chunks: [0, 1].map(i => ({path: `chunks/${name}.map.part00${i}`, size: bytes.length / 2}))}));

async function scenario({cached = [], corrupt = false, quota = true, badManifest = false,
    badChunk = false, truncated = false, cancelDownload = false} = {}) {
    const committed = new Map(cached.map(name => [name, bytes.length]));
    const staged = new Map();
    const requests = [];
    const ui = Object.fromEntries(["download-panel", "download-title", "download-percent", "download-progress", "download-detail"]
        .map(id => [id, {dataset: {}, removeAttribute() {}}]));
    let busy = false, status = "", button;
    const context = {window: {}, URL, Uint8Array, ReadableStream, AbortSignal, AbortController, crypto: webcrypto,
        location: {href: "https://site.example/game/"}, setTimeout: callback => callback(),
        navigator: {storage: {persist: async () => true}},
        document: {getElementById: id => ui[id], createElement: () => button = {remove() {}}},
        fetch: async (url, options) => {
            requests.push(String(url));
            if (String(url).endsWith("manifest.json")) {
                const manifestFiles = structuredClone(badManifest ? files.slice(1) : files);
                if (badChunk) manifestFiles[0].chunks[0].path = "https://elsewhere.example/map";
                return new Response(JSON.stringify({version: 2, files: manifestFiles}));
            }
            if (cancelDownload) { button.onclick(); options.signal.throwIfAborted(); }
            return new Response(corrupt ? new Uint8Array(1024) : bytes.slice(0, truncated ? 512 : 1024));
        }};
    vm.runInNewContext(source, context);
    const api = {
        base: "https://raw.example/pinned-commit/", expectedMaps: names,
        elements: {discInput: {}, folderInput: {}, clearButton: {}, startButton: {},
            status: {after() {}}, progress: {}},
        storedMaps: async () => [...committed].map(([name, size]) => ({name, size, problem: null})),
        mapProblems: maps => names.filter(name => !maps.some(map => map.name === name + ".map")),
        setBusy: value => busy = value, setStatus: value => status = value,
        withGameLock: task => task(), canGrowBy: async () => quota, cacheBytes: 1,
        folderBytes: async () => 0, storageMessage: () => "Not enough storage",
        formatBytes: String, currentManifest: async () => ({}),
        refreshMaps: async () => context.window.haloAutoCache.refresh(await api.storedMaps()),
        directory: async () => ({getFileHandle: async name => ({getFile: async () => new Blob([staged.get(name)])})}),
        storeMap: async (manifest, name, stream, size, progress, verify) => {
            committed.delete(name);
            staged.set(name, new Uint8Array(await new Response(stream).arrayBuffer()));
            progress(size);
            await verify();
            committed.set(name, size);
        },
    };
    const result = await context.window.haloAutoCache(api);
    assert.equal(busy, false);
    assert.equal(api.elements.discInput.disabled || false, false);
    return {result, requests, committed, status, ui};
}

let r = await scenario({cached: names.map(name => name + ".map")});
assert.equal(r.result, true); assert.equal(r.requests.length, 0);
assert.equal(r.ui["download-panel"].hidden, false);
assert.equal(r.ui["download-title"].textContent, "Already downloaded");
assert.equal(r.ui["download-percent"].textContent, "100%");
console.log("PASS complete browser cache makes zero network requests");
r = await scenario({cached: names.slice(1).map(name => name + ".map")});
assert.equal(r.result, true); assert.equal(r.requests.length, 3); assert.equal(r.committed.size, 24);
console.log("PASS partial cache concatenates only the missing map's chunks and verifies its checksum");
r = await scenario({corrupt: true});
assert.equal(r.result, false); assert.equal(r.committed.size, 0); assert.match(r.status, /integrity/);
assert.equal(r.requests.length, 7);
console.log("PASS corrupt data retries twice then remains uncommitted with manual fallback");
r = await scenario({quota: false});
assert.equal(r.result, false); assert.equal(r.requests.length, 1); assert.match(r.status, /storage/);
console.log("PASS quota failure prevents map downloads");
r = await scenario({badManifest: true});
assert.equal(r.result, false); assert.equal(r.requests.length, 1);
console.log("PASS incomplete manifest is rejected before map downloads");
r = await scenario({badChunk: true});
assert.equal(r.result, false); assert.equal(r.requests.length, 1);
console.log("PASS off-site chunk paths are rejected before downloads");
r = await scenario({truncated: true});
assert.equal(r.result, false); assert.equal(r.committed.size, 0); assert.match(r.status, /Incomplete download/);
console.log("PASS truncated chunks never become committed maps");
r = await scenario({cancelDownload: true, cached: names.slice(1).map(name => name + ".map")});
assert.equal(r.result, false); assert.equal(r.committed.size, 23); assert.match(r.status, /paused/);
console.log("PASS cancellation preserves completed maps and restores manual controls");
