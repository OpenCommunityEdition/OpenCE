# MANIFEST - source/rasterizer/xbox/rasterizer_xbox_profile: window/API ownership packet (zero credit; B1/B2/B3 excluded)

Base: 05255584 (clean). Verdict: **ADMISSION_FIX_PROPOSED** (zero credit). The unit stays NonMatching.

Claims:
- The unit: `r2w2-hygiene-rasterizer_xbox_profile`.
- Consumers, same label scheme: rasterizer, rasterizer_transparent_geometry, rasterizer_xbox_active_camouflage, rasterizer_xbox_environment, rasterizer_xbox_models, rasterizer_xbox_transparent_geometry and rasterizer_xbox_detail_objects.

## Two landing options

### FULL: `PACKET_rasterizer_xbox_profile_full.patch`
Contents: 01+02+03+04+05+06+08 plus 07R, over 10 files. `git apply --check` rc 0; round trip PASS.

Strict result:
- 0 exact rows and 0 exact sections change.
- Across 612 TUs, 611 are identical.

Disclosed side effect:
- `rasterizer_xbox_models` changes in exactly one section: `__rasterizer_model_draw` (5168). That function is residual in production (relocs 348 vs 350) and stays residual.
- It moves one instruction further from January: 1334 → 1333 difflib-equal instructions.
- objdiff 3.3.1 fuzzy for models goes from 96.9141 to 96.9107. Matched code is unchanged at 4111/12379.

Cause (bisected and lab-proven):
- 01 alone reproduces the change; 05 has no effect on it.
- LAB `lab_models_pos/`: production models.c with its own extern moved to just after its includes is CANON-IDENTICAL to the packet object.
- So this is purely the declaration position of the name `global_window_parameters`, a context tie.
- Round 1 measured 10/10 identical on 954eebd2. After round 1, P7 of 466698b8 dropped models' `hud_draw.h` include and changed models' declaration context.

### V1 (byte-inert everywhere): `PACKET_rasterizer_xbox_profile_V1_byte_inert_subset.patch`
- Contents: 04+06+08 plus `07R_V1` (07R with the window-extern note added).
- Sweep: 612/612 identical.
- The consumer-local `extern ... global_window_parameters` stays in profile.c. 07R_V1's reason and reopen text record that.

## Individual patches (`individual/`)
- 01-06 and 08 are the round-1/review3 files, unchanged. Each applies at 05255584, alone or in sequence.
- 07R is regenerated because the rejections file was reordered and the profile entry is now first. The same 5 lines are replaced. It validates with `tools/object_admission_policy.rejection_index`, and the evidence path `research/fifty_objects_20260925/w/review3_rasterizer_xbox_profile/REVIEW.md` is tracked.
- Order:
  1. 01+02 (inseparable)
  2. 03 (needs 01)
  3. 04
  4. 05 (needs 01)
  5. 06
  6. 08 (needs 04's context)
  7. 07R

## Evidence per construct (from review3; re-checked here where marked)
- **01 / 03 / 05.** `global_window_parameters` is declared in rasterizer_xbox_internal.h, the backend header of the January COMMON owner rasterizer_xbox.c. The consumer-local externs are removed. Re-checked: the other TUs that keep partial or local views of the variable (debug, lights, text, decals, shadows, widgets, ...) do not include internal.h, and all 612 TUs still compile.
- **02.** The rasterizer.c local view `struct rasterizer_window_parameters` (with `reserved04[4]`) is replaced by the complete type (`camera.position` / `camera.forward`). rasterizer is 131/131 before and after.
- **04.** `rasterizer_profile_check(condition, profile, message)` follows the /Od parameter order at 0x8004c0 (review3).
- **06.** detail_objects gets the Profile/rasterizer API from rasterizer_xbox.h instead of consumer-local prototypes, and 21 becomes `_rasterizer_profile_detail_objects`. detail_objects is 20/20 before and after.
- **08.** Comment only. It adds the BUG methodology marker to the existing, already-landed NONE-branch varargs text. This does not admit B2; bytes are identical.

## Target vs candidate (production split; no symbols.json change)
- profile: 16/16 before and after (`fntable_profile.md`).
- detail_objects: 20/20.
- rasterizer: 131/131.
- models: 12/2 before and after (`fntable_models.md`).

## Audits
- **profile.** object_audit PASS before and after (67 symbols, 0 differ). pdb_storage 0. Surplus is identical to production (.drectve, 4 strings, 3 SDK tables, 3 `__real`). 0 candidate-only code COMDATs. provider_link PASS.
- **detail_objects.** object_audit FAIL(14) and 15 pdb disagreements, both PRE-EXISTING and unchanged: the storage rows of the D3D wrappers plus `_detail_object_build_vertices`. This is a separate storage packet, not part of this one.
- **objdiff 3.3.1 mini report.** profile, detail_objects and the others are identical; models fuzzy changes as disclosed above.

## Tests run
- Full sweeps: FULL gives 611/612 and V1 gives 612/612.
- `CL /Zs /W3`: identical warning multisets in all 10 TUs that include the edited files or are edited.
- fake_match_scan: 0 leads in both.
- No compile failures.

## Held (excluded; unchanged owner questions from review3)
- B1: de-aggregating the 4 invented TU-private structs, plus the unattested +0x45c object.
- B2: admitting the NONE-branch varargs January bug.
- B3: the three load-bearing volatile qualifiers.

## Credit
Zero for both options.
