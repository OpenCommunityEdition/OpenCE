# macOS

`ninja macos` builds the game for Macs with Apple silicon (M1 and later),
as native arm64 code. `ninja macos_app` makes an application from it:
`build/macos/Halo.app`. `ninja macos_x86_64` builds the game for Intel Macs
(it also runs on Apple silicon through Rosetta 2).

The game draws with OpenGL ES 3 through ANGLE on Metal. It plays sound
through SDL3. It accepts a keyboard and mouse, and game controllers (Xbox,
PlayStation, Switch Pro and other controllers that macOS knows). The game
needs macOS 14.4 or later.

## Requirements

- Xcode or the Xcode Command Line Tools (clang, lipo, codesign).
- Python 3.9 or later, and these packages: `pip3 install --user ninja cmake ziglang`.
  The `ziglang` package supplies `ld.lld` and `llvm-ar`, which link the
  game's image. An `ld.lld` and `llvm-ar` on the `PATH` operate too.
- ANGLE, universal (x86_64 and arm64): `libEGL.dylib` and
  `libGLESv2.dylib`. `configure.py` takes them from Steam's or Google
  Chrome's Chromium Embedded Framework if one is installed. The option
  `--macos-angle <folder>` selects a different folder.
- A network connection for the first build. `configure.py` downloads musl
  1.2.5 (its SHA-256 is checked), SDL 3.4.16 (the release commit is
  checked) and the Khronos OpenGL ES headers to `build/macos/third_party`.

## Build and start the game

1. Go to the root folder of the repository.
2. Enter `python3 configure.py`.
3. Enter `ninja macos_app`.
4. Open `build/macos/Halo.app`, or enter `open build/macos/Halo.app`.

`ninja macos` builds the same game without the application bundle, in
`build/macos/Halo`. Start it with `build/macos/Halo/halo`.

## Game data

The game needs the `maps/` folder of the Xbox game (not the Windows game).
Make a disc image (`.iso` or `.xiso`) of your own Xbox disc of Halo: Combat
Evolved. A PC DVD drive cannot read an Xbox game disc: use a modified Xbox,
or an Xbox 360 DVD drive with the Kreon firmware, and a disc image tool
such as extract-xiso.

1. Start the game.
2. At the first start, the game asks for the disc image. Select it.
3. The game extracts `maps/` (approximately 2 GB). Then the game starts.

| Build | Location of `maps/`, `config.toml`, `debug.txt` and `host.txt` |
| --- | --- |
| `Halo.app` | `~/Library/Application Support/Halo` |
| `build/macos/Halo/halo` | `build/macos/Halo`, next to the executable |

The saved games are in `~/Library/Application Support/Halo` for both.
`paths.data` and `paths.saves` in `config.toml` select other folders.

## Controls and settings

The controls and most of the settings are those of the Linux build. Refer
to [port/linux/README.md](../linux/README.md). The keys that the macOS
build adds:

| Key | Function |
| --- | --- |
| F8 | Change the resolution: native, 2160p, 1440p, 1080p, 720p, then the Xbox's 640x480. |
| F9 | Switch the ray-traced lighting on or off. |
| F11 | Switch between fullscreen and a window. |
| F12 | Release or capture the mouse. |

These settings are new, or have a different default on macOS:

| Setting | Default on macOS | Function |
| --- | --- | --- |
| `display.resolution` | `"native"` | The picture's pixels. `"native"`: the display's in fullscreen, the window's in a window. `"720p"`, `"1080p"`, `"1440p"`, `"2160p"`: that many lines, in the shape of the display or window. `"<width>x<height>"`: that picture. `"xbox"`: 640x480. |
| `display.render_scale` | `1.0` | Multiplies the resolution: below 1.0 is faster, above 1.0 supersamples (up to 4.0). |
| `display.ray_tracing` | `"on"` | The ray-traced lighting (refer to "Ray-traced lighting"). `"off"`: off. `"occlusion"` and `"depth"` show what the lighting uses. |
| `display.ray_tracing_occlusion` | `0.8` | How much the traced occlusion darkens corners and creases (0.0 to 1.0). |
| `display.ray_tracing_reflections` | `0.25` | How strongly the surfaces reflect the traced scene (0.0 to 1.0). |
| `display.ray_tracing_bounce` | `0.25` | How much light one traced bounce carries between surfaces (0.0 to 1.0). |
| `network.tailscale` | `true` | System link across a Tailscale network (refer to "Multiplayer"). |
| `network.allow_upnp` | `false` | Tailscale and the local network do not need a forwarded port. |
| `network.join_from_clipboard` | `false` | An invite link on the clipboard does not join a game. |

