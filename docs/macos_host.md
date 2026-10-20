# The macOS host

The game runs on Android as ILP32 AArch64 code (`arm64_32`) in a 64-bit
process: 64-bit instructions with 32-bit pointers (`port/android/README.md`,
"ILP32 code"). The guest image, `halo_guest.elf`, does not depend on
Android, so a macOS host can load the same image. It needs three things from
the system:

1. The guest's fixed addresses: the Xbox memory at `0x80000000` to
   `0x88000000` and the image above it, to `0x8c000000`. The cache files
   hold pointers to these addresses, so they cannot move.
2. An executable mapping of the guest image.
3. A thread pointer for each guest thread. On Android the guest does not
   keep it in `TPIDR_EL0` (the host's bionic uses the register): the guest
   asks the host for it (`host_get_tp`, `pthread_arch.h`). The spike checks
   whether macOS would let a guest use the register, because asking the
   host costs an exit in a VM.

The programs in `port/macos/spikes` check each one on arm64 macOS, and
`tools/test_macos_spikes.py` builds and runs them.

## Results

Checked on macOS 26.6.2 (SDK 26.5), Apple Silicon, Apple clang 21.0.0.

### Memory below 4 GB: refused

`low_memory.c` tries to map `0x80000000` to `0x8c000000`.

Linked with a small page-zero (`-Wl,-pagezero_size,0x4000`), the program
is killed as it starts (status 137, SIGKILL), and nothing is logged.
`otool -l` shows the `__PAGEZERO` segment as asked (`vmsize 0x4000`), so
the kernel refuses the binary itself. An arm64 executable must have a 4 GB
page-zero.

Linked as usual, the program starts, but nothing below 4 GB can be mapped:

```
region at 0x80000000: free; next region 0x100958000-0x10095c000
mmap with a hint: placed at 0x105624000 instead
mach_vm_allocate fixed: (os/kern) invalid address
mach_vm_deallocate of the range: (os/kern) successful
mach_vm_allocate fixed: (os/kern) invalid address
FAIL: no way to map 0x80000000-0x8c000000
```

The lowest address the process may use is 4 GB. Freeing the range changes
nothing, because it is outside the process's address space altogether. Thus
the guest cannot live in the host's own address space, as it does on
Android.

### Thread pointer: overwritten

`thread_pointer.c` sets `TPIDR_EL0` on two threads and checks it after each
yield or sleep:

```
TPIDR_EL0 at start: 0x2006
FAIL: thread 0 lost its TPIDR_EL0 at round 1
```

macOS uses `TPIDR_EL0` itself (it holds a value before the program sets
it), and the kernel writes it again at each context switch. A guest in the
host's process keeps asking the host for its thread pointer, as on Android.
Inside a VM the register is the guest's own, so there the guest can read it
directly and save an exit on each access to thread-local data.

### Executable image

`hv_vm_map` maps the guest's memory executable inside a VM (below), so no
separate check of executable mappings in the host process was made. That
check matters only to the in-process approach (option B).

### Hypervisor.framework: works

`hypervisor.c` creates a VM, maps a page of host memory at guest physical
address `0x80000000`, and runs two instructions there at EL1 with the MMU
off: an add, and an `hvc` that exits to the host. It is signed ad hoc with
the `com.apple.security.hypervisor` entitlement
(`hypervisor.entitlements`):

```
hv_vm_create: success
hv_vm_map at 0x80000000: success
guest code at 0x80000000 ran 100000 times; x0 = 100000
round trip guest -> host -> guest: 1004 ns
PASS
```

A VM has an address space and registers of its own, so the fixed addresses
and `TPIDR_EL0` are the guest's. Each exit to the host costs about 1 µs.

## The ways forward

### A. The guest in a VM (Hypervisor.framework)

The guest image is unchanged. The host:

- creates a VM, maps the guest's memory at its fixed addresses, and one
  vCPU for each guest thread (each on a host thread, as Hypervisor.framework
  requires);
- gives the guest a small EL1 runtime: exception vectors, and page tables
  that map the guest's range as normal, cacheable memory (with the MMU off,
  all memory is device memory: slow, and unaligned accesses fault);
