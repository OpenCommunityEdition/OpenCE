const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const Invite = require('../../port/web/site/native-invite.js');
const Gateway = require('../../port/web/site/gateway.js');
const TOKEN = '0123456789ab' + 'c'.repeat(32);
const ADDRESS = 0x01024064;
function packet(kind = 3, length = 4) {
  const bytes = new Uint8Array((24 + length + 3) & ~3), v = new DataView(bytes.buffer);
  v.setUint32(0, bytes.length, true); v.setUint32(4, kind, true); v.setUint32(20, length, true);
  return bytes;
}
class FakeSocket {
  static OPEN = 1;
  static latest;
  readyState = 1; bufferedAmount = 0; sent = [];
  constructor(url) { this.url = url; FakeSocket.latest = this; }
  send(value) { this.sent.push(value); }
  close() { this.readyState = 3; }
  json(value) { this.onmessage({ data: JSON.stringify(value) }); }
}
global.WebSocket = FakeSocket;

test('native and HTTPS invites preserve the exact secret in a fragment', () => {
  assert.equal(Invite.parse('halo://join/' + TOKEN.toUpperCase()), TOKEN);
  const url = Invite.link('https://fqlx.github.io/halo-ce-universal/?room=OLD', TOKEN);
  assert.equal(url, 'https://fqlx.github.io/halo-ce-universal/#join=' + TOKEN);
  assert.equal(new URL(url).search, '');
  assert.equal(Invite.parse(url), TOKEN);
});
test('reject partial, extra path, misleading scheme, and insecure public links', () => {
  for (const value of [TOKEN.slice(1), TOKEN + 'f', 'halo://join/' + TOKEN + '/extra',
    'javascript:' + TOKEN, 'http://example.com/#join=' + TOKEN]) assert.equal(Invite.parse(value), null);
  assert.throws(() => Invite.link('http://example.com/', TOKEN));
  assert.equal(Invite.parse('http://localhost:8779/#join=' + TOKEN), TOKEN);
});
test('packet validator rejects length, kind, and control-payload mismatches', () => {
  assert.ok(Gateway.validFrame(packet()));
  assert.ok(Gateway.validFrame(packet(2, 0)));
  assert.equal(Gateway.validFrame(packet(2, 4)), false);
  assert.equal(Gateway.validFrame(packet(9, 0)), false);
  const invalid = packet(); new DataView(invalid.buffer).setUint32(0, 24, true);
  assert.equal(Gateway.validFrame(invalid), false);
});
test('virtual addresses and identifier words match the native little-endian ABI', () => {
  assert.ok(Gateway.validAddress(ADDRESS));
  assert.equal(Gateway.validAddress(0x0100007f), false);
  assert.equal(Gateway.validAddress(0x01008064), false);
  assert.deepEqual(Gateway.idWords('010203040506'), [0x04030201, 0x0605]);
});
test('relay handshake publishes identity and peer map, queues reliable bytes until accepted', async () => {
  const statuses = [];
  const promise = Gateway.connect('ws://localhost:8781/join', TOKEN, { onStatus: v => statuses.push(v) });
  const socket = FakeSocket.latest;
  socket.onopen();
  assert.deepEqual(JSON.parse(socket.sent[0]), { type: 'join', invite: TOKEN });
  assert.equal(socket.url.search, '');
  socket.json({ type: 'ready', identifier: '010203040506', address: ADDRESS });
  const gateway = await promise;
  assert.equal(gateway.connected, false);
  assert.equal(gateway.hostAddress, 0);
  socket.json({ type: 'peer', identifier: TOKEN.slice(0, 12), address: ADDRESS + 0x1000000, connected: true });
  assert.equal(gateway.connected, true);
  assert.equal(gateway.hostAddress, ADDRESS + 0x1000000);
  const memory = { buffer: new SharedArrayBuffer(512) };
  gateway.attach({ memory, base: 0, offsets: { gatewayEnabled: 0, gatewayIdentifier: 4,
    netLocalAddress: 12, gatewayPeers: 16, gatewayPeerCount: 32 } });
  const words = new Int32Array(memory.buffer);
  assert.equal(words[0], 1); assert.equal(words[1], 0x04030201); assert.equal(words[3], ADDRESS);
  assert.equal(words[6], ADDRESS + 0x1000000);
  const frame = packet(); socket.onmessage({ data: frame.buffer });
  gateway.flush(() => false);
  let count = 0; gateway.flush(value => { count++; assert.deepEqual(value, frame); return true; });
  gateway.flush(() => { throw new Error('duplicate'); });
  assert.equal(count, 1);
  socket.bufferedAmount = 2 * 1024 * 1024;
  assert.equal(gateway.send(frame), false);
  socket.bufferedAmount = 0;
  assert.equal(gateway.send(frame), true);
  socket.json({ type: 'peer', identifier: TOKEN.slice(0, 12), address: ADDRESS + 0x1000000, connected: false });
  assert.equal(words[6], 0); assert.equal(gateway.connected, false);
  assert.equal(gateway.hostAddress, 0);
  gateway.close();
  assert.equal(gateway.hostAddress, 0);
  assert.equal(words[3], 0);
  socket.json({ type: 'peer', identifier: TOKEN.slice(0, 12), address: ADDRESS + 0x1000000, connected: true });
  assert.equal(gateway.connected, false);
  assert.equal(words[6], 0);
  assert.ok(statuses.some(value => value.state === 'connected'));
});
test('invalid identity and insecure relay fail closed', async () => {
  await assert.rejects(Gateway.connect('ws://example.com/join', TOKEN));
  const promise = Gateway.connect('ws://localhost:8781/join', TOKEN);
  FakeSocket.latest.json({ type: 'ready', identifier: 'bad', address: ADDRESS });
  await assert.rejects(promise, /identity/);
  assert.equal(FakeSocket.latest.readyState, 3);
});

