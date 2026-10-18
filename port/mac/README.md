# Mac

`PowerSpark.app` is the OpenCE game for Mac OS X 10.4 to 10.6, on PowerPC (G4 and G5)
and Intel Macs. It is one universal (ppc and i386) application. The game is
the same decompiled engine as on the other platforms. The Mac port gives it
its own platform layer (`port/mac/`) on Carbon, AGL (OpenGL), OpenAL, IOKit
and QuickTime, because the SDL3 of the other builds does not run on these
systems. The port is not yet released or committed: this document describes
the build in the working tree.

## Requirements

- Mac OS X 10.4 (Tiger), 10.5 or 10.6. The two PowerPC Macs it is tested on
  run 10.5.9; no 10.4 Mac has run it yet. Mac OS X 10.7 and later are not
  supported: they have no PowerPC support, and the Intel build uses the
  OpenGL 2.1 of 10.6.
- A PowerPC G4 or G5 at 1 GHz or more, or any Intel Mac.
- A graphics card with OpenGL 2.1 vertex and fragment programs
  (`GL_ARB_vertex_program`, `GL_ARB_fragment_program`) and 32 MB of video
  memory or more. The Radeon 9550 (32 MB) of an iBook G4 and the Radeon 9600
  class of a PowerBook G4 run the game. A Rage Mobility (8 MB) does not.
- 1 GB of memory. 2 GB of free disk for the Xbox maps (see "Game data").
- The game draws 480 lines and 640 columns, the Xbox's size, in a window of
  that shape; a wider window or display gets a wider picture (480 lines and
  as many columns as its shape gives, to 1280: Settings > Video > Widescreen,
  `display.resolution_scaling`), with the 3D view wider, the HUD at the
  corners and the menus centred. The picture is scaled to the window.

## Install

Open `PowerSpark-0.1.dmg` and drag `PowerSpark.app` to the Applications folder (or
anywhere). Open it. A window asks for the controller setup the first time
(see "Controllers"); then the game starts with the PC version's menus. The
app writes only to two folders of the user. They keep the name `OpenCE`, the
project's, from before the app was called PowerSpark, so that the settings,
saves and map cache of earlier versions are found:

| Folder | Contents |
| --- | --- |
| `~/Library/Application Support/OpenCE/` | `config.toml` (the settings), saved games and profiles, `XboxData/` (the maps copied from the Xbox disc), `data-source` (optional) |
| `~/Library/Logs/OpenCE/` | One log for each run, `OpenCE-YYYYMMDD-HHMMSS.log`. The newest 20 are kept. Send the newest when something is wrong |

To start over, delete these folders. To open the setup windows without the
first-run condition, hold Option while the app opens: it asks whether to
play, to open the Controller Setup or to open the Disc Image Setup.

## Game data

PowerSpark does not contain Halo's maps. It reads them from the player's own copy.

- **Xbox disc image (the intended source).** PowerSpark Setup (a window of the
  app, opened automatically when there is no data) asks for the player's own
  Halo: Combat Evolved Xbox disc image (`.iso` or `.xiso`) and copies its
  maps (24 files, about 2 GB) to `~/Library/Application Support/OpenCE/XboxData/maps`,
  and the disc's `default.xbe` and `bink/` (the movies, 650 MB) beside it.
  Nothing else is read from the image. The Xbox maps load, and the title
  menu, multiplayer and the campaign levels start from them, on Intel and on
  PowerPC Macs (the PowerPC engine converts each map's tags to its byte order
  as the map is read: `xbox_swap.h`, `XBOXMAP-PROGRESS.md`). Nothing but the
  menu pictures of the app, the fonts it carries, and the Application Support
  folder is needed: no Halo for Mac. The movies are the disc's (below), or
  MacSoft's if a Halo for Mac is installed; the inflated copy of each map (unpacked once, on its first
  start) is kept in `Application Support/OpenCE/map-cache`, and saves in
  `Application Support/OpenCE/saves`.
- **MacSoft's Halo, a fallback.** If `/Applications/Halo.app` (or
  `~/Applications/Halo.app`) is installed, PowerSpark reads the version 7
  maps and the intro movies of its `Contents/Resources/GameData`: when there
  are no Xbox maps, or when it is chosen.

Which of the two is used is the data source. By default (`auto`) it is the
Xbox maps whenever `XboxData/maps/ui.map` is there, else MacSoft's Halo if it
is installed, else PowerSpark Setup asks for the disc image. To choose:
`HALO_MAC_DATA_SOURCE=xbox` or `retail` in the environment, `--data-source`
on the launcher's command line, or the first word of the file
`~/Library/Application Support/OpenCE/data-source`. `paths.data` in
`config.toml` or `HALO_MAC_DATA_ROOT` names the MacSoft data folder explicitly.

PAL discs: the maps are read as the NTSC ones are. The engine's own loader
(`scenario_tags_load`, as on Linux) recognises the PAL build (`01.01.14.2342`)
and calls `pal_tags_loaded` (`port/linux/game/pal_tags.c`), which restores the
NTSC values the PAL maps changed for 25 Hz; PowerSpark Setup copies a PAL disc's
`maps` folder like any other. Not tested (there is no PAL image here); the
language folders of a PAL disc (`maps_de` and the others) are not copied, as on
Linux.

