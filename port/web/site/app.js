/*
APP.JS

The launcher page (index.html): it checks that the browser can run the game,
copies the game data in from the player's disc image (xiso-worker.js), and
starts the game (halo.js and halo.wasm: tools/web_build.py), whose canvas
then fills the page.

- The game's threads need SharedArrayBuffer, so a cross-origin isolated
  page: GitHub Pages cannot send the headers for it, the service worker
  (sw.js) adds them, and the first visit loads the page once more under it.
- The game reads its maps, and keeps its saved games, config.toml and
  debug.txt, in the Origin Private File System (port/web/src/web_main.c
  mounts it at /data). Each origin has its own: the github.io address and
  the site's own domain do not share them.
- Query parameters, for testing: ?set=NAME=value (any number) passes a
  setting's environment variable (port/linux/README.md, the settings
  table), ?room=<secret> joins a room (net.js; ?brokers= names other
  message brokers, for internet play's signalling too), ?menu=<screen> opens
  a menu screen (debug.menu_open), ?map=<level> starts a level (a10, or
  levels\a10\a10: the init.txt the game reads, which the page removes again
  the next time), ?play starts the game at once when it can.
- ?join=<invite> (a halo://join/ link's code, which native builds and the
  game here make: port/linux/src/p2p.c) joins that internet game once the
  game starts.
*/

'use strict';

const $ = (id) => document.getElementById(id);
const params = new URLSearchParams(location.search);
// the Xbox window ends the game's memory: tools/web_build.py WEB_MEMORY_BYTES
const WEB_MEMORY_BYTES = 0x88000000;
const MAXIMUM_LOG_LINES = 20000;

// ---------- the log: the game's output, for the player to send

const log = [];

function logLine(text) {
  for (const line of String(text).replace(/\r/g, '').split('\n')) {
    if (line === '') continue;
    log.push(line);
  }
  if (log.length > MAXIMUM_LOG_LINES) log.splice(0, log.length - MAXIMUM_LOG_LINES);
}

function download(name, blob) {
  const link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = name;
  document.body.append(link);
  link.click();
  link.remove();
  setTimeout(() => URL.revokeObjectURL(link.href), 10000);
}

async function readStorageText(name) {
  try {
    const root = await navigator.storage.getDirectory();
    const file = await (await root.getFileHandle(name)).getFile();
    return await file.text();
  } catch {
    return null;
  }
}

async function downloadLog() {
  let text = `OpenCE web ${JSON.stringify(stamp)}\n${navigator.userAgent}\n\n` + log.join('\n') + '\n';
  // (debug.txt, which the game writes, when it is not open: before or after
  // a game)
  if (!gameStarted || gameStopped) {
    const debug = await readStorageText('debug.txt');
    if (debug) text += '\n---------- debug.txt\n' + debug.slice(-2000000);
  }
  download('opence-web-log.txt', new Blob([text], { type: 'text/plain' }));
}

// ---------- errors: the game's own, not a browser extension's

function isExtensionNoise(message, source) {
  const text = String(message || '') + ' ' + String(source || '');
  return /chrome-extension:|moz-extension:|safari-extension:|MetaMask|ethereum/i.test(text);
}

window.addEventListener('error', (event) => {
  if (isExtensionNoise(event.message, event.filename)) return;
  logLine('page error: ' + (event.error && event.error.stack ? event.error.stack : event.message));
});
window.addEventListener('unhandledrejection', (event) => {
  const reason = event.reason;
  const text = reason && reason.stack ? reason.stack : String(reason);
  if (isExtensionNoise(text)) return;
  logLine('page error: ' + text);
});

// ---------- the build (version.json) and its updates

let stamp = {};

async function loadStamp() {
  try {
    stamp = await (await fetch('version.json', { cache: 'no-cache' })).json();
  } catch {
    stamp = {};
  }
  $('about-build').textContent = stamp.build ? `${stamp.build} (${stamp.flavor})` : `local (${stamp.flavor || 'unknown'})`;
  $('about-commit').textContent = stamp.commit ? stamp.commit.slice(0, 12) : '–';
  $('about-network').textContent = stamp.network_version != null ? String(stamp.network_version) : '–';
}

