# Prompt for Claude Fable 5: highest-leverage `units.obj` work

You are taking over the Halo CE January-beta byte-matching campaign in this
existing worktree:

`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\units-integration-20260824`

Do not create a new worktree, reset, rewrite, push, or disturb concurrent
changes. Treat the current filesystem, current branch, target object, and
generated reports as authoritative. Inspect `git status`, HEAD, and the
current object measurements before relying on any historic count.

## Mission

Perform the single highest-leverage research or implementation wave that can
move `source/units/units.obj` toward complete strict byte identity. The
preferred research target is the eight-byte scheduler residual in
`_unit_throw_grenade_release`: decode the VC7 `dag.c` ready-list priority at
`0x107304d1` using a real full-translation-unit `units.c` compile, then turn
that finding into a readable ordinary-C source lever if one exists.

Why this target: the preserved carrier-body probe is 784/784 bytes with all 26
relocations and register roles equal; only two adjacent FP-vs-integer
scheduling transpositions remain, eight bytes total and one bijection event.
It is a cleaner compiler-decision oracle than `_unit_place`, `_unit_update`,
or a broad reconstruction residual. If the compiler decode proves there is no
source-reachable lever, produce reusable, evidence-backed tooling/research and
then advance the next strongest ordinary-C lead:

1. `_unit_animation_set_state`: recover the missing short old-animation-state
   local and its block scope;
2. `_unit_set_or_test_seat_and_weapon_label`: restore the missing
   `animation.seat_index` store and determine why `NONE` is hoisted early;
3. `_code_001a0cf0` / `_unit_update_animation`: preserve the corrected
   `_object_dead_bit` field selection and attack only the remaining register
   role/lifetime residual;
4. `_unit_died`: cold-block sinking only—do not invert the semantic arms;
5. `_unit_update`: last, because it remains broad and low-yield.

Do not spend the wave on already-exhausted spelling changes, the forbidden
exporter barrier, or speculative bulk edits.

## Authoritative production files and objects

- `source/units/units.c`
- `source/units/units.h`
- `source/units/unit_definitions.h`
- `source/units/vehicles.h`
- `source/objects/objects.h`
- `source/game/game_engine.h`
- `source/math/real_math.h`
- `config/config.json`
- `config/symbols.json`
- `config/relocs.json`
- `objdiff.json`
- `build/split/source/units/units.obj` — January target
- `build/base/source/units/units.obj` — current XDK candidate
- `build/report.json`
- `build/semantic_report.json`
- `build/local-tools/ninja.exe`

The target executable is `cachebeta.exe`. The compiler is XDK 3911 VC7
`CL.Exe` 13.00.9254.1 under the repository-generated edge. Padded section
bytes plus complete relocation address/type/destination/addend identity are
the acceptance authority; fuzzy objdiff scores and whole-object hashes are
diagnostic only.

## Read these current Units records first

Read every file below before choosing or compiling a candidate; later records
refine earlier ones:

- `docs/object_matching_logs/units_obj_jonas_cumulative_frontier_20260825.md`
- `docs/object_matching_logs/units_obj_jonas_shared_interface_package_20260826.md`
- `docs/object_matching_logs/units_obj_jonas_enter_seat_collapse_20260826.md`
- `docs/object_matching_logs/units_obj_jonas_place_probe_notes_20260826.md`
- `docs/object_matching_logs/units_obj_jonas_carrier_body_frontier_20260827.md`
- `docs/object_matching_logs/units_obj_jonas_inventory_public_seven_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_postprocess_cluster_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_private_foundation_wave_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_control_leaf_four_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_seat_query_wave_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_typed_damage_record_wave_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_inventory_weapon_closure_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_damage_lifecycle_closure_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_animation_callers_four_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_vehicle_scripting_wave_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_typed_projectile_ray_wave_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_vehicle_exit_wrappers_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_typed_declaration_pair_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_weapon_state_private_helper_0fab_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_update_animation_private_closure_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_typed_debug_queries_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_animation_update_declaration_20260824.md`
- `docs/object_matching_logs/units_obj_jonas_nearby_seat_named_field_20260825.md`
- `docs/object_matching_logs/units_obj_jonas_format_return_compliance_20260825.md`
- `docs/object_matching_logs/README.md`
- `docs/matching_methodology.md`

The current frontier record reports 175 strict-exact, 10 emitted nonexact, and
4 absent code owners out of 189, plus 128/0/1 runtime-data owners. Re-measure
this at current HEAD rather than assuming it stayed unchanged.

## Claude/Fable evidence tree—read, do not transplant blindly

The consolidated older Claude tree is branch
`claude/near-complete-objects-20260816`; the authenticated later carrier is
commit `deb5554ab8d16da80da934e2aa7081bc7d1f594f`. Read branch-owned files with
`git show <ref>:<path>` or in a disposable external scratch checkout. Do not
checkout them over current production files.

Required Claude documentation:

- `claude/near-complete-objects-20260816:docs/object_matching_logs/units_obj.md`
- `claude/near-complete-objects-20260816:research/register_tie_cumulative_findings.md`
- `claude/near-complete-objects-20260816:research/register_tie_reachability.md`
- `claude/near-complete-objects-20260816:research/register_rename_census.md`
- `claude/near-complete-objects-20260816:research/PROMPT_finish_hs.md`
- `deb5554ab8d16da80da934e2aa7081bc7d1f594f:research/PROMPT_next_session.md`
- `deb5554ab8d16da80da934e2aa7081bc7d1f594f:research/residual_frontier.json`
- `deb5554ab8d16da80da934e2aa7081bc7d1f594f:research/unwritten_frontier_recipe.md`

