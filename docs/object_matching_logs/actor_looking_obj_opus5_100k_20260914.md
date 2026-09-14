# `actor_looking.obj` — Opus5 100K consolidated lane (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_looking.c`; target `build/split/source/ai/actor_looking.obj`
  (January 2002 Xbox debug build, sections in January file order 0x15E80–0x18940).
- Compiler: XDK 3911 VC7 13.00.9254 with the repository `build.ninja` flags, driven by
  `tools/campaign/gate.py` (whole-TU compile of the candidate file).
- Names: January map atlas `scratch/atlas/actor_looking.obj.txt` (all 16 owners already
  authentic in `config/symbols.json`; no renames pending). Semantics/topology: HCEA
  `src/blam/ai/actor_look_*.c` (later PPC build; never layout authority), HCEA
  `headers/primary_look_priority.h`, `actor_mode.h`, `actor_combat_status.h`, `idle_timer.h`.
- Ledgers read first: `actor_looking_obj_private_leaf_reconciliation_20260913.md`,
  `..._validity_owner_boundary_20260906.md`, `..._direction_decoder_natural_reconstruction_20260906.md`,
  `..._jonas_secondary_look_research_20260828.md`, `..._jonas_two_body_20260821.md`.
  `branch_sweep.py`: no historical blob better than lane HEAD.

## 2. Validated baseline and result

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| lane HEAD | 3 | 2 (both parked) | 11 |
| this landing (real file) | 4 | 4 (2 parked + 2 fuzzy) | 8 |
| measured proposal candidate (needs actors.h) | 9 | 7 | 0 |

Real-file landing: `_actor_looking_test_validity` strict EXACT (176 padded, 5 relocations,
normalized sha `af81bf40127acb18…`). `_actor_look_valid_aim_vector` (128/3, sha `4a0b82e0…`
vs `88d84360…`) and `_actor_look_valid_look_vector` (240/6, `bae54ff4…` vs `5b839957…`) are now
emitted as zero-credit fuzzy bodies with January size and relocation multiset. The three
inherited exacts are unchanged; both parks are unchanged (decode 1616/99 `bea2ec69…`,
affect_movement 128/4 `d096436e…`). `_point_from_line3d` guard passes; fake scan 0 leads.

The file is now written in January section order. Retained source-only statics (no emitted
caller until `actor_look_update` can be written): `actor_look_get_looking_definition`,
`actor_look_idle_find_prop`, `actor_look_idle_timer`, `actor_look_find_random_vector`,
`actor_look_idle_new_major_direction`, `actor_look_idle_new_minor_direction` — each is the
body measured in the full-graph candidate below.

New code owners in the candidate object are only header `__inline` math fallbacks
(`_normalize2d` — which January calls out of line from the look helper — plus `_magnitude2d`,
`_scale_vector2d`, `_dot_product2d`, `_cross_product2d`), the same systemic extra-COMDAT class
already present in this object's base and in 10–22 other base objects. No data/BSS/COMMON owner.

## 3. Accepted controls

- `actor_looking_test_validity`: typed actor/definition access; aim helper called with the
  input facing vector, look helper with a function-local `real_vector2d` of cosines selected by
  `actor->state.mode == _actor_mode_combat`. The actor-mode domain is a complete TU-local copy
  (values 0–4, DIA-verified) with a disclosure comment; no shared header declares it. This is
  new relative to the 2026-09-06 HOLD, whose blocker was a header-owner trial; no header is touched.
- `actor_look_valid_aim_vector`: January copies only the attempted vector into a 2D local and
  reads the facing vector through the pointer (EDX); ABI eax=attempted, edx=facing, stack yaw.
- `actor_look_valid_look_vector`: 2D copies of look/attempted vectors, aiming read through EDX;
  first `normalize2d` inline, second out of line exactly as January.

## 4. Experiment matrix (all whole-TU gates; siblings and parks re-checked each run)

