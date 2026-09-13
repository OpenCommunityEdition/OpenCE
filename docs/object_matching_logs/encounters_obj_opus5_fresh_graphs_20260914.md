# `encounters.obj` Opus5 fresh-graphs lane (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/encounters.c`, target `build/split/source/ai/encounters.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254 / XDK 3911, project flags
  `/nologo /c /O2 /Oy-`, no per-file override).
- Lane `opus/30k-fresh-graphs-20260914`, HEAD `47d9dd0da` (canonical base `c3e257e9a`).
- Evidence: January COFF + relocations; Ghidra 12.1.2 decompilation
  `scratch/ghidra/out/encounters.obj.decomp.c`; January map atlas
  `scratch/atlas/encounters.obj.txt`; HCEA `src/blam/ai/encounter_*.c` (semantics and PDB enum
  names only; later build); Stian / punpckhdq have no bodies for the worked functions.
- Worker report with per-function blocks: `scratch/workers/encounters.md`. All candidates and
  helper scripts: `scratch/workers/encounters/`.

## 2. Validated baseline and result

| | exact fns | residual | unwritten | exact padded code | data owners exact |
| --- | ---: | ---: | ---: | ---: | ---: |
| lane HEAD | 48 / 61 | 12 | 1 | 9,328 / 20,688 | 70 / 75 (3,028 / 3,170 B) |
| this lane (real file) | **51 / 61** | 10 | 0 | **11,808 / 20,688** | **75 / 75 (3,170 / 3,170 B)** |

Newly strict EXACT (real-file whole-TU gate, `--forbid-emitted-symbol _point_from_line3d` passed):

| function | padded | meaningful | relocs | normalized sha16 |
| --- | ---: | ---: | ---: | --- |
| `_encounter_update_follow` (was UNWRITTEN) | 1,920 | 1,909 | 52 | `039ff256fc8f30f9` |
| `_encounter_spawn_actor` | 288 | 274 | 16 | `8bf571479c96dd30` |
| `_encounter_post_combat_select_random_behavior` | 272 | 259 | 9 | `63a81fd9169ba095` |

Row-by-row listing diff against the HEAD gate shows only those three rows changing. All five
parks (`_encounter_test_rule`, `_encounter_new`, `_encounter_compute_activation_cluster_bit_vector`,
`_encounter_control_actors`, `_encounter_update_respawn`) keep their `measurements.base`
size/relocations/normalized SHA exactly. `tools/fake_match_scan.py`: 0 leads.

Owner census (final vs `build/base`): no lost owner. New owners are the January-owned
`_encounter_update_follow`, `"stay"`, `"migrate"`, the console format, the invalid-platoon warning
and `__real@c47a0000`; `"<none>"`, `__real@447a0000`, `__real@ff7fffff` are referenced undefined by
January (allowed literal COMDATs). `_square_root` is the systemic real_math.h header-inline extra
COMDAT class (base already carries `_distance_squared3d`, `_magnitude_squared3d`,
`_vector_from_points3d`, `_random_range`; 80 base objects carry `_square_root`) - not a real owner.

## 3. Accepted controls (retained in production)

