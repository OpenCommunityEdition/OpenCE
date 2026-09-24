# ai/actor_moving, ai/ai_debug, interface/hud_nav_points, interface/motion_sensor, units/bipeds, units/vehicles

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
null

## candidate_files
null

## production_changes
No file of their own; they are fixed by objects_h.patch + action_vehicle.patch.

## evidence
Before the packet, each emitted a `_object_get_bounding_sphere` copy that referenced the c:\ literal. That copy was DIFFERENT from January's selected copy and hit LNK2005 against the NODUP hand copy.

With the packet:
- The only change in each is the COMDAT's 2 DIR32 literal targets plus the literal section.
- Each copy is now IDENTICAL to January's selected copy (selection ANY).
- The pair links pass in both orders.
- Per-function rows are unchanged: 31/36, 59/60, 31/32, 16/19, 42/51, 37/39.

## blockers
None of them is complete; each still has its own residual functions. bipeds also has unrelated NODUP conflicts: `_cross_product2d` (actor_combat) and `_object_get_type` (items).

## reopen_criteria
n/a

## task notes
No tracked file was edited and I never ran ninja or git-mutating commands. The shadow tree for the final packet is scratch/w/bounding_sphere/treeF.

1. **Mechanism behind the per-unit strings.** January's objects.h asserted with __FILE__, so its spelling depends on how each unit found the header. Units in source\objects found it beside themselves and got "c:\halo\source\objects\objects.h"; everyone else found it through the ..\objects include path and got "..\objects\objects.h". January's literal census across the board fits this rule without exception. The fix models that with a header macro (default "..\") that object_lights overrides. It adds no declarations.

2. **Side effect in physics and collisions.** In the base tree, physics_update_old and collision_move_point each move one register reload when objects.h changes. Both functions were already residual, so no credit changes. The only preprocessed change in those units is the set of distinct string literals inside an unreferenced inline body, so literals appear to count in the unit's name-count currency. The pending collisions packet F (scratch/w/collisions/final) stays 20/20 EXACT together with this packet (tested in tree2).

3. **New tools** in scratch/w/bounding_sphere/: tgate.py (shadow-tree compile), sweep.py (whole-board shadow-tree sweep), objcmp.py, surplus_cand.py (also reports COMDAT selection types), plink.py and provider_link_cand.py (provider link with a candidate provider override).
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\bounding_sphere\LEDGER.md
