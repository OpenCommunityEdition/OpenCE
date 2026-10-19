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
- A graphics card with OpenGL 2.0 and framebuffer objects (Mac OS X 10.5 and
  later give OpenGL 2.1 to most cards; Mac OS X 10.4 gives OpenGL 2.0 and GLSL
  1.10 only to newer cards, and the game makes GLSL 1.10 text there, which no 10.4
  Mac has yet run), vertex and
  fragment programs (`GL_ARB_vertex_program`, `GL_ARB_fragment_program`) and
  32 MB of video memory or more. At start the game checks what the context
  gives; on a card without OpenGL 2.0, the shading language or framebuffer
  objects it says so in a Mac alert, naming the card, and quits. The Radeon 9550 (32 MB) of an iBook G4 and the Radeon 9600
  class of a PowerBook G4 run the game. A Rage Mobility (8 MB) does not.
- 1 GB of memory. 2 GB of free disk for the Xbox maps (see "Game data").
- The game draws 480 lines and 640 columns, the Xbox's size, in a window of
  that shape; a wider window or display gets a wider picture (480 lines and
  as many columns as its shape gives, to 1280: Settings > Video > Widescreen,
  `display.resolution_scaling`), with the 3D view wider, the HUD at the
  corners and the menus centered. The picture is drawn with as many pixels as
  the window has (`mac.render_resolution`, "Render resolution"), not scaled
  up from 640x480.

## Install

Open `PowerSpark-0.1.dmg` and drag `PowerSpark.app` to the Applications folder (or
anywhere). Open it. The startup dialog opens (see "Startup dialog"); Play
starts the game with the PC version's menus, after the controller setup the
first time (see "Controllers"). The
app writes only to two folders of the user. They keep the name `OpenCE`, the
project's, from before the app was called PowerSpark, so that the settings,
saves and map cache of earlier versions are found:

| Folder | Contents |
| --- | --- |
| `~/Library/Application Support/OpenCE/` | `config.toml` (the settings), saved games and profiles, `XboxData/` (the maps copied from the Xbox disc), `data-source` (optional) |
| `~/Library/Logs/OpenCE/` | One log for each run, `OpenCE-YYYYMMDD-HHMMSS.log`. The newest 20 are kept. Send the newest when something is wrong |

To start over, delete these folders. To show the startup dialog once it is
turned off, hold Option while the app opens.

## Startup dialog

PowerSpark opens a window before the game, as MacSoft's Halo and Halo PC do: Display (Full
Screen or Windowed, Resolution from the display's modes, Render resolution
(`mac.render_resolution`), Widescreen, V-Sync, Smooth motion, Frame rate limit), Graphics (Detail, Texture quality, Anti-aliasing, Shadow
quality, Adaptive detail, Gamma, High-resolution HUD and text, Per-pixel lighting), Sound (on, Reverb,
Volume), Options (Play intro movies, Achievements, Button symbols, the Game data source, Set Up from
Disc Image, Controller Setup), and Play (Return), Quit (Escape, Command-Q) and "Don't
show this at startup". Every option is shown on every Mac, PowerPC or Intel; only the
defaults differ with the card (`mac.detail` auto), and the game limits at run time what
a card cannot do. Play writes the settings that were changed to `config.toml` with the
keys the game's Settings menus use (`display.mode`, `display.resolution`,
`display.window_size`, `display.resolution_scaling`, `mac.render_resolution`, `display.vsync`, `display.max_fps`,
`display.interpolation`, `display.gamma`, `display.high_res_hud`, `display.high_res_text`,
`display.anti_aliasing`, `display.shadow_resolution`, `display.per_pixel_lighting`,
`mac.detail`, `mac.adaptive_detail`, `mac.texture_skip_levels`, `audio.enabled`, `audio.volume`, `audio.reverb`,
`game.movies` with `game.intro_movies`, `game.achievements`), each in its section, and
keeps the rest of the file as it is. The data source goes to the `data-source` file;
"Don't show this at startup" writes `off` to the file `startup-dialog` in the support
folder. The dialog is not shown for scripted runs: with game arguments, `--no-dialog`
or `HALO_MAC_LAUNCHER_DIALOG=0`. `--dialog` shows it anyway; `--dialog-dump` prints
what it would show, without a window; `--dialog-layout` prints each control's rectangle
and its text's width in the system font and exits 1 if a text does not fit its control
(the labels beside the popups are as wide as their longest text, so a longer translation
makes the popups narrower, not the text cut). Hovering a control shows a help tag.
The window's "About PowerSpark..." button (and the application menu's About item) opens the About
window, below.

