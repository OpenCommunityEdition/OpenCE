# source/rasterizer/xbox/rasterizer_xbox_profile (hygiene)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/profile/PACKET_rasterizer_xbox_profile_full.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/profile/PACKET_rasterizer_xbox_profile_V1_byte_inert_subset.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/profile/07R_config_profile_rejection_update.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/profile/07R_V1_config_profile_rejection_update.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/profile/individual/"
]

## production_changes
Choose one option. FULL = PACKET_rasterizer_xbox_profile_full.patch (01+02+03+04+05+06+08 plus 07R across 10 files): the owner header rasterizer_xbox_internal.h declares global_window_parameters; the rasterizer.c local window view is replaced by the complete type; the consumer-local externs are removed; the profile_check parameters follow the /Od order; detail_objects uses the rasterizer_xbox.h API; the BUG marker comment is added; the rejection is rewritten. V1 = PACKET_..._V1_byte_inert_subset.patch (04+06+08 plus 07R_V1), which is byte-inert everywhere and keeps profile's consumer-local window extern, as noted in 07R_V1. 07R was regenerated because the rejections file was reordered, and it validates with object_admission_policy. The unit stays NonMatching, with no data entry. B1, B2 and B3 are excluded.

## evidence
01-06 and 08 apply clean at 05255584 and the round trip PASSes. The FULL sweep is 611/612: the only change is rasterizer_xbox_models __rasterizer_model_draw (5168), residual in production and still residual, one instruction further (1334 to 1333 equal instructions; objdiff fuzzy 96.9141 to 96.9107, matched code unchanged). Bisect: 01 alone causes it. LAB: production models.c with its own extern moved above the globals is CANON-IDENTICAL, so this is a pure declaration-position context tie. It was introduced after round 1 by P7 of 466698b8. The V1 sweep is 612/612 identical. fngate: profile 16/16, detail_objects 20/20, rasterizer 131/131 and models 12/2, all unchanged. profile: audit PASS (67/0), pdb 0, 0 code COMDATs, link PASS. /W3 is identical in all 10 affected TUs and fake_scan finds 0 leads.

## blockers
B1: invented TU-private aggregates and the unattested .bss +0x45c object. B2: the NONE-branch varargs January bug. B3: three volatile qualifiers. All three need owner rulings. detail_objects (already Matching) keeps a pre-existing audit FAIL(14) from D3D wrapper storage rows; that is outside this packet.

## reopen_criteria
FULL vs V1: the integrator or owner decides whether the models residual one-instruction shift is acceptable. For admission, the owner rules on the review3 questions (a)-(c).

## manifest
C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/profile/MANIFEST.md

ledger: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/LEDGER.md (index MANIFEST: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/MANIFEST.md; checksums patches_sha256.txt)
