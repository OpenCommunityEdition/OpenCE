# The dedicated server on MCSManager

`mcsmanager-config.json` holds two MCSManager quick-install entries for the
dedicated server (`docs/dedicated_server.md`). Each unzips a release's
`halo-linux-release.zip` into the instance's folder and runs
`./halo --server` there, stopped with `quit`.

| Entry | How it runs |
| --- | --- |
| Host | On the node itself, which needs the 32-bit C runtime (Debian and Ubuntu: `dpkg --add-architecture i386 && apt install libc6:i386`) |
| Docker | In a container of `ghcr.io/ptero-eggs/games:source` (Debian 12 with the 32-bit libraries), the instance's folder mounted at `/workspace` |

## Using it

1. Add the entries to the node's quick-install list, and create an
   instance from one.
2. Upload the `maps/` folder of your own copy of the game into the
   instance's folder. Without `maps/ui.map` and the rotation's maps the
   server exits with code 2 and says which maps it looked for.
3. Start it.

## Folders

The Docker entry's container is made again at each start, so it sets
`HALO_SAVE_ROOT=/workspace/saves` and `HALO_LOG_ROOT=/workspace/logs`: the
save root with the map cache (about 115 MB, built at the first start) and
the logs stay in the instance's folder rather than in the container. The
host entry keeps the save root in the home folder of the daemon's user
(`~/.local/share/halo-linux`), which every host instance of that user
shares; give each its own with
`HALO_SAVE_ROOT=saves` (in the instance's environment, or as
`env HALO_SAVE_ROOT=saves ./halo --server` for its start command).
`config.toml` is written beside `halo` at the first start.

## Ports

UDP 2302 is the tunnel port, the one players need; the web page, its API
and the remote console are on 127.0.0.1:8080. A second instance on the node
needs ports of its own: `HALO_NET_TUNNEL_PORT` and `HALO_SERVER_WEB_PORT`,
and for the Docker entry its published ports to match.
