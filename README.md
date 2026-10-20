<p align="center">
  <img width="420" height="420" alt="gulchceopencelogo" src="https://github.com/user-attachments/assets/1787aa2f-3305-4353-8dd4-27d08661f6d4" />
</p>

<h2 align="center">
  A fork of OpenCE focused on bringing experimental features to OpenCE.
</h2>

<p align="center">
  <a href="https://github.com/chestahh/GulchCE/releases">download the latest release</a> ·
  <a href="https://discord.gg/pRFGPAMZkD">join the discord</a>
  <br>
  <br>
  forever <strong>compatible</strong>, for <strong>everyone</strong>
</p>
<hr>

<h3 align="center">
GulchCE will always be compatible with OpenCE, so everyone can play together.
</h3>
<br>
<p align="center">
Any fork compatible with OpenCE is also compatible with GulchCE. Crossplay galore!
</p>
<br>
<p align="center">
  Halo is a trademark of Microsoft. GulchCE is a fan project, not made or endorsed by Microsoft, Bungie, 343 Industries, or Halo Studios, and does not include any game assets or content.
</p>
<hr>
<br>

## What is GulchCE?
GulchCE is a fork of [OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE) - the Halo: Combat Evolved decompilation project targeting Linux,
Windows, Android and web browsers.

