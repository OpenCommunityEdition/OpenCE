/*
NET.JS

Rooms: browsers that share a room's link play system link with each other
as machines of one local network (port/web/src/web_net.c). Each has an
address of its own in 10.0.0.0/8, which the game is started with
(HALO_WEB_ADDRESS); what the game sends to another's address, or broadcasts,
web_net.c hands here as a frame, which goes to that browser over WebRTC (a
data channel that may lose it for datagrams, one that does not for streams'
frames), and what arrives is handed to the game. The browsers of a room
connect to each other directly (WebRTC finds a way through each one's
router with the STUN servers internet play uses).

They find each other as the native builds' machines do, through public MQTT
brokers, over WebSockets here: each browser publishes to the room's topic
that it is there, and the two of a pair exchange their WebRTC descriptions
there. The room's link carries a 128-bit secret: the topic is named by a
hash of it and every message is sealed with a key derived from it
(AES-GCM), so that the brokers and anyone else see nothing of the room, and
cannot speak in it. Messages older than two minutes, or seen before, are
dropped. What arrives from another browser is untrusted: the game checks
every frame (web_net_receive).

?brokers=ws://host:port[,...] in the page's address uses other brokers (the
tests' own).
*/

'use strict';

window.HaloNet = (() => {
  const BROKERS = ['wss://broker.emqx.io:8084/mqtt', 'wss://broker.hivemq.com:8884/mqtt',
    'wss://test.mosquitto.org:8081/mqtt'];
  // (port/linux/src/port_config.c's network.stun_servers)
  const ICE_SERVERS = [{ urls: ['stun:stun.l.google.com:19302', 'stun:stun.cloudflare.com:3478'] }];
  const TOPIC_PREFIX = 'opence/web/1/room/';
  const HELLO_MS = 5000;
  const PEER_TIMEOUT_MS = 20000;
  // (a pair not connected by then starts again: an offer or an answer sent
  // before the other had its subscription is lost)
  const CONNECT_MS = 12000;
  const MESSAGE_AGE_MS = 120000;
  const GATHER_MS = 4000;
  const MAXIMUM_PEERS = 15;
  const MAXIMUM_MESSAGE = 64 * 1024;
  const MAXIMUM_FRAME = 70000;
  // (a channel that holds this much unsent drops the datagrams sent to it)
  const DATAGRAM_BUFFERED_LIMIT = 1 << 20;
  const EVERYONE = 0xffffffff;

  const encoder = new TextEncoder();
  const decoder = new TextDecoder();
  const hex = (bytes) => [...bytes].map((b) => b.toString(16).padStart(2, '0')).join('');
  const random = (count) => crypto.getRandomValues(new Uint8Array(count));

  let room = null;
  let game = null;
  const incoming = [];
  const listeners = new Set();

  // ---------- MQTT 3.1.1 over a WebSocket: connect, subscribe, publish at
  // QoS 0, ping

  function lengthBytes(length) {
    const bytes = [];
    do {
      let byte = length % 128;
      length = Math.floor(length / 128);
      if (length) byte |= 128;
      bytes.push(byte);
    } while (length);
    return bytes;
  }

  function utf8Field(text) {
    const bytes = encoder.encode(text);
    return [bytes.length >> 8, bytes.length & 255, ...bytes];
  }

  function packet(type, body) {
    const head = [type, ...lengthBytes(body.length)];
    const bytes = new Uint8Array(head.length + body.length);
    bytes.set(head);
    bytes.set(body, head.length);
    return bytes;
  }

  class Broker {
    constructor(url, topic, onMessage) {
      this.url = url;
      this.topic = topic;
      this.onMessage = onMessage;
      this.buffer = new Uint8Array(0);
      this.ready = false;
      this.closed = false;
      this.retry = 1000;
      this.open();
    }

    open() {
      if (this.closed) return;
      try {
        this.socket = new WebSocket(this.url, 'mqtt');
      } catch {
        this.later();
        return;
      }
      this.socket.binaryType = 'arraybuffer';
      this.socket.onopen = () => {
        const id = 'opence' + hex(random(8));
        const body = [...utf8Field('MQTT'), 4, 0x02, 0, 60, ...utf8Field(id)];
        this.socket.send(packet(0x10, body));
      };
      this.socket.onmessage = (event) => this.received(new Uint8Array(event.data));
      this.socket.onclose = () => {
        this.ready = false;
        clearInterval(this.pinger);
        this.later();
      };
      this.socket.onerror = () => {};
    }

    later() {
      if (this.closed) return;
      setTimeout(() => this.open(), this.retry);
      this.retry = Math.min(this.retry * 2, 30000);
    }

    received(bytes) {
      const joined = new Uint8Array(this.buffer.length + bytes.length);
      joined.set(this.buffer);
      joined.set(bytes, this.buffer.length);
      this.buffer = joined;
      for (;;) {
        if (this.buffer.length < 2) return;
        let length = 0;
        let multiplier = 1;
        let at = 1;
        for (;;) {
          if (at >= this.buffer.length || at > 4) return;
          const byte = this.buffer[at++];
          length += (byte & 127) * multiplier;
          multiplier *= 128;
          if (!(byte & 128)) break;
        }
        if (length > MAXIMUM_MESSAGE * 2) {
          this.socket.close();
          return;
        }
        if (this.buffer.length < at + length) return;
        this.handle(this.buffer[0], this.buffer.subarray(at, at + length));
        this.buffer = this.buffer.slice(at + length);
      }
    }

    handle(first, body) {
      const type = first >> 4;
      if (type === 2) {
        // CONNACK: subscribe to the room
        if (body[1] !== 0) {
          this.socket.close();
          return;
        }
        this.retry = 1000;
        this.socket.send(packet(0x82, [0, 1, ...utf8Field(this.topic), 0]));
        this.pinger = setInterval(() => this.socket.readyState === 1 && this.socket.send(new Uint8Array([0xC0, 0])), 30000);
      } else if (type === 9) {
        this.ready = true;
        if (room) room.brokerReady();
      } else if (type === 3 && body.length >= 2) {
        const topicLength = (body[0] << 8) | body[1];
        let at = 2 + topicLength;
        if ((first >> 1) & 3) at += 2;
        if (at <= body.length && body.length - at <= MAXIMUM_MESSAGE) this.onMessage(body.slice(at));
      }
    }

    publish(payload) {
      if (!this.ready || this.socket.readyState !== 1) return;
      this.socket.send(packet(0x30, [...utf8Field(this.topic), ...payload]));
    }

    close() {
      this.closed = true;
      clearInterval(this.pinger);
      try {
        this.socket.close();
      } catch {
        // already
      }
    }
  }

  // ---------- the room

  async function deriveKeys(secret) {
    const material = await crypto.subtle.importKey('raw', secret, 'HKDF', false, ['deriveKey', 'deriveBits']);
    const key = await crypto.subtle.deriveKey({ name: 'HKDF', hash: 'SHA-256', salt: encoder.encode('opence-web-room-1'),
      info: encoder.encode('seal') }, material, { name: 'AES-GCM', length: 256 }, false, ['encrypt', 'decrypt']);
    const topicBits = await crypto.subtle.deriveBits({ name: 'HKDF', hash: 'SHA-256',
      salt: encoder.encode('opence-web-room-1'), info: encoder.encode('topic') }, material, 128);
    return { key, topic: TOPIC_PREFIX + hex(new Uint8Array(topicBits)) };
  }

  function dotted(address) {
    return [address & 255, (address >>> 8) & 255, (address >>> 16) & 255, address >>> 24].join('.');
  }

  // an address in 10.0.0.0/8, in network byte order (as web_net.c keeps it)
  function newAddress() {
    const bytes = random(3);
    return (10 | (bytes[0] << 8) | (bytes[1] << 16) | ((1 + (bytes[2] % 254)) << 24)) >>> 0;
  }

  class Room {
    constructor(secret, brokers) {
      this.secret = secret;
      this.brokerUrls = brokers;
      this.id = hex(random(8));
      this.address = newAddress();
      this.addressFixed = false;
      this.peers = new Map();
      this.byAddress = new Map();
      this.seen = new Map();
      this.brokers = [];
    }

    async start() {
      const { key, topic } = await deriveKeys(this.secret);
      this.key = key;
      this.brokers = this.brokerUrls.map((url) => new Broker(url, topic, (payload) => this.message(payload)));
      this.helloTimer = setInterval(() => this.tick(), HELLO_MS);
    }

    brokerReady() {
      this.hello();
      changed();
    }

    connectedBrokers() {
      return this.brokers.filter((broker) => broker.ready).length;
    }

    async seal(message) {
      const iv = random(12);
      const text = encoder.encode(JSON.stringify({ ...message, from: this.id, time: Date.now(), nonce: hex(random(8)) }));
      const sealed = new Uint8Array(await crypto.subtle.encrypt({ name: 'AES-GCM', iv }, this.key, text));
      const payload = new Uint8Array(12 + sealed.length);
      payload.set(iv);
      payload.set(sealed, 12);
      return payload;
    }

    async send(message) {
      const payload = await this.seal(message);
      for (const broker of this.brokers) broker.publish(payload);
    }

    hello() {
      this.send({ type: 'hello', address: this.address });
    }

    async message(payload) {
      let message;
      try {
        if (payload.length < 13) return;
        const text = await crypto.subtle.decrypt({ name: 'AES-GCM', iv: payload.subarray(0, 12) }, this.key,
          payload.subarray(12));
        message = JSON.parse(decoder.decode(text));
      } catch {
        // not the room's (or damaged)
        return;
      }
      const now = Date.now();
      if (!message || typeof message.from !== 'string' || !/^[0-9a-f]{16}$/.test(message.from) ||
          message.from === this.id || typeof message.nonce !== 'string' || this.seen.has(message.nonce) ||
          typeof message.time !== 'number' || Math.abs(now - message.time) > MESSAGE_AGE_MS) {
        return;
      }
      this.seen.set(message.nonce, now);
      if (message.to !== undefined && message.to !== this.id) return;
      const address = message.address >>> 0;
      if (message.type === 'hello' && (address & 255) === 10) {
        this.helloFrom(message.from, address);
      } else if ((message.type === 'offer' || message.type === 'answer') && typeof message.sdp === 'string' &&
          message.sdp.length < MAXIMUM_MESSAGE) {
        this.description(message.from, address, message.type, message.sdp);
      } else if (message.type === 'bye') {
        this.drop(message.from);
      }
    }

    // a browser that says it is there: the one of the pair with the smaller
    // identifier offers the connection
    helloFrom(id, address) {
      if (address === this.address) {
        // the same address: the later chooser takes another, while it can
        if (!this.addressFixed && id < this.id) {
          this.address = newAddress();
          this.hello();
          changed();
        }
        return;
      }
      let peer = this.peers.get(id);
      if (!peer) {
        if (this.peers.size >= MAXIMUM_PEERS || this.byAddress.has(address)) return;
        peer = this.peer(id, address);
      }
      peer.seen = Date.now();
      if (this.id < id && !peer.offered) this.offer(peer);
    }

    peer(id, address) {
      const connection = new RTCPeerConnection({ iceServers: ICE_SERVERS });
      // (both ends make the same two channels: no negotiation of them)
      const reliable = connection.createDataChannel('streams', { negotiated: true, id: 0, ordered: true });
      const datagrams = connection.createDataChannel('datagrams', { negotiated: true, id: 1, ordered: false,
        maxRetransmits: 0 });
      const peer = { id, address, connection, reliable, datagrams, seen: Date.now(), created: Date.now(),
        offered: false, open: false };
      for (const channel of [reliable, datagrams]) {
        channel.binaryType = 'arraybuffer';
        channel.onmessage = (event) => {
          if (event.data instanceof ArrayBuffer) deliver(peer, channel === reliable, new Uint8Array(event.data));
        };
        channel.onopen = () => {
          peer.open = reliable.readyState === 'open' && datagrams.readyState === 'open';
          changed();
        };
        channel.onclose = () => this.drop(id, peer);
      }
      connection.onconnectionstatechange = () => {
        if (connection.connectionState === 'failed' || connection.connectionState === 'closed') this.drop(id, peer);
      };
      this.peers.set(id, peer);
      this.byAddress.set(address, peer);
      changed();
      return peer;
    }

    async gathered(connection) {
      if (connection.iceGatheringState === 'complete') return;
      await new Promise((resolve) => {
        const timer = setTimeout(resolve, GATHER_MS);
        connection.addEventListener('icegatheringstatechange', () => {
          if (connection.iceGatheringState === 'complete') {
            clearTimeout(timer);
            resolve();
          }
        });
      });
    }

    async offer(peer) {
      peer.offered = true;
      try {
        await peer.connection.setLocalDescription(await peer.connection.createOffer());
        await this.gathered(peer.connection);
        this.send({ type: 'offer', to: peer.id, address: this.address, sdp: peer.connection.localDescription.sdp });
      } catch {
        this.drop(peer.id, peer);
      }
    }

    async description(id, address, type, sdp) {
      let peer = this.peers.get(id);
      if (type === 'offer') {
        // (an offer from a browser not heard from yet, or a new one from a
        // browser that started again: a new connection)
        if (peer && peer.connection.signalingState !== 'stable') return;
        if (peer) this.drop(id);
        if ((address & 255) !== 10 || address === this.address || this.byAddress.has(address) ||
            this.peers.size >= MAXIMUM_PEERS) {
          return;
        }
        peer = this.peer(id, address);
        try {
          await peer.connection.setRemoteDescription({ type: 'offer', sdp });
          await peer.connection.setLocalDescription(await peer.connection.createAnswer());
          await this.gathered(peer.connection);
          this.send({ type: 'answer', to: id, address: this.address, sdp: peer.connection.localDescription.sdp });
        } catch {
          this.drop(id, peer);
        }
      } else if (peer && peer.connection.signalingState === 'have-local-offer') {
        try {
          await peer.connection.setRemoteDescription({ type: 'answer', sdp });
        } catch {
          this.drop(id, peer);
        }
      }
    }

    // drops the browser of that identifier (only that connection to it: a
    // connection replaced by a new one closes after it)
    drop(id, only) {
      const peer = this.peers.get(id);
      if (!peer || (only && peer !== only)) return;
      this.peers.delete(id);
      if (this.byAddress.get(peer.address) === peer) this.byAddress.delete(peer.address);
      try {
        peer.connection.close();
      } catch {
        // already
      }
      if (game && game._web_net_peer_lost) game._web_net_peer_lost(peer.address);
      changed();
    }

    tick() {
      const now = Date.now();
      this.hello();
      for (const [id, peer] of this.peers) {
        // (not heard from in a while, or not connected in time)
        if (!peer.open && (now - peer.seen > PEER_TIMEOUT_MS || now - peer.created > CONNECT_MS)) this.drop(id);
      }
      for (const [nonce, time] of this.seen) {
        if (now - time > MESSAGE_AGE_MS * 2) this.seen.delete(nonce);
      }
    }

    // a frame from the game for a browser (EVERYONE: each one)
    frame(address, reliable, bytes) {
      const targets = address === EVERYONE ? [...this.peers.values()] : [this.byAddress.get(address)];
      for (const peer of targets) {
        if (!peer || !peer.open) continue;
        const channel = reliable ? peer.reliable : peer.datagrams;
        if (!reliable && channel.bufferedAmount > DATAGRAM_BUFFERED_LIMIT) continue;
        try {
          channel.send(bytes);
        } catch {
          // (a channel closing: its peer goes with it)
        }
      }
    }

    leave() {
      this.send({ type: 'bye' }).finally(() => {
        for (const broker of this.brokers) broker.close();
      });
      clearInterval(this.helloTimer);
      for (const id of [...this.peers.keys()]) this.drop(id);
    }
  }

  // a frame from another browser, to the game (queued until the game is
  // there)
  function deliver(peer, reliable, bytes) {
    if (bytes.length < 1 || bytes.length > MAXIMUM_FRAME) return;
    if (!game) {
      if (incoming.length < 256) incoming.push([peer.address, reliable, bytes]);
      return;
    }
    const pointer = game._malloc(bytes.length);
    if (!pointer) return;
    game.HEAPU8.set(bytes, pointer);
    game._web_net_receive(peer.address, reliable ? 1 : 0, pointer, bytes.length);
    game._free(pointer);
  }

  function changed() {
    for (const listener of listeners) listener(status());
  }

  function status() {
    if (!room) return null;
    const peers = [...room.peers.values()];
    return {
      secret: hex(room.secret),
      address: dotted(room.address),
      brokers: room.connectedBrokers(),
      peers: peers.length,
      connected: peers.filter((peer) => peer.open).length,
    };
  }

  return {
    // joins the room of a secret (32 hex digits); returns its status
    async join(secretHex, brokers) {
      if (room) room.leave();
      const secret = new Uint8Array(secretHex.match(/../g).map((pair) => parseInt(pair, 16)));
      room = new Room(secret, brokers && brokers.length ? brokers : BROKERS);
      await room.start();
      changed();
      return status();
    },
    // a new room's secret
    newSecret() {
      return hex(random(16));
    },
    leave() {
      if (room) room.leave();
      room = null;
      changed();
    },
    // a frame from the game (web_library.js's web_js_net_send)
    send(address, reliable, bytes) {
      if (room) room.frame(address, reliable, bytes);
    },
    // the game started: its address stays, and the frames that came before
    // it go to it
    attach(module) {
      game = module;
      if (room) room.addressFixed = true;
      for (const [address, reliable, bytes] of incoming.splice(0)) {
        const peer = room && room.byAddress.get(address);
        if (peer) deliver(peer, reliable, bytes);
      }
    },
    // the game's address in the room (dotted), or null
    address() {
      return room ? dotted(room.address) : null;
    },
    status,
    onChange(listener) {
      listeners.add(listener);
    },
  };
})();
