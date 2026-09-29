/*
APP.JS

The page of the web port (port/web/README.md): it checks that the browser
can run the game, copies the game data out of the player's disc image
(xiso-worker.js), then starts the game (halo.js and halo.wasm, built by
`ninja web`) and serves it on the main thread:

- each frame the game's thread posts as an ImageBitmap goes onto the
  canvas (Module.haloPresent);
- each animation frame advances a counter the game waits on, polls the
  controllers (input.js) and gives the game the canvas's size;
- the game's sound plays through an AudioWorklet (audio-worklet.js).
*/

'use strict';

(() => {
  const $ = (id) => document.getElementById(id);
  // the WebAssembly memory the build expects (tools/web_build.py
  // WEB_MEMORY_BYTES): the Xbox memory window ends at 0x88000000
  const MEMORY_PAGES = 0x88000000 / 65536;
  const REQUIRED_BYTES = 2.1e9;

  const state = {
    memory: null,
    audio: null,
    started: false,
    shared: null,
    offsets: null,
    log: [],
    wakeLock: null,
    version: null,
    invite: null,
    gateway: null,
    gatewayInstalled: false,
    inviteConnecting: false,
    pendingInviteConnect: null,
    maps: null,
    cacheAbort: null,
    dataBusy: false,
    releaseGameLock: null,
  };

  // ---------- settings (this browser's; nothing else depends on them)

  const coarsePointer = matchMedia('(pointer: coarse)').matches;
  const settings = { touch: coarsePointer, look: 1.4, vsync: true, glDebug: false };
  try {
    Object.assign(settings, JSON.parse(localStorage.getItem('halo-web-settings') || '{}'));
  } catch { /* private browsing: the defaults */ }

  function saveSettings() {
    try { localStorage.setItem('halo-web-settings', JSON.stringify(settings)); } catch { /* not kept */ }
  }

  // ---------- log

  function log(line) {
    const text = String(line);
    state.log.push(text);
    if (state.log.length > 2000) state.log.splice(0, state.log.length - 2000);
    console.log(text);
  }

  async function debugText() {
    try {
      const root = await navigator.storage.getDirectory();
      const file = await (await root.getFileHandle('debug.txt')).getFile();
      const text = await file.text();
      return text.length > 200000 ? text.slice(-200000) : text;
    } catch {
      return '';
    }
  }

  async function fullLog() {
    const debug = await debugText();
    return `${navigator.userAgent}\n\n--- page and console ---\n${state.log.join('\n')}` +
      (debug ? `\n\n--- debug.txt ---\n${debug}` : '');
  }

  function toast(text, milliseconds = 3500) {
    const element = $('toast');
    element.textContent = text;
    element.hidden = false;
    clearTimeout(toast.timer);
    toast.timer = setTimeout(() => { element.hidden = true; }, milliseconds);
  }

  async function fatal(text) {
    log('fatal: ' + text);
    $('fatal-text').textContent = text;
    $('fatal').hidden = false;
    document.body.classList.remove('playing');
  }

  // ---------- checks

  function addCheck(ok, text, level) {
    const element = document.createElement('div');
    element.className = 'check' + (ok ? '' : level === 'warn' ? ' warn' : ' bad');
    element.textContent = text;
    $('checks').appendChild(element);
    return ok;
  }

  async function ensureIsolation() {
    if (!('serviceWorker' in navigator)) return;
    try {
      await navigator.serviceWorker.register('sw.js');
    } catch (error) {
      log('service worker: ' + error);
      return;
    }
    if (window.crossOriginIsolated) return;
    await navigator.serviceWorker.ready;
    // the service worker serves the page with the isolation headers from
    // its next load on
    let reloaded = false;
    try { reloaded = sessionStorage.getItem('halo-web-isolation') === '1'; } catch { /* none */ }
    if (!reloaded) {
      try { sessionStorage.setItem('halo-web-isolation', '1'); } catch { /* none */ }
      location.reload();
      await new Promise(() => {});
    }
  }

  function webgl2InWorkers() {
    try {
      return typeof OffscreenCanvas !== 'undefined' && !!new OffscreenCanvas(1, 1).getContext('webgl2');
    } catch {
      return false;
    }
  }

  async function runChecks() {
    let ok = true;
    ok = addCheck(window.crossOriginIsolated && typeof SharedArrayBuffer !== 'undefined',
      window.crossOriginIsolated ? 'Threads (SharedArrayBuffer)' :
        'Threads: this page is not cross-origin isolated. Reload it; if this stays, the browser is too old.') && ok;
    ok = addCheck(webgl2InWorkers(), 'WebGL 2 from a worker (OffscreenCanvas; iOS 17 or later)') && ok;
    ok = addCheck(!!(navigator.storage && navigator.storage.getDirectory), 'Private file storage (OPFS)') && ok;
    if (ok) {
      try {
        state.memory = new WebAssembly.Memory({ initial: MEMORY_PAGES, maximum: MEMORY_PAGES, shared: true });
        addCheck(true, 'Memory (2.1 GB of address space)');
      } catch (error) {
        ok = addCheck(false, 'Memory: the browser would not reserve 2.1 GB of address space for the game (' +
          error.message + '). Close other tabs and apps, then reload.');
      }
    }
    if (navigator.storage && navigator.storage.estimate) {
      try {
        const estimate = await navigator.storage.estimate();
        const free = (estimate.quota || 0) - (estimate.usage || 0);
        $('storage-summary').textContent = `Storage: ${(estimate.usage / 1e9).toFixed(2)} GB used of ` +
          `${(estimate.quota / 1e9).toFixed(1)} GB this site may use.`;
        state.freeBytes = free;
      } catch { /* unknown */ }
    }
    return ok;
  }

  // ---------- game data

  async function mapsState() {
    return HaloCache.mapsState();
  }

  function updatePlayButton() {
    $('play').disabled = !state.maps || state.dataBusy || state.started ||
      !!(state.invite && (!state.gatewayInstalled || !state.gateway?.connected));
  }

  function showSteps(maps) {
    state.maps = maps;
    $('step-data').hidden = !!maps;
    $('step-play').hidden = !maps;
    if (maps) {
      $('data-summary').textContent = `Game data: ${maps.files.length} maps, ${(maps.bytes / 1e9).toFixed(2)} GB.`;
    }
    updatePlayButton();
    connectPendingInvite();
  }

  function connectPendingInvite() {
    if (state.maps && !state.dataBusy && state.pendingInviteConnect) {
      const connect = state.pendingInviteConnect;
      state.pendingInviteConnect = null;
      connect();
    }
  }

  function showDownload(progress) {
    $('download-panel').hidden = false;
    $('download-panel').dataset.state = progress.state;
    $('download-title').textContent = progress.title;
    $('download-detail').textContent = progress.detail;
    $('download-progress').value = progress.fraction;
    $('download-percent').textContent = `${Math.floor(progress.fraction * 100)}%`;
  }

  function setDataBusy(busy) {
    state.dataBusy = busy;
    $('iso-file').disabled = busy;
    $('delete-data').disabled = busy;
    $('download-retry').hidden = busy || !!state.maps;
    $('download-cancel').hidden = !state.cacheAbort;
    updatePlayButton();
    connectPendingInvite();
  }

  async function downloadMaps() {
    if (state.dataBusy || state.started) return;
    state.cacheAbort = new AbortController();
    setDataBusy(true);
    try {
      const maps = await HaloCache.download({ signal: state.cacheAbort.signal, onProgress: showDownload });
      showSteps(maps);
    } catch (error) {
      log('game data: ' + error.message);
      if (!state.cacheAbort.signal.aborted && $('download-panel').dataset.state !== 'error') {
        showDownload({ state: 'error', title: 'Download interrupted', detail: error.message, fraction: 0 });
      }
      showSteps(await mapsState());
    } finally {
      state.cacheAbort = null;
      setDataBusy(false);
    }
  }

  function extract(file) {
    return new Promise((resolve, reject) => {
      const worker = new Worker('xiso-worker.js');
      const started = Date.now();
      $('progress').hidden = false;
      worker.onmessage = (event) => {
        const message = event.data;
        if (message.type === 'progress') {
          const fraction = message.total ? message.done / message.total : 0;
          $('progress-fill').style.width = (fraction * 100).toFixed(1) + '%';
          const seconds = (Date.now() - started) / 1000;
          const rate = message.done / Math.max(seconds, 0.1);
          const left = rate > 0 ? (message.total - message.done) / rate : 0;
          $('progress-text').textContent = `Copying maps/${message.file}: ` +
            `${(message.done / 1e9).toFixed(2)} of ${(message.total / 1e9).toFixed(2)} GB` +
            (seconds > 3 ? `, about ${Math.ceil(left / 60)} min left` : '');
        } else if (message.type === 'done') {
          worker.terminate();
          resolve(message);
        } else if (message.type === 'error') {
          worker.terminate();
          reject(new Error(message.message));
        }
      };
      worker.onerror = (event) => {
        worker.terminate();
        reject(new Error(event.message || 'The copy stopped.'));
      };
      worker.postMessage({ file });
    });
  }

  async function onImageChosen(event) {
    const file = event.target.files && event.target.files[0];
    event.target.value = '';
    if (!file) return;
    if (state.freeBytes !== undefined && state.freeBytes < REQUIRED_BYTES) {
      toast('There may not be enough free storage for the game data (about 1.8 GB).', 6000);
    }
    if (navigator.storage && navigator.storage.persist) {
      // keep the data when the device runs low on space
      navigator.storage.persist().catch(() => {});
    }
    setDataBusy(true);
    try {
      const result = await HaloCache.withLock(() => extract(file));
      log(`extracted ${result.files} files, ${result.bytes} bytes`);
      $('progress-text').textContent = 'Done.';
      showSteps(await mapsState());
      if (state.maps) showDownload({ state: 'ready', title: 'Already downloaded',
        detail: `${(state.maps.bytes / 1e9).toFixed(2)} GB saved in this browser. Ready to play.`, fraction: 1 });
    } catch (error) {
      $('progress-text').textContent = error.message;
      log('extraction failed: ' + error.message);
    } finally {
      setDataBusy(false);
    }
  }

  // ---------- saved games: a .zip of the save folder (stored, not compressed)

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

  function zip(files) {
    const encoder = new TextEncoder();
    const parts = [];
    const central = [];
    let offset = 0;
    for (const { name, bytes } of files) {
      const nameBytes = encoder.encode(name);
      const crc = crc32(bytes);
      const local = new DataView(new ArrayBuffer(30));
      local.setUint32(0, 0x04034b50, true);
      local.setUint16(4, 20, true);
      local.setUint32(14, crc, true);
      local.setUint32(18, bytes.length, true);
      local.setUint32(22, bytes.length, true);
      local.setUint16(26, nameBytes.length, true);
      parts.push(new Uint8Array(local.buffer), nameBytes, bytes);
      const entry = new DataView(new ArrayBuffer(46));
      entry.setUint32(0, 0x02014b50, true);
      entry.setUint16(4, 20, true);
      entry.setUint16(6, 20, true);
      entry.setUint32(16, crc, true);
      entry.setUint32(20, bytes.length, true);
      entry.setUint32(24, bytes.length, true);
      entry.setUint16(28, nameBytes.length, true);
      entry.setUint32(42, offset, true);
      central.push(new Uint8Array(entry.buffer), nameBytes);
      offset += 30 + nameBytes.length + bytes.length;
    }
    const centralSize = central.reduce((sum, part) => sum + part.length, 0);
    const end = new DataView(new ArrayBuffer(22));
    end.setUint32(0, 0x06054b50, true);
    end.setUint16(8, files.length, true);
    end.setUint16(10, files.length, true);
    end.setUint32(12, centralSize, true);
    end.setUint32(16, offset, true);
    return new Blob([...parts, ...central, new Uint8Array(end.buffer)], { type: 'application/zip' });
  }

  async function exportSaves() {
    const files = [];
    async function walk(directory, prefix) {
      for await (const [name, handle] of directory.entries()) {
        if (handle.kind === 'directory') await walk(handle, prefix + name + '/');
        else files.push({ name: prefix + name, bytes: new Uint8Array(await (await handle.getFile()).arrayBuffer()) });
      }
    }
    try {
      const maps = state.maps || await mapsState();
      const savePath = (maps?.saveRoot || '/data/save').split('/').slice(2);
      await walk(await HaloCache.directory(savePath), 'save/');
      try {
        const dataPath = (maps?.dataRoot || '/data').split('/').slice(2);
        const config = await (await (await HaloCache.directory(dataPath)).getFileHandle('config.toml')).getFile();
        files.push({ name: 'config.toml', bytes: new Uint8Array(await config.arrayBuffer()) });
      } catch { /* none yet */ }
    } catch {
      toast('There are no saved games yet.');
      return;
    }
    const link = document.createElement('a');
    link.href = URL.createObjectURL(zip(files));
    link.download = 'halo-saves.zip';
    document.body.appendChild(link);
    link.click();
    link.remove();
    setTimeout(() => URL.revokeObjectURL(link.href), 60000);
  }

  async function deleteData() {
    if (state.dataBusy || state.started) return;
    if (!confirm('Delete the game data (the maps folder)? Saved games are kept.')) return;
    try {
      await HaloCache.withLock(async () => {
        for (const path of [[], ['halo', 'data']]) {
          let folder;
          try { folder = await HaloCache.directory(path); }
          catch (error) { if (error.name === 'NotFoundError') continue; throw error; }
          for (const name of ['maps', 'maps.json']) {
            try { await folder.removeEntry(name, { recursive: true }); }
            catch (error) { if (error.name !== 'NotFoundError') throw error; }
          }
        }
      });
    } catch (error) { toast(error.message, 6000); return; }
    showSteps(await mapsState());
    showDownload({ state: 'paused', title: 'Download needed', detail: 'Download the game again or choose your disc image.', fraction: 0 });
    setDataBusy(false);
  }

  // ---------- the running game

  function landscapeSize() {
    const scale = Math.min(window.devicePixelRatio || 1, 2);
    const long = Math.max(window.innerWidth, window.innerHeight);
    const short = Math.min(window.innerWidth, window.innerHeight);
    let width = Math.round(long * scale);
    let height = Math.round(short * scale);
    // no larger than 1440 lines: the game draws 480 and scales them up
    if (height > 1440) {
      width = Math.round(width * 1440 / height);
      height = 1440;
    }
    return { width: width & ~1, height: height & ~1 };
  }

  function sharedWord(name) {
    return (state.shared + state.offsets[name]) >> 2;
  }

  function readOffsets(module) {
    const pointer = module._web_shared_offsets();
    const words = new Int32Array(state.memory.buffer, pointer, 36);
    const names = ['size', 'eventWrite', 'eventRead', 'events', 'eventSize', 'gamepads', 'gamepadSize',
      'displayWidth', 'displayHeight', 'frameCounter', 'framesPresented', 'vsync', 'audioRate', 'audioOpen',
      'audioWrite', 'audioRead', 'audioUnderruns', 'audioRing', 'audioRingFrames', 'pageHidden', 'gameStarted',
      'eventCapacity', 'gamepadCount', 'netLocalAddress', 'netOutWrite', 'netOutRead', 'netInWrite', 'netInRead',
      'netOut', 'netOutBytes', 'netIn', 'netInBytes', 'gatewayEnabled', 'gatewayIdentifier',
      'gatewayPeers', 'gatewayPeerCount'];
    const offsets = {};
    names.forEach((name, index) => { offsets[name] = words[index]; });
    return offsets;
  }

  function updateDisplaySize() {
    const i32 = new Int32Array(state.memory.buffer);
    const { width, height } = landscapeSize();
    Atomics.store(i32, sharedWord('displayWidth'), width);
    Atomics.store(i32, sharedWord('displayHeight'), height);
    $('rotate').hidden = !(state.started && window.innerHeight > window.innerWidth);
  }

  function animationFrame() {
    const i32 = new Int32Array(state.memory.buffer);
    Atomics.add(i32, sharedWord('frameCounter'), 1);
    Atomics.notify(i32, sharedWord('frameCounter'));
    HaloInput.pollGamepads();
    requestAnimationFrame(animationFrame);
  }

  function onVisibility() {
    if (!state.shared) return;
    const i32 = new Int32Array(state.memory.buffer);
    const hidden = document.hidden ? 1 : 0;
    Atomics.store(i32, sharedWord('pageHidden'), hidden);
    Atomics.notify(i32, sharedWord('frameCounter'));
    if (state.audio) {
      if (hidden) state.audio.suspend().catch(() => {});
      else state.audio.resume().catch(() => {});
    }
    if (!hidden) requestWakeLock();
  }

  async function requestWakeLock() {
    try {
      if (navigator.wakeLock && !document.hidden) state.wakeLock = await navigator.wakeLock.request('screen');
    } catch { /* not allowed now */ }
  }

  async function startAudio() {
    const context = state.audio;
    if (!context) return;
    try {
      await context.audioWorklet.addModule('audio-worklet.js');
      const i32 = new Int32Array(state.memory.buffer);
      const node = new AudioWorkletNode(context, 'halo-audio', {
        numberOfInputs: 0,
        numberOfOutputs: 1,
        outputChannelCount: [2],
        processorOptions: {
          buffer: state.memory.buffer,
          ring: state.shared + state.offsets.audioRing,
          ringFrames: state.offsets.audioRingFrames,
          write: state.shared + state.offsets.audioWrite,
          read: state.shared + state.offsets.audioRead,
          underruns: state.shared + state.offsets.audioUnderruns,
          rate: 48000,
        },
      });
      node.connect(context.destination);
      Atomics.store(i32, sharedWord('audioOpen'), 1);
      log(`audio: ${context.sampleRate} Hz`);
    } catch (error) {
      log('audio: ' + error);
      toast('Sound is not available: ' + error.message);
    }
  }

  async function play() {
    if (state.started || state.dataBusy || !state.maps) return;
    if (state.invite && (!state.gatewayInstalled || !state.gateway?.connected)) {
      toast('Wait for the invited host to connect before starting.');
      return;
    }
    state.started = true;

    // in the tap: browsers start sound only then
    try {
      const AudioContextClass = window.AudioContext || window.webkitAudioContext;
      state.audio = new AudioContextClass({ sampleRate: 48000, latencyHint: 'interactive' });
      state.audio.resume().catch(() => {});
    } catch (error) {
      log('audio context: ' + error);
    }
    try {
      if (navigator.locks) {
        await new Promise((resolve, reject) => {
          navigator.locks.request('halo-game', { ifAvailable: true }, lock => {
            if (!lock) { reject(new Error('Halo is open in another tab. Close it, then press Play.')); return; }
            resolve();
            return new Promise(release => { state.releaseGameLock = release; });
          }).catch(reject);
        });
      }
    } catch (error) {
      state.started = false;
      state.audio?.close().catch(() => {});
      toast(error.message, 6000);
      updatePlayButton();
      return;
    }
    updatePlayButton();
    document.body.classList.add('playing');
    const root = document.documentElement;
    if (root.requestFullscreen && !navigator.standalone) root.requestFullscreen({ navigationUI: 'hide' }).catch(() => {});
    if (screen.orientation && screen.orientation.lock) screen.orientation.lock('landscape').catch(() => {});
    requestWakeLock();
    // the system's back gesture or button (Android) backs out of menus, as
    // the controller's B does, instead of leaving the game
    history.pushState({ playing: true }, '');
    window.addEventListener('popstate', () => {
      HaloInput.pressBack();
      history.pushState({ playing: true }, '');
    });

    const canvas = $('screen');
    const context = canvas.getContext('bitmaprenderer');
    // (tests pass extra --NAME=value settings in window.__haloArgs)
    const argumentsList = Array.isArray(window.__haloArgs) ? window.__haloArgs.slice() : [];
    argumentsList.push('--HALO_DATA_ROOT=' + state.maps.dataRoot, '--HALO_SAVE_ROOT=' + state.maps.saveRoot);
    if (!settings.vsync) argumentsList.push('--HALO_NO_VSYNC=1');
    if (settings.glDebug) argumentsList.push('--HALO_GL_DEBUG=1');

    window.Module = {
      wasmMemory: state.memory,
      arguments: argumentsList,
      print: (text) => log(text),
      printErr: (text) => log(text),
      haloPresent: (bitmap) => {
        if (canvas.width !== bitmap.width || canvas.height !== bitmap.height) {
          canvas.width = bitmap.width;
          canvas.height = bitmap.height;
        }
        context.transferFromImageBitmap(bitmap);
      },
      haloMessage: (kind, text) => {
        if (kind === 0) log('game: ' + text);
        else if (kind === 1) toast(text, 5000);
        else if (kind === 2) log('clipboard: ' + text);
        else if (kind === 4) log('thread error: ' + text);
        else if (kind === 5) {
          log('game: ' + text);
          $('fatal').querySelector('h2').textContent = 'The game quit';
          fatal(text + ' Reload to start it again.');
        }
        else fatal(text);
      },
      onAbort: (what) => fatal('The game stopped: ' + what),
      onRuntimeInitialized: () => {
        const module = window.Module;
        state.shared = module._web_shared_state();
        state.offsets = readOffsets(module);
        updateDisplaySize();
        HaloInput.attach({
          memory: state.memory,
          base: state.shared,
          offsets: state.offsets,
          canvas,
          touchRoot: $('touch'),
          touch: settings.touch,
        });
        HaloInput.setLookSensitivity(settings.look);
        HaloNet.attach({ memory: state.memory, base: state.shared, offsets: state.offsets });
        $('touch').hidden = !settings.touch;
        requestAnimationFrame(animationFrame);
        startAudio();
        log('runtime ready');
      },
    };

    const script = document.createElement('script');
    script.src = 'halo.js';
    script.onerror = () => fatal('Could not load halo.js.');
    document.body.appendChild(script);
  }

  // ---------- online play (net.js)

  function setUpInvites() {
    const relay = window.HALO_BROWSER_CONFIG?.relayUrl || '';
    const linked = new URLSearchParams(location.hash.slice(1)).get('join');
    state.invite = linked ? HaloInvite.parse(linked) : null;
    if (state.invite) $('invite-input').value = 'halo://join/' + state.invite;
    $('invite-status').textContent = linked && !state.invite ? 'This invite is incomplete or invalid.' :
      relay ? 'Ready for a desktop invite.' : 'Desktop invites need a relay. This preview does not have a relay configured yet.';
    $('relay-access-row').hidden = !relay;
    const connect = async () => {
      if (state.inviteConnecting || state.gatewayInstalled || state.started) return;
      const token = HaloInvite.parse($('invite-input').value);
      if (!token) { $('invite-status').textContent = 'Paste a complete halo://join/ invite.'; return; }
      state.invite = token;
      $('play').disabled = true;
      $('browser-rooms').hidden = true;
      if (!relay) {
        $('invite-status').textContent = 'The browser link is valid, but this site needs a relay before it can join desktop games.';
        return;
      }
      if (!state.maps || state.dataBusy) {
        state.pendingInviteConnect = connect;
        $('invite-status').textContent = 'Preparing game data. The invite will connect when the download finishes.';
        return;
      }
      $('invite-connect').disabled = true;
      $('invite-input').disabled = true;
      state.inviteConnecting = true;
      let transport = null;
      try {
        transport = await HaloGateway.connect(relay, token, {
          accessToken: $('relay-access').value,
          onStatus(status) {
            $('invite-status').textContent = status.message;
            if (status.state === 'error') {
              if (state.started) fatal(status.message + ' Reload to start a new session.');
              else {
                state.gateway = null;
                state.gatewayInstalled = false;
                $('invite-connect').disabled = false;
                $('invite-input').disabled = false;
              }
            }
            updatePlayButton();
          },
        });
        await HaloNet.useTransport(transport);
        // A host failure can arrive while leaving the previous browser room.
        if (transport.closed) throw new Error('The relay session ended. Try connecting again.');
        state.gateway = transport;
        state.gatewayInstalled = true;
        $('online-address').textContent = HaloNet.addressText(state.gateway.address);
        $('invite-input').disabled = true;
        $('relay-access').value = '';
        updatePlayButton();
      } catch (error) {
        transport?.close();
        $('invite-status').textContent = error.message;
        $('invite-connect').disabled = false;
        $('invite-input').disabled = false;
      } finally {
        state.inviteConnecting = false;
        updatePlayButton();
      }
    };
    $('invite-connect').onclick = connect;
    $('invite-input').onkeydown = (event) => { if (event.key === 'Enter') connect(); };
    $('invite-share').onclick = async () => {
      try {
        const link = HaloInvite.link(location.href, $('invite-input').value);
        await navigator.clipboard.writeText(link);
        toast('Browser invite copied.');
      } catch (error) { $('invite-status').textContent = error.message; }
    };
    if (state.invite) {
      $('browser-rooms').hidden = true;
      if (relay) connect();
    }
  }

  function onlineOptions() {
    const options = {};
    if (settings.turnUrl) {
      options.turn = { urls: settings.turnUrl, username: settings.turnUser || '', credential: settings.turnPassword || '' };
    }
    // (tests name their own broker: ?signal=ws://...)
    const signal = new URLSearchParams(location.search).get('signal');
    if (signal) options.brokers = [signal];
    return options;
  }

  function roomLink(code) {
    const url = new URL(location.href);
    url.search = '';
    url.hash = '';
    url.searchParams.set('room', code);
    return url.toString();
  }

  function showOnline(status) {
    const inRoom = !!status.room;
    $('online-join').hidden = inRoom;
    $('online-room').hidden = !inRoom;
    if (!inRoom) return;
    $('online-code').textContent = status.room;
    const players = status.players === 1 ? '1 other player' : `${status.players} other players`;
    $('online-status').textContent = status.brokers ? `Connected: ${players} in the room.` :
      'Looking for the room… (checking the connection)';
    $('online-names').textContent = status.names.length ? status.names.join(', ') : '';
  }

  async function joinRoom(code) {
    try {
      const joined = await HaloNet.join(code, onlineOptions());
      try { localStorage.setItem('halo-web-room', joined); } catch { /* not kept */ }
    } catch (error) {
      toast(error.message);
    }
  }

  function setUpOnline() {
    HaloNet.on((type, detail) => {
      if (type === 'status') showOnline(detail);
      if (type === 'joined') toast(`${detail.name} joined the room.`);
      if (type === 'left') toast(`${detail.name} left the room.`);
    });
    let name = '';
    try { name = localStorage.getItem('halo-web-player-name') || ''; } catch { /* none */ }
    $('online-name').value = name;
    $('online-name').onchange = (event) => {
      try { localStorage.setItem('halo-web-player-name', event.target.value.trim().slice(0, 24)); } catch { /* none */ }
    };
    $('online-create').onclick = () => joinRoom(HaloNet.newRoomCode());
    $('online-enter').onclick = () => joinRoom($('online-input').value);
    $('online-input').onkeydown = (event) => { if (event.key === 'Enter') joinRoom(event.target.value); };
    $('online-leave').onclick = async () => {
      await HaloNet.leave();
      try { localStorage.removeItem('halo-web-room'); } catch { /* none */ }
    };
    $('online-share').onclick = async () => {
      const link = roomLink(HaloNet.status().room);
      try {
        if (navigator.share) await navigator.share({ title: 'Halo CE room', text: 'Join my Halo game', url: link });
        else {
          await navigator.clipboard.writeText(link);
          toast('The room link is copied.');
        }
      } catch { /* cancelled */ }
    };
    $('online-address').textContent = HaloNet.addressText(HaloNet.address);
    $('opt-turn-url').value = settings.turnUrl || '';
    $('opt-turn-user').value = settings.turnUser || '';
    $('opt-turn-password').value = settings.turnPassword || '';
    for (const [id, key] of [['opt-turn-url', 'turnUrl'], ['opt-turn-user', 'turnUser'], ['opt-turn-password', 'turnPassword']]) {
      $(id).onchange = (event) => { settings[key] = event.target.value.trim(); saveSettings(); };
    }
    // a room link, or the room of last time
    const linked = new URLSearchParams(location.search).get('room');
    let last = null;
    try { last = localStorage.getItem('halo-web-room'); } catch { /* none */ }
    if (!new URLSearchParams(location.hash.slice(1)).has('join') && (linked || last)) joinRoom(linked || last);
    showOnline(HaloNet.status());
  }

  // ---------- updates

  async function checkForUpdate() {
    try {
      const current = await (await fetch('version.json')).json();
      state.version = current.version;
      $('version').textContent = 'Build ' + current.version;
      const latest = await (await fetch('version.json?latest=1', { cache: 'no-store' })).json();
      if (latest.version && latest.version !== current.version && navigator.serviceWorker.controller) {
        const element = $('toast');
        element.innerHTML = '';
        element.append('A new version is available. ');
        const button = document.createElement('button');
        button.className = 'button';
        button.textContent = 'Update';
        button.onclick = () => {
          button.disabled = true;
          button.textContent = 'Updating…';
          navigator.serviceWorker.controller.postMessage('update');
        };
        element.append(button);
        element.hidden = false;
      }
    } catch { /* offline */ }
  }

  if ('serviceWorker' in navigator) {
    navigator.serviceWorker.addEventListener('message', (event) => {
      if (event.data === 'updated') location.reload();
      if (event.data === 'update-failed') toast('The update could not be downloaded.');
    });
  }

  // ---------- start

  async function main() {
    window.addEventListener('error', (event) => {
      log(`error: ${event.message} (${event.filename}:${event.lineno})`);
      if (state.started) fatal(event.message || 'An error stopped the game.');
    });
    window.addEventListener('unhandledrejection', (event) => log('unhandled: ' + event.reason));
    window.addEventListener('resize', () => { if (state.shared) updateDisplaySize(); });
    document.addEventListener('visibilitychange', onVisibility);

    $('opt-touch').checked = settings.touch;
    $('opt-look').value = settings.look;
    $('opt-vsync').checked = settings.vsync;
    $('opt-touch').onchange = (event) => { settings.touch = event.target.checked; saveSettings(); };
    $('opt-look').oninput = (event) => {
      settings.look = parseFloat(event.target.value);
      saveSettings();
      HaloInput.setLookSensitivity(settings.look);
    };
    $('opt-vsync').onchange = (event) => { settings.vsync = event.target.checked; saveSettings(); };
    $('opt-gldebug').checked = settings.glDebug;
    $('opt-gldebug').onchange = (event) => { settings.glDebug = event.target.checked; saveSettings(); };
    $('iso-file').onchange = onImageChosen;
    $('play').onclick = play;
    $('export-saves').onclick = exportSaves;
    $('delete-data').onclick = deleteData;
    $('download-retry').onclick = downloadMaps;
    $('download-cancel').onclick = () => state.cacheAbort?.abort();
    $('show-log').onclick = async () => {
      $('log-text').textContent = await fullLog();
      $('log-view').hidden = false;
    };
    $('log-close').onclick = () => { $('log-view').hidden = true; };
    $('log-copy').onclick = async () => {
      try { await navigator.clipboard.writeText(await fullLog()); toast('Copied.'); } catch { toast('Could not copy.'); }
    };
    $('fatal-reload').onclick = () => location.reload();
    $('fatal-log').onclick = async () => {
      try { await navigator.clipboard.writeText(await fullLog()); toast('Copied.'); } catch { toast('Could not copy.'); }
    };

    const standalone = navigator.standalone || matchMedia('(display-mode: standalone)').matches ||
      matchMedia('(display-mode: fullscreen)').matches;
    const ios = /iPhone|iPad|iPod/.test(navigator.userAgent) ||
      (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
    $('install-hint').hidden = standalone || !ios;
    // Android (Chrome, Edge, Samsung Internet): the browser's own install
    // prompt when it offers one, otherwise where to find it
    const android = /Android/i.test(navigator.userAgent);
    $('install-android').hidden = standalone || !android;
    window.addEventListener('beforeinstallprompt', (event) => {
      event.preventDefault();
      state.installPrompt = event;
      $('install-android').hidden = standalone;
      $('install-button').hidden = false;
      $('install-android-menu').hidden = true;
    });
    $('install-button').onclick = async () => {
      if (!state.installPrompt) return;
      state.installPrompt.prompt();
      const choice = await state.installPrompt.userChoice.catch(() => null);
      state.installPrompt = null;
      if (choice && choice.outcome === 'accepted') $('install-android').hidden = true;
    };
    window.addEventListener('appinstalled', () => { $('install-android').hidden = true; });

    await ensureIsolation();
    setUpOnline();
    setUpInvites();
    const ok = await runChecks();
    if (!ok) return;
    showSteps(await mapsState());
    if (state.maps) {
      showDownload({ state: 'ready', title: 'Already downloaded',
        detail: `${(state.maps.bytes / 1e9).toFixed(2)} GB saved in this browser. Ready to play.`, fraction: 1 });
      setDataBusy(false);
    } else {
      downloadMaps();
    }
    checkForUpdate();
  }

  main().catch((error) => {
    log('start: ' + (error && error.stack || error));
    addCheck(false, 'The page could not start: ' + error.message);
  });
})();