- turns the guest's host imports into exits. Every call the guest makes
  to the host, system calls included (`__guest_syscall` calls
  `host_syscall`), goes through one generated stub
  (`tools/android_imports.py`) that branches through
  `__host_import_table`. In the VM build the stub is `mov w16, #index;
  svc #0; ret`, and the EL1 vector turns the `svc` into an `hvc`. The host
  reads the arguments from the vCPU's registers, turns guest addresses into
  host addresses, and carries out the call as the Android host does
  (`host_syscall.c`, `host_sdl.c`). The game's sources do not change;
- keeps the thread pointer in `TPIDR_EL0`, which is the guest's own in a
  VM, instead of the `host_get_tp` import, which would be an exit on each
  access to thread-local data;
- sends rendering in batches: a command buffer in shared memory, handed to
  the host once per buffer or per frame, not one exit per call. At about
  1 µs per exit, the thousands of draw calls of a frame cannot each be an
  exit at 120 fps (8.3 ms per frame).

The app needs the `com.apple.security.hypervisor` entitlement. Ad hoc
signing is enough for development. A distributed app is signed with it.

### B. The guest in the host's process, with rewritten code

The guest's code would be changed so that it does not need low addresses.
Every memory access would add a base register (`[x28, w1, uxtw]`). The place for
this is the step that already converts the guest's assembly
(`tools/android_asm_convert.py`). There would be no VM exits, but every
load, store, atomic and address computation in the game must be found and
rewritten, and one that is missed corrupts memory. The image would also need
an executable mapping in the host process (`MAP_JIT` or the hardened
runtime's exceptions).

### Recommendation

Option A, which the host now carries out (below): the guest's code stays as it is, the host carries over most of
the Android host, and the cost (an exit per host call) is limited by
batching the renderer, which the Metal renderer needs in any case.

## Building

The macOS build is for Apple Silicon Macs with macOS 26 or later. Its tools
come from Nix (`flake.nix`): clang 22 and lld for the guest, the macOS 26
SDK and SDL3 for the host, and Rust (`rust-toolchain.toml`) for the
runtime crate, `port/macos/runtime`.

```sh
nix develop                    # the shell: every tool the build needs
python configure.py            # on a Mac, the default target is macos
ninja macos                    # build/macos/halo_host and build/macos/halo_guest.elf
nix build .#host               # the final build: result/bin/halo_host, result/share/opence/halo_guest.elf
ninja macos_app                # build/macos/OpenCE.app (needs Xcode)
nix build .#app                # result/Applications/OpenCE.app
ninja macos_app_test           # the Swift package's tests (port/macos/app/Tests)
```

The app, `OpenCE.app`, is the Swift package `port/macos/app` built with
Xcode's Swift toolchain (`xcrun swift build`; nixpkgs' Swift cannot link
against the macOS 26 SDK) and linked with `libhalo_host.a`, the runtime
crate and SDL3. `tools/macos_app.py` lays out the bundle
(`Contents/MacOS/OpenCE`, `Contents/Info.plist`,
`Contents/Resources/halo_guest.elf`) and signs it ad hoc with the
hypervisor entitlement, using Apple's `codesign`. AppKit keeps the main
thread; the guest's main runs on a thread of its own. Being outside the
store, Xcode makes `nix build .#app` impure: it needs a Nix without the
sandbox.

Swift calls the runtime crate through UniFFI (`port/macos/runtime/src/swift.rs`).
For now that is a view of the guest's memory, `GuestMemoryView`. Its
`hostAddress`, `read` and `write` accept only ranges that are wholly mapped,
and throw `GuestMemoryError` for any other. `runningGuestMemory()` returns the
memory of the guest the C host is running. The memory is mapped when the
guest is made and never again, so the view needs no lock; it keeps the memory
alive while the view exists. The build makes the crate's `uniffi-bindgen`
(`--features bindgen`, in a target folder of its own) and runs it on
`libopence_runtime.a`. It writes the bindings into the package's ignored
folders `Sources/OpenCERuntime` and `Sources/opence_runtimeFFI`.

`ninja macos` builds:

