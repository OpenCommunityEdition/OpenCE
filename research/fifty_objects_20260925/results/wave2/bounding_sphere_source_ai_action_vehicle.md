# source/ai/action_vehicle

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
null

## candidate_files
null

## production_changes
patches/action_vehicle.patch (part of the packet) removes the rename and the NODUP hand copy, so the genuine objects.h inline is used.
OPTIONAL zero-credit ownership fix: patches/optional_action_vehicle_with_static_evaluate_seat.patch REPLACES action_vehicle.patch and adds `static` to both the action_vehicle_evaluate_seat prototype and its definition. patches/optional_symbols_json_with_evaluate_seat.patch REPLACES symbols_json.patch; the full copy is scratch/w/bounding_sphere/config_with_evaluate_seat/symbols.json. It adds `"static": true` to `_action_vehicle_evaluate_seat` (file_offset 38080) and keeps the lighting_storage line op.

## evidence
The unit is already Matching.

With the packet:
- 17/17 EXACT.
- The object is objcmp-IDENTICAL to production in every section, flag and symbol. The genuine inline's COMDAT has the same bytes and relocations as the hand copy; only its COMDAT selection changes from NODUPLICATES to ANY.
- object_audit: PASS 31/31.
- Links clean in both orders with all 8 other users.

Optional evaluate_seat fix:
- `_action_vehicle_evaluate_seat` is non-public in cachebeta and has zero UNDEF references in January.
- Making it static is byte-inert: objcmp shows only storage 2 to 3, and the unit stays 17/17.
- With a scratch split carrying both symbols.json edits (only action_vehicle.obj and render_objects.obj change): object_audit PASS, pdb_storage 0.

## blockers
None. It must land together with objects_h.patch; with the current objects.h, gate --source gives 16/17 (reloc-identity).

## reopen_criteria
None expected.

## task notes
No tracked file was edited and I never ran ninja or git-mutating commands. The shadow tree for the final packet is scratch/w/bounding_sphere/treeF.

1. **Mechanism behind the per-unit strings.** January's objects.h asserted with __FILE__, so its spelling depends on how each unit found the header. Units in source\objects found it beside themselves and got "c:\halo\source\objects\objects.h"; everyone else found it through the ..\objects include path and got "..\objects\objects.h". January's literal census across the board fits this rule without exception. The fix models that with a header macro (default "..\") that object_lights overrides. It adds no declarations.

2. **Side effect in physics and collisions.** In the base tree, physics_update_old and collision_move_point each move one register reload when objects.h changes. Both functions were already residual, so no credit changes. The only preprocessed change in those units is the set of distinct string literals inside an unreferenced inline body, so literals appear to count in the unit's name-count currency. The pending collisions packet F (scratch/w/collisions/final) stays 20/20 EXACT together with this packet (tested in tree2).

3. **New tools** in scratch/w/bounding_sphere/: tgate.py (shadow-tree compile), sweep.py (whole-board shadow-tree sweep), objcmp.py, surplus_cand.py (also reports COMDAT selection types), plink.py and provider_link_cand.py (provider link with a candidate provider override).
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\bounding_sphere\LEDGER.md
