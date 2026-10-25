# The dedicated server in a container

`ghcr.io/opencommunityedition/opence-server` is the dedicated server
(`docs/dedicated_server.md`) as a container image: a release's
`halo-linux-release.zip` on Debian 12 with the 32-bit C runtime, run as
`halo --server` by the user 10001. CI builds it from each release
(`.github/workflows/server-image.yml`) and signs it with cosign; the tags are
`latest`, the release's (`build-206`) and `sha-<commit>`.

The game data is never in the image. Mount the folder that holds `maps/`
(from your own copy of the game: port/linux/README.md, "Game data") at
`/data`, read-only. Without it the server exits with code 2 and says which
maps it looked for.

## Running it

```sh
docker run -d --name halo --restart unless-stopped \
  --read-only --cap-drop ALL --security-opt no-new-privileges --tmpfs /tmp \
  -p 2302:2302/udp -p 127.0.0.1:8080:8080 \
  -v /srv/halo/data:/data:ro -v halo-state:/state \
  -e HALO_SERVER_NAME="My server" \
  -e HALO_SERVER_ROTATION="bloodgulch:ctf, sidewinder:team_slayer" \
  -e HALO_SERVER_WEB_PASSWORD=changeme-please \
  ghcr.io/opencommunityedition/opence-server:latest
docker logs -f halo
```

Or with Compose: `compose.yaml` beside this file runs the same, with the
settings taken from the shell or a `.env` file
(`HALO_DATA=/srv/halo/data`, `HALO_SERVER_NAME=...`). Its `https` profile
adds Caddy in front of the web port, with a certificate for
`HALO_WEB_DOMAIN`:

```sh
HALO_DATA=/srv/halo/data docker compose -f deploy/docker/compose.yaml up -d
HALO_WEB_DOMAIN=halo.example.org docker compose -f deploy/docker/compose.yaml --profile https up -d
```

The Compose file puts both on a network of its own, `172.30.80.0/24`, with
Caddy at the fixed address `172.30.80.10` and the server's
`HALO_SERVER_WEB_TRUSTED_PROXY` set to it: the server believes Caddy's
`X-Forwarded-For`, `-Host` and `-Proto`, so its rate limits and log name the
visitor rather than Caddy, and the session cookie is `Secure`. If that
subnet overlaps one of the host's, change the subnet, Caddy's
`ipv4_address` and the variable together.

`docker stop` sends SIGTERM: the game in progress goes to its postgame,
the players are told, the listing is withdrawn, and the server exits 0
within the 15 seconds Compose waits (`docker stop -t 15` otherwise).

## Folders

| Path | What |
| --- | --- |
| `/opt/opence` | The release: `halo`, `libSDL3.so.0`, `brokers.txt`, `masters.txt`, `healthz.sh`, the licences (read-only) |
| `/data` | The game data: `maps/`, and if you have them `custom_maps/`, `gametypes/` and `init.txt` (yours, read-only) |
| `/state` | The only folder written: `config.toml` (`HALO_CONFIG_FILE`), `publisher.key` beside it, the save root `saves/` with the map cache (about 115 MB), `logs/` (`debug.txt`, `bans.txt`, `cheaters.txt`) |

The image sets `HALO_CONFIG_FILE=/state/config.toml` rather than linking
`config.toml` beside the executable into the volume: the settings file is
then in the volume from the first start, and nothing in `/opt/opence` is
written. The server writes every setting there at the first start, with
the generated web password if none was given.

## Settings

Every setting is an environment variable (port/linux/README.md, "Dedicated
server", and docs/dedicated_server.md). A variable that is set wins over
`config.toml`, and the web page's settings cannot change it. The image sets:

| Variable | Value |
| --- | --- |
| `HALO_SERVER` | `1` |
| `HALO_CONFIG_FILE` | `/state/config.toml` |
| `HALO_DATA_ROOT` | `/data` |
| `HALO_SAVE_ROOT` | `/state/saves` |
| `HALO_LOG_ROOT` | `/state/logs` |
| `HALO_NET_TUNNEL_PORT` | `2302` |
| `HALO_SERVER_WEB_BIND` | `0.0.0.0` (within the container; publish the port on `127.0.0.1` or behind TLS) |
| `HALO_SERVER_WEB_PORT` | `8080` |

The ones most often given: `HALO_SERVER_NAME`, `HALO_SERVER_ROTATION`,
`HALO_SERVER_MAX_PLAYERS`, `HALO_SERVER_PASSWORD`, `HALO_SERVER_PUBLIC`,
`HALO_SERVER_MOTD`, `HALO_SERVER_WEB_PASSWORD`, `HALO_NET_PUBLIC_ADDRESS`,
`HALO_SERVER_PUBLISHER_KEY` (the master servers' key, as 64 hex
characters; `docker run --rm -v halo-state:/state <image> --keygen` makes one
into the volume instead), `HALO_SERVER_INSTANCE`, `HALO_SERVER_REGION`,
`HALO_SERVER_LOG_FORMAT=json`.

A tunnel port other than 2302 is published as the same number on both sides
(`-e HALO_NET_TUNNEL_PORT=2303 -p 2303:2303/udp`), as the server offers its
own port to joiners. Behind a NAT that STUN cannot see through, give the
public address and port in `HALO_NET_PUBLIC_ADDRESS`.

## Health

`HEALTHCHECK` runs `/opt/opence/healthz.sh`, which asks `GET /healthz` on
the web port (bash's `/dev/tcp`: the image has no curl) and is healthy on a
200: the server is hosting (lobby, loading, playing or postgame) and its
frames run. With `HALO_SERVER_WEB_PORT=0` it always passes.

## Building it

```sh
docker build -t opence-server deploy/docker                     # the latest release
docker build -t opence-server --build-arg RELEASE=https://github.com/OpenCommunityEdition/OpenCE/releases/download/build-206/halo-linux-release.zip deploy/docker
```

A build of your own (`python tools/ci_build.py linux release` leaves
`dist/halo-linux-release/`) goes in as a zip beside the Dockerfile:

```sh
(cd dist/halo-linux-release && zip -qr ../../deploy/docker/halo-linux-release.zip .)
docker build -t opence-server --build-arg RELEASE=halo-linux-release.zip deploy/docker
```

`RELEASE` is a URL or a file beside the Dockerfile. The image is 32-bit
x86 and runs on amd64 hosts only.

## Networking

Bridge networking works for internet play: the tunnel port is published one
to one, and the server needs no hole punching of its own. System link
(UDP 5150 and 5151, TCP 5150 and 5153) finds other machines only with
`--network host`.

Verify an image's signature:

```sh
cosign verify ghcr.io/opencommunityedition/opence-server:latest \
  --certificate-identity-regexp '^https://github\.com/OpenCommunityEdition/OpenCE/\.github/workflows/server-image\.yml@' \
  --certificate-oidc-issuer https://token.actions.githubusercontent.com
```
