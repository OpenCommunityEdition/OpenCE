# Review 4: source/interface/hud_draw, ADMISSION_FIX_PROPOSED (worker slug hud_stack_check), tree 16542e46

## Verdict

**approve = false as submitted.** The content of P1 (hud_draw.h) and P2 (hud_draw.c) is correct, and I would
land both unchanged. The problem is the stated landing condition, "P1 and P2 must land together (+ P7)". That
set does not build, and the worker's own "full set" includes a patch that breaks law (i). I have verified an
amended atomic set end to end. Its patches are byte-identical copies in `patches/` (SHA256SUMS.txt; same hashes
as the worker's).

**Amended atomic landing set (one commit):** P1 + P2 + (P3 **or** 01RS) + P5 + P7. P4 is optional. **Exclude P6.**

| Patch | Verdict | Why |
|---|---|---|
| P1 hud_draw.h | OK (shared header) | Content verified; must land inside the set |
| P2 hud_draw.c | OK | hud_draw is byte-identical except the NODUP->ANY selection of `_check_stack_buffer` |
| P3 / 01RS hud_unit.c | **REQUIRED by P1** | With P1, HEAD hud_unit.c fails to compile: C2365/C2086 redefinition of `STACK_BUFFER_LENGTH`/`STACK_BUFFER_FILL` (hud_unit.c:118-119) |
| P5 hud_weapon.c | **REQUIRED by P1** | With P1, the bytes of the unparked residual `_render_weapon_hud` drift |
| P7 models dead include | **REQUIRED by P1**; owner disclosure | With P1, `__rasterizer_model_transparent_geometry_submit` goes EXACT -> residual (12/2 -> 11/3) |
| P4 hud_nav_points.c | optional | Byte-inert without it; its COMDAT has an exact caller |
| P6 motion_sensor.c | **exclude** | Not needed for bytes (0 diffs with P1 alone). All 3 callers of the COMDAT it newly emits are residual AND parked: law (i) / P1 class |

## What I re-ran (own tools in this slug: sweep.py, rawcoff.py, warn.py, plink.py, secorder.py)

- **Patches.** `git apply --check` is clean on 16542e46:
  - each patch alone;
  - P1+P2;
  - {P1,P2,P3,P5,P7}, {P1,P2,01RS,P5,P7} and {P1,P2,01RS,P4,P5,P7}.
  
  `patch` replay onto `git show HEAD:` copies reproduces the worker's cand hud_draw.h/.c exactly.
- **Consumers.** My own grep (`-rlE`, .c/.h/.cpp/.inl, safe for paths with spaces) gives the same 23 TUs:
  - No header includes hud_draw.h.
  - There are no .c-in-.c includes.
  - No alternate include spellings exist.
  - `source/saved films`/`saved games` have no consumer.
- **Harness control.** Each TU copy has its include pointed at a verbatim HEAD header. The result is 23/23 raw-identical
  to production. My comparer checks bytes, relocations by target, COMDAT selection and checksum, the symbol table and
  the section order.
- **Candidate** (P1, P2, 01RS, P4, P5, P6, P7):
  - The 18 non-sentinel consumers show 0 differences (models needs P7).
  - hud_draw differs ONLY in `_check_stack_buffer` selection, 1 -> 2 (checksum 0x4957c551 unchanged), plus the order
    note below.
  - hud_draw gate: 22 EXACT / 1 UNWRITTEN (`_fast_ftol_C`), the same rows as production.
- **Minimal set** (P1, P2, P3, P5, P7):
  - All gate summaries equal production (hud_unit is 22/22 against the PRODUCTION split).
  - Only hud_unit and hud_weapon gain the ANY COMDAT.
  - 20 objects are raw-identical.
- **hud_draw admission tools.**
  - object_audit text is IDENTICAL for production and candidate (pre-existing FAIL(2): `_fast_ftol_C` is unwritten).
  - Data (tinfo --data) is identical.
  - /W3 /Zs warning multisets are identical (hud_draw, hud_unit, nav, weapon, motion, models).
  - fake_match_scan: the same 2 pre-existing leads (`get_return_eip` naked/asm).
- **Surplus identity.** `_check_stack_buffer` in all 5 candidate objects is section_infos_equal to January hud_draw's
  (48 B, 0 relocs, c7049c31...).
- **Pair links.**
  - The candidate hud_draw with each emitter passes in both orders, and so do all five together.
  - Control: production hud_draw (NODUP) with any emitter gives LNK2005 in both orders, so the probe discriminates.
  - The minimal set also passes.

## Rules and evidence

- **Inline construct (strip test).** For hud_draw alone the plain spelling (production) is equally exact, so P1/P2
  are byte-inert there. The `__inline` is needed for the other TUs, and it is attested by first-party evidence:
  - the inlined-function return merge at all 26 January sites;
  - PUBLIC `_check_stack_buffer` in cachebeta (C08F0);
  - zero out-of-line references;
  - the plain goto-free loops (review3 S1/S2) are not exact.

  The helper is not in the all-inlined class for these TUs: hud_draw has exact callers, and the ruling-5
  conditions hold for the emitters in the amended set.
- **Macro.** `match_assert_stack_frame` is attested as a macro, because both asserts at every site carry one line
  literal. Its name is descriptive, in the `match_` explicit-location family; Bungie's name is lost (the /Od build
  has no "corrupt stack" string, and HCEX has no sentinel names). Disclose it.
