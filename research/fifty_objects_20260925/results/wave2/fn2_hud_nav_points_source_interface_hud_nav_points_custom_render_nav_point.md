# source/interface/hud_nav_points::_custom_render_nav_point

## verdict
NO_PROGRESS

## newly_exact_functions
null

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_nav_points\\od1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_nav_points\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_nav_points\\lab.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_nav_points\\tiff\\tv.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_nav_points\\pushload2.py"
]

## production_changes
None. The production body stays as it is (1632/95, 8 bytes off at +0x58A). The park entry in config/parked.json stays active; there are no parks to retire.

## evidence
Baseline gate --all at 931ed8dc: 31 exact, 1 residual, 0 unwritten (_hud_render_nav_points is now EXACT). The only non-naming alndiff difference is at +0x58A. January emits `fistp [ebp-0x10]; push 0; mov eax,[ebp-0x10]; mov ecx,[ebp-0x14]; push 0; push 0; push eax; push ecx`. Ours emits both loads before the three push 0.

New findings this wave (all in scratch/w/hud_nav_points/LEDGER.md):
(1) /Od 0x62fcf0 readout. The later source computes the pow temp in a separate statement, then evaluates the decimal part `fast_ftol(fmod(fabs(t*distance),t))` directly in the argument list, with args 0,0,0.0f. placement is declared before numbers.
(2) The lab harness lab.py appends lab functions to the real TU. Across 19 lab diagnostics the loads were always hoisted to the region head, even when the first register push comes right after P8 (d1). Diagnostics covered: long prototype, no casts, a direct __asm local, an unprototyped call, an aliased local, stores through a pointer, a trivial inline and an assignment in args 7/6, equal-arm and C2-foldable ?: in args 8/7/6, and a C2-dead partner call. Pushes do not alias locals, and inline bodies are hoisted before the whole statement (lab10).
(3) One exact in-compiler donor of the same shape exists: TIFFReadDirectory +0x701. Experiments on a copy of tif_dirread.c (t1/t2/t3) show it is an IL tail-merge block boundary. The load sits exactly at the start of the tuple suffix shared with the sibling MissingRequired sites. It flips to loads-first once the siblings are removed.
(4) January's custom_render window is a single, branch-free call with no jump into +0x58C, so no tail-merge partner exists. Dead partners and folded conditionals are removed before blocks form (lab8/14/15/17).
(5) Oracles, not landable, all inert or worse: 1..12 unused long and 1..9 unused real dummy locals at the function top and in the numbers block; /G6, /G7, /GB, /Ot and /Og give identical output; /G5 breaks 19 functions and /Os breaks all.
(6) The /Od-faithful tail od1 (power(10.0f,4.0f), decimal in the arg list, placement declared first) gives the identical 8-byte residual.
(7) A sibling in January using the same idiom, hud_weapon _render_weapon_hud, uses the identical decimal chain and two tail-merged hud_draw_numbers calls. It is residual in our build (2624!=2656) and belongs to a separate unit.

## blockers
C2 dag.c ordering: VC7 hoists every register load of a region to the region head. The only exceptions found are register conflicts (eax and ecx are free after the asm, so not applicable) and IL block boundaries from tail merging (need a live partner path, which January lacks). No natural source form tested reproduces January's 'push 0 first' window. The methodology stop rule fired after more than 30 prior shapes plus this wave's lab diagnostics.

## reopen_criteria
(a) Authentic January source, or a fragment showing a second hud_draw_numbers path or other live IL block boundary between arg 8's push and arg 7 in custom_render_nav_point. (b) A decoded C2 dag.c rule, from a live trace of the TIFF survivor window against this window, showing another way a constant push is emitted ahead of region-head loads.

## task notes
No tracked files were edited; ninja and git mutations were not run. csplit.exe (a project build tool) was run only against a scratch config copy, writing to scratch/w/hud_nav_points/splitout. The consumer-local prototypes in hud_nav_points.c (object_try_and_get_and_verify_type, object_get_bounding_sphere, game_engine_get_goal_position, unit_get_head_position, etc.) are pre-existing rule-9 debt and were left alone because of declaration-count tie risk. hud_weapon _render_weapon_hud (residual 2624!=2656) likely needs January's two tail-merged hud_draw_numbers calls (if/else); this is useful evidence for that unit's worker.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_nav_points\LEDGER.md
