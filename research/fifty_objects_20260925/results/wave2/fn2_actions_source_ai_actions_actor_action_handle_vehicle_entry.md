# source/ai/actions :: _actor_action_handle_vehicle_entry

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\production_fuzzy.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\cand_fuzzy.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\config\\parked_fuzzy.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\owner_gated_exact.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\cand_exact.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\config\\parked_exact.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\actions\\LEDGER.md"
]

## production_changes
LANDABLE NOW: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actions\production_fuzzy.patch. It passes git apply --check and round-trips byte-identical to cand_fuzzy.c and config/parked_fuzzy.json. (1) source/ai/actions.c: this function's body is replaced with the /Od-attested reconstruction. The patch changes nothing else. The shape is `if ((flee && panic_type > none) || obey) goto result_exit;`, then `if (last_vehicle_check_time == NONE || last_vehicle_check_time + 45 < current_time) {...}`. It stores best_vehicle_index first in the prop arm, uses `while ((prop = prop_iterator_next(&iterator)) != NULL)`, `short slot` and `short index` counters, the /Od `vehicle` local in the scripted loop and DATUM_INDEX_TO_ABSOLUTE_INDEX. The RTC names are vehicle_origin, position and new_state_data. One `return result;`. The square stays the plain `prop->distance * prop->distance`. (2) config/parked.json: the vehicle_entry park stays. Its evidence text is rewritten, measurements.base.normalized_sha256 becomes 7ef7a0e0ac5c66eda94a6d1f28f76ba291e23c27e1d9c10706ffb2822804dd32 (size 960, relocs 22 unchanged) and objdiff_percent becomes 99.319725. HELD (owner-gated, not landable): C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actions\owner_gated_exact.patch. It is the same body with the square spelled `actor_action_distance_squared(prop->distance)`, plus a TU-private `#define actor_action_distance_squared(distance) ((distance) * (distance))` with a disclosure comment, and it removes the vehicle_entry park. It passes git apply --check and round-trips to cand_exact.c and config/parked_exact.json.

## evidence
Baseline at 931ed8dc: gate --all gives 62 exact / 1 residual. The residual is vehicle_entry at 960/22 [sha] with two families: a transposition at +0x17b and a false-epilogue topology. The fuzzy candidate (cand_fuzzy.c) gates 62/1 and alndiff leaves ONE adjacent FP/int transposition at +0x17b (5 bytes, 300/300 instructions): January emits `fld st(0); fmul st(1); mov [ebp-4],ecx`, ours emits `fld st(0); mov [ebp-4],ecx; fmul st(1)`. objdiff 3.3.1 goes from 96.54422 to 99.319725. secdiff shows only the vehicle_entry section changes; fake_match_scan reports 0 leads; parked_functions on a patched report/config gives 93 active, 0 stale, 0 invalid. The owner-gated cand_exact.c gates 63/63 EXACT (sha df9b95a1... equals January). object_audit PASS: 112 symbols, 0 differ. pdb_storage: 0 disagreements. surplus_identity: 11 COMDATs, all identical. provider_link: SELECTED-PROVIDER LINK PASS. No admission rejection. Parks go to 92/0/0. New mechanism finding (square battery d1-d10 on the fuzzy body): a paren around ONE operand, `(d) * d` or `d * (d)`, stops VC7 from sharing the load (`fld d; fmul dword [esi+0x11c]`). So each paren is a separate IL node, and January's two operands carried the same wrapper: one macro argument substituted twice. An outer paren only, a no-op `(real)` cast and `best = d; best *= best` all reproduce the residual; `best = d; best *= d` also breaks the load sharing. The named temp (VE-od6) was already measured inert. /Od 0x42ade6 shows an inline mulss with no helper call. This is the same family as the admitted actor_perception_distance_squared (ruling 2026-09-20) and the actor_moving sideslip square. Full log: scratch/w/actions/LEDGER.md.

## blockers
The strict match needs the operand-parenthesised square. Removing the macro and writing the square plainly (strip test) leaves the function residual. `(d) * (d)` written by hand also matches, which is the same weaker-evidence class as the second actor_perception site. Landing it needs an owner ruling that extends the 2026-09-20 TU-private inferred-macro ruling to actions.c (structural owner packet 20260923, section 7). The object also needs the data admission fix in the next unit.

## reopen_criteria
An owner ruling admitting the TU-private squaring macro in actions.c: apply owner_gated_exact.patch instead of production_fuzzy.patch. Or first-party evidence of a squaring macro, in a shared header or another source. Do not repeat V1-V5, VE1-VE9, od1-od11, g1-g7, the square battery d1-d10, double temps (/Od shows single-precision mulss) or declaration/statement orders.

## task notes
Summary of state:
- Landable now: production_fuzzy.patch. vehicle_entry is still not exact, but it is down to one 5-byte FP/int transposition, with an updated park measurement. It adds zero credit and changes no other section.
- Held: owner_gated_exact.patch makes the whole object's code 63/63 strict exact, but only with a TU-private ((x) * (x)) squaring macro. The owner must extend the 2026-09-20 actor_perception macro ruling to actions.c. If the owner approves, apply it INSTEAD of production_fuzzy.patch; it also retires the vehicle_entry park.

Data: object completion also needs verifier_combined_extent.diff plus semantic_data_entry.json, because a single-section '$'-defect entry cannot cover this unit under objdiff 3.3.1. With both in place, the admission audit reports the object as an admission candidate (function_gap 0, data_gap 0).

All work stayed under scratch/w/actions. No tracked file was edited, and no ninja or git-mutating command was run.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\actions\LEDGER.md
