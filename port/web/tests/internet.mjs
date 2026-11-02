// Internet play between browsers and the native builds, each game played
// on its own (debug.network_test):
//
//     node port/web/tests/internet.mjs <site> <maps folder> [<halo>]
//
// (with Playwright where Node finds it, mosquitto and xvfb-run on the PATH;
// the maps folder holds ui.map and the multiplayer maps, which each
// browser's profile imports, about 300 MB each). It starts a local broker
// with a WebSocket listener for the browsers and a plain one for the native
// build (<halo>, build/linux/halo by default), serves the site, and plays:
//
// - two browsers, one hosting and the other joining by its invite (?join=);
// - the native build hosting a public game, which a browser finds in the
//   server browser and joins (debug.network_test "browse");
// - a browser hosting a public game, which the native build joins the same
//   way;
// - the native build as a dedicated server (halo --server, listed through
//   the brokers: server.list_on_brokers), which a browser finds in the
//   server browser and joins. The server runs from a copy of <halo> in the
//   test's folder, its settings, saves and logs there too.
//
// Each must reach the other over WebRTC (the native build's ICE-lite end and
// DTLS server, port/linux/src/p2p_webrtc.c) and play: both log the two
// players. Run it with no copy of the game of your own open: the native
// build's system link would find it.

import { spawn } from 'node:child_process';
import { copyFileSync, existsSync, mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { setTimeout as sleep } from 'node:timers/promises';

const require = createRequire(import.meta.url);
const { chromium } = require('playwright');

const here = path.dirname(new URL(import.meta.url).pathname);
const root = path.resolve(here, '../../..');
const site = path.resolve(process.argv[2] || 'build/web/site');
const maps = path.resolve(process.argv[3] || 'assets/maps');
const halo = path.resolve(process.argv[4] || 'build/linux/halo');
const PLAY_SECONDS = 45;
const sitePort = 8000 + Math.floor(Math.random() * 1000);
const tcpPort = 18000 + Math.floor(Math.random() * 1000);
const wsPort = tcpPort + 1000;
const work = mkdtempSync(path.join(tmpdir(), 'opence-internet-'));
writeFileSync(path.join(work, 'mosquitto.conf'),
  `listener ${tcpPort} 127.0.0.1\nprotocol mqtt\nlistener ${wsPort} 127.0.0.1\nprotocol websockets\nallow_anonymous true\n`);
writeFileSync(path.join(work, 'brokers.txt'), `127.0.0.1:${tcpPort}\n`);
const broker = spawn('mosquitto', ['-c', path.join(work, 'mosquitto.conf')], { stdio: 'ignore' });
const server = spawn('python3', [path.join(root, 'tools/web_serve.py'), site, '--port', String(sitePort)],
  { stdio: 'ignore', env: { ...process.env, WEB_SERVE_QUIET: '1' } });
const pageUrl = `http://localhost:${sitePort}/`;
const settings = `set=HALO_NET_JOIN_FROM_CLIPBOARD=0&brokers=ws://127.0.0.1:${wsPort}`;

function fail(message) {
  console.error('FAILED: ' + message);
  process.exitCode = 1;
}

// a side of a game: its log's lines, and the invite it hosts with
function side(name) {
  const result = { name, lines: [], invite: null };
  result.add = (text) => {
    for (const line of String(text).split('\n')) {
      if (!line) continue;
      result.lines.push(line);
      const invite = /halo:\/\/join\/([0-9a-f]{64})/.exec(line);
      if (invite && !result.invite) result.invite = invite[1];
      if (/Internet play|network test: (joining|browsing|hosting)/.test(line)) console.log(`  ${name} | ${line.slice(0, 160)}`);
    }
  };
  // both players, once the game runs
  result.playing = () => result.lines.some((line) => /network test: tick .*player 0: .*player 1: /.test(line));
  return result;
}

async function browser(name) {
  const context = await chromium.launchPersistentContext(path.join(work, 'profile-' + name), {
    headless: true, viewport: { width: 800, height: 450 },
  });
  const page = context.pages()[0] || await context.newPage();
  // Local browser checks must not wait for the public analytics service.
  await page.route('https://nexus.mrbruh.com/**', (route) =>
    route.fulfill({ contentType: 'text/javascript', body: '', headers: { 'Access-Control-Allow-Origin': '*' } }));
  await page.goto(pageUrl);
  await page.waitForFunction(() => document.querySelectorAll('#checks li').length >= 5, null, { timeout: 60000 });
  await page.setInputFiles('#folder-input', maps);
  await page.waitForFunction(() => /Copied|cannot/.test(document.querySelector('#import-text').textContent), null,
    { timeout: 600000 });
  const log = side(name);
  page.on('console', (message) => log.add(message.text()));
  return { context, page, log };
}

async function play(game, query) {
  await game.page.goto(`${pageUrl}?${query}&${settings}`);
  await game.page.waitForFunction(() => !document.querySelector('#play').disabled, null, { timeout: 60000 });
  await game.page.click('#play');
}

function native(name, test, extra = {}) {
  const log = side(name);
  const env = { ...process.env, HALO_DATA_ROOT: path.dirname(maps), HALO_NETWORK_TEST: test, HALO_NULL_RENDERER: '1',
    HALO_HIDDEN_WINDOW: '1', SDL_VIDEO_DRIVER: 'x11', HALO_NET_BROKERS_FILE: path.join(work, 'brokers.txt'),
    HALO_NET_JOIN_FROM_CLIPBOARD: '0', HALO_NET_ALLOW_UPNP: 'false', HALO_DISCORD_APPLICATION: '',
    HALO_EXIT_AFTER: '600', ...extra };
  // (its saves and home the test's, not the person's running it)
  Object.assign(env, { HALO_SAVE_ROOT: path.join(work, name + '-saves'), HOME: path.join(work, name + '-home'),
    XDG_DATA_HOME: path.join(work, name + '-xdg') });
  delete env.WAYLAND_DISPLAY;
  const child = spawn('xvfb-run', ['-a', '-s', '-screen 0 640x480x24', halo], { cwd: path.dirname(halo), env, detached: true });
  child.stdout.on('data', (data) => log.add(data));
  child.stderr.on('data', (data) => log.add(data));
  log.stop = () => {
    try {
      process.kill(-child.pid, 'SIGKILL');
    } catch {
      // (ended already)
    }
  };
  running.push(log);
  return log;
}

// the native build as a dedicated server, from a copy of it in the test's
// folder (its config.toml beside it): its standard output's lines
function dedicated(name) {
  const log = side(name);
  const folder = path.join(work, name);
  mkdirSync(folder, { recursive: true });
  copyFileSync(halo, path.join(folder, 'halo'));
  const sdl = path.join(path.dirname(halo), 'libSDL3.so.0');
  if (existsSync(sdl)) copyFileSync(sdl, path.join(folder, 'libSDL3.so.0'));
  // (none of the environment's HALO_* settings: the server's are the test's)
  const inherited = Object.fromEntries(Object.entries(process.env).filter(([key]) => !key.startsWith('HALO_')));
  const env = { ...inherited, HALO_DATA_ROOT: path.dirname(maps), HALO_LOG_ROOT: folder,
    HALO_SAVE_ROOT: path.join(folder, 'saves'), HOME: folder, XDG_DATA_HOME: path.join(folder, 'xdg'),
    HALO_NET_BROKERS_FILE: path.join(work, 'brokers.txt'), HALO_NET_ONLINE: 'true', HALO_DISCORD_APPLICATION: '',
    HALO_SERVER_LIST_ON_BROKERS: 'true', HALO_SERVER_MASTER_PUBLISH: 'false', HALO_SERVER_WEB_PORT: '0',
    HALO_SERVER_LOBBY_SECONDS: '3', HALO_SERVER_ROTATION: 'bloodgulch:slayer', HALO_SERVER_NAME: 'Web Test',
    HALO_NET_TUNNEL_PORT: String(tcpPort + 2000), HALO_NET_ALLOW_UPNP: 'false' };
  const child = spawn(path.join(folder, 'halo'), ['--server'], { cwd: folder, env });
  child.stdout.on('data', (data) => log.add(data));
  child.stderr.on('data', (data) => log.add(data));
  log.stop = () => {
    if (child.exitCode === null && child.signalCode === null) child.kill('SIGTERM');
  };
  running.push(log);
  log.ready = () => log.lines.some((line) => /server: ready/.test(line));
  log.joined = () => log.lines.some((line) => /player: .* joined/.test(line)) &&
    log.lines.some((line) => /game: started/.test(line));
  return log;
}

async function until(test, seconds) {
  for (let waited = 0; waited < seconds && !test(); waited++) await sleep(1000);
  return test();
}

const opened = [];
// (the native builds started, each stopped at the end however the test ends)
const running = [];
try {
  await sleep(1500);
  const first = await browser('first');
  const second = await browser('second');
  opened.push(first, second);

  console.log('two browsers, by invite');
  await play(first, 'set=HALO_NETWORK_TEST=host:bloodgulch');
  if (!await until(() => first.log.invite, 120)) fail('the browser hosting made no invite');
  else {
    await play(second, `join=${first.log.invite}&set=HALO_NETWORK_TEST=join`);
    if (!await until(() => first.log.playing() && second.log.playing(), 120 + PLAY_SECONDS)) {
      fail('the two browsers did not play together');
    }
  }

  console.log('the native build hosting, a browser joining from the server browser');
  first.log.lines.length = 0;
  await first.page.goto('about:blank');
  await second.page.goto('about:blank');
  const host = native('native', 'host:bloodgulch', { HALO_NETWORK_TEST_PUBLIC: 'true' });
  second.log.lines.length = 0;
  if (!await until(() => host.invite, 120)) fail('the native build hosting made no invite');
  else {
    await play(second, 'set=HALO_NETWORK_TEST=browse');
    if (!await until(() => host.playing() && second.log.playing(), 120 + PLAY_SECONDS)) {
      fail('the native build and the browser did not play together');
    }
    if (!host.lines.some((line) => /WebRTC connected/.test(line))) fail('the native build had no WebRTC connection');
  }
  host.stop();
  await second.page.goto('about:blank');

  console.log('a browser hosting, the native build joining from the server browser');
  first.log.lines.length = 0;
  await play(first, 'set=HALO_NETWORK_TEST=host:bloodgulch&set=HALO_NETWORK_TEST_PUBLIC=true');
  if (!await until(() => first.log.invite, 120)) fail('the browser hosting made no invite');
  else {
    const joiner = native('native', 'browse');
    if (!await until(() => joiner.playing() && first.log.playing(), 120 + PLAY_SECONDS)) {
      fail('the browser and the native build did not play together');
    }
    joiner.stop();
  }

  console.log('the native build as a dedicated server, a browser joining from the server browser');
  await first.page.goto('about:blank');
  const dedicatedServer = dedicated('server');
  if (!await until(() => dedicatedServer.ready(), 120)) fail('the dedicated server was not ready');
  else {
    second.log.lines.length = 0;
    await play(second, 'set=HALO_NETWORK_TEST=browse');
    // (the server has no player: the browser's player alone, in a game the server started)
    if (!await until(() => dedicatedServer.joined() &&
      second.log.lines.some((line) => /network test: tick .*player 0: /.test(line)), 120 + PLAY_SECONDS)) {
      fail('the browser did not play on the dedicated server');
    }
    if (!dedicatedServer.lines.some((line) => /WebRTC connected/.test(line))) {
      fail('the dedicated server had no WebRTC connection');
    }
  }
  if (!process.exitCode) console.log('internet play: ok');
} catch (error) {
  fail(String(error && error.stack ? error.stack : error));
} finally {
  for (const side of running) side.stop();
  for (const game of opened) await game.context.close().catch(() => {});
  server.kill();
  broker.kill();
  rmSync(work, { recursive: true, force: true });
}
