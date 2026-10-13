# Custom Edition maps from the community's archives

Map sharing (`port/linux/README.md`, "Map sharing") sends a player who joins
without the game's Custom Edition map the host's copy. Some hosts do not
send it: a host with SHARE MAPS off (`network.map_share = false`), a build
that names its maps without their hash (a level of `custom_maps\<name>`
alone, which map sharing does not download), or a game seen in the server
browser before joining. For those, the game downloads the map from the
community's archives instead, the zips the maps' authors published, and
checks every byte against an index of the maps before it uses it.

There are some 6,000 Custom Edition maps, over 500 GB. Nobody needs them
all: a player downloads the one map a game is on, from its zip.

Nothing of this goes over the game's own connections, so it needs no
network version of its own (`port/linux/NETCODE.md`). A client looks the
host's map up by the name and version (the header checksum) that the
game's advertisement and settings already carry.

## For players

Nothing to set up. The download starts by itself:

- when you join a game from the games list (LAN, Direct Link or the server
  browser) whose map you do not have, under a name map sharing does not
  download. The join goes ahead by itself once the map is there;
- when you join a game whose host does not send its map. You leave the
  game, with a message that the map is being downloaded. Join again when it
  is there.

While it downloads, Halo's loading screen covers the menus, its light
sweeping from left to right as the map comes, with the map's name and how
much has come ("Downloading hugeass.map from HaloNet: 34 MB of 92 MB").
B stops the download. With `maps.download_screen = false` the menus stay
up, and the status line of the games list shows the download instead.

The loading screen's picture is the Xbox's own (the Halo ring and
LOADING...). Importing a disc image writes it into the maps folder as
`loading.tga`: the retail disc has no such file, and its `default.xbe`
holds the picture, which `port/linux/src/xiso.c`, the Android app's
`XisoExtractor.java` and the web page's `xiso-worker.js` take out. Without
one, from a maps folder copied by hand, the light sweeps over clouds that
the game makes.

A map comes from the first of these that has it:

1. The zips of HaloNet's map repository (maps.halonet.net) and of HaloMaps
   (halomaps.org), in `maps.archives`'s order. The map is taken out of its
   zip.
2. The web seeds (`maps.web_seeds`): folders on web servers that hold the
   maps under their file names. The default is a seed box's folder that
   holds every map of the index.

A map that is not the index's version once downloaded (from a zip or a web
seed) is thrown away, and the next source is tried.

Custom Edition's `bitmaps.map`, `sounds.map` and `loc.map`, which every
Custom Edition map needs, are downloaded the same way when missing.

No map already there is ever replaced, renamed or changed. A downloaded map
is written to `custom_maps` as `<name>_<hash>.map`, the name map sharing
gives that version (`hugeass_0e2b3ee4.map`), beside any other version that
you have. A resource map keeps its own name, and is written only when none
is there. A download goes to `custom_maps/downloads` until it is checked.

When a host names its map without the hash (`custom_maps\hugeass`) and your
`hugeass.map` is another version (another header checksum), the game plays
the version beside it whose header checksum is the host's
(`custom_edition_cache_present`, `port/linux/game/custom_edition_cache.c`).

The Linux and Windows builds download from the archives over HTTPS, with
the self-updater's HTTPS (`update.h`). The Android build and the web build
do not download from the archives: Android has no HTTPS of the platform
layer yet, and a web page fetches what it plays itself
(`port/web/src/web_map_downloads.c`).

The settings, in `config.toml` (`port/linux/README.md` has every setting):

| Setting | Default | Meaning |
| --- | --- | --- |
| `maps.downloads` | `true` | Download a map that is missing, and that the host does not send, from the archives. `false`: never. |
| `maps.download_screen` | `true` | Halo's loading screen over the menus while a map downloads (B stops it). `false`: the menus stay up, with the status line. |
| `maps.archives` | `"halonet,halomaps"` | The archives a missing map is taken from first, out of the zip that holds it, in this order: `halonet` (maps.halonet.net), `halomaps` (halomaps.org). Empty: none. Linux and Windows. |
| `maps.web_seeds` | `"https://halomaps.fractumseraph.net/maps/"` | Comma-separated `https://` folders that hold the maps under their file names: tried after the archives, and asked for pieces beside the players when map sharing downloads a map. Empty: none. Linux and Windows. |
| `maps.index` | `"map_index.txt"` | The index of the maps, next to `config.toml` unless a full path. |

Nothing is downloaded while Custom Edition maps cannot be played
(`game.custom_edition`).

## Map sharing's web seeds

