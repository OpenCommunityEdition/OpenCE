# `actors.obj` Opus5 150K lane, wave w3: second-pass law detectors (2026-09-14)

## Scope and provenance

- Translation unit `source/ai/actors.c`, target `build/split/source/ai/actors.obj`.
- Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, HEAD `210aa8f81`. `source/ai/actors.c` is unchanged since
  `5a84a96dd`. The worker run was cut off once; the resume re-gated the baseline and kept only verified notes. The real
  file was never modified.
- Compiler: XDK VC7 13.00.9254 `/O2 /Oy- /DDEBUG /Dxbox` (build.ninja, unchanged).
- Evidence:
  - January split COFF (function listings `scratch/workers/w3_actors/T_*.txt`);
  - `scratch/w2/laws_w2.md` laws A1-A22, used as detectors;
  - HCEA `blam/ai/actor_place.c`, `actors_spawn_from_unit.c` and `actor_input_update.c` (behaviour and topology only);
  - the previous scratch bodies `scratch/workers/actors/sfu1.c` and `iu1.c`, re-gated on HEAD;
  - `L:breakable_surfaces` younger-operand law (A22);
  - methodology item 8 (lvalue topology).
- Worker notes: `scratch/workers/w3_actors.md`. Scratch candidates: `scratch/workers/w3_actors/`.

## Baseline and final (real file)

`gate.py source/ai/actors --all --forbid-emitted-symbol _point_from_line3d`:

- **Baseline:** 70 exact, 6 residual, 0 unwritten. The point guard passes.
- **Final:** identical row for row (`scratch/workers/w3_actors/final_gate.txt`). The point guard passes.
- **Parks:** `scratch/parkcheck.py` reports 5 parks and 0 drift.
- **Owner census:** 182/182 owners against `build/base`, with none candidate-only and none base-only.
- **Fake scan:** 0 leads.
- **Diff:** `git diff --stat -- source/ai/actors.c` is empty.

| Function | Target size/relocs | Real size | Park | w3 outcome |
| --- | ---: | ---: | --- | --- |
| `_actor_create_for_unit` | 624 / 29 | 624 | register-allocation | skipped: no new law (do-not-repeat) |
| `_actor_customize_unit` | 640 / 26 | 672 | unclassified | skipped: no new law (do-not-repeat) |
| `_actors_spawn_from_unit` | 672 / 29 | 672 | instruction-scheduling | not landed; A22 evidence (sfuP) |
| `_actor_place` | 800 / 41 | 816 | unclassified | not landed; 4 shapes classified |
| `_actor_input_update` | 2384 / 89 | 2400 | unclassified | not landed; lvalue-topology hunk fixed (iu3) |
| `_actor_unit_control` | 784 / 48 | 784 | — | owner-blocked (units.c protected) |

## Law detector pass (all six rows)

| Law | Result |
| --- | --- |
| A1 static linkage | Not applicable. `actor_data`, `swarm_data` and `swarm_component_data` are January PDB publics. |
| A7 missing prototype | Not applicable. `scratch/w3/c4013.txt` has no actors.c entry. |
| A3 / A3c single exit and return width | Exhausted in earlier waves: create_for_unit (5 shapes), place (plA), spawn (sfu1). No short NONE epilogue. |
| A6 frame recipes | Place only. The +4 frame follows the register decision, not the scope (plS1 below). |
| A9 / A21 | No switch and no zero aggregate. |
| A10 conditional initializer | Place states. January's zero stores precede `actor_customize_unit`, which confirms the initializer spelling. |
| A12 / methodology item 8 (lvalue topology) | **Hit** in input_update: the swarm-centre copy (iu3). |
| A22 younger operand | **Hit** in spawn, where the launch operand order needs a `point_from_line3d` push-out (sfuP). That helper is forbidden in this lane. |
| A5, A11, A13-A17, A19, A20 | No fingerprint. place's field offsets were checked against the `ai_scenario_definitions.h` offset asserts; one-use aliases were checked on the bytes. |

## Per-function findings

### `_actors_spawn_from_unit`

`sfu1` (the w2 best body) re-gates on HEAD at 688/29, 211 against 207 instructions. Three differences remain:

1. **Swarm/actor selection.** January emits:
   - `cmp ecx,-1; jne actor` on the loaded swarm index;
   - a load of `actor_index` into ecx, then `cmp ecx,-1; je fake`;
   - the actor arm as the fall-through, with a reload of `actor_index`;
   - encounter and squad kept in cx/dx through the join, with `or eax,-1; cmp cx,ax` after it.

   Ours pins −1 in ecx before the tests, lays the fake arm as the fall-through, spills encounter and squad in each arm,
   and rematerialises `mov ecx,0xffffffff` at the tail of the actor arm.
2. **Launch.** January emits `fld forward.i; fmul speed_factor`. Ours emits the reverse.
3. **Alignment.** The only size effect comes from difference 1.

**A22 detector.** Fields read by name rank older than fields reached only through pointers. Our by-name push-out ages
`forward.i/j`. January's push-out x87 stream is the inline expansion of
`point_from_line3d(&position, &forward, 0.3f, &position)` after `position.z += 0.3f`.

**sfuP** (sfu1 with that call, gated without the guard):
- 688/29, sha 934af1c2;
- both launch hunks disappear and the push-out stays identical;
- only difference 1 remains;
- it emits a 48-byte `_point_from_line3d` COMDAT, which is forbidden in this lane.