// Exercise the real launcher's public button handlers and gateway events;
// stub browser capabilities without starting a game or reserving 2.1 GB.
async function launcher(useTransport = async () => {}, options = {}) {
  const elements = new Map();
  const storage = options.storage || new Map(), joins = [], listeners = [];
  const quickCalls = [], phases = [], windowEvents = new Map();
  let room = null;
  function element(id) {
    if (!elements.has(id)) elements.set(id, {
      value: '', disabled: false, hidden: true, dataset: {}, children: [],
      appendChild(child) { this.children.push(child); },
      classList: { add() {}, remove() {} }, getContext() { return {}; },
    });
    return elements.get(id);
  }
  const context = {
    console: { log() {} }, URL, URLSearchParams, SharedArrayBuffer, AbortController, DOMException,
    setTimeout: options.setTimeout || setTimeout, clearTimeout: options.clearTimeout || clearTimeout,
    navigator: { userAgent: 'Test', platform: 'Test', storage: options.gameStorage || { getDirectory() {} },
      ...(options.serviceWorker ? { serviceWorker: options.serviceWorker } : {}) },
    location: new URL(options.url || 'http://localhost:8780/'),
    document: { getElementById: element, createElement: () => element(Symbol()),
      body: element('body'), documentElement: {}, addEventListener() {} },
    localStorage: { getItem: key => storage.get(key) ?? null,
      setItem: (key, value) => storage.set(key, value), removeItem: key => storage.delete(key) },
    matchMedia: () => ({ matches: false }),
    addEventListener(type, listener) {
      if (!windowEvents.has(type)) windowEvents.set(type, []);
      windowEvents.get(type).push(listener);
    },
    screen: {}, history: { pushState() {}, replaceState(_state, _title, url) { context.location.href = String(url); } },
    crossOriginIsolated: true,
    OffscreenCanvas: class { getContext() { return {}; } },
    WebAssembly: { Memory: options.Memory || class {} }, fetch: options.fetch || (async () => { throw new Error('Offline'); }),
    HALO_BROWSER_CONFIG: { relayUrl: options.relayUrl ?? 'ws://localhost:8781/join',
      ...('defaultRoom' in options ? { defaultRoom: options.defaultRoom } : {}) },
    HaloInvite: Invite, HaloGateway: Gateway,
    HaloNet: {
      on(listener) { listeners.push(listener); }, addressText: address => [address & 255,
        (address >>> 8) & 255, (address >>> 16) & 255, address >>> 24].join('.'),
      status: () => ({ room, players: 0, brokers: 1, names: [] }), useTransport,
      quickPlay(request) {
        quickCalls.push(request);
        if (options.quickPlay) return options.quickPlay({ ...request, room });
        return new Promise((_resolve, reject) => request.signal.addEventListener('abort',
          () => reject(new DOMException('Aborted', 'AbortError')), { once: true }));
      },
      cancelQuickPlay() {}, quickPlayStarted() {},
      quickPlayPhase(phase) { phases.push(phase); },
      async join(code) {
        joins.push(code);
        await options.beforeJoin?.();
        room = code.trim().toUpperCase();
        listeners.forEach(listener => listener('status', this.status()));
        return room;
      },
      async leave() {
        room = null;
        listeners.forEach(listener => listener('status', this.status()));
      },
    },
    HaloCache: { mapsState: options.mapsState || (async () => ({ files: ['ui.map'], bytes: 2048,
      dataRoot: '/data', saveRoot: '/data/save' })), download: options.download },
  };
  context.window = context;
  vm.runInNewContext(fs.readFileSync(require.resolve('../../port/web/site/app.js'), 'utf8'), context);
  await new Promise(setImmediate);
  if (!options.mapsState) assert.equal(element('step-play').hidden, false, 'launcher reached the cached-data ready state');
  return { element, context, storage, joins, quickCalls, phases, windowEvents,
    emitNetwork: (type, detail) => listeners.forEach(listener => listener(type, detail)) };
}

