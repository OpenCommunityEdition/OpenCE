# `motion_sensor.obj` — opus5 next-150K lane, wave n4 (2026-09-15), RELOCATION-CENSUS PASS

Unit `source/interface/motion_sensor`. Worker scope: `source/interface/motion_sensor.c` only.
Assigned target: `_motion_sensor_update` (structural, PARKED — scratch only).
**The real file is unchanged** (`git diff --stat -- source/interface/motion_sensor.c` is empty).
Final real-file gate: `== exact 14  residual 5  unwritten 0`, identical to baseline; the
`_point_from_line3d` guard passes; `scratch/parkcheck.py` reports `parks 5 drift 0` against the real
file; `tools/fake_match_scan.py` reports 0 leads.

Worker notes and every probe: `scratch/workers/n4_motion_sensor.md`, `scratch/workers/n4_motion_sensor/`.

## Baseline

| Function | January padded / relocs | Parked body | Gate tag |
| --- | ---: | ---: | --- |
| `_blip_begin` | 112 / 10 | 112 / 10 | sha |
| `_motion_sensor_update` | 1168 / 43 | 1136 / 44 | size, relocs, sha |
| `_render_blip` | 400 / 18 | 384 / 17 | size, relocs, sha |
| `_render_motion_sensor` | 768 / 40 | 768 / 40 | sha |
| `_update_motion_sensor` | 864 / 39 | 816 / 37 | size, relocs, sha |

Duplicate prevention: the four prior ledgers (`..._opus5_next150_n3a_20260915.md`,
`..._opus5_250k_w3c_20260915.md`, `..._opus5_150k_w1_20260914.md`,
`..._jonas_census_tick_one_shot_20260829.md`), laws_w2 and laws_w3 in full, both WORKER_BRIEFs,
`git log --all --oneline -- source/interface/motion_sensor.c` (last body change `800179a20`, byte-equal
to the current file) and `branch_sweep.py` (11 unique blobs; best historical blob `exact 11`, the
current file at `exact 14` is the best that exists).

## 1. `_motion_sensor_update` — PARK-REOPEN PROPOSED (strict EXACT candidate, decoupled)

### 1.1 What the relocation census actually says

`relocdiff --count-by-target --allow-structural`, real file vs January, differing rows only:

| target | JAN | ours |
| --- | ---: | ---: |
| `_blip_type_get` | 1 | 0 |
| `_object_get_and_verify_type` | 1 | 0 |
| `_tag_get` | 1 | 0 |
| `_object_try_and_get_and_verify_type` | 2 | 1 |
| `_motion_sensor_blip_set_type_and_size` | 0 | 1 |
| `_local_player_get_player_index` | 2 | 4 |
| `_player_data` | 1 | 2 |
| `_datum_get` | 1 | 2 |

Two independent findings, in the orchestrator's terms:

1. **Not a missing statement — a missing INLINE.** January's `_blip_type_get` /
   `_object_try_and_get_and_verify_type` / `_object_get_and_verify_type` / `_tag_get` run at target
   `+0x33d..+0x372` and *are* the body of `motion_sensor_blip_set_type_and_size`, which we call out of
   line. That helper is a separate, already-EXACT 96-byte row in the same object, so January emits both a
   standalone copy and an inline one; the reconstruction has to present the same source and let VC7
   choose.
2. **An EXTRA statement on our side.** Our second `local_player_get_player_index` / `_player_data` /
   `_datum_get` triple is the object-loop re-validation
   `if (player_index != NONE && player_get(...)->unit_index != NONE)`. January's object loop has no such
   test: `+0x290..+0x2a7` goes straight from `local_player_indices[i]` to `blip_counts[spi] < 0x10`.

### 1.2 Frame census (structural first)

January `sub esp,0x284`; parked body `sub esp,0x270`.

