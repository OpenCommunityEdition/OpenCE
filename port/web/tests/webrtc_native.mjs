// A native build's WebRTC (port/linux/src/p2p_webrtc.c) with browsers:
//
//     node port/web/tests/webrtc_native.mjs [chromium] [firefox]
//
// (with Playwright where Node finds it, and a C compiler). It builds
// webrtc_native.c for this computer (with posix_dtls.c, Mbed TLS and
// Monocypher), then has each browser open a data channel to it as internet
// play's signalling would have it (port/web/site/p2p.js makes up the native
// end's description the same way): ICE to its ICE-lite end, DTLS with the
// certificates both were told, and SCTP's association. The browser sends it
// messages of every size, split across SCTP's packets and not, which it
// sends back, and the browser checks them; then again with a tenth of the
// datagrams each way lost, when the channel must stay open (what was lost is
// given up, FORWARD-TSN) and carry the rest. No game data is needed.

import { spawn, spawnSync } from 'node:child_process';
import { mkdtempSync, readdirSync, rmSync } from 'node:fs';
import { createRequire } from 'node:module';
import { cpus, tmpdir } from 'node:os';
import path from 'node:path';

const require = createRequire(import.meta.url);
const playwright = require('playwright');

const root = path.resolve(path.dirname(new URL(import.meta.url).pathname), '../../..');
const browsers = process.argv.slice(2).length ? process.argv.slice(2) : ['chromium'];
const work = mkdtempSync(path.join(tmpdir(), 'opence-webrtc-'));

function fail(message) {
  console.error('FAILED: ' + message);
  process.exitCode = 1;
}

// ---------- the harness, built for this computer

async function build() {
  const compiler = process.env.CC || 'cc';
  const mbedtls = path.join(root, 'port/third_party/mbedtls');
  const monocypher = path.join(root, 'port/third_party/monocypher');
  const sources = [
    ...readdirSync(path.join(mbedtls, 'library')).filter((name) => name.endsWith('.c'))
      .map((name) => [path.join(mbedtls, 'library', name), ['-w']]),
    [path.join(root, 'port/linux/src/posix_dtls.c'), []],
    [path.join(root, 'port/linux/src/p2p_webrtc.c'), []],
    [path.join(root, 'port/linux/src/p2p_crypto.c'), []],
    [path.join(monocypher, 'monocypher.c'), ['-w']],
    [path.join(monocypher, 'monocypher-ed25519.c'), ['-w']],
    [path.join(root, 'port/web/tests/webrtc_native.c'), []],
  ];
  const flags = ['-std=gnu11', '-O1', '-g', '-D_GNU_SOURCE', `-I${path.join(mbedtls, 'include')}`,
    `-I${path.join(mbedtls, 'library')}`, `-I${monocypher}`, `-I${path.join(root, 'port/linux/src')}`];
  const objects = [];
  const queue = sources.map(([source, extra], index) => () => new Promise((resolve, reject) => {
    const object = path.join(work, `${index}.o`);
    objects.push(object);
    const child = spawn(compiler, [...flags, ...extra, '-c', source, '-o', object], { stdio: 'inherit' });
    child.on('exit', (code) => (code ? reject(new Error(`cannot compile ${source}`)) : resolve()));
  }));
  await Promise.all(Array.from({ length: Math.max(1, cpus().length) }, async () => {
    while (queue.length) await queue.shift()();
  }));
  const program = path.join(work, 'webrtc_native');
  const link = spawnSync(compiler, ['-o', program, ...objects, '-lpthread'], { stdio: 'inherit' });
  if (link.status) throw new Error('cannot link the harness');
  return program;
}

// ---------- one browser's run

function start(program, loss) {
  const secret = [...crypto.getRandomValues(new Uint8Array(32))].map((b) => b.toString(16).padStart(2, '0')).join('');
  const harness = spawn(program, [secret], { env: { ...process.env, HARNESS_LOSS: String(loss) } });
  const told = {};
  harness.stderr.on('data', (data) => process.stdout.write('  native: ' + data));
  const ready = new Promise((resolve, reject) => {
    harness.stdout.on('data', (data) => {
      for (const line of data.toString().split('\n')) {
        const [key, value] = line.split(' ');
        if (key) told[key] = value;
      }
      if (told.fingerprint) resolve();
    });
    harness.on('exit', () => reject(new Error('the harness ended')));
  });
  return { harness, told, ready };
}

