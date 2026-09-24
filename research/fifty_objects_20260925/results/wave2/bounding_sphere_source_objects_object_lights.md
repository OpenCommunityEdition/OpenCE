# source/objects/object_lights

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
null

## candidate_files
null

## production_changes
patches/object_lights.patch (part of the render_objects packet). Adds `#define OBJECTS_H_FILE "c:\\halo\\source\\objects\\objects.h"` before the includes, with a comment. object_lights.obj stays objcmp-IDENTICAL to production.
The patch composes cleanly, in either order, with scratch/w/data_gap/patches/object_lights.patch (4 statics + should_render_lights) and with data_gap's symbols_json_all.patch.

## evidence
January's __FILE__ census covers every path-literal symbol on the board:
- Main files are always "c:\halo\SOURCE\<dir>\x.c".
- A header gets "c:\halo\source\<dir>\h.h" exactly when the unit using it sits in the header's own directory. Examples: ai\path.h, ai\encounters.h, sound\sound_*.h, hs\hs_library_*.h, widgets\widget_types.h.
- Otherwise the header gets "..\<dir>\h.h", found through the /I list. Examples: real_math.h, bitmaps_inlines.h, reference_lists.h.
- objects.h is the only header with both spellings.

What that shows for this helper:
- `_object_get_bounding_sphere` is defined in action_vehicle, the selected copy, which references the "..\" literal. There are 8 UNDEF users, each of which emitted its own COMDAT copy.
- "c:\halo\source\objects\objects.h" is defined only in object_lights, and no relocation anywhere references it. It is an orphan: object_lights' discarded COMDAT copy was its only possible referrer, and /OPT:NOREF kept it.
- The /Od build confirms the inline used __FILE__: fn 0x443f50 is object_get_bounding_sphere, with assert lines 0x2a9/0x2aa and the string "D:\P4\Halo1\source\objects/objects.h".

Combined with data_gap's packet, against data_gap's scratch split:
- 43/43 EXACT.
- object_audit: PASS 71/71.
- pdb_storage: 0 disagreements.
- provider_link: PASS against the packet's action_vehicle; the full 9-unit group also links clean in both orders.
- Surplus: 11 of 12 COMDATs are identical. `_object_get_bounding_sphere` differs, by design (it uses the c:\ literal).

Control without the define (tree5): the January-owned 33-byte .rdata goes MISSING, .rdata drops to 96.93%, and objdiff matched_data falls from 1104 to 4, costing 1,100 data bytes.

## blockers
Owner ruling needed. Rule 6 requires each surplus COMDAT to be byte-identical to January's selected provider, and object_lights cannot meet that in any source. Its January-owned literal is emitted only if its own `_object_get_bounding_sphere` copy references it, and that makes the copy differ from action_vehicle's. January's own object had the same difference. Admission also still needs data_gap's packet: the 4 statics, the should_render_lights call and the single-section `_lights_section` semantic_data_matches entry with allow_incomplete_unit.

## reopen_criteria
Reopen when the owner rules on whether a surplus header-inline COMDAT may differ from the selected copy only in an authentic, January-proven per-TU __FILE__ literal. If the ruling is yes, the object is ready once data_gap's packet lands. Also reopen if a new asserting objects.h inline appears in another objects/ unit; it would need the same define.

## task notes
No tracked file was edited and I never ran ninja or git-mutating commands. The shadow tree for the final packet is scratch/w/bounding_sphere/treeF.

1. **Mechanism behind the per-unit strings.** January's objects.h asserted with __FILE__, so its spelling depends on how each unit found the header. Units in source\objects found it beside themselves and got "c:\halo\source\objects\objects.h"; everyone else found it through the ..\objects include path and got "..\objects\objects.h". January's literal census across the board fits this rule without exception. The fix models that with a header macro (default "..\") that object_lights overrides. It adds no declarations.

2. **Side effect in physics and collisions.** In the base tree, physics_update_old and collision_move_point each move one register reload when objects.h changes. Both functions were already residual, so no credit changes. The only preprocessed change in those units is the set of distinct string literals inside an unreferenced inline body, so literals appear to count in the unit's name-count currency. The pending collisions packet F (scratch/w/collisions/final) stays 20/20 EXACT together with this packet (tested in tree2).

3. **New tools** in scratch/w/bounding_sphere/: tgate.py (shadow-tree compile), sweep.py (whole-board shadow-tree sweep), objcmp.py, surplus_cand.py (also reports COMDAT selection types), plink.py and provider_link_cand.py (provider link with a candidate provider override).
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\bounding_sphere\LEDGER.md