| January slot | size | meaning (from the bytes) | parked body |
| --- | ---: | --- | --- |
| `[ebp-0x284]` | 0x200 | `long stack_buffer[0x80]` sentinel | same |
| `[ebp-0x84]` | 0x30 | `real_point3d camera_positions[MAXIMUM_LOCAL_PLAYERS]` (`lea ecx,[ebp+edx*4-0x84]`, edx = lpi*3; the object loop uses `esi = spi*12`) | `camera_positions[2]` (0x18) |
| `[ebp-0x54]` | 8 | `short local_player_indices[4]`, walked by the `[ebp-0xc]` cursor (`add esi,2`) | folded into `local_player_slots[4]` at `+2` |
| `[ebp-0x28]` | 8 | `short blip_counts[4] = {0}` (`mov [ebp-0x26],ecx; mov word [ebp-0x28],0; mov word [ebp-0x22],cx`) | folded into `local_player_slots[4]` at `+0` |
| `[ebp-0x4c]` | 0xc | `real_point3d object_position` | same |
| `[ebp-1]` | 1 | `boolean done` (`mov byte [ebp-1],0` at `+0x5e`) | same |
| — | — | **no cell for a displacement vector** | `real_vector3d displacement` (0x10) |

0x284 − 0x270 = 0x14 = +0x18 (wider camera array) + 0x8 (two arrays instead of one packed one)
− 0x10 (the displacement cell we add). The gap closes exactly on three source defects and nothing else.
`MAXIMUM_LOCAL_PLAYERS` is 4 (`source/game/players.h:22`).

### 1.3 January-side evidence

Target disassembly `scratch/workers/n4_motion_sensor/T_msu.txt`.

* `+0x19c..+0x1bb`: `cmp eax,-1; mov ecx,[ebp-0xc]; mov word [ecx],bx; je +0x1bb; lea edx,[edi+edi*2];
  lea ecx,[ebp+edx*4-0x84]; push; push; call _unit_get_camera_position; add esp,8; mov [esi+0x78],0`.
  The `local_player_indices` store is unconditional and the cursor advance at `+0x1dc` is outside the
  `if`, so the index array is filled for every local player, while `camera_positions[]` is written only
  when the unit index is valid. The entry loop is accounted for instruction by instruction: **it contains
  no store to `camera_positions[]`.**
* `+0x2b3`: `fld dword [ebp+esi-0x7c]; fstp dword [ebp-0x44]` = `object_position.z =
  camera_positions[spi].z;` — the sensor range test is deliberately horizontal.
* `+0x2c3..+0x302`: `fld obj.x; fsub cam.x; fld obj.y; fsub [eax+4]; fld obj.z; fsub [eax+8];
  fld [hud_globals+0x2d0]; fld; fmul; fld; fmul; faddp; fld; fmul; faddp; fld; fmul; fcompp` — a single
  expression summed k,j,i (laws_w3 A35(2)) over `object - camera` (laws_w2 A15 role order), i.e.
  `distance_squared3d(&camera_positions[spi], &object_position)` (`source/math/real_math.h:1177`).
* HCEA corroboration, `halocea-review/src/blam/interface/motion_sensor_update.c` (2011 PPC, semantics
  only): its two "FAITHFUL QUIRK" notes — "the Z term ... reads the SAME address for both operands ...
  the range check is effectively 2D" and "The object's own Z position is computed ... but never consulted
  anywhere in the function" — are exactly what that one assignment produces at source level.
* HCEA **divergence**, which is the evidence for the original-bug ruling: the later shipping source *does*
  zero `camera_position[next].{x,y,z}` in the entry loop and *does* re-test
  `local_player_get_player_index(scan_player) != -1 && ...->unit_index != -1` in the object loop.
  January has neither. Our HEAD body transcribes the later, fixed shape — including HCEA's
  decompiler-inferred packed `local_player_slots[4]` with its `+2` offsets and the 2-element camera array
  — and January's bytes refute all of it.

### 1.4 Shapes (3 spent; budget 5)