## About and Send Feedback

The About window shows the icon, the name, the version and build (from `Info.plist`), the build's stage
(`OpenCEBuildStage`, "alpha"), a disclaimer (PowerSpark is not made by or affiliated with Microsoft, Bungie
or 343 Industries; it is built on OpenCE, a community port of the Xbox game's decompilation, and plays from the
player's own copy of the game), a credit to OpenCE with a button to its project page, and whatever the build's
maker adds (below). Links open in the Mac's default browser (`LSOpenCFURLRef`). Both windows are Carbon controls
made in code (`port/mac/app/launcher_about.c`), measured like the start window: a paragraph is measured at
its width in the system font, and the window is as tall as the text needs. `PowerSpark --about-dump` prints each
control's rectangle and its text's size and exits 1 if one does not fit; `--about` and `--feedback` open the
windows alone; `--about-selftest` makes both without showing them and reads their controls back
(`tools/mac_about_checks.sh` runs the lot on the build Mac).

What a build adds of its own is not in the repository. `tools/mac_app_bundle.sh --branding FILE` copies a
`branding.plist` (and the files it names, which sit beside it) into `Contents/Resources`:

| Key | Meaning |
| --- | --- |
| `PublisherText` | a paragraph about the build's maker, in the About window |
| `Links` | up to three `{Title, URL}` buttons (http and https only) |
| `LogoFile` | a PNG shown in the About window, a plain file name beside the plist |
| `CAFile` | the CA list (PEM) that the feedback's HTTPS is checked with |
| `FeedbackURL` | an `https://` endpoint; without it there is no Send Feedback |
| `FeedbackApp` | the report's `app` field (default `powerspark`) |
| `FeedbackToken` | sent as `X-Feedback-Client` |

Without `--branding` the About window has the OpenCE link only and there is no feedback button. The
repository carries no CA list: a build with a `FeedbackURL` supplies its own (Mozilla's, from curl.se,
for instance).

Send Feedback asks for a topic (The game crashed or quit, Something looked wrong, It was too slow, A controller
didn't work, Network play, A suggestion, Something else), what happened, an optional email, and whether to
include the end of the newest log in `~/Library/Logs/OpenCE` (about 48 KB, with the home folder written as
`~`). Nothing is sent until Send is pressed; the window says what travels: the version and build, the Mac's
model, system and processor, and the log if ticked. The report is JSON (`feedback_report.c`: `id`, `app`,
`version`, `build`, `topic`, `summary`, `message`, `email`, `system` with `os`, `arch`, `model`, `cpu`,
`memoryMB`, `screen`, and `log`), written first to `~/Library/Application Support/OpenCE/Outbox/ID.json`
and then posted. A report the site takes, or refuses for good (400 or 413), is deleted; any other failure
leaves it in the Outbox, and PowerSpark tries the whole Outbox again, in the background, each time it starts
(a report older than 60 days is dropped).

The post is made by `Contents/MacOS/PowerSpark-send`, a program of its own (`feedback_send.c`) that the
launcher starts: it uses the vendored mbedtls (the Mac's configuration, `mac_mbedtls_config.h`) with the
branding's CA list, so TLS 1.2 and 1.3 work on every Mac (10.4 to 10.6's own networking stops at TLS 1.0),
PowerPC or Intel, and the launcher carries none of the library. It verifies the site's certificate and name,
and says when the Mac's clock is the reason it cannot. `PowerSpark-send --url URL --ca FILE --handshake`
checks a site's certificate against a CA list and sends nothing. Its objects are kept in `build/mac-feedback`.
No screenshot is offered: the launcher's windows are all there is to see, and the log says more.

## Updates

A build made with `tools/mac_app_bundle.sh --branding FILE` whose branding.plist has `UpdateFeedURL` (an https
address), `UpdatePublicKey` (the release key's public half, base64) and `CAFile` looks for new versions; the
public build has no feed, the About window says so and the app never looks. The feed is an appcast on the
maker's site: a property list with a `versions` array (or one release at the top level) whose items hold
`version`, `build`, `url`, `sha256`, `length`, `minimumSystemVersion`, `notes` and an Ed25519 `signature` of
those facts (`port/mac/app/update_core.h`). `tools/powerspark_sign_update.py` (run on the maker's own Mac, with
the private key kept out of the repository) writes the items; `keygen` makes the key pair.

- At launch, before the start window, the launcher looks quietly, at most once a day (`update-check` in the
  settings folder), unless `update.auto` is false or `HALO_UPDATE_AUTO` says no. Network trouble is only logged
  (`~/Library/Logs/OpenCE/update.log`). A newer release is offered with its notes: Install, Later, or Never Ask
  Again (which writes `update.auto = false`).
- Settings > Network Setup > CHECK FOR UPDATES turns the launch check on and off; CHECK NOW FOR UPDATES saves the
  settings, quits the game and starts `PowerSpark --check-updates --wait-pid N`, which always says what it
  found (a newer release, "up to date", or why it could not look), then goes on to the start window.
  Both rows are hidden on a Mac build with no update feed (the launcher leaves `HALO_MAC_UPDATE_LAUNCHER`
  unset), and the rows after them move up; the other platforms keep the toggle.
- Installing downloads the zip with `PowerSpark-update` (mbedtls and the branding's CA list, since Mac OS X 10.4 to
  10.6 stop at TLS 1.0), checks its size and SHA-256 against the signed appcast, unpacks it with `ditto` into a
  folder beside the app, checks that it is PowerSpark (bundle identifier, version, build, code for this Mac's
  processor), renames the app to `PowerSpark.app.old` and the new one into place (undone if the second rename
  fails), opens the new app and removes the old one. Any failure leaves the old app as it was. If the folder
  holding the app cannot be written (a disk image, another account's folder) it says so before downloading.
  Only PowerSpark.app is replaced; nothing in `~/Library/Application Support/OpenCE` is touched.
- Tests: `tools/test_powerspark_update.py` (the signing tool, and `update_core.c` on the host, including the
  swap in a temporary folder), and `tools/mac_update_checks.py`, which runs `tools/mac_update_scenarios.sh` on an
  old Mac against a local HTTPS server (`tools/mac_update_test_server.py`) through an ssh tunnel: 18 scenarios on
  each processor (install, tampered zip, wrong key, wrong app, truncated download, redirects, chunked answers,
  read-only folder, offline, wrong CA list, Never Ask Again, the daily limit, no feed). Test hooks:
  `HALO_UPDATE_ANSWER=yes|no|never`, `HALO_MAC_UPDATE_HEADLESS=1` (no windows, alerts to stderr),
  `HALO_MAC_UPDATE_OPENER` (replaces `open`), `HALO_MAC_BRANDING` (another branding.plist).

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
(`scenario_tags_load`, as on Linux) recognizes the PAL build (`01.01.14.2342`)
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

The game reads the first HID game pad. The way to connect a modern
controller (PS5 DualSense, PS4 DualShock 4, Xbox One/Series, Switch Pro or an
8BitDo pad) is the 8BitDo Wireless USB Adapter 2: plug it in, press its pair
button, put the controller in pairing mode (Create + PS, Share + PS, the Xbox
pad's pair button, the Switch Pro's Sync button; hold 3 seconds) and wait for
the adapter's light to stay solid. Where 8BitDo's manual puts the adapter in
Mac mode (hold Share, Create or View + Right for 3 seconds on a connected pad)
and for firmware updates (8BitDo's updater, on a newer computer), Controller
Setup says so. Wired USB pads and the Xbox 360 pad driver are not covered.
PowerSpark Controller Setup (`port/mac/controller_setup/`) shows these steps,
a live status line ("No input received yet", "Receiving input", or "Controller
found but sending nothing" after four quiet seconds), the pad's sticks and
buttons, and calibrates the dead zones and the look inversion; its choices go
to `input.gamepad_*` in `config.toml`. Sticks, triggers, d-pad and buttons map
as on the Xbox; the pad's buttons can be those of a later Halo game (the start window's
Controller layout popup, `input.gamepad_layout`: Halo 3, 4, 5 or Infinite; Halo CE's own is the default).
The start window's Options box also has an Unlimited sprint check box (`gameplay.unlimited_sprint`, off: the sprint control is left shift, or the left stick's click with the Halo 4, 5 or Infinite layout; in a network game the host's choice counts).
It needs Mac OS X 10.5 or later. `HALO_SKIP_CONTROLLER_SETUP=1`
skips the first-run window. The app carries and advertises no controller driver (the build's
`--driver` is ignored); a pad that a driver the player installed publishes as an
Xbox 360 pad (045E:028E) is still read.

Sony pads (and the adapter in a Sony mode) need no driver. A DualShock 4 (054C:05C4 and 054C:09CC, which includes
an 8BitDo USB receiver in its PlayStation 4 mode) and a DualSense (054C:0CE6, on a
USB cable) are read from their USB input reports (`mac_gamepad_report.c`, read
byte by byte, so PowerPC and Intel decode alike): Cross, Circle, Square and Triangle
are A, B, X and Y; L1 and R1 the white and black buttons (as an Xbox pad's bumpers); L2 and R2 the triggers (their analog bytes); Share (Create)
is Back, Options is Start; L3, R3 and the d-pad as they are. The touchpad
click and the PS button are decoded but not used. Every other HID pad is mapped by
its usages (X, Y, Z, Rz or Rx, Ry for the sticks, the hat, buttons 1 to 10 in the Xbox order). Up to four pads play
at once and may be connected while the game runs. The probe `tools/mac_gamepad_probe.c`
(`sh tools/mac_gamepad_probe_checks.sh`) shows a pad's raw reports and
the game's view of it. `mac_gamepad_is_playstation_at(slot)` says when a pad is one, and
`input.prompt_glyphs` ("auto") follows it (`platform_prompts_playstation`): the HUD's prompts then draw the
PlayStation faces, and the Xbox ui's button bar (A SELECT, B BACK) the cross and circle, from the button
pictures reduced to `port/assets/hud_mac` with the HUD's (`tools/mac_hud_reduce.py` reduces `port/assets/buttons` too;
run it again after replacing the PNGs there, such as the `ps_*.png`). A pad that cannot say which it is (an
8BitDo adapter in Dinput mode) is chosen once: Button symbols in PowerSpark's start window or in Gamepad Setup.

Controller Setup is one window with two tabs, Controllers and Bluetooth
(experimental), and one instance at a time: a second start (the launcher's
button, `--controller-setup`, Finder) brings the first to the front and quits
(`port/mac/controller_setup/README.md`).

#### Bluetooth (experimental)

A DualSense or DualShock 4 that the Mac's own Bluetooth owns (paired in the system's Bluetooth menu, with
any working controller, a dongle included) is read through IOHID like a USB one: the game reads the pad's
feature report 0x05, which switches it from its simple report to the full report 0x31 (0x11), decodes the
sticks, triggers, buttons, touchpad click and battery, and sends rumble, the light bar and the player lights
as the output report 0x31 with its CRC-32 (the same builder as the own stack's, `mac_gamepad_report.c`). The
own stack below stands aside while such a pad is open. Check with `tools/mac_gamepad_system_probe_checks.sh`.

A DualSense or DualShock 4 can connect over PowerSpark's own Bluetooth stack
(`port/mac/bt/`) through a TP-Link UB500 (Realtek RTL8761BU) dongle. It needs:

- Realtek's firmware, `rtl8761bu_fw.bin` and `rtl8761bu_config.bin`, which
  PowerSpark does not ship (a separate project provides a standalone driver
  for it; that work is in progress). The files are looked for in
  `input.bluetooth_firmware_dir`, `~/Library/Application Support/OpenCE/Bluetooth/`
  and `/Library/Application Support/RealtekBTFirmware/`.
- If Apple's driver takes the dongle, `sudo nvram bluetoothHostControllerSwitchBehavior=never`
  and a replug keep it off (undo with `sudo nvram -d bluetoothHostControllerSwitchBehavior`).
- A pad paired in Controller Setup's Bluetooth tab, and `input.bluetooth` set
  (`HALO_BLUETOOTH=1`), which the tab's checkbox "Use Bluetooth controllers in
  PowerSpark" writes to `config.toml`. It is off by default; with it off the
  dongle is not touched.

With the setting on, `mac_bt_input.c` runs the stack on its own thread; the game
thread only copies its latest snapshot on each input poll, so a pad's frame
enters through `mac_gamepad_poll_all` and takes a player slot, the PlayStation
prompts and hot-plug as a wired one does. Pressing PS on a paired pad connects
it. Rumble goes back through the stack (re-sent every 0.25 s). A dongle that is
missing or in use (Controller Setup, Apple's driver) is tried again every ten
seconds. The stack has not been run against the real dongle and pad with the
game. `sh tools/mac_bt_input_checks.sh` builds and runs a probe of the pad poll
with the stack.

## Settings

The settings are those of the other builds (`config.toml`, with the table in
[port/linux/README.md](../linux/README.md), "Settings"). The game writes the
file with its defaults when it is missing. Of the settings, the Mac reads
`paths.data` and `paths.saves` too, and these only the Mac has:

| Setting | Default | Environment variable | Meaning |
| --- | --- | --- | --- |
| `mac.map` | `"bloodgulch"` | `HALO_MAC_MAP` | The map the game starts on when opened with no arguments (not by PowerSpark.app, which starts the menus) |
| `mac.detail` | `"auto"` | `HALO_MAC_DETAIL` | The performance preset: `"auto"`, `"high"`, `"low"` or `"lowest"` |
| `mac.render_resolution` | `"auto"` | `HALO_MAC_RENDER_RESOLUTION` | Pixels a frame is drawn with: `"native"` (the window's), `"original"` (480 lines), a number of lines (`"720"`) or `"auto"` (native on an Intel Mac with more than 64 MB, else 480 lines). Every choice is offered on every Mac; one that does not fit the window or the card is cut to what does |
| `mac.adaptive_detail` | `"auto"` | `HALO_MAC_ADAPTIVE_DETAIL` | Lowers what the game draws while the frames take longer than a target, and puts it back when there is time. The game plays the same: only drawing changes. `"auto"`: on for a PowerPC Mac whose detail is low or medium, off for the rest (Intel); `"off"`; `"on"` (a target of 30 frames a second); or a target from 10 to 120 (`"45"`). Every choice is offered on every Mac (the start window's Graphics box). It measures the smoothed time of the frames (a limiter's wait is not counted) and steps down a fixed ladder after 40 frames over the target, back up after 240 frames comfortably under it (a step soon undone makes the next wait longer), never twice in a second. The ladder: detail objects (grass), particles no wider than 96 pixels, decals, lens flares, scenery and objects under twice the preset's pixels with a lower model detail level, four times, planar fog and the fog screen, particles no wider than 48 pixels, every model at its lowest level of detail, bipeds and vehicles (and what they carry) under 12, then 24 and then 48 pixels left out, and last the frame's resolution (`mac.render_resolution` drawn as 480 lines). A step the preset already has is passed over, and a client in another's game keeps the host's rules (grass, flares and fog). The log has a line for each step (at most one a second) and, at the end of a run, a summary with the frame time at each level (`adaptive detail`). |
| `mac.texture_cache_mb` | `0` | `HALO_MAC_RETAIL_TEXTURE_CACHE_MB` | Video memory for textures, 4 to 48; 0 by the card |
| `mac.texture_native_depth` | `-1` | `HALO_MAC_TEXTURE_NATIVE_DEPTH` | Uncompressed Xbox textures in video memory: 0 four bytes a texel, 1 the 8-bit formats and grey P8 at their own depth (same picture), 2 also the 16-bit formats; -1 is 1 |
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

### Render resolution and display modes

`mac.render_resolution` (Settings > Video > Graphics > Render Resolution) is
how many pixels the game draws a frame with. The picture keeps its shape (480
lines of the game, and the columns of the window's shape with Widescreen On;
4:3 with bars with it Off); the renderer scales the GL viewport, scissor and
clears so that it is drawn into the lower left of the window's framebuffer
with more pixels, and the present step scales it to the window. The choices
are `native` (the window's, or the display's, own pixels), `original` (480
lines, the Xbox's, which the window scales up) and a number of lines
(`600`, `720`, `1080`...), and `auto` picks by the Mac: native on an Intel Mac
with a card of more than 64 MB, 480 lines on a PowerPC Mac and on the
smaller Intel cards (the iBook's limit is fill rate). Every choice is offered
on every Mac. One that cannot be held is cut at run time, and the log has a
`render resolution:` line with the size drawn and what held it back: more
lines than the window has pixels for; more than the card's largest texture or
viewport; more pixels than its memory allows (twelve bytes a pixel in half
the video memory: 1.4 million on 32 MB, about 1440x1000). A picture drawn with
fewer pixels than the window has is scaled with linear filtering, or doubled
without blur where the window is a whole number of its pixels each way.
The loading screen (mac_loading.c) is not the game's frame: it fills the window's
framebuffer itself, as the movies do (`tools/mac_loading_checks.sh` enters a level
through the menus in a hidden 1280x800 window and checks the loading frames' size). `mac.loading_indicator` adds a "LOADING" label,
a ring of turning dots and a progress bar to it.

What is drawn with more pixels is the 3D view, the HUD's high-res redraws
and everything drawn as geometry. Text and the HUD pictures the game has
from the map are scaled up as before. Screen effects that read the frame
(the sun's glow, camouflage, cinematic effects) read it from a copy of the
size drawn.

Fullscreen (Settings > Video > Display Mode) offers the display's own modes
(`CGDisplayAvailableModes`: not smaller than 640x480, not interlaced, safe
for the hardware, at 24 or more bits, each size once, the largest first, as
Resolution's list). The desktop's own mode, the default, is a borderless
window at the desktop's mode; another mode captures the display and switches
to it, when the game next starts. With Widescreen On the Resolution and Window
Size lists show the modes wider than 3:2 (16:10, 16:9, wider); with it Off,
the 4:3 and 5:4 ones. Halo Custom Edition's own list (all the modes the card
reports, with no Widescreen switch) is from memory and was not checked.

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

`mac.adaptive_detail` (on by default on a PowerPC Mac with the low or medium
preset) keeps a heavy level near 30 frames a second by lowering, one rung at a
time, grass, wide particles, decals, lens flares, small scenery and objects, fog,
model detail, distant bipeds and vehicles and last the frame's resolution while the smoothed frame time is over the target,
and restoring them when it is under; see the settings table. The simulation does
not read any of these, so a game plays the same with it on or off.

Measuring a slow Mac: `HALO_MAC_PHASE_TIMING=<n>` (a hidden `--gpu-render` run, with
`HALO_MAC_RENDER_TIMING=1` for the renderer's buckets) logs, every n frames and for the
whole run, the milliseconds a frame goes to the game's tick, to drawing the world, to
presenting it and to the rest, and the draws a frame. `HALO_MAC_ADAPTIVE_LEVEL=<0 to 12>`
holds the ladder at a rung, `HALO_MAC_ADAPTIVE_MAX_LEVEL=<n>` gives it only its first n rungs. The game keeps the node matrices of objects that nothing moves
(rocks, trees; `HALO_MAC_PERF_NODE_STAMPS=0` computes them every tick as the game does,
`=2` checks every use of a kept one against the computed matrices and reports a count of
differences at the end of a run, which is 0). `HALO_MAC_RENDER_TIMING=2` also times the parts of a
draw call (`dc_attributes`, `dc_vertex_program`, `dc_fragment_program`, `dc_gl_draw`); `=1` is
unchanged. `HALO_MAC_FRAME_SLOW_MS=<n>` adds n milliseconds to each frame of a wall-clock hidden
run, to starve the game of CPU as a slow Mac does. The PowerPC game is compiled with
`-mdynamic-no-pic` (`MAC_PPC_CODEGEN=` restores the old flags), and the collision and update loops
prefetch the next object's memory; neither changes a result.

## Mods

Mods (`docs/mods.md`) work on the Mac: `port_mods.c` is compiled for it, reading files with the C
library in place of SDL. Put a mod's folder in `~/Library/Application Support/OpenCE/mods`, which the launcher creates and
passes to the game as `HALO_MODS_ROOT` (`paths.mods`), so it is the same whichever data root the
player has (the retail install or the Xbox disc's folder) and is always writable. A `mods/` in the
data root, next to `maps/`, that already exists is played while that folder has nothing in it. When mods say they cannot be played
together, a visible run asks before the window opens, in Carbon's standard alert (`platform_ask` in
`platform.c`, 10.4 to 10.6, PowerPC and Intel): play them anyway, turn them off, or quit, with Escape
leaving Quit. A hidden run and the `--gpu-render` checks are not asked and play the mods. Checked on
an Intel Mac, where a mod's bitmap changed the frame of an Xbox Blood Gulch
(`tools/mac_make_test_mod.py` writes a small test mod). On PowerPC, a mod's little-endian text is
swapped to the host's order (`mod_strings.c`) but no PowerPC has run mods, and the alert itself has
not been shown by hand.

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
`--controller-setup` open the two setup windows (the startup dialog has buttons for both). An engine built before
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
    --engine-i386 $OUT/mac_rendered_session_i386 [--version 0.1]
```

`tools/mac_app_bundle.sh` builds the launcher and the two helper apps (when
`--helpers` does not name them), joins the programs with `lipo`, copies the
menus, the icon and `Info.plist`, checks the plist and the architectures,
and makes `build/mac-app/PowerSpark-VERSION.dmg` (`hdiutil`). The icon is
`--icon FILE`, which is not in the repository: PowerSpark's is fan art and is
not the project's to ship. `--branding FILE` (above) is kept out of the repository the same way. Without `--icon`, the app gets OpenCE's own
`port/mac/app/OpenCE.icns`. A `.png` given to `--icon` is
written by `tools/mac_icns.py` (Python 3 with Pillow): the classic icon types
that 10.4 reads (16 to 128 pixels, with 8-bit masks) and JPEG 2000 at 256 and
512 pixels for 10.5 and 10.6. `python3 tools/mac_icns.py --check FILE`
parses a file. Use `nice` on a shared build machine. Do not run the bundle's game on
a machine whose desktop someone else is using.

`tools/mac_rendered_session_checks.sh` also runs `tools/mac_duplicate_symbols.sh` on the objects of the
session link before linking: it runs `nm` over the same object list and names both objects of any symbol
two of them define (common and local symbols and weak definitions are ignored, since the linker merges or
ignores them). A name defined twice was the cause of each link failure from the internet-play patches.

`sh tools/mac_menu_b30_checks.sh` drives the menus with `HALO_MAC_MENU_KEYS` into The Silent Cartographer
(Campaign > Load Level) and checks that the opening plays as it does when the level is started directly (the
cinematic ends at about tick 1100); it passes on Intel (5/0), and the PowerPC run is the one that matters. The
opening Pelican on PowerPC is a fix in progress.

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

## Voice chat microphone

Voice chat's microphone is `mac_voice_capture.c`, a CoreAudio AUHAL input unit
(the HAL output unit with its input bus on; Mac OS X 10.4 and later, PowerPC and Intel).
It reads the device's own rate and channels as native-endian float, averages the
channels to mono and resamples to 48 kHz (`mac_voice_dsp.c`: averaging above the
rate, linear at or below) into a ring that voice chat's thread reads in frames of
960 samples, so the Opus encoder is given the frames of the SDL builds. Nothing
opens the microphone (or makes the system ask for it) until `network_voice.c`
first asks to send: push to talk on the first press, open mic when voice chat
is on; `audio.voice_chat` `"off"` never opens it. `audio.input_device` names the
device (changed, it reopens); a microphone that will not open is tried again every
10 seconds. `sh tools/mac_voice_capture_checks.sh` runs the ring and resampler tests and
builds a hidden probe (`MAC_VOICE_CAPTURE_SECONDS=5` records from the microphone
to a file of float frames). Manual two-machine test: `docs/mac-voice-manual-test.md`.

## Known limitations

- Xbox maps on the PowerPC Macs: the conversion is checked in both byte orders on Intel and
  the maps render on the G4s; long play on a G4 is not yet measured.
- The graphics check: when the window's GL context is made the game logs one line, `native GL capabilities:`
  (the card, `GL_VERSION`, the GLSL version, the video memory, which of framebuffer objects, packed depth
  stencil, non-power-of-two and rectangle textures and the ARB programs the driver has, and which GLSL it will
  be given). A context without OpenGL 2.0, the shading language 1.10 or framebuffer objects ends the program
  there, with a Mac alert naming the card and what it lacks, and removes the 20 MB state file the start had
  made. `HALO_MAC_GL_FORCE_CAPS_FAIL=1` pretends one is missing, to see that path (a hidden run only logs
  it and exits with status 1).
- GLSL 1.10 (Mac OS X 10.4's language): when the driver's shading language is below 1.20 the shaders are given
  `#version 110`; the generated shaders use nothing that 1.20 added, so the text is otherwise the 1.20 text,
  byte for byte (`mac_glsl120.c`). `HALO_MAC_GLSL_VERSION=110` makes it so on any driver, which is how it was
  checked on 10.6: the five hidden runs (Blood Gulch, menu, ui-menu, Xbox ui, b30) give the same counts as with
  1.20. `HALO_MAC_GLSL_DUMP=<existing folder>` writes every shader text once, and
  `sh tools/mac_glsl_validate.sh <folder> 110` checks them with `glslangValidator` (161 shaders from those runs,
  with the ARB path's pixel shaders, have no errors as 1.10). A report from a 10.4 Mac wants
  `HALO_MAC_GL_DIAGNOSTICS=1 /Applications/PowerSpark.app/Contents/MacOS/PowerSpark 2>&1 | tee ~/Desktop/tiger.log`
  from Terminal (the launcher starts the game with that environment; the log has the capabilities line, the
  limits, and every shader compile or link message).
- Render targets (the water, shadows, the screen copies) keep a depth 24 / stencil 8 buffer when the driver has
  `GL_EXT_packed_depth_stencil`, and a depth 24 buffer without a stencil attachment when it does not (logged once);
  `HALO_MAC_NO_PACKED_DEPTH_STENCIL=1` takes that path on any driver (the five hidden runs give the same counts and
  Blood Gulch the same frame). The screen's own buffers come from the window's pixel format and are not touched.
- No Mac 10.4 machine has run the game; the 10.5 and 10.6 results are from
  three machines. The Controller Setup needs 10.5.
- System link (LAN) multiplayer works from the menus (Multiplayer > Create
  Game > LAN, Join Game > LAN; see "System link" above). Voice chat sends and receives
  (below); internet play (its settings rows are shown, and the launcher opens `halo://` invite links into
  `join_link.txt`), map sharing and Custom Edition maps are not proven or not available on the Mac yet.
- Full screen switches the display to 640 x 480 and stretches the picture;
  the menus' pointer is not tracked in full screen.
- Audio is OpenAL (`HALO_MAC_AUDIO_OPENAL=0` selects the software mixer). Music
  and loops that are Ogg Vorbis in MacSoft's data are converted in the
  background.
- The developer console opens with the backquote key in a game (Escape or the backquote closes it; the
  game stands still while it is open), and `debug.telnet_console` listens on `127.0.0.1` for the script
  console, as on the other builds.
- Per-pixel lighting (`display.per_pixel_lighting`) is not drawn: the Mac's shader translator does not make it
  yet. The start window shows its row disabled ("not available") and the log says so when the setting is on.
- Controller rumble works for a DualShock 4 or DualSense on USB (its output report, written through the HID
  interface), and for one over Bluetooth through PowerSpark's own stack (`input.bluetooth`). Xbox One pads (their
  driver is a separate package) have none yet; the Settings menu's vibration row does nothing for them. Not yet
  tried on a pad.
- The Bluetooth stack is experimental and has not been run with the game against the real dongle and pad.
- The app is not signed or notarized (10.4 to 10.6 do not need it).
- `HALO_MAC_*` variables beyond those above are diagnostics for the port's
  tests and may change.
