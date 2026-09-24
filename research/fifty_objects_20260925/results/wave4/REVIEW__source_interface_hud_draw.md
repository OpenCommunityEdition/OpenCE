# REVIEW source/interface/hud_draw

approve: False

## per_object
[
 {
  "unit": "source/interface/hud_draw (P2 hud_draw.c)",
  "approve": true,
  "issues": "The content is correct.\n- Gate rows are unchanged: 22 EXACT / 1 UNWRITTEN.\n- The only raw difference is `_check_stack_buffer` changing from NODUP to ANY (checksum unchanged), plus an unscored order swap.\n- The object_audit text is identical.\nLand it only inside the amended atomic set. The object is still not admissible because `_fast_ftol_C` is unwritten."
 },
 {
  "unit": "source/interface/hud_draw.h (P1 shared header)",
  "approve": true,
  "issues": "The content is correct: a non-static __inline that matches PUBLIC cachebeta, a macro attested by the shared line literals, and an enum moved out of the TUs.\n- It cannot land with P2 alone. HEAD hud_unit.c then fails to compile (C2365/C2086), models loses an exact function unless P7 lands, and hud_weapon drifts unless P5 lands.\n- There are 23 consumers, and I verified the list independently."
 },
 {
  "unit": "source/interface/hud_unit (P3 or 01RS)",
  "approve": true,
  "issues": "Required by P1 for compilation.\n- P3 alone gives 22/22 against the production split, with one new ANY COMDAT from exact callers.\n- 01RS is covered by the hud_unit review and also needs patches 02/03/04R."
 },
 {
  "unit": "source/interface/hud_weapon (P5)",
  "approve": true,
  "issues": "Required by P1: it keeps `_render_weapon_hud` byte-stable.\nThe new ANY COMDAT has exact callers (`render_grenade_hud`, `hud_update_weapon`), is identical to January's copy, and pair-links in both orders."
 },
 {
  "unit": "source/rasterizer/xbox/rasterizer_xbox_models (P7)",
  "approve": true,
  "issues": "Required by P1; disclose to the owner.\n- The include is dead and removing it alone is byte-inert.\n- It is load-bearing only as declared-name-count shielding against P1's inline. That is the class this lane has held for Layer 2 and main header_swap, while the 100K lane landed duplicate removal."
 },
 {
  "unit": "source/interface/hud_nav_points (P4)",
  "approve": true,
  "issues": "Optional: byte-inert without it. Its COMDAT has the exact caller `hud_update_nav_point_local_player`."
 },
 {
  "unit": "source/interface/motion_sensor (P6)",
  "approve": false,
  "issues": "Exclude. It is not needed for bytes (0 differences with P1 alone). It newly emits `_check_stack_buffer` in motion_sensor.obj, and all three callers are residual and parked, which breaks law (i) (P1 class)."
 }
]

## checks
All checks used my own tools in scratch/w/review4_hud_draw/ (sweep.py, rawcoff.py, warn.py, plink.py, secorder.py). The tree was 16542e46 and stayed clean throughout.

(1) Patch application:
- `git apply --check` is clean for each patch alone.
- It is also clean for P1+P2 and for the sets {P1,P2,P3,P5,P7}, {P1,P2,01RS,P5,P7} and {P1,P2,01RS,P4,P5,P7}.
- Replaying the patches with `patch` onto `git show HEAD:` copies reproduces the worker's candidate hud_draw.h and hud_draw.c exactly.

(2) Consumer list:
- My own grep (`-rlE`, .c/.h/.cpp/.inl, safe for paths with spaces) finds the same 23 TUs.
- No header includes hud_draw.h, there are no .c-in-.c includes, and there are no alternate include spellings.

(3) Harness control: each TU copy points its include at a verbatim HEAD header. All 23 objects are raw-identical to production, including bytes, relocations by target, COMDAT selection and checksum, symbol table and section order.

(4) Candidate sweep (P1 P2 01RS P4 P5 P6 P7):
- gate.py --source --all rows are identical to production for all 23 consumers. The one exception is hud_unit, which shows 20 exact + 2 unwritten because the 01RS renames need symbols.json patch 02.
- hud_draw is 22 EXACT / 1 UNWRITTEN (_fast_ftol_C), the same as production.
- The 18 non-sentinel consumers show 0 differences, provided P7 is applied.
- hud_draw differs only in `_check_stack_buffer` selection, NODUP to ANY. Its checksum 0x4957c551 is unchanged, and the section order changes (below).

(5) Header-only probes (P1 against HEAD TUs):
- hud_draw fails to compile (C2365/C2086/C2084).
- hud_unit fails to compile (C2365/C2086 redefinition of STACK_BUFFER_LENGTH/FILL at hud_unit.c:118-119).
- In models, `__rasterizer_model_transparent_geometry_submit` goes from EXACT to residual (12/2 becomes 11/3).
- The bytes of `_render_weapon_hud` in hud_weapon drift; it is residual and unparked.
- nav and motion_sensor show 0 differences.

