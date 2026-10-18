# OpenCE for PowerPC Tiger and Leopard: initial audit

Audited 2026-10-15 against upstream commit
`8102e711c353068d22d62efe6bf67ebc8a90a22e`.
Checkout: the repository root. This is a source audit, not a working port.

MacBook storage: use `$PROBES` (a folder on a separate volume, linked from the
home folder) for remote OpenCE sources, builds and test artifacts. Preserve the
link because older build manifests and source-clone links use the home path.
Check that the volume is mounted before remote work. Do not create additional
OpenCE scratch folders directly in the MacBook home directory.

Current status: the native version-7 map loader and original engine now pass
player creation, movement, look, weapon fire, projectiles, audio mixing, map
unload and reload on the MacBook Intel control and both G4s. Hidden rendering
includes the sky, textured BSP terrain and bases, weapon, HUD and text. The
latest pruned-input batch passes 837 checks with zero failures on each host,
including 30 rendered frames/30 engine ticks and a second map lifecycle.
Five-frame fixed-step captures match the preceding baseline on each host.
See [ppc-mac-progress.md](ppc-mac-progress.md) for artifacts and exact scope.

Performance is still the immediate playable-build blocker. With opt-in bone
constant gathering and the 16 MiB texture cache, the latest clean five-frame
PowerBook indexed-reuse test takes 10.007 seconds, including 7.893 seconds for the first
frame: about 0.53 seconds per warm frame. The 16 MiB cache avoids observed
texture evictions, but remains opt-in; the default is 8 MiB. Unused vertex
input pruning preserves the images but has not shown a measured speedup.
Stream decoding costs about 120 milliseconds per warm frame in that run.
A short PowerBook CPU sample confirms rendering dominates the sampled frame.
Submission timings include driver stalls and are not GPU execution times.

Gathering moves most relative-address vertex shaders onto hardware; shaders
31 and 50 still use software vertex processing on the G4s. An isolated ARB
prototype passes focused finite-input comparisons on all three hosts, but
has not been integrated: exceptional-value handling, strict program
recognition and live renderer state still require work. Shader identity
caching and within-draw vertex decode reuse now pass integrated hidden tests
on all three hosts with unchanged per-host captures. Indexed driver submission
passes short native scene comparisons with a modest PowerBook improvement;
compact vertex uploads were slower on all three hosts and remain disabled.
Static BSP decode reuse is the next targeted renderer change, with explicit
resource lifetime and write-generation validation before cache hits.
The original Mac application is a reference for matching behavior and
measuring performance, not evidence that OpenCE is playable.

A responsive interactive session, actual Tiger runtime parity and minimum-spec
performance remain unverified. The findings below retain the initial audit
context; [ppc-mac-progress.md](ppc-mac-progress.md) and
[ppc-mac-hardware.md](ppc-mac-hardware.md) supersede its initial access and
untested-probe status.

## Target and first acceptance gate

Native 32-bit PowerPC on Mac OS X 10.4 and 10.5, first on the user's
PowerBook G4. Preserve an eventual original-Mac-Halo hardware target:
800 MHz G4, 256 MB RAM, 32 MB early-generation GPU under Tiger. Leopard
has its own higher OS floor. Core 2 Duo is later work.

Current data-source decision (user, 2026-10-15): use the installed retail
PC/Mac version7 Blood Gulch; see `ppc-mac-retail-maps.md` and
`ppc-mac-bsp-geometry.md`. The initial Xbox-only assumption below is superseded.

First playable-map target: a Blood Gulch local multiplayer session,
one player, with correct world geometry, collision, movement, weapons,
textures, depth, HUD, keyboard/mouse and audio. Blood Gulch is the chosen
engineering default, contingent on available user-owned game data. Record
a repeatable ten-minute session, map provenance/hash, OS, hardware, renderer,
resolution, frame-time distribution, memory and any missing effects. This
does not prove campaign, multiplayer interoperability or minimum-spec speed.
Controller integration follows basic input; it must not block map loading.

No game data was present in this checkout during the audit. Upstream's
README describes Xbox disc extraction. Upstream also has Custom Edition
support; it need not be brought up for this first milestone.

## Findings and changes required

