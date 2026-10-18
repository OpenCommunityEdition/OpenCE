# Live Mac inventory and first native checks

Measured over SSH on 2026-10-15. No SSH settings, system software, display
power, or existing projects were changed. Small probes were staged under
`$PROBES/20261015` (the probe folder: any folder on each participating Mac) on
each participating Mac. The names in the first column are the labels these
notes use for the hosts (for example in the names of evidence logs).

| Label | Actual machine | OS | Memory | GPU | Development tools |
| --- | --- | --- | --- | --- | --- |
| `powerbook` (also `pbg4`) | PowerBook5,4, G4 1.5 GHz | 10.5.9 / 9L31b, Darwin 9.8.0 | 2 GiB | Mobility Radeon 9700, 64 MB | Xcode 3.1.4, GCC 4.0.1; 10.4u/10.5 SDKs |
| `ibook` (also `g4`) | PowerBook6,7, iBook G4 1.42 GHz | 10.5.9 / 9L31b, Darwin 9.8.0 | 1.5 GiB | Mobility Radeon 9550, 32 MB | Xcode 3.1.4, GCC 4.0.1; 10.4u/10.5 SDKs |
| `intel` | MacBook6,1, Intel 2.26 GHz | 10.6.8 / 10K549 | 8 GiB | GeForce 9400M, 256 MB | GCC 4.2.1 and working GCC 4.0 PPC cross-build; 10.4u/10.5/10.6 SDKs |
| `g3` | PowerBook3,1, G3 500 MHz | 10.4.11 / 8S165 | 1 GiB | Rage Mobility, 8 MB | GCC 4.0.1; 10.3.9/10.4u SDKs; DevToolsCore 798.0 |

The G3 is an OS/loader/portable-code test host, not a supported Halo hardware
target. Neither G4 is running stock Leopard; stock 10.5.8 validation remains.
The Core 2 Duo is not the Merom model described in an older SSH comment:
live hardware reports MacBook6,1. Do not use the stale comment as CPU evidence.

Two emulated Tiger and Leopard hosts were also tried; one rejected
public-key authentication and the other refused the connection. Neither is
needed to continue.

## Graphics proof

The standalone `tools/mac_capabilities.c` compiled and ran natively on both
G4s. It uses an accelerated CGL context with no window or desktop drawable.

Both report OpenGL 2.0 ATI-1.5.48, GLSL 1.20, maximum texture dimension 2048,
eight fixed-function texture units, VBOs, EXT framebuffers, S3TC, shader objects
and occlusion queries. These are reported features, not complete conformance.
PowerBook's GL renderer string is `ATI Radeon 9600 XT OpenGL Engine`, despite
System Profiler identifying Mobility Radeon 9700; record both rather than
silently replacing one. iBook reports `ATI Radeon 9550 OpenGL Engine`.

Native ABI probes report big-endian, four-byte pointers/long/wchar and a double
at offset four after a char. Thus 16-bit wchar must be selected explicitly for
engine units, and structure layout needs per-type verification.

Both passed an 8x8 offscreen color/depth framebuffer test:

- Red triangle followed by a farther green triangle: readback 255,0,0,255.
- Half-alpha blue over red: readback 128,0,126,191, no GL error.
- The initial exact blend tolerance rejected blue=126. The diagnostic now
  explicitly allows two 8-bit levels around the mathematical 127.5 RGB result
  for fixed-point blending; raw values remain logged. This is approximate
  blend validation, not cross-GPU bit equality or a game graphics benchmark.

Actual shader compilation, textured geometry, render-target sampling, and NV2A
shader resource limits remain to test. GLSL 1.20 is now an evidence-based first
renderer target for these G4s; it does not meet the older minimum-GPU goal alone.

## Cross-build and engine proof

The MacBook's `xcodebuild -version` fails because dyld cannot load
DevToolsFoundation.framework. No repair was attempted. GCC 4.2 targeting PPC
with the 10.4u SDK failed to locate a stdarg.h include. GCC 4.0 with the same
SDK successfully produced PPC Mach-O executables, bypassing xcodebuild.

`tools/mac_byteswap_smoke.sh` reproduces the engine-unit build from the repo
root. It compiles the actual `source/memory/byte_swapping.c`, not a reimplemented
algorithm, with 32-bit PPC and short wchar. One production change replaces
three MSVC-only wide-integer literal suffixes with equivalent shifted casts.
It preserves the MSVC type spelling; the harness maps that type for GCC.

The resulting executable built on `intel` ran successfully on `powerbook`, `ibook`
and `g3`: 2-, 4-, and 8-byte reversal and round-trip checks passed on every
machine. This proves a small real engine unit can cross-build and execute on
both Tiger and Leopard PPC. It does not prove a complete ABI or endian port.

Existing cseries warnings about integer constants and a prototype-local struct
remain; the Tiger startup object also produces an obsolete long-branch warning.
The isolated harness removes an unused extern TLS qualifier only for this test.
A real Darwin TLS strategy remains unresolved. A trial globally redefining
`__inline` caused duplicate-static errors in Apple's headers and was discarded.

Raw probe/build logs are in `build/mac-audit/` locally (ignored build evidence).
The checked-in probes and this report preserve the reproduction procedure and
the meaningful results. No game data has been located or loaded yet.

## Selected roles and next work

Use `intel` with GCC 4.0 for initial Tiger PPC cross-builds, `powerbook` as primary
game target, `ibook` as the 32 MB VRAM check, and `g3` for Tiger compatibility
of architecture-neutral code. Keep native G4 builds as independent controls.

Next: compile representative math units, resolve anonymous structs and inline
linkage, test full engine layouts and scripting scalar byte order (current converters return long); add GLSL
1.20 shader/texture readback tests and measure shader uniform limits. Continue
the complete map conversion audit before expecting any playable map.

## Compiler and shader follow-up

See [ppc-mac-compiler-shaders.md](ppc-mac-compiler-shaders.md) for successful
MacBook-built GLSL 1.20 and ABI probes on both G4s, and a reproduced scripting
endianness failure. The earlier union-return scripting suspicion is disproven.


## MacBook build storage

OpenCE remote work lives under `$PROBES` on a separate volume of the Intel
MacBook (journaled HFS+, owners enabled), reached by a symlink from its home
folder. Keep the link: historical build manifests and the shared
`20261015/engine/build` target use it. Future snapshots and outputs must stay
under the workspace, not new top-level home folders. Check the volume is
mounted before building; do not replace a dangling link with a new local
directory if the disk is unavailable.

The migration checksum check reported all 318,526 entries unchanged: 309,665
files, 8,215 directories, 646 symlinks. A native Intel helper compiled and ran
through the compatibility path with output on the volume, passing 32/0. Local
verification records are in `build/mac-audit/macbook-storage/`. Other
projects' snapshots, game installations and personal files were untouched.
