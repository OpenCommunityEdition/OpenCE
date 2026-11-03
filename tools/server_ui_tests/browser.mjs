// The admin page in real browsers (Playwright's Chromium and Firefox),
// against the protocol simulator: signing in through the page's form, and a
// change the page makes with its own fetch. The page's other tests fake the
// browser and set the Origin header themselves; a real browser decides it
// (an HTML form's post under a no-referrer page carries Origin: null, as
// the master's login form found: this page posts by fetch, which keeps the
// real one, and the server takes null only with Sec-Fetch-Site).
//
//     node tools/server_ui_tests/browser.mjs [chromium] [firefox]
//
// (with Playwright where Node finds it: NODE_PATH, as port/web/tests' do)

import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const playwright = require('playwright');
const { startSim } = require('./helpers.js');

const kinds = process.argv.slice(2).length ? process.argv.slice(2) : ['chromium', 'firefox'];
const sim = await startSim(['--still']);
let failed = 0;
try {
  for (const kind of kinds) {
    const browser = await playwright[kind].launch({ headless: true });
    try {
      const page = await browser.newPage();
      await page.goto(sim.url + '/');
      await page.fill('#login-password', 'opence-admin');
      await page.click('#login-button');
      await page.waitForFunction(() => document.querySelector('#login').hidden, null, { timeout: 15000 })
        .catch(() => {});
      const signedIn = await page.evaluate(() => document.querySelector('#login').hidden);
      const said = await page.evaluate(async () => {
        const session = await (await fetch('/api/session')).json();
        const answer = await fetch('/api/say', { method: 'POST', headers: { 'Content-Type': 'application/json',
          'X-CSRF-Token': session.csrf }, body: JSON.stringify({ text: 'hello from a browser' }) });
        return answer.status;
      });
      const ok = signedIn && said === 200;
      console.log(`${kind}: ${ok ? 'ok' : 'FAILED'} (signed in: ${signedIn}, a change: ${said})`);
      if (!ok) failed++;
    } finally {
      await browser.close();
    }
  }
} finally {
  sim.stop();
}
process.exit(failed ? 1 : 0);
