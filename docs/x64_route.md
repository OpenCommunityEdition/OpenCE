# The 64-bit route

Status: decision record for the fork's 64-bit build (branch `x64`, based on
upstream build-337, `899219cc`). Written 2026-11-05.

## Recommendation

**Take route (a): the game as an x32 (ILP32, x86-64 instructions) guest image
in a 64-bit host process, as PR #359 does on Linux. Then give the same guest
image a Windows x64 host. Move the Vulkan renderer into the 64-bit host
behind a command ring.**

Route (a) is the only route that does all of these at once:

- it keeps every data format the game has (tags, map caches, the Xbox
  window at `0x80000000`, Custom Edition's `0x40440000`, saved games,
  network messages and pool sizes) byte for byte;
- it compiles the 466 decompiled files nearly unchanged (21 lines of
  `source/` in PR #359);
- it puts the GPU driver, Vulkan with ray tracing, the asset store and the
  OS in a real 64-bit process.

Android already runs this design (`arm64_32` guest). A true LP64 port would
take far more work, would break the data formats, and is not likely to be
accepted upstream. It would also have to live beside the ILP32 guest builds
forever, because Android and the web stay ILP32.

## What the game assumes (why this is hard)

- `struct tag_block { long count; void *address; struct tag_block_definition *definition; }`
  (`source/tag_files/tag_groups.h:87`), used 372 times in `source/`. Map
  caches hold raw 32-bit absolute pointers into a tag window at a fixed
  address. Custom Edition tags are linked to `0x40440000`
  (`port/linux/src/xbox_memory.c`).
- The Xbox window is physical memory at `0x80000000 + P`
  (`platform.h:146-150`). D3D resources keep physical addresses in `Data`
  (`d3d8_resources.c`, `xbox_textures.c`). The game state and the tag cache
  sit at fixed addresses (`physical_memory_map.c`).
- Saved games are the game-state block, pointers included (`source/saved
  games/game_state*.c`). `data_array` and the other runtime containers
  hold native pointers (`source/memory/data.h`).
- The decompiled code treats `long` and pointers as interchangeable in
  many places.

Each of these is free in an ILP32 guest and costly in LP64.

## The routes

### (a) x32 guest in a 64-bit host (PR #359)

**What PR #359 is.** It is open, 36 commits, +10,018/-982, and was based on
`8102e711`, which is 616 commits behind build-337. It contains three things:

- the desktop arm64 build (#354, closed by the maintainer);
- the guest-owned system-call interface (#358, closed: "new architectures,
  or groundwork that exists only to add them");
- the x32 build itself.

Where the added lines are:

| Area | Lines added |
| --- | --- |
| host (`port/android/host`, `port/linux/host`) | 3,726 |
| tests | 2,838 |
| tools | 1,529 |
| guest runtime | 701 |
| guest musl arch | 451 |
| docs | 281 |
| platform layer | 224 |
| game source | 21 |

A test merge onto build-337 conflicts in 9 files: `build.yml`, `README.md`,
`configure.py`, `host_imports.list`, `d3d8_gl.c`, `port_config.c`,
`updater.c`, `android_build.py`, `linux_build.py`. The maintainer's
comment on #359 (2026-10-29): "Love it but 10k lines of code changed is
excessive ... trim down to sub 1-2k lines".

**How it works.**

- clang `--target=x86_64-linux-gnux32 -fno-pic -march=x86-64 -mlong-double-64`
  builds the game, the platform layer and musl into a static ELF image
  linked at `0x70000000`. It must sit below 2 GB because of x32's
  sign-extended `R_X86_64_32S` absolute addressing.
- A 64-bit PIE host (the Android host, `port/android/host` +
  `port/linux/host/host_main.c`) does the following:
  - reserves the low 4 GB;
  - loads the image;
  - runs guest threads on guest stacks;
  - translates the guest's own system-call ABI to the kernel's (113 cases
    in `host_syscall.c`);
  - serves SDL, GL (GLVND `libOpenGL`), dialogs and updater imports
    through stubs (`jmpq *__host_import_table+off(%rip)`).
- The guest passes pointers zero-extended, and this is tested
  (`x32_boundary_test.c`). Stack arguments are never pointers
  (`check_host_imports.py`).
- The guest never touches `%fs`/`%gs` (TLS is emulated), and
  `guest_image_check.py` enforces it.
- The kernel's x32 ABI (`CONFIG_X86_X32_ABI`) is **not** needed, and is
  absent on this CachyOS kernel. I checked that clang 23.1.4 here
  generates x32 code: `sizeof(void*)=4`, `long=4`, `long double=8`, SSE
  scalar maths.
- It uses the desktop's 512 MB Xbox window and caches, as the i686 build
  does. The PR reports that b30 plays headless and "a network game
  between this and the i686 build stays in step (the next PR)".

**What it unlocks.**

| Goal | With route (a) |
| --- | --- |
| 64-bit Vulkan drivers with RT | Yes, in the host. NVIDIA's 32-bit driver has no acceleration-structure, ray-query or RT-pipeline extensions (measured, `overhaul_research/vulkan.md` §0.3). A Vulkan backend in the host gets all of them, plus the 64-bit validation layers and gamescope's 64-bit WSI layer (there is no 32-bit one). |
| Address space for the renderer and assets | Unlimited in the host. The host reads guest memory **at the same virtual addresses** (a guest pointer zero-extended is a valid host pointer), so a host-side renderer reads vertex and index buffers and textures in the Xbox window with no copy. Upscaled textures, lightmap re-bakes, RT acceleration structures and streaming caches all live in host memory or VRAM. The guest keeps only IDs. The guest's own budget is about 4 GB minus the image (0x70000000), and it holds only simulation data. |
| No 32-bit libraries | The process is a plain x86-64 binary. It needs no multilib SDL, Mesa or NVIDIA user-space. Arch derivatives and Fedora are dropping these, and Valve's own long-term direction is away from them. |
| Steam Deck | x86-64 Linux, SteamOS 3. Same binary as desktop Linux, with a 64-bit Vulkan WSI under gamescope. |
| Windows | Needs a Windows x64 host for the same guest image ("Windows" below). This is the main new work. |
| Android | Already this model. A host-side Vulkan backend serves Android `libmain.so` and desktop with the same code (`vulkan.md` §8). |
| Larger maps and a new map format | Not limited by route (a). The limits today are the fixed tag window `0x803A6000`/22 MB (`physical_memory_map.h:15`, `cache_files.c`), 16-bit indices, UVs and skinning, and pool sizes. That is format work, not pointer width. See "Larger maps". |

**Multiplayer and determinism risk: low, and testable bit for bit.**

The x32 guest compiles the same C with the same clang and almost the same
flags: `-ffp-contract=off`, `-fwrapv`, `-fms-extensions`, `-fshort-wchar`,
`-fno-strict-aliasing`, and the vendored musl maths (`halo_math.h`). The
data layout is the same ILP32 layout:

- doubles and `long long` are 8-aligned in both, as i686's `-malign-double`
  gives;
- `long` is 32 bits in both;
- `wchar_t` is 16 bits in both.

So the network version (`HALO_PORT_NETWORK_VERSION 41`), the messages,
the saved games and the pool sizes do not change.

I checked the i686 release binary built here (`ninja linux`, release,
portable):

- All float and double **arithmetic is SSE**. clang uses SSE2 for i686
  with `-march=x86-64`, so it computes the same IEEE results as x32's SSE.
- x87 appears only in these places:
  - 443 `fstps` and 139 `fstpl`: i386 float return values passed through
    ST0, which is exact;
  - 101 `fildll` and 7 `fistpll`: int64↔float conversions;
  - 14 `fldcw`;
  - 6 `fldt`/`fstpt` (`long double`, CRT only).
- The game sets x87 precision to 53 bits (`real_math.c:132`,
  `CW_DEFAULT = _RC_NEAR|_PC_53`). Under that setting, `fild` of an int64
  is exact below 2^53, so the conversion matches SSE's `cvtsi2ss`
  everywhere except |x| ≥ 2^53. The simulation sites are `game_tick`,
  `object_compute_function_values` and the `main_loop` timing. Their
  values are tick and time counters, far below 2^53.
- `long double` (80-bit on i686, 64-bit on x32) appears only in the CRT,
  never in the game's sources. The one `grep` hit is a variable named
  `doubled_spin`.

Residual risks:

- Optimiser differences between targets. Without fast-math, IEEE results
  stay identical.
- UB-dependent code that compiles differently.
- Address arithmetic. x32 computes addresses in 64-bit registers, but
  clang's x32 truncates pointers to 32 bits, so the Xbox window wraps as
  it does on i686.

The netcode is also host-authoritative with client prediction
(`NETCODE.md`), so a tiny drift would be corrected rather than desync the
game. But the target is bit-exact, and route (a) can **prove** it: the
game-state block has the identical layout at the identical address in
both builds, so hashing it every tick from the same seeded input must give
identical hashes (see the plan, step 2).

**Upstream acceptability: medium-low as one PR, good as a series.** The
maintainer closed #354 and #358 as "new architectures" and asked for #359
to be 1-2k lines. Its 2.8k test lines, the arm64 desktop build, and the
CI/updater/release plumbing can all go in separate PRs. The core
(guest-arch descriptor, guest syscall table, x32 musl arch, x86-64 loader
path, FP control words, the desktop host's entry point) is about 2-3k
lines. That core can be offered as a self-contained series. The fork
carries it either way.

### (b) A true LP64 port

**Work.** Every pointer-bearing persistent or shared structure needs one of
two fixes:

- a translation layer: a relocating tag loader that rewrites every tag
  struct from its 32-bit cache layout to a 64-bit layout, driven by the
  tag field definitions, for both Xbox and CE caches; or
- an annotation: `XPTR`/`__ptr32` fields, which PR #22 started and then
  abandoned into a plan.

Beyond the tags, it also needs all of the following:

- the Xbox window's physical-address `Data` fields, which are 32-bit by
  definition;
- the game-state block and saved games: convert them on save and load, or
  break compatibility with 32-bit saves;
- `data_array` and every runtime container;
- every `(long)ptr` idiom in decompiled code;
- the hs script runtime's union and long casts;
- network encoders that use `sizeof` of structs containing pointers.

It touches a large share of the 466 game files, against the project's rule
that they compile "as they are with few changes".

**What it unlocks.** Everything (a) unlocks, plus a guest with more than
4 GB of simulation memory, which no goal needs.

**Multiplayer risk.** Medium-high. The arithmetic stays the same, but any
missed layout or `sizeof` change alters behaviour, pool usage or message
encoding. The game-state hash oracle no longer works (the layouts differ),
and 32-bit saved games need a converter.

**Upstream.** Very unlikely. It would also be one more ABI beside the
ILP32 Android and web builds, which stay.

**Verdict:** no.

### (c) A 32-bit game process plus a 64-bit render-host process

**Work.**

- Make the 512 MB Xbox window a shared mapping (memfd on Linux,
  `CreateFileMapping`/`MapViewOfFileEx` on Windows; today it is a private
  reservation, `xbox_memory.c:78,212`, `win32_posix.c:543-554`), mapped at
  the same address in both processes.
- Add a command ring and fences across processes, and keep texture and
  query results in sync.
- Give the render host the window, and forward input to the game. On
  Wayland a surface cannot be shared across processes.
- Handle the life cycle of two processes, crashes, and the debugger.

**What it unlocks.** RT and 64-bit drivers for the renderer only. The game
still needs 32-bit SDL and audio libraries and 32-bit OS support. Its
address space stays 4 GB with the window inside it. It does nothing for
later 64-bit goals, and it doubles the windowing and test matrix
(`plan.md` review point 3).

**Multiplayer risk.** None: it is the same i686 game binary.

**Upstream.** Poor. It is a large IPC design for one feature.

**Verdict:** only a stopgap if Windows RT were needed before the Windows
x64 host exists. Not recommended. The Windows host is the better use of
the same effort.

### (d) Other options

- **(d1) A native `-mx32` Linux process.** It needs `CONFIG_X86_X32_ABI`.
  That is off or disabled at boot on most distributions, missing on this
  machine, and not in the SteamOS kernel. It would also need x32 builds of
  glibc, SDL, Mesa and the GPU drivers, and no vendor ships x32 drivers.
  It does not get 64-bit Vulkan at all. **No.**
- **(d2) LP64 with `__ptr32`/`__uptr` annotations** (MSVC extension, which
  clang accepts with `-fms-extensions`): pointers stay 64-bit in code,
  32-bit in structs, and all game memory is kept below 4 GB. It is still
  route (b)'s audit of every pointer field, and silent layout breakage on
  any miss. It gains nothing over (a), whose compiler makes *every*
  pointer 32-bit for free. **No.**
- **(d3) The x32 guest on Windows** is route (a) with a Windows host, and
  is part of the recommendation. Windows has no x32 ABI, but the guest
  never calls the OS: it is a freestanding ELF image whose only exits are
  host imports and the guest syscall table. A Windows x64 host that loads
  the **same image** gives one simulation binary on Linux, Steam Deck and
  Windows, the strongest possible cross-play guarantee.
- **(d4) Box64/FEX-style or emulation-based approaches.** Irrelevant on
  x86-64: the 32-bit code is native. The problem is the libraries and
  drivers, not the instruction set.
- **(d5) WoW64 on Windows** (keep the 32-bit PE). It works today, but
  32-bit Windows drivers have the same RT and address-space limits.
  Status quo, not a route.

## Windows x64 host (route a, second half)

The guest image is the Linux x32 image unchanged. The new parts are a PE
host, `port/windows/host`, built with clang `--target=x86_64-pc-windows-msvc`.
`--windows-cross` already builds Windows from Linux. The host does this:

1. **Address space.** At start-up, before anything else allocates low
   memory, it reserves `0x40440000` (CE), `0x70000000` (image) and
   `0x80000000-0xA0000000` (Xbox window) plus guest pools, with
   `VirtualAlloc(MEM_RESERVE)`. The exe links high (x64's default base,
   `0x140000000`, with `/HIGHENTROPYVA`). System DLLs live above 4 GB on
   Win64. A tiny loader that relaunches if the reservation fails is the
   fallback.
2. **Calling convention.** Host import functions and guest callbacks are
   `__attribute__((sysv_abi))`, so the existing stubs need no change.
   clang supports `sysv_abi` on win64.
3. **Stacks.** Guest threads run on guest stacks below 4 GB. Win32 code
   (and `__chkstk`, and exception dispatch) checks `RSP` against the TEB's
   `StackBase`/`StackLimit`. So each import stub switches to the host
   thread's stack (save guest RSP, call, restore), or the host updates
   the NT_TIB limits around guest entry. Stack switching is the robust
   choice, as Wine's WoW64 does.
4. **Syscalls.** It implements the guest syscall table (113 cases on Linux;
   the game uses about 60: files, `getdents`, `mmap`/`mprotect` in guest
   memory, futex via `WaitOnAddress`, thread create, clocks, sleep, sockets
   via Winsock, `poll`, interface enumeration, `getrandom`). Most of the
   Win32 side already exists in the 32-bit Windows build:
   - `win32_posix.c`, `win32_files.c`, `win32_net.c` (3,622 lines in all of
     `port/windows/src`);
   - memory watch from `win32_memory_watch.c` (VEH + `VirtualProtect`);
   - crash reports from `win32_crash.c`.
5. **Platform specifics.** Windows paths (`%APPDATA%`, Documents), the
   updater and the clipboard are host imports, or follow the environment
   (`HOME`/XDG equivalents) that the guest already reads from the host.
6. **SDL3 and GL.** SDL3 x64 DLL and `opengl32` for the GL path. The
   Vulkan backend lives in the host anyway.

## Renderer placement (how Vulkan fits)

- The **GL path** in the guest keeps working through today's per-call
  thunks (`host_gl.c`, `android_gl_stubs.py`). It is the fallback and
  stays upstream-compatible.
- The **Vulkan backend runs in the host**. The guest's D3D8 layer writes a
  command ring of fixed-width records into guest memory, and a handful of
  imports drive it: `host_vk_init`, `host_vk_submit(ring_offset,size)`,
  `host_vk_present`, `host_vk_texture_upload`, `host_vk_query_results`,
  `host_vk_shutdown` (`vulkan.md` §8).
- Draws reference guest memory directly at the same addresses. The ring
  carries guest addresses, and the host dereferences them.
- The same backend source serves the Linux, Steam Deck and Windows hosts
  and Android `libmain.so`. Vulkan cannot be thunked across ILP32/LP64
  (struct layouts, `pNext` chains, 64-bit non-dispatchable handles), so
  the host is the only place it can live in any guest build.
- The renderer split under way in `opence-vulkan` should define its
  backend interface as this ring from the start, so the 32-bit in-process
  build and the 64-bit host consume the same records.
- FSR runs in the host. A DLSS plugin needs a 64-bit process (the DLSS SDK
  ships 64-bit only on Linux), and the host is one. XeSS is the same.

## Larger maps

Route (a) gives the guest about 2.3 GB above the Xbox window
(`0xA0000000` up) for a relocatable heap. A new map format should do
three things:

1. Use 32-bit offsets and handles, not absolute pointers, so it loads
   anywhere in the guest heap rather than at `0x803A6000`.
2. Widen indices to 32 bits.
3. Split **simulation data** (tags, collision BSP, scripts, AI data),
   which goes in the guest, from **render-only data** (meshes, textures,
   lightmaps), which goes host-side, streamed, 64-bit, with no guest
   footprint.

A map whose simulation data alone exceeds 2 GB is not a realistic target,
so 32-bit guest pointers do not cap the Forge or map-format goals. If
they ever do, any subsystem can move host-side behind the same import
boundary.

## Implementation plan

1. **Port the minimal x32 core onto build-337** (fork branch `x64`). Take
   these from #359's commits:
   - the guest-arch descriptor (`tools/guest_arch.py`);
   - the guest syscall interface and host translation (#358's
     `ae11d4cb`, `72144f4c`, `b1f6def2`, `43ac8789`, `6c741788`, `03f43c0a`);
   - the x32 musl arch;
   - the x86-64 loader, stubs and zero-extension (`d5df8e45`);
   - FP control words (`8bfdaa2b`);
   - the image check (`96626806`);
   - the desktop host and desktop imports (`port/linux/host`);
   - the build (`c37eab5e`, `9d37f48d`, `bc2500c9`).

   Resolve the 9 conflicts against main's newer `d3d8_gl.c`,
   `port_config.c`, `updater.c`, `linux_build.py` and `android_build.py`.
   Leave out the arm64 desktop build, its OpenGL ES desktop renderer, and
   CI and release plumbing until later. Build `python3 configure.py
   --release --portable --linux-x86-64 && ninja linux_x86_64`. Musl 1.2.5
   and SDL 3.4.16 are downloaded at configure time with SHA-256 checks.
   Keep `ninja linux` (i686) building and unchanged.
2. **Prove multiplayer compatibility**, headless (`HALO_NULL_RENDERER=1
   HALO_HIDDEN_WINDOW=1 HALO_NO_AUDIO=1`, console ports 26100-26109, a
   separate data root reading `~/Desktop/halo-linux-release/maps`):
   - Add a debug setting that logs a hash of the game-state block and the
     tick, every tick. It is identical layout in both builds, so it must
     match bit for bit. Run the same seeded `debug.test_input bot:<seed>`
     on i686 and x32 and compare the logs for every MP map and a30, b30
     and c10 in single player.
   - Run `tools/net_compare.py --a build/linux/halo --b
     build/linux_x86_64/halo --swap` on several maps, i686 host to x32
     client and the reverse.
   - Cross-load saved games and checkpoints (save on i686, load on x32
     and back), and check `config.toml` and profiles.
   - Load CE maps (`game.custom_edition`, the `0x40440000` window) and run
     the dedicated server mode.
   - Run the guest tests (`tools/test_host_units.py`,
     `test_host_syscall.py`, `test_check_host_imports.py`,
     `test_guest_image_check.py`) and the Linux tests.
   - Audit int64→float conversion sites (≥2^53 cannot occur).
3. **Make x32 the fork's Linux and Steam Deck build.** Package
   `build/linux_x86_64/halo` + `libSDL3.so.0`. Test on a Steam Deck
   (Game Mode under gamescope, Steam Input, suspend/resume), on the RX
   7900 XTX machine (RADV) and on NVIDIA. Point the fork's updater at the
   x86_64 asset.
4. **Windows x64 host**, as in "Windows x64 host" above, loading the same
   guest image. Bring it up under Wine for CI smoke tests, then on real
   Windows. Run net_compare across Windows x64, Windows i686 PE and Linux.
5. **Vulkan backend in the host** on the command ring, shared with the
   Android host. RT, FSR, the DLSS and XeSS plugins, and the high-res asset
   store go host-side. GL stays the fallback.
6. **Upstream as a series of separate, self-contained PRs**, each passing
   on its own:
   1. the two Android fixes the maintainer asked for (musl SHA-256,
      `sysinfo`);
   2. the guest syscall interface, as Android hardening, with its tests;
   3. the guest-arch descriptor refactor (no behaviour change);
   4. the x32 guest build behind `--linux-x86-64`, at or under about 2k
      lines;
   5. its CI and release job;
   6. the Windows x64 host;
   7. the Vulkan host backend.

   The fork carries whatever upstream declines.

## Open risks

- **Windows stack and TEB handling** across the guest/host boundary
  (plan step 4). This is the only part with no in-tree precedent; Wine's
  WoW64 is the reference.
- **Low-4 GB reservation on Windows.** Overlays (Steam, Discord, capture
  tools) and anti-virus DLL injection can take low addresses. Mitigations:
  reserve at the very start of the process, and relaunch on failure.
- **Per-call GL thunk overhead** in the GL fallback on the host (already
  shipped on Android). Vulkan's ring removes it.
- **Upstream may decline** new architectures outright, as it did with
  #354 and #358.
