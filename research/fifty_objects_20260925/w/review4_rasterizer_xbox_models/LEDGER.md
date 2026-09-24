# review4_rasterizer_xbox_models - adversarial review of P7 (drop dead hud_draw.h include) - LEDGER

Reviewer slug. Tree 16542e46, git status clean at start (0 entries).
Proposal under review: hud_stack_check P7_rasterizer_xbox_models_c_drop_dead_hud_draw_include.patch (verdict ADMISSION_FIX_PROPOSED).

## R0 reading
- WORKER_BRIEF 0-9, house rules, lane log (held table: models .bss held; hud_unit row = the wave-4 check_stack_buffer packet), worker LEDGER (hud_stack_check).
- parks: _rasterizer_model_draw_environment_shader (base nsha 5179768e...), __rasterizer_model_draw (base nsha 89b0d7ea...). No rejection entry for the unit.

## V1 patch mechanics + candidate identity
- git apply --check P7 on 16542e46: clean. My own derivation (python, CRLF kept: delete the one line
  `#include "interface/hud_draw.h"` from the worktree file) == worker cand/interface/rasterizer_xbox_models.c byte-for-byte.
## V2 gate + raw object comparison (rawcmp.py, reviewer-written: every section in order incl. bytes, flags,
   relocations by symbol name, full symbol table incl. aux/COMDAT selection; .debug$* by presence only)
- gate prod.c: 12 exact / 2 residual (draw [relocs 348!=350, sha], env [sha]); gate cand_p7.c: IDENTICAL rows.
- rawcmp prod.obj vs cand_p7.obj: 71/71 sections, 233/233 symbols, only diff = .file aux (source path). Same vs build/base obj.
  => P7 alone is byte-inert on the whole object (parked bodies byte-identical => no park drift).
## V3 P1 interaction (shadow header p1tree/ = git blob + P1 via patch; == worker cand header; ctrl harness ctrlhdr/ inert)
- tu_ctrl (include redirected to verbatim copy): identical to prod (only .file). Harness inert.
- tu_p1only (P1 header, include kept): 11/3 - __rasterizer_model_transparent_geometry_submit EXACT -> residual [sha]
  (only section #52 differs; no _check_stack_buffer COMDAT emitted in models). Worker claim REPRODUCED.
- P1+P7 == cand_p7 (TU no longer includes hud_draw.h at all) => production bytes. Worker claim REPRODUCED.
## V4 /W3 /Zs warning census (warn_r.py, slug temp file)
- prod 13 warnings, cand_p7 13 warnings; multiset identical; only TU line shift 1690->1689 (C4013
  rasterizer_frame_statistics_count_static_vertices, pre-existing). No new C4013 => no hud_draw.h prototype was used.
## V5 provenance
- 635bd83d models.c used real_argb_color_to_pixel32 / real_rgb_color_to_pixel32 (lines 1463-2250) and its hud_draw.h declared
  both; ed3e308a (owner, 2026-09-23) moved real_argb_color_to_pixel32 to bitmap_color_conversion.h (models already includes it)
  and did not touch models.c. hud_draw.h's own includes (integer_math.h via bitmap_color_conversion.h; real_math.h directly)
  are already in the TU. grep of all 27 hud_draw.h names in models.c: 0 hits except `struct bitmap_data` (declared by
  rasterizer.h / render.h before use at line 337). Worker provenance claim VERIFIED.
## V6 K-oracle (LAB ONLY, kor/mk.py: K dummy enum constants at the include position; never lands)
- prod : submit exact at K=0..3,5,27..30; residual 4,6..26  (reproduces worker's K-oracle exactly)
- P7   : submit exact at K=0..10,12..15,24..30; residual 11,16..23
  => P7 does not move models onto a knife edge; it widens the tolerance to later header additions (+10 vs +3).
  All other 11 exact rows stay exact for every K in both modes.
## V7 whole-object audit / surplus / links / data (cand_p7.obj)
- object_audit: text IDENTICAL for prod.obj and cand_p7.obj: FAIL(7) = the 2 parked residual .text sections (draw, env) + the
  5 XDK wrapper storage rows (fixed by the separate, already-reviewed symbols.json ownership packet). Object NOT complete; P7
  changes nothing here.
- surplus_identity (production; cand is byte-identical): 3 code COMDATs (_vector_from_points3d, _dot_product3d,
  _plane3d_distance_to_point) all IDENTICAL. provider_link cand_p7.obj: 16 surplus, all PASS both orders.
- build/report.json: data 1912/1912; code 12/14 functions - unchanged (rawcmp: every data section identical).
## V8 consumer list (for the P1 context): grep -rl 'hud_draw\.h' source (all extensions): 23 .c files, no header includes
   it - equals the worker's list; P7 removes models from it (22 remain).
## V9 composition with the models worker's wave-4 production.patch/parked.patch (scratch/w/rasterizer_xbox_models)
- P7 applies (git apply --check) on top of production.patch and vice versa (patch: offset -1, all 10 hunks).
- production.patch alone: 12/2, draw nsha 771bf2e7 (= its parked.patch), env a4a4ca43, submit exact.
- production.patch + P7: 12/2, submit exact, env a4a4ca43, but draw nsha d0f694cc2484838753ad6add239c2493bfd8b345ecc4b7008df7f8c9323f192b
  (5168/348) - neither parked.json's 89b0d7ea nor parked.patch's 771bf2e7 => if both land, the draw park must be re-baselined
  on the combined tree (else parks STALE / PROGRESS fails). P7 alone keeps both park hashes (89b0d7ea / 5179768e).
## VERDICT: APPROVE P7 as-is (zero credit, byte-inert, prerequisite of P1). Not an admission of models. Notes in REVIEW.md.