async function run(program, kind, loss, rounds) {
  const { harness, told, ready } = start(program, loss);
  const browser = await playwright[kind].launch();
  try {
    await ready;
    const page = await browser.newPage();
    await page.setContent('<!doctype html><title>WebRTC</title>');
    const fingerprint = await page.evaluate(async () => {
      const pc = new RTCPeerConnection();
      const dc = pc.createDataChannel('opence', { negotiated: true, id: 0, ordered: false, maxRetransmits: 0 });
      dc.binaryType = 'arraybuffer';
      window.pc = pc;
      window.dc = dc;
      await pc.setLocalDescription(await pc.createOffer());
      window.mid = /^a=mid:(\S+)/m.exec(pc.localDescription.sdp)[1];
      return /^a=fingerprint:sha-256 ([0-9A-F:]+)/im.exec(pc.localDescription.sdp)[1].replace(/:/g, '').toLowerCase();
    });
    harness.stdin.write(`browser ${fingerprint}\n`);
    // (the answer p2p.js makes up for a native build)
    const answer = (mid) => [
      'v=0', 'o=- 1 2 IN IP4 127.0.0.1', 's=-', 't=0 0', `a=group:BUNDLE ${mid}`, 'a=ice-lite',
      'm=application 9 UDP/DTLS/SCTP webrtc-datachannel', 'c=IN IP4 0.0.0.0', `a=mid:${mid}`,
      `a=ice-ufrag:${told.ufrag}`, `a=ice-pwd:${told.password}`,
      `a=fingerprint:sha-256 ${told.fingerprint.toUpperCase().match(/../g).join(':')}`,
      'a=setup:passive', 'a=sctp-port:5000', 'a=max-message-size:262144',
      `a=candidate:1 1 udp 2130706431 ${told.address} ${told.port} typ host`, '',
    ].join('\r\n');
    const result = await page.evaluate(async ({ sdp, rounds }) => {
      await pc.setRemoteDescription({ type: 'answer', sdp });
      await new Promise((resolve, reject) => {
        if (dc.readyState === 'open') resolve();
        dc.onopen = resolve;
        setTimeout(() => reject(new Error(`not open: ICE ${pc.iceConnectionState}, ${pc.connectionState}`)), 20000);
      });
      // (as large as a tunnel packet: p2p.c's MAXIMUM_PACKET_SIZE)
      const sizes = [1, 10, 100, 1000, 1100, 1101, 1200, 1431];
      const sent = new Map();
      let echoed = 0;
      let wrong = 0;
      let lastEchoed = -1;
      dc.onmessage = (event) => {
        const bytes = new Uint8Array(event.data);
        const number = bytes.length >= 2 ? bytes[0] | bytes[1] << 8 : -1;
        const expected = sent.get(number);
        if (!expected || expected.length !== bytes.length || expected.some((b, i) => b !== bytes[i])) wrong++;
        else {
          echoed++;
          lastEchoed = Math.max(lastEchoed, number);
        }
      };
      let number = 0;
      for (let round = 0; round < rounds; round++) {
        for (const size of sizes) {
          const bytes = new Uint8Array(Math.max(size, 2));
          for (let i = 2; i < bytes.length; i++) bytes[i] = (i * 7 + number) & 255;
          bytes[0] = number & 255;
          bytes[1] = number >> 8;
          sent.set(number++, bytes);
          dc.send(bytes);
        }
        await new Promise((resolve) => setTimeout(resolve, 25));
      }
      await new Promise((resolve) => setTimeout(resolve, 2000));
      return { sent: number, echoed, wrong, lastEchoed, open: dc.readyState === 'open' };
    }, { sdp: answer(await page.evaluate(() => window.mid)), rounds });
    return result;
  } finally {
    await browser.close();
    harness.stdin.write('quit\n');
    harness.kill();
  }
}

try {
  const program = await build();
  for (const kind of browsers) {
    const clean = await run(program, kind, 0, 40);
    console.log(`${kind}: ${clean.echoed} of ${clean.sent} messages back`);
    if (clean.wrong || clean.echoed !== clean.sent) fail(`${kind}: ${JSON.stringify(clean)}`);
    const lossy = await run(program, kind, 10, 120);
    console.log(`${kind}, a tenth of the datagrams lost: ${lossy.echoed} of ${lossy.sent} messages back`);
    // (a message comes back if its every datagram passed each way: about
    // two thirds for the larger, most for the smaller; and to the end)
    if (lossy.wrong || !lossy.open || lossy.echoed < lossy.sent / 2 || lossy.lastEchoed < lossy.sent - 64) {
      fail(`${kind} with losses: ${JSON.stringify(lossy)}`);
    }
  }
} catch (error) {
  fail(String(error && error.stack ? error.stack : error));
} finally {
  rmSync(work, { recursive: true, force: true });
}
