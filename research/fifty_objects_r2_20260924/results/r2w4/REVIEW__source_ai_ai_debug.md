# REVIEW source/ai/ai_debug

approve: True

amended_patch: Use C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_ai_debug\01_ai_debug_amended.patch instead of scratch\w\critic_3\patches\01_ai_debug.patch. It is LF like the index and passes git apply --check at 5d3ca708. Optional 02 (scratch\w\critic_3\patches\02_ai_debug_extra_static.patch, copied into the review dir) applies unchanged after it.

The only difference from critic_3's 01 is in source/ai/ai_debug.c; the symbols.json hunk is identical.
- In the stub-removal hunk (now `@@ -755,27 +748,6 @@`), the line `-/* Render the path-node overlays selected by the caller. */` is removed together with the stub.
- In the a910 hunk (now `@@ -3851,7 +3823,8 @@`), it becomes:
```
-static void code_0003a910(
+/* Render the path-node overlays selected by the caller. */
+static void ai_debug_render_path_nodes(
```
The amended candidate source is scratch\w\review_critic_3_ai_debug\t01a\source\ai\ai_debug.c.

## checks
All checks ran at HEAD 5d3ca708. The task text named 7b9de585; the tree was clean apart from the untracked research/ directory. My slug is scratch/w/review_critic_3_ai_debug/, because the orchestrator's slug string was garbled. I claimed and then released source/ai/ai_debug. I edited no tracked file and ran no ninja, configure or git mutation.
1. git apply --check 01 at HEAD: OK. In a scratchpad copy outside any repo, 01 and then 02 both applied cleanly. GNU patch onto `git show HEAD:` copies reproduces critic_3's shadow_ALLxs ai_debug.c (ignoring CR). symbols.json changes in exactly 32 rows: 21 renamed with "static" in 01, and 11 given "static" only in 02. No code_ identifiers remain.
2. Names, checked independently:
 - Sept-2001 cachebeta.map, Static symbols: all 21 names are statics. The in-order pairing between shared anchors is unique, and 20 of 21 sizes match. The exception is 0x38280, which is 0x150 in January and 0xf0 in September.
 - January atlas 4cc87b45 (lib ai_debug.obj): it gives the proposed name at the exact January address for 19 of 21 rows. The task text says 18.
 - The other two: render_spatial_effects is exact in the Jan-2002 cachebeta_xbe atlas. get_this_actor is exact in the Aug-2001 atlas and appears in the Sept map.
 - From the January split relocations, 0x38280's callees are the Sept callee list with local_player_exists, display_assert and system_exit added. Its only caller is 0x39e10 (select_this_actor).
 - cachebeta.pdb has no public at any of the 28 function or 4 data RVAs, and none under any of the new names.
 - January exe literals (ai_render_spatial_effects, ai_render_paths_nodes, the ai_debug_lineofsight overflow asserts and others) support the names; none contradicts them.
 - Neither HCEX PDB contains any ai_debug function.
 - The removed stub came from 37e96a1c, a 'TODO: finish' skeleton (the punpckhdq project is a skeleton, not first-party source). Removing it loses nothing authentic.
3. Emulated split, from my own csplit runs: split_ctl equals build/split (833/833). split_01 and split_02 differ only in ai_debug.obj, and only by renames plus storage 2->3 (21, then 11 more). Section bytes and relocation rows are identical apart from the target's storage. No relocation symbol name changes with 02.
4. Gate: prod, 01, 02 and my amended 01 are all 59/1, and every per-function row is identical after mapping names. The only residual is _ai_debug_render_actor at 24976 vs 25008, unchanged.
5. objcmp, prod vs candidate: identical except .debug$S and a uniform -35 shift in $L label numbers. lab/stubless.c (HEAD minus only the stub and its prototype) reproduces the shift, and stubless vs candidate is identical under the name map.
6. objdiff 3.3.1 (sha1 3130e428) on prod, 01 and 02: 16446/42523 code, 58/61 functions, 5818/5874 data, 98.09388 fuzzy. Per-function measures are identical.
7. Battery via the with_split redirect:
 - object_audit FAIL(34) -> FAIL(13) -> FAIL(2) with 02.
 - pdb_storage 33 -> 12 -> 1 with 02.
 - surplus_identity 19 surplus COMDATs, all identical to January's copies.
 - provider_link passes in both link orders in every state.
 - Data sections are byte- and relocation-identical, and objdiff data is unchanged.
8. CL /Zs /W3: the candidate drops the stub's 7 C4101 unreferenced-local warnings and adds no new warning.
9. House rules: rule 7 is now satisfied. The patch adds no new scopes, declarations, casts or unusual constructs, so no strip test or /Od declaration-order check applies. It touches no held class, no config entry keys on the old names, and ai_debug has no parks.
Not run (integrator): full ninja, pytest, board sweep, csplit regen.

## issues
1. Defect in 01, fixed by the amendment: removing the dead stub left its leading comment `/* Render the path-node overlays selected by the caller. */` orphaned directly above ai_debug_render_surface, where it misdescribes that function. The amendment moves it above the real ai_debug_render_path_nodes definition. This is byte-inert: gate rows are identical and the object is identical apart from .debug$S.
2. Minor evidence corrections:
 - The January atlas agrees at 19 addresses, not 18.
 - critic_3's note that csplit spells 4 static .data relocations as `_global_ai_debug_firing_position_color_count+N` with 02 is wrong. Raw csplit relocation symbol names do not change. The `+N` form comes from coff_compare's _defined_noncode_destination, which anchors to the nearest external symbol. Both sides normalise the same way, so gate and objdiff are unaffected.
3. Pre-existing issues, not introduced here:
 - The `ai_debug_drawstack_setup` prototype is extern but its definition is static; 02 relies on the static result.
 - The file has consumer-local prototypes for actor_iterator_new and actor_iterator_next.
 - C4013 implicit declarations of console_printf and error.
 - Docstring examples in real_regions.py, emission_shape_oracle.py, context_diff_oracle.py and c2_diag_map.py still name _code_00039990 / _code_0003a910. The comment at ai_scenario_definitions.h:313 still says code_00041220.
 - _ai_debug_render_actor_jmptable has storage 2 in the split but is not a PDB public.
4. Zero credit. The object does not complete: the render_actor residual and the jmptable remain.
5. The integrator must regenerate csplit after landing and run full ninja, pytest and the stable sweep.
6. Patch 02 stays optional. It applies after either 01 or the amended 01, and I verified it.
