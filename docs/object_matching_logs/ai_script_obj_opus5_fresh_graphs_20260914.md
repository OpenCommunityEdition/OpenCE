# `ai_script.obj` Opus5 fresh-graphs lane, wave 2 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/ai_script.c`, target `build/split/source/ai/ai_script.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254.1 / XDK 3911, project flags
  `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`, no per-file override).
- Lane `opus/30k-fresh-graphs-20260914` (wave 2, residual-only). Measured against the lane tree
  containing `ef7bc742a` (ai_scenario_definitions.h owners) and `423067414`; later lane commits
  `6a65c04c1`/`c078622f2` touch only encounters.c/action_obey.c (no shared header).
- Evidence: January COFF + relocations + `.rdata` assert strings; Ghidra 12.1.2 decompilation
  `scratch/ghidra/out/ai_script.obj.decomp.c`; January map atlas `scratch/atlas/ai_script.obj.txt`;
  HCEA `src/blam/ai/ai_scripting_*.c` (later build: semantics only).
- Prior ledgers honoured in full: `ai_script_obj_opus_coherent_reconciliation_20260909.md`
  (owner/ABI boundaries, rejected donor paths) and the other eleven `ai_script_obj_*` ledgers.
- Per-function worker blocks: `scratch/workers/ai_script.md`. Candidates, lab files and helper
  scripts: `scratch/workers/ai_script/` (lab harness `lab/tulab.py` compiles function variants with
  the ai_script.c header context and unit flags and aligns them against the January section).

## 2. Validated baseline and result

| | exact fns | residual | unwritten | exact padded code | data owners exact |
| --- | ---: | ---: | ---: | ---: | ---: |
| lane HEAD | 101 / 116 | 15 | 0 | 14,304 / 20,112 | 98 / 103 (3,341 / 3,756 B) |
| this lane (real file) | **109 / 116** | 7 | 0 | **18,512 / 20,112** | **102 / 103 (3,755 / 3,756 B)** |

Newly strict EXACT (final real-file whole-TU gate, `--forbid-emitted-symbol _point_from_line3d` passed):

| function | padded | meaningful | relocs | normalized sha16 |
| --- | ---: | ---: | ---: | --- |
| `_ai_scripting_migrate_internal` | 1,360 | 1,352 | 67 | `b775cd218af477e5` |
| `_ai_scripting_migrate_find_target_squad` | 1,072 | 1,067 | 48 | `fb21929e521bfee9` |
| `_ai_scripting_migrate` | 160 | 145 | 10 | `3c116181c54e6517` |
| `_ai_scripting_migrate_by_unit_internal` | 240 | 232 | 9 | `e46a4572f6fab0f0` |
| `_ai_scripting_command_list_status` | 592 | 590 | 24 | `859cf195324941c4` |
| `_ai_scripting_command_list_status_internal` | 80 | 78 | 3 | `42d4335fb62e9924` |
| `_ai_index_actor_iterator_new` | 208 | 204 | 9 | `c21d192405b121c2` |
| `_ai_scripting_vehicle_encounter` | 496 | 491 | 19 | `6a91443a942be677` |
| **total** | **4,208** | **4,159** | | |

Row-by-row listing diff against the HEAD gate shows only these eight rows changing
(residual -> EXACT). Both parks (`_ai_scripting_teleport_starting_location_private`,
`_ai_scripting_allegiance_broken`) keep `measurements.base` size/relocations/normalized SHA.
Owner census (final vs `build/base`): new owners are the four January-owned assert literals
(`(source_iterator.squad_index >= 0) && ...`, `(current_squad_index >= 0) && ...`,
`(target_squad_indices[current_squad_index] >= 0) && ...`, `(found_squad_index >= 0) && ...`);
lost owners are three non-January `VALID_INDEX(...)` literals. The only missing data owner is the
1-byte `_ai_script_squad_separator`. `tools/fake_match_scan.py`: 0 leads.

Residuals at close (first divergence):

