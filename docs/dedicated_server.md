# The dedicated server: reference

`halo --server` runs the game as a host with no window, sound or player of
its own: other machines join it on the LAN, by its invite link or from the
server browser, as they join any host. `port/linux/README.md`, "Dedicated
server", says how to run one; this document is the reference for what it
takes and answers: the command line, the exit codes, the commands, the
HTTP API, the remote console (RCON) over WebSocket, the events, the log and
the files. `docs/master_protocol.md` is the master servers' protocol, which
the server publishes to and the server browser reads.

The code is `port/linux/src/server_*.c` (the platform side: the mode, the
standard input, the log, the rotation's grammar, the HTTP and WebSocket
server, the master servers' publisher) and `port/linux/game/server_*.c`
(the game side: the director that plays one game after another, the
commands, the gametypes, the status). The Android and web builds have no
dedicated server: they compile those files to nothing, but for
`server_json.c` (the JSON reader, which the server browser's list of master
servers uses in every build) and `server_rotation.c` (the rotation's
grammar and the gametype rules), which every build compiles.

## Command line

| Argument | What it does |
| --- | --- |
| `--server` | Dedicated server mode (as `HALO_SERVER=1` or `server.enabled = true`) |
| `--config <path>` | The settings file; default `config.toml` beside the executable. `HALO_CONFIG_FILE`, when it is set, wins over `--config` |
| `--version` | Prints `OpenCE build-<n> (network version <v>)` and exits 0 |
| `--keygen` | Writes a new publisher key (`server.publisher_key_file`) unless one exists, prints its public key and key id, exits 0; exits 3 when the key cannot be made, read or written |
| `--sign <text>` | Prints the publisher key's Ed25519 signature of the text, in hex (an application's proof of key), exits 0; exits 3 without a text or a usable key |
| `--probe [<invite>]` | A look at a server, for a list site that shows its state: joins nothing and never starts a server (whatever `--server`, `HALO_SERVER` or `server.enabled` say). It finds the invite's game over its tunnel (without an invite, the LAN's first) and prints it as one line of JSON, the game of the invite's host alone (never another game that answers), and exits 0; when nothing answers in 30 seconds it prints `{"ok":false,"error":"no game answered in 30 seconds"}` and exits 1. Internet play is as set (`network.online`, on by default), which an invite needs |
| `--help` | Prints these, exits 0 |

`--version`, `--help`, `--keygen`, `--sign` and `--probe` write no settings: the
settings file is read, never completed or made. `--probe` wins over
`--server`, `HALO_SERVER` and `server.enabled`: a probe never writes to
`config.toml`.

A probe's line:

```json
{"ok":true,"name":"OpenCE Server","map":"bloodgulch","gametype":"ctf","players":5,"max_players":16,
 "teams":true,"in_progress":true,"open":true,"dedicated":true,"network_version":38,"kind":0}
```

`gametype` is the game engine's name (`ctf`, `slayer`, `oddball`, `king`,
`race`, or `none`), `kind` the OpenCE gametype's kind the host sends (0 for
none), `open` whether it takes players now.

The release has a `halo-server` script beside `halo`, which runs
`halo --server` with its arguments.

## Exit codes and signals