- the guest image, `build/macos/halo_guest.elf`: the Android guest's
  sources with `HALO_GUEST_VM` defined (its imports are `svc` stubs, and
  it keeps its thread pointer in `TPIDR_EL0`);
- the runtime crate, a static library with a C header made by cbindgen
  (`build/macos/runtime/opence_runtime.h`): the guest's memory plan, its
  page tables and the image's checks;
- the host library, `build/macos/libhalo_host.a`: `port/macos/host`, the
  Android host's SDL bridge (`host_sdl.c`), the platform layer's files and
  sockets (`posix_files.c`, `posix_net.c`), UPnP and DTLS, and the import
  dispatcher that `tools/macos_dispatch.py` writes from
  `port/android/host_imports.list`. Its entry point is `halo_host_run`
  (`port/macos/host/halo_host.h`);
- the command-line host, `build/macos/halo_host`: `port/macos/cli/main.c`
  linked with the library.

### Signing

The host needs the `com.apple.security.hypervisor` entitlement
(`port/macos/host/halo_host.entitlements`). The build signs it ad hoc
with the entitlement, which is enough to run it on the Mac that built it.
A copy for other Macs must be signed with a Developer ID that has it.
The Nix package skips the fixup phase, whose signing would drop the
entitlement.

## Running

```sh
build/macos/halo_host [build/macos/halo_guest.elf] [arguments for the game]
```