- **Header location.** hud_draw.h is inferred as the closest genuine associated header (house rule 15). It is
  not first-party attested. The evidence for it: January's selected owner is hud_draw.obj, `get_return_eip`'s
  prototype is already there, all 5 users include it, and the Sept-2001 map marks `_get_return_eip` as `i hud_draw.obj`.
  The Sept-2001 map does not list `check_stack_buffer` at all (a 0x30 gap), so that part is inference.
- **Law (g) scan.** No views, consumer prototypes, float, invented aggregates or pragmas. The hand copies are
  removed (not added). The enum names already existed in production. The block scope in the macro is not new in
  hud_draw, and the /Od build has no sentinel, so no declaration-order evidence exists either way.
- **Storage.** `check_stack_buffer` is non-static `__inline` and PUBLIC in cachebeta. No symbols.json change for hud_draw.

## Disclosures / advisories for the integrator and owner

1. **P7 is count shielding.** models uses no hud_draw.h name; I grepped 25 names, and only the struct tag
   `bitmap_data` appears. January's models references only `real_argb/rgb_color_to_pixel32`, which come from
   bitmap_color_conversion.h. The include came from project commit 635bd83d, and removing it alone is raw- and
   warning-inert. But P7 is load-bearing only because P1 adds about 9 declared-name units. This lane HELD the Layer-2
   prototype move and main header_swap as count compensation; the 100K lane LANDED the removal of verbatim
   duplicates. State P7's role explicitly in the ruling request.
2. **COMDAT selection.** hud_draw's January-exact `_check_stack_buffer` goes from NODUP to ANY. csplit writes
   NODUP for every section, and objdiff, gate and object_audit ignore selection. But `tools/regression_gate.py`
   fingerprints `comdat_selection`, and its only adjudication recipe is xdk-stock-d3dinline. If canonical runs that
   gate with a baseline that covers hud_draw, it will report "accepted function evidence changed". The Layer 1
   provider repairs already made the same change without it; the lane's chain does not run it.
3. **Emission order (unscored).** The candidate emits `_check_stack_buffer` before `_get_return_eip`; January's image
   has the reverse. Production's order already diverges from January's throughout (order_*.txt): for example,
   January's 3rd section is `get_sprite_clip_rect`, which is our 15th. The landed ARGB header inlines are 9th-10th in
   January and 3rd-4th in ours. The swap is not evidence against P1. A later packet could move `get_return_eip`
   into hud_draw.h as well (the Sept-2001 `i` flag), but that would newly emit `_get_return_eip` in motion_sensor from
   parked callers (law (i)). Not proposed.
4. hud_draw itself stays NOT admissible: `_fast_ftol_C` is unwritten, and the `_fast_ftol` candidate-only-comdat-owner
   rejection stands. This packet is zero-credit for hud_draw.
5. **Composition.** After applying, re-sweep all 23 hud_draw.h consumers and the parks on the live tree, together
   with any other wave-4 header packets. Isolated measurements do not compose.

Probe log: `LEDGER.md`. Outputs: `out_{prod,ctrl,cand,hdronly,min}/`, `audit_hud_draw_*.txt`, `warn_*`, `order_*.txt`.
