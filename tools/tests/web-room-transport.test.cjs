const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

function fixture() {
  const intervals = [];
  class RTC {
    createDataChannel() {
      return { readyState: 'open', bufferedAmount: 0, sent: [],
        send(packet) { this.sent.push(packet.slice()); } };
    }
    close() { this.closed = true; }
  }
  const context = { RTCPeerConnection: RTC, Uint8Array, Int32Array, DataView, Atomics,
    crypto: { getRandomValues(bytes) { bytes.fill(1); return bytes; } },
    localStorage: { getItem() { return null; }, setItem() {} },
    setInterval(fn) { intervals.push(fn); }, clearInterval() {}, clearTimeout() {},
  };
  // Expose only the boundary needed by this isolated VM; run the production
  // channel callbacks, ring writes and pump unchanged, without an MQTT server.
  let source = fs.readFileSync(require.resolve('../../port/web/site/net.js'), 'utf8');
  assert.ok(source.includes('return { attach, join, leave,'));
  source = source.replace('return { attach, join, leave,',
    'return { peerFor, createConnection, pump, attach, join, leave,');
  vm.runInNewContext(source + '\nglobalThis.net = HaloNet;', context);
  const net = context.net;
  const memory = { buffer: new SharedArrayBuffer(1024) };
  const offsets = { netInWrite: 0, netInRead: 4, netOutWrite: 8, netOutRead: 12,
    netLocalAddress: 16, netIn: 64, netOut: 320, netInBytes: 256, netOutBytes: 256 };
  net.attach({ memory, base: 0, offsets });
  const peer = net.peerFor('2222222222222222', 0x0201010a, 'Peer');
  const pc = net.createConnection(peer);
  peer.reliable.onopen();
  const words = new Int32Array(memory.buffer), bytes = new Uint8Array(memory.buffer);
  return { net, peer, pc, words, bytes, intervals,
    receive(packet) { peer.reliable.onmessage({ data: packet.buffer }); },
    consume() {
      const result = [];
      let read = words[1] >>> 0;
      const write = words[0] >>> 0;
      while (read !== write) result.push(bytes[64 + ((read++) & 255)]);
      Atomics.store(words, 1, read);
      return result;
    },
  };
}

function packet(kind, length = 24, fill = 0) {
  const bytes = new Uint8Array(length).fill(fill), view = new DataView(bytes.buffer);
  view.setUint32(0, length, true);
  view.setUint32(4, kind, true);
  view.setUint32(12, 0x0201010a, true);
  return bytes;
}

test('reliable OPEN, DATA and CLOSE survive a full receive ring in order', () => {
  const f = fixture();
  const packets = [packet(2), packet(3, 180, 1), packet(3, 180, 2), packet(4)];
  for (const p of packets) f.receive(p);
  assert.ok(f.peer.receivedBytes > 0, 'overflow remains queued instead of being dropped');
  const received = f.consume();
  f.net.pump();
  received.push(...f.consume());
  assert.deepEqual(received, packets.flatMap(p => Array.from(p)));
  assert.equal(f.peer.receivedBytes, 0);
  assert.equal(f.peer.received.length, 0);
});

test('outbound reliable bytes remain in the ring under WebRTC backpressure', () => {
  const f = fixture(), p = packet(3, 180, 7);
  f.bytes.set(p, 320); f.words[2] = p.length;
  f.peer.reliable.bufferedAmount = 1024 * 1024;
  f.net.pump();
  assert.equal(f.words[3], 0);
  assert.equal(f.peer.reliable.sent.length, 0);
  f.peer.reliable.bufferedAmount = 0;
  f.peer.reliable.onbufferedamountlow();
  assert.equal(f.words[3], p.length);
  assert.deepEqual(Array.from(f.peer.reliable.sent[0]), Array.from(p));
});

test('incoming network events flush output even without timer callbacks', () => {
  const f = fixture(), p = packet(3, 80, 9);
  f.bytes.set(p, 320); f.words[2] = p.length;
  f.peer.unreliable.onmessage({ data: packet(1).buffer });
  assert.equal(f.words[3], p.length);
  assert.equal(f.peer.reliable.sent.length, 1);
});

test('failed send retains stream bytes and malformed input disconnects', () => {
  const f = fixture(), p = packet(3, 80, 9);
  f.bytes.set(p, 320); f.words[2] = p.length;
  f.peer.reliable.send = () => { throw new Error('buffer full'); };
  f.net.pump();
  assert.equal(f.words[3], 0);
  const malformed = packet(3, 40);
  new DataView(malformed.buffer).setUint32(0, 80, true);
  f.receive(malformed);
  assert.equal(f.pc.closed, true);
  assert.equal(f.net.status().players, 0);
});