The image defaults to `halo_guest.elf` beside the host, or
`../share/opence/halo_guest.elf` (the Nix package's layout). The game's data
folder is `HALO_DATA_ROOT`, else the current folder: its `maps` folder holds
the maps, and `config.toml` and `debug.txt` are written there. Saves and
profiles go to `XDG_DATA_HOME`, by default `~/Library/Application
Support/OpenCE`.

Without a person at it, as on Linux (`AGENTS.md`):

```sh
HALO_DATA_ROOT=assets HALO_NULL_RENDERER=1 HALO_HIDDEN_WINDOW=1 \
SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy \
HALO_NETWORK_TEST=host:bloodgulch HALO_NET_ADDRESS=127.0.0.1 HALO_EXIT_AFTER=30 \
build/macos/halo_host
```

hosts a game on bloodgulch and logs its player's state each second
(`network test: tick ...`), then exits with status 0.

The app's data folder is `HALO_DATA_ROOT` too, else
`~/Library/Application Support/OpenCE` (`DataRoot` in
`port/macos/app/Sources/OpenCEShell`). The app makes its `maps` folder.
If `ui.map` is not in it, the app does not start the game: a dialog says
where to copy the maps, offers to show the folder in the Finder, and quits.

Two copies on one Mac need loopback addresses of their own, and macOS has
only 127.0.0.1 on `lo0`. Add others with
`sudo ifconfig lo0 alias 127.0.0.2 up` (and `127.0.0.3`). They last until
the Mac restarts.

The host's own settings:

| Variable | Meaning |
| --- | --- |
| `HALO_HOST_QUIET` | Logs only the host's warnings and errors |
| `HALO_HOST_IMPORT_COUNTS` | At exit, reports the VM's exits per second, in all and by import (busiest first) |
| `HALO_HOST_FRAME_LOG` | A file the app writes a line to for each frame presented: its number, the milliseconds since the one before, the GPU's milliseconds on its last command buffer and the VM's exits in between (the benchmark) |
| `HALO_HOST_PHYSICS_LOG` | Logs each physics step: the commands refused, the ragdolls and bodies in the world, whether the level is loaded and the step's milliseconds on the host (the benchmark) |
| `HALO_HOST_SAMPLE=<seconds>` | Every that many seconds, prints each vCPU's guest address and its frames |
| `HALO_HOST_MAIN_LOOP=1` | Runs the guest on a thread of its own and the main dispatch queue on the main thread, as the app does |
| `HALO_HOST_TRACE_MAIN_THREAD` | Reports each import the first time it is carried out on the main thread |
| `HALO_HOST_RENDER_LOG` | Reports each render batch the host checks: its commands, those refused, and its bytes' FNV-1a |
| `HALO_HOST_GUEST_IMAGE=<elf>` | The app only: runs that guest program in the game's place, without the maps check |
| `HALO_HOST_CAPTURE=<png>` | The app only: draws the window off the screen and writes one frame to that PNG ("Frame captures") |
| `HALO_HOST_CAPTURE_FRAME=<n>` | The frame written: the guest's nth `PRESENT` (1) |
| `HALO_HOST_CAPTURE_SIZE=<w>x<h>` | The size the window is drawn at while capturing (640x480) |

### The main thread

AppKit, and SDL's windows and events with it, must be used on the main
thread. The app's main thread runs AppKit, so the guest's threads cannot
call those imports there themselves. The imports that need it are listed in
`tools/macos_dispatch.py` (`MAIN_THREAD`): SDL's initialisation, the window
and its size, relative mouse mode, the event queue, message boxes and the
clipboard. The OpenGL context's imports are not among them, because they
belong to the thread that made the context current.

When the guest asks for one of these imports, its vCPU thread queues the
request (`port/macos/host/host_main_thread.c`). The main dispatch queue is
asked to run it, and the thread waits. The run loop drains that queue in its
common modes, so a request also runs during a modal alert or a live resize.
The import's result and errno are brought back to the vCPU.

The app turns this on with `halo_host_use_main_queue` before it starts the
guest. Before that, and on the main thread itself, an import runs where it
is asked for. As the app terminates, `halo_host_shutdown` fails any queued
request with `ECANCELED`, and any made later, so that no guest thread
waits for a run loop that has stopped.

SDL keeps its error message per thread. After a failed import on the main
thread, `SDL_GetError` on the guest's thread does not have that import's
message.

### The window

In the app, the guest's window is an AppKit window, not SDL's. Before it
starts the guest, the app installs its windows with
`halo_host_set_window_backend` (`port/macos/host/host_window.c`). From then
on, `host_sdl_create_window` and `host_sdl_window_size_in_pixels` are the
app's (`Sources/OpenCE/WindowBackend.swift`). The command-line host
installs none and keeps SDL's.

The window (`Sources/OpenCEWindow`) is resizable and can go fullscreen in a
Space of its own, from View > Enter Full Screen or Control-Command-F. The
guest asks for a fullscreen window, as Android's does; the app opens it as a
window instead. Its first size is the one the game asks for (640 by 480
times `display.window_scale`), shrunk to fit the screen if needed while
keeping 4:3. Its content is a `CAMetalLayer` at the window's backing scale,
cleared to black until the Metal renderer draws in it. The guest reads the
window's size in pixels each frame, so a resize reaches the game on its
next frame. Closing the window quits the app.

Until the Metal renderer draws, the app's windows have a stand-in OpenGL
context. `host_sdl_gl_create_context`, `host_sdl_gl_make_current`,
`host_sdl_gl_set_swap_interval` and `host_sdl_gl_swap_window` succeed for
them, so the guest's video starts and its event loop runs. The OpenGL ES
imports themselves are still refused, so nothing is drawn.

### The keyboard and mouse

The window's view passes AppKit's key, modifier, mouse, wheel and focus
events to the host (`halo_host_post_*`). `port/macos/host/host_input.c`
turns them into the SDL events that the guest's event loop reads
(`port/linux/src/sdl_platform.c`), and keeps them in a queue until the guest
polls. With the app's windows, `host_sdl_poll_event` returns those events
first, then SDL's own. `host_sdl_set_relative_mouse`
captures the mouse through the app: the pointer is hidden, held at the
window's middle, and only relative moves are reported.

- **Keys:** a key is its place on the keyboard. AppKit's key codes map to
  SDL's scancodes in a table. A key with no scancode, such as Fn, is not
  sent. Caps lock is a press and a release each time it changes.
- **Mouse:** the moves since the last poll are one motion event. Positions
  are in points from the view's top left. A trackpad's scrolling is a
  tenth of a notch per point.
- **Focus:** losing the focus releases the mouse at once and lets go of
  the modifiers held. Gaining it captures the mouse again if the game
  still wants it, because the guest, built as Android's, does not ask
  again.
- **A full queue** keeps the events it already has and drops new ones (the
  guest polls each frame).
- **No gamepads:** the app is played with the keyboard and mouse only.
  SDL's joystick and gamepad events (0x600 to 0x6ff) are left out of the
  guest's polls, so the guest opens no pad and reads none. The command-line
  host keeps SDL's pads.

### Sound

The app plays the game's sound through AVAudioEngine
(`Sources/OpenCEAudio`), not SDL. `port/macos/host/host_audio.c` answers
the guest's audio-stream imports. A stream with a callback (the game's
playback, `port/linux/src/dsound_sdl.c`) gets a handle from 0x10000 up.
Each of these streams has its own engine, whose source node is fed from a
ring of 8192 float frames. A stream with no callback (the microphone) is
still SDL's.

