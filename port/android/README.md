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

On Windows, build in WSL2 (Ubuntu 24.04). The Android build does not run
on Windows itself. Enter
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
3. Push the button. The file picker of the system opens.
4. Select the disc image.
5. Wait while the app extracts the data (approximately 1.8 GB). Then the
   game starts.
6. You can delete the disc image.

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
button. A Bluetooth or USB keyboard operates as on Linux.

The touchscreen operates the menus: tap an item to select it (on a
setting with values, tap its left or right half), tap a button of the key
at the bottom of a screen (for example "B = Back") to push it, and drag to
scroll a list (down or right steps back, up or left steps forward). A drag
stops at the first and the last item, and it does not change a setting's
value. Touches that start in the edge-gesture zones of Android do not tap
or scroll at the sides, and do not scroll at the top and bottom, because the
first swipe from an edge in full screen only shows the system bars.

A tap during a cinematic that can be skipped skips it, as A does. On the
on-screen keyboard, tap a key to press it, "B =BACK" to cancel and
"A =ENTER" to accept the name.

### Touch controls

In a game, the app shows touch controls over the picture. They are a
controller for player 1. With the default buttons of the profile:

| Control | Function in the game |
| --- | --- |
| stick (lower left) | move |
| swipe on the screen away from the buttons | look |
| Fire | right trigger |
| Grenade | left trigger |
| A / Jump, B / Melee, X / Reload, Y / Weapon | A, B, X, Y |
| Crouch, Zoom | left and right stick clicks |
| Light, Gren. type | white, black |
| Pause, Back | start, back |
| Up, Down, Left, Right | D-pad |

The names on the buttons follow the profile's "Button layout" (Settings >
Gamepads): with "Swap triggers", the right trigger's button says "Grenade".
The stick moves the player with every "Stick layout", southpaw too.

A finger that holds a button can also swipe to look, so you can fire and
aim with one thumb. The controls are a finger wide (48 dp) or larger, but
on a small screen of high density they stay apart rather than reach that
size. A short tap reaches the game even when it is shorter than one frame.
Swipes that start in the edge-gesture zones of Android do not turn the
view. The swipe aims as the controller's stick does, not as a mouse: the
aim slows over a target and follows a moving one, as with a controller,
and it follows the profile's "invert look" (the gyroscope turns the view
as the phone turns, never inverted). The setting `input.touch_aim_assist`
turns the aim assist off. "Look sensitivity" sets how far a swipe turns;
the mouse settings do not apply.

The touch controls show only in a game. In the menus and during
cinematics they hide, and the touchscreen operates the menus as described
above. They also hide when a controller is connected, for example the
built-in controller of a handheld. A device without a touchscreen (a TV)
never shows them. The setting `input.touch_controls` changes this (refer
to "Settings").

The buttons at the top of the screen:

- "Hide" removes the controls (and stops the gyroscope aiming and the
  vibration) until you push "Touch"; the app remembers it.
- "Options" opens these items:
  - "General": the phone's vibration (on by default; it follows the
    vibration setting of the game's profile), aiming with the gyroscope
    (off by default), a floating move stick (off by default: the stick goes
    where your thumb lands in the lower left of the screen), the opacity of
    the controls, "Hide or add buttons" (hide a button, add a copy of a
    button) and "Edit buttons size".
  - "Edit buttons layout": drag the controls to new positions, then push
    "Save". "Export" and "Import" write and read a layout file with the
    file picker of the system.
  - "Look sensitivity".

"Hide" and "Options" act when the finger lifts on them: a swipe that
starts on them turns the view instead.

The app keeps the layout in its own preferences, not in `config.toml`.
Removing the app's data or the app removes the layout.

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
| `input.touch_aim_assist` | `true` (the default): the touch controls' swipe aiming gets the aim assist of a controller (the aim slows over a target and follows a moving one). `false`: none, as with a mouse; the bullets' own autoaim stays. |
| `input.touch_controls` | The touch controls in a game. `"auto"` (the default): shown on a touchscreen while no controller is connected. `"on"`: also shown with a controller. `"off"`: never shown. A device without a touchscreen never shows them. The menus take taps in any case. |
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
  (`host/host_sdl.c`), OpenGL ES (`host/host_gl.c`), and the file and socket
  functions of `port/linux/src/posix_*.c`.

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
- Touch operates the menus and skips cinematics. In the game, the touch
  controls appear when no controller is connected; their size follows the
  height of the screen (larger on a tablet than on a phone), never smaller
  than a finger.
- Kernels with 16 KB pages (a developer option of Android 15) do not
  operate. The Xbox memory uses 4 KB pages.
- The x86 Android emulator runs the app through its ARM translation. The
  translation cannot deliver the page faults that the renderer uses to
  notice changed textures and vertices. On the emulator the app compares
  page contents once a frame instead ("write tracking: page hashes" in the
  log). This is slower, and a write shows one frame later, so moving
  geometry can glitch briefly. `debug.memory_watch = false` does the same
  on a device.