| Priority | Finding and evidence | Required work |
| --- | --- | --- |
| P0 | `configure.py` has Linux, Windows, Android and web generators, no Darwin target. `tools/linux_build.py` hardcodes i686, MSVC-compatible flags, ELF-oriented tooling and SDL3. | Add an explicit Mac PPC build rather than routing Darwin to Linux. Build host tools separately from target objects. Prove compiler, assembler, Mach-O linker, SDK and deployment target together. |
| P0 | `port/linux/include/halo_linux_prefix.h:13` rejects PPC and later defines x86 selection macros. The source relies on 32-bit pointers/long, short wchar, MSVC extensions and struct-return behavior. | Create PPC prefix/ABI definitions; check real struct sizes/offsets, packed fields, bitfields and function-pointer signatures. The old scripting union-return claim was disproven; current converters return long. Merely removing the guard is unsafe. |
| P0 | `source/cache/cache_files.c` reads headers and tag data into native structures and dereferences embedded pointers. PPC is big endian. | Decode little-endian disk data with bounds checks before any count/pointer use; convert all typed numeric data, pointers/handles, BSP/model streams, scripts and strings at defined boundaries. Never indiscriminately swap opaque bytes. |
| P0 | `port/linux/game/tag_schema.h` explicitly excludes plain numbers and flags. `source/memory/byte_swapping.c` supplies useful primitives but does not establish complete cache conversion. | Inventory full field coverage before reusing schemas. Existing CE bounded readers in `cache_file_formats.c` are useful examples, not an endian-complete Xbox loader. Build asset-free conversion fixtures and later compare real map decoding on both byte orders. |
| P0 | `port/linux/src/xbox_memory.c` reserves fixed-address arenas with `MAP_FIXED_NOREPLACE`, Linux-specific flags and constructor ordering. `platform.h` uses base 0x80000000, desktop size 512 MB, versus 128 MB on Android/web. | Implement non-clobbering Darwin address reservation, verify the window early, and fail safely on collision. Reservation is virtual address space, not proof of 512 MB resident use. Audit capacities before choosing a smaller PPC arena. |
| P0 | `sdl_platform.c:777` requests GL 4.5 core. `d3d8_gl.c` uses VAOs, modern vertex binding, samplers, clip control, framebuffer blits and persistent query storage. | Add a capability-selected legacy backend; do not just reduce the context version. Replace unsupported features with measured alternatives. |
| P0 | `nv2a_vsh.c` and `nv2a_psh.c` emit GLSL 450, or GLSL 300 ES. They use packed-normal integer operations, explicit locations, texelFetch, and optional atomic counters. | Reuse NV2A instruction/combiner interpretation with older shader emission where supported. CPU unpack packed normals; test constant/attribute limits. HALO_GLES alone is not a Tiger backend. |
| P1 | SDL3 calls span window/events, gamepads, audio streams, filesystem, settings, screenshots and update code. | Introduce a narrow native Mac platform layer or deliberately scoped legacy shim. Prefer Carbon/CGL, Core Audio and era-appropriate HID interfaces for initial feasibility. Choose after an inventory of the exact required calls; avoid attempting all of SDL3. |
| P1 | `dsound_sdl.c` uses SDL3 streams and monotonic timing; POSIX files contain `/proc/self/exe`, `/proc/self/cmdline`; other units use clock_gettime. | Preserve the mixer but supply compatible device/timing integration; use Mach timing and Darwin executable-path facilities. Audit pthreads, atomics, sockets, TLS and all vendored dependencies against the actual SDK. |
| P1 | `xbox_textures.c` contains useful bytewise source decoding, but packed native output at approximately line 809 reaches GL_BGRA/GL_UNSIGNED_BYTE. | Test channel order and bytes explicitly on PPC, including compressed textures, vertex/index buffers, audio sample formats and UTF-16. |
| P1 | Upstream requires identical native math and disables fused multiply-add for networking. | Retain vendored math; disable PPC contraction with verified compiler options and compare numerical results. Do not assume swapping in system libm is equivalent. |
| Later | Network serialization, save state, replay data and checksums can expose native layout/endianness assumptions. | Audit wire and persistence boundaries before claiming cross-platform play. Preserve protocol where possible; follow AGENTS.md versioning rules for changes that alter agreement. |

## Renderer decision

Measure the actual PowerBook first. If accelerated GLSL is available, a
GLSL 1.20/shader-object path is the preferred first-map route, subject to
the device's actual limits. The 192-vector vertex constant bank and shader
resource use need explicit validation. Use legacy vertex arrays/VBOs,
texture-object sampler state, shader coordinate conversion, and supported
EXT framebuffer features. Queries and render-target copies need defined
fallbacks; unsupported calls must never be dispatched speculatively.

