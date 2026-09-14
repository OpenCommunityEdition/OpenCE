# `model_animations.obj` Opus5 100K consolidated lane (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/models/model_animations.c`; January target
  `build/split/source/models/model_animations.obj`.
- Compiler: XDK 3911 VC7 `CL.Exe` 13.00.9254 from `xbox/bin/vc7`, repository flags
  `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` plus the generated include list.
- Lane `opus/100k-consolidated-20260914` at `b8cd50113`; worker claim
  `waveA:model_animations`. Only `model_animations.c` was modified. No header,
  configuration, parked manifest, build file, or other source was edited.
- Name/type provenance: January map atlas `scratch/atlas/model_animations.obj.txt`
  (exact tier, `_animation_update_internal` at 0x00111480); January assert strings
  in `.rdata` (`state`; `(animation_update_kind_affects_game_state==render_or_affects_game_state)...`
  used by the adjacent exact `animation_choose_random_permutation_internal`);
  HCEA `src/blam/models/animation_update_internal.c` and
  `src/headers/animation_update_result.h` (semantics, DB-verified result names; later build,
  never layout proof); Ghidra `scratch/ghidra/out/model_animations.obj.decomp.c:1013-1066`.

## 2. Validated baseline

Focused gate at lane HEAD (real file clean):

```text
python -B tools/campaign/gate.py source/models/model_animations --all --forbid-emitted-symbol _point_from_line3d
== exact 25  residual 6  unwritten 1  (of 32 listed)
```

6,688 / 12,144 padded code bytes exact. Non-exact rows:

| function | target padded | base padded | relocs | base nsha16 | status |
|---|---:|---:|---:|---|---|
| `_animation_get_keyframe_rotation` | 784 | 768 | 43/43 | b07863488ad7253e | parked |
| `_animation_get_keyframe_translation` | 768 | 752 | 37/37 | 2261009b94bd607e | parked |
| `_animation_get_keyframe_scale` | 752 | 736 | 37/37 | 013ee0c2d8e47636 | parked |
| `_inverse_kinematics_adjust_matrices` | 864 | 864 | 7/7 | d9fc11bebc1ea09f | parked |
| `_overlay_animation_apply_continuous` | 960 | 960 | 34/34 | 9f9b2604c3c535b6 | parked |
| `_overlay_animation_apply_continuous_scaled` | 1024 | 1008 | 37/37 | a35ac9998b7fb96b | parked |
| `_animation_update_internal` | 304 (300 meaningful) | absent | 8 | target 41e02539e6d17193 | unwritten |

Data owners were not changed by this lane apart from the pooled `state` assert literal
(see section 3).

## 3. Accepted controls (landed)

`animation_update_internal` was written as a complete, coherent body and landed as
**zero-credit fuzzy** under brief section 6:

- `#include "objects/objects.h"`: genuine owner of `struct animation_state`.
- TU-local complete `enum animation_update_result` (HCEA DB-verified names
  `_animation_running`, `_animation_key_frame`, `_animation_will_restart_on_next_frame`,
  `_animation_restarted`, `_animation_looped`) with a disclosure comment: no shared
  header declares it; `first_person_weapons.c` holds a partial copy.
- TU-local `struct animation_graph_sound_reference` (0x14, `struct tag_reference sound`
  at +0x0, index at +0xc) with a disclosure comment naming the identical copy in
  `first_person_weapons.c`.
- Body shape proved by January bytes: `tag_get` before the `state` assert (line 147);
  sound lookup `!=NONE && ==frame` then-lookup/else-`NONE`; `state->frame_index++`;
  end-of-clip branch first; loop frame clamp `MIN(loop, frame_count-1)`; restart via
  `animation_choose_random_permutation_internal(render_or_affects_game_state, ...,
  runtime_parent_animation_index)`; **single exit** with
  `short result = _animation_running;` and an else-if chain with no final else.

Landed gate: `== exact 25 residual 7 unwritten 0`. `_animation_update_internal` is
304/8, nsha a89af4bb25ab25b5. `alndiff` shows exactly **one** differing instruction:
`movsx edx, word ptr [ebp+8]` (ours) vs January `mov edx, dword ptr [ebp+8]`, caused
solely by the shared prototype declaring parameter 1 as `short`.

Checks: all 25 baseline-exact rows stay exact and every residual keeps its candidate
hash; `_point_from_line3d` guard passes; parked drift passes for all six parks
(size/relocs/nsha equal `measurements.base`); owner census adds only
`_animation_update_internal` (January owner) and `??_C@_05HDBKIDE@state?$AA@` (a literal
January references undefined); `tools/fake_match_scan.py`: 0 leads; `/W3` warning
count unchanged (27).

