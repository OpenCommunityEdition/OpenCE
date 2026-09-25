# Review r2w2 rasterizer_xbox_profile ledger

Base: HEAD 434f0151 (== 05255584 + ledger doc). Claim review-r2w2-profile-adv on profile.

## R1 apply + sweep (own harness: sweep.py/canon.py, shadow trees t_ctl/t_full/t_v1 = copies of source/ + rejections json; applyp.sh = CRLF-safe GNU patch)
- git apply --check at HEAD: FULL rc0, V1 rc0.
- 10 affected TUs (8 internal.h includers + profile + detail_objects), canonical compare vs build/base
  (all non-debug sections bytes+relocs by name, symbol table minus .file/@comp.id):
  - t_ctl 10/10 IDENT (harness valid).
  - t_full 9/10 IDENT; models DIFF only section .text __rasterizer_model_draw.
  - t_v1 10/10 IDENT.
- __rasterizer_model_draw: split 5168/350 99d8b95f; base 5168/348 89b0d7ea; t_full 5168/348 cbfa8585; t_v1 89b0d7ea.

## R2 FINDING: FULL breaks the park gate (undisclosed)
- config/parked.json:1166 parks source/rasterizer/xbox/rasterizer_xbox_models:__rasterizer_model_draw with measurements
  base normalized_sha256 89b0d7ea... and objdiff_percent 95.08453 (== current build/report.json).
- tools/parked_functions.validate_parked_functions marks an entry STALE when base sha/percent differ; tools/project_x86.py:717
  require_valid_parked_functions -> sys.exit("Parked-function verification failed") in the ninja progress step.
- FULL changes that body to cbfa8585 -> stale park -> ninja progress FAILS. The FULL packet has no parked.json hunk and
  the MANIFEST never mentions the park. FULL is not landable as submitted.

## R3 FINDING: 07R / 07R_V1 stretch the collision_debug SDK-table precedent
- 07R(_V1) states the three stock SDK tables "are now accounted for as authentic topology" and drops them from reopen.
- Canonical reconciliation docs/object_matching_logs/claude_fifty_objects_canonical_reconciliation.md (641e1466):
  "The stock SDK-table coalescing criterion conflicts with canonical's existing ruling ... No whole-object credit is granted
  pending a uniform policy/owner audit." Lane doc claude_fifty_objects_20260925.md:518-541 records the same open conflict.
  Brief s8: dynavobgeom hold reasons (SDK-table coalescing vs canonical ruling) STILL APPLY.
- The 20260906 profile evidence (#remaining-whole-object-boundaries) also records that linking against January's split
  selected owners gives LNK2005 in both orders; provider_link.py tests our base providers only.
- So collision_debug's standard is the lane's, not a canonical ruling that settles the question: the rejection must keep the
  SDK tables as an owner-policy item. Amend.

## R4 V1 checks (t_v1)
- gate profile 16/16 EXACT; detail_objects 20/20 EXACT.
- object_audit profile PASS (67 symbols, 0 differ; .data 132/132 ok, .bss 1122/1122 ok); surplus = .drectve, 4 literals,
  3 SDK tables, 3 __real (== production). detail_objects audit FAIL(14) byte-identical to production audit (pre-existing).
- pdb_storage 0; surplus_identity 0 code COMDATs; provider_link 10/10 PASS both orders.
- SDK tables vs January split definers (headers/progress_bar/decals): section_infos_equal + flags equal, 3/3.
- January split UNDEFs include _D3DDevice_InsertCallback@12, _QueryPerformance{Counter,Frequency}@4, _global_window_parameters.
- /W3 (CL /Zs /W3, line-normalised): profile 12 / detail_objects 12 warnings, identical ctl/v1/full/v1r.
- fake_match_scan: 0 leads ctl/v1/full/v1r.
- 04 /Od attestation re-read: odbuild fn 0x8004c0 ("rasterizer\dx9\rasterizer_dx9_profile.c", later revision):
  [ebp+8]=condition byte, [ebp+0xc]=profile short, [ebp+0x10]=message; NONE branch pushes message then profile (BUG
  persists). January private ABI (esi=message, di=profile, stack=condition) is param-order-inert: measured IDENT.
- 06: rasterizer_xbox.h declares rasterizer_error / profile_begin/end / set_pixel_shader / set_texture (returns
  union point2d *, the removed local prototype wrongly said void) / set_vertex_shader_permutation / global_d3d_device /
  pixel_shader; _rasterizer_profile_detail_objects is rasterizer.h:70 (== 21, names[21] "detail objects").
- admission audit (tools.audit_object_admission, --admission-rejections for prod/full/v1): summary identical 9/0/2/0; the
  edited entry loads through load_rejection_index.

## R5 FULL model_draw lab (independent)
- ctl models.c with its own extern moved (A) after the internal.h include or (B) after the models.h include:
  both CANON-IDENTICAL to the t_full models object -> pure declaration-position effect (confirms worker).
- worker mini objdiff report: park function percent 95.08453 -> 95.07629 (FULL would need that re-baseline).

## R6 amended V1R (reviewer)
- V1R = V1 (04+06+08) + the byte-inert 03 listing-comment hunk (header "symbols in this file" block synced to the
  existing symbols.json static rows: 0x30CF00 frequency/globals, 0x465E28 eight statics; offsets checked against
  symbols.json file_offsets) + 07R_V1R (SDK-table sentence rewritten to keep the canonical owner-policy question open;
  reopen adds the park re-baseline for the future window move and the SDK decision).
- git apply --check rc0 at HEAD; round trip (applyp onto pristine copy) byte-equal to the tested tree.
- sweep 10/10 IDENT; profile gate 16/16; audit PASS; provider_link PASS; /W3 same; fake scan 0; rejection_index OK.
- sha256 in patches_sha256.txt. Shadow trees deleted after measurement (reproduce: copy source/ + config json, applyp.sh).