- **The render** runs on Core Audio's real-time thread. It only reads the
  ring (`halo_host_audio_render`). Frames the ring lacks are silence. After
  each render it wakes the stream's thread.
- **The stream's thread** starts when the guest resumes the stream. It
  calls the guest's callback for the bytes the ring lacks, in the stream's
  own format, as SDL would. It keeps about 30 ms queued, and never less
  than 1536 frames: one render of Core Audio's and one chunk of the game's
  mixer.
- **Formats:** U8, S8, S16, S32 and F32, at 1 to 8 channels, are converted
  to float as the guest puts them. The engine's mixer converts to the
  device's rate and channels.
- **A new output device** stops the engine. It is started again.
- The command-line host keeps SDL's sound.

### The renderer

The guest's Direct3D 8 will draw through Metal, not OpenGL. It writes
Metal-level commands in batches in its own memory. The format is
`port/macos/include/opence_render.h`:

- A batch is a header (`OCRB`, version, size, count) and its commands,
  all little-endian words.
- A command's first word is its op, with its length in words in the high
  half.
- The commands make objects (buffers, textures, shader libraries,
  pipelines, depth-stencil states, samplers), and fill buffers and
  textures from guest memory. They open and close passes, set draw state
  and draw inside a pass, and copy, read back, make mipmaps and present
  outside one.

Before Metal sees any of it, `port/macos/runtime/src/render.rs` decodes and
checks each batch, because the guest is untrusted.

- **The batch is copied out of guest memory first**
  (`RenderDecoder.decodeGuest`), so the guest cannot change it after the
  checks.
- **Every command is checked against the decoder's state:** the objects
  made so far, the open pass and its targets, the pipeline and the vertex
  buffers bound. Checked: ids, values, sizes, formats, and that every
  region lies inside its texture level or buffer. Guest ranges must be
  mapped. A pipeline must match its pass. A draw must read only bytes
  inside its vertex buffers.
- **A command that fails is refused alone**, with its offset and reason
  (`DecodedBatch.rejected`), and the rest of the batch goes on. A command
  whose length runs past the batch ends the batch.
- **What is not checked:** an indexed draw's indices themselves. Only the
  index buffer's range is checked. A wild index makes the GPU read past a
  vertex buffer, which Metal on Apple silicon keeps inside the process's
  own GPU address space.


The guest writes its batches with `port/macos/guest/opence_render_writer.c`
(`opence_render_writer.h`), a buffer of its own per thread. It rings the
host with one import, `host_render_submit(batch, size)`, once a batch: when
the frame asks, or when the next command would not fit. That is one exit
for a batch of up to 16 MB, not one for each call, as OpenGL ES has.
The host copies the batch out before it returns, so the buffer is the
writer's again at once, with no ring to share and no fence to wait on.
The answer is the number of commands refused, or -1 for a batch refused
whole.

`host_render_submit` gives the batch to the app's renderer once it has
installed one (`halo_host_set_render_backend`). Without one, as in the
command-line host, the runtime checks it (`opence_render_check`, with its
own objects) and drops it. Refusals are logged as warnings, the first 32
of them.

The app's renderer is `Sources/OpenCERender/MetalRenderer.swift`, installed
by `Sources/OpenCE/RenderBackend.swift`. It is made at the first batch, with
its own `RenderDecoder`, which it tells the window's drawable size before
each batch. It draws in the layer of the first window the guest opens, and
from then on the view leaves the layer's drawables to it (a resize no longer
clears the window). What the decoder lets through is carried out in the
order the guest wrote it:

