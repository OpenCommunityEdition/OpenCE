import test from 'node:test';
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';
import { readFileSync } from 'node:fs';
import { webcrypto } from 'node:crypto';
import vm from 'node:vm';
const { HaloQuickCoordinator: Coordinator } = createRequire(import.meta.url)('../port/web/site/net.js');

const A = '1111111111111111', B = '2222222222222222', C = '0000000000000000';
const ADDRESS_A = 0x0101010a, ADDRESS_B = 0x0201010a, ADDRESS_C = 0x0301010a;
const peer = (coordinator, open = true) => ({ id: coordinator.id, address: coordinator.address, open,
  quick: { ...coordinator.presence } });

function runTogether(coordinators, end = 10000) {
  const results = new Map();
  for (let now = 0; now <= end; now += 100) {
    const snapshot = coordinators.map(coordinator => peer(coordinator));
    for (const coordinator of coordinators) {
      const status = coordinator.tick(now, snapshot.filter(p => p.id !== coordinator.id), true);
      if (status.result) results.set(coordinator.id, status.result);
    }
  }
  return results;
}

test('idle and download-only room peers are never elected', () => {
  const active = new Coordinator(B, ADDRESS_B, 0);
  const idle = { id: C, address: ADDRESS_C, open: true, quick: null };
  assert.equal(active.tick(0, [idle], true).result, undefined);
  assert.equal(active.tick(5999, [idle], true).result, undefined);
  assert.equal(active.tick(6000, [idle], true).result, undefined); // host reservation is not yet committed
  assert.equal(active.tick(7499, [idle], true).result, undefined);
  assert.deepEqual(active.tick(7500, [idle], true).result,
    { role: 'host', hostId: B, hostAddress: ADDRESS_B });
});

test('simultaneous connected quick entrants agree on one host and its actual address', () => {
  const a = new Coordinator(A, ADDRESS_A, 0), b = new Coordinator(B, ADDRESS_B, 0);
  const results = runTogether([b, a]); // iteration order must not decide the host
  assert.equal(results.size, 2);
  assert.deepEqual(results.get(A), { role: 'host', hostId: A, hostAddress: ADDRESS_A });
  assert.deepEqual(results.get(B), { role: 'join', hostId: A, hostAddress: ADDRESS_A });
});

test('host waits for connected candidates to acknowledge its reservation', () => {
  const a = new Coordinator(A, ADDRESS_A, 0), b = new Coordinator(B, ADDRESS_B, 0);
  a.tick(0, [peer(b)], true);
  a.tick(6000, [peer(b)], true);
  assert.equal(a.presence.role, 'candidate'); // B has not observed an election yet
  b.tick(6100, [peer(a)], true);
  a.tick(6200, [peer(b)], true);
  assert.equal(a.presence.role, 'host');
  assert.equal(a.tick(7699, [peer(b)], true).result, undefined);
  assert.equal(a.tick(7700, [peer(b)], true).result.role, 'host');
});

test('a running host survives a later participant with a smaller ID', () => {
  const host = new Coordinator(B, ADDRESS_B, 0);
  runTogether([host]);
  host.launched('playing');
  const newcomer = new Coordinator(C, ADDRESS_C, 12000);
  newcomer.tick(12000, [peer(host)], true);
  const result = newcomer.tick(13500, [peer(host)], true).result;
  assert.deepEqual(result, { role: 'join', hostId: B, hostAddress: ADDRESS_B });
  assert.equal(host.tick(13500, [peer(newcomer)], true).result.role, 'host');
});

test('known active but unreachable peers time out instead of making a second host', () => {
  const a = new Coordinator(A, ADDRESS_A, 0), b = new Coordinator(B, ADDRESS_B, 0);
  for (let now = 0; now < 35000; now += 1000) {
    assert.equal(a.tick(now, [peer(b, false)], true).result, undefined);
    assert.equal(a.presence.role, 'candidate');
  }
  assert.match(a.tick(35000, [peer(b, false)], true).error, /reachable host/);
});

test('an existing host must be connected before quick join resolves', () => {
  const host = new Coordinator(A, ADDRESS_A, 0);
  runTogether([host]);
  const joiner = new Coordinator(B, ADDRESS_B, 10000);
  assert.equal(joiner.tick(10000, [peer(host, false)], true).result, undefined);
  assert.equal(joiner.tick(20000, [peer(host, false)], true).result, undefined);
  assert.equal(joiner.tick(21000, [peer(host)], true).result, undefined);
  assert.equal(joiner.tick(22500, [peer(host)], true).result.role, 'join');
});

test('a missing host reported by an active joiner is not replaced by another game', () => {
  const a = new Coordinator(A, ADDRESS_A, 0);
  const joined = { id: B, address: ADDRESS_B, open: true,
    quick: { role: 'join', hostId: C, phase: 'launched', gamePhase: 'playing' } };
  assert.equal(a.tick(0, [joined], true).result, undefined);
  assert.equal(a.tick(10000, [joined], true).result, undefined);
  assert.equal(a.presence.role, 'candidate');
});

test('new hosting never proceeds without a working signalling broker', () => {
  const a = new Coordinator(A, ADDRESS_A, 0);
  a.tick(0, [], false);
  assert.equal(a.tick(10000, [], false).result, undefined);
  a.tick(11000, [], true);
  assert.equal(a.tick(16000, [], true).result, undefined);
  a.tick(17000, [], true);
  assert.equal(a.tick(18500, [], true).result.role, 'host');
});

