# Devices and Game Sound small fuzzy harvest (2026-09-08)

## Result

This pass fills two previously unwritten functions with complete,
evidence-backed candidates while preserving every inherited strict match. It
adds **zero strict-exact bytes** and makes no exact-match claim for either
candidate.

| Unit | Before | After | Strict exact gain |
| --- | --- | --- | ---: |
| `source/devices/devices` | `27 exact / 1 residual / 2 unwritten` | `27 exact / 2 residual / 1 unwritten` | `0` |
| `source/sound/game_sound` | `27 exact / 3 residual / 1 unwritten` | `27 exact / 4 residual / 0 unwritten` | `0` |

## `_device_frontfacing`

The retained function follows the HCEA control/marker reconstruction and the
January CFG. It uses `control_try_and_get`, the owner enum
`_control_usable_from_both_sides_bit`, the typed `object_marker`, and the
three-component dot product. A natural result variable reproduces January's
single true-return tail and byte Boolean register lifetime.

| Evidence | Meaningful / padded | Instructions | Relocations |
| --- | ---: | ---: | ---: |
| January | `119 / 128` | `58` | `4` |
| Retained candidate | `119 / 128` | `58` | `4` |

The streams differ only in evaluation order of the independent J and K x87
products. The candidate spells the component expression directly, as the
HCEA reconstruction does, so it emits neither the incompatible standalone
`_dot_product3d` owner nor `_point_from_line3d`. Reordering commutative source
terms merely to steer those two instructions would not be original-source
evidence, so the function is parked fuzzy without credit.

The public control flag enum is placed in its true owner,
`source/devices/device_controls.h`. Direct consumer gates preserve all exact
functions in Devices (`27`), Device Controls (`11`), and HUD (`19`); no exact
regression occurs. The fake-match scan reports zero review leads.

`_device_render_debug` remains unwritten. Its already-proven natural body is
strict exact, but it also emits a forbidden `_point_from_line3d` COMDAT. The
existing boundary in
`docs/object_matching_logs/devices_obj_debug_renderer_inline_owner_boundary_20260904.md`
still applies: there is no new authentic inline-schedule evidence, and the
caller is not admitted by itself.

## `_compute_sound_obstruction`

The retained implementation combines January's three assertion sites,
collision-user stack discipline, exact `0xC0E1` flag composition, PVS and
encoded-distance gates, and terminal occlusion clamp with HCEA's explicit
three-component camera-to-source vector construction. The explicit
components are genuine donor evidence and avoid the previously demonstrated
duplicate `_vector_from_points3d` owner.

| Evidence | Meaningful / padded | Instructions | Relocations |
| --- | ---: | ---: | ---: |
| January | `498 / 512` | `161` | `33` |
| Retained candidate | `497 / 512` | `161` | `33` |

The remaining difference is the documented allocator family: January pins
`NONE` in `EBX` across the function and reuses it for assertion exits and
cluster comparisons, while VC7 rematerializes the constants in this natural
source. That changes prologue register ownership, the two independent cluster
loads/call-argument schedule, and the matching epilogue pop, but not behavior
or relocation identity. No synthetic local, register keyword, volatile,
assembly, control-flow distortion, or constant pinning was introduced. The
candidate is parked fuzzy with no exact credit.

The final Game Sound gate preserves all 27 inherited exact functions and
passes guards for both `_point_from_line3d` and `_vector_from_points3d`.
Fake-match scan and `git diff --check` are clean.

## Artifacts

- `scratch/devices-frontfacing-result-20260908.obj`
- `scratch/game-sound-obstruction-hcea-direct-20260908.obj`

No configuration, shared build, stage, commit, or push was performed.
