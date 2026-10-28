/*
SERVERS.JS

The launcher's Servers panel: the games the official master servers list
(masters.json names them, each with its Ed25519 key; the official ones,
built in, when it names none), fetched every 30
seconds while the page shows, and joined with Play here (the page's ?join=).

- A master's list (GET /v1/servers) is signed over the exact bytes of its
  body (the header Signature: ed25519=<base64>, Signature-Key its key's id;
  docs/master_protocol.md). The page checks it with WebCrypto's Ed25519 and
  refuses a list whose signature fails, without its Signature-Key or with
  another key's, that is not protocol 1 of that key, or whose signed body's
  time (master.time) is more than 5 minutes old (a list replayed later) or
  ahead of this machine's clock. A browser without Ed25519 refuses every
  list, and says why: nothing unchecked is shown. (A master answers with
  Access-Control-Expose-Headers naming the signature's headers, without
  which the page cannot read them.)
- Each entry carries the host's own signed listing (port/linux/src/
  p2p_lobby.c), whose signature the page checks too, and whose name, map,
  gametype, players and network version are what a row shows (the entry's
  own fields are the master's word); an open game's listing holds the token
  of its invite, and the invite is the hash of the host's key with it
  (p2p.c). A game with a password keeps its token sealed: the game's own
  server browser joins those.
- Entries of several masters with one id are one game: the newest updated
  wins. Every field is checked for its type, names lose their control
  characters, and 512 entries at most are kept. Listed servers are never
  contacted: the region stands in for a ping.
*/

'use strict';