async function checkForUpdate() {
  if (!navigator.serviceWorker || !navigator.serviceWorker.controller || !stamp.version) return;
  try {
    const latest = await (await fetch('version.json?latest', { cache: 'no-store' })).json();
    if (latest.version && latest.version !== stamp.version) {
      $('update-text').textContent = latest.build ? `Build ${latest.build} is out.` : 'A new build is out.';
      $('update').hidden = false;
    }
  } catch {
    // offline: the build there is plays
  }
}

$('update-button').addEventListener('click', () => {
  $('update-button').disabled = true;
  $('update-text').textContent = 'Updating…';
  navigator.serviceWorker.addEventListener('message', (event) => {
    if (event.data === 'updated') location.reload();
    else if (event.data === 'update-failed') $('update-text').textContent = 'The update could not be downloaded.';
  });
  navigator.serviceWorker.controller.postMessage('update');
});

// ---------- cross-origin isolation (sw.js)

async function isolate() {
  if (window.crossOriginIsolated || !('serviceWorker' in navigator) || !window.isSecureContext) return;
  try {
    await navigator.serviceWorker.register('sw.js');
    await navigator.serviceWorker.ready;
  } catch (error) {
    logLine('service worker: ' + error);
    return;
  }
  // once: a page the worker still does not isolate stays as it is
  if (!sessionStorage.getItem('opence-isolating')) {
    sessionStorage.setItem('opence-isolating', '1');
    $('checks-note').textContent = 'Preparing the page (it loads once more)…';
    $('checks-note').hidden = false;
    location.reload();
    await new Promise(() => {});
  }
}

// ---------- the browser's checks

const results = {};

function addCheck(name, state, text, detail) {
  results[name] = state;
  const item = document.createElement('li');
  item.className = state;
  item.append(text);
  if (detail) {
    const span = document.createElement('span');
    span.className = 'detail';
    span.textContent = ' ' + detail;
    item.append(span);
  }
  $('checks').append(item);
}

function workerWebGL2() {
  return new Promise((resolve) => {
    const source = "try { postMessage(!!new OffscreenCanvas(1, 1).getContext('webgl2')); } catch (e) { postMessage(false); }";
    let worker;
    try {
      worker = new Worker(URL.createObjectURL(new Blob([source], { type: 'text/javascript' })));
    } catch {
      resolve(false);
      return;
    }
    const timer = setTimeout(() => { worker.terminate(); resolve(false); }, 5000);
    worker.onmessage = (event) => { clearTimeout(timer); worker.terminate(); resolve(event.data === true); };
    worker.onerror = () => { clearTimeout(timer); worker.terminate(); resolve(false); };
  });
}

function memoryAvailable() {
  try {
    const pages = WEB_MEMORY_BYTES / 65536;
    new WebAssembly.Memory({ initial: pages, maximum: pages, shared: true });
    return true;
  } catch {
    return false;
  }
}

async function runChecks() {
  $('checks').textContent = '';
  addCheck('secure', window.isSecureContext ? 'good' : 'bad', 'A secure page (https)');
  addCheck('isolated', window.crossOriginIsolated ? 'good' : 'bad', 'Threads (cross-origin isolation)',
    window.crossOriginIsolated ? '' : 'reload the page; a private window may not allow it');
  const webgl = typeof OffscreenCanvas !== 'undefined' && await workerWebGL2();
  addCheck('webgl', webgl ? 'good' : 'bad', 'WebGL 2 from a worker');
  const opfs = !!(navigator.storage && navigator.storage.getDirectory);
  addCheck('opfs', opfs ? 'good' : 'bad', "The site's private storage (OPFS)");
  const memory = window.crossOriginIsolated && memoryAvailable();
  addCheck('memory', memory ? 'good' : 'bad', 'Memory for the game (2.1 GB)',
    memory ? '' : 'this browser could not give the game that much: a 64-bit browser on a computer has it');
  const required = ['secure', 'isolated', 'webgl', 'opfs', 'memory'];
  return required.every((name) => results[name] === 'good');
}

// ---------- the game data (xiso-worker.js)

let dataReady = false;

function formatBytes(bytes) {
  return bytes >= 1e9 ? (bytes / 1e9).toFixed(2) + ' GB' : Math.round(bytes / 1e6) + ' MB';
}

