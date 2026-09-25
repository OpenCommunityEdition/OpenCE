# MANIFEST - review_critic3_rasterizer_lights (adversarial review of critic_3 patch 17)

- Base/tip: 5d3ca708 (clean; only the pre-existing untracked research/fifty_objects_r2_20260924/). No tracked edit, no
  ninja/configure/git mutation. Claim review_critic3_rasterizer_lights (released at end).
- Packet: scratch/w/critic_3/patches/17_rasterizer_lights.patch (unchanged; no amendment).
  config/symbols.json:6206 (file_offset 1509488 = 0x170870, static) `_lens_flare_submit_parameter_get` -> `_lens_flare_parameters_get`;
  source/rasterizer/rasterizer_lights.c: listing comment, prototype, definition, 5 call sites.

## Source evidence (independent readers: body_ident.py + inline scans; LEDGER R1)
- Sept-2001 cachebeta.map (3bb0dabe) statics: _screenshot_in_progress 0x16e440, _lens_flare_parameters_get 0x16e470 (0x40),
  _lens_flare_occlusion_test_results_get 0x16e4b0 (0xf0), reset 0x16e5a0 = January's sequence and sizes at 0x170840..0x1709a0.
  Aug-2001 map: same name, 0x40.
- Bytes: January exe 0x170870 == Sept xbe 0x17f470 except 6 relocated operands; assert line immediate 0x43 (67) in both.
- Callers: 2 E8 sites in each build, same owners at the same offsets (submit +0x14c, submit_occlusion_tests +0x79).
- January strings: none names either identifier; `lens_flare_parameters` is assert text (supporting vocabulary only).
- Atlas: Aug exact / Sept map / 2002-01-14 xbe exact all `_lens_flare_parameters_get`. HCEX: neither name. cachebeta publics:
  neither -> static (kept).

## Target vs candidate (emulated split = csplit with patched symbols.json; control split == build/split 833/833)
| function | size | gate prod (build/split) | gate cand (emulated split) |
|---|---|---|---|
| _lens_flare_parameters_get (was _lens_flare_submit_parameter_get) | 64 | EXACT | EXACT |
| _rasterizer_lens_flare_submit | 656 | EXACT | EXACT |
| _rasterizer_lens_flares_submit_occlusion_tests | 400 | EXACT | EXACT |
| other 7 exact rows | - | EXACT | EXACT |
| _rasterizer_lens_flare_submit_for_cluster / _rasterizer_lens_flares_draw / _rasterizer_lights_reset_for_new_map | 400/2240/48 | residual [sha] | residual [sha] |
Totals 10/3 both. objcmp cand.obj vs prod.obj and vs build/base: IDENTICAL (name mapped). Split ctl vs cand: only
rasterizer_lights.obj differs; IDENTICAL name-mapped.

## Section-4 battery (identical prod vs cand modulo the rename)
object_audit FAIL(3) (pre-existing residuals; 41/41 symbols ok) | pdb_storage 0 | surplus_identity 11/0 | provider_link PASS
both orders | data (tinfo --data) identical | objdiff 3.3.1: 1996/5244 code, 9/13, 335089/335089 data, 99.06512 all sides.

## House rules
Identifier-only change to an authentic first-party name; no construct to strip-test; no new scope. Old name remains only in
historical docs prose and round-1 research patches (not to be edited).

## Credit
Zero (byte-inert). Admission effect: first-party name for a January static. No object completes (3 residuals remain).

## Integrator notes
- Land symbols.json + .c in one commit, then csplit regen (negative control: .c alone -> 7/5/1 vs the old split).
- Round-1 P2 draw packet (research/fifty_objects_20260925/w/rasterizer_lights/P2_draw_*.patch, not landed) touches the same
  line 1145 with the old name; rebase it if it is ever landed after 17.