| shape | change | result |
| --- | --- | --- |
| `m1.c` | current real file + January's body: `camera_positions[MAXIMUM_LOCAL_PLAYERS]`, `short local_player_indices[4]`, `short blip_counts[4] = {0}`; no entry zero-init; no object-loop re-validation; `long players_full`; `object_position.z = camera_positions[spi].z;` then `distance_squared3d(...)`; `blip_counts[spi]++` before `sensor->blip_count++`; `boolean done;` with a separate `done = FALSE;` (laws_w3 A38 — January stores `mov byte [ebp-1],0` at `+0x5e`) | **1168 / 43 strict EXACT**; `relocdiff --count-by-target` → 0 differing rows |
| `m2.c` | as `m1` but the helper hand-expanded to `real_vector3d displacement; displacement.i/.j/.k = ...; magnitude_squared3d(&displacement)` (our HEAD form) — a direct test of whether the R14 COMDAT pair is avoidable | **1184, `[size]`** — the named vector takes a frame cell, +0x10, exactly as the frame census predicted |
| `m3.c` | as `m1` but the entry loop respelled as `for (i = 0; i < player_count; i++)` | 1168 but `[sha]`; `alndiff` leaves ONE hunk — January advances the cursor (`mov esi,[ebp-0xc]; add esi,2; mov [ebp-0xc],esi`) **before** `call _local_player_get_next`, `m3` after it |
| `m4.c` | `m1` plus the docs' `/* BUG (preserved for exact matching): ... */` disclosure and a one-line comment on the flatten | **1168 / 43 strict EXACT** — the proposal candidate |

`m3` is a useful negative: a `for` increment clause is emitted after the body (laws_w3 A25), so it can
never precede the `local_player_get_next` call. By the STORE/CALL-ORDER law (WORKER_BRIEF §9) January's
bytes therefore *prove* the source writes `player_scan_index++;` as its own statement ahead of the
`local_player_index = local_player_get_next(...)` assignment. The while-plus-explicit-increment spelling
is byte-proven, not an allocation steer.

### 1.5 New law candidate — INLINE-HELPER TEMPORARY OWNS NO FRAME CELL

`distance_squared3d`'s internal `real_vector3d v` is fully forwarded when the helper inlines (no stack
home). The identical arithmetic written with a caller-level named `real_vector3d` is homed (+0x10 here,
`m2`). So a frame that is exactly one vector cell short of the hand-expanded form, with otherwise
identical instructions, proves the source called the helper. This upgrades the R14 question in this unit
from a style preference to a mechanical necessity: **no spelling of `_motion_sensor_update` reaches
January's frame without the `_vector_from_points3d` / `_distance_squared3d` COMDAT pair.**

### 1.6 Checks

Whole-TU gate of `m4.c`: `== exact 15  residual 4  unwritten 0`; all 14 baseline-EXACT rows preserved
(listings diffed row by row). `--forbid-emitted-symbol _point_from_line3d` passes.
`scratch/parkcheck.py source/interface/motion_sensor scratch/workers/n4_motion_sensor/m4.obj` →
`parks 5 drift 1`, the single drift being `_motion_sensor_update` itself (expected for a reopen);
`_blip_begin` 112/10, `_render_blip` 384/17, `_render_motion_sensor` 768/40, `_update_motion_sensor`
816/37 all unchanged. `tools/fake_match_scan.py` → 0 leads. Owner census vs `build/base`: candidate-only
`_distance_squared3d`, `_vector_from_points3d`; base-only: none.

## 2. Rows not targeted this wave

* **`_blip_begin` (112)** — SKIPPED-EXHAUSTED here; n3a's reopen proposal (`w3c_motion_sensor/bb1.c`,
  coupled with the `_render_motion_sensor` re-baseline 768/40 → 784/40 sha `de9701add36aae94`) stands
  unchanged and is blocked only on the owner ruling for the never-read second parameter. **n4's
  contribution: it is no longer entangled with `_motion_sensor_update`** — n3a's only exact body for
  `_motion_sensor_update` (`motion_sensor/v13.c`) carried the `_blip_begin` ABI change as well, so the
  two reopens had to be taken or refused together. `m4.c` separates them.
* **`_render_blip` (400)** — census re-run: 18 vs 17, one extra `__real@3f800000` on January's side plus
  two literal-precision swaps (`3fe6666666666666` vs our widened `3fe6666660000000`; `3dd6774d` vs
  `3dd67750`). Not a missing statement in this `.c`: n3a's `rb6.c` reaches 400 / 18 under the laws_w3
  **E7** shadow `real_math.h`. Header-gated (a worker may not edit headers), and n3a measured that E7 has
  no count-neutral compensation.
