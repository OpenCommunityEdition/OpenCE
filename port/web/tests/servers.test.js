// The launcher's Servers panel (port/web/site/servers.js) on lists signed
// here with Ed25519 keys made for the test: what it verifies and refuses,
// how it merges masters' entries, and the invite it reads from a host's
// listing. node --test "port/web/tests/*.test.js"

'use strict';

const test = require('node:test');
const assert = require('node:assert');
const crypto = require('node:crypto');
const servers = require('../site/servers.js');

const subtle = globalThis.crypto.subtle;
const NOW = 1760900000;

function keyPair() {
  const { publicKey, privateKey } = crypto.generateKeyPairSync('ed25519');
  const jwk = privateKey.export({ format: 'jwk' });
  return { privateKey, raw: Buffer.from(jwk.x, 'base64url'), seed: Buffer.from(jwk.d, 'base64url'), publicKey };
}

const master = keyPair();
const masterEntry = { url: 'https://master1.example.org', key: master.raw };
const keyId = crypto.createHash('sha256').update(master.raw).digest('hex').slice(0, 16);

// a host's listing as p2p_lobby.c's listing_make writes it, signed
function listing(host, { flags = 1, token = crypto.randomBytes(16), name = 'Fast CTF', map = 'bloodgulch', gametype = 'ctf',
  players = 3, maximum = 16, version = 38 } = {}) {
  const text = (value) => Buffer.concat([Buffer.from([Buffer.byteLength(value)]), Buffer.from(value)]);
  const head = Buffer.alloc(14);
  head.write('HL', 0, 'latin1');
  head[2] = 1;
  head.writeUInt16BE(version, 3);
  head[5] = flags;
  head.writeUInt32BE(7, 6);
  head.writeUInt32BE(NOW, 10);
  const secret = flags & 16 ? crypto.randomBytes(56) : token;
  const signed = Buffer.concat([head, host.raw, secret, Buffer.from([players, maximum, 1]), text(name), text(map), text(gametype),
    Buffer.alloc(8)]);
  const signature = crypto.sign(null, Buffer.concat([Buffer.from('hceu-lobby-1'), signed]), host.privateKey);
  return { base64: Buffer.concat([signed, signature]).toString('base64'), token };
}

// the invite p2p.c makes: the hash of the host's X25519 key, and the token
function expectedInvite(host, token) {
  const scalar = crypto.createHash('sha512').update(host.seed).digest().subarray(0, 32);
  const der = Buffer.concat([Buffer.from('302e020100300506032b656e04220420', 'hex'), scalar]);
  const x25519 = crypto.createPublicKey(crypto.createPrivateKey({ key: der, format: 'der', type: 'pkcs8' }))
    .export({ format: 'jwk' }).x;
  return crypto.createHash('sha256').update(Buffer.from(x25519, 'base64url')).digest().subarray(0, 16).toString('hex') +
    token.toString('hex');
}

function entry(id, fields = {}) {
  const host = keyPair();
  const made = listing(host);
  return { id, publisher: { id: 'p_1', name: "Milenko's servers", since: '2026-10' }, name: 'OpenCE Dedicated',
    map: 'bloodgulch', gametype: 'ctf', variant: 'CTF', players: 5, max_players: 16, teams: true, in_progress: false,
    password: false, dedicated: true, network_version: 38, build: 'build-206', region: 'eu', reachable: true,
    updated: NOW - 10, listing: made.base64, ...fields };
}

// a master's answer: the body, signed
function signedList(list, { time = NOW, key = master, keyHeader = keyId, body } = {}) {
  const bytes = Buffer.from(body || JSON.stringify({ protocol: 1, master: { name: 'Master 1', key_id: keyId, time }, servers: list }));
  const headers = new Headers({ 'Signature': 'ed25519=' + crypto.sign(null, bytes, key.privateKey).toString('base64'),
    'Signature-Key': keyHeader, 'Signature-Time': String(time), 'Content-Type': 'application/json' });
  return { bytes: new Uint8Array(bytes), headers };
}