| Code | Meaning |
| --- | --- |
| 0 | Stopped: `quit`, SIGTERM or SIGINT |
| 1 | A fatal error in the game (a halt, a map that failed to load); the last error lines were logged. The process exits within 5 seconds of the error, even when what runs at the exit (a hook, a listing's withdrawal) does not return |
| 2 | No usable game data: `maps/ui.map`, or every rotation map, missing or not a cache file; nothing was bound |
| 3 | Bad server configuration: no rotation entry that can be played (and `server.rotation_fallback` off), `server.web_port` past 65535, a web port the server may not bind (a port below 1024 without the right to it) or whose `server.web_bind` is an address the machine does not have, or a web password (admin, moderator or viewer) shorter than 8 characters with the web port bound beyond loopback (on loopback, a warning); `--keygen` or `--sign` failing. An entry that cannot be played (a bad gametype file among them) is left out with a warning: the server stops only when none is left |
| 4 | A port in use: the tunnel's UDP port, or the web port (another program listens on it) |
| 5 | Restart asked for (`sv_restart`, or an update installed: `server.auto_update = "install"` installs a new release once the server has been empty for 10 minutes, on a thread of its own: the server goes on meanwhile, and stops with 5 once it is empty again (a player who joined during the install plays on). An install that fails is tried again at the next check, 6 hours on) |
| 128 + n | Stopped by signal n: a second SIGTERM or SIGINT, or the shutdown not done 5 seconds after the first |

SIGTERM and SIGINT stop the server: a game in progress goes to its
postgame, every machine is told, the listings are withdrawn, and it exits 0.
SIGHUP reloads the server's settings, the rotation and the gametype files
(`sv_reload`). SIGUSR1 prints the status on standard output. SIGPIPE and
SIGTTIN are ignored.

## Standard output

One line per event, for the operator and the panel:

```
2026-10-19T18:03:11Z INFO server: ready (tunnel 2302/udp, web 127.0.0.1:8080, listing public)
2026-10-19T18:03:40Z INFO player: Milenko joined (slot 0, machine 1)
2026-10-19T18:04:02Z INFO game: started: CTF on bloodgulch, 2 players
```

The fields are the time (UTC), the level (`INFO`, `WARN`, `ERROR`), the
category (`server`, `lobby`, `game`, `player`, `chat`, `moderation`, `net`,
`master`, `console`, `health`) and the text. `server: ready` is printed once,
when the lobby is open and, for a public server, its listing has been
published (or 10 seconds on, as the brokers are other people's): it is the
line a panel waits for. With `server.log_format = "json"` each line is an
object instead, and the line a panel waits for holds `"msg":"ready (`:

```json
{"time":"2026-10-19T18:03:11Z","level":"info","cat":"server","msg":"ready (tunnel 2302/udp, ...)"}
```

Every line is also kept in the log the web's viewers read (`/api/log`,
`sv_log`, the RCON's `log` events), but for these, which go to standard
output alone: the web password a first start makes, the invite (a
private server's lets anyone in; `/api/status` gives it to admins), and
the owner's console, the commands typed on standard input or read from the
init file and what they answered (a password's command is shown as `(a
password's command)` there too).

A health line comes once a minute: `health: players=3 frame_ms_max=4.1
frame_ms_avg=1.20 cpu=2.1% rss=96MB` (the longest and the mean frame of
the last minute, in milliseconds). Standard error keeps the platform
layer's `halo-linux:` lines, and `debug.txt` everything, as for a player's
game.

## Commands

The same commands come from standard input, `server.init_file` (run once at
the start, a command a line, `#`, `;` and `//` starting comments), the telnet
console (`debug.telnet_console`, loopback), the RCON, `POST /api/command`
and the web page. Each runs on the game's thread between frames. Every
command has a role; a caller may run it if their role is that one or above:
**viewer** < **moderator** < **admin** < **owner** (standard input, the
init file and the telnet console are the owner's: whoever holds the
process). When standard input ends and `server.console_exit_on_eof` is on,
its last line runs, whether or not a line's end ended it, and then the
server stops.

A `<player>` is a slot number (`sv_players` lists them) or the start of a
name; a name that begins more than one player's is refused. A `<duration>`
is a number and a unit (`45s`, `30m`, `2h`, `7d`, `1w`), a bare number of
minutes, or `forever`; ten years at most. A word in quotes (`"two words"`)
is one word.

A command's text (a notice, a reason, `sv_set`'s value, a name) is kept to
printable ASCII: control characters (a line's end, a tab) and other bytes
outside it are dropped, as are the quotes around the whole text, and
standard input's and the init file's lines are cleaned so before they run.
The web is stricter where it takes a value as it is: `PUT /api/settings`
refuses a value with a control character (`schema`), and nothing is changed
for that setting. A command's reply is sent whole, however many lines.

Commands that are not the server's (the game's own console commands, the
scripts' globals and functions) need the owner role: from the web or the
RCON they are refused (`not a command of the server's`). The server's
commands come first, so `kick` and `ban` are always the server's below, not
the game's console commands of those names.

| Command | Role | What it does |
| --- | --- | --- |
| `help [command]` | viewer | The commands, or one command's use |
| `sv_status [--json]` | viewer | The server's state, map, gametype, players, uptime, games played, network version, listing and next game |
| `sv_players [--json]` | viewer | Each player's slot, name, team, score, kills, deaths, assists, ping and machine; with the owner role or `server.log_addresses`, their address and hardware id |
| `sv_rotation` (`sv_mapcycle`), `sv_maps`, `sv_gametypes`, `sv_version`, `sv_log [n]` | viewer | Lists: the rotation, the maps there are, the gametypes, the build, the log's last n lines |
| `sv_bans` (`sv_banlist`), `sv_settings` | admin | Lists: the bans of `bans.txt`, the settings `sv_set` changes |
| `sv_masters`, `sv_publisher` | viewer | The master servers and how each answered; the publisher key's id and public key (never its seed) |
| `say <text>` (`sv_say`) | moderator | A notice on every machine's screen |
| `tell <player> <text>` | moderator | A notice on one player's machine |
| `kick <player>` (`sv_kick`) | moderator | Drops the player's machine; it may join again |
| `ban <player> [<duration>] [<reason>]` (`sv_ban`) | moderator (a day at most), admin | Drops the player's machine and bans its address and hardware id, for the duration (else for ever). A moderator must give a duration of a day or less: a ban for ever, or longer, is refused |
| `unban <number\|address\|hardware id>` (`sv_unban`) | admin | Takes a ban out of `bans.txt`; the machine may join again at once, by the LAN or the internet |
| `mute <player> [<minutes>]`, `unmute <player>` | moderator | The host's mute of a player's chat and voice for everyone |
| `sv_map <map> [<gametype>]`, `sv_map <entry>` | moderator | Plays that game (or a rotation entry, `map:gametype+rule=value`; a co-op one too), below |
| `sv_map_next` (`sv_next`) | moderator | Ends the game without its scores and plays the rotation's next |
| `sv_end_game` | moderator | Ends the game: its scores, then the next |
| `sv_rotation_set <entries>`, `sv_rotation_add <entry>`, `sv_rotation_del <n>`, `sv_rotation_begin [<n>]` (`sv_mapcycle_add`, `_del`, `_begin`) | admin | Changes the rotation (written to `server.rotation`, 4096 characters at most) |
| `sv_set <setting> <value>` | admin | Changes one of the server's settings, now where it can be, and in `config.toml` (below); a setting its environment variable sets is refused. A value is 256 characters at most |
| `sv_name <name>`, `sv_maxplayers <n>`, `sv_password [<password>]`, `sv_public <0\|1>`, `sv_motd <text>` | admin | `sv_set` of `server.name`, `server.max_players`, `server.password`, `server.public`, `server.motd` |
| `sv_reload` | admin | Reads `config.toml`'s server settings, the rotation and the gametype files again (as SIGHUP) |
| `sv_restart` | owner | Stops as `quit` does, with exit code 5 (restart me) |
| `quit` (`exit`) | owner | Stops the server (exit code 0) |

`sv_map`'s game is played as soon as it can be, and its answer says when:
`next: <entry> (now)` in the lobby before its start was sent (the lobby
takes the entry, and the rotation's entry it replaced is played after it),
`(now: this game ends)` while a game is played (it ends at once, without
its scores), and `(once this game is over)` once the lobby's start was sent
or while a map loads (it is the next game). `sv_map` of the game being
played plays it again. The rotation goes on after it.

A co-op entry (`<level>:coop`) is ended as a game is: by `sv_end_game`,
`sv_map_next`, `sv_map`, `server.empty_seconds` and `server.idle_minutes`.
It has no scores: the server goes back to the lobby at once (no postgame),
and a level won ends it with `game_ended`'s reason `score`.

`sv_set` (and the settings of the web page, `PUT /api/settings`) changes
these settings, which `sv_settings` and `GET /api/settings` list:
`server.name`, `server.max_players`, `server.min_players`,
`server.password`, `server.public`, `server.rotation_fallback`,
`server.lobby_seconds`, `server.postgame_seconds`, `server.empty_seconds`,
`server.idle_minutes`, `server.team_start`, `server.motd`,
`server.frame_rate`, `server.web_password`,
`server.web_moderator_password`, `server.log_format`,
`server.log_addresses`, `server.status_file`, `server.auto_update`,
`server.coop_difficulty`, `server.region`, `server.master_publish`,
`server.master_publish_address`, `server.metrics`, and `network.text_chat`,
`network.voice_lobby`, `network.voice_mode`, `network.voice_kbps`,
`network.votekick`, `network.votekick_minutes`,
`network.votekick_ban_minutes`, `network.map_share`,
`network.map_share_upload`, `network.public_address`,
`network.coop_friendly_fire`, `network.coop_player_collisions`,
`network.coop_ally_friendly_fire` and `network.shield_effects`. Of them,
`server.status_file`, `server.auto_update` and `server.log_addresses` need
the owner role: from the web's admin, `sv_set` of one of them (`POST
/api/command`) answers `403 forbidden`, and `PUT /api/settings` leaves it
out, in its `errors` with the code `forbidden` and the message `<setting>
needs the owner role (the server's console)`. A web password is empty or
8 characters or more. A value outside a setting's choices or range is
refused (`400 failed` from `sv_set`, `schema` in `PUT /api/settings`'s
`errors`), with `<setting> takes a whole number from <min> to <max>` (or
`from <min>`) or `<setting> is one of <a>|<b>|<c>`; a choice may be given
in either case, and is kept as the table spells it:

| Setting | Choices or range |
| --- | --- |
| `server.max_players` | 2 to 127 |
| `server.min_players` | 1 to 16 |
| `server.lobby_seconds`, `server.postgame_seconds` | 0 to 300 |
| `server.empty_seconds`, `server.idle_minutes` | 0 or more |
| `server.frame_rate` | 30 to 120 |
| `server.team_start` | `one`, `both`, `skip` |
| `server.log_format` | `text`, `json` |
| `server.auto_update` | `off`, `check`, `install` |
| `server.coop_difficulty` | `easy`, `normal`, `hard`, `impossible` |
| `network.text_chat` | `on`, `links`, `off` |
| `network.voice_mode` | `off`, `team_proximity`, `team_enemy_proximity`, `team_global`, `team_global_enemy_proximity` |
| `network.voice_kbps` | 8 to 64 |
| `network.votekick_minutes` | 0 to 60 |
| `network.votekick_ban_minutes` | 1 to 1440 |
| `network.map_share_upload` | 16 to 20480 |
| `network.coop_friendly_fire` | `off`, `on`, `shields_only`, `explosives_only` |
| `network.shield_effects` | `stock`, `glow`, `light_show`, `overshield`, `glow_overshield`, `light_show_overshield`, `players` |

The settings `sv_set` does not change (`server.web_viewer_password`,
`server.web_port`, `server.web_bind`, `server.web_trusted_proxy`, the
paths, the master servers' files...) are changed in `config.toml` or their
environment variables: `sv_reload` reads the file's `server.*` settings
again, and the web port and its address are bound once, at the start. The
rotation is changed by the `sv_rotation_*` commands.

## HTTP

The server listens on `server.web_port` (8080; 0 for none) at
`server.web_bind` (`127.0.0.1`, an IPv4 address). Bound beyond loopback,
every web password that is set (admin, moderator, viewer) must have 8
characters or more, else the server exits with code 3. The page (`/`), the API
(`/api/...`), the RCON (`/ws`), `/healthz` and `/metrics` share the port.

Requests are HTTP/1.1 (or 1.0), `GET`, `HEAD`, `POST`, `PUT` or `DELETE`
(`HEAD` is answered as `GET` is, without the body, and counts as a read,
as a `GET`; `/api/login` takes `POST` alone), a head of 8 KB at most whose
lines end with CRLF (a bare LF, a CR inside a line, or more after the empty
line that ends it: `400 bad_request`) and a body of 64 KB at most, sent within 10 seconds of the
connection's opening or of its last answer (a connection that sends
nothing is closed then too). `Transfer-Encoding` and `Expect` are refused.
24 connections are taken at once; when they are all taken, the oldest that
waits for a request (no RCON, no request being answered) is closed for a
new one. A client that does not read its answers is read no further once
256 KB of them wait, and is closed past 2 MB.

Every request but `/healthz` counts against its address's allowance, the
page's files, `/metrics`, a sign-in and the RCON's upgrades (`/ws`) too: 60
a minute, or 600 with a session (429 `rate_limited` past it, with
`Retry-After: 60`). Behind `server.web_trusted_proxy`, a request's address
is the last entry of its `X-Forwarded-For` (the one the proxy appended: an
IPv4 address, `a.b.c.d` or `a.b.c.d:port`, or an IPv6 one, `v6`, `[v6]` or
`[v6]:port`), and its `X-Forwarded-Host` and `X-Forwarded-Proto` are
believed, whether or not it sends `X-Forwarded-For`; from any other
address they are ignored. IPv6 clients are counted by their /64: one
host's addresses share its allowance and its lockouts. Every answer is JSON
(`Content-Type: application/json`) unless said, with
`Cache-Control: no-store`, `X-Content-Type-Options: nosniff`,
`Referrer-Policy: same-origin` (under `no-referrer` a browser would send
the page's own changes with `Origin: null`) and
`Content-Security-Policy: default-src 'self'; frame-ancestors 'none'`.

### Signing in

`POST /api/login` with `{"password":"…"}` answers
`{"ok":true,"token":"<64 hex>","role":"admin","csrf":"<32 hex>","expires":<unix seconds>}`
and sets the cookie `opence_session=<token>; HttpOnly; SameSite=Strict;
Path=/`, with `Secure` when the request came through
`server.web_trusted_proxy` with `X-Forwarded-Proto: https` (never from
another address, whatever it says). The admin password
(`server.web_password`) gives the admin role, the moderator password
(`server.web_moderator_password`) the moderator role, the viewer password
(`server.web_viewer_password`) the viewer role; `{"by":"…"}` beside the
password names who signed in, in the log. A session lasts 12 hours, or 30
minutes without a request (the RCON's messages do not count: a page left
open asks `GET /api/session` now and then); 16 are kept at most.

A script passes `Authorization: Bearer <token>`. The page passes the
cookie, and on every request that changes something the header
`X-CSRF-Token: <csrf>`; the `Origin` of such a request (and of the
WebSocket's) must be the server's own. An `Origin: null` is taken only
when the browser's `Sec-Fetch-Site` says `same-origin`.

`POST /api/logout` ends the session (with the `X-CSRF-Token` by the
cookie; `GET` answers 405). `GET /api/session` answers
`{"ok":true,"role":…,"csrf":…,"expires":…}`, or 401.

When `server.web_password`, `server.web_moderator_password` or
`server.web_viewer_password` changes (`sv_set` and the settings change the
first two; `sv_reload` reads any of them from `config.toml`),
the sessions of its role end and that role's RCON connections close
(1008, `the password changed`).

Five wrong passwords from an address (an IPv6 one's /64) in 10 minutes
lock it out for 10 minutes (429), and no more than 30 passwords a minute
are checked in all. `/api/login` takes `POST` alone (any other method:
405).

### Errors

```json
{"ok":false,"error":{"code":"forbidden","message":"needs the admin role"}}
```

| Status | Codes |
| --- | --- |
| 400 | `bad_request`, `schema` (a field missing or out of range), `failed` (the command ran and failed: its lines in `lines`), `rejected` (rotation entries: `rejected[]`), `invalid` (a gametype file: `line`) |
| 401 | `denied` (no session, or a wrong password) |
| 403 | `forbidden` (the role), `csrf`, `origin` |
| 404 | `not_found` (also a command that is not one, a player or a ban that is not there) |
| 405 | `method` |
| 409 | `pinned` (a setting its environment variable sets), `changed` (a player's action whose slot holds another player now: the current name in `name`, nothing done) |
| 413 | `too_large` |
| 426 | `bad_request` (the WebSocket's version is not 13) |
| 429 | `rate_limited`, `locked` (with `Retry-After`) |
| 500 | `internal` (out of memory, a file that cannot be written) |
| 503 | `loading` (the game did not answer in 20 seconds, as while a map loads; with `Retry-After: 5`), `starting`, `busy` (8 RCON connections already; with `Retry-After: 5`) |

### Endpoints

| Method, path | Role | Body → answer |
| --- | --- | --- |
| `GET /healthz` | none | `200 {"state":"lobby"}` (or `loading`, `playing`, `postgame`) while the server hosts and its frames run; `503` with `{"state":"starting"}`, `{"state":"stalled"}` (no frame for a minute) or the other state otherwise |
| `GET /metrics` | none (`server.metrics`) | Prometheus text |
| `GET /api/status` | viewer | The status (below) |
| `GET /api/players` | viewer | `{"ok":true,"players":[<player>…]}` |
| `POST /api/players/{slot}/kick` | moderator | `{"name":"<the player's name, checked>"}` → `{"ok":true,"lines":[…]}`, or `409 changed` when the slot holds another player |
| `POST /api/players/{slot}/ban` | moderator | `{"name":…,"duration":"1d","reason":"…"}` (no duration: for ever, which a moderator's ban cannot be: `400 failed`) |
| `POST /api/players/{slot}/mute` | moderator | `{"name":…,"minutes":10}` |
| `POST /api/players/{slot}/unmute` | moderator | `{"name":…}` |
| `POST /api/players/{slot}/tell` | moderator | `{"name":…,"text":"…"}` |
| `POST /api/say` | moderator | `{"text":"…"}` |
| `GET /api/rotation` | viewer | `{"ok":true,"index":0,"entries":[<entry>…],"next":<entry>}` |
| `PUT /api/rotation` | admin | `{"entries":["bloodgulch:ctf",…]}` → the rotation, or `400 rejected` with `"error":{…,"rejected":[{"index":1,"entry":"…","reason":"…"}]}` and nothing changed |
| `POST /api/rotation/next` | moderator | `{}`: `sv_map_next` |
| `POST /api/rotation/play` | moderator | `{"map":"…","gametype":"…"}`: `sv_map` (a map or gametype that cannot be played: `400 failed`) |
| `GET /api/gametypes` | viewer | `{"ok":true,"builtin":["slayer",…],"schema":{"fields":[<field>…]},"files":[{"name":…,"base":…,"description":…,"error":null}]}` (`builtin`: the Xbox's gametypes, then OpenCE's; a file that cannot be read has `base` and `description` empty and `error` saying why) |
| `GET /api/gametypes/{name}` | viewer | `{"ok":true,"name":…,"text":"<the TOML>"}` |
| `PUT /api/gametypes/{name}` | admin | `{"text":"<TOML>"}` → `{"ok":true}`, or `400 invalid` with `line` and `message` (a name is `a`–`z`, `0`–`9`, `_` and `-`, 32 at most) |
| `DELETE /api/gametypes/{name}` | admin | `{"ok":true}` |
| `GET /api/maps` | viewer | `{"ok":true,"maps":[{"name":"bloodgulch","level":"levels\\test\\bloodgulch\\bloodgulch","kind":"xbox","title":"Blood Gulch"}…]}` (`kind`: `xbox` or `custom`; `title` is the map's display name, else its `name`) |
| `GET /api/bans` | admin | `{"ok":true,"bans":[{"n":0,"when":"2026-10-19 18:00:00","ip":"…","hwid":"…","players":"…","reason":"…","until":<unix>\|null}…]}` |
| `DELETE /api/bans/{n}` | admin | `{"ok":true}` |
| `GET /api/settings` | admin | `{"ok":true,"settings":[<setting>…]}` |
| `PUT /api/settings` | admin | `{"server.name":"…","server.max_players":16,"server.public":true,…}` (a value as JSON, a boolean, a number or a string, or its text: `"true"`, `"16"`; `null` empties it) → `{"ok":true,"applied":["server.name"],"errors":[{"name":…,"code":…,"message":…}]}`; each setting is changed or refused on its own |
| `POST /api/command` | the command's | `{"line":"sv_status"}` (511 characters at most) → `{"ok":true,"lines":["…"]}`, or a refusal (`failed`, `forbidden`, `not_found`, `pinned`) with the command's `lines` |
| `GET /api/log?after=<n>&limit=<m>` | viewer | `{"ok":true,"lines":[{"n":12,"time":<ms>,"level":"info","cat":"game","text":"…"}…],"next":12,"missed":0}` (`limit` 1 to 1024, 256 by default; the chat's lines for admins only) |
| `GET /api/events?after=<seq>` | viewer | `{"ok":true,"events":[<event>…],"next":<seq>,"missed":0}` |
| `GET /ws` | | The RCON (below) |
| `GET /`, `/app.js`, `/app.css`, `/icon.svg` | none | The page (`server.web_ui`) |

`next` is the `after` of the next request: the last line's `n` (or event's
`seq`) read. An `after` past the latest (as after a restart) reads from the
start. `missed` counts lines or events that left the ring (2048 lines, 512
events) before they were asked for.

### The status

```json
{"ok":true,"time":1760900000123,"name":"OpenCE Server","state":"playing",
 "build":"build-206","network_version":38,"uptime":3600,"games_hosted":4,
 "map":"bloodgulch","level":"levels\\test\\bloodgulch\\bloodgulch","gametype":"ctf","variant":"CTF","teams":true,
 "players":5,"max_players":16,"machines":5,"countdown":-1,"game_seconds":312,"team_scores":[2,1],
 "listing":{"online":true,"public":true,"password":false,"invite":"halo://join/…"},
 "rotation":{"index":0,"count":3,"next":{"text":"sidewinder:team_slayer","map":"sidewinder","gametype":"team_slayer"}},
 "frame_ms_max":4.1,"frame_ms_avg":1.2,"tick_ms_avg":1.2,
 "masters":[{"url":"https://master1.example.org","listed":true,"probe":"ok","code":"","message":"","updated":1760899990}],
 "update":{"available":false,"build":""}}
```

`state` is `waiting` (the menus settling), `hosting` (making the game),
`lobby`, `loading`, `playing`, `postgame` or `stopping`. `countdown` is the
lobby's seconds left, `-1` when it is not counting. `team_scores` is `null`
in a game without teams. `frame_ms_max` and `frame_ms_avg` are the longest
and the mean frame of the last minute (`tick_ms_avg` is the mean again, for
older readers). `invite` is given to admins only (empty for others). An entry of the rotation is
`{"text":"sidewinder:team_slayer+score=50","map":"sidewinder","gametype":"team_slayer","rules":{"score":50},"rejected":null}`
(`rejected` is why it cannot be played, else `null`). The rotation's
`index` (in the status and `GET /api/rotation`) is the entry being played,
`-1` while an `sv_map` game is played or before the first lobby; `next` is
the entry after the one played last (the first while none has been).

A player is
`{"slot":0,"name":"Milenko","team":0,"score":3,"kills":3,"deaths":1,"assists":0,"ping":42,"machine":1,"joined":1760899000,"muted":false}`;
with the owner role, or `server.log_addresses`, also
`"address":"203.0.113.5","hwid":"…"`. `team` is 0 (red), 1 (blue) or -1
(no teams); `ping` is in milliseconds, `null` when it is not known yet.

A setting is
`{"name":"server.name","type":"string","value":"OpenCE Server","default":"OpenCE Server","comment":"…","env":"HALO_SERVER_NAME","pinned":false,"secret":false,"choices":null,"min":null,"max":null}`;
`type` is `boolean`, `integer`, `real` or `string`, and `value` and
`default` are of that type in JSON (`true`, `16`, `"OpenCE Server"`); a
reader should also take their text (`"true"`, `"16"`), which older servers
sent. `choices` lists a word setting's words, `min` and `max` bound a
number's, each `null` where there is none (the table above). `env` is
`null` for a setting no environment variable sets. A secret one (the
passwords) has `value` and `default` empty and `"set":true` when it is
set. `pinned` is a setting
its environment variable (`env`) sets: it cannot be changed.

A gametype schema field is
`{"key":"score_to_win","alias":"score","table":"universal","engine":"all","type":"integer","min":0,"max":999,"choices":null,"default":null,"description":"…"}`
(`table`: the gametype file's table it goes in, `universal`, `options`,
`ctf`, `slayer`, `oddball`, `king` or `race`; `engine`: `all`, or the game
engine whose table it is; `type`: `integer`, `boolean` or `choice`).

## The RCON

`GET /ws` with `Upgrade: websocket` and the subprotocol `opence-rcon-1`.
Every frame is text holding one JSON object. The server speaks first:

```
→ {"type":"hello","protocol":1,"server":"OpenCE Server","build":"build-206","network_version":38,
   "capabilities":["events","status","rotation","settings","bans"]}
← {"type":"auth","password":"…","by":"paul"}       or {"type":"auth","token":"<session token>"}
→ {"type":"auth","ok":true,"role":"admin"}
→ {"type":"auth","ok":false,"error":{"code":"denied"}}  and the connection closes (1008)
← {"type":"command","id":1,"line":"kick Milenko"}    or {"type":"command","id":1,"command":"say","args":["two words"]}
→ {"type":"reply","id":1,"ok":true,"lines":["Milenko kicked by the host"]}
→ {"type":"reply","id":2,"ok":false,"lines":["…why…"]}                       (the command ran and failed)
→ {"type":"reply","id":3,"ok":false,"lines":[],"error":{"code":"busy","message":"…"}}   (not run)
← {"type":"subscribe","events":["player","chat","game","moderation","status","log","master"],"after":1830}
→ {"type":"event","seq":1831,"time":1760900000123,"event":"player_joined","slot":3,"name":"Milenko",…,"replay":true}
→ {"type":"event","seq":1831,"time":…,"event":"status","status":{…the status…}}   (at the subscribe, then every 2 s)
→ {"type":"event","time":…,"event":"log","n":5120,"level":"info","cat":"game","text":"…"}   (no seq: n is its sequence)
← {"type":"ping"}
→ {"type":"pong"}
→ {"type":"overflow"}  and the connection closes (1013): the client read too slowly
→ {"type":"error","id":…,"error":{"code":"…","message":"…"}}   (an unknown type, a malformed message)
```

A connection signed in with the page's cookie (on the same origin) needs no
`auth` message: its `hello` is followed by `{"type":"auth","ok":true,…}`.
A command is a `line`, or a `command` and its `args`, which the server
joins with spaces, quoting an argument that holds a space or is empty (511
characters in all at most). Ids are the client's (a number or a string),
echoed in the reply; each command gets exactly one reply. A connection
runs one command at a time: a command sent before the last one's reply is
not run. A reply whose command ran has `ok` and the command's `lines`, and
no `error`, whether it succeeded or failed; one that was not run has `ok`
false, no lines and an `error`: `denied` (not signed in), `rate_limited`
(32 commands a second, in bursts of 64), `bad_request` (no command, or too
long), `busy` (another command's reply is awaited) or `loading` (the game
did not answer in 20 seconds; the command may still have run, and is never
sent again by the server). `after` replays the events still in the ring
since that sequence number, marked `"replay":true`. A connection holds 1024
messages for the client at most; 8 connections are taken at once (a ninth
upgrade answers `503 busy`), and each upgrade counts against its address's
allowance (above). The
event groups are `player` (`player_joined`, `player_left`, `map_share`), `chat`
(`chat`), `game` (`lobby_opened`, `countdown`, `game_started`,
`game_ended`, `postgame`, `score`, `kill`), `moderation` (`kick`, `ban`,
`unban`, `mute`, `unmute`, `speed_hack`), `status` (`status`), `log`
(`log`, each log line as `{"n","level","cat","text"}`), `master`
(`master`), and `server` (`server_started`, `reloaded`, `warning`,
`shutdown`), which every subscriber gets.
The log's lines of the `chat` category go to the `chat` group, the others
to `log`.
`status` and `log` events go to their subscribers as they come and are
not kept in the ring: `after` does not replay them (`/api/log` reads the
log's own ring). A `status` event's `seq` is the latest event's (no new
number: the same one comes again until another event), and a `log` event
has no `seq`: its `n`, the log line's number and `/api/log`'s cursor, is
its sequence. A client keeps one cursor for the events (`seq`, of the
events of the ring) and another for the log's lines (`n`); a client that
reconnects with `after` may get events it had, and drops those whose `seq`
it has seen (a `status` event is never dropped so).

## Events

Every event has `seq` (rising, never reused while the server runs; a
`status` event has the latest one's, a `log` event none: above), `time`
(milliseconds since the epoch) and `event`:

| Event | Fields |
| --- | --- |
| `server_started` | `name, build, network_version, rotation[]` (the entries' texts) |
| `lobby_opened` | `map, gametype, variant` (the gametype's name as the game shows it), `teams, index` (the rotation's entry) |
| `countdown` | `seconds` (-1: paused) |
| `game_started` | `map, gametype, variant, teams, team_scores[]` (team games), `players[]` |
| `game_ended` | `map, gametype, variant, teams, reason` (`score`, `time`, `idle`, `empty`, `command`, `next`), `duration` (seconds), `team_scores[]` (team games), `players[]` |
| `postgame` | `seconds` |
| `player_joined` | `slot, name, team, machine` |
| `player_left` | `slot, name, reason` (`left`; `kicked` and `banned` when the server's `kick` or `ban` dropped the machine, `banned` for a speed hack's drop too) |
| `chat` | `channel` (`everyone`, `team`, `vehicle`), `slot, name, text` (admins only; the lines the host sends on) |
| `kill` | `killer, victim` (slots; -1 for none), `weapon` (the killer's weapon's or the killing object's tag name, or null), `friendly_fire` |
| `score` | `map, gametype, variant, teams, team_scores[]` (team games), `players[]` (once a second at most, on a change) |
| `kick`, `ban`, `unban`, `mute`, `unmute` | `name, by, reason?, until?` |
| `speed_hack` | `machine, names, rate, ahead_ticks, action` (`kicked`, or `unverified`: its datagrams alone said so, and it stays) |
| `map_share` | `machine, map, state` (`asked`, `downloading` at each tenth, `downloaded` or `failed` once for each request), `progress` (0 to 100: a percentage already) |
| `warning` | `text` (a newer release, with `server.auto_update`) |
| `reloaded` | `rotation_changed, rejected[]` (`{"entry","reason"}` of each entry left out) |
| `master` | `url, listed, probe, code, message` |
| `shutdown` | `reason, code` |

Players in events are as in `/api/players` for the viewer role (no
addresses). `variant` is the gametype's name as the game shows it (`Co-op`
in a co-op entry).

## Bans

A ban is a line of `bans.txt` (in `paths.logs`, else the data root). A
timed ban is kept out by its line's `until=` alone, and ends then. A ban
for ever, and a machine dropped for cheating, also keep the machine out
for the rest of the run by its address, its real address or its hardware
id. `unban` takes the line out and readmits the machine by
the line's address or hardware id, whether it joins on the LAN or by
internet play.

## The rotation

`server.rotation` (and `sv_rotation_set`, `PUT /api/rotation`) is a list of
entries, `map[:gametype[+rule=value...]]`, separated by commas, semicolons
or new lines; a line that begins with `#` is a comment (to its line's end,
commas and all). The text is 4096 characters at most (the entries past it
are left out, with a warning), an entry 255 characters and the list 64
entries. `config.toml` keeps a string setting of up to 4096 characters, so
a rotation that long is saved whole and read back after a restart.

- The map is a multiplayer map's name (`bloodgulch`: the Xbox's
  `maps/<name>.map`, else a Custom Edition map of `custom_maps`),
  `custom_maps\<name>` (a Custom Edition map; `/` is taken for `\`), or a
  level's path (`levels\test\bloodgulch\bloodgulch`). A campaign level is
  played as co-op only: `<level>:coop`, at `server.coop_difficulty`.
- The gametype (`slayer` when there is none) is a built-in's name
  (`slayer`, `team_slayer`, `slayer_pro`, `elimination`, `phantoms`,
  `endurance`, `rockets`, `snipers`, `stalker`, `ctf`, `ironctf`,
  `ctf_pro`, `invasion`, `oddball`, `team_oddball`, `reverse_tag`,
  `accumulation`, `juggernaut`, `king`, `team_king`, `king_pro`,
  `crazy_king`, `race`, `team_race`, `rally`, `team_rally`, and Halo PC's
  names `ffa`, `ffa_slayer`, `teamslayer`, `tslayer`, `koth`, `team_koth`,
  `iron_ctf`, `teamrace`, `teamoddball`), an OpenCE gametype's (`swat`,
  `infection`, `gun_game`, `hardcore`, `hardcore_ctf`, `hardcore_oddball`,
  `hardcore_king`), a gametype file's name (`gametypes/<name>.toml`), or
  `file=<name>` (a gametype a player's game saved, below). A gametype's
  name is letters, digits, `_`, `-` and `.`, in any case.
- A rule is a gametype rule's name or its short name (`score`, `time`,
  `weapons`, `ff`...; `GET /api/gametypes`'s schema lists them), with a
  value it takes; a rule of another game engine than the gametype's is
  refused. 48 rules at most; a rule given twice is the last.

An entry that cannot be played (a map that is not there, a bad gametype
file, a rule out of range) is left out with a warning, at the start and at
`sv_reload`; `sv_rotation_set` and `PUT /api/rotation` change nothing when
one of their entries cannot be played.

A change of the rotation (`sv_rotation_*`, `PUT /api/rotation`,
`sv_reload`) keeps its place: the next game is the entry after the one
played last, found again in the new list (the first entry only when none
has been played yet). `sv_reload` takes the rotation again when its text
changed, and also when only its entries' rejections changed (a map or a
gametype file added or taken out since).

`server.team_start` decides how a team game starts. `one` (the default)
starts it with the players of one team. `both` waits, saying nothing,
for a player on each team. `skip`, when one player waits for a team game
that `sv_map` did not ask for, plays the rotation's next entry without
teams instead (if it has one); it is checked when the lobby opens and
again when its countdown would begin. With no entry left, the server
stops with exit code 3 (2 when no rotation map is there), or plays
`bloodgulch:slayer` with `server.rotation_fallback`.

## Gametype files

A gametype file, `gametypes/<name>.toml` (the name `a`–`z`, `0`–`9`, `_`
and `-`), is a built-in or OpenCE gametype (`base`) and rules over it:

```toml
base = "team_king"     # the gametype it starts from
name = "Moving Hill"   # the name the game shows: 11 characters of printable ASCII

[universal]
score_to_win = 3

[options]
time_limit = 15
friendly_fire = "off"

[king]
moving_hill = true
```

Each rule goes in its table (the schema's `table`): `[universal]`,
`[options]`, and one game engine's, `[ctf]`, `[slayer]`, `[oddball]`,
`[king]` or `[race]`. A file gives each rule its full name
(`vehicle_set`, `team_vehicles`, `score_to_win`): the short names
(`vehicles`, `score`) are the rotation's alone. A file is 64 KB at most.
`deploy/gametypes` has examples. The server reads the `gametypes` folder
beside `config.toml` if there is one, else the data root's, and writes the
web page's files to it.

A gametype a player's game saved is played with `file=<name>`: copy the
saved gametype, as `<name>.gametype`, into the same `gametypes` folder
(beside `config.toml` if there is one, else the data root's).

## The listing

A public server (`server.public`, with `network.online` and
`network.public_lobby`) is published to each master server of
`masters.txt` (`docs/master_protocol.md`). A change that a list shows (the
listing made public or private, a player joining or leaving, the name, the
map) is published within 5 seconds. Made private, the server makes a new
invite and sends the masters a closed publish, which takes it off their
lists;
`network.public_lobby = false` stops the publishing too. A master's
`Retry-After` is kept (a day at most). `key_revoked` and
`publisher_revoked` stop the publishing to that master for the run;
`unknown_key`, `publisher_pending`, `application_denied` and
`listing_taken` are logged once a minute while they last.

## Files

| File | What |
| --- | --- |
| `config.toml` | Every setting (beside the executable; `HALO_CONFIG_FILE`, else `--config`, names another) |
| `init.txt` (`server.init_file`) | Commands run at the start, in the data root unless a full path |
| `gametypes/<name>.toml` | Gametype files: the `gametypes` folder beside `config.toml` if there is one, else the data root's |
| `gametypes/<name>.gametype` | Gametypes saved by a player's game (`file=<name>` in the rotation): the same `gametypes` folder |
| `publisher.key` | The publisher key for the master servers (beside `config.toml`). On Windows, a server's `config.toml` and `publisher.key` are given an access list of their owner alone (the entries inherited for Administrators and SYSTEM taken out) |
| `masters.txt` | The master servers (`server.masters_file`: beside `config.toml`, else beside the executable) |
| `bans.txt`, `cheaters.txt`, `debug.txt` | In `paths.logs`, else the data root |
| `status.json` (`server.status_file`) | The status, written every 2 seconds |
