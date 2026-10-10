// Rooms (port/web/site/net.js) in two headless Chromiums, through a local
// MQTT broker (mosquitto, with a WebSocket listener):
//
//     node port/web/tests/rooms.mjs dist/halo-web-release
//
// (with Playwright where Node finds it, and mosquitto on the PATH:
// .github/workflows/build.yml). Two browsers join one room by its link; each
// must find the other through the broker, connect to it over WebRTC, and say
// so, with an address of its own in 10.0.0.0/8. No game data is needed: the
// game is not started. (A system link game between them needs the maps:
// port/web/README.md, "Testing".)

import { spawn } from 'node:child_process';
import { mkdtempSync, writeFileSync, rmSync } from 'node:fs';
import { createRequire } from 'node:module';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { setTimeout as sleep } from 'node:timers/promises';

const require = createRequire(import.meta.url);
const { chromium } = require('playwright');

const site = path.resolve(process.argv[2] || 'build/web/site');
const here = path.dirname(new URL(import.meta.url).pathname);
const sitePort = 8000 + Math.floor(Math.random() * 1000);
const brokerPort = 9000 + Math.floor(Math.random() * 1000);
const work = mkdtempSync(path.join(tmpdir(), 'opence-rooms-'));
const config = path.join(work, 'mosquitto.conf');
writeFileSync(config, `listener ${brokerPort}\nprotocol websockets\nallow_anonymous true\n`);
const broker = spawn('mosquitto', ['-c', config], { stdio: 'ignore' });
const server = spawn('python3', [path.join(here, '../../../tools/web_serve.py'), site, '--port', String(sitePort)],
  { stdio: 'ignore', env: { ...process.env, WEB_SERVE_QUIET: '1' } });

function fail(message) {
  console.error('FAILED: ' + message);
  process.exitCode = 1;
}

const browsers = [];
try {
  await sleep(1500);
  const secret = [...crypto.getRandomValues(new Uint8Array(16))].map((b) => b.toString(16).padStart(2, '0')).join('');
  const url = `http://localhost:${sitePort}/?room=${secret}&brokers=ws://localhost:${brokerPort}`;
  const pages = [];
  for (let index = 0; index < 2; index++) {
    // (each its own browser, as two players' are; their local addresses in
    // the clear, which the sandbox cannot resolve as mDNS names)
    const browser = await chromium.launch({ args: ['--disable-features=WebRtcHideLocalIpsWithMdns'] });
    browsers.push(browser);
    const page = await browser.newPage();
    page.on('pageerror', (error) => console.log(`browser ${index}: ${error}`));
    await page.goto(url);
    pages.push(page);
  }
  for (const [index, page] of pages.entries()) {
    await page.waitForFunction(() => /1 of 1 other browsers connected/.test(document.querySelector('#room-status').textContent),
      null, { timeout: 60000 });
    const status = await page.textContent('#room-status');
    console.log(`browser ${index}: ${status}`);
    if (!/Your address: 10\.\d+\.\d+\.\d+/.test(status)) fail('no address in the room');
  }
  // the room's link, as the page gives it to copy
  const link = await pages[0].evaluate(() => location.href);
  if (!link.includes(secret)) fail("the page's address is not the room's link");
} catch (error) {
  fail(String(error && error.stack ? error.stack : error));
} finally {
  for (const browser of browsers) await browser.close();
  server.kill();
  broker.kill();
  rmSync(work, { recursive: true, force: true });
}