test("masters.json: https masters (with a path or none) with an Ed25519 key, four at most", () => {
  const key = 'ed25519:' + 'ab'.repeat(32);
  const parsed = servers.parseMasters({ masters: [
    { url: 'https://master1.example.org/', key }, { url: 'http://master2.example.org', key },
    { url: 'https://master3.example.org/opence//', key }, { url: 'https://master4.example.org', key: 'ed25519:abc' },
    { url: 'http://127.0.0.1.example.net', key }, { url: 'https://q.example/?a', key },
    { url: 'http://127.0.0.1:8090', key }, { url: 'https://a.example', key }, { url: 'https://b.example', key },
  ] });
  assert.deepStrictEqual(parsed.map((item) => item.url), ['https://master1.example.org',
    'https://master3.example.org/opence', 'http://127.0.0.1:8090', 'https://a.example']);
  assert.deepStrictEqual(servers.parseMasters({ masters: [{ url: 'http://localhost/m/', key }] }).map((item) => item.url),
    ['http://localhost/m']);
  assert.strictEqual(parsed[0].key.length, 32);
  assert.deepStrictEqual(servers.parseMasters({ masters: [] }), []);
  assert.deepStrictEqual(servers.parseMasters(null), []);
  assert.deepStrictEqual(require('../site/masters.json'), { masters: [] });
});

test("the official master is asked when masters.json names none", () => {
  const official = ['https://master.opencommunityedition.com'];
  const key = 'ed25519:' + 'ab'.repeat(32);
  assert.deepStrictEqual(servers.mastersOrDefault({ masters: [] }).map((item) => item.url), official);
  assert.deepStrictEqual(servers.mastersOrDefault(null).map((item) => item.url), official);
  assert.deepStrictEqual(servers.mastersOrDefault({ masters: [{ url: 'http://master2.example.org', key }] })
    .map((item) => item.url), official);
  assert.deepStrictEqual(Buffer.from(servers.mastersOrDefault(null)[0].key).toString('hex'),
    '89f4eda71229f11f3538679f49ef7fb0c2b407b380093f7f4fe02105b92225aa');
  assert.deepStrictEqual(servers.mastersOrDefault({ masters: [{ url: 'https://a.example', key }] })
    .map((item) => item.url), ['https://a.example']);
});

test('a signed list is read; a changed, stale or misnamed one is refused', async () => {
  const good = signedList([entry('8c1e0001')]);
  const checked = await servers.verifyList(subtle, masterEntry, good.bytes, good.headers, NOW + 30);
  assert.ok(checked.ok && checked.verified);
  assert.strictEqual(checked.json.servers.length, 1);

  const changed = signedList([entry('8c1e0001')]);
  changed.bytes[changed.bytes.length - 3] ^= 1;
  assert.strictEqual((await servers.verifyList(subtle, masterEntry, changed.bytes, changed.headers, NOW)).reason, 'a bad signature');

  const other = signedList([entry('8c1e0001')], { key: keyPair() });
  assert.strictEqual((await servers.verifyList(subtle, masterEntry, other.bytes, other.headers, NOW)).reason, 'a bad signature');

  const stale = signedList([], { time: NOW - servers.MAXIMUM_AGE - 1 });
  assert.strictEqual((await servers.verifyList(subtle, masterEntry, stale.bytes, stale.headers, NOW)).reason, 'an old list');
  // (an old body under a new Signature-Time: the header is not signed, the body's time is)
  const replayed = signedList([], { time: NOW - 3600 });
  replayed.headers.set('Signature-Time', String(NOW));
  assert.strictEqual((await servers.verifyList(subtle, masterEntry, replayed.bytes, replayed.headers, NOW)).reason, 'an old list');

  const misnamed = signedList([], { keyHeader: '0000000000000000' });
  assert.strictEqual((await servers.verifyList(subtle, masterEntry, misnamed.bytes, misnamed.headers, NOW)).reason,
    'signed with another key');
  const wrongProtocol = signedList([], { body: JSON.stringify({ protocol: 2, master: { key_id: keyId, time: NOW }, servers: [] }) });
  assert.strictEqual((await servers.verifyList(subtle, masterEntry, wrongProtocol.bytes, wrongProtocol.headers, NOW)).reason,
    'not a list');
  const otherId = signedList([], { body: JSON.stringify({ protocol: 1, master: { key_id: '1'.repeat(16), time: NOW }, servers: [] }) });
  assert.strictEqual((await servers.verifyList(subtle, masterEntry, otherId.bytes, otherId.headers, NOW)).reason,
    'signed with another key');
  const unsigned = signedList([]);
  unsigned.headers.delete('Signature');
  assert.strictEqual((await servers.verifyList(subtle, masterEntry, unsigned.bytes, unsigned.headers, NOW)).reason, 'no signature');
  // (a list without its Signature-Key is refused, as the game refuses it)
  const unnamed = signedList([]);
  unnamed.headers.delete('Signature-Key');
  assert.strictEqual((await servers.verifyList(subtle, masterEntry, unnamed.bytes, unnamed.headers, NOW)).reason,
    'no signature key');
  // (the unsigned Signature-Time is not needed, nor read)
  const untimed = signedList([]);
  untimed.headers.delete('Signature-Time');
  assert.ok((await servers.verifyList(subtle, masterEntry, untimed.bytes, untimed.headers, NOW)).ok);
  untimed.headers.set('Signature-Time', 'nonsense');
  assert.ok((await servers.verifyList(subtle, masterEntry, untimed.bytes, untimed.headers, NOW)).ok);
  // (dated ahead of this machine's clock: 300 seconds taken, 301 not)
  const ahead = signedList([], { time: NOW + servers.MAXIMUM_AGE });
  assert.ok((await servers.verifyList(subtle, masterEntry, ahead.bytes, ahead.headers, NOW)).ok);
  const further = signedList([], { time: NOW + servers.MAXIMUM_AGE + 1 });
  assert.match((await servers.verifyList(subtle, masterEntry, further.bytes, further.headers, NOW)).reason, /ahead/);
});

