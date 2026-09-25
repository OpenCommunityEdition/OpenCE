# source/ai/actor_perception

## decision
OWNER

## est_probability
0.5

## route
Round-1 Packet B (production.patch / final_full.c, blockage body only) closes the object at 44/44. Packets A and C have already landed at f6d00a8c. The link and distance3d provider blockers are cleared by Layer 2, pdb has 0 disagreements and data has no gap. The owner must rule on structural owner packet 20260923 section 5: a load-bearing /Od-attested aggregate 3D->2D view copy `source_planar_direction = *(real_vector2d const *)source_vector;`. The field copy gives 384!=400. The review also discloses two further /Od-only constructs at 0x4710a0: an empty then-arm at hes<0.36 and an explicit `else blockage = 0;`. On a yes, apply the blockage hunk and retire the park. Under 3c74fa36 the new COMDATs (_normalize2d, _scale_vector2d, _dot_product2d, _negate_vector3d) no longer need a separate ruling: all are byte-identical and provider_link passes.

## why_new
Nothing is new at worker level. The new fact is that the blocker set has shrunk to s5 alone. The distance3d/distance_squared2d provider conflict is gone (board: SELECTED-PROVIDER LINK PASS), and the COMDAT-emission question is covered by the 2026-09-23 house-rule exception.

## prior_negatives_checked
Read the round-1 fn_actor_perception and REVIEW results (field copies F1-F3 give 384; plain !(hes<0.36) gives [sha]; dropping the else zero store gives 384; M8 bo1-7 inert), the park evidence (2026-09-20 short-carrier decode), structural owner packet s5 and the 0925 main ledger. Measured without editing: the unmodified round-1 final_full.c gates `== exact 44 residual 0` at f6d00a8c and differs from production only in the blockage function. Production's blockage body currently hand-expands point_from_line3d, negate and dot (rule 6/13 debt).

## notes
On a yes, 1 object completes (+400 B). No castless January spelling exists in any ledger. The /Od aggregate integer copy (one base load, both dwords loaded before both stores) is the attestation. Workers should not spend more shapes here.