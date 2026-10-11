# Android

`ninja android` builds the game for 64-bit ARM Android (arm64-v8a).
`ninja android_apk` makes an app from it:
`port/android/app/build/outputs/apk/debug/app-debug.apk`.

The game shows its graphics with OpenGL ES 3. It plays sound through SDL3
(AAudio). It accepts input from game controllers, for example a PlayStation
5 DualSense on Bluetooth, and from the touchscreen (refer to "Controls").
The app needs Android 9 (API 28) or later. It operates on 64-bit-only
devices, for example the Pixel 9 Pro XL.

The Android build uses the platform layer of the Linux build
(`port/linux/src`). Refer to [port/linux/README.md](../linux/README.md).
Physical mice use the same control bindings. During play, F12 toggles mouse
capture; captured mode hides Android's pointer and uses relative movement for
continuous aiming, while releasing capture restores normal pointer behavior.

## Requirements

You do not need the Xbox SDK. You need the tools of the Linux build (Python,
ninja) and these items:

- A clang with the `arm64_32` target, for example the clang of the system.
  The option `--android-guest-cc` of `configure.py` selects a different
  compiler.
- The Android NDK. `configure.py` looks for it in `ANDROID_NDK_HOME`, then
  in `$ANDROID_HOME/ndk`, `~/Android/Sdk/ndk` and `/opt/android-sdk/ndk`.
  The option `--android-ndk` selects a different NDK.
- CMake, and a JDK 17 or later for Gradle.
- A network connection for the first build. `configure.py` downloads musl
  1.2.5 and SDL 3.4.16 to `build/android/third_party`. Gradle downloads the
  Android Gradle Plugin.

On Windows, build in WSL2 (Ubuntu 24.04), or on Windows itself with Git for
Windows: its Bash runs the build's commands and the Windows NDK's clang
builds the guest and the host (`tools/android_build.py`,
`WindowsAndroidWriter`). Set `ANDROID_HOME` to the SDK folder, with the NDK
and Android API 35, then enter `python configure.py` and `ninja android_apk`
as on Linux. For WSL, enter
`wsl -d Ubuntu -- bash tools/wsl_setup_android.sh` one time: it installs
the tools and the Android SDK and NDK in WSL (apt asks for your
password). Then enter
`wsl -d Ubuntu -- bash tools/wsl_build_android.sh` for each build. The
script copies the repository into WSL (`~/halo-build/src`) with LF line
endings (a Windows checkout has CRLF line endings, which the build's text
tools do not accept), builds there, and puts the APK in
`dist/android/app-debug.apk`. Only the changed files are copied, so the
next build is incremental. Install the APK with the adb of Windows.

A build that you make yourself has a different signature from the builds
of GitHub Actions. Android installs it only after you remove the app, and
removing the app deletes its data folder (`maps/`, `save/` and
`config.toml`). Thus a change between the builds of GitHub Actions and your
own builds loses the saved games, unless you copy them first (below).
Your own builds all have the same signature (the debug key of WSL,
`~/.android`), so they install over each other and keep the data
(`adb install -r`).

To keep the saved games before you remove the app, enter
`adb pull /sdcard/Android/data/com.halo.decomp/files/save/u` (the profiles).
After the new install, before the first start of the app, enter
`adb push u /sdcard/Android/data/com.halo.decomp/files/save/u`. The app reads
and overwrites what adb pushes. adb cannot add files to `save/u` or `save/z` once the app made them, or
change files that the app wrote, so push before the first start. adb cannot read
some files that the app writes (the `blam.lst` files of `save/z`), so `adb pull`
of the whole `save` folder stops there.

## Build and install the app

1. Go to the root folder of the repository.
2. Enter `python configure.py`.
3. Enter `ninja android_apk`.
4. Connect the device with adb.
5. Enter `adb install -r port/android/app/build/outputs/apk/debug/app-debug.apk`.

`ninja android` builds only the game image and the native libraries.