Original minimum GPUs require a separate fixed-function/multipass path,
with CPU vertex processing where necessary. This is a substantial second
renderer task. It must remain on the roadmap even if the PowerBook supports
shaders. Document effects that are approximated, then compare like-for-like
performance against MacSoft Halo. A renderer that omits effects is not an
unqualified speed improvement.

`tools/mac_capabilities.c` is a standalone first diagnostic. It uses an
accelerated CGL context, opens no window, prints native ABI properties,
renderer strings, legacy texture limits and extensions. It does not render
a triangle or validate framebuffer correctness yet. Build on the target:

```sh
cc -std=c89 -Wall -Wextra tools/mac_capabilities.c -framework OpenGL -o mac_capabilities
./mac_capabilities > mac-capabilities.txt
```

Compile separately with each intended SDK/deployment target before claiming
Tiger/Leopard compatibility. The diagnostic uses native wchar size; the game
will require an explicitly validated 16-bit ABI.

## What the existing projects contribute

An earlier emulation project's Radeon 9800 feasibility note documents the R200/R300
distinction, texture combiners, vertex programs, render targets and surface
coherence. Its Metal renderer runs on the modern host; it cannot become a
native PowerBook renderer. Reuse research and test design, not GPL emulator
code in this CC0 repository.

`STUDIO_TIGER_RESULTS.md` records Tiger 10.4.11 desktop and deterministic
Blender CPU-render tests. It explicitly does not certify general GPU/game
performance. It reports Quartz Extreme but no Core Image on that configuration.
`STUDIO_DISPLAY_INVESTIGATION.md` identifies display sleep as a source of stale
captures. Capture power state and timestamps during emulator graphics tests.
`CHATGPT_SUMMARY.md` is older and contains driver claims superseded or questioned
by later notes. Record the actual loaded driver and GL capabilities each time.

Use disposable VM overlays for ABI, loading and rendering regressions. Validate
performance and final compatibility on physical hardware. Do not alter the
existing emulation project or boot its valuable base disks in place.

An earlier controller project's hardware reconnaissance note reports Xcode 3,
GCC 4.0.1/4.2.1 and 10.4u/10.5 SDKs on the PowerBook. It reports a modified
Leopard 10.5.9 build 9L31b; this is not stock 10.5.8 validation. Its Bluetooth
Classic controller input evidence is useful. Native gamepad input should
consume a normalized device through a separate driver boundary; do not copy
GPL-derived controller implementation into this repository. README status is
inconsistent with the existing kext source, so recheck actual installed state.

## Execution order

1. Collect actual PowerBook GL/ABI/OS/CPU/RAM/VRAM data and verify toolchain.
   Build/run the probe under Tiger and Leopard; follow with deterministic
   textured triangle, depth, blending and render-target readback tests.
2. Prove a native PPC Mach-O build of representative engine units and ABI
   checks. Select a capable compiler rather than assuming GCC 4 accepts the
   current MSVC extensions and newer dependency code. Keep modern generators
   on the host if needed. Start without LTO or PGO.
3. Prove address reservation and endian-clean map loading without graphics.
   Compare decoded structures and asset checksums with a little-endian run.
4. Integrate legacy rendering and minimal platform services. Load Blood Gulch
   with keyboard/mouse, then audio and HUD. Log unsupported effects explicitly.
5. Verify a real playable session, then controller integration and Tiger parity.
6. Profile and optimize against MacSoft; validate standard network games before
   scaling toward 128 players. Keep hosting and joining performance separate.

## Initial verification and limits (superseded by progress log)

The capability probe compiled warning-free (deprecated API warnings suppressed
only on the modern host) and ran on Darwin 25.6 arm64 / Apple M5, reporting
OpenGL 2.1. This validates the probe's modern-host execution only. No PPC
compiler run, game build, map load, triangle test or hardware performance test
has passed yet.

A bounded SSH attempt to the previously recorded PowerBook address reached
an SSH server, but trusted host-key verification failed. No credentials were
sent, trust bypassed, or remote files changed. Establish identity through an
existing trusted alias/key record or user-verified fingerprint before remote
work. Continue local implementation and asset-free checks meanwhile.
