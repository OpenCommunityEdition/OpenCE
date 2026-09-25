# source/ai/ai_debug

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/01_ai_debug.patch: word-bounded rename of all 21 static code_<addr> functions to Sept-2001 names in ai_debug.c (65 code_ occurrences go to 0, symbol-listing comment updated), e.g. code_00037890->ai_debug_render_path_line, code_00039990->ai_debug_render_path_node, code_0003a2e0->ai_debug_render_vehicles_enterable, code_00038280->ai_debug_get_this_actor, code_0003a910->ai_debug_render_path_nodes. The dead, never-called stub ai_debug_render_path_nodes and its prototype are removed so 0x3a910 can take that first-party name (the w2 20260914 proposal). symbols.json: 21 rows renamed in place and given "static": true (source already static; PDB not public). Optional patches/02_ai_debug_extra_static.patch (apply after 01) adds "static": true to 11 more rows (7 functions + 4 .data). Zero credit.

## evidence
Sept vs January relocation-masked streams: 15 of 21 identical, 5 at 0.93-0.98. code_00038280 (0x150 vs Sept 0xf0) is Sept's callee list plus a prefixed local_player_exists assert; its sole caller is select_this_actor in both builds and the Aug-2001 map has the same name. All callee and caller lists agree. January atlas 4cc87b45 carries the same names at 18 addresses. No January literal contradicts any name. Gate is 59/1 before and after with identical rows. objcmp against production is identical except $L local-label numbering, which comes only from removing the dead stub (lab/ai_debug_stubless.c proves it). In the emulated split only ai_debug.obj changes: renames plus 21 storage flips from 2 to 3. objdiff 3.3.1 is unchanged (16446/42523 code, 58/61 functions, 5818/5874 data). object_audit goes FAIL(34)->FAIL(13), or FAIL(2) with 02. pdb_storage goes 33->12, or 1 with 02. surplus 19/0 and provider_link PASS are unchanged.

## blockers
Not complete: _ai_debug_render_actor is still residual (25008 vs 24976), and the _ai_debug_render_actor_jmptable ownership is still MISSING (object_audit FAIL(2) even with 02). With 02, csplit spells 4 static .data relocations as _global_ai_debug_firing_position_color_count+N. This is disclosed; gate, objdiff and audit are unaffected.

## reopen_criteria
A newer first-party map, or a January literal naming any of these functions differently. The render_actor residual and the jmptable are separate routes.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md