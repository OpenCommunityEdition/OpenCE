# REVIEW (wave 4): rasterizer_xbox_models - P7 drop the dead `#include "interface/hud_draw.h"`

Reviewer slug: scratch/w/review4_rasterizer_xbox_models/ (tree 16542e46; git status clean before and after, 0 entries).
Under review: scratch/w/hud_stack_check/patches/P7_rasterizer_xbox_models_c_drop_dead_hud_draw_include.patch
(worker verdict ADMISSION_FIX_PROPOSED; worker ledger sections L1, H2, RESULT).

VERDICT: APPROVE P7 as-is. It earns zero credit and is byte-inert. It is a prerequisite of the hud_draw.h P1 packet.
It does NOT admit rasterizer_xbox_models: the object stays 12/14 with 2 parks, and see "Scope" below.

## Mechanics
- `git apply --check` of P7 on 16542e46: clean. My own CRLF-preserving derivation (delete that one line) equals the
  worker's candidate file byte for byte.
- P7 also applies on top of the models worker's wave-4 production.patch, and that patch applies on top of P7
  (the hunks are independent).

## Byte evidence (independent tools: rawcmp.py, gate.py, object_audit.py, provider_link.py)
- The gate rows for prod.c and cand_p7.c are identical: 12 exact, 2 residual (draw, env).
- rawcmp (every section's bytes, flags and relocations by name, plus the full symbol table including COMDAT aux):
  prod vs cand_p7 has 71/71 sections and 233/233 symbols, and the only difference is the `.file` aux (source path).
  The same holds against build/base. So the parked bodies are byte-identical: draw 89b0d7ea and env 5179768e, both
  equal to config/parked.json.
- P1 interaction, reproduced with a shadow header built from git blob + P1 (equal to the worker's header; the redirect
  harness is inert):
  - P1 without P7: `__rasterizer_model_transparent_geometry_submit` goes EXACT -> residual [sha]. Only that section moves.
  - P1 with P7: the TU no longer includes hud_draw.h, so its bytes are production's.
- /W3 /Zs census: 13 warnings for both prod and cand, the same multiset. The only change is a 1-line TU shift of the
  pre-existing C4013. No new C4013, so no hud_draw.h prototype was being used.
- object_audit gives the same text for prod and cand. Its FAIL(7) consists of the 2 parked .text sections and the
  5 XDK-wrapper storage rows. Those rows are fixed by the separately reviewed symbols.json ownership packet.
- surplus_identity: the 3 code COMDATs are IDENTICAL. provider_link on cand_p7.obj: 16/16 PASS in both orders.
  Data is 1912/1912 and unchanged.

## Dead-include evidence
- None of the 27 names hud_draw.h declares (enum and constants, struct tags, 13 prototypes, guard) is referenced in
  rasterizer_xbox_models.c. The one exception is `struct bitmap_data`, which rasterizer.h and render.h already
  declare before its use at line 337.
- hud_draw.h's own includes are already in the TU: integer_math.h via bitmap_color_conversion.h, and real_math.h
  directly. Both stay inside the REAL_MATH_EXTERNAL_POINT_FROM_LINE3D bracket, and no point_from_line3d COMDAT
  appears (symbol table identical).
- Provenance (verified in git): 635bd83d added the include while hud_draw.h declared real_argb_color_to_pixel32 and
  real_rgb_color_to_pixel32, which models.c called. The owner's ed3e308a moved the ARGB prototype to
  bitmap_color_conversion.h ("Its consumers include that owner header"). models.c already included that header and
  was left untouched, so the include is migration residue.

## House rules, strip test, owner-gated classes
- The diff only deletes a line. It adds no decoration, filler, prototype, name, block scope, view or COMDAT. P7 is
  itself the plainer spelling, so the strip test passes trivially.
- No section-7/8/9 blocker is introduced. It is not a header edit, so no consumer sweep is needed for P7 itself.
  It needs no symbols.json edit.
- Name-count class disclosure: P7 is load-bearing only relative to P1. I do not consider it declared-name-count
  compensation (the held Layer-2 class). That class chose which prototypes to move by count alone. P7 removes an
  include that is dead and documented as a reconstruction artifact. It is byte-inert on its own, and it would be
  correct hygiene even without P1.
- The K-oracle (lab only, dummies at the include position) confirms that P7 does not put `submit` on a knife edge:
  - production is exact at K = 0..3, 5 and 27..30, which reproduces the worker's oracle;
  - with P7 it is exact at K = 0..10, 12..15 and 24..30.

  So P7 tolerates more later header additions than production does. If the owner still rejects P7, the worker's
  P-A fallback (a focused header, measured identical) remains.

## Integrator notes
1. Apply P7 together with, or before, P1. P1 without P7 loses submit (1,296 B).
2. COMPOSITION HAZARD: if the models worker's wave-4 production.patch and parked.patch
   (scratch/w/rasterizer_xbox_models) also land, the combined tree gives these results:
   - draw normalized_sha256 = d0f694cc2484838753ad6add239c2493bfd8b345ecc4b7008df7f8c9323f192b (5168/348);
   - that differs from parked.patch's 771bf2e7 and from parked.json's 89b0d7ea;
   - env a4a4ca43 and submit exact are unchanged.

   You must re-baseline the draw park, including objdiff_percent, on the combined tree. Otherwise the parks go
   stale or PROGRESS fails.
3. The object is not admissible. The two residual parked functions remain, and so does the TU-local struct-view and
   C4013 debt recorded in the models ledger. Read ADMISSION_FIX_PROPOSED as "clears models out of the hud_draw.h
   packet's blast radius", not as an admission of models.