### `_actor_input_update`

`iu1` re-gates on HEAD at 2384/89. Three differences remain:

- **(a) Store scheduling.** January emits the loop-guard compare before the third swarm-centre store.
- **(b) Aiming else arm.** January uses `lea edx,[ebx+0x1ec]` and keeps the unit in ebx until the looking copy. Ours
  destroys ebx with `add` and reloads the unit into eax at the join. That extra frame reference changes the unit slot
  from [ebp-8] to [ebp-4].
- **(c) Unit load before the threat loop.** January loads the unit into eax, ours into ebx.

In both builds, ebx=unit has no use between 0x56e and 0x6a6, so (b) is a single split decision.

**Shapes:**
- **iu3:** `swarm->swarm_center = *global_origin3d;` instead of `*swarm_center = ...`. This is the lvalue-topology form;
  the pointer local stays because it has 5 uses. Result 2384/89, sha f6ff0151. **(a) is fixed**; (b) and (c) are
  unchanged. All 70 exact rows are unchanged and the guard passes.
- **iu4:** iu3 plus A5 removal of the one-use alias `gunner_vehicle_definition`. Result sha 892db420. The only change is
  the x87 operand order inside the two inlined cross products: the second now matches January and the first no longer
  does. This is a lateral move (declaration-count sensitivity).

January field ages in the first cross product increase i < j < k independently of the vector.

**A5 candidates rejected on the bytes:**
- `actor_squad` / `vehicle_squad`: both calls precede both tests.
- `stored_prevehicle_encounter`: the field is read before the four input stores.

### `_actor_place`

The four shapes are from the cut-off run, re-read and classified here.

**January facts:**
- The frame is 0x190. swarm is at [ebp-8] and braindead at [ebp-4].
- unit_index is memory-only in `initial_variant`'s home [ebp+0x1c].
- initial_state is in `upgrade_major`'s home [ebp+0x18].
- default_state is in edi (zeroed before the customize call).
- actor_definition is in edi, and starting_location is in ebx from entry.

**Base:** starting_location is in edi, actor_definition is in memory, unit_index is in edi with its own cell (+4 frame),
and ebx is shrink-wrapped.

**Shapes:**

| Shape | Result |
| --- | --- |
| plT | Control, identical to base |
| lab_p2 (declaration swap) | Only the two zero stores swap, so stores follow declaration order. January's order matches base. |
| lab_p6 | Pointer declarations at the top of the block: inert |
| plS1 (states assigned after `actor_customize_unit`, HCEA order) | 800/41, sha 6e020ab1, size-equal. But the frame is 0x18c and the states sit in ecx/eax. This contradicts January's before-call zero stores. Not the source, and not exact. |

**Residual:** the ebx/edi priority among starting_location, actor_definition, unit_index and default_state.

### `_actor_create_for_unit` and `_actor_customize_unit`

Both are on the laws_w2 do-not-repeat list. The listings were re-read and every law was run as a detector. No
genuinely new law applies, so no shapes were spent.

- **create_for_unit:** January pins NONE in esi for the entry compares plus two distinct NONE exits, and shrink-wraps
  ebx. Ours uses imm8 compares and one merged exit.
- **customize_unit:** the callee-saved permutation is unchanged. The frame is identical at 0x98; the +32 bytes are the
  three unit reloads and the far jumps.

### `_actor_unit_control`

Unchanged. The only code hunk is the short `animation_impulse` push. This needs the units.h/units.c prototype change
(wave-D proposal 2), and units.c is protected.

## Orchestrator proposals

1. **Evidence for the existing C3 `_point_from_line3d` COMDAT owner ruling.** No header or config text.
   - **Scope:** list `_actors_spawn_from_unit` (672) as a PARTIAL.
   - **Evidence:** with the helper, the launch operand order matches (`scratch/workers/w3_actors/sfuP.c`).
   - **Limit:** the swarm/actor −1 pin residual remains, so the ruling alone would not close the function.
2. **Unchanged from w2:**
   - `actors.h` / `actor_create_for_unit` short `noncombat_sequence_id` (type fact; re-baseline the place park);
   - units.h/units.c short `animation_impulse` (owner-blocked).

## Do-not-repeat (additions)

| Function | Spellings that are inert, lateral or rejected |
| --- | --- |
| place | state initializers moved after `actor_customize_unit` (contradicts the bytes); declaration-order swaps; block-top pointer declarations |
| input_update | one-use `gunner_vehicle_definition` removal (lateral x87 permutation) |
| spawn_from_unit | `point_from_line3d` push-out (fixes the launch but is forbidden in this lane) |

## Reopen criteria

- **Parks:** unchanged. A future wave should start from:
  - `scratch/workers/w3_actors/iu3.c` (input_update, supersedes iu1);
  - `scratch/workers/w3_actors/sfuP.c` (spawn, once the point_from_line3d ruling exists; otherwise sfu1);
  - `scratch/workers/actors/ov/ncs.c` (place, with the type fact).
- **input_update:** needs a lever for the unit split at the aiming join.
- **spawn:** needs a lever for the −1 pin and arm layout.
- **place:** needs January local records or a same-compiler donor for the ebx/edi priority.
- **create_for_unit / customize_unit:** unchanged.

## Disposition

The object is still active at 70/76 strict. Nothing was landed and no fuzzy credit was taken.