## Multiplayer

### Local network

Start the game on each machine. One machine creates a system link game. The
other machines see the game in the list of system link games.

### Tailscale

Machines on one Tailscale network (tailnet) play system link as if they
were on one local network, from any location:

1. Install Tailscale on each machine, and sign in to the same tailnet.
2. Start the game on each machine.
3. One machine creates a system link game. The other machines see the game
   in the list and join it.

When Tailscale operates, the game gets the tailnet's online machines from
the `tailscale` command every 15 seconds. It sends the game's
announcements to them, and to the local network. A machine that hears a
game connects to the address that the announcement came from, so a game
found across the tailnet is joined through the tailnet. Linux machines also
do this. On Windows, put the Tailscale addresses of the other machines in
`network.broadcast`.

macOS asks one time for permission to use the local network. Select
"Allow".

### The internet, with forwarded ports

A machine can join a game on the internet without Tailscale when the host
forwards ports on its router:

1. The host forwards TCP port 5150 and UDP port 5150 to its computer, and
   creates a system link game.
2. The other machine sets `network.broadcast = "<host's internet address>"`
   in `config.toml`, starts the game and opens the list of system link
   games.

The search goes to the host. The host answers each machine outside its
local network that searched in the last minute, and the other machine
connects to the address that the answer came from. If the other machine's
router changes the ports of its connections, that machine also forwards UDP
port 5151.

### Internet play

Invite links (`halo://join/...`) operate as on Linux. Refer to "Internet
play" in [port/linux/README.md](../linux/README.md). The host's game puts
its link on the clipboard and in `debug.txt`. To join on macOS, give the
link on the command line:
`build/macos/Halo/halo 'halo://join/...'`. Or set
`network.join_from_clipboard = true`, copy the link, and switch to the
game. (macOS does not open `halo://` links with the game: the application
does not register the scheme.) Tailscale needs no invite.

## Ray-traced lighting

After the game draws the solid parts of the 3D world, and before the
transparent parts, the fog, the effects and the HUD, the lighting
(`port/linux/src/raytrace_gl.c`) sends rays through that picture's depth
buffer:

- Ambient occlusion: rays go from each pixel across the half sphere above
  the surface. A ray that hits a surface near it makes the pixel darker.
  Corners, creases and the ground below objects get darker, as in the real
  world.
- One bounce of indirect light: the color that a ray hits adds a small
  quantity of light. A red wall makes the floor next to it a little red.
- Reflections: a ray goes in the mirror direction of the view. The surface
  reflects the color where the ray hits. Surfaces reflect more at glancing
  angles (the Fresnel effect).

The rays go through the screen's depth buffer, because the Xbox game has
no other data about the scene, and Macs before the M3 have no ray tracing
hardware. A ray that goes off the screen hits nothing. Its effect fades.
At the display's resolution, the lighting takes a few milliseconds of an
M2 Pro's time for each frame.

`port/macos/tests/run_raytrace_test.sh` draws a test scene through the
lighting on ANGLE and writes the pictures to `build/macos/raytrace_test`.

## How the port operates

### The guest and the host

The game's data contains 32-bit pointers, so the game must operate with
32-bit pointers (refer to "ILP32 code" in
[port/android/README.md](../android/README.md)). The macOS port uses the
design of the Android port: the game ("the guest") is ILP32 code in a
static ELF image, and a 64-bit program ("the host",
[host/](host)) loads it and does its system calls, its SDL calls and its
OpenGL calls.

An arm64 macOS process cannot map memory below 4 GB (its `__PAGEZERO`
covers the low 4 GB), and arm64_32 code keeps its addresses below 4 GB. The
two builds solve this in different ways:

| Build | Guest code | Guest memory |
| --- | --- | --- |
| `macos` (native) | arm64_32, as on Android, with its memory accesses rebased (below) | A 4 GB region at any 4 GB-aligned address |
| `macos_x86_64` | x32 (x86-64 instructions, 32-bit pointers) | The low 4 GB: the host has a 64 KB `__PAGEZERO` |

### Rebasing (the native build)

`tools/macos_arm64_rebase.py` changes the guest's assembly: each memory
access and each indirect branch goes through `x28`, the base of the guest's
region, plus the low 32 bits of the address register:

```
ldr w0, [x1, #8]   ->   add x27, x28, w1, uxtw
                        ldr w0, [x27, #8]
```

