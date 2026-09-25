# source/ai/actor_looking

## decision
RETIRE

## est_probability
0.03

## route
None for closure. Only _actor_look_update is left: 4704 vs 4720 bytes, same 0x58 frame, 159/159 relocations with an identical multiset. The difference is register/slot colouring: January puts primary_priority in edi and pins zero in ebx, and secondary_aim_valid spills to [ebp-7]; ours has them in ebx, edi and bl. Even if update closes, admission still needs the RESERVED Codex actor_combat _cross_product2d provider packet, because provider link currently fails on _cross_product2d.

## why_new
Nothing new. I re-checked round 1's reopen criterion (2), a missed header-helper spelling at another update site, against the full /Od call map of update (0x45f5d0). Every call matches our spelling: cosine 0x455220 x5 (already landed), dot_product3d x3, dot_product2d x2, normalize2d 0x42e020 x4, and fabs x1 at the realcmp site 0x460ffe. The fabs chain has two levels (0x42dec0 -> 0x42e400 -> CRT). That is the C++ fabs(float) -> fabsf chain, not a Halo helper. Cosine's chain has three levels (cosine -> cos(float) -> cosf). So realcmp's direct fabs is already the /Od spelling and this was a negative result.

## prior_negatives_checked
actor_looking_obj_evidence_closeout_20260919 (census of 139 candidates, 12 history blobs); astra_90pct_research10 + independent review (saved-major-results shape was byte-inert); round-1 wave1 fn_actor_looking_*_actor_look_update (stop rule after 9 /Od shapes: cosine() sites, U1/U2/U4/U5, Va/Vb view copies, Vc dot argument order, name-count K=1..24); round-1 REVIEW of find_prop (the cosine lever closed find_prop, not update). My own check: /Od callee census (scratch/w/triage_ai_b/od_look_update.txt).

## notes
Reopen only for round 1's criterion (1): a genuine source construct that gives the facing/aiming CSE pointers long-lived webs, verified with the lane A allocator replay. Criterion (2) is now exhausted. Criterion (3) is an owner decision on the /Od view copies once update is otherwise exact. Evidence: scratch/w/triage_ai_b/LEDGER.md.