test('a browser without Ed25519 refuses the lists: nothing unchecked is shown', async () => {
  const withoutEd25519 = {
    importKey: async () => { throw new DOMException('Unrecognized name', 'NotSupportedError'); },
    verify: async () => { throw new Error('not called'); },
    digest: (name, data) => subtle.digest(name, data),
  };
  const list = signedList([entry('8c1e0001')]);
  const checked = await servers.verifyList(withoutEd25519, masterEntry, list.bytes, list.headers, NOW);
  assert.deepStrictEqual([checked.ok, checked.verified], [false, false]);
  assert.match(checked.reason, /cannot check Ed25519/);
  const fetched = await servers.fetchMaster(masterEntry, { fetch: async () => new Response(list.bytes, { headers: list.headers }),
    subtle: withoutEd25519, now: () => NOW, networkVersion: 38 });
  assert.deepStrictEqual(fetched.entries, []);
  assert.match(fetched.error, /cannot check Ed25519/);
  assert.strictEqual(await servers.decodeListing(withoutEd25519, listing(keyPair()).base64), null);
});

test("a host's listing gives the invite p2p.c makes; a password's keeps it sealed", async () => {
  const host = keyPair();
  const open = listing(host);
  const decoded = await servers.decodeListing(subtle, open.base64);
  assert.strictEqual(decoded.invite, expectedInvite(host, open.token));
  assert.deepStrictEqual([decoded.locked, decoded.verified, decoded.dedicated, decoded.name, decoded.map],
    [false, true, false, 'Fast CTF', 'bloodgulch']);
  assert.deepStrictEqual([decoded.players, decoded.max_players, decoded.network_version, decoded.teams, decoded.in_progress],
    [3, 16, 38, false, false]);
  const counted = await servers.decodeListing(subtle, listing(host, { flags: 1 | 2 | 4, players: 7, maximum: 12, version: 39 }).base64);
  assert.deepStrictEqual([counted.players, counted.max_players, counted.network_version, counted.teams, counted.in_progress],
    [7, 12, 39, true, true]);
  assert.strictEqual((await servers.decodeListing(subtle, listing(host, { flags: 1 | 32 }).base64)).dedicated, true);
  // (names as the game's sanitize reads them)
  assert.strictEqual((await servers.decodeListing(subtle, listing(host, { name: 'Caf\u00e9\u0000hidden' }).base64)).name, 'Caf??');
  assert.strictEqual(await servers.decodeListing(subtle, listing(host, { name: 'x'.repeat(33) }).base64), null);
  const locked = await servers.decodeListing(subtle, listing(host, { flags: 1 | 16 }).base64);
  assert.deepStrictEqual([locked.invite, locked.locked], ['', true]);
  // (the longest listing: a password's, every name at its most, 268 bytes)
  const longest = listing(host, { flags: 1 | 16, name: 'n'.repeat(32), map: 'm'.repeat(32), gametype: 'g'.repeat(24) });
  assert.strictEqual(Buffer.from(longest.base64, 'base64').length, 268);
  assert.strictEqual((await servers.decodeListing(subtle, longest.base64)).name, 'n'.repeat(32));
  const forged = Buffer.from(open.base64, 'base64');
  forged[20] ^= 1;
  assert.strictEqual(await servers.decodeListing(subtle, forged.toString('base64')), null);
  assert.strictEqual(await servers.decodeListing(subtle, listing(host, { flags: 8 }).base64), null);
  assert.strictEqual(await servers.decodeListing(subtle, Buffer.from('HL').toString('base64')), null);
  assert.strictEqual(await servers.decodeListing(subtle, '%%%'), null);
  // (a key of small order has no X25519 key a host could hold)
  assert.strictEqual(servers.edwardsToX25519(new Uint8Array(32).fill(0).map((_, index) => (index === 0 ? 1 : 0))), null);
});

