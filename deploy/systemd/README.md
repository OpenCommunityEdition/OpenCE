# The dedicated server under systemd

`halo-server@.service` runs the dedicated server (`docs/dedicated_server.md`)
as a templated unit: one instance for each folder
`/opt/opence/servers/<name>`, all sharing one release in `/opt/opence/bin`
and one read-only copy of the game data in `/opt/opence/data`.

| Path | What |
| --- | --- |
| `/opt/opence/bin/` | The release (`halo-linux-release.zip`, unzipped) |
| `/opt/opence/data/` | The game data, only read: `maps/`, `custom_maps/`, `gametypes/` |
| `/opt/opence/servers/<name>/` | The instance's own, the only folder it may write: `config.toml`, `init.txt`, `publisher.key`, `server.env`, `saves/` (the map cache), `logs/` |

## Installing

```sh
sudo useradd --system --home-dir /opt/opence --shell /usr/sbin/nologin halo
sudo install -d -o halo -g halo -m 0750 /opt/opence/servers/main
sudo cp deploy/systemd/halo-server@.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now halo-server@main
journalctl -u halo-server@main -f
```

Open UDP 2302 (the instance's `network.tunnel_port`) in the firewall.

## Settings

`server.env` in the instance's folder holds its settings as `HALO_*`
variables, one a line (port/linux/README.md, "Dedicated server"). A
variable set there wins over `config.toml`, and `sv_set` and the web page
cannot change it; leave out what you want to change from the web page.

A second instance on the machine needs ports of its own, and its own
loopback address for system link:

```sh
HALO_NET_TUNNEL_PORT=2303
HALO_SERVER_WEB_PORT=8081
HALO_NET_ADDRESS=127.0.0.2
HALO_NET_BROADCAST=127.0.0.1
```

The web port stays on 127.0.0.1 unless `server.env` says otherwise (and
then it needs a password, and TLS in front of it).

## Exit codes

Exit code 5 (`sv_restart`, or an update installed) restarts the server; 2
(no usable maps) and 3 (bad configuration) do not, as a restart would fail
the same way. The unit has no console (`StandardInput=null`): commands come
from `init.txt` and the web port.
