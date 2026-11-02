# The dedicated server on Pterodactyl and Pelican

`egg-opence-dedicated-server.json` is an egg (`PTDL_v2`) that installs a
release of OpenCE from GitHub and runs it as a dedicated server
(`docs/dedicated_server.md`). Pelican imports the same file.

## Importing it

1. Admin > Nests (Pelican: Eggs) > Import Egg, and choose the file.
2. Create a server with the egg. Its primary allocation is the tunnel port
   (`network.tunnel_port`, UDP): the one port players need, from 1025 up
   (the container cannot bind lower ports).
3. Upload the game data: the `maps/` folder of your own copy of the game
   (port/linux/README.md, "Game data"), into the server's folder over SFTP
   (the panel's SFTP details, on the Settings tab). Without `maps/ui.map`
   and the rotation's maps the server stops at once with exit code 2 and
   says which maps it looked for.
4. Start it. The panel shows it running once it prints its ready line
   (`server: ready`, or `"msg":"ready (` with `server.log_format = "json"`).

## Files

| Path (in `/home/container`) | What |
| --- | --- |
| `halo`, `halo-server`, `libSDL3.so.0`, `brokers.txt`, `masters.txt`, the licences | The release (replaced on Reinstall) |
| `maps/` | The game data (yours) |
| `custom_maps/` | Custom Edition maps, with `bitmaps.map`, `sounds.map` and `loc.map` (shared with players who join) |
| `gametypes/*.toml` | Gametypes of your own (the release adds examples, never replacing yours) |
| `init.txt` | Commands run at each start (`init.txt.example` has examples) |
| `config.toml` | Every setting, written at the first start |
| `publisher.key` | The master servers' key: made by `./halo --server --keygen` on any machine with the release and uploaded, or given as the Publisher key variable instead |
| `logs/` | `debug.txt`, `bans.txt`, `cheaters.txt` |

Reinstall (Settings tab) downloads the release again, the one the Release
variable names, and keeps `config.toml`, `init.txt`, `publisher.key`,
`gametypes/`, `custom_maps/`, `maps/` and `logs/`.

## Variables

Every variable is one of the server's settings, by its environment name
(docs/dedicated_server.md and port/linux/README.md, "Dedicated server"). A
variable wins over `config.toml`, and the web page's settings cannot
change it.

| Variable | Setting |
| --- | --- |
| Release (`OPENCE_RELEASE`) | `latest` or a tag (`build-206`); used by Reinstall |
| Server name (`HALO_SERVER_NAME`) | `server.name`, 15 characters |
| Rotation (`HALO_SERVER_ROTATION`) | `server.rotation` |
| Max players (`HALO_SERVER_MAX_PLAYERS`) | `server.max_players` |
| Join password (`HALO_SERVER_PASSWORD`) | `server.password` |
| Public (`HALO_SERVER_PUBLIC`) | `server.public` |
| Message of the day (`HALO_SERVER_MOTD`) | `server.motd` |
| Lobby seconds (`HALO_SERVER_LOBBY_SECONDS`) | `server.lobby_seconds` |
| Web UI port (`HALO_SERVER_WEB_PORT`) | `server.web_port`; 0 for none |
| Web UI password (`HALO_SERVER_WEB_PASSWORD`) | `server.web_password` |
| Public address (`HALO_NET_PUBLIC_ADDRESS`) | `network.public_address` |
| Publisher key (`HALO_SERVER_PUBLISHER_KEY`) | The master servers' key itself, in place of `publisher.key` |
| Instance name (`HALO_SERVER_INSTANCE`) | `server.instance` |
| Region (`HALO_SERVER_REGION`) | `server.region` |
| Log format (`HALO_SERVER_LOG_FORMAT`) | `server.log_format` |

The startup command sets `HALO_NET_TUNNEL_PORT` to the primary allocation's
port and `HALO_LOG_ROOT` to `logs/`. Two variables are the administrator's
only: Web UI bind (`0.0.0.0`, as the allocation's address is the node's,
not the container's) and GitHub repository (`OpenCommunityEdition/OpenCE`).

## The web page

The admin page, its API and the remote console need a second allocation:
the node's administrator adds one to the server (or raises its allocation
limit so the owner can, on the Network tab). Give its port as Web UI port
and a password of 8 characters or more as Web UI password, restart, and
open `http://<allocation address>:<port>/`. The page has no TLS of its own:
put it behind a reverse proxy with HTTPS for anything but a private network.

## Stopping and restarting

The panel's Stop sends SIGINT, which the server takes as SIGTERM: the game
in progress goes to its postgame, the players are told, the listing is
withdrawn, and it exits 0. The console's `quit` command does the same, but
Wings counts an exit it did not ask for as a crash and starts the server
again; use Stop. `sv_restart` exits with code 5, which Wings also takes as a
crash and restarts, which is what it asks for.

Updates: `server.auto_update` is `install` by default, which installs a new
release into the server's own files and exits with code 5 (empty for 10
minutes, or after the game for a new network version), which Wings restarts
into the new build. To manage releases from the panel instead, set it to
`check` (`HALO_SERVER_AUTO_UPDATE`), change the Release variable (or keep
`latest`) and Reinstall, by hand or as a schedule.

## Checking the egg

`python tools/egg_build.py` copies `install.sh` into the egg and checks it;
`python -m pytest -q tools/test_egg.py` checks the egg and runs `install.sh`
in a `debian:bookworm-slim` container against a release served locally.