test("several masters' entries: one game a host key, the newest; dedicated first, then the fullest", () => {
  const read = (fields) => servers.readEntry(entry(fields.id, fields));
  const same = listing(keyPair()).base64;
  const merged = servers.mergeServers([
    [read({ id: 'a1', name: 'Old name', updated: NOW - 50, listing: same }), read({ id: 'b2', dedicated: false, players: 9 })],
    [read({ id: 'a1', name: 'New name', updated: NOW - 5, listing: same }), read({ id: 'c3', players: 2 }),
      read({ id: 'd4', players: 12 })],
  ]);
  assert.deepStrictEqual(merged.map((item) => item.id), ['d4', 'a1', 'c3', 'b2']);
  assert.strictEqual(merged[1].name, 'New name');
  // (one game under two masters' ids is one; one id of two hosts' games, two)
  const shadowed = servers.mergeServers([
    [read({ id: 'x1', updated: NOW - 9, listing: same })], [read({ id: 'y2', updated: NOW - 3, listing: same })],
  ]);
  assert.deepStrictEqual(shadowed.map((item) => item.id), ['y2']);
  const apart = servers.mergeServers([[read({ id: 'z9' })], [read({ id: 'z9' })]]);
  assert.strictEqual(apart.length, 2);
});

test('every field is checked, and names lose their control characters', () => {
  assert.strictEqual(servers.readEntry(entry('x', { players: '5' })), null);
  assert.strictEqual(servers.readEntry(entry('x', { players: 300 })), null);
  assert.strictEqual(servers.readEntry(entry('bad id!')), null);
  assert.strictEqual(servers.readEntry(entry('x', { listing: 'not base64!' })), null);
  assert.strictEqual(servers.readEntry(null), null);
  const odd = servers.readEntry(entry('x', { name: 'Evil\u0000\u001b[31m name' + 'x'.repeat(80), dedicated: 'yes', region: 7 }));
  assert.strictEqual(odd.name, 'Evil[31m name' + 'x'.repeat(19));
  assert.strictEqual(odd.dedicated, false);
  assert.strictEqual(odd.region, '');
});

test('a row: what it shows, and when Play here cannot join', () => {
  const base = { ...servers.readEntry(entry('a1')), invite: 'ab'.repeat(32), locked: false };
  const row = servers.serverRow(base, 38);
  assert.ok(row.playable);
  assert.strictEqual(row.detail, "bloodgulch, CTF · 5/16 players · eu · dedicated · by Milenko's servers");
  assert.match(servers.serverRow({ ...base, network_version: 39 }, 38).reason, /network version 39/);
  assert.match(servers.serverRow({ ...base, locked: true, invite: '' }, 38).reason, /password/);
  assert.strictEqual(servers.serverRow({ ...base, players: 16 }, 38).reason, 'Full');
  assert.strictEqual(servers.serverRow({ ...base, dedicated: false, publisher: '', region: '' }, 38).detail,
    'bloodgulch, CTF · 5/16 players · player-hosted');
});

