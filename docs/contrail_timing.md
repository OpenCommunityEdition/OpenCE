# Projectile contrail timing

[Issue 84](https://github.com/chestahh/GulchCE/issues/84) reports thinner,
dimmer Covenant turret trails above 30 FPS. This is a fix to the existing
shared effect implementation, not an MCC data conversion or a new feature.

## Cause and correction

Before GulchCE's original fix, `contrails_update` in
[contrails.c](../source/effects/contrails.c) batched point emission using
accumulated frame time, but passed only the current frame time to
`contrail_update_points` for aging existing points.

Point-state transitions reset their normalized time to zero, discarding
overshoot. Consequently, a short state lasts until the next update boundary.
A local inspection of Xbox b40's `vehicles\c gun turret\c gun turret`
contrail found a 30-points/second rate, a 0.01-second state at width 0.15 and
alpha 1, then a 0.05-second transition to width/alpha zero. Per-frame aging
shortened that bright body as render frequency increased.

The correction supplies the emission interval as a separate state-aging
argument. The 30 Hz transition behavior remains; point physics, bitmap
sequence animation and texture scrolling still use frame time. Calling the
point updater with zero state time remains necessary: a point created by an
attachment change or collision must have a valid state before it is rendered.
The original fix also reset the emission clock on map initialization and disposal.

This changes no tags, loader, map-family dispatch, projectile simulation,
network messages or shared random seed. Contrail state durations use the
local random stream. No network version change is needed. Xbox, Custom Edition
and MCC keep their existing loaders and use the same effect timing afterward.
No feature-owned hook is necessary for a correction within this implementation.

## Integration with OpenCE PR 546

The November upstream integration retains GulchCE's separate state-aging and
per-frame motion arguments. It adopts upstream's nominal tick duration for
emission and aging: 1/30 second per elapsed game tick, halved in double-speed
mode. At 144 FPS, four or five render frames can fall between ticks; their
accumulated time therefore varies even though each game tick has equal duration.
Nominal tick time removes that variation in short bright states.

When a render frame spans several ticks, existing point states advance one
tick at a time because each transition discards the remainder of its step.
Point physics still runs once with the frame delta. This does not reconstruct
historical attachment positions or reproduce every intermediate emission at
low frame rates. New points still initialize when state time is zero.

`contrails_reset_render_state` owns the clock reset. Map initialization,
disposal and the existing `after_load_procs` table in
[game_state.c](../source/saved%20games/game_state.c) call it. A checkpoint can
restore a different point pool at the same tick, so comparing tick numbers
alone cannot invalidate the old clock. This is a general saved-state lifecycle
fix, shared by existing map families; it adds no MCC conversion logic.

The [original harness](../tools/harness/tests/test_contrails.py) now also checks
same-tick checkpoint restoration through the real callback table, two-tick
state aging, and double speed. The
[upstream harness](../tools/harness/tests/test_contrail_tick_points.py) checks
steady 144 FPS, uneven frame intervals and time jumps with real point emission.
Both compile production functions; attachment/physics stubs do not establish
visual equivalence or full map compatibility.

## Validation on Windows, 1 November 2026

Base: `origin/testing` at `6303ff6a0847525d385bcb9aca5fed518b211130`.
Build: portable release, PGO off, LTO off; LLVM 23.1.4, OpenGL, hidden windows.

- [Real-code harness](../tools/harness/tests/test_contrails.py): nine tests,
  including identical tick-boundary point counts/widths at 30/60/120/240 FPS,
  a non-aligned 144 FPS schedule, zero-time pause, attachment reactivation,
  initialization between ticks, detached cleanup and map-clock resets.
  Deliberately restoring per-frame aging fails both rate checks; removing
  clock resets fails the lifecycle check. Attachment placement and physics
  are stubs; this is not a rendering or map-loader test.
- Xbox b40: before/after captures at 30 and 120 FPS, 640x480, Smooth Motion
  enabled, optional lighting disabled. The existing capture fixture moves
  three Shade projectiles along a controlled path near the camera. Visual
  inspection shows the bright trail retained at 120 FPS, matching the
  substantial bright body at 30 FPS. These are not pixel-identical frames:
  simulation/render phase and emission positions can differ. Measured rates
  were approximately 30.00 and 120.01 FPS for the corrected captures.
- Custom Edition Death Island: three Ghost projectile captures at about
  119.92 FPS, same renderer/settings, successful exit. This checks a second
  trail and the existing CE path, not every CE contrail definition.
- Final combined regression run (contrails, BMP readers, cache formats and
  web build checks): 203 passed, five skipped.
- Windows build passed. Full `ninja android` and `ninja web` were attempted
  but their generated shell commands require unavailable Unix utilities on
  this Windows host. The changed translation unit compiled successfully
  with the generated arm64_32 Android and wasm32 flags; Android assembly
  conversion and AArch64 assembly also passed. Full platform links/runtime
  remain unverified. Linux/WSL is unavailable here.
- MCC DigsiteMP maps 1 and 10 converted and loaded their tags, then refused
  the `host_spawn` script's unsupported `debug_camera_load` call. The smoke
  runner stopped before its simulation-advance check. MCC gameplay and real
  cross-family transitions therefore remain unverified; those script paths
  were not changed for this issue.

The original b40 free-flight fixture hit nearby scenery too soon for a useful
trail comparison; the controlled-path captures above replaced it. Blood Gulch
had no requested multiplayer Shade tag, so those attempts are not validation.

Self-review covered the entire source change, its sole private caller,
initialization/disposal, point cleanup, renderer state access, tests and this
reading route. Per-frame physics was deliberately retained when separating
state time. No unrelated refactoring was needed. This preserves the engine's
existing tick-batched emission and state quantization; it does not introduce
sub-tick trail interpolation or solve low-FPS/catch-up sampling in general.

Local logs, capture manifests and compressed images are retained under the
worktree's ignored `build/issue84-evidence`. Test map copies and generated map
caches are removed; test saves and captures are preserved losslessly.