## Game data

The game needs the `maps/` folder from an Xbox disc image (`.xiso` or
`.iso`) of any version of the game. The app extracts `maps/` from the disc
image. The app keeps the data in `/sdcard/Android/data/com.halo.decomp/files`.

To install the data with the app:

1. Copy the disc image to the phone.
2. Start the app.
3. Push **Choose disc image**. The file picker of the system opens.
4. Select the disc image.
5. Wait while the app extracts the data (approximately 1.8 GB). Then the
   game starts.
6. You can delete the disc image.

Alternatively, choose **Choose extracted maps** and select the folder that
contains `ui.map` and the other extracted map files, or its parent folder
with a `maps` subfolder. The system folder picker grants read access; the
app copies the files into its own storage. An invalid or failed import
leaves the existing maps in place. The source folder is never modified.

To install the data from a computer:

1. Start the app one time. The app makes its folders.
2. Enter `adb push <folder>/. /sdcard/Android/data/com.halo.decomp/files/`.

| Item | Location in `/sdcard/Android/data/com.halo.decomp/files` |
| --- | --- |
| Saved games (`z:\` and `u:\`) | `save` |
| Log | `debug.txt` |
| Settings | `config.toml` |

To make a copy of the saved games, enter
`adb pull /sdcard/Android/data/com.halo.decomp/files/save/u` (the profiles).
An `adb pull` of the whole `save` folder stops at the `blam.lst` files of
`save/z`, which adb cannot read.

The main menu's build label shows the installed app's version, read from the
package at every start: a release's name (`build 186`), or another build's
version name and code (`v1.0 (1)`).
Tap it to open https://github.com/cybersecurity/halo-ce-universal.

## Controls

The game reads controllers through the gamepad functions of SDL3. All the
controllers that Android knows operate. The first controller is player 1.
The other controllers are players 2 to 4 (split screen). The buttons agree
with the positions on the Xbox controller:

| DualSense | Xbox | Function in the game |
| --- | --- | --- |
| left stick, right stick | left stick, right stick | move, look |
| R2 | right trigger | fire |
| L2 | left trigger | throw a grenade |
| Cross | A | jump, accept |
| Circle | B | melee, back |
| Square | X | action, reload |
| Triangle | Y | change the weapon |
| L1 | white | flashlight |
| R1 | black | change the grenade |
| L3, R3 | left and right stick clicks | crouch, zoom |
| D-pad | D-pad | |
| Options | start | pause menu |
| Create | back | |

The controller gets the rumble. The back gesture of Android is the B
button. Bluetooth and USB keyboards also work. The screen
accepts multi-touch input through an on-screen controller for player 1.
The left stick moves; drag anywhere outside it to look around. Looking uses
relative finger displacement directly, with no right stick acceleration.
Action buttons stay held while dragging, so either Fire button can also aim.
A second Fire button on the left lets you shoot while aiming with the right hand.
Menus accept direct taps on the rendered items, including the in-game
keyboard used for names. The game maps touch positions through the same
letterboxing and widescreen centering as the renderer. Android Back returns
to the previous screen. Taps on empty left/right screen edges select the
previous/next item using the current list's direction, including mission
selection and option lists. **Touch/Hide** remains visible in menus and gameplay unless
**Disable all overlay** is enabled. Menus start with the controller hidden; **Touch** restores its
buttons and movement stick for screens that need them. Swipe aiming, phone
rumble and gyro aim stop while a game menu is open.

### Porting options

The main menu replaces **Quit** with **Porting options** in the original
list position, using the native menu font, size and blue color. Android Back
at the main screen no longer opens the Quit dialog. The pause
menu also adds a single **Porting options** entry containing **Overlay
settings**, **General** and **Cheats**. Its frame, original text and touch
targets are enlarged by 12%. These screens are rendered by Halo,
using the menu font and colors from the loaded map's widget definitions.
The engine extends `ui.map` and the level pause UI at runtime; imported map
files do not need to be replaced or modified.

Porting screens use a full-screen native blue UI bitmap and text enlarged to 160% of the
map's large UI font, with 60-pixel touch rows in the 640x480 UI space.
Lists show every setting in a vertically scrollable viewport: swipe up/down
or drag the scrollbar on the right. **Back** stays below the list. Page
titles are cyan to distinguish them from the option labels. Standalone
labels use an explicit alpha so main-menu tags whose
original text alpha is zero remain visible outside their plasma pass.
Labels use the original text-box renderer with an independent visible instance;
scaling is scoped to glyph vertices and resets after each label. The blue UI
bitmap is selected from the current map and fills the entire widescreen area.

- **Overlay settings** contains **Edit buttons layout**, **Hide or add
  buttons**, **Edit buttons size**, **Look sensitivity** and **Disable all overlay**. The
  global toggle also hides Hide/Touch and FPS and releases held inputs. Direct
  menu taps and physical controllers still work. The default positions and
  sizes match the supplied October 3 layout; Reset restores them. Layout editing
  shows the controls for dragging while keeping the current game menu open
  and the campaign paused. Tap **Save and exit** to return to Overlay settings.
  **Export** and **Import** retain Android's document picker. The button
  manager lets you hide/show, duplicate and add buttons, including restoring
  the movement stick. Reset requires confirmation. Size sliders range from
  50% to 200%; look sensitivity ranges from 0.25x to 4x. All controls,
  including copies, can be reached by scrolling without Previous/Next pages.
- **General** contains **Rumble**, **Gyroscope**, **Gyroscope sensitivity**,
  **FPS counter**, **Field of view (FOV)** and **Edit in-game UI** (below). FOV uses a 55–90 degree slider
  with a 70 degree default, preserves weapon zoom and is saved for both menus.
  Unsupported hardware is marked unavailable. Rumble
  defaults on, gyro and FPS default off. Gyro sensitivity has its own
  horizontal slider from 0.25x to 4x, independent of swipe look sensitivity.
  Gyro handles both landscape orientations. FPS measures presented game
  frames over half-second intervals and displays during gameplay.
- **Cheats** in the main menu selects startup cheats in a dedicated `init.txt`
  block, with all sixteen disabled initially and other file contents preserved.
  Flags apply after player spawn; instant actions run once per map (teleport
  waits for a valid camera). The pause menu retains all ten live engine flags
  and six instant actions and never edits the startup file. Enabled
  flags have green rows; instant actions remain repeatable. Requests and
  displayed states follow the existing engine acknowledgement path. Cheats
  require an active player; network clients retain the host's existing rules.
  Available spawned objects depend on the current map.

All mobile controls now use original cyan vector HUD icons instead of text.
**Camera mode** switches between first-person, flying and following cameras
with one tap, following the engine's existing host/client camera rules.
It can be moved, resized, hidden and duplicated in Overlay settings from
both main and pause menus. **Gren. type** keeps its original hold behavior.
In flying mode, **Zoom** toggles player/camera control; swipe and gyro aiming
rotate the flying camera while its controls are active, and the movement
stick translates it at about 8 metres per second at full deflection.
Zoom switches once per press, including when held across several frames.
Swipe aiming uses the same **Look sensitivity** setting as the player camera
(default **2x** for new configurations; existing preferences are preserved).
The sensitivity slider can be dragged beyond either edge to reach its full
0.25x–4x range. Fire/Grenade control vertical camera movement.

Version 4 `.halolayout` files include the Camera mode control. Imports of
versions 1–3 insert it without losing existing positions or button copies.
These files also preserve gyro sensitivity and the FPS
preference, FOV and Disable all overlay. Missing new settings use 70 degrees
and enabled overlays. Startup cheats are separate from layout exports. Version 1 and 2 layouts remain supported; their gyro sensitivity
defaults to 1x and FPS to off. Version 1 retains its default button sizes,
Rumble on and Gyroscope off. Clearing app data clears the saved settings.

Validation: `python tools/test_touch_layout.py` checks persistence, legacy
imports, invalid settings and gyro integration/orientation. `ninja android`
and `ninja android_apk` compile the native renderer/input bridge and app.
`python tools/test_menu_touch.py` executes the actual Java gesture handlers
with recording inputs to check Touch/Hide, menu button release, direct taps,
swipe suppression and slider dragging across setting acknowledgements.
Visual layout, taps on imported map variants, phone rumble and sensor
response must also be checked on an Android device.
`python tools/test_camera_touch.py` executes the actual director input function
to check tap switching, held-grenade compatibility, Zoom control ownership,
flying-camera swipe and movement. `python tools/test_porting_ui.py` executes the actual text-box/porting functions
with a recording renderer to check zero-alpha/faded template visibility,
enlarged row dispatch and full-screen native bitmap selection. It requires
the Windows Android NDK and Node; it does not validate GPU presentation.

### Flying camera

Flying-camera control
uses the same touch/gyro look reader as the player camera. While free-camera
controls are selected, the earlier player-input pass leaves the motion
accumulator intact for the director, so every swipe reaches the camera.

### Porting options: the menus' own look

**PORTING OPTIONS** is written in capitals in both menus:

- In the main menu the other items are pictures of text, not text. The entry
  is now a picture made the same way (`tools/porting_assets.py`): the same
  typeface (OpenCE), size, half-transparent blue and, when selected, the same
  white text with a blue glow. The game's own widget draws and animates it in
  Quit's place; the picture is twice as wide as the other items' for the longer
  text. With `display.menus = "xbox"` there is no such picture, and the entry
  is drawn as text as before.
- In the pause menu the entry has the font, colour, justification and 112%
  size of the menu's own items, and its text sits in its row where theirs does.

Every title and row of the Porting options screens is in capitals. Opened from
the main menu they are set in the main menu's typeface, one picture a
character (`pc\porting\font`, tinted as it is drawn, made smaller when a row
would not fit); opened from the pause menu, in the pause menu's own font. The
layout editor's labels are capitals too.

**General > Edit in-game UI** moves the game's own HUD. The editor shows three
panels to drag: **RADAR** (the motion sensor), **WEAPONS** (the panel at the
top left) and **HEALTH** (the panel at the top right). **SAVE** keeps them and
returns to General, **RESET** puts them back, and Android Back saves too. The
game moves everything it anchors at that corner of the screen by the same
distance, so a panel moves whole; the HUD's messages stay. A panel cannot
leave the screen. The places are fractions of the screen, kept in the app's
preferences (`hud-layout`) and applied again at every start. They are not part
of a `.halolayout` file: **Export** and **Import** leave them alone. A split
screen keeps the game's own layout.

Validation: `python tools/test_touch_layout.py` also checks the panels' rest
places, dragging, screen limits, saving and that an export holds no HUD.
`python tools/test_hud_layout_engine.py` runs the game's actual HUD placement
with moved panels, `python tools/test_porting_ui.py` the main menu's picture
swap, the pause menu's style and the typeface's layout. The engine tests need
the Windows Android NDK and Node, or Edge or Chrome when Node is not installed
(`tools/wasm_test.py`). How the menus and the moved HUD look must still be
checked on a device.

## Settings

The settings are in `config.toml` in the data folder of the app. To change
them:

1. Enter `adb pull /sdcard/Android/data/com.halo.decomp/files/config.toml`.
2. Change the file.
3. Enter `adb push config.toml /sdcard/Android/data/com.halo.decomp/files/`.

At the first start, the game writes the file with the default values. To
get the default values again, delete the file.

The settings are the settings of Linux, without the window, the mouse and
the paths. Refer to [port/linux/README.md](../linux/README.md#settings).
These settings are only for Android:

| Setting | Function |
| --- | --- |
| `display.screen_width` | The number of columns of the 480-line picture. `0` (the default): the shape of the display (1068 on a 20:9 phone). `640`: the 4:3 shape of the Xbox. |
| `debug.sample_seconds` | Refer to "Find problems". |
| `debug.memory_watch` | `true` (the default): the app notices the game's writes to textures and vertices by page protection. `false`: it compares page contents once a frame instead, which is slower. Refer to "Limits". |

## Internet play

Internet play operates as on Linux, but without Discord. When the game
hosts a system link game, it puts the invite link on the clipboard and
shows a notice.

To join a game, do one of these steps:

- Open the link. The app is the handler of `halo://join/...` links. If the
  game does not operate, the app starts it. The app writes the link to
  `files/join_link.txt`, and the game reads it.
- Copy the link and go to the game.

On the local network:

- The game uses the address of the Wi-Fi (or of the hotspot of the
  phone), not the address of the mobile data.
- The app holds a Wi-Fi multicast lock while the game operates. Some
  phones otherwise drop the broadcasts that find system link games.

Keep the game in the front during a network game. When the app goes to the
background, Android stops the game. After 15 seconds the other machines
drop it, and when it hosts, its players leave.

## Updates

The app from GitHub Actions can update itself, as on Linux (refer to
"Updates" in [port/linux/README.md](../linux/README.md#updates)). When you
select "Yes":

1. The app downloads the new version.
2. The package installer of Android opens. At the first update, Android asks
   you to let Halo install apps. Allow it.
3. Select "Update". Android replaces the app.
4. Select "Open" to start the new version.

To install over the previous version, each build must have the same
signature. GitHub Actions signs each build with the key in the
`ANDROID_KEYSTORE_BASE64` and `ANDROID_KEYSTORE_PASSWORD` secrets of the
repository. If you installed a build that has a different signature, remove
that build before you install a new build. Removing the app deletes its data
folder: first make a copy of `maps/` and `save/u`.

## Widescreen

The game shows 480 lines in the shape of the display, not the 640x480 of
the Xbox:

- The 3D view is wider. The camera keeps the vertical field of view.
- The HUD stays at the edges of the screen.
- The menus, the loading bar and the screens after a game have 640
  columns, at the center of the screen.
- Black bars and fades cover all of the screen, and so do the menus' dims
  and backgrounds (the pause menu's dim, dialogs, the menus' gradient).

All the ports draw this way.

## How the port operates

### ILP32 code

The data of the game contains 32-bit pointers. The cache files have the
layout of the Xbox memory. The saved games are copies of the memory.
Direct3D resources contain 32-bit physical addresses. Current Android
devices cannot execute 32-bit ARM code. 64-bit pointers change the layout of
the structures that the game reads from its files.

Thus the game is ILP32 AArch64 code: 64-bit ARM instructions with 32-bit
`int`, `long` and pointers, in a 64-bit app.

### The guest image

The guest is the game, the platform layer and a small runtime:

1. clang compiles the guest for `arm64_32-apple-watchos`, the only ILP32
   AArch64 target of clang. The options `-U__APPLE__` and
   `-fno-define-target-os-macros` hide the Darwin environment.
2. `tools/android_asm_convert.py` changes the Mach-O assembly to ELF
   assembly.
3. The AArch64 assembler makes the objects.
4. `ld.lld` links the objects with `guest/guest.ld` to a static image,
   `build/android/halo_guest.elf`, at a fixed address above the Xbox memory
   (`include/halo_android_abi.h`).

The APK contains the image as an asset.

The C library of the guest is a part of musl for a new `arm64_32`
architecture (`guest/libc/arch/arm64_32`). It has ILP32 types and a 32-bit
`time_t`, as in the MSVC runtime of the game. Its system calls go to the
host (`syscall_arch.h`). `guest/runtime/guest_thread.c` makes the threads
and supplies the thread pointer and TLS.

### The host library

`libmain.so` is an arm64 NDK library. The activity of SDL3 starts it
(`app/.../HaloActivity.java`). The host library:

- Reserves the address space of the guest below 4 GB: the Xbox memory at
  `0x80000000`, the image, and pools for the memory of the guest
  (`host/host_memory.c`). Refer to "The fixed addresses".
- Loads the image and fills its import table (`host/host_loader.c`).
- Starts the `main` of the game and each guest thread on a stack in guest
  memory, because ILP32 code keeps stack addresses in 32-bit registers
  (`host/host_thread.c`).
- Gives the audio callback of SDL to a thread with a guest stack
  (`host/host_sdl.c`).
- Does the calls of the guest: system calls (`host/host_syscall.c`), SDL
  (`host/host_sdl.c`), OpenGL ES (`host/host_gl.c`), and the file, socket,
  UPnP and DTLS functions of `port/linux/src/posix_*.c` (DTLS with Mbed TLS,
  for internet play with browsers: `posix_dtls.c`).

The guest calls the host through stubs (`tools/android_imports.py`). The
two ABIs use the same registers for 32-bit integers, floats and pointers.
`tools/android_gl_stubs.py` makes the OpenGL ES stubs from
`port/linux/src/gl.h`. `tools/android_posix_stubs.py` makes the stubs of the
`posix_*` functions, which copy the `errno` of the host.

### The fixed addresses

The guest needs the Xbox memory at `0x80000000` to `0x88000000` and the
image above it, to `0x8c000000`. The cache files contain pointers to these
addresses. The Java runtime of Android (ART) also reserves its spaces below
4 GB. On some devices, for example handhelds with a large Java heap (the
AYN Thor, the Retroid Pocket), ART's large object space covers
`0x80000000`. ART fills that space from its bottom, so the part at
`0x80000000` is usually empty.

Thus:

1. The game operates in a process of its own (`:game`), with a fresh Java
   heap. The launcher, the import of a disc image and an earlier game do
   not leave objects there.
2. At the start of that process, `HaloApplication` loads `libmain.so`. Its
   `JNI_OnLoad` reserves the fixed addresses before the Java side
   allocates large objects.
3. If ART's large object space is in the way, the host takes back only the
   part that covers the fixed addresses, and only if no page of it is in
   use (`/proc/self/pagemap`, or `mincore` and the swap of that part if
   the device refuses `pagemap`). The host never takes the other spaces of
   ART.

If the addresses are not available, the game shows a message, and writes
the mappings below 4 GB to `memory_map.txt` in the data folder and to the
log.

To test the reclaim on any device, set a system property before you start
the game. The app then puts a stand-in for ART's large object space over
the fixed addresses:

- `adb shell setprop debug.halo.art_overlap idle`: the stand-in is empty
  at `0x80000000`. The log shows `reclaimed idle ART range`, and the game
  starts.
- `adb shell setprop debug.halo.art_overlap busy`: a page at `0x80100000`
  is in use. The game shows the message and writes `memory_map.txt`.
- Add `-nopagemap` to either value (`idle-nopagemap`) to test as on a
  device that refuses `/proc/self/pagemap`. Add `-swapped`
  (`idle-nopagemap-swapped`) to also move the objects of the stand-in to
  swap; with `busy`, the page in the fixed addresses too.
- `adb shell setprop debug.halo.art_overlap ""`: normal operation.

### OpenGL ES

The renderer (`port/linux/src/d3d8_gl.c`) uses OpenGL ES 3.0, and some
functions of OpenGL ES 3.2 if they are available:

- The vertex shaders flip y and change the depth range from 0..1. The front
  face winding is inverted.
- BGRA textures go to the GPU as RGBA with a swizzle. If the driver has no
  S3TC (Mali GPUs), the CPU decodes the DXT textures.
- The pixel shaders apply the LOD bias of the sampler.
- The upload changes the byte order of `D3DCOLOR` vertex attributes.
- Dynamic vertex and index data goes into a ring of three buffers, one for
  each frame. On Mali, other methods used too much memory.
- On OpenGL ES 3.2, indexed draws use a base vertex. Before 3.2, the CPU
  changes the indices.
- On OpenGL ES 3.1 and later, the visibility tests (lens flares) count
  samples with an atomic counter, as the NV2A did. OpenGL ES 3.0 tells only
  if a sample is visible. The GPU copies the counters at the end of each
  frame, and the CPU reads the copy two frames later, when the frame's fence
  has passed: a result is the latest count the GPU has finished, as with
  the query buffer of desktop OpenGL. A read of the counters themselves
  waits for the GPU, which halved the frame rate on Turnip (Zink).

### Calling conventions

Some files of the game declare a function differently from its definition,
or call a function without a prototype. On 32-bit x86, this has no effect.
The guest ABI passes floating-point arguments in their own registers and
variadic arguments on the stack. Thus such a call gives incorrect values.

`tools/android_abi_check.py` compares each declaration with its definition
in the LLVM IR. The problems are repaired:

- `hs.c` declared the red component of the script fades as `long`.
- Some files call `error`, `console_printf` or `terminal_printf` without a
  prototype. `include/halo_android_variadic_prototypes.h` gives the
  prototypes.

`guest/runtime/guest_misc.c` supplies the Darwin library functions that the
target calls (`__sincos_stret`, `__exp10f`). The guest compiles without
floating-point contraction, as on x86.

### Game source changes

The port's code tells three things apart, each its own macro, which the
Android build defines all of (`tools/android_build.py`):

- `HALO_ARM64_GUEST`: the guest's ABI (ILP32 AArch64 code in a 64-bit
  process);
- `HALO_GLES`: the OpenGL ES renderer;
- `HALO_ANDROID`: the app (its display, input, files and lifecycle).

The x86 inline assembly is replaced by C (refer to
[port/linux/README.md](../linux/README.md#game-source-changes)).
These changes are in `#ifdef HALO_ARM64_GUEST`:

- Seven `#pragma bss_seg(".bss")` lines are removed. The Darwin target does
  not accept them.
- A stack walker follows the AArch64 frame records. Thus the log of an
  assertion (`debug.txt`) shows the call sites.

The musl of the guest uses its C math, not the AArch64 assembly. The only
assembly of the port is necessary:

- The import stubs. A 32-bit guest cannot keep or go to a 64-bit host
  address.
- The symbol aliases in `guest/libc/src_include/features.h`. The Darwin
  target does not accept alias attributes.

## Find problems

- Enter `adb logcat -s halo` to see the log of the port and the errors of
  the game. `files/debug.txt` is the log of the game.
- If the guest code stops, the log shows the registers and the frame chain.
  To find the functions, enter
  `llvm-symbolizer --obj=build/android/halo_guest.elf <address>`.
- Set `sample_seconds = <seconds>` in `[debug]` of `config.toml`. The log
  then shows the program counter and the frame chain of each guest thread
  at this interval. This finds hangs on devices without root access.
- Set `gl_debug = true` in `[debug]` of `config.toml`. The log then shows
  the OpenGL ES errors.
- The log says which write tracking the app uses: "page protection"
  (devices) or "page hashes" (the emulator, or `debug.memory_watch = false`).

## Limits

- Bink video is not available. The game skips the movies.
- The device must let the app reserve the fixed guest addresses, from
  `0x80000000` to `0x8c000000`. If ART uses them, the app shows a message
  (refer to "The fixed addresses").
- Kernels with 16 KB pages (a developer option of Android 15) do not
  operate. The Xbox memory uses 4 KB pages.
- The x86 Android emulator runs the app through its ARM translation. The
  translation cannot deliver the page faults that the renderer uses to
  notice changed textures and vertices. On the emulator the app compares
  page contents once a frame instead ("write tracking: page hashes" in the
  log). This is slower, and a write shows one frame later, so moving
  geometry can glitch briefly. `debug.memory_watch = false` does the same
  on a device.
