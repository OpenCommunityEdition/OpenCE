# `game_sound.obj` Opus 5 150K house-clean lane, wave w2 (2026-09-14)

## Scope and provenance

- Translation unit `source/sound/game_sound.c`; target `build/split/source/sound/game_sound.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/150k-house-clean-20260914`, wave w2 worker (files: bink_playback.c, game_sound.c,
  sound_dsound_xbox.c). No header, config, park or build edits.
- Ledgers read first: `game_sound_obj_obstruction_current_owner_boundary_20260905`,
  `devices_game_sound_small_fuzzy_harvest_20260908`, `game_sound_obj_pas_current_context_integration_20260904`,
  `game_sound_obj_pas_independent_review_20260904`, `game_sound_obj_jonas_public_leaf_pair_20260828` and the
  other `game_sound_obj_*` ledgers; the 100K handoff. `git log --all` (5d3444f79 ... e830efbd7) and
  `branch_sweep.py` (13 blobs, none better than current 28/3/0).
- Worker notes: `scratch/workers/bink_playback_game_sound_etc.md`; candidates in
  `scratch/workers/bink_playback_game_sound_etc/`.

## Baseline and final

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (point guard passed) | 28 | 3 | 0 |
| final real file (unchanged, guard passed) | 28 | 3 | 0 |

`git diff --stat -- source/sound/game_sound.c` is empty. Both parks drift 0; owner census equals `build/base`;
fake-match scan 0 leads.

## Per-function outcomes

### `_compute_sound_obstruction` (512 padded / 498 meaningful / 33 relocs) — strict EXACT in scratch, owner-blocked

Baseline real file: 496 / 33 / `13363fc7c551b842`. Scratch candidate `gs_v6.c`: **512 / 33 /
`2040bfc2f77a485e` = target**, whole-TU gate 29/2/0 with only this row changing, point guard passed, parks drift 0,
fake scan 0.

January evidence and the shapes that close it:

1. Assert literal `??_C@_0EB@JEFKCLHK@` is `source->spatialization_mode==_sound_spatialization_mode_absolute`
   (no spaces). The real file stringifies a spaced expression (`_0ED@OGIIIPOH`), a wrong relocation target.
2. `movzx edx,al; and edx,0xffffff7f` is `& ~FLAG(_cluster_sound_distance_unreachable_bit)`, the spelling the
   exact sibling `compute_combined_pas` already uses; `& CLUSTER_SOUND_DISTANCE_VALUE_MASK` narrows to
   `and al,0x7f` (16 bytes short).
3. Store/call-order law: the 0.6/1.0 obstruction and occlusion stores sit between the 887 assert's compare and
   branch, before its `display_assert` call, so the stores precede the assert in source.
4. January calls `global_structure_bsp_get()` before loading either cluster-index argument of the encoded-distance
   lookup, but pushes the PVS cluster argument before its own `global_structure_bsp_get()` call. The distance lookup's
   BSP is therefore a block-local `struct structure_bsp *structure_bsp = global_structure_bsp_get();`; the PVS
   lookup re-fetches inline. Binding the local removes the zero-extension CSE, which is what produced the documented
   "NONE pinned in EBX" allocator difference (`or ebx,-1` is now reproduced).
5. `fld [source.x]` after the 0.45 store is the inline `vector_from_points3d(&camera->position,
   &source->location.position, &vector)` schedule, which was the 2026-09-05 natural spelling. Explicit components
   hoist the load.

Rejected or intermediate shapes:

| shape | result |
| --- | --- |
| v1 | literal only: 496 |
| v2 | + mask: 512, sha differs |
| v3 | + store order: stores match |
| v4 | + BSP local: one fld position left |
| v5 | `byte` distance local instead of BSP local: pin lost |
| v7 | helper without the BSP local: pin lost |

**Blocker:** checklist item 5. The candidate emits `_vector_from_points3d` (a COMDAT January does not
reference). This is the systemic header-inline surplus COMDAT: `build/base` already emits it with selection 2 (ANY)
in **75 objects**. Only csplit's January `action_alert.obj` copy has selection 1 (NODUPLICATES), which is why the
2026-09-05 link probe failed. That probe would fail identically for all 75 existing base objects. See
ORCHESTRATOR-PROPOSAL below.

### `_update_potentially_audible_looping_sound` (624, PARKED) — structurally closed in scratch, not exact

January control flow has one shared bottom tail (`last_audible_frame_index = update_index`), one shared
`state = inactive` store reached from three edges, and one shared `game_looping_sound_delete`. The early
`cmp ax,2; je tail; test serviced; jne main; cmp ax,2; je tail; store` is the jump-threaded fingerprint of
`if (audible || (state != inactive && serviced)) {...} else if (state != inactive) state = inactive;`.
That is the **single-exit law**; the real body has six returns. `audible` is also computed before the
`source.scale` store.

| shape | result |
| --- | --- |
| `pas_v1` single-exit restructure | 624 / 25 |
| `pas_v2` + statement order | 624 / 25 / `f3008036d008e7f4` |

What remains in `pas_v2` is only the zero-register pin: January hoists `xor ecx,ecx` before the object_index test,
re-zeroes it after the object path, and spends it on spatialization none, the `state == active` compare, the `0.f`
push and the `state = active` store. The park stays; `pas_v2.c` is the new starting point.

### `_game_sound_update` (464, PARKED) — confirmed register-colour tie

A register-masked alignment leaves one structural pair: the else-if re-fetch loads `game_looping_sound_data` into a
second register instead of reusing EAX after the push. Everything else is a volatile EAX/ECX/EDX permutation.

| shape | result |
| --- | --- |
| s1: `SET_FLAG` directly on the accessor | worse (8 structural rows) |
| s2: separate block-local re-fetch | byte-identical |

The park premise holds.

## ORCHESTRATOR-PROPOSAL (census ruling, no header/config text)

Admit the systemic header-inline surplus COMDAT class for `_compute_sound_obstruction`:

- Evidence: 75 `build/base` objects (actions, action_alert, action_charge, ..., players, hs_runtime, ...) already
  emit `_vector_from_points3d` with selection ANY.
- Consumers: none beyond game_sound.obj.
- If admitted, apply the `compute_sound_obstruction` body from
  `scratch/workers/bink_playback_game_sound_etc/gs_v6.c`: +512 padded / 498 meaningful, no other change.

## Reopen criteria

| function | criterion |
| --- | --- |
| `_compute_sound_obstruction` | the census ruling above |
| `_update_potentially_audible_looping_sound` | a source lever for the zero-register pin, starting from `pas_v2.c` |
| `_game_sound_update` | authentic evidence for the re-fetch statement |