Required compiler/debugger tooling records at the carrier commit:

- `tools/c2dbg32/README.md`
- `tools/c2dbg32/IR_LAYOUT.md`
- `tools/c2dbg32/BACKEND_MODULE_MAP.md`
- `tools/c2dbg32/dbg32.c`
- `tools/c2dbg32/c2dis.py`
- `tools/c2dbg32/c2func.py`
- `tools/c2dbg32/acc_sampler.py`
- `tools/c2dbg32/diffsample.py`
- `tools/c2dbg32/gen_config_probe.py`
- `tools/c2dbg32/gen_config_repro198.py`
- `tools/c2dbg32/repro_00198fd0.c`
- `tools/campaign/README.md`
- `tools/campaign/gate.py`
- `tools/campaign/lab_compile.py`
- `tools/campaign/units_hunt_all.py`
- `tools/campaign/mk_repro.py`
- `tools/campaign/verdicts.py`

Two cautions are binding: the old `README.md` six-hit breakpoint statement is
stale relative to the implementation's `maxhits = 250`, and the early
`IR_LAYOUT.md` interpretation of node `+0x28` as a physical register is
superseded—the field is an operand width/size class. Re-validate the dirty
`dbg32.c` change before trusting or importing it. Prefer fixing/running tools
in an external scratch directory unless a production-tool change is proven,
tested, and independently useful.

## User-supplied external source oracles

Search these only for semantic/source topology and independent corroboration;
never copy raw addresses, assembly lifts, or unsafe carrier constructs:

- `https://github.com/stianeklund/halo`
- `https://github.com/pastudan/halo`
- `https://github.com/punpckhdq/halo`
- `https://github.com/surreptitiousresearch/halocea`
- `https://surreptitiousresearch.github.io/halocea_docs/`
- `https://github.com/Aerocatia/halopc-restored/tree/master/archive`
- `https://github.com/Aerocatia/demon`
- `https://github.com/chadski/nimbus`
- `https://github.com/Zoephie/Baboon`
- `https://github.com/sp00nznet/xboxrecomp/pull/5`

The two Stian Eklund golden runners are useful for understanding reproducible
target/candidate validation, not as substitutes for this repository's strict
COFF gate:

- `tools/verify/run_golden_tests.py`
- `tools/verify/run_golden_xbox.py`

## House rules and prohibited techniques

Use readable C89 and preserve behavior. A no-argument parameter list places
`void` on its own line; every parameter gets its own line; every function,
including `void`, ends in an explicit return. Typed tag access must use the
subsystem macro wrapping `tag_get`; typed object access must use the established
object-access macro, except for the two already documented target-proven Units
exceptions. Use named fields, enums, and real subsystem interfaces.

Forbidden: inline assembly, `register`, `volatile`, barriers including
`_ReadWriteBarrier`, pragmas, intrinsics, attributes/`__declspec`, raw
addresses or datum offsets, pointer-integer reconstruction, pointer/union
punning, undefined behavior, synthetic anchors, byte forcing, object patches,
comparator/admission exceptions, or compiler/tool substitution. Do not use the
nominal 175-owner Claude carrier wholesale; it contains several forbidden
constructs and is evidence only.

Protected production sources—do not edit:

- `source/units/vehicles.c`
- `source/ai/ai_debug.c`
- `source/ai/ai_script.c`
- `source/ai/actions.c`
- `source/game/game_engine.c`

Preserve every currently exact Units owner and the integration-only
`_unit_add_weapon_to_inventory` sentinel.

## Experimental discipline

1. Snapshot current source/object/report evidence and perform a direct strict
   Units census before editing.
2. Freeze one bounded hypothesis and its dependency closure from target
   disassembly, documented source provenance, or compiler-internal evidence.
3. Run a parse-only `/Zs` check first.
4. Permit one normal code-producing compile for that frozen evidence wave.
   Retry only after genuinely new evidence; do not spelling-tune.
5. Measure padded bytes, normalized bytes, and ordered relocations for every
   changed owner and all exact siblings. Retain only strict gains, or a clearly
   documented target-proven structural reconstruction that improves the real
   frontier without regressing any accepted owner.
6. Revert failed production edits exactly. Preserve useful scratch evidence
   outside tracked production paths.

## Required finish package

Return a concise evidence report containing:

- the exact current census and residual list;
- every source/tool/document path read;
- target and candidate padded sizes, relocation inventories, normalized hashes,
  first divergence, and changed-instruction/bijection counts;
- the decoded VC7 decision and why it is or is not source-reachable;
- the one frozen candidate and its result;
- a do-not-repeat matrix for rejected forms;
- strict sibling/data/ownership regression results;
- retained source diff, or proof of exact reversion;
- the strongest next lever if the object is not complete.

If production code is retained, run full `halobetacache_build` and
`libcmt_build`, regenerate objdiff and semantic/progress reports, run object
admission and parked audits, all tooling tests, `git diff --check`, and a clean
forced-object replay. Commit verified work locally with a focused message; do
not push. Do not label `units.obj` `Matching` until every one of its 189 code
owners and 129 runtime-data owners passes strict padded-byte, relocation, and
ownership equality.
