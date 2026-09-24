# source/objects/widgets/light_volumes

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "_light_volume_render"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\light_volumes\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\light_volumes\\symbols.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\light_volumes\\config_parks_status.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\light_volumes\\candidate\\source\\objects\\widgets\\light_volumes.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\light_volumes\\candidate\\source\\objects\\widgets\\light_volumes.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\light_volumes\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\light_volumes\\config\\parked.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\light_volumes\\config\\config.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\light_volumes\\cand_final.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\light_volumes\\split\\source\\objects\\widgets\\light_volumes.obj"
]

## production_changes
Apply three patches. Together they pass `git apply --check` against 931ed8dc.
(1) C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\light_volumes\production.patch changes source/objects/widgets/light_volumes.c and light_volumes.h:
- pow1 body becomes `return exponent != 1.f ? pow(value, exponent) : value;`.
- light_volume_render is rewritten to the /Od shape: a `brightness` accumulator set to 1.f after the marker lookup; an inner block that declares eye_to_marker_vector, parallel_factor and external_scale; vector_from_points3d; `ABS(dot_product3d(&render.camera.forward, &marker.matrix.forward))`; `external_scale = 1.f` before the far-fade if; `brightness *= PIN(...)` twice; the object_get_function_value test inside the inner block; a `for` loop with a short count and a short sprite_index; point_from_line3d for the sprite position; `color.alpha *= brightness` as its own statement.
- `static struct light_volume_globals light_volume_globals = {0};`.
- The light_volume_get macro moves from the header into the .c macros section. The header's `extern struct light_volume_globals light_volume_globals;` and its empty globals section are removed.
- Full files: scratch\w\light_volumes\candidate\source\objects\widgets\light_volumes.c and light_volumes.h.
(2) config/symbols.json: full edited copy at scratch\w\light_volumes\config\symbols.json, one line edited in place (line 23105): `{ "file_offset": 4550032, "flags": 0, "name": "_light_volume_globals", "static": true },`. The same edit is in scratch\w\light_volumes\symbols.patch. Needs a csplit regen. Measured: of the 833 split objects, only light_volumes.obj changes, and only this symbol's storage class (2 to 3).
(3) scratch\w\light_volumes\config_parks_status.patch, with full copies in scratch\w\light_volumes\config\parked.json and config.json:
- retires the _light_volume_render park (the only light_volumes park);
- sets config.json light_volumes.c to "Matching". Apply the status change only after the full integration chain passes.
No other files change. There is no admission-rejection or data entry for this unit, and none is needed.

## evidence
Before: 9 exact / 1 residual; render 928/47, frame 0xa8. After: gate --all 10/10 EXACT; render 912/47 passes strict section_infos_equal.
The prior waves did not use the later /Od build (0x79ef80, RTC: marker, eye_to_marker_vector, external_scale, position, color). It fixed the statement shape:
- brightness accumulator, 1.0 set after the marker lookup;
- vector_from_points3d, plus dot_product3d as real calls;
- external_scale = 1.0 set before the far-fade test;
- `brightness *= PIN` twice;
- for loop with a short index;
- point_from_line3d as a real call.
With that shape (probe v1), everything before the loop became instruction-identical to January.

Escape-scope law: January's [ebp+8] holds both the escaped external_scale and the count-1 temp. So external_scale's block closes before the loop (v2, frame 0xac).

The last residual was the four inlined pow1 joins: ours used memory phis, January keeps the result on the x87 stack. The /Od pow1 at 0x79fb90 is a ternary with the pow arm falling through. Its callee chain is pow(float,float) 0x525c50, then powf 0x525d60, then CRT pow. The real_math `power` wrapper is a different function (0x69fab0), called only from weapons.c's `power(...)` site. So pow1 calls C pow directly.
In C, `exponent != 1.f ? pow(value, exponent) : value` has type double. It compiles to January's exact out-of-line _pow1 and to its x87-phi inline form. The w2 ternary negative used power(), whose ternary is float. Result: EXACT.

Strip tests, each changing one element from the exact candidate:
- Load-bearing (reverting breaks exactness): point_from_line3d, vector_from_points3d, dot_product3d, the accumulator (reverting gives 928/48), external_scale's inner scope (0xa8), the for loop, and the statement positions of both 1.f assignments (initializer forms go residual).
- Inert: the two-statement alpha and the loop-local declaration order.

Whole-object audit on cand_final.obj:
- object_audit against the regenerated split: PASS; pdb_storage: 0 disagreements.
- _light_volume_globals is absent from the cachebeta publics, no January object references it, and HCEX calls it `File Static`.
- Surplus _dot_product3d, _point_from_line3d and _vector_from_points3d are IDENTICAL to January's selected copies (action_charge, action_charge, action_alert). nodup_census finds 0 NODUP definers board-wide. provider_link passes in both link orders.
- The _power surplus is gone.
- Data is 284/284 at 100% in build/report.json.
- widgets.obj, the only other includer of light_volumes.h, is raw-identical (20/20 sections).
- fake_match_scan: 0 leads. /W3: only a new C4244 (double to real) at pow1's return.

## blockers
None at the object level. The integrator still needs to:
- run the full chain (ninja, progress, stable diff, parks, admission, pytest);
- regenerate the split with csplit after the symbols.json edit;
- run a reviewer check of the header-inline COMDAT class under house rule 6 / owner ruling 20260921 #2. The new surplus is _dot_product3d, _point_from_line3d and _vector_from_points3d, all byte-identical and link-clean, with a strictly exact caller.
Note: January's light_volumes object does not reference _point_from_line3d out of line. Ruling 20260921 #1 was narrower, but the later rule-6 text in docs/campaign_house_rules.md admits the all-inlined class, including _point_from_line3d, under exactly these conditions. The hand-expanded alternative is both forbidden by rule 6 and measured non-exact (s1).

## reopen_criteria
Not applicable: every function is exact and the audit passes. Reopen only if the integration sweep shows a regression in widgets.c or in the three helpers' provider links, or if the owner rejects the all-inlined header-inline COMDAT class for _point_from_line3d.

## task notes
A new, reusable VC7 law came out of this work. The lab evidence is in scratch/w/light_volumes/lab/r.* and lab/p.*.
- Suppose an inlined helper's result is a float expression: an if-form `if (c) v = f(v); return v;`, multiple returns, or `c ? power(v,e) : v`. VC7 merges that return temp with the caller's float destination, which gives a memory phi.
- `c ? pow(v,e) : v` with C pow is typed double. It cannot merge, so VC7 keeps it on the x87 stack: an eager `fld v` after the compare, then `fstp dest` at the join.
- Both forms produce the same out-of-line bytes. So an 'x87 residency tie' in an inlined helper may really be the expression type of the helper's return.
- A board scan (cipow_scan.py) found January's eager-load pow diamond only in _light_volume_render.

The earlier 'pre-loop x87 contest tie' was a source-shape difference. The /Od statement order and the real helper calls fixed it outright.

Tool note: the Bash tool mangles backslashes inside heredocs. Use the Write tool for any Python that contains backslashes.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\light_volumes\LEDGER.md
