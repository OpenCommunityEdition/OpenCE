# Lightning tick-shape integration

This change incorporates [OpenCE PR 552](https://github.com/OpenCommunityEdition/OpenCE/pull/552),
commit `688eeb8a0a54da06103f0919bfdf7be0c16d952f`, with `git cherry-pick -x`
onto GulchCE `d0b597a59157d8ae4ca269df2aaad8f26b1d78e2`. The dedicated branch
is `fix-lightning-tick-shape`. The original author and commit provenance are
preserved. Validation below was performed on 5 November 2026.

## Scope and lifecycle correction

The inherited renderer consumes new local random numbers on every submission
of a lightning widget. That ties its flicker to rendering frequency, although
the game simulation advances at 30 ticks per second. The PR caches the seed
per lightning slot and split-screen window for a tick, retaining fresh marker
positions and camera-facing geometry each frame.

The follow-up clears that render-only cache on unload and after saved-state
restoration. A new regression test failed against the imported PR: the real
post-load callback list left the old seed usable when the restored tick and
slot matched. The fix uses a reset owned by `lightning.c` and one entry in the
existing post-load callback list. Map initialization and slot reuse retain
their resets. No persistent object layout, save format or map adapter changes.

This is an existing shared-renderer behavior fix, not an MCC conversion rule.
Xbox, Custom Edition and MCC keep their existing loading paths. No wire data,
simulation random seed or synchronized gameplay contract changes, so the
network version is unchanged. The additional reset cannot be handled solely
inside submission: restoring game state does not call `lightning_new`, and
tick and datum values alone cannot identify that restoration.

## Automated checks

- Windows x86 release built with LLVM clang 23.1.4 and
  `--portable --release --pgo=off --lto=off`. The build has existing Expat
  `LNK4217` import warnings; it is not a warning-free build.
- `python -m pytest -q tools/harness/tests/test_lightning_tick_shape.py
  tools/harness/tests/test_coop_vehicles.py tools/test_mcc_checkpoint.py
  tools/test_web_build.py`: 76 passed, 1 skipped. The skipped native socket
  harness needs POSIX pthreads and is explicitly skipped on Windows.
- The lightning portion has 17 checks, including six deliberate-fault
  controls. It covers tick zero, marker movement, all four views, simultaneous
  bolts and the last pool slot, allocation refusal, slot reuse, map reset,
  the actual post-load callback list, and missing-marker, texture and vertex
  allocation failures. The checkpoint test failed before the reset was added.
- `python tools/test_checkpoint_timing.py`: passed at its 30, 60, 120 and
  990 FPS schedules, including pause and time rewind.
- Tests used the installed local pytest dependencies and Windows x86 runtime
  libraries. The existing harness's Unix `-lm` argument was satisfied by the
  pre-existing empty compatibility `m.lib` in the local test toolchain; math
  implementations came from the Windows C runtime for these fake-world tests.

## Native rendering and transitions

Hidden 960x720 windows used OpenGL 4.5 on an NVIDIA RTX 5070 Ti, driver 617.42,
with V-sync off and isolated data/save folders. Original maps and saves were
not modified; no more than two game instances ran concurrently.

A production executable completed b40 (Xbox) -> deathisland (Custom Edition)
-> mercury_falling (MCC package) -> c10 (Xbox), accepted save/revert commands
on b40 and Mercury, and exited normally. Captures were inspected for CE,
MCC after revert, and the return to Xbox. This establishes bounded rendering
and transition checks, not complete campaign compatibility.

A temporary diagnostic build instantiated b40's authored bridge-lightning
object near the camera, selected its marker-based view and logged seeds and
submitted vertex hashes. It bypassed camera interpolation only for this
diagnostic view; production sources were restored after building the probe.
The scene is staged, not a traversal to the campaign's original bridge.

| FPS cap | Draws after initial camera setup | Repeated tick/slot/view groups | Seed or geometry changes within those groups |
| --- | ---: | ---: | ---: |
| 30 | 1516 | 0 | 0 |
| 60 | 2956 | 1450 | 0 |
| 144 | 7216 | 1504 | 0 |

Bridge captures were inspected at 30 and 144 FPS. A further 144 FPS run moved
the diagnostic camera, held game speed at zero for three seconds, resumed,
and reverted a checkpoint. It exited normally; across 1322 repeated groups,
seeds stayed stable within each restored-state epoch while geometry changed
with the moving camera. Up to 449 draws reused one tick's seed during the
time hold. The reset hook ran again after revert. Tick numbers were grouped
separately across resets because a revert legitimately revisits earlier ticks.

## Remaining limits and retained evidence

`ninja android` and `ninja web` were attempted but reported unknown targets:
configure found neither an Android NDK nor Emscripten. WSL is not installed.
Full Android/web builds and Linux execution remain unverified. The web graph
tests above do not substitute for a wasm link or browser gameplay.

c10 loaded and rendered, but the staged monitor probe produced no lightning
draws at 30, 60 or 144 FPS. Guilty Spark's tendril appearance therefore remains
unverified. Real split-screen gameplay, device/browser rendering, and network
sessions were not exercised. Four-view seed isolation is harness coverage.

Local `build/validation` holds build/test logs, runtime transcripts, probe
scripts and manifests, and `capture-results.json`. Captures and inactive test
saves are losslessly archived in `captures-and-saves.zip`; its manifest records
original paths, sizes and SHA-256 hashes, verified against every ZIP member.
Temporary map copies/links, game caches and duplicate executables were removed.
The production Windows build remains available. No retail assets are committed.
