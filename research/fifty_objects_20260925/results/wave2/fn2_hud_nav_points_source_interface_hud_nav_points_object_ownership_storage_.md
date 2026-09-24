# source/interface/hud_nav_points (object ownership/storage)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
null

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_nav_points\\ownership.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_nav_points\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_nav_points\\st1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_nav_points\\tl\\object_audit_regen.py"
]

## production_changes
Zero-credit ownership correction, independent of the residual.
(1) Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_nav_points\ownership.patch (`git apply --check` is clean). It adds `static` to the hud_update_nav_point_local_player prototype (line 290) and to its definition (line 1099) in source/interface/hud_nav_points.c.
(2) config/symbols.json: use the edited full copy at C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_nav_points\config\symbols.json. It keeps CRLF and is edited in place, not re-serialised. Line 3422 becomes `{ "file_offset": 811472, "flags": 32, "name": "_hud_update_nav_point_local_player", "static": true },` and line 23038 becomes `{ "file_offset": 4537020, "flags": 0, "name": "_nav_point_data", "static": true },`.
Then run a csplit-only regen.

## evidence
pdb_storage lists 2 disagreements. _nav_point_data and _hud_update_nav_point_local_player are both absent from the cachebeta publics, so January had them file-static, but the split objects mark them external (storage 2). nav_point_data is already static in our source. hud_update_nav_point_local_player's only caller is in this TU; no other source or header references it.

st1 candidate (static added): gate 31 exact, 1 residual, rows unchanged.

csplit into scratch with the edited symbols.json (cfg/, splitout/): only source/interface/hud_nav_points.obj changes, in exactly those 2 storage classes (2 to 3).

object_audit of st1.obj against the regenerated split (tl/object_audit_regen.py): 46 January symbols, 0 differ; FAIL(1) is only the _custom_render_nav_point bytes. Data sections are at 100% in build/report.json.

## blockers
The object still cannot complete:
(1) The _custom_render_nav_point 8-byte schedule (see the unit above).
(2) The _object_get_bounding_sphere provider conflict, the known item in brief section 7. provider_link fails with LNK2005 in both link orders. Our COMDAT comes from the objects.h __inline (assert file "c:\\halo\\source\\objects\\objects.h"), pulled in via units.h and collisions.h. January's selected copy is action_vehicle's hand copy, using "..\\objects\\objects.h". The January string census shows: the relative path is defined in action_vehicle and referenced by render_objects; the absolute path is defined in object_lights. This needs the objects.h / action_vehicle provider lane.
The surplus _fast_ftol, _arctangent and _square_root are IDENTICAL and pass the link.

## reopen_criteria
Land this with the next batch; it is independent of the residual. Admit the object once _custom_render_nav_point is exact and the objects.h/action_vehicle _object_get_bounding_sphere provider repair passes provider_link in both link orders.

## task notes
No tracked files were edited; ninja and git mutations were not run. csplit.exe (a project build tool) was run only against a scratch config copy, writing to scratch/w/hud_nav_points/splitout. The consumer-local prototypes in hud_nav_points.c (object_try_and_get_and_verify_type, object_get_bounding_sphere, game_engine_get_goal_position, unit_get_head_position, etc.) are pre-existing rule-9 debt and were left alone because of declaration-count tie risk. hud_weapon _render_weapon_hud (residual 2624!=2656) likely needs January's two tail-merged hud_draw_numbers calls (if/else); this is useful evidence for that unit's worker.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_nav_points\LEDGER.md