- **`encounter_update_follow`** (new static body, between `encounter_update_platoons` and
  `encounter_control_actors`, prototype made `static`):
  - platoon validity as `boolean defending = FALSE; if (VALID_INDEX(...)) defending = platoon->defending;
    else if (platoon_index != NONE) error(...)` - January's threaded branch graph (the invalid-platoon
    warning string is January `.rdata`);
  - `BIT_VECTOR_SET_FLAG(migrating_squads, ...)` before `platoon_index = squad_definition->platoon_index`
    (January's `or [ebp+esi-0x48],edx` precedes the platoon load);
  - one target loop `{distance = REAL_MAX; object_get_origin(...)}` (VC7 extracts the `rep stosd`
    and keeps the i*4 IV January uses);
  - `real_point3d follow_position` at function scope (frame `0x208`, identical aggregate slot set);
  - a distinct `candidate_squad_index` for the best/current selection loop (live-range slot
    colouring then matches all four spilled scalars);
  - `!definition ? "<none>" : definition->name` (January loads `"<none>"` first);
  - constants: `NUMBER_OF_FIRING_POSITION_GROUP_INDICES = 26` (HCEA PDB enum name),
    `MAXIMUM_FOLLOW_TARGET_UNITS = 8` (signed `cmp si,8` proves a non-`size_t` bound) as `#define`s in
    the constants section (encounters.h already `#define`s its MAXIMUM counts); the follow-target
    enum (identical to the TU-local copies in `actors.c` and `ai_script.c`) sits immediately before
    the function - see section 6 for why not at the top.
- **`encounter_spawn_actor`**: nested `if (ai_globals->ai_initialized_for_map) { if (encounter_place_actor(...)) {...} }`
  instead of `&&`; VC7 then saves ebx before the first branch exactly as January.
- **`encounter_post_combat_select_random_behavior`**: direct `possibilities[behavior_index][0].field`
  indexing (the pointer local produced `mov/add` instead of January's `lea` and the inverse struct-copy
  load order).

## 4. Experiment matrix

| ID | function | shape | size T/C | relocs T/C | result | decision |
| --- | --- | --- | ---: | ---: | --- | --- |
| F1 | update_follow | natural nested body | 1920/1968 | 52/52 | frame 0x204 | superseded |
| F2 | update_follow | defending flag + else-if warning | 1920/1920 | 52/52 | frame 0x204 | superseded |
| F3 | update_follow | follow_position at function scope | 1920/1920 | 52/52 | frame 0x208, slot set equal | superseded |
| F4 | update_follow | bit vectors (+/-follow_position) at function scope | -/- | - | frame 0x204 / 0x1f8 | reverted |
| F5 | update_follow | SET_FLAG before platoon load; single target loop | 1920/1920 | 52/52 | slot permutation only | superseded |
| F6 | update_follow | distinct selection loop index; ternary polarity | 1920/1920 | 52/52 | **EXACT** | retained |
| F7 | update_follow | enum at top of file | - | - | EXACT but drifts parked update_respawn | reverted (placement) |
| P1 | post_combat | loop init from global_empty_possibility; NONE assignments after csmemset; best_actor -> behavior_actor; found_possibility reuse | 1984/1952 | 76/76 | frame 0xe0 | superseded |
| P2 | post_combat | separate reply/search iterators | 1984/1920 | 76/76 | frame 0xe4 | superseded |
| P3 | post_combat | generic shift loop; search in `if (find_secondary_reply)` after if/else; living chain on `encounter->current_count` | 1984/1984 | 76/76 | **EXACT** | NOT landed: parked update_respawn drift |
| P4 | post_combat | `living_count > 1` spellings, renames, extra named local | 1984/1936-1984 | 76 | EXACT variants all drift the park | reverted |
| S1 | update_status | loop declaration order | 1136/1152 | 38/39 | unchanged | reverted |
| S2 | update_status | single `if (stand_down)` call flag | 1136/1136 | 38/38 | one push site (January has two), then-arm still sunk | rejected |
| S3 | update_status | nested else block for the stand-down chain | 1136/1152 | 38/39 | inert | reverted |
| A1 | test_activation | `ultimate_parent_index` locals, `|` flag combination, iterator.datum_index | 976/976 | 50/50 | sha: force_active load order, link_active home | not landed |
| A2 | test_activation | or-tree regroupings | 976/976 | 50/50 | byte-identical to A1 | reverted |
| A3 | test_activation | force_active as separately assigned local | 976/976 | 50/50 | different spill | reverted |
| C1 | create | `initial_variant = 0` after skip test | 752/736 | 38/38 | first 0x116 bytes equal | superseded |
| C2 | create | placement print inside marine block | 752/752 | 38/38 | register-priority tie (actor_type vs create_leader) | not landed |
| C3 | create | actor_type hoisted / create_leader declared first | 752/752 | 38/38 | inert / still tie | reverted |
| L1 | place_actor | `struct scenario *scenario` local before variant test | 368/384 | 14/14 | ebx register parameter lost | reverted |
| L2 | place_actor | `actor_palette_count` local | 368/384 | 14/14 | same ABI loss | reverted |
| R1 | spawn_actor | nested ifs | 288/288 | 16/16 | **EXACT** | retained |
| B1 | select_random_behavior | direct element indexing | 272/272 | 9/9 | **EXACT** | retained |

## 5. Do-not-repeat

- update_follow: do not move the follow-target enum (or add >=2 enumerators) before
  `encounter_update_respawn`; it flips that park (declaration-count tie).
- post_combat: all natural exact spellings flip the park (section 6); renaming or adding locals does not help.
- update_status: flag-based single stand_down call, declaration order and nested else are measured.
- test_activation: or-tree regrouping is canonicalised; separately assigned force_active locals spill differently.
- create: scope hoisting / declaration order of actor_type/create_leader are inert.
- place_actor: any extra scenario/count local before the variant test costs the ebx register parameter.

## 6. Residual classification

- `_encounter_post_combat` - **source solved, blocked by a parked sibling tie.** Candidate
  `scratch/workers/encounters/post_combat_exact_candidate.txt` is strict EXACT 1984/76 in a whole-TU
  gate (also on top of the final real file: 52 exact). The parked `_encounter_update_respawn`
  flips between two fuzzy forms (sha `32d0a247...` <-> `647cc2e7...`, a commuting
  `mov esi,[ebp-0x10]` / `mov edi,[ebp-4]` reload swap at +0x90). Measured fact: adding dummy
  enumerators before it (diagnostic only) shows a periodic internal counter tie - the base state holds
  only at +0/+1/+64, and the exact post_combat moves that window to +40..+54. Inference: no
  natural post_combat spelling can hold the base state.
- `_encounter_update_status` - layout: January keeps the recent-enemy then-arm inline so the two
  stand_down call sites tail-merge; ours sinks it (+16 B, +1 reloc). Plus ebx/edi role swap.
- `_encounters_test_activation` - scheduling/allocation after structural fix (A1 candidate kept in
  `test_activation_best_candidate.txt`).
- `_encounter_create` - register-priority tie after structural fix (C2 in `create_best_candidate.c`).
- `_encounter_place_actor` - ABI/register coupling with callers.

## 7. Reopen criteria / ORCHESTRATOR-PROPOSALs

1. Re-measure `config/parked.json` `_encounter_update_respawn` `measurements.base` to
   `{704, 27, 647cc2e7c6b1161aac51d47e0d4b97bdc81e429042cca996570a7d1f86e8917e}` and apply the
   post_combat candidate body verbatim (+1,984 padded exact; private static, no external consumers).
2. Move the follow-target enum into `source/ai/encounters.h` (removing the identical TU-local copies in
   `actors.c` and `ai_script.c`) and `NUMBER_OF_FIRING_POSITION_GROUP_INDICES` into
   `ai_scenario_definitions.h`; requires the same update_respawn re-measure and a gate of every
   `encounters.h` consumer.
3. Parked `_encounter_compute_activation_cluster_bit_vector`: the `ultimate_parent_index` local that
   reproduces January's call/push order in test_activation moves it 1232 -> 1216 (target 1200) in a
   diagnostic build - a reopen hint for its park owner (body not edited here).
4. update_status / create / test_activation / place_actor: reopen only with a source form that changes
   the named layout/allocation decision (see section 6), not by further permutation.

## 8. Disposition

Three functions strict exact and eligible (update_follow, spawn_actor,
post_combat_select_random_behavior). post_combat exact candidate awaits the park re-measure. The
other residuals remain active (unparked), with zero credit.
