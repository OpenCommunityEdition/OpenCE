# source/ai/ai_debug

## decision
RETIRE

## est_probability
0.03

## route
None for closure. _ai_debug_render_actor (24,976 vs 25,008 bytes) has frame, slots and the 1,905-entry relocation multiset equal (structural batch b5, 0923). real_regions shows 97 REAL regions out of 1,165; the rest are addressing-only. The samples are x87 staging, e.g. Jan `fmul 0.1f; fst [ebp-0x1c]; fadd [ebp-0x10]` vs ours `fstp; fld; fadd` at the hand-written mid_point/p1/p3 offsets. January CSEs the product across + and -, so the arithmetic is authentic, not two point_from_line3d calls. There is no admissible lever.

## why_new
Nothing new for code. Ready admission prerequisites for when render_actor closes: (1) 19 `_code_` placeholders have Halo CE atlas names, verified against 25 already-named anchors at off-0x400000 (e.g. code_00039990 -> _ai_debug_render_path_node, code_0003a910 -> _ai_debug_render_path_nodes, code_000386a0 -> _ai_debug_speech_update, code_000411d0 -> _ai_debug_render_all_actors). Two are also MAP-attested (astra research18). The batch was measured byte-inert in 0914/0915 and has been pending since. code_00038280 has no atlas entry. (2) 33 symbols are storage 2 in the split, 3 in ours, and not cachebeta publics, so ours is right and symbols.json needs "static": true. (3) _ai_debug_render_actor_jmptable is a label inside the parent section (ledger 0923). (4) .data 56 is coff-identical; the <100% score is the scorer defect.

## prior_negatives_checked
claude_structural_HANDOFF_20260923 (b5: remaining ~43 width-normalised bytes are x87 staging + one tail merge, no admissible lever); ai_debug_obj_opus5_next150_n2_20260915 (atlas rename proposal, A23 law); ai_debug_obj_opus5_150k_w2 (20-name batch inert); astra wave2/research12 packets; claude_lane_a_waveC (four defects fixed); ai_debug_jmptable_owner_20260923.

## notes
The render_actor residual is x87 register-stack staging spread over a 25 KB function. Reopen only with a decoded x87 spill/staging law. The naming and storage packet can ride with any future closure.