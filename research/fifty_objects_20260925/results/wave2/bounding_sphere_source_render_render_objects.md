# source/render/render_objects

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bounding_sphere\\final\\render_objects.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bounding_sphere\\final\\objects.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bounding_sphere\\final\\action_vehicle.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bounding_sphere\\final\\object_lights.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bounding_sphere\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bounding_sphere\\patches\\render_objects.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bounding_sphere\\patches\\objects_h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bounding_sphere\\patches\\action_vehicle.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bounding_sphere\\patches\\object_lights.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bounding_sphere\\patches\\symbols_json.patch"
]

## production_changes
Apply all of these together; each passes `git apply --check`, also as a set:
(1) scratch/w/bounding_sphere/patches/objects_h.patch. Adds to the objects.h macros section: `#ifndef OBJECTS_H_FILE` / `#define OBJECTS_H_FILE "..\\objects\\objects.h"` / `#endif`, with a comment. All 4 objects.h match_assert sites (lines 535/536/551/552) now use OBJECTS_H_FILE.
(2) patches/action_vehicle.patch. Removes the `#define object_get_bounding_sphere object_get_bounding_sphere_inline` / `#undef` pair and the NODUP hand copy. This part is mandatory: without it render_objects gets LNK2005.
(3) patches/object_lights.patch. Adds `#define OBJECTS_H_FILE "c:\\halo\\source\\objects\\objects.h"` before object_lights' includes. It keeps object_lights byte-identical.
(4) patches/render_objects.patch. Removes the file-scope `struct render_lighting lighting_storage = { 0 };` and adds `static struct render_lighting lighting_storage;` (uninitialised) as the first declaration in object_get_cached_render_lighting.
(5) patches/symbols_json.patch. The full edited copy is scratch/w/bounding_sphere/config/symbols.json. It is one in-place line op at line 23251: `{ "file_offset": 4980856, "flags": 0, "name": "_lighting_storage" },` becomes `{ "file_offset": 4980856, "flags": 0, "name": "?lighting_storage@?1??object_get_cached_render_lighting@@9@9", "static": true },`. After it, re-run csplit.
The full candidate files are in scratch/w/bounding_sphere/final/.

## evidence
All gates ran on a full shadow tree (tgate.py). That harness was validated first: an unmodified copy reproduced build/base byte-for-byte for every section and symbol.

Function and object checks, against a scratch csplit carrying the symbols.json rename (only render_objects.obj changes in that split):
- render_objects: 22/22 EXACT.
- object_audit: PASS, 41/41 symbols.
- pdb_storage: 0 disagreements.
- Surplus identity: all 9 surplus COMDATs are identical to January's selected copies. `_object_get_bounding_sphere` now references "..\objects\objects.h", as January's did.
- provider_link: PASS in both orders against the packet's action_vehicle. The current base fails both orders with LNK2005.
- Mini objdiff 3.3.1: code 5437/5437 and data 4678/4678, with .bss, .data and .rdata each at 100.

lighting_storage:
- HCEX.pdb lists it as a "Static Local, Type: struct render_lighting, lighting_storage" inside object_get_cached_render_lighting. In the same build render_object_globals is a File Static.
- It is absent from the cachebeta publics, and no January object has an UNDEF reference to it.
- January's .bss puts lighting_storage at offset 0 of 1150 bytes.
- An uninitialised function static reproduces that layout exactly. The `= { 0 }` function-static form moves it to offset 1032 and is refuted.
- A file-scope static (L1) also matches the bytes. It is the fallback if the owner declines the rename, but it contradicts HCEX.

Board sweep: all 447 cl units recompiled from the final tree. Changed units are the 7 objects.h users whose only change is the literal, plus render_objects, plus two functions that were already residual: physics_update_old and collision_move_point. Per-function rows are identical to base in every affected unit. No exact function was lost.

## blockers
None in the object itself. The packet must land in one step: objects.h + action_vehicle.c + object_lights.c + render_objects.c + symbols.json with a csplit regen. The integrator's full ninja / stable diff still needs to run.

## reopen_criteria
Reopen if full ninja shows any exact-row loss, or if the owner rejects the symbols.json function-static rename. In that case use the L1 fallback: `static struct render_lighting lighting_storage = { 0 };` at file scope plus `"static": true` on `_lighting_storage`. It gives 22/22 and the same layout.

## task notes
No tracked file was edited and I never ran ninja or git-mutating commands. The shadow tree for the final packet is scratch/w/bounding_sphere/treeF.

1. **Mechanism behind the per-unit strings.** January's objects.h asserted with __FILE__, so its spelling depends on how each unit found the header. Units in source\objects found it beside themselves and got "c:\halo\source\objects\objects.h"; everyone else found it through the ..\objects include path and got "..\objects\objects.h". January's literal census across the board fits this rule without exception. The fix models that with a header macro (default "..\") that object_lights overrides. It adds no declarations.

2. **Side effect in physics and collisions.** In the base tree, physics_update_old and collision_move_point each move one register reload when objects.h changes. Both functions were already residual, so no credit changes. The only preprocessed change in those units is the set of distinct string literals inside an unreferenced inline body, so literals appear to count in the unit's name-count currency. The pending collisions packet F (scratch/w/collisions/final) stays 20/20 EXACT together with this packet (tested in tree2).

3. **New tools** in scratch/w/bounding_sphere/: tgate.py (shadow-tree compile), sweep.py (whole-board shadow-tree sweep), objcmp.py, surplus_cand.py (also reports COMDAT selection types), plink.py and provider_link_cand.py (provider link with a candidate provider override).
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\bounding_sphere\LEDGER.md