The guest is compiled with `-ffixed-x27 -ffixed-x28`, so the compiler does
not use the two registers. A register contains a guest address (a 32-bit
pointer) or an address in the region (from `sp` or `adrp`). The low 32 bits
of both are the guest address, so the one formula is correct for both.
Accesses through `sp` and `x29` do not change: the guest's stacks are in
the region.

The functions of the host that the guest calls get thunks
(`tools/macos_host_thunks.py`, `tools/android_gl_stubs.py --host-thunks`).
A thunk adds the base of the region to each pointer argument. A host
pointer that the guest keeps (a directory of `posix_directory_open`) goes
to the guest as a small handle.

### The host

| File | Function |
| --- | --- |
| `host_main.c` | Finds the folders, loads ANGLE and the image, runs the game's `main()` on the main thread (Cocoa needs it) on a stack in guest memory. |
| `host_memory.c` | The guest's region, the Xbox memory window, `mmap` for the guest, and the write tracking of the texture cache. |
| `host_loader.c` | Loads the ELF image and fills its import table. |
| `host_thread.c` | Threads with stacks in guest memory, and the calls from the host into the guest. |
| `host_syscall.c` | The guest's Linux system calls on macOS: the numbers, flags, error codes, `stat` and `dirent`, and futexes on `os_sync_wait_on_address`. |
| `host_sdl.c`, `host_gl.c` | SDL3 and OpenGL ES for the guest. |

Apple silicon has 16 KB pages, and the Xbox had 4 KB pages. The game puts
blocks at fixed 4 KB-aligned addresses in the Xbox memory window, so the
window stays mapped, and the guest's `mmap` and `munmap` in it only clear
the memory.

### Game source changes

`HALO_ANDROID` marked all the code of the Android port. It is now three
macros (`port/linux/include/halo_linux_prefix.h`):

- `HALO_GUEST`: the ILP32 guest (Android and macOS), for example the
  stack walker and the true signature of `player_effect_screen_fade_in`.
- `HALO_GLES`: the OpenGL ES renderer (Android and macOS).
- `HALO_ANDROID`: only the Android app (its storage, touch, toasts).

The macOS guest is compiled with `HALO_MACOS`, so it uses the desktop's
code for the window, the mouse, the keyboard and the first start.
`source/render/render.c` calls the ray-traced lighting (`HALO_LINUX`).

## Tests

- `ninja macos_test` (and `ninja macos_x86_64_test`) builds
  [tests/guest_runtime_test.c](tests/guest_runtime_test.c) as a guest image
  with the host. Start it with `build/macos/test/Halo/halo`. It tests musl,
  threads and futexes, thread-local storage, files and the host's
  directory handles, time, the rebased code, sockets and the Tailscale
  lookup. `port/macos/tests/run_guest_tests.sh` does both steps.
- `port/macos/tests/run_raytrace_test.sh`: refer to "Ray-traced lighting".
- `port/macos/tests/run_determinism_test.sh` runs the game's matrix maths
  and `halo_` functions over 1.4 million inputs on the native and the
  x86-64 builds and compares hashes of the results. Machines in a system
  link game must compute alike, so an optimisation (compiler flags, SIMD)
  must keep these hashes.

## Performance

- The native guest is compiled for the M1 (`-mcpu=apple-m1`), without
  fused multiply-add and without `-ffast-math`, so the results are those
  of the other builds (refer to "Tests").
- `HALO_PROFILE=1 build/macos/Halo/halo` samples every thread 1000 times a
  second, and writes `profile.txt` to the game's folder at exit: the game
  functions and the host functions (SDL, ANGLE) where the time goes.

## Find problems

- `host.txt` in the game's folder is the log of the host. `debug.txt` is
  the log of the game. Start `halo` in a terminal to see both.
- If the guest code stops, `host.txt` shows the registers and the frame
  chain. To find the functions, enter
  `llvm-symbolizer --obj=build/macos/Halo/halo_guest.elf <address>` with
  the guest addresses.
- `HALO_HIDDEN_WINDOW=1 HALO_EXIT_AFTER=5 build/macos/Halo/halo` starts
  the game without a window and stops it after 5 seconds.

## Security

Before this port was made, the repository was examined for code that could
harm a Mac (refer to `SECURITY_AUDIT.md` next to the repository). The
macOS port:

- does not update itself (`port/linux/src/updater.c` is not part of it);
- does not ask the router to forward a port, and does not join games from
  the clipboard, unless `config.toml` says so;
- lets the internet play tunnel give a remote machine's traffic only to the
  ports of the game's own sockets (`xnet_is_game_port`), not to other
  programs on the Mac;
- runs the `tailscale` command only from its usual locations, with no
  arguments other than `status --json`.
