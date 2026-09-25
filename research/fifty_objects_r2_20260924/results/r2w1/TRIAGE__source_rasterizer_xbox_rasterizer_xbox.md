# source/rasterizer/xbox/rasterizer_xbox

## decision
OWNER

## est_probability
0.35

## route
Owner-queue item 2 (research/fifty_objects_20260925/w/owner_queue/names_xbox_hs_scen, patches 01-05, apply-ready). The only blocker is the 9 non-first-party .bss static names. New partial evidence: HCEX.pdb's target enum (DIA2Dump -t, values 0..9: render_d3d, render_primary, render_secondary, shadow_primary, shadow_secondary, motion_sensor_primary, sun_glow_primary, sun_glow_secondary, water, z) ends with `_rasterizer_target_z`. January's 8th target (after water) is the render-primary descriptor whose Data is global_d3d_surface_render_primary_z->Data, i.e. the Z-buffer target. So names #8/#9 (`*_render_primary_copy`, currently INVENTED) can become HCEX-enum convention stems such as global_d3d_{texture,surface}_z, and the tree enum `_rasterizer_target_water_bumpmap` can take HCEX's `_rasterizer_target_water`. This offers the owner a better-evidenced name set. It does not remove the ruling: node_matrix_constants stays invented, and #2-#7 stay convention stems.

## why_new
Round-1 RULING.md checked HCEX for `_rasterizer_target_render_primary` but did not use the full enum. The `_z` target identity for the 8th slot is new. No other first-party names turned up: the January-hash atlas and the Sept-2001 map-tier atlas files list only rasterizer_xbox.obj functions, and the PC DX9 /Od build uses a global_render_targets[] array with no node_matrix, secondary_z, bitmap_dimensions or _copy strings.

## prior_negatives_checked
claude_fifty_objects_20260925_owner_queue.md item 2, names_xbox_hs_scen RULING.md/MEASURED.md, review2_rasterizer_xbox REVIEW.md (approve=false only on owner policy), owner-rulings-20260915 (no descriptive static names), 2026-09-24 canonical path_obstacle_avoidance hold.

## notes
95/95 exact. The packet gives audit PASS, provider_link 15/15, storage 216/0. The 05_parked.json re-baseline for models __rasterizer_model_draw still applies. The independent rasterizer_xbox_debug static-wrapper patch is not name-gated. Evidence: LEDGER.md P1, P14.