| function | shape | result |
| --- | --- | --- |
| test_validity | first compile with facing copied in aim helper | residual (helper ABI moved facing to ECX) |
| test_validity | aim helper reads facing through pointer | EXACT, retained |
| aim | aggregate init / j-then-i copy / 2D struct-copy cast / set_real_vector2d / swapped param order / nested if / stored magnitude / cast `dot_product2d` / `(real)` wrapper | all [sha], reverted |
| look | per-field copies / set_real_vector2d / struct copy | [sha]; per-field retained |
| full graph (shim) | first compile | exact 6 (get_looking_definition, idle_timer, test_validity + inherited) |
| minor | MAX/MIN as call arguments → yaw_minimum/yaw_maximum locals | [sha] → EXACT |
| major | early return after random failure → single `found` flag | [sha] → EXACT |
| secondary | bounds inline → locals; table index `>=` → `boolean certain` local (also `(boolean)` cast) | [sha] → EXACT |
| find_prop | `||` valid → if/else; `(game_time - last)` → `(real)game_time - last`; ai_debug compare order ×2 | 624 → 608 [sha]; 1 instruction left |
| find_random_vector | do/while (retained old), while, do+break, for+return-inside, for+break (×3), for with flag condition (×2), `long` counter, else-branch flag | do/while/while call `_normalize3d` (544/34); `for` forms inline it (608/35); break/return forms move base_vector off EAX (breaks major/minor); flag-condition `for` keeps EAX but adds a loop-top test |
| update | asleep-first if/else (4736); `if(!enabled)` else (4736); `if(enabled)` else (4720, wrong layout); else-if can_look chain (setne reproduced); HCEA `goto update_facing` early copy (4688, January block layout) | relocation multiset identical (159) |

## 5. Do-not-repeat list

- aim/look magnitude product order: the nine spellings above; adding every January caller does
  not change it. A census of January split objects found only three i-first inline
  `normalize2d` sites (these two helpers and `ai_communication_event`).
- find_random_vector: the loop-keyword lever is measured (only `for` inlines the final
  `normalize3d`); do not re-sweep do/while/while.
- update: the block layout is solved by the early-copy `goto`; remaining differences are the
  zero-register/priority register swap and frame-slot permutation — no more slot sweeps
  without a census-based argument.
- Do not duplicate actor_move_force_stop / actor_combat_currently_firing_burst prototypes
  locally, and do not `#define` around `actor_look_secondary` in production (the shim exists
  only in the scratch proposal candidate).

## 6. Residual classification

- aim/look: x87 evaluation-order tie inside the header inline (fact: same size, relocations and
  instruction count; inference: tuple-order tie).
- find_prop: register scheduling (one `mov`/`cmp` pair).
- find_random_vector: loop-shape vs register-ABI tie (fact: no measured form gets both inline
  normalize and January's EAX/ESI roles without a loop-top test).
- update: register/slot tie after structural match (fact: equal relocation multiset, same
  frame size 0x58, 32 bytes of register-choice drift).
- Header ownership (OWNER-BLOCKED): update, secondary, and every static reached only from update.

## 7. Reopen criteria

- actors.h gains `boolean actor_move_force_stop(long)`, `boolean actor_combat_currently_firing_burst(long)`,
  `boolean actor_look_secondary(...)`, and the 8-value primary-priority enum (with the byte-neutral
  consumer renames in action_vehicle.c, action_converse.c and actor_looking.c). Then delete the two
  `SCRATCH-ONLY SHIM` regions from `scratch/workers/actor_looking/proposal_candidate_actor_looking.c`
  and land it (measured exact 9: +secondary 880, major 528, minor 368, idle_timer 352,
  get_looking_definition 80).
- aim/look: evidence of a different January inline normalize spelling for this TU.

## 8. Disposition

Still active. Credit this lane: `_actor_looking_test_validity` (176 padded) strict EXACT.
aim/look fuzzy (zero credit). Parks unchanged. Header proposal recorded in
`scratch/workers/actor_looking.md`.

## Orchestrator admission note (checkpoint 1)

The verifier accepted `_actor_looking_test_validity` (exact) and the two fuzzy vector validators. It rejected the real-file hunks that add or rewrite five uncalled statics (idle_find_prop, idle_new_major/minor_direction, idle_timer, find_random_vector): nothing in the TU emits or measures them. The orchestrator therefore kept the whole actor_looking landing out of checkpoint 1. The worker file is preserved as `scratch/orch/actor_looking_waveA.c`. It will be re-landed as one packet together with the actors.h header proposals (actor_move_force_stop / actor_combat_currently_firing_burst prototypes, boolean actor_look_secondary, primary-priority enum), which root the whole update graph.

## Header packet (orchestrator, checkpoint 4)

The actors.h packet landed as a separate commit, together with the profile.h duplicate-prototype cleanup that the units declaration-count tie required. The complete look graph then lands from the worker candidate, with the scratch shims removed and all statics rooted by `actor_look_update`. Full build: +6 exact (update helpers, secondary and test_validity; 2,384 padded), 0 regressions, parks clean. `actor_look_update` (4688 vs 4720), `idle_find_prop`, `find_random_vector` and the two vector validators stay fuzzy at zero credit.
