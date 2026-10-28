# The master servers' protocol, version 1

A master server lists dedicated servers and vouches for who publishes them.
An operator applies to a master once with the public half of an Ed25519
key, the *publisher key*, through the master's Discord bot; once an
approver approves the application, the
operator's servers publish to the master with requests signed by that key,
and the master lists them after probing their UDP port. The game's server
browser fetches each master's list, checks the master's signature with the
key pinned in `masters.txt`, and joins a listed game by the invite in its
listing, as it joins a game heard on the brokers.

A master carries no game traffic: each list entry holds the host-signed
listing the game publishes to the signalling brokers (`p2p_lobby.c`), so
the join path is unchanged and nobody needs the master to be honest about a
game's details.

This document is the contract between the game (the publisher in
`port/linux/src/server_master.c`, the probe's answer, the list client in
`port/linux/src/master_list.c`) and the masters' implementations. The
project's implementation is `opence-master`
(github.com/OpenCommunityEdition/opence-master-server, Go, AGPL-3.0-or-later),
whose `testdata/vectors.json` holds fixed keys and every value this
document derives from them, and whose `tests/conformance` is the test a
master must pass to be an official one. `docs/dedicated_server.md` is the
server's side: its settings, `--keygen` and `--sign`.

## Conventions

| Thing | Form |
| --- | --- |
| Hexadecimal | Lowercase, two digits a byte, no prefix |
| Base64 | The standard alphabet (RFC 4648 section 4) with `=` padding |
| Times | Unix seconds, decimal, UTC |
| JSON | UTF-8; every string the protocol defines is printable ASCII (0x20 to 0x7E) |
| Signatures | Ed25519 as RFC 8032 defines it (with SHA-512): Monocypher's `crypto_ed25519_sign` and `crypto_ed25519_check`, Go's `crypto/ed25519` |
| Transport | HTTPS. A master refuses publishes, key changes and peers' requests over plain HTTP except from loopback |
| Unknown JSON fields | Ignored by both sides, so later versions can add fields |

## Keys and identifiers

| Thing | Definition |
| --- | --- |
| Key file (`publisher.key`, `master.key`) | One line of 64 hexadecimal digits: the Ed25519 seed (32 bytes). Anything after `#` is a comment; blank lines are skipped. Mode 0600 |
| Public key, as text | `ed25519:` and the 32-byte public key in hexadecimal (64 digits) |
| Key id | The first 8 bytes of SHA-256 of the 32-byte public key. Written as 16 hexadecimal digits (`keyid=`, `key_id`, `Signature-Key`); in the probe's packets it is the 8 raw bytes |
| Instance | A name the operator gives each server process (`server.instance`): 1 to 32 characters of `a-z`, `0-9`, `-` and `_`, the first a letter or a digit |
| Server id | The first 16 hexadecimal digits (8 bytes) of SHA-256 of the ASCII text of the key id's 16 hexadecimal digits followed by the instance's text, with nothing between them |

The server id hashes the key id **as its 16 characters of text**, not its 8
bytes. With the vectors' publisher key (RFC 8032's first test seed):

```
seed        9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60
public key  ed25519:d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a
key id      21fe31dfa154a261            (SHA-256 of the 32 key bytes: 21fe31dfa154a261...)
instance    main
hashed      "21fe31dfa154a261main"      (20 ASCII bytes)
server id   4c5f15237c866f6f            (SHA-256 of those 20 bytes: 4c5f15237c866f6f...)
```

The same instance under another key is another server id, so a server
keeps its id across restarts as long as it keeps its key and its instance.
A master refuses a public key whose point has a small order (its X25519
form multiplied by a clamped scalar is zero, as `p2p_ed25519_verify`
checks), since anyone can make signatures that verify under one.

