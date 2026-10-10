/*
P2P.JS

Internet play's WebRTC connections (port/web/src/web_p2p.c): the game's
tunnel to each machine of an internet game, a native build or a browser, is
a data channel of an RTCPeerConnection made here. The game asks (the
commands below) and is answered through its exports: each connection's ICE
credentials, IPv4 addresses and mDNS names (web_p2p_described), this run's
certificate's hash (web_p2p_fingerprint), and the messages that arrive
(web_p2p_receive).

Every connection makes an offer, with one data channel negotiated
beforehand (id 0, unordered, no retransmissions: the tunnel's datagrams);
the other end's answer is made up from what the game's signalling told of
it: a native build's is an ICE-lite agent and a DTLS server
(port/linux/src/p2p_webrtc.c), and of two browsers, both offering, the host
is the DTLS server (and ICE settles which end controls). Every connection
has the run's certificate, whose hash the game signals.
*/

'use strict';

window.HaloP2P = (() => {
  // (port/linux/src/port_config.c's network.stun_servers, as net.js)
  const ICE_SERVERS = [{ urls: ['stun:stun.l.google.com:19302', 'stun:stun.cloudflare.com:3478'] }];
  const COMMAND_CREATE = 1;
  const COMMAND_CONNECT = 2;
  const COMMAND_CANDIDATES = 3;
  const COMMAND_SEND = 4;
  const COMMAND_CLOSE = 5;
  // the addresses told (web_p2p.c keeps 4: P2P_MAXIMUM_CANDIDATES), and the
  // mDNS names of a browser's own (2: P2P_MAXIMUM_WEBRTC_NAMES)
  const MAXIMUM_ADDRESSES = 4;
  const MAXIMUM_NAMES = 2;
  // (a channel that holds this much unsent drops what is sent to it, as a
  // full network would)
  const BUFFERED_LIMIT = 1 << 20;

  const encoder = new TextEncoder();
  const decoder = new TextDecoder();
  let game = null;
  let certificate = null;
  let fingerprintTold = false;
  // index → { pc, dc, chain (the commands, in order), closed }
  const connections = new Map();

  function attach(module) {
    game = module;
  }

  // a copy of bytes in the game's memory, for a call that does not keep it
  function withBytes(bytes, call) {
    const pointer = game._malloc(bytes.length || 1);
    if (!pointer) return;
    try {
      game.HEAPU8.set(bytes, pointer);
      call(pointer);
    } finally {
      game._free(pointer);
    }
  }

  function withText(text, call) {
    withBytes(encoder.encode(text + '\0'), call);
  }

  function certificateMade() {
    certificate = certificate || RTCPeerConnection.generateCertificate({ name: 'ECDSA', namedCurve: 'P-256' });
    return certificate;
  }

  // this end's credentials and addresses, from its offer as gathered so far:
  // IPv4 over UDP, the public ones first; and its own addresses, which the
  // browser hides behind mDNS names (another browser on the network looks
  // them up; a native build cannot, and needs none: the browser reaches it)
  function describe(index, entry) {
    const description = entry.pc.localDescription;
    if (!description || entry.closed || !game) return;
    const sdp = description.sdp;
    const ufrag = /^a=ice-ufrag:(\S+)/m.exec(sdp);
    const password = /^a=ice-pwd:(\S+)/m.exec(sdp);
    if (!ufrag || !password) return;
    const addresses = [];
    for (const match of sdp.matchAll(/^a=candidate:\S+ \d+ udp \d+ (\d+\.\d+\.\d+\.\d+) (\d+) typ (host|srflx)/gim)) {
      addresses.push({ text: `${match[1]}:${match[2]}`, public: match[3] === 'srflx' });
    }
    addresses.sort((a, b) => b.public - a.public);
    const unique = [...new Set(addresses.map((address) => address.text))].slice(0, MAXIMUM_ADDRESSES);
    const names = [];
    for (const match of sdp.matchAll(/^a=candidate:\S+ \d+ udp \d+ ([0-9a-zA-Z-]+\.local) (\d+) typ host/gim)) {
      names.push(`${match[1]}:${match[2]}`);
    }
    if (!fingerprintTold) {
      const fingerprint = /^a=fingerprint:sha-256 ([0-9A-F:]+)/im.exec(sdp);
      if (fingerprint) {
        const bytes = new Uint8Array(fingerprint[1].split(':').map((pair) => parseInt(pair, 16)));
        if (bytes.length === 32) {
          withBytes(bytes, (pointer) => game._web_p2p_fingerprint(pointer));
          fingerprintTold = true;
        }
      }
    }
    withText([ufrag[1], password[1], ...unique, ...[...new Set(names)].slice(0, MAXIMUM_NAMES)].join(' '),
      (pointer) => game._web_p2p_described(index, pointer));
  }

  async function create(index, entry) {
    const pc = new RTCPeerConnection({ iceServers: ICE_SERVERS, certificates: [await certificateMade()] });
    const dc = pc.createDataChannel('opence', { negotiated: true, id: 0, ordered: false, maxRetransmits: 0 });
    entry.pc = pc;
    entry.dc = dc;
    if (entry.closed) {
      pc.close();
      return;
    }
    dc.binaryType = 'arraybuffer';
    dc.onmessage = (event) => {
      if (!game || entry.closed || !(event.data instanceof ArrayBuffer) || !event.data.byteLength) return;
      withBytes(new Uint8Array(event.data), (pointer) => game._web_p2p_receive(index, pointer, event.data.byteLength));
    };
    pc.onicecandidate = () => describe(index, entry);
    await pc.setLocalDescription(await pc.createOffer());
    entry.mid = /^a=mid:(\S+)/m.exec(pc.localDescription.sdp)?.[1] || '0';
    describe(index, entry);
  }

  function candidateLine(address, number) {
    const [ip, port] = address.split(':');
    // (each its own foundation and priority)
    return `candidate:${number} 1 udp ${2130706431 - number} ${ip} ${port} typ host`;
  }

  // the other end, as the game was told: "lite|full passive|active ufrag
  // password fingerprint address:port ... name.local:port ..."
  async function connect(entry, text) {
    const [kind, setup, ufrag, password, fingerprint, ...addresses] = text.split(' ');
    if (!/^[0-9a-f]{64}$/.test(fingerprint || '')) return;
    entry.addressCount = 0;
    const lines = [
      'v=0', 'o=- 1 2 IN IP4 127.0.0.1', 's=-', 't=0 0', `a=group:BUNDLE ${entry.mid}`,
      ...(kind === 'lite' ? ['a=ice-lite'] : []),
      'm=application 9 UDP/DTLS/SCTP webrtc-datachannel', 'c=IN IP4 0.0.0.0', `a=mid:${entry.mid}`,
      `a=ice-ufrag:${ufrag}`, `a=ice-pwd:${password}`,
      `a=fingerprint:sha-256 ${fingerprint.toUpperCase().match(/../g).join(':')}`,
      `a=setup:${setup === 'active' ? 'active' : 'passive'}`, 'a=sctp-port:5000', 'a=max-message-size:262144',
      ...addresses.map((address) => `a=${candidateLine(address, ++entry.addressCount)}`),
      '',
    ];
    await entry.pc.setRemoteDescription({ type: 'answer', sdp: lines.join('\r\n') });
  }

  async function addCandidates(entry, text) {
    for (const address of text.split(' ').filter(Boolean)) {
      await entry.pc.addIceCandidate({ candidate: candidateLine(address, ++entry.addressCount), sdpMid: entry.mid,
        sdpMLineIndex: 0 }).catch(() => {});
    }
  }

  function close(entry) {
    entry.closed = true;
    if (entry.pc) entry.pc.close();
  }

  // a command of the game's (web_library.js's web_js_p2p)
  function command(kind, index, bytes) {
    if (kind === COMMAND_SEND) {
      const entry = connections.get(index);
      if (entry && entry.dc && entry.dc.readyState === 'open' && entry.dc.bufferedAmount < BUFFERED_LIMIT) {
        try {
          entry.dc.send(bytes);
        } catch {
          // (closing meanwhile: lost, as a datagram may be)
        }
      }
      return;
    }
    if (kind === COMMAND_CREATE) {
      const old = connections.get(index);
      if (old) close(old);
      const entry = { closed: false };
      entry.chain = create(index, entry).catch((error) => console.warn('internet play: WebRTC:', error));
      connections.set(index, entry);
      return;
    }
    const entry = connections.get(index);
    if (!entry) return;
    const text = bytes ? decoder.decode(bytes) : '';
    if (kind === COMMAND_CLOSE) {
      connections.delete(index);
      entry.chain = entry.chain.then(() => close(entry));
      close(entry);
    } else if (kind === COMMAND_CONNECT) {
      entry.chain = entry.chain.then(() => !entry.closed && connect(entry, text))
        .catch((error) => console.warn('internet play: WebRTC:', error));
    } else if (kind === COMMAND_CANDIDATES) {
      entry.chain = entry.chain.then(() => !entry.closed && addCandidates(entry, text));
    }
  }

  return { attach, command };
})();
