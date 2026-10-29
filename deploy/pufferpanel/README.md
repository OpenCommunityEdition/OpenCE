# The dedicated server on PufferPanel

`template.json` is a PufferPanel template for the dedicated server
(`docs/dedicated_server.md`), run in PufferPanel's host environment: it
downloads a release's `halo-linux-release.zip` into the server's folder,
runs `./halo --server` there, and stops it by writing `quit` to its
standard input.

## Using it

1. The node needs the 32-bit C runtime (Debian and Ubuntu:
   `dpkg --add-architecture i386 && apt install libc6:i386`). The template
   mounts `/lib32` and `/usr/lib32` into PufferPanel's sandbox, which does
   not otherwise; without them the 32-bit `halo` cannot start.
2. Add the template (Templates, New, and paste the file), and create a
   server from it.
3. Upload the `maps/` folder of your own copy of the game into the
   server's folder. Without `maps/ui.map` and the rotation's maps the server
   exits with code 2 and says which maps it looked for.
4. Start it.

## Ports

The **Tunnel port** (UDP, 2302 by default) is the one port players need.
The **Web UI port** is the web page, its API and the remote console, on
127.0.0.1 only. Its default is 8090, because PufferPanel itself listens on
8080. Each server on a node needs a tunnel port and a web port of its own
(or a web port of 0, for none).

## Files

Everything the server writes stays in its folder: `config.toml` (every
setting, written at the first start, with the web page's admin password if
none was given), `saves/` (`HALO_SAVE_ROOT`: the profiles and the map cache,
about 115 MB, built at the first start), `logs/` (`HALO_LOG_ROOT`:
`debug.txt`, `bans.txt`, `cheaters.txt`) and `status.json`
(`server.status_file`, the status every 2 seconds).

The template's variables are passed as `HALO_*` variables, which win over
`config.toml`: `sv_set` and the web page cannot change those settings.

## PufferPanel versions

The template is in the format of PufferPanel 2's templates (`run.command`
as a string, `run.stop` as the command written to standard input). We have
not confirmed how PufferPanel 3 imports it, nor the form of its standard
input settings; if a server made from it on PufferPanel 3 does not stop
with `quit`, set its stop to send `quit` to standard input (or SIGTERM,
which the server takes as well) in the server's settings.
