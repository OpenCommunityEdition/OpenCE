# Reversed depth and decal integration validation

Date: 5 November 2026. Base: `origin/testing` at
`ef03a840121f87b4a6e5de66afe8b5690c91c959`.

The base already contains OpenCE #495 (`49c8260b98bda1fe02060628809d2ce42a09d22b`),
including the desktop default `display.reversed_depth = true`. OpenCE #540
(`8468b4c347a5675946d4a53873398e47c6878ade`) was cherry-picked with its author
and provenance retained. Its draw-cache additions were combined with the
existing shield-honeycomb inputs and uniforms. The follow-up also applies
the constant bias to the fixed-function shader; otherwise #540 removes the
hardware constant term without supplying its replacement on that path.

## Review

The combined self-review covered depth allocation, clears, comparisons,
viewport ranges, projection recognition and fallback, constant and slope
bias, shader/program caching, fixed-function draws and lighting integration.
The existing model-lighting and MCC environment paths retain their guards.
No map-family loading, gameplay or network protocol changes were made.
No network version increase is required for these render-only changes.

The implementation remains in the responsible OpenGL backend. No new shared
game-engine hook or MCC adapter is needed. Inspection found no justification
for restructuring the projection translator or adding a second feature
module. Precision claims in the settings help were narrowed: reversed depth
improves distant precision but does not guarantee separation at all distances.
The [walkthrough](reading/reversed_depth.md) records the near-plane clipping
and fragment-written depth limitations.

## Build and automated checks

Windows release, portable, PGO and LTO off:
`python configure.py --release --portable --pgo=off --lto=off`, then
`ninja windows`. The build passed. The linker emits Expat `LNK4217` import
warnings in `menu_files.o`; this is not a warning-free build.

On an NVIDIA GeForce RTX 5070 Ti, OpenGL 4.5, driver 617.42:

- Nine depth tests passed with `HALO_TEST_GPU=1`. They compile 536 combinations
  of the 67 stock vertex programs, reversed/conventional depth, optional
  model lighting and packed attributes. They also check production bias
  calculations at different viewport ranges and render scales, fixed-function
  bias removal, both depth formats, 4x MSAA resolves, and projection output
  for surfaces 0.0016 world units apart through 1,000 world units.
- A temporary negative control removing the fixed-function correction
  reproduced the missing shader expression detected by the new assertion.
- The desktop, Android and web shader-emitter tests cover lighting enabled
  and disabled. The initial focused renderer/lighting suite passed 30 tests.
- After the combined review, the expanded focused suite passed 161 tests,
  including the GPU checks, all selected lighting/shield/emitter/loading
  harnesses and web build tests. One native socket test was explicitly skipped
  on Windows because it requires POSIX pthreads. The duplicate CRT define
  mentioned below was corrected in the local wrapper; its MCC mod-lifecycle
  test passed in this final run.
- The broader harness plus touch-menu, depth, render-stutter, lighting and
  web-build tests passed 1,643 tests and skipped eight with Windows CRT
  compatibility defines. It also had 372 failures: this was **not** a green
  full-suite run. Failures were in unchanged harnesses, including unavailable
  POSIX headers/functions, missing `m.lib`, Windows type-declaration conflicts,
  and one duplicate CRT define in the local compiler wrapper. None of the
  lighting, shield, plasma, emitter, loading or new depth tests failed.
- `ninja android` and `ninja web` were attempted. Configuration generated
  neither target because this machine lacks an Android NDK and Emscripten.
  Source-level platform tests do not replace those builds. Linux runtime and
  full Linux harness validation remain outstanding; WSL is unavailable here.

## Rendered game checks

All runs used hidden windows, isolated data and save directories, and at most
two game instances. Default-mode runs omitted both the config setting and
environment override; their startup logs reported `32-bit float, reversed`.
Explicit-false runs reported `24 bits, as the Xbox's`.

| Scenario | Settings and observed outcome |
| --- | --- |
| Xbox Blood Gulch, two peers | Opposing local lighting settings, host shield policy, respawns and inventory changes; 81 paired snapshots, no reported divergence, clean exits. |
| CE Death Island, two peers | Same compatibility checks; 81 paired snapshots, no reported divergence, clean exits. |
| Xbox `a30` opening | 640x480 conventional depth with lights off; 1920x1080 reversed and conventional with lights on; 4x MSAA in both depth modes; 640x480 reversed with 2x SSAA. Captures and clean exits, no detected shader or GL errors. This is the drop-pod opening, not the Pelican scene. |
| Xbox `b30` Pelicans | 640x480 conventional depth and 1920x1080 default reversed depth, plus reversed 4x MSAA. Captures show the Pelican's side light as a continuous glow at 1080p. |
| Available MCC `mcc_maps/a10.map` | Successful MCC conversion and rendered gameplay with default reversed depth/lights on and conventional depth/lights off, 1280x720. The local file contains a multiplayer scene; this is not evidence of completing an MCC campaign. |
| Map-family transitions | Xbox Blood Gulch -> MCC `a10` -> CE Death Island -> Xbox Blood Gulch in one process, default reversed depth and dynamic lighting on. Console commands, loading logs and captures confirm the transitions; clean exit. |

The native OpenGL debug log also contains driver shader-recompilation
performance messages. These captures are visual and lifecycle checks, not a
performance benchmark or proof of compatibility on every GPU. No new internet
transport code was introduced or transport-specific validation claimed.

Local evidence is retained under `build/lighting-compatibility/`,
`build/depth-runtime/`, `build/depth-transitions/` and the build/test logs in
`build/`. The early `pelican-*` labels without `b30` are the `a30` opening
checks described above. Staged maps and generated map caches were removed;
test saves were archived losslessly with verified manifests. Original maps,
saves and the user's working checkout were not changed.
