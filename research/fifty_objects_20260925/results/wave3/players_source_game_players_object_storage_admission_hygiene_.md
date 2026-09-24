# source/game/players (object storage / admission hygiene)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/config/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/ps3.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/st1.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/shadow/sweep_results.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/pdb_storage_players.txt"
]

## production_changes
Land two things together:

A) C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/production.patch (git apply --check is clean). In players.c it:
- makes `static long machine_to_player_table[...] = { 0 };` and `static struct players_static_data players_static_data =` static;
- deletes the consumer-local `extern short player_spawn_count;`, a verbatim duplicate of main/main.h:192, which players.c already includes.

B) config/symbols.json: edited full copy at C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/config/symbols.json. It changes three lines in place:
- line 2598: `{ "file_offset": 696704, "flags": 32, "name": "_player_examine_nearby_unit", "static": true },`
- line 22521: `{ "file_offset": 3010056, "flags": 0, "name": "_players_static_data", "static": true },`
- line 23014: `{ "file_offset": 4535304, "flags": 0, "name": "_machine_to_player_table", "static": true },`

A and B must land together. The source statics without the csplit regeneration add 2 object_audit symbol differences.

## evidence
- cachebeta publics (pdb_storage) list 5 January-static symbols that are external in the split.
- The production object_audit already FAILS on `_player_examine_nearby_unit` storage (split 2, ours 3; source static, PDB absent).
- xref_check: 0 UNDEF references from any other January split object or any of our objects.
- ps3.obj against the production object: every section identical except .debug$S. Data symbols keep their section and offset, gate is 68/2, and there is no $L label churn.
- Data is 100% in build/report.json.
- The 13 surplus COMDATs are identical to January's selected copies, and the provider link PASSES.
- The cseries/errors.h include (C4013) was rechecked and dropped. Code is byte-inert, but internal $L labels renumber.

## blockers
Two January-static functions, _player_teleport_internal and _player_handle_powerup_equipment, stay external. Making them static requires moving their prototypes out of players.h (73 includers).

A shadow sweep (scratch/w/players/shadow/sweep_results.txt) used a caveated configuration: cseries.h plus the shadow header force-included with /FI, which perturbs the base, and 5 TUs failed to build that way. It compared the unmodified header with the moved one. The move flips these declaration-count canaries:
- _rasterizer_frame_statistics_draw
- _race_engine_player_update
- _race_touch_flag
- _ai_test_ballistic_line_of_fire
- _player_profile_3wide_list_update

The alternative, keeping the extern header prototype over a static definition, is byte-inert in players.c (st1.c), but it is an owner-gated form.

The object also still needs both residual functions above.

## reopen_criteria
Choose one: the owner rules on an extern prototype over a static definition, or a count-neutral recovery of real declarations lets the players.h prototype move land with a clean full-ninja sweep.

## task notes
1. The laws_w3 C3 refutation of a wrapper macro for fast_vector_intersects_sphere is logically invalid: the integer-copy sites it cites are exactly the sites where a parenthesis was measured inert. Owner packet: owner_gated_wrapper_macro.patch.
2. Teleport: the /Od-attested point_from_line3d helper is January's source. Replacing the hand expansion flips both magnitude x87 orders to January's. Any future teleport landing must use the helper and will emit a byte-identical _point_from_line3d COMDAT; the provider link already passes.
3. The allocator oracle (C2.Dll forcing) proves o1 matches January except for three allocator inputs. This is diagnostic only and not landable.
4. The shadow-header sweep ran with /FI force-includes (cseries.h plus the shadow players.h). That perturbs the base, and 5 TUs failed to compile that way. Before any players.h prototype move, the integrator should rerun it as a real full ninja.
5. o3 finding: HCEX's static-local adjustment_weights is a later revision. January's symbol `_adjustment_weights` proves file scope, so the current source is right.
6. Background workers (greedy, sweep) have finished. Their outputs are under scratch/w/players/oracle/ and scratch/w/players/shadow/.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/players/LEDGER.md holds the full record: every probe (hypothesis, evidence, change, result), the /Od and HCEX readouts, the allocator traces and the audits. The allocator tooling copied from Lane A (with its paths pointed at this tree) is in scratch/w/players/oracle/: oracle, greedy, greedy2, greedy3, setpri_id, prio_probe, rounds_dump and blinddiff.

Results:
- Baseline gate is 68 exact / 2 residual.
- **Device:** exact only with a parenthesised radius. A new census shows one real_math.h callee-wrapper macro reproduces all 11 January call sites and lets ai.c and collisions.c drop their per-site decorations with zero section change. It is owner-gated (RB2).
- **Teleport:** the /Od-attested point_from_line3d call fixes January's x87 orders, which proves the hand expansion is byte-wrong. o1 is exact under three forced allocator decisions, but no source lever was found and the stop rule applied.
- **Object storage:** the zero-byte packet (production.patch plus 3 symbols.json static flags) is ready. The remaining storage for the two header-declared functions is blocked by declaration-count canaries.

No tracked file was edited.
