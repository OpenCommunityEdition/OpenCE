// Exercise production MCC staging and the browser transaction worker through
// the actual WasmFS backend. Each browser gets an isolated disposable origin.
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
assert(engines.every(name => ['chromium', 'firefox'].includes(name)));
const work = await fs.realpath(await fs.mkdtemp(path.join(os.tmpdir(), 'halo-mcc-storage-')));
const reports = [];
let server, browser;
try {
  // Delay entry to the real decoder, not merely its completed service reply.
  // This wrapper is linked only into this disposable fixture.
  const delayedProxy = path.join(work, 'delayed-proxy.js');
  const timeoutDeclaration = path.join(work, 'test-timeout.h');
  await fs.writeFile(timeoutDeclaration, 'int mcc_storage_test_wait_ms(void);\n');
  await fs.writeFile(delayedProxy, `addToLibrary({
    web_mcc_test_storage: LibraryManager.library.web_js_mcc_storage,
    web_js_mcc_storage__deps: ['web_mcc_test_storage'],
    web_js_mcc_storage: (action,payload,mailbox,generation) => {
      if (new URLSearchParams(location.search).get('case') === 'begin_proxy_delayed' &&
          UTF8ToString(action) === 'begin')
        setTimeout(() => _web_mcc_test_storage(action,payload,mailbox,generation),1500);
      else _web_mcc_test_storage(action,payload,mailbox,generation);
    }
  });`);
  const compile = spawnSync(process.env.WEB_EMCC || 'emcc', [
    'port/web/tests/mcc_storage_test.c', 'port/web/src/web_mcc_storage.c',
    'port/linux/game/mcc_share_stage.c', 'port/linux/game/mcc_share_rules.c',
    'port/linux/game/mcc_sha256.c', 'port/linux/src/posix_mcc_share.c',
    '-Iport/web/src', '-Iport/linux/game', '-Iport/linux/src', '-DHALO_WEB=1',
    '-DWEB_MCC_STORAGE_WAIT_MS=mcc_storage_test_wait_ms()', '-include', timeoutDeclaration,
    '-std=gnu89', '-O1', '-pthread', '-sPROXY_TO_PTHREAD=1', '-sPTHREAD_POOL_SIZE=5',
    '-sWASMFS=1', '-lopfs.js', '-sINITIAL_MEMORY=33554432', '-sEXIT_RUNTIME=1',
    '-sENVIRONMENT=web,worker', '--js-library', 'port/web/src/web_mcc_library.js',
    '--js-library', delayedProxy,
    '-o', path.join(work, 'fixture.js'),
  ], { cwd: root, encoding: 'utf8', timeout: 120000 });
  assert.equal(compile.status, 0, String(compile.error || '') + compile.stdout + compile.stderr);
  await fs.writeFile(path.join(work, 'index.html'), `<!doctype html><meta charset="utf-8">
<script src="map-import-storage.js"></script><script>
window.result={output:[],errors:[],actions:[],exit:null};
const scenario=new URLSearchParams(location.search).get('case');
var Module={arguments:[scenario],
 print:line=>result.output.push(String(line)),printErr:line=>result.errors.push(String(line)),
 onAbort:reason=>{result.errors.push(String(reason));result.exit=-1;},onExit:code=>{result.exit=code;},
 haloMccStorage:async(action,payload)=>{
   result.actions.push(action);
   // Reproduce an ordinary BEGIN exceeding the former global one-second
   // fixture deadline, before testing the deliberately lost COMMIT reply.
   if(scenario==='commit_reply_lost'&&action==='begin')
     await new Promise(resolve=>setTimeout(resolve,1500));
   const answer=await HaloMapStorage.dispatch(action,payload);
   if((scenario==='begin_reply_lost'&&action==='begin')||(scenario==='commit_reply_lost'&&action==='commit'))
     return new Promise(()=>{});
   if(scenario==='commit_reply_delayed'&&action==='commit')
     return new Promise(resolve=>setTimeout(()=>resolve(answer),1500));
   return answer;
 }};
HaloMapStorage.startGame().then(()=>{
 const script=document.createElement('script');script.src='fixture.js';document.head.append(script);
}).catch(error=>{result.errors.push(String(error));result.exit=-2;});
</script>`);
  const assets = new Set(['map-import-storage.js', 'map-storage-worker.js']);
  server = http.createServer(async (request, response) => {
    const name = new URL(request.url, 'http://localhost').pathname.slice(1);
    if (!['index.html', 'fixture.js', 'fixture.wasm', ...assets].includes(name)) {
      response.writeHead(404).end(); return;
    }
    try {
      const data = await fs.readFile(assets.has(name) ? path.join(root, 'port/web/site', name) : path.join(work, name));
      response.writeHead(200, { 'Content-Type': name.endsWith('.wasm') ? 'application/wasm' :
        name.endsWith('.js') ? 'text/javascript' : 'text/html',
      'Cross-Origin-Opener-Policy': 'same-origin', 'Cross-Origin-Embedder-Policy': 'require-corp' }).end(data);
    } catch { response.writeHead(404).end(); }
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  for (const engine of engines) {
    browser = await playwright[engine].launch({ headless: true });
    const groups = [['cancel', 'flush_failure', 'crash', 'recovered', 'install', 'reload', 'exists'],
      ['begin_reply_lost', 'recovered'], ['commit_reply_lost', 'reload'], ['commit_reply_delayed', 'reload'],
      ['sidecar', 'sidecar_reload'], ['begin_proxy_delayed', 'recovered']];
    if (engine === 'chromium') groups.push(['quota_refused']);
    for (const scenarios of groups) {
      const context = await browser.newContext();
      for (const scenario of scenarios) {
      const page = await context.newPage();
      const pageErrors = [];
      page.on('pageerror', error => pageErrors.push(String(error)));
      const origin = `http://127.0.0.1:${server.address().port}`;
      if (scenario === 'quota_refused') {
        const cdp = await context.newCDPSession(page);
        await cdp.send('Storage.overrideQuotaForOrigin', { origin, quotaSize: 1 });
      }
      await page.goto(`${origin}/index.html?case=${scenario}`);
      await page.waitForFunction(() => result.exit !== null || result.output.includes('PASS crash ready'), null, { timeout: 90000 });
      const result = await page.evaluate(() => ({ ...window.result, isolated: crossOriginIsolated }));
      reports.push({ engine, version: browser.version(), scenario, ...result, pageErrors });
      assert.equal(result.isolated, true);
      assert.equal(result.exit, scenario === 'crash' ? null : 0, JSON.stringify(reports.at(-1)));
      assert(result.output.includes('PASS ' + (scenario === 'crash' ? 'crash ready' : scenario)), JSON.stringify(reports.at(-1)));
      if (scenario.includes('reply_') || scenario === 'begin_proxy_delayed') assert.deepEqual(result.errors,
        ['mcc browser storage: response unavailable; close this page and reopen it to recover the download.']);
      else assert.deepEqual(result.errors, [], JSON.stringify(reports.at(-1)));
      assert.deepEqual(pageErrors, []);
      if (scenario === 'commit_reply_lost') {
        assert(result.actions.includes('begin'));
        assert(result.actions.includes('commit'), 'The lost reply must be COMMIT, not an earlier timeout');
      }
      console.log(`${engine} ${browser.version()}: ${scenario} passed`);
      if (scenario === 'commit_reply_delayed' || scenario === 'begin_proxy_delayed') await page.waitForTimeout(700);
      await page.close();
      }
      await context.close();
    }
    await browser.close();browser=null;
  }
} finally {
  if (browser) await browser.close();
  if (server) await new Promise(resolve => server.close(resolve));
  if (process.env.WEB_MCC_STORAGE_RESULTS) await fs.writeFile(process.env.WEB_MCC_STORAGE_RESULTS,
    JSON.stringify({ cases: reports }, null, 2) + '\n');
  assert.equal(await fs.realpath(work), path.resolve(work));
  await fs.rm(work, { recursive: true, force: true });
}
