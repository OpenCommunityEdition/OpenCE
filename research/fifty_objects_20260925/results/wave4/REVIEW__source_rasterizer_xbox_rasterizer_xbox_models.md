# REVIEW source/rasterizer/xbox/rasterizer_xbox_models

approve: True

## per_object
null

## checks
All work is in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_rasterizer_xbox_models\ (LEDGER.md sections V1-V9, REVIEW.md). Tree 16542e46; git status was clean (0 entries) before and after.

1. Patch check. `git apply --check` of P7 on the current tree is clean. I derived the candidate myself (CRLF kept, the one include line deleted) and it matches the worker's cand/interface/rasterizer_xbox_models.c byte for byte.

2. Gate. `gate.py --source prod.c --all` and `--source cand_p7.c --all` give identical rows: 12 exact, 2 residual (`__rasterizer_model_draw` [relocs 348!=350, sha]; `_rasterizer_model_draw_environment_shader` [sha]).

3. Raw object comparison. I wrote my own comparer (rawcmp.py): every section's bytes, flags and relocations by symbol name, plus the full symbol table including COMDAT aux records. prod vs cand_p7 is 71/71 sections and 233/233 symbols. The only difference is the `.file` aux (source path). The same holds against build/base. The park hashes are unchanged: draw 89b0d7ea and env 5179768e, both equal to config/parked.json.

4. P1 interaction, reproduced with a shadow hud_draw.h (git blob plus P1, identical to the worker's header; the redirect harness control is inert).
   - P1 without P7: `__rasterizer_model_transparent_geometry_submit` goes EXACT -> residual [sha]. Only that section changes, and no `_check_stack_buffer` COMDAT appears.
   - P1 with P7: production bytes, because the TU no longer includes hud_draw.h.

5. Warnings. A `/W3 /Zs` census (my own copy, temp file kept in the slug) finds 13 warnings in both versions, the same multiset. The pre-existing C4013 shifts by one line; there is no new C4013.

6. Dead-include evidence.
   - Grep for all 27 names hud_draw.h declares: 0 hits, except `struct bitmap_data`, which rasterizer.h and render.h already declare before its use at line 337.
   - hud_draw.h's own includes are already in the TU: integer_math.h via bitmap_color_conversion.h, and real_math.h directly.
   - git provenance: 635bd83d added the include while hud_draw.h declared `real_argb_color_to_pixel32` and `real_rgb_color_to_pixel32`, which models.c called. The owner's ed3e308a moved the ARGB prototype to bitmap_color_conversion.h, which models already includes.

7. object_audit. The output text is identical for prod and cand: FAIL(7) = the 2 parked .text sections plus 5 XDK wrapper storage rows, which the separately reviewed symbols.json packet fixes.

8. Surplus and links.
   - surplus_identity: the 3 code COMDATs are IDENTICAL.
   - provider_link on cand_p7.obj: 16/16 PASS in both orders.
   - build/report.json: data 1912/1912.

9. Consumer grep. `grep -rl 'hud_draw\.h' source` finds 23 .c files and no header that includes it, matching the worker's list.

10. K-oracle (lab only: K dummy enum constants at the include position, never landed).
   - Production: submit is exact at K=0..3, 5 and 27..30. This reproduces the worker's oracle.
   - With P7: submit is exact at K=0..10, 12..15 and 24..30.
   - All other exact rows stay exact at every K. P7 widens the tolerance rather than placing submit on a knife edge.

11. Composition with the models worker's wave-4 production.patch (scratch\w\rasterizer_xbox_models). The two patches apply in either order.
   - production.patch alone: draw 771bf2e7 (= its parked.patch).
   - production.patch + P7: 12/2 and submit stays exact, but draw becomes d0f694cc2484838753ad6add239c2493bfd8b345ecc4b7008df7f8c9323f192b.

12. House rules, strip test and owner-gated classes. The diff only deletes a line: no added decoration, name, prototype, scope, view, COMDAT or symbols.json edit. P7 is itself the plain spelling, so the strip test passes trivially. It adds no section 8/9 blocker.

## issues
P7 is approved as-is. It is a zero-credit, byte-inert removal of a dead include. The notes below are for the integrator and owner; none is a defect in the patch.

(1) This is not an admission of rasterizer_xbox_models. The object stays at 12/14 functions with 2 parks (draw, env). object_audit still fails with FAIL(7): the 2 parked sections plus 5 wrapper storage rows, which the sibling symbols.json packet fixes. The TU-local struct views and the C4013 debt are also still there. Read ADMISSION_FIX_PROPOSED as "takes models out of the hud_draw.h P1 packet's blast radius", not as closing models.

(2) Land P7 together with P1, or before it. P1 without P7 turns `__rasterizer_model_transparent_geometry_submit` (1,296 B) from exact to residual. I reproduced that.

(3) Composition hazard. If the models worker's wave-4 production.patch and parked.patch also land, the combined tree moves the zero-credit draw park body:
- the draw normalized_sha256 becomes d0f694cc2484838753ad6add239c2493bfd8b345ecc4b7008df7f8c9323f192b (5168/348);
- parked.patch expects 771bf2e7 and parked.json expects 89b0d7ea, so both would be wrong;
- env (a4a4ca43) and submit (exact) are unaffected.

In that case, re-baseline the draw park on the combined tree, including objdiff_percent, or the parks go stale and PROGRESS fails. P7 alone keeps both current park hashes.

(4) Disclosure for the owner. P7 changes bytes only in combination with P1. I do not class it as declared-name-count compensation (the held Layer-2 class), because the include was not chosen by count. It is provably dead: no hud_draw.h name is used, and no warning changes. git history documents it as residue of the owner's own ed3e308a ARGB migration. It is byte-inert on its own. There is no first-party evidence of January's include list either way. If the owner still objects, the worker's P-A fallback applies: a focused header included only by the 5 sentinel TUs, measured identical.

(5) Housekeeping. I left gate.py's usual scratch/_gate_<pid>.c/.obj temporaries in scratch/ and did not delete them, because they are outside my slug.

Production change: apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_stack_check\patches\P7_rasterizer_xbox_models_c_drop_dead_hud_draw_include.patch unchanged. It deletes only `#include "interface/hud_draw.h"` from source/rasterizer/xbox/rasterizer_xbox_models.c, and `git apply --check` is clean on 16542e46. No amended patch is needed. Evidence and notes are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_rasterizer_xbox_models\REVIEW.md and LEDGER.md.