The decompilation is of the Xbox build 2342. (`cachebeta.exe`, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`).

## Will you always stay compatible with OpenCE?

Yes! GulchCE and OpenCE players can play online together on both Xbox and
Custom Edition maps. The only time they can't do crossplay is when GulchCE
players are playing MCC maps.

This is due to MCC-specific netcode additions that only function when an
MCC map is being played online. OpenCE clients wouldn't recognise the
network messages.

Once MCC map support is fully ironed out on GulchCE, OpenCE will be asked
if they would like the MCC map support feature on their own, main repo.

## How do I download a build?

Release & debug builds are available in the Releases section of the repository.
The release builds are for playing; the debug builds stop at the first failed assertion and log it.

### Arch-based Linux Distributions

The Linux binary release is available on the [AUR](https://aur.archlinux.org/packages/gulchce-bin).

## What about game data and assets?

The port does not include the game data. You would need an Xbox disc image
(`.xiso` or `.iso`) of Halo: Combat Evolved. All versions of the game
work. The port converts PAL maps to play as the North American (NTSC) maps do,
so players of the two versions can play together.

1. Start the game.
2. At the first start, the game asks for the disc image. Select it in the
   file picker. On Linux and Windows, a disc image (`.iso` or `.xiso`) next
   to the executable is used without the file picker. Where no file picker
   can be shown (on Linux, without the desktop portal or `zenity`), the game
   says to put the disc image next to the executable, and stops.
3. The game extracts the `maps/` folder. Then the game starts.

The `maps/` folder holds the Xbox maps only. If it holds Halo PC's maps
instead (a Halo PC or Custom Edition install's `maps` folder), the game says
so and stops: Custom Edition maps go in `custom_maps/`, beside `maps/`.

On Linux and Windows, the game puts `maps/` next to the executable. On
Android, copy the disc image to the phone first. The app puts `maps/` in its
data folder. Refer to [port/android/README.md](port/android/README.md). On
the web, the page copies `maps/` out of the disc image into the browser's
storage for the site; nothing is uploaded. Refer to
[port/web/README.md](port/web/README.md).

## How do I install MCC maps?

MCC custom maps use the separate `mcc_maps` directory and the
**MCC SINGLEPLAYER** and **MCC MULTIPLAYER** menu options. Maps appear in
the appropriate category automatically.

Inside `mcc_maps`, you need to create a `_shared` folder, and two subfolders inside
the `_shared` folder: `singleplayer` and `multiplayer`. From your existing Halo 1 MCC
installation, you need to take:

- The bitmaps.map file from `halo1/maps` and put it into `mcc_maps/_shared/singleplayer`.
- You then need to take the bitmaps.map AND sounds.map files from `halo1/maps/custom_edition`
and put these into `mcc_maps/_shared/multiplayer`.

You must obtain these files from an legally-owned installed copy of Halo 1 on the Halo: The Master Chief Collection.

**Some custom maps come with their own bitmaps.map and sounds.map resources. These are separate from the `_shared` folder.**

- These resources can be found inside `workshop/content/976730/[workshop ID]/halo1/maps`.
- Install these along the custom level .map files in your specific custom map folder, e.g. `mcc_maps\MercuryRising`.
- If you don't do this, the MCC compatibility layer will recognise this and refuse to load the map until these resources are installed.

Not every custom map will come with their own custom resources, and will instead rely on what's inside the `_shared` folder if necessary.

See [MCC map support](docs/mcc_maps.md) for installation, supported formats,
validation results and remaining compatibility limits.

## Mods

Mods are folders of Halo PC tag files, as the Halo Editing Kit and Invader
write them, in `mods/` next to `maps/`: texture, sound, text and model packs
that play over Xbox and Custom Edition maps on your machine only. Each has a
`mod.toml`. Every mod in the folder is on; the main menu's Mods turns them
off and on and sets the order they load in.
MCC maps always use their authored resources; loose-tag mods are bypassed.
Refer to [docs/mods.md](docs/mods.md).

## Platforms

Each platform has its own instructions:

| Platform | Instructions |
| --- | --- |
| Linux (32-bit x86 executable, OpenGL 4.5, SDL3) | [port/linux/README.md](port/linux/README.md) |
| Windows (32-bit x86 executable, OpenGL 4.5, SDL3) | [port/windows/README.md](port/windows/README.md) |
| Android (arm64 app, OpenGL ES 3, SDL3) | [port/android/README.md](port/android/README.md) |
| Web (WebAssembly, WebGL 2, SDL3; desktop Chrome, Edge and Firefox) | [port/web/README.md](port/web/README.md) |

The Linux README also gives the controls, the settings and the multiplayer
functions. These are almost the same on all platforms.

## Can I play multiplayer?

Yes! The game can play system link games on a local network and on the internet:

- A system link game can have up to 128 players on up to 128 machines.
- Linux, Windows and Android machines and browsers can play in the same
  game. On a local network, the native builds play system link; browsers
  play each other in rooms, over WebRTC
  ([port/web/README.md](port/web/README.md)).
- An invite link, or the server browser, lets a machine join a game on the
  internet, browsers too: a native build takes a browser's WebRTC
  ([port/linux/README.md](port/linux/README.md), "Browsers"). No server of
  this project is necessary.
- The netcode is new. Each machine moves its own player at once,
  and the host makes the decisions for the game. Refer to
  [port/linux/NETCODE.md](port/linux/NETCODE.md).

## How do I build the game?

You do not need the Xbox SDK. The port supplies the SDK declarations that
the game uses. Refer to [port/include/xdk](port/include/xdk/README.md).

To build the game:

1. Install Python and [ninja](https://ninja-build.org/).
2. Install the tools for your platform. Refer to the README for the
   platform.
3. In the root folder of the repository, enter `python configure.py`.
4. Enter `ninja` with the target for the platform:

| Target | Result |
| --- | --- |
| `ninja linux` | `build/linux/halo` |
| `ninja windows` (on Windows, or on Linux after `configure.py --windows-cross`) | `build/windows/halo.exe` and `SDL3.dll` |
| `ninja android_apk` | `port/android/app/build/outputs/apk/debug/app-debug.apk` |
| `ninja web` | `build/web/site/`, the web site (`python tools/web_serve.py` serves it) |

If you enter `ninja` without a target, ninja builds the game for the
computer that you use.

`tools/ci_build.py` makes the same builds as GitHub Actions. For example,
enter `python tools/ci_build.py linux release`.

### What build options do I have?

Give these options to `configure.py`:

| Option | Result |
| --- | --- |
| (none) | A debug build. A failed assertion stops the game. |
| `--release` | A release build. The game does not examine assertions, as in the retail game. |
| `--portable` | The Linux and Windows builds operate on all x86-64 processors. The Linux build also operates on older distributions and on SteamOS: refer to "Portable build" in [port/linux/README.md](port/linux/README.md#portable-build). Use this option for builds that you give to other persons. |
| `--lto=thin`, `--lto=off` | Less link-time optimization. The link is faster. |
| `--pgo=off` | No profile-guided optimization. |
| `--pgo=train` | Records a new optimization profile. Refer to "Optimization profiles". |
| `--profile` | A profiling build. It records CPU times. Refer to "Profiling builds". |
| `--web-emcc PATH` | The Emscripten `emcc` for `ninja web` (else the one on the PATH, or `~/emsdk`'s). |

Without `--portable`, the Linux and Windows builds use all the instructions
of the processor that builds them (`-march=native`). Such a build does not
always start on a different computer.

### Optimization profiles

The builds use profiles of the game to optimize the code:

- `pgo/halo_linux.profdata` for Linux and Android.
- `pgo/halo_windows.profdata` for Windows.

The profiles need clang 22 or later. With an older clang, the builds do not
use the profiles.

To record a new profile:

1. Delete the profile.
2. Enter `python configure.py --pgo=train`.
3. Enter `ninja linux` or `ninja windows`.

The build then plays the main menu and the first minute of each campaign
level. This procedure continues for approximately 15 minutes. The game
data must be in `assets/`.

### What about profiling builds?

A profiling build records where the game spends its time.

1. Enter `python configure.py --profile`, then build as usual.
2. Start a recording. Enter `profile_record` in the console (or the telnet
   console), or set `debug.profile_record = true` in `config.toml`. To
   record each game of a session, also set
   `debug.profile_record_when = "game"`.
3. Stop the recording. Enter `profile_stop`, load another map, or quit.

The game writes numbered `.part<n>.json` files under `profiles/` in its data root
and logs their paths. Parts are retained after the recording stops. Logs go to
standard error; a Windows release build without standard error writes them to
`halo.log` next to `halo.exe`.

`tools/net_report.py` is a helper for developers. Enter
`python tools/net_report.py <recording.part1.json>` to read a recording
as tables. The command also writes `<recording>.summary.txt`, a short text
file.

| Command | Result |
| --- | --- |
| `profile_record [seconds]` | Starts a recording. With a number of seconds, the recording stops after that time. |
| `profile_stop` | Stops the recording. |

`debug.profile_memory` sets the memory of a recording in MB (default 256,
from 4 to 1024). A recording has no length limit:
the game writes a part each time the memory is full. Refer to "Settings" in
[port/linux/README.md](port/linux/README.md).

On Android, the files are in `/sdcard/Android/data/com.halo.decomp/files/profiles/`.
Enter `adb pull` to copy the recording parts to a computer.

A profiling build plays with normal builds. It does not use
`--pgo=train`.

## Testing

The tests are listed in [AGENTS.md](AGENTS.md) ("Testing"). Most run
without the game data, in GitHub Actions too.

One check needs the retail maps, so it runs on your own computer (or on a
self-hosted runner), not in GitHub Actions: `tools/net_compare.py` plays
two builds together in a network game on one computer and checks that they
stay in step. Run it when a change can alter how a build calculates the
game, as a change to the simulation, the compiler or the maths does,
before it is merged. For example, to check a build with such a change
against a build without it:

```sh
python tools/net_compare.py --a build/linux/halo --b <other checkout>/build/linux/halo \
    --data <folder with maps/> --maps bloodgulch,beavercreek,hangemhigh --swap
```

Each game prints PASS or FAIL. A FAIL names the first player, tick and
value that differ. Refer to "Comparing two builds" in
[port/linux/NETCODE.md](port/linux/NETCODE.md).

## Licence

OpenCE is free software: you can give it to others and change it under the
terms of the GNU Affero General Public License as published by the Free
Software Foundation, version 3 of the License or (at your option) any later
version (`AGPL-3.0-or-later`). It comes with no warranty. The full text is in
[LICENSE](LICENSE).

- **From the change.** The AGPL applies from the commit that made the change,
  and to each release built from that commit or a later one. Earlier
  releases, and the source at every earlier commit, were released under
  [CC0 1.0](LICENSES/CC0-1.0.txt) and stay that way.
- **What OpenCE builds on.** The decompilation of
  [punpckhdq/halo](https://github.com/punpckhdq/halo) is under the AGPL 3.0
  from 17 October 2026, and under CC0 1.0 before that date.
  [bnunu/halo-1](https://github.com/bnunu/halo-1) is under CC0 1.0.
- **Contributions** are under `AGPL-3.0-or-later`. A change taken from a
  project under GPL 3.0 keeps that licence and its notices; the GPL 3.0 and
  the AGPL 3.0 let the two be combined (section 13 of each).
- **Hosts and servers.** If you change the game and let other persons play
  with your version over a network (for example, as the host of an internet
  game, or on a web site), the AGPL asks you to offer them the source of
  your version (section 13). The game as released has its source here, and
  each release's source is its tag (`build-<n>`).
- **Third-party code, fonts and art** keep their own licences, which are
  next to them (`port/third_party`, `port/assets/fonts`,
  `port/assets/icons`, `port/assets/text_filter`). The release downloads and
  the web site's `licenses.txt` carry their notices.
- **Halo is not covered.** The licence covers the work of OpenCE's authors.
  It gives no rights in Halo, its maps or anything else that Microsoft owns.