(6) Minimal set {P1,P2,P3,P5,P7}:
- gate summaries equal production for all 23, and hud_unit is 22/22 against the production split.
- Only hud_unit and hud_weapon gain the ANY COMDAT, and both have exact callers.
- The other 20 objects are raw-identical.

(7) hud_draw admission tools:
- object_audit text is identical for production and candidate. It carries the same pre-existing FAIL(2) for the unwritten `_fast_ftol_C`.
- Data sections are identical (tinfo --data), apart from `.debug$S`, which holds the gate's temp-file path.
- `/W3 /Zs` warning multisets are identical for hud_draw, hud_unit, nav, weapon, motion_sensor and models.
- fake_match_scan reports the same 2 pre-existing leads, from `get_return_eip`.

(8) Surplus identity: `_check_stack_buffer` in all five candidate objects is section_infos_equal to January hud_draw's copy (48 B, 0 relocations, sha c7049c31...).

(9) Pair links (the same Link.Exe command as provider_link.py):
- The candidate hud_draw passes with each emitter in both orders, and with all five together.
- Control: production hud_draw (NODUP) gives LNK2005 in both orders, so the check discriminates.
- The minimal set also passes.

(10) Evidence:
- `_check_stack_buffer` and `_get_return_eip` are PUBLIC in cachebeta.
- HCEX.pdb bytes contain no sentinel names, and the /Od build has no "corrupt stack" or "corrupt return" strings.
- The Sept-2001 atlas marks get_return_eip, fast_ftol_C, real_argb, real_rgb and real_alpha_intensity as 'i hud_draw.obj', and does not list check_stack_buffer.
- models uses none of the 25 hud_draw.h names. January's models references only real_argb/rgb, which come from bitmap_color_conversion.h.

(11) Section-order table: January vs production vs candidate.

(12) tools/regression_gate.py compares the comdat_selection of accepted functions, which matters for issue 3.

## issues
APPROVE=FALSE AS SUBMITTED. Land P1/P2 unchanged; only their packaging needs fixing. I verified an amended atomic landing set end to end.

1. **The landing condition is wrong.** "P1 and P2 must land together (+P7)" does not build.
   - With P1 applied, HEAD hud_unit.c fails to compile: C2365/C2086 for STACK_BUFFER_LENGTH/FILL at hud_unit.c:118-119. So P3 or 01RS must land in the same commit.
   - P5 is also required; without it the bytes of hud_weapon's unparked residual `_render_weapon_hud` drift.
   - P7 is also required; without it models loses the exact `__rasterizer_model_transparent_geometry_submit` (1,296 B).

2. **The worker's own "full atomic set" includes P6.** P6 is not needed for bytes: motion_sensor shows 0 differences with P1 alone. It also breaks law (i): it newly emits `_check_stack_buffer` in motion_sensor.obj, and all three callers are residual and parked. Exclude it. P4 is optional; it is byte-inert without it and its caller `hud_update_nav_point_local_player` is exact.
   - **Amended set:** P1 + P2 + (P3 or 01RS) + P5 + P7, with P4 optional and P6 excluded.
   - Byte-identical copies are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_hud_draw\patches\ with SHA256SUMS.txt (same hashes as the worker's).
   - Verified: git apply --check clean; all 23 gate summaries equal production; 20 objects raw-identical; hud_unit and hud_weapon gain only the new ANY COMDAT, identical to January's; pair links pass in both orders; warnings unchanged.

3. **Disclosures for the owner and integrator (not defects in P1/P2 content).**
   - (a) P7 is load-bearing only as declared-name-count shielding for P1, whose inline costs about 9 name units in models. The include is genuinely dead (635bd83d origin; removing it alone is byte- and warning-inert). But this lane held the Layer-2 prototype move and the main header_swap as count compensation, while the 100K lane landed duplicate removal. The ruling request must state P7's role.
   - (b) hud_draw's January-exact `_check_stack_buffer` changes selection from NODUP to ANY. gate, object_audit and objdiff don't see this, but tools/regression_gate.py would report "accepted function evidence changed". Its only adjudication recipe is xdk-stock-d3dinline. This is the same change class as the Layer 1 provider repairs.
   - (c) Emission order (unscored): the candidate emits `_check_stack_buffer` before `_get_return_eip`, the reverse of January's image. Production's hud_draw order already diverges from January's throughout; for example, January has get_sprite_clip_rect 3rd and we have it 15th, and the landed ARGB header inlines are 9th-10th in January and 3rd-4th in ours. So this is not evidence against P1.
   - (d) The header location (hud_draw.h) and the macro name `match_assert_stack_frame` are inferred and descriptive, not first-party. Neither the /Od build nor HCEX has the sentinel.
   - (e) hud_draw itself stays non-admissible and gets zero credit: `_fast_ftol_C` is unwritten and the `_fast_ftol` rejection stands.
   - (f) After applying, re-sweep all 23 consumers and the parks on the live tree, together with other wave-4 header packets.

Details are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_hud_draw\REVIEW.md and LEDGER.md.
