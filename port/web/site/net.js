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

// Election is deliberately limited to visible, connected quick-play players.
// A room is not a consensus service: disconnected network partitions can each
// form a game. Once a host is reserved, later participants join that host.
class HaloQuickCoordinator {
  static DISCOVERY_MS = 6000;
  static SETTLE_MS = 2000;
  static RESERVE_MS = 1500;
  static TIMEOUT_MS = 35000;
  static HOST_GRACE_MS = 10000;
  static HEARTBEAT_TIMEOUT_MS = 25000;
  constructor(id, address, now) {
    this.id = id;
    this.address = address;
    this.started = now;
    this.brokerSince = null;
    this.selectedSince = now;
    this.selected = null;
    this.reservedSince = null;
    this.result = null;
    this.epoch = 0;
    this.recovering = false;
    this.missingSince = null;
    this.presence = { role: 'candidate', hostId: null, phase: 'electing', gamePhase: '', epoch: 0, failover: true };
  }
  recover(now, epoch = this.epoch + 1) {
    this.epoch = epoch;
    this.recovering = true;
    this.started = now;
    this.brokerSince = null;
    this.selectedSince = now;
    this.selected = null;
    this.reservedSince = null;
    this.result = null;
    this.missingSince = null;
    this.presence = { role: 'candidate', hostId: null, phase: 'electing', gamePhase: '', epoch, failover: true };
  }
  select(id, now) {
    if (id !== this.selected) {
      this.selected = id; this.selectedSince = now;
      if (this.presence.role === 'join') this.reservedSince = now;
    }
    this.presence.hostId = id;
  }
  tick(now, peers, brokerReady) {
    const newest = Math.max(this.epoch, ...peers.filter(peer => peer.open && peer.quick)
      .map(peer => peer.quick.epoch || 0));
    if (newest > this.epoch) this.recover(now, newest);
    if (this.result?.role === 'join') {
      const host = peers.find(peer => peer.id === this.result.hostId);
      const healthy = host?.open && host.quick?.role === 'host' &&
        (host.quick.epoch || 0) === this.epoch && (!host.quick.failover ||
          !Number.isFinite(host.lastPacketAt) || now - host.lastPacketAt < HaloQuickCoordinator.HEARTBEAT_TIMEOUT_MS);
      if (healthy) this.missingSince = null;
      else {
        if (this.missingSince === null) this.missingSince = now;
        if (now - this.missingSince < HaloQuickCoordinator.HOST_GRACE_MS)
          return { state: 'reconnecting', message: 'The host connection was lost. Waiting briefly for it to return…' };
        this.recover(now);
      }
    }
    if (this.result) return { result: this.result };
    if (now - this.started >= HaloQuickCoordinator.TIMEOUT_MS)
      return { error: 'The room could not agree on a reachable host. Check the connection, then try again.' };
    if (brokerReady && this.brokerSince === null) this.brokerSince = now;
    const active = peers.filter(peer => peer.quick && (peer.quick.epoch || 0) === this.epoch && Number.isInteger(peer.address) &&
      peer.address > 0 && peer.address <= 0xffffffff);
    const hosts = active.filter(peer => peer.quick.role === 'host');
    if (this.presence.role === 'host') hosts.push({ id: this.id, address: this.address, open: true, quick: this.presence });
    hosts.sort((a, b) => Number(b.quick.phase === 'launched') - Number(a.quick.phase === 'launched') || a.id.localeCompare(b.id));
    if (hosts.length) {
      const host = hosts[0];
      this.select(host.id, now);
      if (!host.open) return { state: 'connecting', message: 'Connecting to the room’s host…' };
      if (host.id !== this.id && this.presence.role !== 'join') {
        this.presence = { role: 'join', hostId: host.id, phase: 'reserved', gamePhase: '', epoch: this.epoch, failover: true };
        this.reservedSince = now;
      }
      if (now - this.reservedSince < HaloQuickCoordinator.RESERVE_MS)
        return { state: 'coordinating', message: 'Confirming the multiplayer host…' };
      this.result = { role: host.id === this.id ? 'host' : 'join', hostId: host.id, hostAddress: host.address };
      return { result: this.result };
    }
    if (this.presence.role === 'join') return { error: 'The selected host left before the game started. Try again.' };
    if (active.some(peer => peer.quick.role === 'join'))
      return { state: 'connecting', message: 'Waiting for the room’s existing host…' };
    if (!brokerReady) return { state: 'connecting', message: 'Connecting to the multiplayer room…' };
    const candidates = active.filter(peer => peer.quick.role === 'candidate');
    const selected = [this.id, ...candidates.map(peer => peer.id)].sort()[0];
    this.select(selected, now);
    if (candidates.some(peer => !peer.open))
      return { state: 'connecting', message: 'Connecting the players before choosing a host…' };
    const settled = now - this.brokerSince >= HaloQuickCoordinator.DISCOVERY_MS &&
      now - this.selectedSince >= HaloQuickCoordinator.SETTLE_MS;
    // Each connected candidate must have observed and acknowledged the same
    // winner before it can reserve the host role. Idle room members do not vote.
    const acknowledged = candidates.every(peer => peer.quick.hostId === selected);
    if (selected === this.id && settled && acknowledged) {
      this.presence = { role: 'host', hostId: this.id, phase: 'reserved', gamePhase: '', epoch: this.epoch, failover: true };
      this.reservedSince = now;
      return { state: 'coordinating', message: 'Preparing to host the room…' };
    }
    return { state: 'coordinating', message: 'Choosing a multiplayer host…' };
  }
  launched(phase = 'loading') {
    if (this.result) { this.presence.phase = 'launched'; this.presence.gamePhase = phase; }
  }
}

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
  const MAX_RELIABLE_QUEUE = 8 * 1024 * 1024;
  const MAX_CHANNEL_BUFFER = 1024 * 1024;
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
    transport: null,     // native invite gateway, when selected
    quick: null,
    quickSequence: 0,
    roomGeneration: 0,
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
      broker.buffer = new Uint8Array(0);
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
        if (body.length !== 2 || body[1] !== 0) { broker.socket.close(); return; }
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
    const generation = state.roomGeneration;
    message.from = state.id;
    message.mid = randomId();
    const payload = await seal(message);
    if (generation !== state.roomGeneration || !state.room) return;
    const body = [...mqttString(state.topic), ...payload];
    const packet = mqttPacket(0x30, body);
    for (const broker of state.brokers) {
      if (broker.ready && broker.socket.readyState === 1) broker.socket.send(packet);
    }
  }

  async function receiveSignal(bytes) {
    const generation = state.roomGeneration;
    const message = await open(bytes);
    if (generation !== state.roomGeneration || !state.room) return;
    if (!message || message.from === state.id || state.seen.has(message.mid)) return;
    state.seen.add(message.mid);
    if (state.seen.size > 5000) state.seen = new Set([...state.seen].slice(-2000));
    if (message.to && message.to !== state.id) return;
    handleSignal(message);
  }

  // ---------- peers

  function hello() {
    publish({ type: 'hello', address: state.address, name: playerName(),
      quick: state.quick ? { ...state.quick.coordinator.presence } : null, quickSequence: state.quickSequence });
  }

  function playerName() {
    try { return localStorage.getItem('halo-web-player-name') || 'Player'; } catch { return 'Player'; }
  }

  function peerFor(id, address, name) {
    let peer = state.peers.get(id);
    if (!peer) {
      peer = { id, address, name: name || 'Player', pc: null, reliable: null, unreliable: null, open: false,
        lastSeen: Date.now(), pendingCandidates: [], quick: null, quickSequence: -1 };
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
    peer.received = [];
    peer.receivedHead = 0;
    peer.receivedBytes = 0;
    for (const channel of [peer.reliable, peer.unreliable]) {
      channel.binaryType = 'arraybuffer';
    }
    peer.reliable.onmessage = (event) => {
      if (peer.pc !== pc || !state.shared) return;
      peer.lastPacketAt = Date.now();
      const packet = new Uint8Array(event.data);
      if (packet.length < PACKET_HEADER || packet.length > state.shared.offsets.netInBytes ||
          new DataView(packet.buffer).getUint32(0, true) !== packet.length ||
          peer.receivedBytes + packet.length > MAX_RELIABLE_QUEUE) {
        // Disconnect on overflow instead of silently corrupting the byte stream.
        dropPeer(peer);
        return;
      }
      peer.received.push(packet);
      peer.receivedBytes += packet.length;
      pump();
    };
    peer.unreliable.onmessage = (event) => {
      if (peer.pc !== pc) return;
      peer.lastPacketAt = Date.now();
      // Heartbeats use the room's established channel, independently of MQTT.
      // Older launchers ignore these strings and still send normal game packets.
      if (typeof event.data === 'string') {
        if (event.data === 'halo-room-ping-v1' && peer.unreliable.readyState === 'open')
          peer.unreliable.send('halo-room-pong-v1');
        return;
      }
      incoming(new Uint8Array(event.data));
      // Network events also drain output while background timers are throttled.
      pump();
    };
    peer.reliable.bufferedAmountLowThreshold = MAX_CHANNEL_BUFFER / 2;
    peer.reliable.onbufferedamountlow = () => { if (peer.pc === pc) pump(); };
    peer.reliable.onopen = () => {
      if (peer.pc !== pc) return;
      peer.open = true;
      peer.lastPacketAt = Date.now();
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
      if (Number.isSafeInteger(message.quickSequence) && message.quickSequence > peer.quickSequence) {
        peer.quickSequence = message.quickSequence;
        peer.quick = quickPresence(message.quick, message.from);
      }
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
      if (peer.open && peer.quick?.failover && now - peer.lastPacketAt > HaloQuickCoordinator.HEARTBEAT_TIMEOUT_MS) {
        dropPeer(peer);
      } else if (!peer.open && now - peer.lastSeen > PEER_TIMEOUT) {
        dropPeer(peer);
      } else if (!peer.open && peer.pc && now - peer.connectingSince > 15000) {
        // a connection that never opened (a lost offer): start again at the
        // next hello
        try { peer.pc.close(); } catch { /* closed */ }
        peer.pc = null;
      }
      if (peer.open && peer.unreliable?.readyState === 'open' && peer.unreliable.bufferedAmount < MAX_CHANNEL_BUFFER)
        peer.unreliable.send('halo-room-ping-v1');
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
    if (!state.shared || packet.length < PACKET_HEADER) return false;
    const i32 = words();
    const bytes = new Uint8Array(state.shared.memory.buffer);
    const capacity = state.shared.offsets.netInBytes;
    const ring = state.shared.base + state.shared.offsets.netIn;
    const write = Atomics.load(i32, field('netInWrite')) >>> 0;
    const read = Atomics.load(i32, field('netInRead')) >>> 0;
    const size = new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getUint32(0, true);
    if (size !== packet.length || capacity - ((write - read) >>> 0) < size) return false;
    const start = write & (capacity - 1);
    const first = Math.min(capacity - start, size);
    bytes.set(packet.subarray(0, first), ring + start);
    if (first < size) bytes.set(packet.subarray(first), ring);
    Atomics.store(i32, field('netInWrite'), (write + size) | 0);
    return true;
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
      if (channel.bufferedAmount + packet.length > MAX_CHANNEL_BUFFER) return false;
      try { channel.send(packet); return true; } catch { /* closing */ }
    }
    return false;
  }

  function pump() {
    if (!state.shared) return;
    if (state.transport) state.transport.flush(incoming);
    for (const peer of state.peers.values()) {
      if (!peer.received) continue;
      while (peer.receivedHead < peer.received.length && incoming(peer.received[peer.receivedHead]))
        peer.receivedBytes -= peer.received[peer.receivedHead++].length;
      if (peer.receivedHead === peer.received.length) {
        peer.received.length = 0;
        peer.receivedHead = 0;
      } else if (peer.receivedHead > 1024) {
        peer.received.splice(0, peer.receivedHead);
        peer.receivedHead = 0;
      }
    }
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
      if (state.transport) {
        // Leave reliable bytes in the ring until the socket can accept them.
        if (!state.transport.send(packet)) break;
      } else if (header.kind === KIND.DATAGRAM) {
        if (isBroadcast(header.destination)) {
          for (const peer of state.peers.values()) if (peer.open) send(peer.unreliable, packet);
        } else {
          const peer = state.byAddress.get(header.destination);
          if (peer && peer.open) send(peer.unreliable, packet);
        }
      } else {
        const peer = state.byAddress.get(header.destination);
        // WebRTC backpressure must not consume bytes the channel did not take.
        if (peer && peer.open) { if (!send(peer.reliable, packet)) break; }
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
    if (state.transport) state.transport.attach(state.shared);
    // (the game's sockets look at the incoming ring every few milliseconds;
    // this looks at the outgoing one as often)
    state.pumpTimer = setInterval(pump, 4);
  }

  async function join(code, options = {}) {
    if (state.transport) throw new Error('Reload to switch from a desktop invite to a browser room.');
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
    cancelQuickPlay();
    state.roomGeneration++;
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

  async function useTransport(transport) {
    if (state.shared) throw new Error('Reload before joining another desktop invite.');
    await leave();
    if (state.transport) state.transport.close();
    state.transport = transport;
    state.address = transport.address;
  }

  function quickPresence(value, id) {
    if (!value || !/^[a-f0-9]{16}$/.test(id || '') || !['candidate', 'host', 'join'].includes(value.role)) return null;
    if (value.hostId !== null && !/^[a-f0-9]{16}$/.test(value.hostId || '')) return null;
    if (!['electing', 'reserved', 'launched'].includes(value.phase)) return null;
    if ((value.role === 'candidate') !== (value.phase === 'electing')) return null;
    if (value.role === 'host' && value.hostId !== id) return null;
    if (value.role === 'join' && (!value.hostId || value.hostId === id)) return null;
    return { role: value.role, hostId: value.hostId, phase: value.phase,
      epoch: Number.isSafeInteger(value.epoch) && value.epoch >= 0 && value.epoch <= 0x7fffffff ? value.epoch : 0,
      failover: value.failover === true,
      gamePhase: ['hosting', 'searching', 'joining', 'waiting', 'loading', 'playing'].includes(value.gamePhase) ? value.gamePhase : '' };
  }

  function publishQuick() { state.quickSequence++; hello(); }

  function cancelQuickPlay() {
    const attempt = state.quick;
    if (!attempt) return;
    state.quick = null;
    clearInterval(attempt.timer);
    attempt.signal?.removeEventListener('abort', attempt.abort);
    if (!attempt.resolved) attempt.reject(new DOMException('Quick play cancelled.', 'AbortError'));
    publishQuick();
  }

  async function quickPlay({ signal, onStatus = () => {}, onFailover = () => {} } = {}) {
    if (signal?.aborted) throw new DOMException('Quick play cancelled.', 'AbortError');
    if (state.transport) {
      if (!state.transport.connected || !state.transport.hostAddress) throw new Error('The invited host is not connected.');
      return { role: 'join', room: null, hostId: null, hostAddress: state.transport.hostAddress };
    }
    if (!state.room) throw new Error('Join a browser room before starting multiplayer.');
    if (state.quick) return state.quick.promise;
    const coordinator = new HaloQuickCoordinator(state.id, state.address, Date.now());
    const attempt = { coordinator, room: state.room, signal, timer: null, resolved: false,
      previous: JSON.stringify(coordinator.presence), lastStatus: '', abort: cancelQuickPlay, lastResult: null };
    attempt.promise = new Promise((resolve, reject) => { attempt.resolve = resolve; attempt.reject = reject; });
    state.quick = attempt;
    signal?.addEventListener('abort', attempt.abort, { once: true });
    const tick = () => {
      if (state.quick !== attempt) return;
      const result = coordinator.tick(Date.now(), [...state.peers.values()], state.brokers.some(broker => broker.ready));
      const presence = JSON.stringify(coordinator.presence);
      if (presence !== attempt.previous) { attempt.previous = presence; publishQuick(); }
      if (result.error) {
        if (attempt.resolved) onStatus({ state: 'error', message: result.error });
        attempt.resolved = true; // report this failure, rather than an AbortError
        attempt.reject(new Error(result.error));
        cancelQuickPlay();
      } else if (result.result) {
        const selection = { ...result.result, room: attempt.room };
        const signature = JSON.stringify(result.result) + ':' + coordinator.epoch;
        if (signature !== attempt.lastResult) {
          attempt.lastResult = signature;
          if (!attempt.resolved) { attempt.resolved = true; attempt.resolve(selection); }
          else onFailover(selection);
        }
        coordinator.recovering = false;
      } else if (result.message !== attempt.lastStatus) {
        attempt.lastStatus = result.message;
        onStatus({ state: coordinator.recovering ? 'recovering' : result.state,
          message: coordinator.recovering ? 'Choosing a replacement host. The match will restart…' : result.message });
      }
    };
    publishQuick();
    attempt.timer = setInterval(tick, 100);
    tick();
    return attempt.promise;
  }

  function quickPlayPhase(phase) {
    if (phase === 'menu' || phase === 'error') {
      const coordinator = state.quick?.coordinator;
      if (coordinator && (coordinator.recovering || coordinator.missingSince !== null))
        return true;
      cancelQuickPlay(); return false;
    }
    if (!state.quick || !state.quick.resolved) return;
    if (!['hosting', 'searching', 'joining', 'waiting', 'loading', 'playing'].includes(phase)) return;
    state.quick.coordinator.launched(phase);
    publishQuick();
  }

  function quickPlayLost() {
    if (!state.quick?.resolved || state.quick.coordinator.result?.role !== 'join') return false;
    state.quick.coordinator.recover(Date.now());
    publishQuick();
    return true;
  }

  return { attach, join, leave, newRoomCode, on, status, addressText, useTransport,
    quickPlay, cancelQuickPlay, quickPlayLost, quickPlayPhase, quickPlayStarted: () => quickPlayPhase('loading'),
    get address() { return state.address; } };
})();
if (typeof module !== 'undefined') module.exports = { HaloQuickCoordinator };