When map sharing downloads a map from the game's players
(`port/linux/game/map_share.c`), a web seed sends pieces of it too, if the
index has the file at the size the host offered (the map by its name and
header checksum, a resource map by its name). The seed is asked for runs of
16 pieces (4 MiB) that no player has been asked for, with an HTTP range
request (`update_request_range`, `update.h`), and each piece is checked
against its hash, as a player's piece is (the pieces' hashes come from the
host or the players, checked against the file's hash the host offered). So a
web seed cannot send a wrong map: a piece that is not the file's ends the
web seed for that file, and three failed requests end it for the download.
Nothing of this goes over the game's connections, so it needs no network
version, and players of builds without it are unaffected.

With the default settings this uses the seed box's folder of every map, so a
host whose upload is slow does not hold its players' downloads up. It needs
`maps.downloads` on and an `https://` folder in `maps.web_seeds`; the
Android and web builds have none.

## How a map is known: the index

`port/assets/network/map_index.txt` lists every map, one a line,
tab-separated: the file name, the header checksum, the size, the piece
length and the info hash, then a column for each archive's zip that holds
it: `halonet:<zip file name>` (https://maps.halonet.net/maps/<zip file
name>) or `halomaps:<file id>` (https://halomaps.org/hce/detail.cfm?fid=<file
id>). The Linux and Windows builds put it next to the game as
`map_index.txt`. It was made from the halomaps.org and HaloNet archives
(5,829 maps, with the three resource maps) by `tools/map_index.py`.

A host's game carries the map file's header checksum as the map version
(`cache_files_map_version`, `source/cache/cache_files.c`), which a client
checks its own copy against. So a map is known here by its file name and
that checksum, and two versions of a map with one name are two entries. The
three resource maps have a checksum of 0 and are found by name. A map
named for its hash (`hugeass_0e2b3ee4`) is looked up without the hash. A
host's version that is not in the index is not downloaded.

The piece length and info hash are those of a single-file BitTorrent v1
torrent of the map whose info dictionary is exactly `{length, name, piece
length, pieces}`, in that order. A machine that has the file can make the
info hash from the index line alone: the game hashes each piece of a
download with SHA-1 (`port/linux/src/map_sha1.c`), and then the info
dictionary, and the download is the index's only if that hash is the
index's (`map_archives_verify`, `port/linux/src/map_archives_zip.c`). The
same torrents can be seeded by any BitTorrent client, and a seed box that
seeds them can serve its folder as a web seed.

To make the index from folders of maps:

```
python tools/map_index.py make --maps <folder> [--maps <folder>...]
    --resource-maps <Custom Edition maps folder> --torrents <output folder>
```

which writes `<info hash>.torrent` files (for seed boxes) and merges the
entries into the index, remembering the files hashed in
`<output folder>/.map_torrents_cache.json` so that a rerun hashes only new
ones. `python tools/map_index.py check` checks an index against its
torrent files. The archives' columns come from copies of their zips:

```
python tools/map_index.py sources --halonet <folder> --halomaps <folder>
```

with HaloNet's zips under their names and HaloMaps's named
`<file id>-<anything>.zip`. A zip holds a map when one of its files has the
map's name (in any folder), size and header checksum.

## How it operates

- `port/linux/src/map_downloads.c` reads the index, takes the game's
  requests (`map_downloads_fetch`: the level, its version and the files
  missing), downloads the files one at a time, and puts each in place.
  `map_downloads_poll` runs it each frame (`source/main/main.c`).
- `port/linux/src/map_archives.c` fetches one file on a thread of its own:
  HaloNet's zip, HaloMaps's (its page gives a token that its form takes,
  as a browser's download does), then the web seeds, with the
  self-updater's HTTPS (`update_request`, `update.h`). A download that
  stops starts again from the start next time.
- `port/linux/src/map_archives_zip.c` takes a map out of a zip (stored or
  deflated, with zlib) and checks a file against the index's info hash.
- `custom_edition_cache_files_missing` (`port/linux/game/custom_edition_cache.c`)
  says which files a level needs that are not there: the map, when no copy
  of the host's version is there, and the resource maps.
- The game asks for a download when map sharing cannot have the map
  (`cache_files_map_present`, `source/cache/cache_files.c`), when a host
  declines map sharing's request (`download_offered`,
  `port/linux/game/map_share.c`), and when a game in the games list is
  joined (`ui_widget_port_join_map_fetch`, `port/linux/game/menu_functions.c`).
- `port/linux/game/download_screen.c` draws the loading screen over the
  menus (`source/interface/progress_bar.c`).

Map files and zips are untrusted input: the zip reader checks every offset
and size against the file, writes nothing but the one file asked for, and
nothing is used before its info hash is the index's.

## Tests

- `tools/test_map_index.py` tests the index tool with made-up maps.
- `tools/test_map_download_check.py` runs `build/linux/map_download_check`
  (`tools/map_download_check.c`, which `ninja linux` builds): a map taken out
  of made-up zips, damaged zips refused, and a file checked against the
  info hash that `tools/map_index.py` gives it, other files refused. It
  also checks that the committed index starts as the game reads it.

Neither needs game data or the network.