async function refreshData() {
  dataReady = false;
  let text = 'No game data yet: choose your disc image of Halo: Combat Evolved for the Xbox.';
  try {
    const root = await navigator.storage.getDirectory();
    const maps = await root.getDirectoryHandle('maps');
    const marker = await (await maps.getFileHandle('.complete')).getFile();
    const complete = JSON.parse(await marker.text());
    text = `${complete.files.length} files of maps (${formatBytes(complete.bytes)}), ready.`;
    dataReady = true;
  } catch {
    // none, or a copy that did not finish
  }
  $('data-status').textContent = text;
  $('delete-data').disabled = !dataReady;
  await refreshStorage();
  updatePlay();
}

async function refreshStorage() {
  if (!navigator.storage || !navigator.storage.estimate) return;
  const { usage, quota } = await navigator.storage.estimate();
  const persisted = navigator.storage.persisted ? await navigator.storage.persisted() : false;
  $('storage-status').textContent = `This site keeps ${formatBytes(usage || 0)} of the ${formatBytes(quota || 0)} ` +
    `the browser allows it. ${persisted ? 'The browser keeps it.' : 'The browser may clear it when space runs low.'}`;
  $('persist').disabled = persisted;
}

function importData(message) {
  const worker = new Worker('xiso-worker.js');
  $('import-progress').hidden = false;
  $('import-bar').style.width = '0';
  $('import-text').textContent = 'Reading…';
  setImporting(true);
  // (asked before the copy: the maps are 2 GB the browser should not drop)
  if (navigator.storage && navigator.storage.persist) navigator.storage.persist().catch(() => {});
  worker.onmessage = async (event) => {
    const data = event.data;
    if (data.type === 'progress') {
      $('import-bar').style.width = (100 * data.done / data.total).toFixed(1) + '%';
      $('import-text').textContent = `Copying ${data.file}: ${formatBytes(data.done)} of ${formatBytes(data.total)}`;
      return;
    }
    worker.terminate();
    setImporting(false);
    if (data.type === 'done') {
      $('import-text').textContent = `Copied ${data.files} files.`;
      logLine(`import: ${data.files} files, ${data.bytes} bytes`);
    } else {
      $('import-text').textContent = data.message;
      logLine('import failed: ' + data.message);
    }
    await refreshData();
  };
  worker.onerror = (event) => {
    worker.terminate();
    setImporting(false);
    $('import-text').textContent = 'The copy failed: ' + event.message;
  };
  worker.postMessage(message);
}

function setImporting(importing) {
  for (const id of ['image-label', 'folder-label']) $(id).classList.toggle('disabled', importing);
  $('image-input').disabled = importing;
  $('folder-input').disabled = importing;
  $('play').disabled = importing || !canPlay();
}

$('image-input').addEventListener('change', (event) => {
  const file = event.target.files[0];
  event.target.value = '';
  if (file) importData({ type: 'image', file });
});
$('folder-input').addEventListener('change', (event) => {
  const files = [...event.target.files];
  event.target.value = '';
  if (files.length) importData({ type: 'folder', files });
});

$('delete-data').addEventListener('click', async () => {
  if (!confirm('Delete the maps copied into this browser? Saved games stay.')) return;
  const root = await navigator.storage.getDirectory();
  await root.removeEntry('maps', { recursive: true }).catch(() => {});
  // (and the game's copies of them, save/z/cache000.map and on)
  try {
    const drive = await (await root.getDirectoryHandle('save')).getDirectoryHandle('z');
    for await (const [name] of drive.entries()) {
      if (/^cache\d+\.map$/i.test(name)) await drive.removeEntry(name).catch(() => {});
    }
  } catch {
    // no saved games yet
  }
  await refreshData();
});

$('persist').addEventListener('click', async () => {
  if (navigator.storage && navigator.storage.persist) await navigator.storage.persist();
  await refreshStorage();
});

// ---------- saved games out, as a zip (stored, not compressed)

const crcTable = (() => {
  const table = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1;
    table[n] = c >>> 0;
  }
  return table;
})();

function crc32(bytes) {
  let crc = 0xFFFFFFFF;
  for (let i = 0; i < bytes.length; i++) crc = crcTable[(crc ^ bytes[i]) & 0xFF] ^ (crc >>> 8);
  return (crc ^ 0xFFFFFFFF) >>> 0;
}