## Controls

The keyboard and mouse: WASD to move, the mouse to look, left click to fire,
Space to jump, Ctrl to crouch, E to use. In the menus: the arrow keys, Return
to choose and Backspace to go back. F12 frees the mouse, which a game window holds (`input.mouse_capture`), and
holds it again. Escape in a game opens the pause menu
(Resume, Settings, Leave Game); Quit is on the main menu.

### Controllers

The game reads the first HID game pad: an Xbox One S controller through its
driver (`XboxOneController-1.0.pkg`, a separate project that is not in this
repository), or another USB pad. Sticks, triggers, d-pad and buttons map as
on the Xbox. PowerSpark Controller Setup (`port/mac/controller_setup/`) installs
the driver and calibrates the dead zones and the look inversion; its
choices go to `input.gamepad_*` in `config.toml`. It needs Mac OS X 10.5 or
later. The driver package is put in the app only when the build is given
one (`--driver`); otherwise place it in
`~/Library/Application Support/OpenCE/Driver/`. Pair the controller over
Bluetooth first, and never update its firmware. `HALO_SKIP_CONTROLLER_SETUP=1`
skips the first-run window.

## Settings

The settings are those of the other builds (`config.toml`, with the table in
[port/linux/README.md](../linux/README.md), "Settings"). The game writes the
file with its defaults when it is missing. Of the settings, the Mac reads
`paths.data` and `paths.saves` too, and these only the Mac has:

| Setting | Default | Environment variable | Meaning |
| --- | --- | --- | --- |
| `mac.map` | `"bloodgulch"` | `HALO_MAC_MAP` | The map the game starts on when opened with no arguments (not by PowerSpark.app, which starts the menus) |
| `mac.detail` | `"auto"` | `HALO_MAC_DETAIL` | The performance preset: `"auto"`, `"high"`, `"low"` or `"lowest"` |
| `mac.texture_cache_mb` | `0` | `HALO_MAC_RETAIL_TEXTURE_CACHE_MB` | Video memory for textures, 4 to 48; 0 by the card |
| `mac.texture_skip_levels` | `-1` | `HALO_MAC_TEXTURE_SKIP_LEVELS` | Largest texture levels left out, 0 to 4; -1 by the card |

`game.intro_movies` (the start-up movies) and `game.movies` (which ones),
`display.texture_pack` (see [docs/texture_packs.md](../../docs/texture_packs.md))
and `input.prompts` apply as on the other builds. Refer to the table in
[port/linux/README.md](../linux/README.md) for each one.

### Movies

The start-up movie and the campaign's ending come from the Xbox data when it
has them, and else from MacSoft's Halo. If `XboxData/bink/intro.bik` and
`default.xbe` are there (`game.movies`, on by default), the start-up movie is
the disc's `intro.bik` (the Microsoft and Bungie logos in one movie) and the
ending is its `credits.bik`; the launcher passes the folder as
`HALO_MAC_XBOX_DATA`. They are decoded by the port's own decoder
(`port/linux/src/bink_decode.c`, which takes the format's tables from
`default.xbe`), drawn as a texture in the game's window, with the sound on the
audio unit, and a key, a click or a pad button skips one (Escape or Start
skips what follows too). The decoding is in plain C: on the slowest Macs it
may not keep 30 frames a second (`port/tools/bink_bench.c` times it);
a movie that falls behind its clock shows fewer pictures and the sound goes
on. If the Xbox data has none (a kit made before the launcher copied them:
make it again from the disc image, or copy `default.xbe` and `bink/` from the
disc into `XboxData`), or `game.movies = false`, MacSoft's `mgs.mov`,
`bungie.mov` and `ending.mov` play instead, with QuickTime, as before.

### Performance presets

`mac.detail = "auto"` looks at the graphics card: a PowerPC Mac with 64 MB of
video memory or less draws `"low"` (no specular, shadows or decals, low
particles, scenery smaller than 16 pixels left out, 32 in the campaign,
reverb off), and every other Mac draws the stock detail (`"high"`).
`"lowest"` is `"low"` without particles. A `display.*` setting that is not at
its default wins over the preset. Interpolation stops by itself while frames
take 32 ms or more (`HALO_MAC_AUTO_INTERPOLATION=0` keeps the setting's
answer). The measured frame times of hidden 640 x 480 runs of Blood Gulch,
after the optimizations: PowerBook G4 1.5 GHz about 35 ms, iBook G4 1.42 GHz
about 31 ms (the low preset), MacBook (GeForce 9400M) about 14 ms. 33 ms is 30
frames a second. The campaign levels are slower (The Silent Cartographer is
about 45 ms on the iBook).

## How PowerSpark.app starts the game

