# `encounters.obj` Opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/encounters.c`, target `build/split/source/ai/encounters.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254 / XDK 3911).
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4` (the completed Opus 100K donor lane).
- Evidence used:
  - January COFF bytes and relocations;
  - HCEA `src/blam/ai/encounter_*.c` / `encounters_*.c` (later PPC build; names, semantics and statement order only);
  - Fable 50K resume-lane worker report and bodies (`C:\halo-worktrees\fable-50k-resume-20260906\scratch\reports\encounters.md`, `scratch/w_encounters_*`);
  - the fresh-graphs lane candidates (`C:\halo-worktrees\opus5-30k-fresh-graphs-20260914\scratch\workers\encounters\`).
- Worker report with per-function blocks: `scratch/workers/encounters.md`. Candidates, gate listings and helper scripts: `scratch/workers/encounters/`.

## 2. Baseline and result

| | exact | residual | unwritten | parks drift |
| --- | ---: | ---: | ---: | --- |
| baseline (real file, guard) | 52 | 9 | 0 | 0 / 5 |
| final (real file, guard) | **53** | 8 | 0 | 0 / 5 |
| with the two proposed park reopens (scratch) | 55 | 6 | 0 | only the two reopened rows |

### Newly strict EXACT in the real file

| function | padded | relocs | normalized sha16 |
| --- | ---: | ---: | --- |
| `_encounter_place_actor` | 368 | 14 | `d005c7cf24715e91` |

Row-by-row listing diff against baseline: only this row changes. Other checks:

- `_point_from_line3d` guard passed;
- all five parks equal `measurements.base`;
- owner census against `build/base`: no new or lost owner;
- `tools/fake_match_scan.py`: 0 leads (HEAD 0).

### Park reopens proposed (scratch only, for the orchestrator)

| park | padded | relocs | candidate |
| --- | ---: | ---: | --- |
| `_encounter_control_actors` | 448 | 16 | `scratch/workers/encounters/reopen_control_actors.c` |
| `_encounter_new` | 656 | 31 | `scratch/workers/encounters/reopen_encounter_new.c` |
| both | 1,104 | — | `scratch/workers/encounters/reopen_combined.c` (55 exact, no other row change) |

## 3. Accepted control: `encounter_place_actor` (landed)

Two source changes produce the exact body:

- **Single-exit law.** January's three failure paths share one `xor al,al` epilogue, and its success path is `setne al` of a byte-typed result. The old `return actor_place(...) != NONE;` emits `xor edx,edx; setne dl; mov al,dl`. The new body declares `boolean placed = FALSE;` as the first local, assigns `placed = actor_place(...) != NONE;` and returns `placed` once. Sibling result-first locals in this TU follow the same pattern (`boolean marked = FALSE;` and `boolean added = FALSE;`).
- **Store/call order.** January stores the squad's `actor_palette_index` and then calls `global_scenario_get()` before loading `starting_location->actor_variant_index`. It keeps that scenario pointer for `ai_actor_palette.count` and calls `global_scenario_get()` again for the element fetch. The new body adds `struct scenario *scenario = global_scenario_get();` after the palette index and uses the cseries `VALID_INDEX` macro.

The fresh-graphs lane measured the scenario local on its own (L1) at 384 bytes, because it cost the private `ebx` encounter-index parameter. Combined with the single-exit result, the body is exact and the callers (`encounter_create`, `encounter_spawn_actor`) do not change.

## 4. Park reopen evidence

### `_encounter_control_actors`

The park was classified as register-allocation. Two pieces of evidence refute that:

- **Frame size.** January uses `sub esp,0x14`; the park body uses 0x18 because of an extra scoped `short platoon_index` home.
- **Statement order.** January reads `platoon->defending` (`mov dl,[eax]`) immediately after `encounter_get_platoon`, before the maneuvering test. It also re-reads `actor->meta.platoon_index` for the definition fetch. HCEA has the same `defending`-then-`maneuvering` order.

The Fable 50K resume lane's "first natural body, no tuning" was never landed because a prototype was missing at the time. It is strict exact on the current file. That satisfies the park's criterion of a "natural same-compiler allocation donor".

### `_encounter_new`

January emits `mov di,[edi+0x88]; test di,di; mov eax,0x3e7; je; movsx eax,di; mov [ebx+0xc],ax`. This is VC7's rvalue `?:` materialization: the constant arm is preloaded, the tested field is CSE'd into a register, and there is a single store.

The park kept `short respawn_actors_left = total; if (== 0) ... = 999; store`, calling that direct assignment the natural spelling. The target's own idiom refutes that premise.

- **Landed spelling.** `squad->respawn_actors_left = squad_definition->respawn_total_count == 0 ? SQUAD_UNLIMITED_RESPAWN_ACTOR_COUNT : squad_definition->respawn_total_count;` is exact.
- **Inverted polarity.** `total ? total : 999` is not exact. January's `je`-to-constant layout proves the polarity.
- **HCEA.** HCEA's PPC decompile spells it as local/if. Decompiler output is not evidence of source spelling here.

## 5. Experiment matrix (not landed)

| ID | function | shape | size T/C | relocs T/C | result |
| --- | --- | --- | ---: | ---: | --- |
| U1 | update_status | `boolean stay_active = <timer predicate>; if (stay_active) ... else if` (HCEA form) | 1136/1136 | 38/38 | sha: actor/body_count ebx-edi role swap + zero register at loop tail |
| U2 | update_status | U1 + function-scope hoists of squad/body_count/strength/fighting (11 subsets) | 1136/1136 | 38/38 | inert |
| T1 | test_activation | A1 (fresh-graphs) re-measured | 976/976 | 50/50 | sha, 65 blocks |
| T2 | test_activation | force_active local before `game_in_editor()` (function scope / loop scope) | 976/992 | — | load before call but spilled; 57 blocks |
| T3 | test_activation | `boolean active` result locals; active reused as link flag | 976/960 | — | 64 blocks |
| T4 | test_activation | `|=` accumulators | 976/976 | 50/50 | 56 blocks; not credible source |
| C1 | create | fresh-graphs C1+C2 re-measured | 752/752 | 38/38 | sha, 54 blocks |
| C2 | create | create_leader / encounter / actor_type scope hoists | 752/752 | 38/38 | byte-identical (inert) |
| P1..P12 | place_actor | see worker report | 368 | 14 | P7/P8/P12 EXACT; P12 landed |
| V1 | activation_cluster (park) | ultimate_parent_index locals | 1200/1216 | 46/46 | 37 blocks |
| V5 | activation_cluster (park) | V1 + `disable ? FALSE : desire` + single count test per pass | 1200/1200 | 46/46 | 3 blocks: select is 32-bit, January 8-bit |
| V6 | activation_cluster (park) | V1 + `if (disable) desire = FALSE; store` + loops | 1200/1216 | — | select bytes exact; post-loop allocation flips |
| V8–V17 | activation_cluster (park) | scope hoists, if/else, inverted ternary, `&&` | — | — | not exact |
| R2 | update_respawn (park) | `while (left > 0 && current < min)` loop | 704/704 | 27/27 | 57 -> 50 blocks, sha |
| R-diag | update_respawn (park) | 0..71 dummy enumerators on R2 (diagnostic only) | — | — | two fuzzy states, never exact |
| Q2 | test_rule (park) | else-branch order + separate `case never: result = FALSE; break;` + `default: result = FALSE; break;` | 656/656 | 46/46 | **EXACT but policy-blocked** (identical fake arms / redundant store) |
| Q3 | test_rule (park) | else order + `case never` without default | 656/656 | 46/46 | instructions identical; `ja` lands on join instead of the FALSE block |

## 6. Residual classification and reopen criteria

- `_encounter_update_status`
  - **Classification:** structural part solved by U1 (size and relocations exact); the remainder is zero-register pinning / callee-saved role swap.
  - **Reopen:** land U1 only together with a source form that gives `body_count`/`prop` the ebx web.
- `_encounters_test_activation`
  - **Classification:** byte-register anchor. January holds `force_active` in `bl` across `game_in_editor()`, which evicts the actor pointer to edi.
  - **Reopen:** a credible form that keeps that byte load in `bl` across the call.
- `_encounter_create`
  - **Classification:** byte-register anchor. January's `create_leader` is in `bl` and `encounter_definition` is spilled around the normal case; the missed jump-thread re-test follows from that allocation.
  - **Reopen:** a use-count or source difference raising `create_leader`'s priority for `bl`.
- `_encounter_compute_activation_cluster_bit_vector` (park)
  - **Classification:** two source defects identified (nested ultimate-parent call; redundant `count > 0` guards). V5 reaches 1200/46 with only the select width differing.
  - **Reopen:** a credible 8-bit select spelling that keeps V5's post-loop allocation.
- `_encounter_update_respawn` (park)
  - **Classification:** R2 loop-structure fix; the remainder is `encounter_definition` versus the `(long)squad_index` temp for ebx.
  - **Reopen:** a form giving `encounter_definition` ebx, applied with R2.
- `_encounter_test_rule` (park)
  - **Classification:** exact only with an explicit never case plus an identical defensive default (Q2). This contradicts the house rules and the park's own "do not force the redundant table entry".
  - **Reopen:** an owner ruling on that style, or authentic source.

## 7. Orchestrator proposals

- **PARK-REOPEN `_encounter_control_actors`.** Candidate `scratch/workers/encounters/reopen_control_actors.c`, or `reopen_combined.c` together with `_encounter_new`. No header or config change is needed; the parked.json entry must be removed in a dedicated commit.
- **PARK-REOPEN `_encounter_new`.** Candidate `scratch/workers/encounters/reopen_encounter_new.c`. Same conditions as above.
- **No header or config edits requested.**

## 8. Do-not-repeat

- update_status: U2 scope hoists, together with the fresh-graphs S1–S3.
- test_activation: T2–T4 force_active local / result local / `|=` shapes.
- create: create_leader / encounter / actor_type scope hoists.
- activation_cluster: V6–V17 select spellings on top of the loop fixes.
- update_respawn: dummy-enumerator windows do not contain the exact state for R2.
- test_rule: do not land Q2 without an owner ruling.