| function | T/B size | T/B relocs | base sha16 | first divergence / class |
| --- | --- | --- | --- | --- |
| `_ai_index_from_string` | 304 / 320 | 13 / 13 | `78a2a67e..` (T) | +0x6 January pins NONE in ESI; ours stores immediates - zero-register pinning |
| `_ai_scripting_renew` | 368 / 368 | 22 / 22 | sha differs | +0x116 unit_index load order around the grenade calls - scheduling |
| `_ai_scripting_set_current_state` | 160 / 160 | 10 / 10 | sha differs | +0x11 `movsx esi,si`: C4013 implicit declaration - header ownership |
| `_ai_scripting_migrate_and_speak` | 224 / 224 | 16 / 16 | sha differs | January omits the `speech_type` vararg - owner ruling |
| `_ai_scripting_follow_target_ai` | 176 / 176 | 11 / 11 | sha differs | January formats ai_reference twice - owner ruling (previously rejected) |
| 2 parks | locked | | | unchanged |

## 3. Accepted controls (retained source shapes and their evidence)

- **Migration graph.** Nested NONE guards with a second guard on the masked indices
  (`cmp edi,-1` after `and edi,0xFFFF`; HCEA keeps the second test); scenario local plus a fresh
  `&global_scenario_get()->ai_encounters` per encounter definition (3 and 2+1 `_global_scenario_get`
  relocations - the file's existing exact convention); January assert expressions copied from
  `.rdata`; the three iterators in lexically disjoint blocks (January shares
  [ebp-0x3c..-0x20] between the squad, encounter-actor and actor iterators; the same body with
  function-scope iterators is not exact); `target_squad_indices[current_squad_index]` re-read at
  each use; `DATUM_INDEX_TO_ABSOLUTE_INDEX` on both sides of the encounter comparisons (masked value
  CSE'd into [ebp-0x18]). The prevehicle flag test on the masked index is January's own behaviour
  (`test edi,0xC0000000` on the restored masked index) and carries a `BUG (original)` comment.
- **find_target_squad.** Matching-index declarations in January's initialiser store order
  (first/type/actor/variant/squad), variant pointer declared before actor pointer, target squad
  definitions fetched inside `if (ai_debug.print_migration)` and bound to locals, same-variant
  branch nests the source-definition fetch between the two NONE tests,
  `short found_squad_index = NONE;` (January returns a literal `or eax,-1` on the empty path).
- **migrate / migrate_by_unit_internal.** Private ABI (source reference in EAX) followed from the
  callee; `actor_index` from one conditional-expression initialiser (January homes it on both
  arms); definition lookups hoisted in source order; doubly masked same-encounter comparison
  (`xor/and/neg/sbb/inc`).
- **command_list_status family.** The helper's select is an if/else statement (lab: every ternary
  or arithmetic spelling yields `shr/not/and/or`); the command element is a NULL-initialised
  pointer local tested `!command` first (lab ord2 f4 is the only layout placing the `1` arm between
  the NULL test and the select). The caller uses a conditional timer store and
  `result = MAX(result, status)`.
- **ai_index_actor_iterator_new.** The NONE-guarded `encounter_actor_iterator_new` sits inside the
  valid-scenario block (lab sw.c f2/f4 reproduce January's case-block placement; semantics
  unchanged because the else arm always leaves NONE).
- **vehicle_encounter.** The platoon search uses `squad_index` itself as the loop index with an
  `if (squad_index >= count) squad_index = 0;` reset and a bound squad-definition pointer
  (January keeps ESI = squad_index through the loop and compares through EAX).

## 4. Experiment matrix (summary; full rows in scratch/workers/ai_script.md)

| ID | function | shape | size T/B | relocs T/B | result |
| --- | --- | --- | --- | --- | --- |
| M1 | migrate_internal | nested guards + fresh scenario gets + January asserts + disjoint iterator blocks | 1360/1360 | 67/67 | EXACT, retained |
| M1' | migrate_internal | M1 with function-scope iterators | 1360/1360 | 67/67 | sha, control |
| F1 | find_target_squad | fresh scenario gets + found_squad_index assert | 1072/1072 | 48/48 | sha, superseded |
| F2 | find_target_squad | + decl order, definitions inside debug, nested same-variant | 1072/1072 | 48/48 | 1-insn register + return literal |
| F3 | find_target_squad | + variant-first decl, `found_squad_index = NONE` init, no else | 1072/1072 | 48/48 | EXACT, retained |
| B1-B2 | migrate_by_unit_internal | if-NONE reassignment (&& / nested) | 240/240 | 9/9 | sha |
| B3 | migrate_by_unit_internal | conditional-expression initialiser | 240/240 | 9/9 | EXACT, retained |
| B4 | migrate_by_unit_internal | B3 with single-masked compare | 240/240 | 9/9 | sha, control |
| S1 | migrate_and_speak | drop speech_type argument (diagnostic) | 224/224 | 16/16 | EXACT, NOT retained (UB) |
| C1 | command_list_status | conditional timer store + MAX | 592/592 | 24/24 | sha (size closed) |
| I1..Y1 | command_list_status_internal | ternary/long/inverted/default-1/bool-valid spellings | 80/80..96 | 3/3 | sha |
| X3 | command_list_status_internal | NULL-init element pointer, `!command` first, if/else select | 80/80 | 3/3 | EXACT (+ caller), retained |
| F1-F3, lab | ai_index_from_string | none-path NONE store, single exit, decl/assert order, separator-first | 304/320 | 13/13 | no NONE pin |
| A1-A3 | ai_index_actor_iterator_new | case label orders | 208/208 | 9/9 | inert |
| AI1 | ai_index_actor_iterator_new | call inside the valid block | 208/208 | 9/9 | EXACT, retained |
| R1, lab | renew | HCEA local, compare/local type variants | 368/368 | 22/22 | inert/worse |
| P1 | set_current_state | owner prototype visible (diagnostic) | 160/160 | 10/10 | EXACT, needs header |
| VE lab | vehicle_encounter | 24 case orders (diagnostic) | 496/496 | 19/19 | inert |
| VE3 | vehicle_encounter | squad_index loop + if reset + bound definition | 496/496 | 19/19 | EXACT, retained |

## 5. Do-not-repeat list

- Case-label order in the `(unsigned long)ai_reference >> 30` switches (actor iterator,
  vehicle_encounter): inert for VC7's compare chains.
- `TEST_FLAG(...) ? 2 : 3`, `!TEST_FLAG ? 3 : 2`, hand-mask ternaries, `long` return type, inverted
  `||` conditions, `status = 1` defaults, `boolean valid_command`: none changes the helper select or
  block order.
- ai_index_from_string: NONE stores, single-exit rewrites, separator branch order, declaration
  order - the ESI pin never appears (zero-register pinning class).
- renew: grenade local/type/compare variants - inert or worse.
- Consumer-local prototype for actor_action_set_default_state (proves the header fix but is
  inadmissible in ai_script.c).
- Donor January-diagnostic defects (follow_target_ai double ai_reference, migrate_and_speak missing
  vararg): measured exact, withheld pending owner ruling.

## 6. Residual classification

- `_ai_index_from_string`: ABI/register - January pins NONE in ESI (measured); no source lever
  found (inference: zero-register pinning class).
- `_ai_scripting_renew`: scheduling - identical instructions except argument load order
  (measured).
- `_ai_scripting_set_current_state`: ownership - C4013 implicit declaration promotes the short
  argument (measured: `CL /Zs /W3` reports exactly this C4013; prototype makes it exact).
- `_ai_scripting_migrate_and_speak`, `_ai_scripting_follow_target_ai`: source/topology with
  owner hold - January's own diagnostic-argument defects (measured in bytes; admissibility is an
  owner decision; the missing vararg is undefined behaviour).

## 7. Reopen criteria

- set_current_state: `boolean actor_action_set_default_state(long actor_index, short override_state);`
  declared in `source/ai/actions.h` (and the actions.c-local prototype removed) - re-gate, no body
  change expected.
- migrate_and_speak / follow_target_ai: owner ruling admitting January diagnostic-argument defects
  under the README `BUG (original)` policy.
- from_string: a demonstrated source lever for constant-register pinning in any January TU.
- renew: a scheduling lever for per-argument reloads of a repeated field.

## 8. Disposition

Eight functions strict EXACT in the real-file gate and eligible for `Matching` credit
(4,208 padded / 4,159 meaningful bytes). Five residuals remain active with the classifications
above; the two parks are unchanged. No configure, Ninja, commit, header or config edit was made.