- **One command buffer** lasts from batch to batch, so a pass may begin in
  one batch and end in the next. It is committed at `PRESENT`, without
  waiting, and at `READ_TEXTURE`, which waits for it and then writes the
  pixels to guest memory.
- **Uploads are blits** from a copy of the guest's bytes, in the same
  command buffer as the draws, so a draw sees what was uploaded before it
  and not what comes after. Buffers are shared, textures private.
- **An upload inside a pass** ends the pass's encoder, storing its targets.
  The next draw begins another, which loads them, and the pass's state is
  set on it again. A multisampled pass resolves only at `END_PASS`. At 4K
  each such split stores and loads about 66 MB of targets.
- **Writes are not blits.** The stream buffer (the game's own vertex and
  index arrays, copied each draw, about 400 KB a frame on bloodgulch) is
  filled with `WRITE_BUFFER`, which the host copies into the shared buffer
  as it reads the command, keeping the pass's encoder. The guest writes it
  as a ring and so never over what this frame has drawn from (a frame that
  goes round the whole ring uploads instead); the host keeps, for each
  buffer, the ranges written for each command buffer the GPU has not
  finished, and waits for one before writing over its range. It took
  bloodgulch's frames from about 195 encoders to 3.
- **The window** is colour texture 0. Its passes draw one drawable of the
  layer until `PRESENT` presents it. `nextDrawable` blocks while the
  layer's three drawables are all in flight, which keeps the guest at most
  three frames ahead of the GPU.
- **States are made once** for what they are made of: a library for its
  source, a pipeline for its libraries and descriptor, a depth-stencil or
  sampler state for its fields. Direct3D's state changes make the same few
  states again and again, and Metal compiles them slowly.
- **What Metal will not make** (a shader that does not compile, a pipeline
  whose functions do not match) is refused, and so is what uses it: the
  `SET_*` commands that name it, and draws without a pipeline. These count
  with the decoder's refusals in what `host_render_submit` answers, and are
  logged (the first 99) in the `render` category of the app's log.

The guest's shaders name their functions `vertex_main` and `fragment_main`.
A scissor is clipped to the pass's target, which a window resized since the
pass began can make smaller than the decoder's drawable size.

`display.anti_aliasing`'s `"fxaa"` and `"smaa"` are the GL build's passes
(`xgpu_post.c`) written in MSL (`port/macos/guest/d3d8_metal_post.c`), at
the same point of the frame: the 3D view's rectangle is copied out of its
target (`COPY_TEXTURE`), and each pass is one triangle drawn over that
rectangle, the last one into the target with alpha masked off. SMAA
(`port/third_party/smaa`, HIGH preset) has three passes, through an RG8
edges texture and an RGBA8 weights texture, both cleared first since the
edge pass discards where there is no edge. Its HLSL compiles as MSL with
porting macros and a few rewrites of its `out` parameters into references.
Its size is a literal in the source, so a library is made for each size of
view. `"ssaa2x"` and the multisampled settings are not drawn by the Metal
renderer, which logs this and draws without them.

### Frame captures

With `HALO_HOST_CAPTURE`, the app draws the guest's window off the screen,
in a BGRA8 texture of `HALO_HOST_CAPTURE_SIZE`, not in the layer: a frame
then depends neither on the screen's backing scale nor on a window being
shown. At the `PRESENT` that `HALO_HOST_CAPTURE_FRAME` names, the renderer
waits for the frame's commands and writes the texture to the PNG (8-bit
RGBA, with ImageIO) before it answers the guest, so a guest may exit at
once. The app logs what it wrote, or why it could not; a setting it cannot
read is logged and the window drawn as usual. While capturing, the
window's size in pixels (`host_sdl_window_size_in_pixels`) is the capture's,
so the game draws at that size.

