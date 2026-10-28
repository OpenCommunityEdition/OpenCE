# OpenCE - Halo: Combat Evolved for Linux, Windows, Android and the web

<p align="center">
  <img src="port/assets/icon/opence-icon.png" alt="OpenCE" width="128">
</p>


<p align="center">
  <a href="https://github.com/OpenCommunityEdition/OpenCE/releases/latest"><img src="https://img.shields.io/github/v/release/OpenCommunityEdition/OpenCE?label=Download&style=for-the-badge" alt="Download"></a>
  <a href="https://halocombatevolved.com/"><img src="https://img.shields.io/badge/Play-in_browser-blue?style=for-the-badge" alt="Play in browser"></a>
  <a href="https://discord.gg/9gqcHyr5km"><img src="https://img.shields.io/badge/Discord-join-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Discord"></a>
</p>

**OpenCE** (Open Community Edition) is an open source port of the Halo: Combat Evolved decompilation (Halo CE) based on an early Xbox development build (2342) and forked from
[bnunu/halo-1](https://github.com/bnunu/halo-1). The project aims to enhance the existing game and adds many features, such as 128-player multiplayer, online campaign co-op, mod packs, field-of-view settings, a server browser, peer-to-peer hosting, automatic map downloads, voice chat, support for Custom Edition maps and much more! 

The game runs natively on **Windows, Linux, Android** and in the **web browser** with nothing to install. A macOS port is in progress. Worth noting that it is not an emulator, **the game's own code is compiled for each platform** with a new layer in place of the Xbox's hardware. Runs on OpenGL (Android is on OpenGL ES and WebGL 2 in the browser). Sound, input, gamepads are all handled through SDL3. The game currently draws a frame every refresh of your display with Smooth Motion enabled, while the simulation runs at 30 ticks just like the original Xbox. 

<img width="1284" height="1007" alt="The game running on Windows" src="https://github.com/user-attachments/assets/0a4412e1-f6d3-48ed-8e17-0a374c2122f1" />
<p align="center"><em>OpenCE running on Windows (release build)</em></p>

## A copy of Halo: Combat Evolved is required

OpenCE does not include the game data. You **must provide your own Xbox disc image** (`.xiso` or `.iso`) of Halo: Combat Evolved. Every version of the game works. The maps of the European (PAL) version were made for a slower console - the game changes them to play as the North American (NTSC) maps do, so players of the two versions can play together.

1. Start the game (`halo.exe` on Windows and `halo` on Linux). 
2. At the first start you will get a pop up message asking for the disc image. Select it in the
   file picker. On Linux and Windows, a disc image (`.iso` or `.xiso`) next
   to the executable is used without the file picker. Where no file picker
   can be shown (on Linux, without the desktop portal or `zenity`), the game
   says to put the disc image next to the executable, and stops.
3. The game extracts the `maps` folder. Then the game starts.

The `maps` folder holds the Xbox maps only. If it holds Halo PC's maps
instead (a Halo PC or Custom Edition install's `maps` folder), the game says
so and stops: Custom Edition maps go in `custom_maps` folder, beside `maps` folder.

On Linux and Windows, the game puts `maps` next to the executable. On
Android, copy the disc image to the phone first. The app puts `maps/` in its
data folder. Refer to [port/android/README.md](port/android/README.md). On
the web, the page copies `maps/` out of the disc image into the browser's
storage for the site - nothing is uploaded ([port/web/README.md](port/web/README.md)).

## Download

GitHub Actions builds the game for each commit. 

Builds for latest release:

| Platform | Release | Debug | Guide |
| --- | --- | --- | --- |
| Linux | [halo-linux-release.zip](https://github.com/OpenCommunityEdition/OpenCE/releases/latest/download/halo-linux-release.zip) | [halo-linux-debug.zip](https://github.com/OpenCommunityEdition/OpenCE/releases/latest/download/halo-linux-debug.zip) | [Linux](port/linux/README.md) |
| Windows | [halo-windows-release.zip](https://github.com/OpenCommunityEdition/OpenCE/releases/latest/download/halo-windows-release.zip) | [halo-windows-debug.zip](https://github.com/OpenCommunityEdition/OpenCE/releases/latest/download/halo-windows-debug.zip) | [Windows](port/windows/README.md) |
| Android | [halo-android-release.zip](https://github.com/OpenCommunityEdition/OpenCE/releases/latest/download/halo-android-release.zip) | [halo-android-debug.zip](https://github.com/OpenCommunityEdition/OpenCE/releases/latest/download/halo-android-debug.zip) | [Android](port/android/README.md) |
| Web | [halocombatevolved.com](https://halocombatevolved.com/) (or [halo-web-release.zip](https://github.com/OpenCommunityEdition/OpenCE/releases/latest/download/halo-web-release.zip) for your own server) | [halo-web-debug.zip](https://github.com/OpenCommunityEdition/OpenCE/releases/latest/download/halo-web-debug.zip) | [Web](port/web/README.md) |


Release build is recommended for regular play. 

The debug build stops at the first failed check (assertion) and writes it to the log, so use it when you want to find and report problems.

The game updates itself. At start-up it looks for a newer release, and asks
if you want to install it. Refer to "Updates" in
[port/linux/README.md](port/linux/README.md#updates). The web site offers
each new build when it is out.

Each build of the `main` branch that passes on all four platforms is a new
release, and the web site then plays it. The [Releases](https://github.com/OpenCommunityEdition/OpenCE/releases)
page keeps the last five releases. If the latest build has a problem, get
an older build from that page.

Each release also has `SHA256SUMS`, the SHA-256 of each of its files, and
`SHA256SUMS.sig`, that file's Ed25519 signature by the project's release
key, whose public key is [deploy/release-key.pub](deploy/release-key.pub).
`sha256sum -c --ignore-missing SHA256SUMS` checks the downloads, and
`tools/release_sign.sh verify SHA256SUMS` (OpenSSL 3) the signature.

The Linux README also covers the controls, every setting and the multiplayer features. These are almost the same on all platforms. The settings are in `config.toml` next to the game, and many are in the menus too.

The Arch-based distribution release is available on the [AUR](https://aur.archlinux.org/packages/opence-bin).

## Multiplayer 

- **Supports up to 128 players** on up to 128 machines and up to 4 players on each device in split screen.
- **Every platform plays together:** Windows, Linux, Android and the browser all support cross-play! On a local network, the native builds can play LAN. Browser players can play each other in rooms over WebRTC ([port/web/README.md](port/web/README.md)).
- **Peer-to-peer internet hosting:** join through the server browser or an invite link. A host lists a game as public in the server browser, or keeps it private for players with the invite link. With the Discord integration, the host can also send the invite through Discord.
- **Online co-op campaign:** with settings for friendly fire, player collisions and the number of enemies as a multiplier based on per player or static. 
- **New gametypes:** SWAT, Infection, Gun Game and Hardcore (Slayer, CTF, Oddball and King), in Select Gametype's OPENCE bank; Edit Gametypes' OPENCE OPTIONS gives any gametype their rules ([port/linux/README.md](port/linux/README.md), "OpenCE gametypes").
- **Voice Chat:** push to talk (V) or an open microphone (can be changed in the menu's audio settings)
- **Map sharing:** a player who does not have the game's Custom Edition map downloads it from the other players.
- **Moderation:** the host can kick and ban players from the scoreboard menu or the `kick` and `ban` console commands. Players can vote to kick. 
- **Scrollable Scoreboard:** the mouse wheel or PageUp / PageDown scrolls through list of players in large multiplayer lobbies. 
- **A new netcode:** each machine moves its own player at once, and the host makes the decision for the game. Refer to [port/linux/NETCODE.md](port/linux/NETCODE.md).

## Custom Edition Maps

Halo Custom Edition maps are placed in `custom_maps` folder, not in the `maps` folder itself. It requires `bitmaps.map`, `sounds.map`, and `loc.map` in the `custom_maps` folder, which you must copy over from your existing installed copy of Halo Custom Edition. The maps folder is only for Xbox maps that's extracted from your disc image, not Halo Custom Edition. 

Halo PC Gearbox exclusive maps (Ice Fields, Death Island, Danger Canyon, Infinity, Timberland and Gephyrophobia) **are not included by default**. They would need to be copied over from your existing Halo Custom Edition install's `maps` folder into `custom_maps` folder too.

Once that's setup, the maps will appear on CUSTOM SINGLEPLAYER and CUSTOM MULTIPLAYER in the map list in Campaign's New Game, and on the Map screen of Create Game (Internet or LAN). A CUSTOM SINGLEPLAYER map plays as a campaign level, alone or as
online co-op.

Maps that require OpenSauce are not compatible.

Both the server browser and the LAN browser support automatic map downloads - when you join a game it will download the custom maps from other players, with any of `bitmaps.map`, `sounds.map` and `loc.map` you are missing. This works on a LAN and on the
Internet, when the host has map sharing enabled (on by default).
Refer to [docs/custom_edition_caches.md](docs/custom_edition_caches.md).

## Mods

Mods go in a `mods` folder next to `maps`. Each mod is a folder of Halo PC tag files, as the Halo Editing Kit and Invader make them, with a `mod.toml` describing it. Every mod in the folder is on by default. Mods in the main menu turns them on and off and sets their order. Think of it like a resource pack list, the mod at the top plays over the ones below it. The game also refuses mods that say they cannot be played together. 

Refer to [docs/mods.md](docs/mods.md) to make your own.


## Graphics

The game retains the original look and feel as it did on the Xbox by default. The video settings menus have additional options that go beyond it.

**Display**

| Option | What it does | Default |
| --- | --- | --- |
| DISPLAY MODE, RESOLUTION, WINDOW SIZE | Fullscreen, borderless or windowed, at any resolution (F11 switches) | Borderless, at the display's resolution |
| RESOLUTION SCALING | Draw at the window's resolution, or at the Xbox's 640x480 and scale it up | Native resolution |
| V-SYNC, FRAME RATE LIMIT | Wait for the display, or run free up to a frame cap; works with G-Sync and FreeSync | V-sync on |
| SMOOTH MOTION | A frame for every refresh of the display (60, 120, 240 Hz and more), blended between the game's 30 ticks a second | On |
| INSTANT AIM | In first person, the view turns the frame the mouse moves, not up to two ticks later | On |
| FOV, VIEWMODEL FOV | Field of view of the view and of the weapon and hands, from 80 to 150 degrees | Stock |

**Image quality**

| Option | What it does | Default |
| --- | --- | --- |
| ANTI-ALIASING | FXAA, SMAA, MSAA or SSAA | Off, as on the Xbox |
| TEXTURE FILTERING | Anisotropic filtering, from 2 to 16 - provides sharper floors, walls and terrain at an angle | Off, as on the Xbox |
| SHADOW RESOLUTION | Object shadows at 128 (the Xbox's), 256, 512 or 1024 pixels | 128 |
| PER-PIXEL LIGHTING | Lights models for each pixel, without the facets of the Xbox's per-vertex lighting | Off |
| MODEL DETAIL | Keep more model detail at a distance at high resolutions, or always the most detailed | Stock |
| HDR, HDR DISPLAY | Keeps explosions, plasma and the sun's glare past white and tone maps them; on an HDR monitor, shows them brighter than white | Off |
| HIGH-RES HUD, HIGH-RES TEXT | The HUD from hand-drawn redraws at 8x the size, and text in the Overpass font, sharp at any resolution | On |

**Extras**

| Option | What it does | Default |
| --- | --- | --- |
| LEGS | See your own legs when you look down in first person | Off |
| ARM SHIELDS | The energy shield's flare on your first-person arms too | Off |
| SHIELD GLOW, GLOW INTENSITY | Energy shields light up what is around them as they flare | Off |
| SHIELD COLOR | Your own shield in a multiplayer armor color, as your machine draws it | Default |
| OVERSHIELD | The overshield's own color, rising up the body as it charges | Off |
| PASSENGER THIRD PERSON | Third-person view for Warthog and Scorpion passengers (Campaign-only) | Off |


## How to build the game

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

### Build options

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

## For developers

### Testing

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

### Profiling builds

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


## Credits

The decompilation is of the Xbox build 2342 (`cachebeta.exe`, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`).
OpenCE starts from the decompilation of
[bnunu/halo-1](https://github.com/bnunu/halo-1), which is a fork of
[punpckhdq/halo](https://github.com/punpckhdq/halo). The artists of the
high-res redraws are in [AUTHORS.md](AUTHORS.md). The main contributors are Jonas Volman (@bnunu) and MrBruh (@cybersecurity). Everyone else who has contributed is on the
[contributors page](https://github.com/OpenCommunityEdition/OpenCE/graphs/contributors).

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


## Disclaimer
OpenCE (Open Community Edition) is an unofficial fan project. It is not endorsed by or affiliated with Microsoft, Xbox Game Studios, Halo Studios, Activision or Bungie. Halo and Halo: Combat Evolved are trademarks of Microsoft Corporation; Halo © Microsoft Corporation.
No game data is included. You must provide your own copy of the game.