test("a master fetched: its list checked, its listings read, a broken one left out", async () => {
  const host = keyPair();
  const made = listing(host);
  const list = signedList([entry('a1', { listing: made.base64 }), entry('b2', { listing: 'SEw=' }), { id: 'c3' }]);
  const asked = [];
  const fetched = await servers.fetchMaster(masterEntry, {
    fetch: async (url, init) => {
      asked.push([url, init.mode, init.credentials]);
      return new Response(list.bytes, { headers: list.headers });
    },
    subtle, now: () => NOW, networkVersion: 38,
  });
  assert.deepStrictEqual(asked, [['https://master1.example.org/v1/servers?network_version=38', 'cors', 'omit']]);
  assert.strictEqual(fetched.verified, true);
  assert.deepStrictEqual(fetched.entries.map((item) => item.id), ['a1']);
  assert.strictEqual(fetched.entries[0].invite, expectedInvite(host, made.token));
  // (what the host signed is shown, not the master's word for it)
  const signedHost = keyPair();
  const signedListing = listing(signedHost, { name: 'Signed name', map: 'hangemhigh', gametype: 'Slayer', players: 2,
    maximum: 8, version: 39 });
  const claimed = signedList([entry('e5', { listing: signedListing.base64, name: 'Claimed name', map: 'bloodgulch',
    players: 15, max_players: 16, network_version: 38 })]);
  const shown = await servers.fetchMaster(masterEntry, { fetch: async () => new Response(claimed.bytes, { headers: claimed.headers }),
    subtle, now: () => NOW, networkVersion: 39 });
  assert.deepStrictEqual(['name', 'map', 'gametype', 'players', 'max_players', 'network_version'].map((field) => shown.entries[0][field]),
    ['Signed name', 'hangemhigh', 'Slayer', 2, 8, 39]);
  assert.strictEqual(servers.serverRow(shown.entries[0], 39).playable, true);
  const down = await servers.fetchMaster(masterEntry, { fetch: async () => { throw new TypeError('offline'); },
    subtle, now: () => NOW });
  assert.deepStrictEqual([down.entries, down.error], [[], 'cannot be reached']);
  const refused = await servers.fetchMaster(masterEntry, { fetch: async () => new Response('no', { status: 503 }), subtle, now: () => NOW });
  assert.strictEqual(refused.error, 'HTTP 503');
  const stale = await servers.fetchMaster(masterEntry, { fetch: async () => new Response(list.bytes, { headers: list.headers }),
    subtle, now: () => NOW + 3600 });
  assert.deepStrictEqual([stale.entries, stale.error], [[], 'an old list']);
});

test("docs/master_protocol.md's listing vector: verified, dedicated, its token in the invite", async () => {
  const vector = '484c01' + '0026' + '25' + '0000019c' + '68f533a0' +
    'fc51cd8e6218a1a38da47ed00230f0580816ed13ba3303ac5deb911548908025' + 'a0a1a2a3a4a5a6a7a8a9aaabacadaeaf' + '051001' +
    '10' + '4f70656e434520446564696361746564' + '0a' + '626c6f6f6467756c6368' + '03' + '435446' + '0000000000000000' +
    '633dc63eb27828d6e589e34dc4336dbb2d514b2018e4bf4a3adf5aeb774b6a523fe662c3d29241c6f91c8714a8ca2b5e493aacb32bcaeb9bce715f15b76d230f';
  const bytes = Buffer.from(vector, 'hex');
  assert.strictEqual(bytes.length, 169);
  const decoded = await servers.decodeListing(subtle, bytes.toString('base64'));
  assert.deepStrictEqual([decoded.verified, decoded.dedicated, decoded.locked, decoded.name, decoded.map, decoded.gametype],
    [true, true, false, 'OpenCE Dedicated', 'bloodgulch', 'CTF']);
  assert.match(decoded.invite, /^[0-9a-f]{32}a0a1a2a3a4a5a6a7a8a9aaabacadaeaf$/);
  bytes[100] ^= 1;
  assert.strictEqual(await servers.decodeListing(subtle, bytes.toString('base64')), null);
});