(function (root) {
  const REFRESH = 30000;
  const MAXIMUM_AGE = 300;
  const MAXIMUM_ENTRIES = 512;
  const MAXIMUM_MASTERS = 4;
  // the official master servers, asked when masters.json names none (as
  // master_list.c's master_list_read_defaults)
  const DEFAULT_MASTERS = [
    { url: 'https://master.opencommunityedition.com', key: 'ed25519:89f4eda71229f11f3538679f49ef7fb0c2b407b380093f7f4fe02105b92225aa' },
  ];
  const LISTING_LABEL = 'hceu-lobby-1';
  // a listing (p2p_lobby.c listing_make): HL, format 1, network version, flags,
  // sequence, time, the host's key, its token (sealed: 56 bytes), counts,
  // three texts, a stamp of 8, the signature
  const LISTING_IN_PROGRESS = 2;
  const LISTING_TEAMS = 4;
  const LISTING_PASSWORD = 16;
  const LISTING_CLOSED = 8;
  const LISTING_DEDICATED = 32;
  const LISTING_SIZES = [140, 268];
  const LISTING_TEXTS = [32, 32, 24];

  const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

  function hexBytes(text) {
    return new Uint8Array(text.match(/../g).map((pair) => parseInt(pair, 16)));
  }

  function base64Bytes(text) {
    const binary = atob(text);
    return Uint8Array.from(binary, (character) => character.charCodeAt(0));
  }

  // masters.json: {"masters":[{"url":"https://…","key":"ed25519:<64 hex>"}]};
  // a URL as master_list.c takes one: https, or plain http to this machine
  // alone, with a path the master is served under or none, its trailing
  // slashes dropped
  function parseMasters(json) {
    const masters = json && Array.isArray(json.masters) ? json.masters : [];
    return checkMasters(masters);
  }

  // masters.json's masters, else (it names none, or cannot be read) the
  // official ones
  function mastersOrDefault(json) {
    const masters = parseMasters(json);
    return masters.length ? masters : checkMasters(DEFAULT_MASTERS);
  }

  function checkMasters(masters) {
    return masters.filter((master) => master && typeof master.url === 'string' && typeof master.key === 'string' &&
      master.url.length < 256 &&
      /^(https:\/\/[^\s/?#]+|http:\/\/(127\.0\.0\.1|localhost)(:\d+)?)(\/[^\s?#]*)?$/.test(master.url) &&
      /^ed25519:[0-9a-f]{64}$/.test(master.key))
      .slice(0, MAXIMUM_MASTERS)
      .map((master) => ({ url: master.url.replace(/\/+$/, ''), key: hexBytes(master.key.slice(8)) }));
  }

  // an Ed25519 key for verify, or null where the browser has no Ed25519
  async function importKey(subtle, raw) {
    try {
      return await subtle.importKey('raw', raw, { name: 'Ed25519' }, false, ['verify']);
    } catch {
      return null;
    }
  }

  const NO_ED25519 = 'this browser cannot check Ed25519 signatures';
  const cleanText = (value, maximum) => (typeof value === 'string' ? value.replace(/[\u0000-\u001f\u007f-\u009f]/g, '').trim().slice(0, maximum) : '');
  const isCount = (value, maximum) => Number.isInteger(value) && value >= 0 && value <= maximum;

  // a list's body checked against its master's key: {ok, verified, reason,
  // json}; ok false refuses the whole list (Signature-Time, not signed, is
  // not read: the signed body's time is)
  async function verifyList(subtle, master, bytes, headers, now) {
    const refuse = (reason) => ({ ok: false, verified: false, reason });
    const signature = /^ed25519=([A-Za-z0-9+/]+={0,2})$/.exec((headers.get('Signature') || '').trim());
    if (!signature) return refuse('no signature');
    const keyId = hex(new Uint8Array(await subtle.digest('SHA-256', master.key))).slice(0, 16);
    const named = (headers.get('Signature-Key') || '').trim();
    if (!named) return refuse('no signature key');
    if (named !== keyId) return refuse('signed with another key');
    const key = await importKey(subtle, master.key);
    if (!key) return refuse(NO_ED25519);
    let good = false;
    try {
      good = await subtle.verify({ name: 'Ed25519' }, key, base64Bytes(signature[1]), bytes);
    } catch {
      good = false;
    }
    if (!good) return refuse('a bad signature');
    let json;
    try {
      json = JSON.parse(new TextDecoder().decode(bytes));
    } catch {
      return refuse('not JSON');
    }
    // (docs/master_protocol.md, the list: its protocol, its key, and the
    // time in the signed body, which the unsigned header only repeats)
    if (!json || json.protocol !== 1 || !json.master || !Array.isArray(json.servers)) return refuse('not a list');
    if (json.master.key_id !== keyId) return refuse('signed with another key');
    if (!Number.isInteger(json.master.time) || now - json.master.time > MAXIMUM_AGE) return refuse('an old list');
    if (json.master.time - now > MAXIMUM_AGE) return refuse('a list dated ahead of this machine\'s clock');
    return { ok: true, verified: true, json };
  }

  // an entry of a list with every field of its type, else null
  function readEntry(raw) {
    if (!raw || typeof raw !== 'object' || typeof raw.id !== 'string' || !/^[0-9A-Za-z_-]{1,64}$/.test(raw.id)) return null;
    if (!isCount(raw.players, 255) || !isCount(raw.max_players, 255) || !Number.isInteger(raw.updated)) return null;
    if (typeof raw.listing !== 'string' || !/^[A-Za-z0-9+/]+={0,2}$/.test(raw.listing)) return null;
    const publisher = raw.publisher && typeof raw.publisher === 'object' ? raw.publisher : {};
    return {
      id: raw.id, name: cleanText(raw.name, 32), map: cleanText(raw.map, 32), gametype: cleanText(raw.gametype, 24),
      variant: cleanText(raw.variant, 24), players: raw.players, max_players: raw.max_players, teams: raw.teams === true,
      in_progress: raw.in_progress === true, password: raw.password === true, dedicated: raw.dedicated === true,
      network_version: Number.isInteger(raw.network_version) ? raw.network_version : 0, build: cleanText(raw.build, 32),
      region: cleanText(raw.region, 16), reachable: raw.reachable !== false, updated: raw.updated,
      publisher: cleanText(publisher.name, 48), listing: raw.listing,
    };
  }

  // a listing's host's signing key (its bytes 14 to 46), as hex: what tells
  // one game from another across masters, as the game's own browser does;
  // null for a listing too short to hold one
  function hostKeyOf(base64) {
    try {
      const text = atob(base64);
      if (text.length < 46) return null;
      let key = '';
      for (let index = 14; index < 46; index++) key += text.charCodeAt(index).toString(16).padStart(2, '0');
      return key;
    } catch {
      return null;
    }
  }

  // every master's entries as one list: one game a host key (a master's id
  // for it is that master's, and another's may shadow it), its newest;
  // dedicated servers first, then the fullest
  function mergeServers(lists) {
    const games = new Map();
    for (const entries of lists) {
      for (const entry of entries) {
        const game = hostKeyOf(entry.listing) || 'id:' + entry.id;
        const known = games.get(game);
        if (!known || entry.updated > known.updated) games.set(game, entry);
      }
    }
    return [...games.values()].sort((a, b) => (b.dedicated - a.dedicated) || (b.players - a.players) ||
      a.name.localeCompare(b.name)).slice(0, MAXIMUM_ENTRIES);
  }

  // ---------- the host's listing, and the invite in it

  const P = (1n << 255n) - 19n;
  // (the u of Curve25519's points of small order)
  const SMALL_ORDER = [0n, 1n, P - 1n, 325606250916557431795983626356110631294008115727848805560023387167927233504n,
    39382357235489614581723060781553021112529911719440698176882885853963445705823n];

  function power(base, exponent) {
    let result = 1n;
    base %= P;
    while (exponent > 0n) {
      if (exponent & 1n) result = (result * base) % P;
      base = (base * base) % P;
      exponent >>= 1n;
    }
    return result;
  }

  // an Ed25519 public key's X25519 one (u = (1 + y) / (1 - y)), as
  // Monocypher's crypto_eddsa_to_x25519; null for one of small order
  function edwardsToX25519(key) {
    let y = 0n;
    for (let index = 31; index >= 0; index--) y = (y << 8n) | BigInt(index === 31 ? key[index] & 0x7f : key[index]);
    const denominator = ((1n - y) % P + P) % P;
    if (y >= P || denominator === 0n) return null;
    const u = ((1n + y) * power(denominator, P - 2n)) % P;
    if (SMALL_ORDER.includes(u)) return null;
    const bytes = new Uint8Array(32);
    for (let index = 0, value = u; index < 32; index++, value >>= 8n) bytes[index] = Number(value & 0xffn);
    return bytes;
  }

  // {invite (64 hex, or '' with a password), locked, verified, dedicated,
  // name, map, gametype, players, max_players, teams, in_progress,
  // network_version} of a listing's base64, or null when it is not one, its
  // signature fails, or the browser cannot check it
  async function decodeListing(subtle, base64) {
    let bytes;
    try {
      bytes = base64Bytes(base64);
    } catch {
      return null;
    }
    if (bytes.length < LISTING_SIZES[0] || bytes.length > LISTING_SIZES[1] || bytes[0] !== 0x48 || bytes[1] !== 0x4c ||
        bytes[2] !== 1) {
      return null;
    }
    const flags = bytes[5];
    if (flags & LISTING_CLOSED) return null;
    const key = bytes.slice(14, 46);
    const locked = (flags & LISTING_PASSWORD) !== 0;
    let offset = 46 + (locked ? 56 : 16);
    const token = bytes.slice(46, 62);
    const counts = bytes.slice(offset, offset + 3);
    offset += 3;
    const texts = [];
    for (const maximum of LISTING_TEXTS) {
      if (offset >= bytes.length) return null;
      const length = bytes[offset++];
      if (length > maximum || offset + length > bytes.length) return null;
      // (as the game's sanitize: a zero ends it, a byte past printable ASCII is ?)
      const text = Array.from(bytes.slice(offset, offset + length), (byte) => (byte >= 0x20 && byte <= 0x7e ? String.fromCharCode(byte) : byte ? '?' : '\0'));
      texts.push(text.join('').split('\0')[0]);
      offset += length;
    }
    offset += 8;
    if (offset + 64 !== bytes.length) return null;
    const hostKey = await importKey(subtle, key);
    if (!hostKey) return null;
    const label = new TextEncoder().encode(LISTING_LABEL);
    const signed = new Uint8Array(label.length + offset);
    signed.set(label);
    signed.set(bytes.slice(0, offset), label.length);
    let good = false;
    try {
      good = await subtle.verify({ name: 'Ed25519' }, hostKey, bytes.slice(offset), signed);
    } catch {
      good = false;
    }
    if (!good) return null;
    const x25519 = edwardsToX25519(key);
    if (!x25519) return null;
    const hash = new Uint8Array(await subtle.digest('SHA-256', x25519)).slice(0, 16);
    return { invite: locked ? '' : hex(hash) + hex(token), locked, verified: true,
      dedicated: (flags & LISTING_DEDICATED) !== 0, name: texts[0], map: texts[1], gametype: texts[2],
      players: counts[0], max_players: counts[1], teams: (flags & LISTING_TEAMS) !== 0,
      in_progress: (flags & LISTING_IN_PROGRESS) !== 0, network_version: (bytes[3] << 8) | bytes[4] };
  }

  // ---------- fetching

  async function fetchMaster(master, options) {
    const { fetch: get, subtle, now, networkVersion } = options;
    const query = Number.isInteger(networkVersion) ? `?network_version=${networkVersion}` : '';
    let response;
    try {
      response = await get(`${master.url}/v1/servers${query}`, { mode: 'cors', credentials: 'omit', cache: 'no-store' });
    } catch {
      return { master, entries: [], error: 'cannot be reached' };
    }
    if (!response.ok) return { master, entries: [], error: `HTTP ${response.status}` };
    const bytes = new Uint8Array(await response.arrayBuffer());
    const checked = await verifyList(subtle, master, bytes, response.headers, now());
    if (!checked.ok) return { master, entries: [], error: checked.reason };
    const entries = [];
    for (const raw of checked.json.servers.slice(0, MAXIMUM_ENTRIES)) {
      const entry = readEntry(raw);
      if (!entry) continue;
      const listing = await decodeListing(subtle, entry.listing);
      if (!listing) continue;
      // (what the host signed, over the master's word for it)
      entries.push({ ...entry, name: listing.name, map: listing.map, gametype: listing.gametype, players: listing.players,
        max_players: listing.max_players, teams: listing.teams, in_progress: listing.in_progress,
        network_version: listing.network_version, invite: listing.invite, locked: listing.locked || entry.password,
        listingVerified: listing.verified, dedicated: entry.dedicated || listing.dedicated });
    }
    return { master, entries, verified: checked.verified };
  }

  // what a row shows, and whether Play here can join it
  function serverRow(entry, networkVersion) {
    const players = `${entry.players}/${entry.max_players} players`;
    const game = [entry.map, entry.variant || entry.gametype].filter(Boolean).join(', ');
    const detail = [game, players, entry.region, entry.dedicated ? 'dedicated' : 'player-hosted',
      entry.publisher && `by ${entry.publisher}`].filter(Boolean).join(' · ');
    let reason = '';
    if (Number.isInteger(networkVersion) && entry.network_version !== networkVersion) {
      reason = `Needs network version ${entry.network_version} (this build is ${networkVersion})`;
    } else if (entry.locked || !entry.invite) {
      reason = 'A password: join it from the game\'s server browser';
    } else if (entry.players >= entry.max_players) {
      reason = 'Full';
    }
    return { id: entry.id, name: entry.name || 'A game', detail, invite: entry.invite, playable: !reason, reason,
      dedicated: entry.dedicated };
  }

  // ---------- the panel

  function start(options) {
    const doc = root.document;
    const $ = (id) => doc.getElementById(id);
    let masters = [];
    let timer = null;
    let busy = false;

    function render(results) {
      const lists = results.map((result) => result.entries);
      const rows = mergeServers(lists).map((entry) => serverRow(entry, options.networkVersion()));
      $('servers-list').replaceChildren(...rows.map((row) => {
        const item = doc.createElement('li');
        const label = doc.createElement('span');
        const name = doc.createElement('span');
        const detail = doc.createElement('span');
        const play = doc.createElement('button');
        item.className = row.dedicated ? 'dedicated' : '';
        name.className = 'name';
        name.textContent = row.name;
        detail.className = 'detail';
        detail.textContent = row.detail;
        label.append(name, detail);
        play.textContent = 'Play here';
        play.disabled = !row.playable;
        play.title = row.reason || 'Start the game and join this server';
        play.addEventListener('click', () => options.onPlay(row.invite));
        item.append(label, play);
        return item;
      }));
      const failed = results.filter((result) => result.error && result.error !== NO_ED25519)
        .map((result) => `${new URL(result.master.url).host}: ${result.error}`);
      const unchecked = results.some((result) => result.error === NO_ED25519);
      $('servers-status').textContent = (rows.length ? `${rows.length} game${rows.length === 1 ? '' : 's'} listed.` :
        'No games are listed now.') + (failed.length ? ` Not heard from ${failed.join('; ')}.` : '') +
        (unchecked ? ' This browser cannot check the lists\' Ed25519 signatures, so it shows none: the game\'s own ' +
          'Join Game lists them.' : '');
    }

    async function refresh() {
      if (busy || !masters.length || doc.visibilityState !== 'visible' || !options.visible()) return;
      busy = true;
      try {
        const now = () => Math.floor(Date.now() / 1000);
        render(await Promise.all(masters.map((master) => fetchMaster(master, {
          fetch: root.fetch.bind(root), subtle: root.crypto.subtle, now, networkVersion: options.networkVersion() }))));
      } finally {
        busy = false;
      }
    }

    (async () => {
      try {
        masters = mastersOrDefault(await (await root.fetch('masters.json', { cache: 'no-cache' })).json());
      } catch {
        masters = mastersOrDefault(null);
      }
      if (!masters.length) {
        $('servers-status').textContent = 'No master servers are listed yet: games hosted for the internet are found ' +
          'in the game\'s own Join Game, and invites still join them.';
        $('servers-refresh').hidden = true;
        return;
      }
      $('servers-refresh').addEventListener('click', refresh);
      doc.addEventListener('visibilitychange', refresh);
      timer = setInterval(refresh, REFRESH);
      refresh();
    })();
    return { refresh, stop: () => clearInterval(timer) };
  }

  const api = { parseMasters, mastersOrDefault, verifyList, readEntry, mergeServers, edwardsToX25519, decodeListing, fetchMaster, serverRow,
    start, MAXIMUM_AGE };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.HaloServers = api;
})(typeof window !== 'undefined' ? window : globalThis);