* **`_render_motion_sensor` (768)** — census shows address shift only, no missing or extra call; 15+
  shapes spent across w1/w3c with a clear scheduler-tie verdict. Outside this wave's scope.
* **`_update_motion_sensor` (864)** — census differences are the two `__real@00000000` loads (the folded
  `z-z` term, already explained by n3a) and symbol-vs-section spelling artifacts; **no missing or extra
  call**. Structurally closed by `w3c_motion_sensor/u7.c` (864/39, 285 vs 285 instructions, one 8-byte
  hunk); the residual lives inside `magnitude_squared3d` (`real_math.h:1123`), and its exact form needs
  the same R14 ruling. Outside this wave's scope.

## Proposals

1. **Park reopen — `_motion_sensor_update` (1,168 B).** Candidate
   `scratch/workers/n4_motion_sensor/m4.c` = the current real file plus this one body, strict EXACT,
   relocation multiset identical, zero sibling loss, `parks 5 drift 1`. The recorded park criterion
   ("January emits the otherwise-discarded `game_engine_running` call") was already refuted by n3a; this
   wave replaces it with the measured cause: a missing inline of
   `motion_sensor_blip_set_type_and_size` plus an invented per-object re-validation.
   Requires two rulings:
   * **Original-bug ruling (uninitialised read).** WORKER_BRIEF §5 forbids preserving an uninitialised
     read without one; `m4.c` carries the `/* BUG (preserved for exact matching): ... */` comment in the
     form `docs/matching_methodology.md` specifies. For the owner: the choice is not "add a bug" versus
     "clean code" — our HEAD body avoids the read only by inventing three zero stores and a
     four-relocation re-validation that January does not have, i.e. by importing the 2011 shipping fix
     into a January reconstruction.
   * **R14 `_vector_from_points3d` census ruling.** Now shown mechanically unavoidable (§1.5), and in the
     same csplit-attribution class as the surplus this object already emits at HEAD with 14 exact rows.
     Measured over all objects: `build/split` owns each helper exactly once (`_vector_from_points3d` 1,
     `_distance_squared3d` 1, `_magnitude_squared3d` 1, `_normalize3d` 1, `_scale_vector3d` 1), while
     `build/base` emits `_magnitude_squared3d` in 91 objects, `_normalize3d` 60, `_scale_vector3d` 61,
     `_vector_from_points3d` 75, `_distance_squared3d` 33. `_magnitude_squared3d` is already an accepted
     laws_w2 A4 / laws_w3 A30 systemic disclosure and is already in this object's surplus set.
     Unit scope remains 2,032 padded B (`_motion_sensor_update` 1,168 + `_update_motion_sensor` 864).
2. **parked.json evidence text** for `_motion_sensor_update`: replace the `game_engine_running` criterion
   with "January inlines `motion_sensor_blip_set_type_and_size` and has no per-object player
   re-validation; the body reads `camera_positions[]` uninitialised for a local player without a unit".
3. **Law for the next laws file:** INLINE-HELPER TEMPORARY OWNS NO FRAME CELL (§1.5) — a detector for
   deciding helper-call versus hand-expansion from the frame alone.
4. n3a's proposals 1, 4 and 5 stand unchanged (`_blip_begin` reopen; E7 re-evaluation; the
   `_update_motion_sensor` parked.json text).

## Reopen criteria

* `_motion_sensor_update`: the two rulings in proposal 1. With them, land `m4.c` and unpark in one
  commit; no other park in this unit moves.
* `_blip_begin`: owner ruling on the unused parameter's name and type (n3a).
* `_update_motion_sensor`: the R14 ruling plus a spelling in which the folded-zero `k` term is not
  reassociated into the first partial sum (start from `w3c/u7.c`) (n3a).
* `_render_blip`: E7 landed together with re-closures of its three measured victims (n3a).
* `_render_motion_sensor`: a spelling that emits `sub eax, dword ptr [ebp-0x18]` as a memory operand and
  finishes the integer sensor-index statement before the x87 weight chain (n3a).