function zip(entries) {
  const encoder = new TextEncoder();
  const parts = [];
  const central = [];
  let offset = 0;
  for (const { name, data } of entries) {
    const nameBytes = encoder.encode(name);
    const crc = crc32(data);
    const local = new DataView(new ArrayBuffer(30));
    local.setUint32(0, 0x04034b50, true);
    local.setUint16(4, 20, true);
    local.setUint16(6, 0x0800, true);
    local.setUint32(14, crc, true);
    local.setUint32(18, data.length, true);
    local.setUint32(22, data.length, true);
    local.setUint16(26, nameBytes.length, true);
    const header = new DataView(new ArrayBuffer(46));
    header.setUint32(0, 0x02014b50, true);
    header.setUint16(4, 20, true);
    header.setUint16(6, 20, true);
    header.setUint16(8, 0x0800, true);
    header.setUint32(16, crc, true);
    header.setUint32(20, data.length, true);
    header.setUint32(24, data.length, true);
    header.setUint16(28, nameBytes.length, true);
    header.setUint32(42, offset, true);
    parts.push(local.buffer, nameBytes, data);
    central.push(header.buffer, nameBytes);
    offset += 30 + nameBytes.length + data.length;
  }
  const centralSize = central.reduce((sum, part) => sum + part.byteLength, 0);
  const end = new DataView(new ArrayBuffer(22));
  end.setUint32(0, 0x06054b50, true);
  end.setUint16(8, entries.length, true);
  end.setUint16(10, entries.length, true);
  end.setUint32(12, centralSize, true);
  end.setUint32(16, offset, true);
  return new Blob([...parts, ...central, end.buffer], { type: 'application/zip' });
}

// the saved games and profiles, not the maps the game keeps a copy of as the
// Xbox's hard disk did (save/z/cache000.map and on: hundreds of MB the game
// makes again)
async function collect(directory, prefix, entries) {
  for await (const [name, handle] of directory.entries()) {
    if (/^cache\d+\.map$/i.test(name) || (prefix === 'save/z/' && name === 'maps')) continue;
    if (handle.kind === 'directory') await collect(handle, prefix + name + '/', entries);
    else entries.push({ name: prefix + name, data: new Uint8Array(await (await handle.getFile()).arrayBuffer()) });
  }
}

$('export-saves').addEventListener('click', async () => {
  if (gameStarted && !gameStopped) {
    alert('Quit the game first: its files are open while it runs.');
    return;
  }
  const entries = [];
  const root = await navigator.storage.getDirectory();
  try {
    await collect(await root.getDirectoryHandle('save'), 'save/', entries);
  } catch {
    // no saved games yet
  }
  const config = await readStorageText('config.toml');
  if (config != null) entries.push({ name: 'config.toml', data: new TextEncoder().encode(config) });
  if (!entries.length) {
    alert('There are no saved games yet.');
    return;
  }
  download('opence-saves.zip', zip(entries));
});

$('download-log').addEventListener('click', downloadLog);
$('fatal-log-download').addEventListener('click', downloadLog);
$('fatal-reload').addEventListener('click', () => location.reload());

// ---------- the game

let checksPassed = false;
let gameStarted = false;
let gameStopped = false;
let wakeLock = null;

function canPlay() {
  return checksPassed && dataReady && !gameStarted;
}

function updatePlay() {
  $('play').disabled = !canPlay();
  $('play-note').textContent = !checksPassed ? 'This browser cannot run the game (above).' :
    !dataReady ? 'The game data is needed first.' : '';
}

// init.txt in the data folder: the console commands the game runs as it
// starts. ?map= writes one, and the page removes it on the next visit
// without ?map=, only if it wrote it.
const INIT_MARKER = 'opence-page-wrote-init';

async function prepareInit() {
  const root = await navigator.storage.getDirectory();
  let wrote = false;
  try {
    wrote = localStorage.getItem(INIT_MARKER) === '1';
  } catch {
    // (no localStorage: nothing to undo)
  }
  if (params.has('map')) {
    const name = params.get('map');
    const level = name.includes('\\') ? name : `levels\\${name}\\${name}`;
    if (!/^[A-Za-z0-9_\\]+$/.test(level)) return;
    const handle = await root.getFileHandle('init.txt', { create: true });
    const writable = await handle.createWritable();
    await writable.write(`map_name ${level}\r\n`);
    await writable.close();
    try {
      localStorage.setItem(INIT_MARKER, '1');
    } catch {
      // as above
    }
  } else if (wrote) {
    await root.removeEntry('init.txt').catch(() => {});
    try {
      localStorage.removeItem(INIT_MARKER);
    } catch {
      // as above
    }
  }
}