## 4. Experiment matrix

`animation_update_internal` (parameter 1 held at 32 bits through a scratch-only rename of
the header declaration, never landed; see `scratch/workers/model_animations/build_variant.py`):

| form | shape | result |
|---|---|---|
| f1 | result variable, if / else-if / else chain | 304/8, 3 insns differ: key-frame test laid out else-first |
| f2 | early returns | `mov ax,N` returns (single-exit law violated) + same layout diff |
| f3/f4 | f1/f2 with `==NONE ||` sound test | byte-identical to f1/f2 (neutral) |
| f6 | ternary for key-frame result | same as f1 |
| **f5** | `short result = _animation_running;` default, chain without final else | **EXACT** 304/8 nsha 41e02539e6d17193 |
| f5-short | f5 with the current `short` header | residual, one instruction (landed) |

Header measurement: shadow include directories
(`scratch/workers/model_animations/shadow_ctl` vs `shadow_new`, verified with
`-showIncludes`) compiled all 13 transitive consumers of
`model_animation_definitions.h` with the current and the proposed one-line prototype.
Per-function status, candidate normalized hash, size and relocation count were
identical for camera_scripting, devices, first_person_weapons, weapons (HEAD snapshot and
dirty lane copy), model_animation_definitions, objects, scenery, render_sky,
biped_limp_noodle, bipeds (HEAD snapshot and dirty copy), units (189/189) and vehicles;
model_animations gains `_animation_update_internal` exact (26/6/0) with parks unchanged.

Keyframe parks (scratch-only, parks not edited):

| shape | result |
|---|---|
| s1 `word const *keyframe_frame_indices` in the three getters | sizes 784/768/752 equal target |
| s2 + `first_keyframe_index` declared before `keyframe_count` | EBX / `[ebp+0x10]` assignment matches January |
| s3 + keyframe value pointer before frame-index pointer; scale `default_scales` before node header | residual scheduling only: scale 3 insns, translation 5, rotation 5 (nsha 4feb2973 / 31f7de61 / e72996c4) |

## 5. Do-not-repeat list

- Early-return spelling of `animation_update_internal` (emits 16-bit returns).
- Sound-test operand/negation order (neutral).
- Widening the `animation_update_internal` RETURN type (callers are exact with `short`).
- A definition whose parameter type disagrees with the shared prototype (ABI lie).
- Signed `short` keyframe frame-index pointers in the three getters (January reads the
  elements with `movzx`).

## 6. Residual classification

- `_animation_update_internal`: ABI width (measured). The only difference is the shared
  prototype's 16-bit parameter; the proposed 32-bit parameter is exact with zero consumer
  deltas.
- Keyframe getters: structural type fix measured (s1); what remains is register/move
  scheduling (inference: statement order inside the first interpolation branch).
- `_overlay_animation_apply_continuous_scaled`: observation only, frame `sub esp,0x80`
  vs ours `0x74` (12 bytes). By the frame-layout lever this is structural (a local's
  scope/type), not a tie. Not investigated; the park is locked.
- IK and `_overlay_animation_apply_continuous`: parked scheduling/allocation, unchanged.

## 7. Reopen criteria

- Orchestrator applies
  `short animation_update_internal(long render_or_affects_game_state, long animation_graph_index, struct animation_state *state, long *sound_index);`
  in `model_animation_definitions.h` together with the matching definition parameter
  type in `model_animations.c`; the landed body then gates exact.
- Keyframe parks: orchestrator unlocks them for a word-typed frame-index pass starting
  from `scratch/workers/model_animations/kf_s3b.c`.

## 8. Disposition

No strict-exact credit is claimed in this lane. `_animation_update_internal` is
zero-credit fuzzy and becomes exact after the header packet above. The six parks stay
parked and untouched (no drift). Final gate:

```text
python -B tools/campaign/gate.py source/models/model_animations --all --forbid-emitted-symbol _point_from_line3d --out scratch/workers/model_animations/final.obj
== exact 25  residual 7  unwritten 0  (of 32 listed)
```

## Header packet H1 (orchestrator, applied)

The parameter width of `animation_update_internal` is now `long render_or_affects_game_state`, in both `model_animation_definitions.h` and the definition. The 14 includers were rebuilt in a full ninja. Stable diff: +1 exact (`_animation_update_internal`, 304 padded), 0 regressions; parks 465 active / 0 stale / 0 invalid. The declaration count is unchanged: only a parameter type changed.