test('a host disappearing during reservation produces a retryable error', () => {
  const host = new Coordinator(A, ADDRESS_A, 0);
  runTogether([host]);
  const joiner = new Coordinator(B, ADDRESS_B, 10000);
  joiner.tick(10000, [peer(host)], true);
  assert.match(joiner.tick(10500, [], true).error, /host left/);
});

// Exercise the real public API and encrypted presence publication with a fake
// broker and deterministic clock. This does not open a network or run Halo.
async function network() {
  let now = 0, nextTimer = 0;
  const timers = new Map(), sockets = [], messages = [], encryption = [];
  class Socket {
    readyState = 1;
    constructor() { sockets.push(this); }
    send() {}
    close() { this.readyState = 3; }
  }
  const subtle = {};
  for (const method of ['importKey', 'deriveKey', 'digest', 'decrypt'])
    subtle[method] = webcrypto.subtle[method].bind(webcrypto.subtle);
  subtle.encrypt = (...args) => {
    messages.push(JSON.parse(new TextDecoder().decode(args[2])));
    const pending = webcrypto.subtle.encrypt(...args);
    encryption.push(pending);
    return pending;
  };
  const context = {
    crypto: { getRandomValues: webcrypto.getRandomValues.bind(webcrypto), subtle },
    Date: class extends Date { static now() { return now; } },
    TextEncoder, TextDecoder, Uint8Array, DOMException,
    localStorage: { getItem() { return null; }, setItem() {} }, WebSocket: Socket,
    setInterval: fn => { const id = ++nextTimer; timers.set(id, fn); return id; },
    clearInterval: id => timers.delete(id), setTimeout() {}, clearTimeout() {},
  };
  vm.runInNewContext(readFileSync(new URL('../port/web/site/net.js', import.meta.url), 'utf8') +
    '\nglobalThis.net = HaloNet;', context);
  await context.net.join('TEST42', { brokers: ['wss://test.invalid'] });
  sockets[0].onmessage({ data: new Uint8Array([0x20, 2, 0, 0]).buffer });
  return { net: context.net, messages,
    tick(time) { now = time; for (const fn of [...timers.values()]) fn(); },
    async close() { await context.net.leave(); await Promise.all(encryption); },
    latest() { return messages.filter(message => message.type === 'hello').at(-1); },
  };
}

test('public quick-play API withdraws cancelled eligibility and allows a fresh attempt', async () => {
  const fixture = await network();
  try {
    assert.equal(fixture.latest().quick, null, 'being in a room is not playing');
    const controller = new AbortController(), statuses = [];
    const attempt = fixture.net.quickPlay({ signal: controller.signal, onStatus: value => statuses.push(value) });
    const rejection = assert.rejects(attempt, { name: 'AbortError' });
    const activeSequence = fixture.latest().quickSequence;
    assert.equal(fixture.latest().quick.role, 'candidate');
    controller.abort();
    await rejection;
    assert.equal(fixture.latest().quick, null);
    assert.ok(fixture.latest().quickSequence > activeSequence);
    assert.ok(statuses.every(value => typeof value.state === 'string' && typeof value.message === 'string'));
    const retry = fixture.net.quickPlay();
    fixture.tick(6000); fixture.tick(7500);
    const result = await retry;
    assert.equal(result.role, 'host'); assert.equal(result.room, 'TEST42');
    assert.equal(result.hostAddress, fixture.net.address);
  } finally { await fixture.close(); }
});

test('abort stays attached after resolution and clears the launched host claim', async () => {
  const fixture = await network();
  try {
    const controller = new AbortController();
    const attempt = fixture.net.quickPlay({ signal: controller.signal });
    fixture.tick(6000); fixture.tick(7500);
    await attempt;
    fixture.net.quickPlayStarted();
    assert.equal(fixture.latest().quick.phase, 'launched');
    assert.equal(fixture.latest().quick.gamePhase, 'loading');
    fixture.net.quickPlayPhase('playing');
    assert.equal(fixture.latest().quick.gamePhase, 'playing');
    controller.abort();
    assert.equal(fixture.latest().quick, null);
    fixture.net.quickPlayPhase('playing');
    assert.equal(fixture.latest().quick, null, 'late game phase cannot resurrect cancellation');
  } finally { await fixture.close(); }
});

test('returning to menu or leaving the room withdraws quick-play eligibility', async () => {
  const fixture = await network();
  try {
    let attempt = fixture.net.quickPlay();
    fixture.tick(6000); fixture.tick(7500); await attempt;
    fixture.net.quickPlayPhase('menu');
    assert.equal(fixture.latest().quick, null);
    attempt = fixture.net.quickPlay();
    const rejection = assert.rejects(attempt, { name: 'AbortError' });
    await fixture.net.leave(); await rejection;
    assert.equal(fixture.net.status().room, null);
    await assert.rejects(fixture.net.quickPlay(), /Join a browser room/);
  } finally { await fixture.close(); }
});

test('native invite transport joins only its connected host and never elects one', async () => {
  const fixture = await network();
  try {
    const transport = { connected: true, address: 0x01404064, hostAddress: 0x02404064, close() {} };
    await fixture.net.useTransport(transport);
    const result = await fixture.net.quickPlay();
    assert.equal(result.role, 'join'); assert.equal(result.room, null);
    assert.equal(result.hostAddress, transport.hostAddress);
    transport.connected = false;
    await assert.rejects(fixture.net.quickPlay(), /host is not connected/);
  } finally { await fixture.close(); }
});