A frame of the game is the same from run to run only if the game's clock
is: its menus' backgrounds and its ticks follow the time. With
`HALO_FRAME_CLOCK=1` (`debug.frame_clock`), from the first frame shown the
game's clock steps a 60th of a second at each `PRESENT`, however long the
frame took (`platform_clock_frame_shown`, `port/linux/src/xbox_kernel.c`).
While no frame is shown for more than a tenth of a second, as a map loads,
it follows the real time, so that the game's waits end; the vertical blank,
sleeps and sound keep the real clock. The game's random numbers start from
a fixed seed (`source/math/random_math.c`), so that a game spawns its
players in the same places.

`tools/frame_compare.py` compares a capture with an approved reference: a
pixel differs when one of its channels is more than 8 from the
reference's (a GPU's rounding), and the frame matches when no more than
0.5% of its pixels differ (an edge rasterised a pixel over) and none by
more than 64 (a pixel drawn wrong). `--difference` writes the reference,
faint, with the differing pixels red.

`tools/test_macos_frames.py` runs guest programs in the app
(`HALO_HOST_GUEST_IMAGE`), each with a capture, and compares their frames
with the references in `port/macos/tests/frames`. The first,
`vm_render.c`'s `scene`, is a clear, a triangle of three colours and a
quad of half-transparent white blended over it. A frame that does not
match is left in `build/macos/frames` with its difference image. A new or
changed frame is approved, once looked at, by running the test with
`HALO_APPROVE_FRAMES=1`, which writes the references.

A guest crash is reported with its registers, the exception's syndrome and
the guest's frames, by address in the image (`llvm-symbolizer` on
`halo_guest.elf` names them).

### Exits

Each call the guest makes to the host is an exit from the VM, at about
1 µs. A hosted bloodgulch game, headless (null renderer) for 30 seconds,
made 503 000 exits a second, half of the time of one core:

| Import | Calls a second |
| --- | --- |
| `host_syscall` | 248 000 |
| `host_sdl_ticks` | 164 000 |
| `host_errno` | 32 000 |
| `hostposix_socket_last_error` | 12 000 |
| `hostposix_socket_recv` | 9 000 |

The renderer's commands go in batches (one `host_render_submit` a frame,
or when the ring fills), and the guest now answers the calls that ran most
itself:

- `SDL_GetTicks` asks the host once, then counts with the guest's own
  counter (`CNTVCT_EL0`): the host lets the guest read it at EL0
  (`CNTKCTL_EL1.EL0VCTEN`) and gives every vCPU the same count (a virtual
  timer offset of 0).
- A `hostposix_*` import's stub takes its errno back in `w1`, which the
  host sets at each exit, and stores it in the guest's errno, so no
  `host_errno` follows the call.
- The memory watch (which finds the game's writes to the textures and
  buffers the renderer copied, `port/linux/src/memory_watch.c` on Linux)
  is read in the guest. The host tracks writes with the VM's own page
  tables (`port/macos/host/host_watch.c`): `host_memory_watch_protect`
  takes write access from the window's 16 KB pages (`hv_vm_protect`), and
  the guest's first write to one is an exit (a data abort, `EC 0x24`) at
  which the host gives the page a new generation, gives write access back
  and lets the write run again. The generations are in a table in guest
  memory that the guest shares once (`host_memory_watch_share`) and reads
  without an exit, so asking whether a range was written costs nothing,
  and the guest calls the host only to protect a page again or to forget
  a range freed. The host's own writes into the window (file reads) go
  through a buffer of its own and `memory_watch_prepare_write`, as on
  Linux. Before it, every range looked written every frame, and the
  renderer copied every texture again each frame.

The same game then made about 24 000 exits a second, nine in ten of them
`host_syscall` (futexes, sleeps and the like).

### The benchmark