`masters.txt` (the one beside `config.toml`, else the one beside the
executable) lists the masters, one per line:
`https://master1.example.org ed25519:<64 hex>`; `#` starts a comment; the
first 4 are used. A file that names none (missing, empty, or comments alone,
as the release's) leaves the official master, built into the game
(`master_list_read_defaults` in `port/linux/src/master_list.c`) and the web
page (`servers.js`):
`https://master.opencommunityedition.com ed25519:89f4eda71229f11f3538679f49ef7fb0c2b407b380093f7f4fe02105b92225aa`.
The web build reads its site's `masters.json` before the official master. The URL is an origin with no path, or a path
the master is served under as it is (a proxy must not rewrite paths, since
the path is signed).

## Signed requests

A publisher's requests carry a detached signature in one header:

```
Authorization: OpenCE-Ed25519 keyid=<16 hex>, ts=<unix seconds>, nonce=<32 hex>, sig=<base64 of 64 bytes>
```

The signature is the key's over the bytes of this string, its parts joined
by single line feeds (0x0A) and no line feed at the end:

```
OpenCE-Master-v1
<the receiving master's key id>
<METHOD>
<PATH>
<ts>
<nonce>
<SHA-256 of the body, in hexadecimal>
```

| Part | Rule |
| --- | --- |
| Master key id | The key id of the master the request is sent to: the first 8 bytes of SHA-256 of the master's list-signing public key (the key pinned for it in `masters.txt`), as 16 lowercase hexadecimal digits. It binds the request to that master: a request signed for one master does not verify at another, even within the replay window. A master checks the signature with its own key id |
| `METHOD` | The request's method in capitals: `PUT`, `DELETE`, `GET`, `POST` |
| `PATH` | The path exactly as the request line sends it, without the query string: `/v1/servers/4c5f15237c866f6f`, `/v1/me`, `/v1/publishers` |
| `ts` | The same decimal text as in the header: no sign, no leading zeros |
| `nonce` | 16 random bytes in hexadecimal (32 digits), new for each request |
| Body hash | Of the exact body bytes sent; an empty body's is SHA-256 of no bytes, `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |

The four parameters come once each, in any order, separated by commas with
optional spaces; a publisher sends them in the order above. The master
checks, in this order, and answers the first failure:

| Check | Failure |
| --- | --- |
| The header parses (scheme, the four parameters and their forms) | `401 bad_signature` |
| `ts` is within 300 seconds of the master's clock | `401 clock` (the body's `master_time` is the master's clock, for the server to log) |
| The key id is a publisher's, or an application's | `403 unknown_key` |
| The signature verifies, for this master's key id | `401 bad_signature` |
| The nonce was not seen with this key in the last 600 seconds | `401 replay` |

Since `unknown_key` is answered before the signature is checked, anyone can
learn whether a key id is known to a master (a publisher's or an
application's) by sending it with any signature. This is a known property
of version 1: key ids are not secret (each is a hash of a public key, which
an operator hands out when applying), and the answer says nothing more of
the key than that the master has seen it.

The vectors' `GET /v1/me` to the vectors' master (key id
`39f713d0a644253f`) at 1760900001 with the nonce
`f0e0d0c0b0a090807060504030201000` signs

```
OpenCE-Master-v1\n39f713d0a644253f\nGET\n/v1/me\n1760900001\nf0e0d0c0b0a090807060504030201000\ne3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
```

and sends

```
Authorization: OpenCE-Ed25519 keyid=21fe31dfa154a261, ts=1760900001, nonce=f0e0d0c0b0a090807060504030201000, sig=duRZ+WKAI8hbvidtC63U1XHxOSoeRN5mcrZvw4qihhlilhZjbVBCLYjsQLr4+F2C1gy6lN9/ixqevFX7dfNGDA==
```

## Publishing

`PUT /v1/servers/{server_id}`, signed, with a JSON body of 16 KB at most.
A server publishes to every master in `masters.txt` every 30 seconds, and
within 5 seconds of a change (the lobby opened, a game started or ended, a
player joined or left, the name or map changed). On shutdown it publishes
`closed: true` (or `DELETE`s) and waits up to 2 seconds.

```json
{"protocol":1,"instance":"main","name":"OpenCE Dedicated","map":"bloodgulch","gametype":"CTF","variant":"CTF",
 "players":5,"max_players":16,"teams":true,"in_progress":false,"password":false,"dedicated":true,
 "network_version":38,"build":"build-206","region":"eu","tunnel_port":2302,
 "listing":"<base64 of the host-signed listing>","closed":false,"serial":412}
```

(The vectors' publish, made at network version 38: "The listing" says
why the examples keep it.) The game's publisher sends `protocol`,
`instance`, `listing` (but in a tombstone), `tunnel_port`, `dedicated`,
`build`, `region` and `variant` when they are set, `address` with
`server.master_publish_address`, `serial` and `closed`; the listing holds
the rest. A server that goes private, or off the internet, sends one more
publish, `closed: true`, to each master that listed it.

| Field | Type | Required | Meaning |
| --- | --- | --- | --- |
| `protocol` | number | yes | 1 |
| `instance` | string | yes | The instance; the path's server id must be the server id of the signing key's id and this instance, else `400 server_id` |
| `listing` | string | yes, but with `closed: true` | Base64 of the exact bytes `listing_make` makes, the bytes the game publishes to the brokers |
| `tunnel_port` | number | yes, unless `address` is given | The UDP port the game's tunnel socket is bound to (`network.tunnel_port`), 1 to 65535 |
| `address` | string | no | `"203.0.113.5:2302"`: an IP address (IPv4, or IPv6 in brackets) and a port to probe instead of the request's source address. Sent only with `server.master_publish_address`; a master serves it in its list only if its own policy allows (`MASTER_SERVE_ADDRESSES`). It must be globally routable (below); a private or loopback address is taken only from a private or loopback source |
| `dedicated` | boolean | no | A dedicated server. Also read from the listing's flag 32, which a dedicated server sets; either makes the entry dedicated. A player's public game, published with the player's publisher key, sends `false` |
| `variant` | string | no | The gametype's name as the server shows it; printable ASCII, cut to 24 |
| `build` | string | no | The game's build (`build-206`); cut to 32 |
| `region` | string | no | Up to 16 of `a-z`, `0-9` and `-` (`server.region`) |
| `serial` | number | no | A whole number the server raises at each change; kept and shown to admins |
| `closed` | boolean | no | `true`: unlist the server now |
| `name`, `map`, `gametype`, `players`, `max_players`, `teams`, `in_progress`, `password`, `network_version` | | no | Informational only: the listing is authoritative. A value that differs from the listing's is ignored, and the answer's `warnings` says so |

The master parses the listing and checks the host's signature (below), and
takes from it the name, the map, the gametype, the players and most
players, the gametype's engine, the game's kind, the open, in-progress,
teams and password flags, and the network version, so a publish cannot
claim more than the host signed. A listing that does not parse or verify
is `400 listing`. So is one whose own time is more than 3600 seconds from
the master's clock. A listing with the same host key and a lower sequence
than the one the master holds is not taken (the newer stays; a warning
says so). A listing with the closed flag unlists the server, as
`closed: true` does.

The master records the request's source address (behind a reverse proxy,
the address its trusted proxy forwards) to probe the server. It keeps the
addresses 7 days at most after a server's last publish, for probes and
abuse handling, and never serves them.

The addresses a master probes and serves are an allowlist of globally
routable unicast addresses: an IPv4 address outside IANA's special-purpose
ranges (`0/8`, `10/8`, `100.64/10`, `127/8`, `169.254/16`, `172.16/12`,
`192.0.0/24`, `192.0.2/24`, `192.88.99/24`, `192.168/16`, `198.18/15`,
`198.51.100/24`, `203.0.113/24`, `224/4`, `240/4`); an IPv6 address in
`2000::/3` outside `2001::/23`, `2001:db8::/32`, `3fff::/20` and
`5f00::/16`; a 6to4 (`2002::/16`) or well-known NAT64 (`64:ff9b::/96`)
address whose embedded IPv4 address is global. An IPv4-mapped address is
judged as its IPv4 address. Everything else (link-local, site-local
`fec0::/10`, unique local, multicast, `64:ff9b:1::/48`, zones) is refused
(`400 schema`, `field: address`), except a private or loopback address
from a private or loopback source (a master on the same network). A
publish from a source address outside the allowlist without `address` is
refused the same way when the master probes.

A listing's host key is published by one publisher at a time: a listing
whose host key is that of a live server of another publisher is refused,
`403 listing_taken`, so a listing's bytes copied from a list cannot be
published by someone else. The same publisher may carry it under another
server id (another key or instance). A host makes a new listing (a higher
sequence) at each of its publishes, so a live server that has taken no newer
listing of its host key for 90 seconds (3 intervals) is replaying a copy:
a publish of a newer listing of that host key takes the host key from it,
and its server is unlisted. A server an admin hid, or whose publisher or key
is not approved, holds no host key. A master shows its admins the publishes
it refused with `listing_taken`, to settle (see "Known limitations").

### The answer

`200` once the publish is taken and the master's decision for now is made;
`202` while the first probe of the server's address is unanswered:

```json
{"ok":true,"server_id":"4c5f15237c866f6f","listed":true,"interval":30,"ttl":90,"probe":"ok","reachable":true,
 "observed_address":"203.0.113.5","master_time":1760900000}
{"ok":true,"server_id":"4c5f15237c866f6f","listed":false,"interval":30,"ttl":90,"probe":"pending","reachable":false,
 "observed_address":"203.0.113.5","probe_address":"203.0.113.5:2302","master_time":1760900000}
```

| Field | Meaning |
| --- | --- |
| `listed` | Whether the list shows the server now |
| `interval` | Seconds between publishes the master expects (30) |
| `ttl` | Seconds after its last publish that a server is unlisted (90): one lost publish does not unlist it, three do |
| `probe` | `pending` (no answer yet at this address), `ok`, `failed` (three attempts unanswered), `skipped` (an admin marked the server trusted), `off` (the master does not probe) |
| `reachable` | The probe was answered |
| `observed_address` | The address the master saw the request come from |
| `probe_address` | With `pending` and `failed`: the address and port the master probes |
| `hidden`, `reason` | An admin hid the server: it is listed to nobody, and `reason` says why. With `probe: failed`, `reason` is the hint to log: `let UDP port <n> through your firewall or NAT, or set network.public_address` |
| `closed` | `true` after `closed: true`, a tombstone or `DELETE` |
| `warnings` | Body fields the listing contradicts, which were ignored |
| `master_time` | The master's clock |

A server with a failed probe is unlisted unless the master's policy lists
unreachable servers (`MASTER_LIST_UNREACHABLE`), as `reachable: false`.
The server logs each change of outcome once (not every 30 seconds), and
the failed probe's hint once a minute until the probe passes.

`DELETE /v1/servers/{server_id}`, signed by any key of the server's
publisher with an empty body, unlists the server at once and answers
`200 {"ok":true,"server_id":…,"listed":false,"closed":true,…}`; `404
not_found` for a server that is not the publisher's.

### Limits

| Limit | Answer |
| --- | --- |
| A server publishes at most once each 5 seconds, with a burst of 2 | `429 rate_limited` with `Retry-After` |
| 120 publishes a minute per source address, and per key | `429 rate_limited` |
| A body is 16 KB at most | `413 too_large` |
| A publisher has at most its quota of live servers at once (default 4; live: published within `ttl` and not closed) | `403 quota_exceeded` |
| An instance is published by one key of a publisher at a time | `403 instance_taken` (another key's server of that instance is live) |
| A publisher holds at most its quota and 16 server records (one per server id); for a new server id past that, its oldest records that are not live are dropped, and if none can be | `403 too_many_instances` |

## The listing

The listing is `p2p_lobby.c`'s (`listing_make`, `listing_read`), in network
byte order. The master reads it as `listing_read` does and checks its
signature as `listing_signed` does.

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | `"HL"` |
| 2 | 1 | Format: 1 |
| 3 | 2 | Network version (`HALO_PORT_NETWORK_VERSION`, 39 now) |
| 5 | 1 | Flags (below) |
| 6 | 4 | Sequence: the host's count of its listings (the newest wins) |
| 10 | 4 | The host's Unix time when it made the listing |
| 14 | 32 | The host's Ed25519 public key (one per run of the game) |
| 46 | 16, or 56 with the password flag | The invite's token (zero in a tombstone); with the password flag the token sealed with the password's key: nonce (24), MAC (16), the token encrypted (16), XChaCha20-Poly1305 with the host's key as additional data (`p2p_seal_token`) |
| then | 1 | Players (at most 255) |
| | 1 | Most players |
| | 1 | The gametype's engine: 0 none (co-op), 1 CTF, 2 Slayer, 3 Oddball, 4 King of the Hill, 5 Race |
| | 1 + up to 32 | The game's name: a length byte, then that many bytes |
| | 1 + up to 32 | The map's name |
| | 1 + up to 24 | The gametype's name |
| | 8 | The stamp: the game's kind (`game_modes_kind`: the OpenCE mode, 1 SWAT, 2 Infection, 3 Gun Game, or 0; with 0x80 for Hardcore), then 7 reserved bytes, zero |
| | 64 | The Ed25519 signature, by the listing's key, of the 12 ASCII bytes `hceu-lobby-1` followed by every byte before the signature |

| Flag | Meaning |
| --- | --- |
| 1 | Open: it takes players now |
| 2 | In progress: a game is under way (or loading) |
| 4 | Teams |
| 8 | Closed: a tombstone, the host stopped |
| 16 | Password: the token is sealed |
| 32 | Dedicated: a dedicated server (`halo --server` sets it, since network version 39) |

A listing is 140 bytes at least (all names empty) and 268 at most (a
password and every name at its length). A reader refuses one outside those
sizes, without `"HL"` and format 1, with a name's length byte past its
field's maximum or past the end, or whose stamp and signature do not end it
exactly. The names are sanitised as the game's `sanitize` does: a byte
outside 0x20 to 0x7E becomes `?`, and a zero ends the name. A master also
refuses a listing whose key has a small order.

The vectors' first listing (an open team game of a dedicated server,
sequence 412, 5 of 16 players, CTF on bloodgulch, named `OpenCE Dedicated`)
is these 169 bytes, of which the first 105 are signed. The vectors were made
at network version 38 and are kept as they are (the bytes and signatures
are fixed): their listings carry version 38 with flag 32, which the game
sets from version 39 on, so a game of version 39 reads them, checks their
signatures and hides them from its browser as another version's games, as
it does any. The examples' `network_version` of 38 in this document are the
vectors' too.

| Field | Bytes |
| --- | --- |
| Magic, format | `484c` `01` |
| Network version 38, flags 37 (open, teams, dedicated) | `0026` `25` |
| Sequence 412, time 1760900000 | `0000019c` `68f533a0` |
| Host key | `fc51cd8e6218a1a38da47ed00230f0580816ed13ba3303ac5deb911548908025` |
| Token | `a0a1a2a3a4a5a6a7a8a9aaabacadaeaf` |
| Players 5, most 16, engine 1 (CTF) | `05` `10` `01` |
| Name, map, gametype | `10` `4f70656e434520446564696361746564`, `0a` `626c6f6f6467756c6368`, `03` `435446` |
| Stamp (kind 0) | `0000000000000000` |
| Signature | `633dc63eb27828d6e589e34dc4336dbb2d514b2018e4bf4a3adf5aeb774b6a523fe662c3d29241c6f91c8714a8ca2b5e493aacb32bcaeb9bce715f15b76d230f` |

(`testdata/vectors.json` has it whole, with a password game's, a
tombstone's and an altered one that must be refused.)

## The probe

Before it lists a server, the master proves that the server's UDP port is
reachable from the internet and that the publisher key's holder answers on
it. It sends a datagram to the probe address (the published `address`, or
the request's source address with `tunnel_port`) from its probe socket:

| Offset | Size | Probe (35 bytes) |
| --- | --- | --- |
| 0 | 10 | `"OPCE-PROBE"` |
| 10 | 1 | 1 |
| 11 | 8 | The master's key id, raw |
| 19 | 16 | A nonce |

The game's tunnel socket, which dispatches what it receives by its first
bytes, recognises the magic and answers from the same socket to the
address the probe came from:

| Offset | Size | Answer (103 bytes) |
| --- | --- | --- |
| 0 | 14 | `"OPCE-PROBE-ACK"` |
| 14 | 1 | 1 |
| 15 | 8 | The master's key id, as the probe had it |
| 23 | 16 | The nonce, as the probe had it |
| 39 | 64 | The publisher key's Ed25519 signature of the 29 bytes `"probe"`, the master's key id, the nonce |

The server answers only a probe of exactly 35 bytes whose key id is that of
a master in its `masters.txt`, only to the probe's source, a given nonce at
most once per second, at most 2 answers a second to one source address and
at most 10 answers a second in all, so the probe cannot be used to reflect
traffic (a 103-byte answer to a 35-byte probe is the whole amplification),
and probes forged from one address cannot use up the answers a master's
probe needs. A probe whose nonce was answered in the same second counts
against neither limit.
The master takes only an answer from the probed address and port, with its
own key id, a nonce it made for that address and key, and a signature that
verifies with the key that signed the publish.

| Rule | Value |
| --- | --- |
| Attempts | 3, 2 seconds apart; failed 2 seconds after the third |
| Again | Every 5 minutes while the server is reachable (it stays listed meanwhile); a failed server at its first publish a minute or more after the failure; at once when its probe address or key changes |
| Nonce | Opaque to the server. A master uses a fresh nonce for each probe round (the round's attempts share it), and never the same one for two rounds of one address and key. `opence-master`'s is stateless: the first 16 bytes of HMAC-SHA-256(seed, `"opence-probe-1"` 0x00 address 0x00 key id 0x00 step), the step the decimal Unix time of the round's start divided by 30; it accepts the current and the previous hour's random seed and the current and the previous step, and starts rounds of one address and key at least a step apart |
| Duplicates | A server answers a given nonce at most once per second: it remembers the nonces it answered (the last 64) with the Unix second they came in, and ignores a probe whose nonce it answered in the same second, whatever came between (A, B, A in one second: the second A is ignored). A master's retries, 2 seconds apart, are answered again; a nonce resent within the second is not |

With the vectors' master key (id `39f713d0a644253f`) and the nonce
`101112131415161718191a1b1c1d1e1f`, the probe is
`4f5043452d50524f42450139f713d0a644253f101112131415161718191a1b1c1d1e1f`, the
signed message `70726f626539f713d0a644253f101112131415161718191a1b1c1d1e1f`,
and the publisher's answer
`4f5043452d50524f42452d41434b0139f713d0a644253f101112131415161718191a1b1c1d1e1f`
followed by the signature
`a63af355db038dff90e6c82abc0e252eddbd1ed3562580d7becaf3e4edbdd39cef7ef3163b91849ba57f6e83240b92df1efbce15dfa42f4a12981590a0234109`.

## The list

`GET /v1/servers`, with the optional query `network_version=<n>`,
`dedicated=1` (or `0`), `region=<region>`, `q=<text>` (a case-insensitive
substring of the name, map, gametype or variant; 32 characters at most).
No signature is needed and plain HTTP is allowed: the body is signed.

```json
{"protocol":1,"master":{"name":"OpenCE Master 1","key_id":"39f713d0a644253f","time":1760900000},
 "servers":[{"id":"4c5f15237c866f6f","publisher":{"id":"p_kz3c5lb2xvq7mdpa","name":"Milenko's servers","since":"2026-10"},
   "name":"OpenCE Dedicated","map":"bloodgulch","gametype":"CTF","variant":"CTF","players":5,"max_players":16,
   "teams":true,"in_progress":false,"password":false,"open":true,"dedicated":true,"kind":0,"engine":1,
   "network_version":38,"build":"build-206","region":"eu","reachable":true,"updated":1760899990,
   "listing":"<base64>"}]}
```

| Entry field | Meaning |
| --- | --- |
| `id` | The server id |
| `publisher` | The publisher's id (`p_` and up to 32 of `a-z0-9`), display name, and the year and month it was approved |
| `name`, `map`, `gametype`, `players`, `max_players`, `teams`, `in_progress`, `password`, `open`, `kind`, `engine`, `network_version` | From the listing |
| `variant`, `build`, `region` | From the publish |
| `dedicated` | The listing's flag 32 or the publish's `dedicated` |
| `reachable` | The last probe was answered (`false` for a trusted server or under `MASTER_LIST_UNREACHABLE`) |
| `updated` | The time of the server's last publish |
| `address` | Only with the operator's and the master's consent (above); absent otherwise |
| `listing` | Base64 of the listing's exact bytes, as the server published them |

The list holds at most 512 entries: dedicated servers first, then by
players. Its headers:

| Header | Value |
| --- | --- |
| `Signature` | `ed25519=` and the base64 of the master key's signature of the body's exact bytes (before any `Content-Encoding`) |
| `Signature-Key` | The master's key id, 16 hexadecimal digits |
| `Signature-Time` | The body's `master.time` |
| `ETag` | `"` and the first 16 hexadecimal digits of SHA-256 of the body, and `"`; `If-None-Match` with it answers `304` |
| `Cache-Control` | `public, max-age=5` (the master serves the same body for 5 seconds) |
| `Content-Encoding` | `gzip` when the request's `Accept-Encoding` asks for it |
| `Access-Control-Allow-Origin` | `*` (and `Access-Control-Expose-Headers` names the four above), for the web build |

A client:

1. checks `Signature` over the decompressed body bytes with the key in
   `masters.txt`, and that `Signature-Key` is that key's id, and refuses
   the whole body otherwise (a missing `Signature-Key` too; a client that
   cannot check Ed25519, such as an old browser, refuses every list rather
   than show one unchecked);
2. parses the body, and refuses it unless `protocol` is 1, `master.key_id`
   is the key's id, and `master.time` is no older than 300 seconds and no
   more than 300 seconds ahead of the client's clock (a list dated ahead
   would be taken for longer when it is replayed later, and a clock that
   far off is wrong). The time is in the signed body: `Signature-Time` only
   repeats it, since a header is not signed, and a client need not read
   it;
3. type-checks each entry, decodes its `listing`, runs `listing_read` and
   the host signature's check on it, and skips an entry that fails rather
   than the list; it shows the name, map, gametype and players of the
   signed listing (an entry's own fields are the master's word, and may
   differ); it filters control characters from every name, and takes at
   most 512 entries (a 512-entry list is some 26,000 JSON tokens: the game
   reads a list of 65,536 tokens at most);
4. merges the masters' lists: entries with the same `id` are one game (the
   newest `updated` wins), and an entry whose listing key is also heard on
   the brokers is one game, shown as master-listed;
5. never pings listed servers (that would tell every server the player's
   address): it shows the `region`, and the ping once a game is joined.

The vectors' signed body is
`{"protocol":1,"master":{"name":"OpenCE Master 1","key_id":"39f713d0a644253f","time":1760900000},"servers":[]}`
with `Signature: ed25519=18dmQnIfmRnC+wUwRJ/loxAH1VfASRBVnZgCfRuHrAw/2uo4thTibWnNQFScgRju+5lQtt709blK86BLT0fYCw==`
and `ETag: "cd755a50c5cfc457"`.

`GET /v1/servers/{server_id}` answers one listed server the same way:
`{"protocol":1,"master":{…},"server":{<entry>}}` with the same headers, or
`404 not_found`.

## Other endpoints

| Request | Who | Answer |
| --- | --- | --- |
| `GET /v1/publishers/{id}` | anyone (the list's limit) | `{"ok":true,"id","name","since":"2026-10","status","servers":<listed now>}`, or `404` |
| `POST /v1/applications`, `/v1/applications/…` | anyone | `410 applications_by_discord`: applications are made with the master's Discord bot (below), not over HTTP |
| `POST /v1/keys` | signed, an approved publisher's active key | Body `{"public_key":"ed25519:…"}`: adds a key (8 active at most). `201 {"ok":true,"key_id","status":"active"}`; `409 key_exists`; `403 too_many_keys` |
| `GET /v1/me` | signed, any key the master knows | The publisher's record: `{"ok":true,"key_id","status","publisher":{"id","name","since","status","quota","origin","keys":[{"key_id","public_key","status","updated","last_used"}],"servers":[{"id","instance","key_id","listed","probe","reachable","hidden","trusted","updated","name","players","max_players"}]}}`; for a key still in an application, `"publisher":null` and `"application":{"id","status","reason","created"}`; `403 key_revoked` |
| `GET /v1/master` | anyone | The master's name, key id, public key, contact, data policy and limits (informational: clients use the key in `masters.txt`) |
| `GET /healthz` | anyone | `200 {"ok":true,…}` while the database answers |
| `GET /metrics` | anyone (the list's limit) | Prometheus text, read at most once each 5 seconds: `master_servers{reachable}`, `master_publishers{status}`, `master_publishes_total{result}`, `master_probes_total{result}`, `master_list_requests_total`, `master_peer_requests_total`, `master_build_info{version,protocol}` |
| `GET /`, `GET /apply`, `/admin/*` | people | The list page (the list's limit), how to apply (the Discord bot, and an invite to its server), the admin pages |

## Applications

An operator applies with the master's Discord bot, in the master's
Discord server (the `/apply` page links to it): the bot's `/apply` command
takes the region (a menu of the master's choices) and the number of servers
at once as its options, and answers the applicant alone with a nonce for
the proof of key,
`opence-apply-<unix>-<16 hex>`, good for 24 hours and bound to that Discord
user (a nonce given to another user, or to none, does not prove this one's
key), and a button that opens the application's form: a display name, the
public key, the proof, and a note. The application's name is 3 to 32 printable ASCII characters,
`servers` is 1 to 64, and the region up to 16 of `a-z`, `0-9` and `-`. The
proof of key is required unless the master's policy says otherwise
(`MASTER_REQUIRE_PROOF`, true by default): it is the publisher key's
Ed25519 signature of the nonce's ASCII text, in hexadecimal (128 digits,
what `halo --server --sign <nonce>` prints) or base64. An application
without one is shown to the approvers as unproven. A key already a
publisher's is refused, and so is a key with an application pending. The
applicant is answered on Discord, and told of the decision by a direct
message; the applicant's identity is their Discord user id and username.

Each application is posted to the master's log channel with Approve and
Deny buttons, which only the members of the master's approver roles may
press; the admin pages decide too. Once decided, anywhere, the message
shows the outcome and who decided. The master's operator can also add an
application on its command line (`opence-master challenge` for the nonce,
then `add-application`), as the conformance test does.

A master limits applications, since each one is posted to its approvers:
`opence-master` takes one pending application at a time and 3 a day per
Discord user, 3 a day per key, and, of applications without a proof of key,
100 an hour in all; one with a proof is not refused for the hour's number,
but past 100 applications in an hour the approvers are alerted once and the
next ones are not posted one by one (its settings change these numbers). It
deletes a denied application 30 days after its denial.

The earlier HTTP application (`POST /v1/applications` with an e-mail
address verified by a mailed link, `GET /v1/applications/challenge`, and
the application's status by its mailed token) is gone: its endpoints
answer `410 applications_by_discord`. A master keeps the applications made
that way readable to its admins.

## Approval sharing between masters

The official masters share approvals, so an operator applies once. Each
master fetches each peer's `GET /v1/publishers?since=<unix>` (every minute
in `opence-master`), signed with its own master key in the request scheme
above (`keyid` is its master key id, and the signed string's master key id
is the peer's, the one it fetches from; a master answers only the peers it
is configured with, else `403 not_a_peer`). The answer is signed as the list
is (`Signature`, `Signature-Key`, `Signature-Time`, the body's
`master.time`):

```json
{"protocol":1,"master":{"name","key_id","time"},
 "publishers":[{"id","name","status","since","updated","origin","quota",
   "keys":[{"key_id","public_key","status","updated"}]}]}
```

A master shares only what it holds a word of its own on: the publishers it
approved or moderated (`status` is its own word: `approved`, `suspended` or
`revoked`; `null` when it only vouches for keys), and the keys it added or
revoked (`active` or `revoked`). `origin` is the key id of the master that
approved the publisher, as the sharing master knows it (a master records a
publisher new to it as come from the peer it fetched the record from,
whatever origin the record claims, and only that peer's records rename
it); `since` its approval time; `updated` the time of
the newest word in the record; `since=` returns the records updated after
it. The next fetch's `since` is the answer's `master.time` less 120 seconds,
on the peer's own clock.

The importing master keeps each master's word apart and takes the most
restrictive of the trusted masters' words as the publisher's status
(`revoked` over `suspended` over `approved`) and the smallest quota given;
a newer word from a master replaces that master's older one. So a
suspension or revocation anywhere wins everywhere, and only the master that
suspended a publisher can lift its suspension. A key revoked by any trusted
master is revoked, for good; a key two publishers claim is refused. A
master may refuse a peer's words (`MASTER_PEERS_TRUST`); they then count
for nothing.

## Errors

Every error is `{"ok":false,"error":{"code":"…","message":"…","field":"…"?},"master_time":<unix>}`.
The message is for people and may change; programs read the code.

| Status | Code | Meaning | A server's publisher should |
| --- | --- | --- | --- |
| 400 | `schema` | A field is missing, of the wrong type or out of range (`field` names it), or the body is not one JSON object | Log it once; it is a bug |
| 400 | `server_id` | The path's id is not the key's and the instance's | Log it once |
| 400 | `listing` | The listing does not parse, its signature fails, its key has a small order, or its time is an hour off | Log it once |
| 401 | `bad_signature` | The Authorization header is malformed or its signature fails | Log it once |
| 401 | `clock` | `ts` is more than 300 seconds from `master_time` | Log the difference (from the answer's `master_time`): the machine's clock is wrong |
| 401 | `replay` | The nonce was used in the last 600 seconds | Send a new nonce |
| 403 | `unknown_key` | No publisher or application has the key | Log `master: apply with key <id>` once a minute |
| 403 | `publisher_pending` | The key's application waits for review | Log `master: publisher pending` once a minute; keep publishing every 30 s |
| 403 | `application_denied` | The key's application was denied (`message` says why) | Log it once a minute |
| 403 | `publisher_suspended` | The publisher is suspended | Log it; keep publishing slowly |
| 403 | `publisher_revoked`, `key_revoked` | Revoked for good | Log it; stop publishing to that master |
| 403 | `quota_exceeded` | The publisher has its quota of servers live | Log it |
| 403 | `instance_taken` | Another key of the publisher has a live server of this instance | Log it; the old server expires after `ttl` |
| 403 | `listing_taken` | The listing's host key is that of another publisher's live server, which has renewed it within 90 seconds | Log it once a minute; it is someone else's listing, or the host key is shared; keep publishing |
| 403 | `too_many_instances` | The publisher holds its quota and 16 server records, none of which can be dropped | Log it; use fewer instance names |
| 403 | `tls_required` | Plain HTTP from beyond loopback | Use the `https://` URL |
| 403 | `not_a_peer` | A peer's request from a master not configured as a peer | |
| 404 | `not_found` | No such server, publisher or endpoint | |
| 409 | `key_exists` | The key added (`POST /v1/keys`) is known already | |
| 410 | `applications_by_discord` | An application sent over HTTP: applications are made with the master's Discord bot | Log it: apply with the bot's `/apply` |
| 413 | `too_large` | The body is over 16 KB | |
| 429 | `rate_limited` | Too many requests; `Retry-After` gives the seconds to wait | Wait that long (the header's name read in any case; the game waits a day at most, and 30 seconds without one) |
| 500 | `internal` | The master failed | Try again at the next interval |

## Privacy and limits

| What | Rule |
| --- | --- |
| Servers' addresses | Kept 7 days after the server's last publish, for probes and abuse handling; never served unless the operator and the master both chose to; not in the master's log unless its operator asks (`MASTER_LOG_ADDRESSES`) |
| Applicants | Their Discord user id and username, seen by the master's admins and approvers (the log channel) only; no address is kept |
| Applications | A denied one: deleted 30 days after the denial |
| Servers' records | A closed server's: deleted an hour after it closed; one not published again: after 24 hours (unless an admin flagged it) |
| Contacts | Discord names (and the earlier HTTP applications' e-mail addresses) are seen by the master's admins and approvers only, and are not shared with peers |
| Strings | Printable ASCII, cut to the listing's lengths: name 32, map 32, gametype and variant 24, build 32 |
| Requests | 60 list requests a minute per address (the list, one server, a publisher's page, the list page and the metrics together); the application limits above; bodies 16 KB at most; the publish limits above. Every limit by address counts an IPv6 address by its /64 |

## Known limitations

A listing's host key is bound to the first publisher whose live server
publishes it, not to the host's own publisher: the listing does not say
which publisher key the host chose. A publisher that copies a listing from a
list while the host's publisher is not publishing (more than `ttl`) holds
the host key, and the host's publisher is refused `listing_taken`, until the
copy goes 90 seconds without a newer listing; one that keeps copying the
host's newest listings from other lists keeps it until an admin hides its
server or suspends it. The planned fix is in the listing: the host signs the
key id of the publisher key it publishes with into the listing (a new
listing format, with a new network version), and a master takes a listing
only from that key.

## Versions

This is protocol 1: `protocol` in bodies, `/v1/` in paths and
`OpenCE-Master-v1` in signed strings (followed by the receiving master's
key id). A change that an older client or
server would misread is a new version, with new paths; fields may be added
to version 1, and both sides ignore what they do not know. The listing's
own format follows the game's network version: a client hides entries
whose `network_version` is not its own, as it does for the brokers'
listings.

## Testing an implementation

`testdata/vectors.json` in the master's repository has, from fixed seeds:
three key pairs with their key ids (the publisher's, the master's, a
host's), a server id, a signed publish and a signed `GET /v1/me` to the
vectors' master (its key id, the canonical strings, the signatures, the
headers), a probe and its answer,
four listings (a dedicated team game, a password game in progress with its
sealed token, a tombstone, and an altered listing that must be refused), a
signed list body with its headers and ETag, a small-order key that must be
refused, and a proof of key. Monocypher's `crypto_ed25519_sign`,
`crypto_ed25519_check` and `crypto_aead_lock` give the same bytes.
`tests/conformance/opence_publisher.py` is a reference publisher in Python
(keys, signatures, listings, publishing, the probe's answer), and
`tests/conformance/test_master_protocol.py` runs a master through the whole
flow: an application (on the master's command line, since the Discord bot
is beyond a test's reach), the HTTP application's `410`, a pending
publish, approval, the probe, the listed
server's exact bytes in a signed list, closing, every refusal, and two
masters sharing an approval, a suspension and a revocation.
