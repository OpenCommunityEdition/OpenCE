const test = require('node:test');
const assert = require('node:assert/strict');
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
  socket.json({ type: 'peer', identifier: TOKEN.slice(0, 12), address: ADDRESS + 0x1000000, connected: true });
  assert.equal(gateway.connected, true);
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
  gateway.close();
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