`tools/test_macos_benchmark.py` plays bloodgulch for 60 seconds at
3840×2160, off the screen, on the real clock, from the debug camera of the
golden frames, with `HALO_HOST_FRAME_LOG` set. The app's renderer logs each
frame as its PRESENT's commands complete on the GPU, so the times are the
GPU's pace, not only how fast the guest sends its batches. The frames after
the longest wait (the map's load) are the game's; their times, the GPU's
and the exits per frame go to `build/macos/benchmark.txt`:

```sh
python -m pytest -q tools/test_macos_benchmark.py -s
```

The first run, before any of the work to make it faster: 80 ms a frame at
p50 and 84 ms at p99 (about 12 a second), 28 ms of it the GPU's on the
frame's last command buffer, with 125 exits a frame. The main menu drew in
7.5 ms.

| On an M1 Max, at 3840×2160 | p50 | p99 | GPU p50 | Exits a frame |
| --- | --- | --- | --- | --- |
| The first run | 80 ms | 84 ms | 28 ms | 125 |
| Writes tracked by stage 2 (textures copied only when written) | 13.0 ms | 17.2 ms | 13 ms | — |
| Stream writes without blits (about 195 encoders a frame to 3) | 4.17 ms | 4.78 ms | 0.53 ms | 79 |

The last is 10 061 frames in 42.3 seconds, about 240 a second, with p99
well inside 8.33 ms (120 a second). The longest frame, 124 ms, is the first
after the map's load.

The second test plays the same with `game.ragdolls` and
`game.physics_debris` on and `debug.network_test_crowd` at 10 seconds of
the game's time: thirty bipeds in six rows before the camera, shot dead
three a tick, and two hundred assault rifle casings thrown up among them,
ten a tick. With `HALO_HOST_PHYSICS_LOG` the host times each physics step,
and the steps with bodies go to `build/macos/benchmark_physics.txt`. The
target for a step is a millisecond.

| On an M1 Max, at 3840×2160, with the crowd | Frame p50 | Frame p99 | Step p50 | Step p99 | Step max |
| --- | --- | --- | --- | --- | --- |
| Jolt on one thread (the frame clock) | — | — | 0.82 ms | 8.07 ms | — |
| Jolt on the calling thread and three workers | 4.52 ms | 29.2 ms | 0.45 ms | 2.82 ms | 2.96 ms |

At most 30 ragdolls and 119 bodies were in the world; two seconds after
the last shot, 118 casings were left, all of them Jolt's bodies. The step's
p50 is within the millisecond; its peak is not: in the ticks after the
shots, the thirty ragdolls' 480 parts and the casings are all awake and in
contact (about 590 active bodies). The step is the host's time, while the
guest waits for it; at its peak it is a third of a frame at 120 a second.

The frames' p99 is the stock game's, not the physics': for the thirty
seconds the casings live (their tag's life), the scene's frames take about
26 ms, 11 ms of it the GPU's, and about the same with both settings off
(25.4 ms against 25.9 ms). Thirty bipeds standing take about 11 ms a frame.

## Tests

In `nix develop`, after `ninja macos`:

```sh
python -m pytest -q tools/test_macos_spikes.py tools/test_macos_build.py tools/test_macos_vm.py tools/test_macos_host.py
python -m pytest -q tools/test_macos_render.py   # render batches through the doorbell
ninja macos_app_test                              # the app's Swift tests, the Metal renderer's pixels too
python -m pytest -q tools/test_macos_post.py      # the post passes' MSL compiles; SMAA's lookup textures
python -m pytest -q tools/test_frame_compare.py   # the frame comparison's tolerance
python -m pytest -q tools/test_macos_frames.py    # golden frames from the app (after ninja macos_app)
python -m pytest -q tools/test_macos_benchmark.py -s   # bloodgulch's frame times at 4K
cargo test --manifest-path port/macos/runtime/Cargo.toml
python -m pytest -q tools/harness         # the asset-free harness, in the VM
```

`tools/test_macos_vm.py` runs guest programs (`port/macos/tests`) in the
host: threads, futexes, time, files, memory, sockets and SDL, headless, and
a window made from a guest thread in the main-loop mode. It also builds two
of the host's files natively and tests them: the system calls, and the
main thread's queue with its shutdown.
`tools/test_macos_host.py` runs the game: without maps, with them, a
hosted network game, and a second copy that joins it (when `lo0` has the
aliases).

## Not supported yet

- Intel Macs, and macOS before 26.
- Gamepads: the app takes the keyboard and the mouse only, by choice, and
  leaves SDL's joystick and gamepad events out of the guest's polls.
- Drawing from the command-line `halo_host`: it stays headless
  (`debug.null_renderer`); the window and the Metal renderer are the app's.
