/*
NET.JS

Online play: the players of a room form one network, over which the game's
own system link (its discovery broadcasts and its connections) works as on
a LAN.

- Every copy of the game has an address on that network, 10.x.y.z, kept in
  this browser. The game's sockets (port/web/src/web_net.c) put what they
  send to other addresses in a ring in the shared memory, and take what
  arrives from a second ring.
- The page carries those packets over WebRTC: a data channel to each other
  player, reliable and ordered for the game's connections, unreliable for
  its datagrams. Broadcasts go to every player.
- Players find each other through public MQTT brokers (over secure
  WebSockets), in a topic derived from the room's code. Everything sent there
  is encrypted with a key derived from the code, so only those who have it
  can read the room's messages or join it.

A room is a code the players share (or a link with ?room=CODE). NAT that
WebRTC cannot cross without a relay (some mobile networks) needs a TURN
server, which the settings can name.
*/

'use strict';

const HaloNet = (() => {
  const BROKERS = [
    'wss://broker.emqx.io:8084/mqtt',
    'wss://broker.hivemq.com:8884/mqtt',
    'wss://test.mosquitto.org:8081/mqtt',
  ];
  const STUN = [{ urls: ['stun:stun.l.google.com:19302', 'stun:stun.cloudflare.com:3478'] }];
  const HELLO_INTERVAL = 3000;
  const PEER_TIMEOUT = 20000;
  const PACKET_HEADER = 24;
  const KIND = { DATAGRAM: 1, OPEN: 2, DATA: 3, CLOSE: 4, REFUSE: 5 };

  const state = {
    shared: null,        // { memory, base, offsets }
    room: null,
    key: null,
    topic: null,
    id: randomId(),
    address: 0,          // network byte order
    brokers: [],
    peers: new Map(),    // id -> peer
    byAddress: new Map(),// address -> peer
    seen: new Set(),     // message ids already handled (several brokers)
    listeners: new Set(),
    iceServers: STUN,
    pumpTimer: null,
    helloTimer: null,
  };

  function randomId() {
    const bytes = crypto.getRandomValues(new Uint8Array(8));
    return Array.from(bytes, (b) => b.toString(16).padStart(2, '0')).join('');
  }

  // ---------- this machine's address

  function addressText(address) {
    return [address & 255, (address >>> 8) & 255, (address >>> 16) & 255, address >>> 24].join('.');
  }

  function localAddress() {
    let text = null;
    try { text = localStorage.getItem('halo-web-net-address'); } catch { /* none */ }
    const parts = text ? text.split('.').map(Number) : null;
    if (!parts || parts.length !== 4 || parts[0] !== 10) {
      const random = crypto.getRandomValues(new Uint8Array(3));
      const b = 1 + (random[0] % 254), c = random[1], d = 1 + (random[2] % 254);
      text = `10.${b}.${c}.${d}`;
      try { localStorage.setItem('halo-web-net-address', text); } catch { /* not kept */ }
    }
    const p = text.split('.').map(Number);
    return (p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24)) >>> 0;
  }

  // ---------- events for the page

  function emit(type, detail) {
    for (const listener of state.listeners) listener(type, detail);
  }

  function status() {
    const connected = [...state.peers.values()].filter((peer) => peer.open).length;
    return {
      room: state.room,
      address: addressText(state.address),
      brokers: state.brokers.filter((broker) => broker.ready).length,
      players: connected,
      names: [...state.peers.values()].filter((peer) => peer.open).map((peer) => peer.name),
    };
  }

  // ---------- encryption (AES-GCM, a key from the room's code)

  async function deriveRoom(code) {
    const encoder = new TextEncoder();
    const material = await crypto.subtle.importKey('raw', encoder.encode(code), 'PBKDF2', false, ['deriveKey']);
    const key = await crypto.subtle.deriveKey(
      { name: 'PBKDF2', salt: encoder.encode('halo-web-room-v1'), iterations: 50000, hash: 'SHA-256' },
      material, { name: 'AES-GCM', length: 256 }, false, ['encrypt', 'decrypt']);
    const digest = new Uint8Array(await crypto.subtle.digest('SHA-256', encoder.encode('halo-web-topic-v1:' + code)));
    const topic = 'halo-web/v1/' + Array.from(digest.slice(0, 12), (b) => b.toString(16).padStart(2, '0')).join('');
    return { key, topic };
  }

  async function seal(message) {
    const iv = crypto.getRandomValues(new Uint8Array(12));
    const plain = new TextEncoder().encode(JSON.stringify(message));
    const cipher = new Uint8Array(await crypto.subtle.encrypt({ name: 'AES-GCM', iv }, state.key, plain));
    const result = new Uint8Array(12 + cipher.length);
    result.set(iv);
    result.set(cipher, 12);
    return result;
  }

  async function open(bytes) {
    try {
      const plain = await crypto.subtle.decrypt({ name: 'AES-GCM', iv: bytes.slice(0, 12) }, state.key, bytes.slice(12));
      return JSON.parse(new TextDecoder().decode(plain));
    } catch {
      return null; // another room's, or damaged
    }
  }

  // ---------- MQTT 3.1.1 over WebSockets (QoS 0 only)

  function encodeLength(length) {
    const bytes = [];
    do {
      let byte = length % 128;
      length = Math.floor(length / 128);
      if (length > 0) byte |= 128;
      bytes.push(byte);
    } while (length > 0);
    return bytes;
  }

  function mqttString(text) {
    const bytes = new TextEncoder().encode(text);
    return [bytes.length >> 8, bytes.length & 255, ...bytes];
  }

  function mqttPacket(type, body) {
    return new Uint8Array([type, ...encodeLength(body.length), ...body]);
  }

  function connectBroker(url) {
    const broker = { url, socket: null, ready: false, buffer: new Uint8Array(0), ping: null, retry: null };
    const start = () => {
      let socket;
      try {
        socket = new WebSocket(url, 'mqtt');
      } catch {
        return;
      }
      broker.socket = socket;
      socket.binaryType = 'arraybuffer';
      socket.onopen = () => {
        const body = [...mqttString('MQTT'), 4, 0x02, 0, 60, ...mqttString('halo-' + state.id.slice(0, 12) + '-' +
          Math.floor(Math.random() * 1e6))];
        socket.send(mqttPacket(0x10, body));
      };
      socket.onmessage = (event) => {
        const incoming = new Uint8Array(event.data);
        const joined = new Uint8Array(broker.buffer.length + incoming.length);
        joined.set(broker.buffer);
        joined.set(incoming, broker.buffer.length);
        broker.buffer = joined;
        parseMqtt(broker);
      };
      socket.onclose = () => {
        broker.ready = false;
        clearInterval(broker.ping);
        emit('status', status());
        if (state.room && state.brokers.includes(broker)) broker.retry = setTimeout(start, 5000);
      };
      socket.onerror = () => {};
    };
    start();
    return broker;
  }

  function parseMqtt(broker) {
    for (;;) {
      const bytes = broker.buffer;
      if (bytes.length < 2) return;
      let length = 0, multiplier = 1, index = 1, byte;
      do {
        if (index >= bytes.length) return;
        byte = bytes[index++];
        length += (byte & 127) * multiplier;
        multiplier *= 128;
      } while (byte & 128);
      if (bytes.length < index + length) return;
      const type = bytes[0] & 0xF0;
      const body = bytes.slice(index, index + length);
      broker.buffer = bytes.slice(index + length);
      if (type === 0x20) {
        // CONNACK: subscribe to the room
        const body2 = [0, 1, ...mqttString(state.topic), 0];
        broker.socket.send(mqttPacket(0x82, body2));
        broker.ready = true;
        broker.ping = setInterval(() => {
          if (broker.socket.readyState === 1) broker.socket.send(new Uint8Array([0xC0, 0]));
        }, 30000);
        emit('status', status());
        hello();
      } else if (type === 0x30) {
        const topicLength = (body[0] << 8) | body[1];
        let offset = 2 + topicLength;
        if (bytes[0] & 0x06) offset += 2; // (a packet id, QoS above 0)
        receiveSignal(body.slice(offset));
      }
    }
  }

  async function publish(message) {
    if (!state.room) return;
    message.from = state.id;
    message.mid = randomId();
    const payload = await seal(message);
    const body = [...mqttString(state.topic), ...payload];
    const packet = mqttPacket(0x30, body);
    for (const broker of state.brokers) {
      if (broker.ready && broker.socket.readyState === 1) broker.socket.send(packet);
    }
  }

  async function receiveSignal(bytes) {
    const message = await open(bytes);
    if (!message || message.from === state.id || state.seen.has(message.mid)) return;
    state.seen.add(message.mid);
    if (state.seen.size > 5000) state.seen = new Set([...state.seen].slice(-2000));
    if (message.to && message.to !== state.id) return;
    handleSignal(message);
  }

  // ---------- peers

  function hello() {
    publish({ type: 'hello', address: state.address, name: playerName() });
  }

  function playerName() {
    try { return localStorage.getItem('halo-web-player-name') || 'Player'; } catch { return 'Player'; }
  }

  function peerFor(id, address, name) {
    let peer = state.peers.get(id);
    if (!peer) {
      peer = { id, address, name: name || 'Player', pc: null, reliable: null, unreliable: null, open: false,
        lastSeen: Date.now(), pendingCandidates: [] };
      state.peers.set(id, peer);
    }
    if (address) {
      peer.address = address >>> 0;
      state.byAddress.set(peer.address, peer);
    }
    if (name) peer.name = name;
    peer.lastSeen = Date.now();
    return peer;
  }

  function createConnection(peer) {
    const pc = new RTCPeerConnection({ iceServers: state.iceServers });
    peer.pc = pc;
    peer.connectingSince = Date.now();
    peer.reliable = pc.createDataChannel('reliable', { negotiated: true, id: 0, ordered: true });
    peer.unreliable = pc.createDataChannel('unreliable', { negotiated: true, id: 1, ordered: false, maxRetransmits: 0 });
    for (const channel of [peer.reliable, peer.unreliable]) {
      channel.binaryType = 'arraybuffer';
      channel.onmessage = (event) => incoming(new Uint8Array(event.data));
    }
    peer.reliable.onopen = () => {
      peer.open = true;
      emit('joined', { name: peer.name, address: addressText(peer.address) });
      emit('status', status());
    };
    peer.reliable.onclose = () => { if (peer.pc === pc) dropPeer(peer); };
    pc.onicecandidate = (event) => {
      if (event.candidate) publish({ type: 'candidate', to: peer.id, candidate: event.candidate.toJSON() });
    };
    pc.onconnectionstatechange = () => {
      if (peer.pc !== pc) return;
      if (pc.connectionState === 'failed' || pc.connectionState === 'closed') dropPeer(peer);
    };
    return pc;
  }

  function dropPeer(peer) {
    if (!state.peers.has(peer.id)) return;
    state.peers.delete(peer.id);
    if (state.byAddress.get(peer.address) === peer) state.byAddress.delete(peer.address);
    try { peer.pc && peer.pc.close(); } catch { /* closed */ }
    if (peer.open) emit('left', { name: peer.name });
    peer.open = false;
    emit('status', status());
  }

  async function handleSignal(message) {
    if (message.type === 'hello') {
      const known = state.peers.get(message.from);
      const peer = peerFor(message.from, message.address, message.name);
      if (!known) hello(); // (so that it knows this machine without waiting)
      // the smaller id makes the offer
      if (!peer.pc && state.id < message.from) {
        const pc = createConnection(peer);
        const offer = await pc.createOffer();
        await pc.setLocalDescription(offer);
        publish({ type: 'offer', to: peer.id, sdp: pc.localDescription.sdp, address: state.address, name: playerName() });
      }
    } else if (message.type === 'offer') {
      const peer = peerFor(message.from, message.address, message.name);
      if (peer.pc) {
        try { peer.pc.close(); } catch { /* closed */ }
      }
      const pc = createConnection(peer);
      await pc.setRemoteDescription({ type: 'offer', sdp: message.sdp });
      for (const candidate of peer.pendingCandidates.splice(0)) await pc.addIceCandidate(candidate).catch(() => {});
      const answer = await pc.createAnswer();
      await pc.setLocalDescription(answer);
      publish({ type: 'answer', to: peer.id, sdp: pc.localDescription.sdp });
    } else if (message.type === 'answer') {
      const peer = state.peers.get(message.from);
      if (peer && peer.pc && peer.pc.signalingState === 'have-local-offer') {
        await peer.pc.setRemoteDescription({ type: 'answer', sdp: message.sdp });
        for (const candidate of peer.pendingCandidates.splice(0)) await peer.pc.addIceCandidate(candidate).catch(() => {});
      }
    } else if (message.type === 'candidate') {
      const peer = state.peers.get(message.from);
      if (!peer) return;
      if (peer.pc && peer.pc.remoteDescription) await peer.pc.addIceCandidate(message.candidate).catch(() => {});
      else peer.pendingCandidates.push(message.candidate);
    } else if (message.type === 'bye') {
      const peer = state.peers.get(message.from);
      if (peer) dropPeer(peer);
    }
  }

  function sweep() {
    const now = Date.now();
    for (const peer of [...state.peers.values()]) {
      if (!peer.open && now - peer.lastSeen > PEER_TIMEOUT) {
        dropPeer(peer);
      } else if (!peer.open && peer.pc && now - peer.connectingSince > 15000) {
        // a connection that never opened (a lost offer): start again at the
        // next hello
        try { peer.pc.close(); } catch { /* closed */ }
        peer.pc = null;
      }
    }
  }

  // ---------- the rings (port/web/src/web_shared.h)

  function words() {
    return new Int32Array(state.shared.memory.buffer);
  }

  function field(name) {
    return (state.shared.base + state.shared.offsets[name]) >> 2;
  }

  function incoming(packet) {
    if (!state.shared || packet.length < PACKET_HEADER) return;
    const i32 = words();
    const bytes = new Uint8Array(state.shared.memory.buffer);
    const capacity = state.shared.offsets.netInBytes;
    const ring = state.shared.base + state.shared.offsets.netIn;
    const write = Atomics.load(i32, field('netInWrite')) >>> 0;
    const read = Atomics.load(i32, field('netInRead')) >>> 0;
    const size = new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getUint32(0, true);
    if (size !== packet.length || capacity - ((write - read) >>> 0) < size) return; // full: lost
    const start = write & (capacity - 1);
    const first = Math.min(capacity - start, size);
    bytes.set(packet.subarray(0, first), ring + start);
    if (first < size) bytes.set(packet.subarray(first), ring);
    Atomics.store(i32, field('netInWrite'), (write + size) | 0);
  }

  function refuse(header) {
    // nobody at that address: the connection is refused
    const packet = new Uint8Array(PACKET_HEADER);
    const view = new DataView(packet.buffer);
    view.setUint32(0, PACKET_HEADER, true);
    view.setUint32(4, KIND.REFUSE, true);
    view.setUint32(8, header.destination, true);
    view.setUint32(12, header.source, true);
    view.setUint16(16, header.destinationPort, true);
    view.setUint16(18, header.sourcePort, true);
    view.setUint32(20, 0, true);
    incoming(packet);
  }

  function isBroadcast(address) {
    return address === 0xFFFFFFFF || (address >>> 24) === 255;
  }

  function send(channel, packet) {
    if (channel && channel.readyState === 'open') {
      try { channel.send(packet); } catch { /* closing */ }
    }
  }

  function pump() {
    if (!state.shared) return;
    const i32 = words();
    const memory = new Uint8Array(state.shared.memory.buffer);
    const capacity = state.shared.offsets.netOutBytes;
    const ring = state.shared.base + state.shared.offsets.netOut;
    let read = Atomics.load(i32, field('netOutRead')) >>> 0;
    const write = Atomics.load(i32, field('netOutWrite')) >>> 0;
    while (read !== write) {
      const start = read & (capacity - 1);
      const headerBytes = new Uint8Array(PACKET_HEADER);
      for (let index = 0; index < PACKET_HEADER; index++) headerBytes[index] = memory[ring + ((start + index) & (capacity - 1))];
      const view = new DataView(headerBytes.buffer);
      const size = view.getUint32(0, true);
      if (size < PACKET_HEADER || size > capacity) {
        read = write;
        break;
      }
      const packet = new Uint8Array(size);
      const first = Math.min(capacity - start, size);
      packet.set(memory.subarray(ring + start, ring + start + first));
      if (first < size) packet.set(memory.subarray(ring, ring + size - first), first);
      const header = {
        kind: view.getUint32(4, true),
        source: view.getUint32(8, true),
        destination: view.getUint32(12, true),
        sourcePort: view.getUint16(16, true),
        destinationPort: view.getUint16(18, true),
      };
      if (header.kind === KIND.DATAGRAM) {
        if (isBroadcast(header.destination)) {
          for (const peer of state.peers.values()) if (peer.open) send(peer.unreliable, packet);
        } else {
          const peer = state.byAddress.get(header.destination);
          if (peer && peer.open) send(peer.unreliable, packet);
        }
      } else {
        const peer = state.byAddress.get(header.destination);
        if (peer && peer.open) send(peer.reliable, packet);
        else if (header.kind === KIND.OPEN) refuse(header);
      }
      read = (read + size) >>> 0;
    }
    Atomics.store(i32, field('netOutRead'), read | 0);
  }

  // ---------- the page's interface

  function attach({ memory, base, offsets }) {
    state.shared = { memory, base, offsets };
    const i32 = words();
    Atomics.store(i32, field('netLocalAddress'), state.address | 0);
    // (the game's sockets look at the incoming ring every few milliseconds;
    // this looks at the outgoing one as often)
    state.pumpTimer = setInterval(pump, 4);
  }

  async function join(code, options = {}) {
    code = String(code || '').trim().toUpperCase().replace(/[^A-Z0-9]/g, '');
    if (code.length < 4) throw new Error('A room code has at least 4 letters or digits.');
    await leave();
    const { key, topic } = await deriveRoom(code);
    state.room = code;
    state.key = key;
    state.topic = topic;
    if (options.turn && options.turn.urls) state.iceServers = [...STUN, options.turn];
    const brokers = options.brokers && options.brokers.length ? options.brokers : BROKERS;
    state.brokers = brokers.map(connectBroker);
    state.helloTimer = setInterval(() => { hello(); sweep(); }, HELLO_INTERVAL);
    emit('status', status());
    return code;
  }

  async function leave() {
    if (!state.room) return;
    await publish({ type: 'bye' }).catch(() => {});
    clearInterval(state.helloTimer);
    for (const peer of [...state.peers.values()]) dropPeer(peer);
    for (const broker of state.brokers) {
      clearTimeout(broker.retry);
      clearInterval(broker.ping);
      try { broker.socket && broker.socket.close(); } catch { /* closed */ }
    }
    state.brokers = [];
    state.room = null;
    emit('status', status());
  }

  function newRoomCode() {
    const alphabet = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
    const bytes = crypto.getRandomValues(new Uint8Array(6));
    return Array.from(bytes, (b) => alphabet[b % alphabet.length]).join('');
  }

  function on(listener) {
    state.listeners.add(listener);
  }

  state.address = localAddress();

  return { attach, join, leave, newRoomCode, on, status, addressText, get address() { return state.address; } };
})();