// ---------- rooms (net.js)

function roomLink(secret) {
  return `${location.origin}${location.pathname}?room=${secret}`;
}

function showRoom(status) {
  const inRoom = !!status;
  $('room-create').hidden = inRoom || gameStarted;
  $('room-copy').hidden = !inRoom;
  $('room-leave').hidden = !inRoom || gameStarted;
  if (!inRoom) return;
  const brokers = status.brokers ? '' : ' (finding the message brokers…)';
  $('room-status').textContent = status.peers ?
    `In a room with ${status.connected} of ${status.peers} other browsers connected${brokers}. Your address: ${status.address}.` :
    `In a room, alone so far${brokers}: send its link to your friends.`;
}

async function joinRoom(secret) {
  const brokers = (params.get('brokers') || '').split(',').filter((url) => /^wss?:\/\//.test(url));
  showRoom(await window.HaloNet.join(secret, brokers));
  history.replaceState(null, '', `?${new URLSearchParams({ ...Object.fromEntries(params), room: secret })}`);
}

window.HaloNet.onChange(showRoom);

$('room-create').addEventListener('click', () => joinRoom(window.HaloNet.newSecret()));
$('room-copy').addEventListener('click', async () => {
  const status = window.HaloNet.status();
  if (!status) return;
  try {
    await navigator.clipboard.writeText(roomLink(status.secret));
    $('room-copy').textContent = 'Copied';
    setTimeout(() => { $('room-copy').textContent = "Copy the room's link"; }, 1500);
  } catch {
    prompt("The room's link:", roomLink(status.secret));
  }
});
$('room-leave').addEventListener('click', () => {
  window.HaloNet.leave();
  params.delete('room');
  history.replaceState(null, '', params.toString() ? `?${params}` : location.pathname);
});

function gameArguments() {
  const args = [];
  // (the game's address in the room: port/web/src/web_net.c)
  if (window.HaloNet.address()) args.push('--HALO_WEB_ADDRESS=' + window.HaloNet.address());
  for (const value of params.getAll('set')) {
    if (/^[A-Z0-9_]+=/.test(value)) args.push('--' + value);
  }
  if (params.has('menu')) args.push('--HALO_MENU_OPEN=' + params.get('menu'));
  // internet play's brokers (port/linux/src/p2p_signal.c), if not its own
  const brokers = (params.get('brokers') || '').split(',').filter((url) => /^wss?:\/\/[^,\s]+$/.test(url));
  if (brokers.length) args.push('--HALO_WEB_BROKERS=' + brokers.join(','));
  // an invite to join (its code, or a whole link)
  const invite = /([0-9a-f]{64})\s*$/i.exec(params.get('join') || '');
  if (invite) args.push('halo://join/' + invite[1].toLowerCase());
  return args;
}

function showFatal(text) {
  gameStopped = true;
  $('fatal-text').textContent = text;
  $('fatal-log').textContent = log.slice(-60).join('\n');
  $('fatal').hidden = false;
  if (document.exitPointerLock) document.exitPointerLock();
}

// a game hosted for the internet: its invite as this page's address with
// ?join= (which native builds take too), and a button that copies it (a
// page may write the clipboard only when it is clicked)
function showInvite(text) {
  const code = /([0-9a-f]{64})/.exec(text);
  if (!code) return;
  $('invite-link').value = `${location.origin}${location.pathname}?join=${code[1]}`;
  $('invite-copy').textContent = 'Copy';
  $('invite').hidden = false;
}

$('invite-copy').addEventListener('click', async () => {
  try {
    await navigator.clipboard.writeText($('invite-link').value);
    $('invite-copy').textContent = 'Copied';
  } catch {
    $('invite-link').select();
  }
});
$('invite-close').addEventListener('click', () => { $('invite').hidden = true; });

function loadScript(src) {
  return new Promise((resolve, reject) => {
    const script = document.createElement('script');
    script.src = src;
    script.onload = resolve;
    script.onerror = () => reject(new Error('could not load ' + src));
    document.head.append(script);
  });
}

async function play() {
  if (!canPlay()) return;
  gameStarted = true;
  watchFrames();
  $('launcher').hidden = true;
  $('update').hidden = true;
  $('game').hidden = false;
  const canvas = $('canvas');
  canvas.focus();
  try {
    if (navigator.wakeLock) wakeLock = await navigator.wakeLock.request('screen');
  } catch {
    // (a phone may still dim)
  }
  try {
    await prepareInit();
    await loadScript('halo.js');
    const options = {
      canvas,
      arguments: gameArguments(),
      print: (text) => { logLine(text); console.log(text); },
      printErr: (text) => {
        logLine(text);
        console.warn(text);
        if (!$('starting').hidden && /data root:/.test(text)) $('starting-text').textContent = 'Loading the menus…';
      },
      haloNetSend: (address, reliable, bytes) => window.HaloNet.send(address, reliable, bytes),
      // internet play's WebRTC connections (p2p.js)
      haloP2P: (command, connection, bytes) => window.HaloP2P.command(command, connection, bytes),
      haloMessage: (kind, text) => {
        logLine(text);
        if (kind === 2) showInvite(text);
        if (kind === 3) showFatal(text);
      },
      onAbort: (what) => showFatal('The game stopped: ' + what),
      onExit: (status) => {
        if (status) showFatal(`The game stopped (status ${status}). The log below says why.`);
        else location.reload();
      },
    };
    // (the module these options become, which p2p.js calls into as soon as
    // the game asks it for a connection)
    window.HaloP2P.attach(options);
    game = await window.createHalo(options);
    // the on-screen touch controls (touch.js), and the room's frames (net.js)
    window.HaloTouch.start(game, $('touch'));
    window.HaloNet.attach(game);
  } catch (error) {
    showFatal(String(error && error.message ? error.message : error));
  }
}

// The game counts the frames it shows in its memory (web_state,
// port/web/src/web_main.c), which the page reads: while the count stands
// still, the game is starting or loading a map, and the page says so.
let game = null;

function watchFrames() {
  let shown = -1;
  let changed = performance.now();
  const timer = setInterval(() => {
    if (gameStopped) {
      clearInterval(timer);
      $('starting').hidden = true;
      return;
    }
    if (!game || !game._web_state) return;
    const now = game.HEAPU32[game._web_state() >> 2];
    if (now !== shown) {
      if (shown < 0 && now === 0) return;
      shown = now;
      changed = performance.now();
      $('starting').hidden = true;
    } else if (performance.now() - changed > 600 && $('starting').hidden) {
      $('starting-text').textContent = 'Loading…';
      $('starting').hidden = false;
    }
  }, 200);
}

$('play').addEventListener('click', play);

$('fullscreen').addEventListener('click', async () => {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await $('game').requestFullscreen({ navigationUI: 'hide' });
    // (Escape is the game's in fullscreen too, where the browser lets it)
    if (navigator.keyboard && navigator.keyboard.lock && document.fullscreenElement) {
      await navigator.keyboard.lock(['Escape']).catch(() => {});
    }
  } catch (error) {
    logLine('fullscreen: ' + error);
  }
});

