// Run the same production allocator fixture in real browser workers and Node
// (tools/test_web_map_memory.py). No game data or game process is involved.
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import fs from 'node:fs/promises';
import http from 'node:http';
import { createRequire } from 'node:module';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const playwright = require('playwright');
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../../..');
const engines = process.argv.slice(2);
if (!engines.length) engines.push('chromium', 'firefox');
assert(engines.every(name => name === 'chromium' || name === 'firefox'), 'expected chromium or firefox');
const work = await fs.mkdtemp(path.join(os.tmpdir(), 'halo-map-memory-'));
const cases = [
  { name: 'small', bytes: 0x01000000, expected: 'insufficient map-window memory refused' },
  { name: 'full', bytes: 0x88000000, expected: 'map windows preserved' },
];
const reports = [];
let server;
let browser;
try {
  for (const item of cases) {
    const folder = path.join(work, item.name);
    await fs.mkdir(folder);
    const compiled = spawnSync(process.env.WEB_EMCC || 'emcc', [
      'port/web/tests/map_memory_test.c', 'port/web/src/web_memory.c',
      '-Iport/web/src', '-Iport/linux/src', '-DHALO_WEB=1', '-std=gnu89', '-O2', '-pthread',
      '-sPROXY_TO_PTHREAD=1', '-sPTHREAD_POOL_SIZE=5', '-sALLOW_MEMORY_GROWTH=0', '-sABORTING_MALLOC=0',
      '-sEXIT_RUNTIME=1', '-sENVIRONMENT=web,worker', `-sINITIAL_MEMORY=${item.bytes}`,
      '-o', path.join(folder, 'memory.js'),
    ], { cwd: root, encoding: 'utf8', timeout: 120000 });
    assert.equal(compiled.status, 0, String(compiled.error || '') + compiled.stdout + compiled.stderr);
    await fs.writeFile(path.join(folder, 'index.html'), `<!doctype html><meta charset="utf-8">
<script>
window.result = { output: [], errors: [], exit: null };
var Module = {
  print: line => result.output.push(String(line)),
  printErr: line => result.errors.push(String(line)),
  onAbort: reason => { result.errors.push(String(reason)); result.exit = -1; },
  onExit: code => { result.exit = code; }
};
</script><script src="memory.js"></script>`);
  }
  server = http.createServer(async (request, response) => {
    const match = /^\/(small|full)\/(index\.html|memory\.(?:js|wasm))$/.exec(request.url);
    if (!match) { response.writeHead(404).end(); return; }
    try {
      const data = await fs.readFile(path.join(work, match[1], match[2]));
      response.writeHead(200, {
        'Content-Type': match[2].endsWith('.wasm') ? 'application/wasm' :
          match[2].endsWith('.js') ? 'text/javascript' : 'text/html',
        'Cross-Origin-Opener-Policy': 'same-origin',
        'Cross-Origin-Embedder-Policy': 'require-corp',
      }).end(data);
    } catch { response.writeHead(404).end(); }
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  for (const name of engines) {
    browser = await playwright[name].launch({ headless: true });
    for (const item of cases) {
      const page = await browser.newPage();
      const pageErrors = [];
      page.on('pageerror', error => pageErrors.push(String(error)));
      await page.goto(`http://127.0.0.1:${server.address().port}/${item.name}/index.html`);
      await page.waitForFunction(() => window.result?.exit !== null, null, { timeout: 90000 });
      const result = await page.evaluate(() => ({ ...window.result, isolated: crossOriginIsolated,
        userAgent: navigator.userAgent }));
      reports.push({ engine: name, version: browser.version(), memory: item.bytes, ...result, pageErrors });
      assert.equal(result.isolated, true);
      assert.equal(result.exit, 0, JSON.stringify(reports.at(-1)));
      assert(result.output.some(line => line.includes(item.expected)), JSON.stringify(reports.at(-1)));
      assert.deepEqual(result.errors, []);
      assert.deepEqual(pageErrors, []);
      console.log(`${name} ${browser.version()}: ${result.output.join('; ')}`);
      await page.close();
    }
    await browser.close();
    browser = null;
  }
} finally {
  if (browser) await browser.close();
  if (server) await new Promise(resolve => server.close(resolve));
  if (process.env.WEB_MEMORY_RESULTS) await fs.writeFile(process.env.WEB_MEMORY_RESULTS,
    JSON.stringify({ cases: reports }, null, 2) + '\n');
  // Only this process's newly created fixture directory is removed.
  assert.equal(await fs.realpath(work), path.resolve(work));
  await fs.rm(work, { recursive: true, force: true });
}