`Contents/MacOS/PowerSpark` is a small C launcher (`port/mac/app/opence_launcher.c`,
10.4 APIs only). It finds the data (above), makes the two folders, opens the
controller setup the first time (when `input.gamepad_setup_done` is not true
in `config.toml`), sends the game's output to a new log and replaces itself
with `Contents/MacOS/PowerSpark-game`, the game, with these settings in the
environment: `HALO_START_IN_MENU=1` (the PC version's menus; the title scene
`ui.map` behind them), `HALO_MAC_DATA_ROOT`, `HALO_MAC_MENU_ART_DIR` (the
bundle's `Resources/menus`) and `HALO_MAC_CONFIG_DIR` (the settings' folder).
Everything else is the game's own choice for the Mac it is on.

`PowerSpark --dry-run` prints the folders, the data source, the settings it would
set and the command, and does nothing else. `--setup` and
`--controller-setup` open the two setup windows. An engine built before
`HALO_MAC_CONFIG_DIR` existed (`Resources/engine.txt` lacks `config_dir`)
reads `config.toml` beside its own program; the launcher then copies it to
`Application Support/OpenCE/engine/` with a link to the real file.

## Building

On the Intel Mac that builds the port (Xcode 3.x, gcc 4.0 and 4.2, the 10.4u,
10.5 and 10.6 SDKs; Python 2.6, so no Python 3 there). The game's two
programs come from the rendered-session checker; the bundle script joins
them:

```sh
sh tools/mac_config_checks.sh                    # first, with MAC_CHECK_DIR=<output>/config
sh tools/mac_rendered_session_checks.sh bloodgulch.map bitmaps.map sounds.map
    # writes mac_rendered_session_ppc and _i386 in $MAC_RENDERED_SESSION_OUT
sh tools/mac_app_bundle.sh --engine-ppc $OUT/mac_rendered_session_ppc \
    --engine-i386 $OUT/mac_rendered_session_i386 [--driver PKG] [--version 0.1]
```

`tools/mac_app_bundle.sh` builds the launcher and the two helper apps (when
`--helpers` does not name them), joins the programs with `lipo`, copies the
menus, the icon and `Info.plist`, checks the plist and the architectures,
and makes `build/mac-app/PowerSpark-VERSION.dmg` (`hdiutil`). The icon is
`--icon FILE`, which is not in the repository: PowerSpark's is fan art and is
not the project's to ship. Without `--icon`, the app gets OpenCE's own
`port/mac/app/OpenCE.icns`. A `.png` given to `--icon` is
written by `tools/mac_icns.py` (Python 3 with Pillow): the classic icon types
that 10.4 reads (16 to 128 pixels, with 8-bit masks) and JPEG 2000 at 256 and
512 pixels for 10.5 and 10.6. `python3 tools/mac_icns.py --check FILE`
parses a file. Use `nice` on a shared build machine. Do not run the bundle's game on
a machine whose desktop someone else is using.

The other Mac tools are `tools/mac_*_checks.sh`; each one builds and runs a
test of a part of the port on the Mac. Notes on the work are in
`docs/ppc-mac-*.md`.

## System link

The game's netcode (`source/networking`, `port/linux/game/network_*.c`) runs in
the Mac session over the Mac's socket adapter (`port/mac/engine_sockets.c`:
Winsock calls on BSD sockets; the system link address is `network.address` and
`network.broadcast`, which `HALO_NET_ADDRESS` and `HALO_NET_BROADCAST` set).
The menus are the shared ones: Create Game > LAN hosts a game (this machine's
split screen players join it as its players), Join Game > LAN lists the games
found by broadcast and joins one. A session holds one map, so a game that
begins in the menus' session (the title's) is started again in a session of its
map, the netcode and its connections staying as they are
(`port/mac/engine_network_test.c`). Leave Game in the pause menu, a game's
end and a host that goes all return to the title.

`HALO_NETWORK_TEST=host:<map>` or `join` (with `HALO_MAC_NETWORK_TEST` the
same) plays a system link game without the menus, as the other ports' test
does. No machine may use `127.0.0.1` as its `network.address`: it means "this
machine" in the game's addressing; give each copy on one machine its own
(`127.0.0.2`, ...). Use `HALO_MAC_ENGINE_LOG=1` to see the netcode's own log.

## Known limitations

- Xbox maps on the PowerPC Macs: the conversion is checked in both byte orders on Intel and
  the maps render on the G4s; long play on a G4 is not yet measured.
- No Mac 10.4 machine has run the game; the 10.5 and 10.6 results are from
  three machines. The Controller Setup needs 10.5.
- System link (LAN) multiplayer works from the menus (Multiplayer > Create
  Game > LAN, Join Game > LAN; see "System link" above). Internet play, voice
  chat, map sharing and Custom Edition maps are not available on the Mac yet.
- Full screen switches the display to 640 x 480 and stretches the picture;
  the menus' pointer is not tracked in full screen.
- Audio is OpenAL (`HALO_MAC_AUDIO_OPENAL=0` selects the software mixer). Music
  and loops that are Ogg Vorbis in MacSoft's data are converted in the
  background.
- The app is not signed or notarized (10.4 to 10.6 do not need it).
- `HALO_MAC_*` variables beyond those above are diagnostics for the port's
  tests and may change.