$('leave').addEventListener('click', () => {
  if (confirm('Quit the game? Progress since the last checkpoint is lost.')) location.reload();
});

// Tab is the game's (switch weapons, the scoreboard), not the page's focus;
// the key still reaches the game (SDL listens on the window)
window.addEventListener('keydown', (event) => {
  if (gameStarted && !gameStopped && event.key === 'Tab') event.preventDefault();
}, true);

$('canvas').addEventListener('click', () => $('canvas').focus());
$('canvas').addEventListener('contextmenu', (event) => event.preventDefault());

document.addEventListener('visibilitychange', async () => {
  if (gameStarted && !gameStopped && document.visibilityState === 'visible' && navigator.wakeLock && !wakeLock) {
    try {
      wakeLock = await navigator.wakeLock.request('screen');
    } catch {
      // as before
    }
  }
  if (document.visibilityState === 'hidden') wakeLock = null;
});

// ---------- start

(async () => {
  await isolate();
  await loadStamp();
  checksPassed = await runChecks();
  if (!checksPassed) {
    $('checks-note').textContent = 'Desktop Chrome, Edge and Firefox run the game; Safari 17 or later may.';
    $('checks-note').hidden = false;
  }
  if (results.opfs === 'good') await refreshData();
  else updatePlay();
  checkForUpdate();
  if (/^[0-9a-f]{32}$/.test(params.get('room') || '')) await joinRoom(params.get('room'));
  if (params.has('play')) play();
})();
