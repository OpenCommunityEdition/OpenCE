# source/rasterizer/xbox/rasterizer_xbox_lights

## decision
OWNER

## est_probability
0.3

## route
Two owner choices, from round 1. (1) Admit cand_g2_diagnostic_R15.c: the vector_from_points3d return pointer plus the 3-statement brightness accumulation, previously rejected under R15. It gives STRICT EXACT 14/14, object_audit PASS, 7 identical COMDATs, provider_link PASS and 0 pdb disagreements. (2) Waive Rule 6's caller-exact condition and land cand_g4.c (the /Od-attested vector_from_points3d/normalize3d/dot_product3d/point_from_line3d shape) as a zero-credit fuzzy re-baseline. For EITHER option, the TU must also replace its consumer-local `extern struct rasterizer_lights_window_parameters global_window_parameters;` (line 187; a view with reserved02/20/3C/A0 opaque pads) with the genuine rasterizer.h:250 `struct rasterizer_window_begin_parameters` (camera/frustum members), and re-gate. That is a section-9 admission blocker that round 1 did not name.

## why_new
No new byte route. The residual is the term/operand order of one commutative x87 dot (January i,j,k local-first; g4 i(global-first),k,j). Round-1 bisect, esearch and oracles show it is keyed by function-wide IL content that does not appear in the bytes. I checked the /Od premise for an M8 local-count route: 0x7edb90 is a 4950-byte DX9 rewrite (frame 0x1c0, extra dx9 state calls), so its scalar-local census does not transfer. What is new: the consumer-local window-parameters view is a separate admission blocker. Round 1's tusweep varied only the declaration COUNT, never the genuine type, so switching to it is required hygiene. It could in principle move the dot, but the premise is weak (the field offsets are identical).

## prior_negatives_checked
Round-1 ledger: g1/g4/g5/g6/g7/n1/n4/n5/va/r1/r2/sc/q1/q2, dead-long oracle K=0..28 at four positions, the tusweep 1..40, the esearch dead-read oracles, the bisect L=34..327, the micro lab, and the corpus scan_ijk. opus5_150k_w2 (scoping fixed the 0x98 frame; 20+ single-expression dot spellings) and w3 (V1/W1 rejected under R14/R15/R16). opus_reconciliation_20260903. The Rule-6 exception text in docs/campaign_house_rules.md. The parked.json evidence is stale: it says frame 0xA4 vs 0x98, but production is already 0x98.

## notes
The Sept-2001 map names (rasterizer_project_billboard, sun_glow_copy_source, sun_glow_convolve, sun_glow_draw) already match. The genuine-struct switch touches rasterizer_project_billboard's projection_matrix and world_to_view accesses too. Measure it once on g4 (it might move the dot) and once on g2 (it must stay exact) before any owner packet. Other local structs (pixel_shader_definition, lens_flare_definition, rasterizer_lens_flare_submit_parameters) should be checked for owner headers in the same pass.