# source/rasterizer/xbox/rasterizer_xbox_profile

## decision
OWNER

## est_probability
0.2

## route
Three owner rulings from review3 (REVIEW__source_rasterizer_xbox_rasterizer_xbox_profile.md). B1: de-aggregate the four invented TU-private structs (pad02/pad06/pad12/pad16/reserved06[6]) into loose statics with descriptive .bss names (held class). This is exact except an unattested 4-byte .bss object at +0x45c. B2: admit the /Od-attested NONE-branch '### PROFILE: %s -- tell Bernie!' varargs defect (BUG class, /Od 0x8004c0; 08 BUG-marker patch ready). B3: confirm the three load-bearing volatile qualifiers (stripping them gives 12/16). The zero-credit ownership patches 01-06 + 08 + 07R rejection rewrite are ready. The 132-byte .data is the objdiff 3.3.1 '$' defect; the pinned semantic_data entry lands only with the Matching flip.

## why_new
Nothing new. HCEX has no profile TU statics (review3 V10), the /Od .bss map was already read, and none of the checked sources gives a first-party name for the +0x45c object.

## prior_negatives_checked
results/wave3 xbox_profile + REVIEW, rasterizer_xbox_profile_obj_* ledgers (bss_attestation, remaining_boundary, shared_owner_reconciliation, data_accounting_audit), config/object_admission_rejections.json entry (candidate-only-comdat-owner _D3DSIMPLERENDERSTATEENCODE; SDK tables resolved by review3).

## notes
Canonical f6d00a8c still carries the consumer-local `extern ... global_window_parameters` at profile.c:238. The reviewed zero-credit 01-06 window/API owner patches have not landed. 16/16 exact, audit PASS. Evidence: LEDGER.md P3.