test('Safari storage rejection gives recovery guidance before memory allocation and still offers updates', async () => {
  let allocations = 0, inspections = 0;
  const messages = [];
  const { element, context } = await launcher(undefined, {
    defaultRoom: '',
    gameStorage: { async getDirectory() {
      throw new DOMException('The operation failed for an unknown transient reason (e.g. out of memory).', 'UnknownError');
    } },
    Memory: class { constructor() { allocations++; } },
    mapsState: async () => { inspections++; return null; },
    fetch: async url => ({ json: async () => ({ version: url.includes('latest') ? 'new' : 'old' }) }),
    serviceWorker: { register: async () => {}, addEventListener() {},
      controller: { postMessage: message => messages.push(message) } },
  });
  assert.equal(allocations, 0, 'unavailable storage must not leave a 2.1 GB memory reservation');
  assert.equal(inspections, 0, 'stop before attempting to read the blocked map cache');
  assert.equal(context.Module, undefined);
  const checks = element('checks').children;
  assert.ok(checks.some(check => check.className === 'check bad' && /regular tab.*Private Browsing/.test(check.textContent)));
  assert.ok(!checks.some(check => /page could not start|Memory \(/.test(check.textContent)));
  assert.ok(!checks.some(check => check.className === 'check' && /storage/i.test(check.textContent)));
  assert.equal(element('update-notice').hidden, false, 'repairs remain reachable after a failed check');
  element('update-button').onclick();
  assert.deepEqual(messages, ['update']);
});

test('missing storage API fails cleanly without inspecting data or allocating memory', async () => {
  const { element } = await launcher(undefined, {
    defaultRoom: '', gameStorage: {},
    Memory: class { constructor() { assert.fail('unexpected memory allocation'); } },
    mapsState: async () => { assert.fail('unexpected cache inspection'); },
  });
  assert.ok(element('checks').children.some(check => check.className === 'check bad' && /storage is unavailable/.test(check.textContent)));
});

test('usable storage is opened before memory allocation and normal cached startup remains available', async () => {
  const order = [];
  const { element } = await launcher(undefined, {
    defaultRoom: '',
    gameStorage: { async getDirectory() { order.push('storage'); return {}; } },
    Memory: class { constructor() { order.push('memory'); } },
    mapsState: async () => {
      order.push('maps');
      return { files: ['ui.map'], bytes: 2048, dataRoot: '/data', saveRoot: '/data/save' };
    },
  });
  assert.deepEqual(order, ['storage', 'memory', 'maps']);
  assert.equal(element('step-play').hidden, false);
  assert.ok(element('checks').children.some(check => check.className === 'check' && /Game file storage/.test(check.textContent)));
});

for (const autoLaunch of [false, true]) {
  test(`update notice survives room notifications and remains retryable ${autoLaunch ? 'after auto-launch' : 'in the launcher'}`, async () => {
    const messages = [], timers = new Map();
    let receiveWorkerMessage, nextTimer = 0;
    const { element, context, emitNetwork } = await launcher(undefined, {
      url: autoLaunch ? 'http://localhost:8780/' : 'http://localhost:8780/?menu=1',
      quickPlay: async ({ room }) => ({ role: 'host', room, hostAddress: ADDRESS }),
      setTimeout: callback => { timers.set(++nextTimer, callback); return nextTimer; },
      clearTimeout: id => timers.delete(id),
      fetch: async url => ({ json: async () => ({ version: url.includes('latest') ? 'new' : 'old' }) }),
      serviceWorker: {
        register: async () => {},
        controller: { postMessage: message => messages.push(message) },
        addEventListener: (_type, listener) => { receiveWorkerMessage = listener; },
      },
    });
    assert.equal(!!context.Module, autoLaunch);
    assert.equal(element('update-notice').hidden, false, 'new build has a persistent update notice');
    const runToastTimer = () => {
      for (const callback of timers.values()) callback();
      timers.clear();
    };
    emitNetwork('joined', { name: 'Player' });
    assert.equal(element('toast').textContent, 'Player joined the room.');
    runToastTimer();
    assert.equal(element('toast').hidden, true);
    assert.equal(element('update-notice').hidden, false, 'toast expiry cannot hide the update');
    element('update-button').onclick();
    assert.deepEqual(messages, ['update'], 'update action remains reachable after status messages');
    assert.equal(element('update-button').disabled, true);
    assert.match(element('update-message').textContent, /restart/);
    receiveWorkerMessage({ data: 'update-failed' });
    const failure = element('update-message').textContent;
    assert.match(failure, /could not be downloaded/);
    assert.equal(element('update-button').disabled, false);
    assert.equal(element('update-button').textContent, 'Retry update');
    emitNetwork('left', { name: 'Player' });
    runToastTimer();
    assert.equal(element('update-notice').hidden, false);
    assert.equal(element('update-message').textContent, failure, 'room status cannot overwrite the persistent update status');
    element('update-button').onclick();
    assert.deepEqual(messages, ['update', 'update']);
  });
}

test('an offline host after relay ready allows a fresh invite; in-game disconnect still requires reload', async () => {
  const { element, context } = await launcher();
  element('invite-input').value = TOKEN;
  const first = element('invite-connect').onclick();
  const failed = FakeSocket.latest;
  failed.json({ type: 'ready', identifier: '010203040506', address: ADDRESS });
  await first;
  assert.equal(element('invite-input').disabled, true);
  failed.json({ type: 'error', message: 'Native host did not connect' });
  assert.equal(element('invite-connect').disabled, false);
  assert.equal(element('invite-input').disabled, false);
  assert.equal(context.Module, undefined, 'offline host does not launch the engine');
  assert.equal(element('fatal').hidden, true);

  const fresh = 'abcdefabcdef' + 'd'.repeat(32);
  element('invite-input').value = fresh;
  const second = element('invite-connect').onclick();
  const live = FakeSocket.latest;
  assert.notEqual(live, failed);
  live.onopen();
  assert.equal(JSON.parse(live.sent[0]).invite, fresh);
  live.json({ type: 'ready', identifier: '112233445566', address: ADDRESS });
  await second;
  live.json({ type: 'peer', identifier: fresh.slice(0, 12), address: ADDRESS + 0x1000000, connected: true });
  assert.ok(context.Module, 'native host availability automatically launches the game');
  assert.ok(context.Module.arguments.includes('--HALO_QUICK_PLAY=join'));
  assert.ok(context.Module.arguments.includes('--HALO_QUICK_PLAY_TARGET=100.64.2.2'));
  live.json({ type: 'error', message: 'Native session ended' });
  assert.equal(element('fatal').hidden, false);
  assert.match(element('fatal-text').textContent, /Reload/);
  assert.equal(element('invite-connect').disabled, true);
  assert.equal(element('invite-input').disabled, true);
  assert.equal(element('play').disabled, true);
  await element('invite-connect').onclick();
  assert.equal(FakeSocket.latest, live, 'running game cannot change identity');
});

test('failure while installing relay transport cannot relock the pre-game invite form', async () => {
  let finishInstall;
  const { element, context } = await launcher(() => new Promise(resolve => { finishInstall = resolve; }));
  element('invite-input').value = TOKEN;
  const attempt = element('invite-connect').onclick();
  const socket = FakeSocket.latest;
  socket.json({ type: 'ready', identifier: '010203040506', address: ADDRESS });
  await new Promise(setImmediate);
  socket.json({ type: 'error', message: 'Native session ended' });
  finishInstall();
  await attempt;
  assert.equal(element('invite-connect').disabled, false);
  assert.equal(element('invite-input').disabled, false);
  assert.equal(context.Module, undefined);
});

test('normal startup joins the public default room and waits for a ready multiplayer role', async () => {
  const { joins, element, context, storage } = await launcher();
  assert.deepEqual(joins, ['FQLX01']);
  assert.equal(element('online-default-code').textContent, 'FQLX01');
  assert.equal(element('online-code').textContent, 'FQLX01');
  assert.equal(element('online-default').disabled, true);
  assert.equal(context.Module, undefined);
  assert.equal((await launcher(undefined, { url: 'http://localhost:8780/?menu=1' })).quickCalls.length, 0);
  assert.equal(context.location.search, '');
  assert.equal(storage.has('halo-web-room'), false, 'automatic fallback is not remembered as a private choice');
  const configured = await launcher(undefined, { defaultRoom: 'CUSTOM01' });
  assert.deepEqual(configured.joins, ['CUSTOM01']);
  assert.equal(configured.element('online-default-code').textContent, 'CUSTOM01');
  const disabled = await launcher(undefined, { defaultRoom: '' });
  assert.deepEqual(disabled.joins, []);
  assert.equal(disabled.element('online-default-room').hidden, true);
});

test('explicit room wins over remembered choice and leave flag; private choice wins over default', async () => {
  const storage = new Map([['halo-web-room', 'PRIVATE7']]);
  const remembered = await launcher(undefined, { storage });
  assert.deepEqual(remembered.joins, ['PRIVATE7']);
  assert.equal(remembered.element('online-default').disabled, false);
  storage.set('halo-web-room-left', '1');
  const linked = await launcher(undefined, { storage, url: 'http://localhost:8780/?room=FRIENDS9' });
  assert.deepEqual(linked.joins, ['FRIENDS9']);
  assert.equal(storage.get('halo-web-room'), 'FRIENDS9');
  assert.equal(storage.has('halo-web-room-left'), false);
});

test('native invite suppresses explicit, remembered, and default browser rooms', async () => {
  const storage = new Map([['halo-web-room', 'PRIVATE7']]);
  const native = await launcher(undefined, { storage, relayUrl: '',
    url: 'http://localhost:8780/?room=FRIENDS9#join=' + TOKEN });
  assert.deepEqual(native.joins, []);
  assert.equal(native.element('browser-rooms').hidden, true);
  assert.equal(storage.get('halo-web-room'), 'PRIVATE7');
  assert.equal(native.context.Module, undefined);
});

test('Leave survives reload from a room link and Join default room restores automatic joining', async () => {
  const first = await launcher(undefined, { url: 'http://localhost:8780/?room=PRIVATE7' });
  await first.element('online-leave').onclick();
  assert.equal(first.context.location.search, '');
  assert.equal(first.storage.get('halo-web-room-left'), '1');
  assert.equal(first.element('online-room').hidden, true);
  const reloaded = await launcher(undefined, { storage: first.storage, url: first.context.location.href });
  assert.deepEqual(reloaded.joins, []);
  assert.equal(reloaded.element('online-default').disabled, false);
  await reloaded.element('online-default').onclick();
  assert.deepEqual(reloaded.joins, ['FQLX01']);
  assert.equal(first.storage.has('halo-web-room-left'), false);
  assert.equal(first.storage.get('halo-web-room'), 'FQLX01');
  assert.equal(reloaded.context.location.search, '?room=FQLX01');
  const returned = await launcher(undefined, { storage: first.storage });
  assert.deepEqual(returned.joins, ['FQLX01']);
});

test('leaving while automatic room setup is pending remains outside the room', async () => {
  let finishJoin;
  const pending = await launcher(undefined, { beforeJoin: () => new Promise(resolve => { finishJoin = resolve; }) });
  const leaving = pending.element('online-leave').onclick();
  finishJoin();
  await leaving;
  assert.equal(pending.element('online-room').hidden, true);
  assert.equal(pending.storage.get('halo-web-room-left'), '1');
  assert.equal(pending.storage.has('halo-web-room'), false);
});

test('stable host selection automatically loads multiplayer and first gesture unlocks controls', async () => {
  const launched = await launcher(undefined, { quickPlay: async ({ room }) => ({ role: 'host', room }) });
  assert.ok(launched.context.Module.arguments.includes('--HALO_QUICK_PLAY=host'));
  assert.equal(launched.context.Module.arguments.some(value => value.includes('NETWORK_TEST')), false);
  assert.equal(launched.element('quick-panel').hidden, false);
  assert.equal(launched.element('interaction-prompt').hidden, false);
  launched.windowEvents.get('pointerdown').forEach(listener => listener({}));
  assert.equal(launched.element('interaction-prompt').hidden, true);
  launched.context.Module.haloMessage(6, JSON.stringify({ phase: 'playing', message: 'Playing Blood Gulch.' }));
  assert.equal(launched.element('quick-panel').hidden, true);
  assert.deepEqual(launched.phases, ['playing']);
});

test('existing host selection joins its exact address without a menu click', async () => {
  const joined = await launcher(undefined, { url: 'http://localhost:8780/?room=FRIENDS9',
    quickPlay: async ({ room }) => ({ role: 'join', room, hostAddress: 0x0403020a }) });
  assert.ok(joined.context.Module.arguments.includes('--HALO_QUICK_PLAY=join'));
  assert.ok(joined.context.Module.arguments.includes('--HALO_QUICK_PLAY_TARGET=10.2.3.4'));
});

test('connection-panel Show log opens the same current runtime and saved diagnostic log as Settings', async () => {
  const page = await launcher(undefined, { quickPlay: async ({ room }) => ({ role: 'join', room, hostAddress: ADDRESS }) });
  let diskLog = 'native search attempt one';
  page.context.navigator.storage.getDirectory = async () => ({
    async getFileHandle(name) {
      assert.equal(name, 'debug.txt');
      return { getFile: async () => ({ text: async () => diskLog }) };
    },
  });
  const module = page.context.Module;
  module.print('runtime: searching for the elected host');
  module.haloMessage(6, JSON.stringify({ phase: 'searching', message: 'Finding game…' }));
  assert.equal(page.element('quick-panel').hidden, false);
  assert.equal(page.element('log-view').hidden, true, 'connecting does not open the log by itself');
  assert.equal(page.element('quick-log').onclick, page.element('show-log').onclick);
  await page.element('quick-log').onclick();
  assert.equal(page.element('log-view').hidden, false);
  assert.match(page.element('log-text').textContent, /runtime: searching for the elected host/);
  assert.match(page.element('log-text').textContent, /--- debug.txt ---\nnative search attempt one/);
  page.element('log-close').onclick();
  assert.equal(page.element('log-view').hidden, true);
  diskLog += '\nnative search timed out';
  module.printErr('runtime: no game response');
  module.haloMessage(6, JSON.stringify({ phase: 'error', message: 'No game response.' }));
  assert.equal(page.element('quick-panel').hidden, false);
  assert.equal(page.element('log-view').hidden, true, 'an error leaves the viewer closed until requested');
  await page.element('quick-log').onclick();
  assert.match(page.element('log-text').textContent, /runtime: no game response/);
  assert.match(page.element('log-text').textContent, /native search timed out/);
});

for (const dataRoot of ['/data', '/data/halo/data']) {
  test(`Show log reads the active ${dataRoot} game directory without confusing the OPFS mount`, async () => {
    const page = await launcher(undefined, { url: 'http://localhost:8780/?menu=1',
      mapsState: async () => ({ files: ['ui.map'], bytes: 2048, dataRoot, saveRoot: '/data/save' }) });
    const reads = [];
    const directory = path => ({
      async getDirectoryHandle(name, options) {
        assert.equal(options?.create, undefined, 'log lookup never creates directories');
        return directory(path + name + '/');
      },
      async getFileHandle(name) {
        reads.push(path + name);
        return { getFile: async () => ({ text: async () => 'log from ' + path + name }) };
      },
    });
    page.context.navigator.storage.getDirectory = async () => directory('');
    await page.element('show-log').onclick();
    const expected = dataRoot === '/data' ? 'debug.txt' : 'halo/data/debug.txt';
    assert.deepEqual(reads, [expected]);
    assert.ok(page.element('log-text').textContent.includes('log from ' + expected));
  });
}

test('an unavailable legacy log does not display an unrelated stale root log', async () => {
  const page = await launcher(undefined, { url: 'http://localhost:8780/?menu=1',
    mapsState: async () => ({ files: ['ui.map'], bytes: 2048, dataRoot: '/data/halo/data', saveRoot: '/data/save' }) });
  let rootReads = 0;
  page.context.navigator.storage.getDirectory = async () => ({
    getDirectoryHandle: async () => { throw new Error('Log directory unavailable'); },
    async getFileHandle() {
      rootReads++;
      return { getFile: async () => ({ text: async () => 'stale root log' }) };
    },
  });
  await page.element('show-log').onclick();
  assert.equal(rootReads, 0);
  assert.equal(page.element('log-text').textContent.includes('stale root log'), false);
  assert.equal(page.element('log-view').hidden, false, 'page diagnostics remain readable');
});

test('map downloaders remain ineligible until complete, then launch automatically', async () => {
  let finishMaps;
  const loading = await launcher(undefined, {
    mapsState: async () => null,
    download: () => new Promise(resolve => { finishMaps = resolve; }),
    quickPlay: async ({ room }) => ({ role: 'host', room }),
  });
  assert.equal(loading.quickCalls.length, 0);
  assert.equal(loading.context.Module, undefined);
  finishMaps({ files: ['ui.map'], bytes: 2048, dataRoot: '/data', saveRoot: '/data/save' });
  await new Promise(setImmediate);
  assert.equal(loading.quickCalls.length, 1);
  assert.ok(loading.context.Module.arguments.includes('--HALO_QUICK_PLAY=host'));
});

test('Main menu cancels pending election and ignores its stale host result', async () => {
  let choose;
  const waiting = await launcher(undefined, { quickPlay: ({ room }) =>
    new Promise(resolve => { choose = () => resolve({ role: 'host', room }); }) });
  waiting.element('main-menu').onclick();
  assert.equal(waiting.quickCalls[0].signal.aborted, true);
  assert.ok(waiting.context.Module);
  assert.equal(waiting.context.Module.arguments.some(value => value.startsWith('--HALO_QUICK_PLAY')), false);
  choose();
  await new Promise(setImmediate);
  assert.equal(waiting.context.Module.arguments.some(value => value.startsWith('--HALO_QUICK_PLAY')), false);
});

test('Main menu cancels a running quick session through the native export', async () => {
  const active = await launcher(undefined, { quickPlay: async ({ room }) => ({ role: 'host', room }) });
  let canceled = 0;
  active.context.Module._web_quick_play_cancel = () => canceled++;
  active.element('quick-menu').onclick();
  assert.equal(canceled, 1);
  assert.equal(active.quickCalls[0].signal.aborted, true);
  assert.equal(active.element('quick-panel').hidden, true);
});

test('room failure requires an explicit retry and does not repeatedly launch', async () => {
  let attempts = 0;
  const failed = await launcher(undefined, { quickPlay: async ({ room }) => {
    if (++attempts === 1) throw new Error('No host can connect.');
    return { role: 'host', room };
  } });
  assert.equal(attempts, 1);
  assert.equal(failed.context.Module, undefined);
  assert.equal(failed.element('quick-retry').hidden, false);
  assert.match(failed.element('quick-status').textContent, /No host can connect/);
  await new Promise(setImmediate);
  assert.equal(attempts, 1);
  await failed.element('quick-retry').onclick();
  assert.equal(attempts, 2);
  assert.ok(failed.context.Module.arguments.includes('--HALO_QUICK_PLAY=host'));
  failed.context.Module.haloMessage(6, JSON.stringify({ phase: 'error', message: 'Match failed.' }));
  assert.equal(failed.element('quick-panel').dataset.state, 'error');
  assert.equal(failed.quickCalls[1].signal.aborted, true);
  assert.equal(attempts, 2);
});

test('Main menu supersedes a pending native handshake without installing it or reopening errors', async () => {
  let installs = 0;
  const manual = await launcher(async () => { installs++; });
  manual.element('invite-input').value = TOKEN;
  const pending = manual.element('invite-connect').onclick();
  const socket = FakeSocket.latest;
  manual.element('main-menu').onclick();
  assert.ok(manual.context.Module);
  assert.equal(manual.context.Module.arguments.some(value => value.startsWith('--HALO_QUICK_PLAY')), false);
  socket.json({ type: 'ready', identifier: '010203040506', address: ADDRESS });
  await pending;
  assert.equal(installs, 0);
  assert.equal(socket.readyState, 3);
  assert.equal(manual.element('quick-panel').hidden, true);
  assert.equal(manual.element('quick-status').textContent, 'Main menu selected.');
});

test('Main menu during native transport installation reloads manual mode before engine launch', async () => {
  let finishInstall;
  const manual = await launcher(() => new Promise(resolve => { finishInstall = resolve; }));
  manual.element('invite-input').value = TOKEN;
  const pending = manual.element('invite-connect').onclick();
  const socket = FakeSocket.latest;
  socket.json({ type: 'ready', identifier: '010203040506', address: ADDRESS });
  await new Promise(setImmediate);
  manual.element('main-menu').onclick();
  assert.equal(manual.context.location.searchParams.get('menu'), '1');
  assert.equal(manual.context.Module, undefined, 'old page cannot launch with a partially installed transport');
  finishInstall();
  await pending;
  assert.equal(socket.readyState, 3);
  assert.equal(manual.element('quick-panel').hidden, true);
  assert.equal(manual.element('quick-status').textContent, 'Main menu selected.');
});

test('Main menu clears an invite queued behind the map download', async () => {
  let finishMaps;
  const before = FakeSocket.latest;
  const manual = await launcher(undefined, {
    mapsState: async () => null,
    download: () => new Promise(resolve => { finishMaps = resolve; }),
  });
  manual.element('invite-input').value = TOKEN;
  await manual.element('invite-connect').onclick();
  manual.element('main-menu').onclick();
  finishMaps({ files: ['ui.map'], bytes: 2048, dataRoot: '/data', saveRoot: '/data/save' });
  await new Promise(setImmediate);
  assert.equal(FakeSocket.latest, before);
  assert.ok(manual.context.Module);
  assert.equal(manual.context.Module.arguments.some(value => value.startsWith('--HALO_QUICK_PLAY')), false);
});
