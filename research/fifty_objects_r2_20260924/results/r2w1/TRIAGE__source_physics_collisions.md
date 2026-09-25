# source/physics/collisions

## decision
OWNER

## est_probability
0.35

## route
_collision_move_point (4,752 B) is strict exact in the genuine form: the real_math.h __inline valid_real_plane3d body (January inlines it at 0x485-0x4e8 and calls it out of line at case 1; /Od calls fn 0x6c7730) plus the /Od-attested function-scope `position`. That form loses _bitmap_copy (restorable by the genuine removal of the duplicate bitmap_delete prototype) and _rasterizer_frame_statistics_draw (4,176 B; no declaration count from -6 to +30 restores it). Owner question: accept the frame_statistics_draw loss (net +576 B code, +1 whole object), or hold. The rejected focused-header / block-scope packet must not be re-offered.

## why_new
The provider blocker r1 listed is gone: decals plane3d_negate and path_obstacles point_in_sphere NODUP copies are absent at f6d00a8c (grep), and link_fail is empty. So the only blocker left is the frame_statistics_draw count canary, the same function that blocks breakable B1. One ruling unblocks both.

## prior_negatives_checked
r1 fn_collisions_source_physics_collisions.md; REVIEW__source_physics_collisions.md (reject: scope steering against /Od and a one-consumer header; reopen (a) a genuine frame_statistics declaration fix, (b) an explicit owner ruling); collisions_obj_large_closeout_evidence_pass_20260919.md; Lane B try_13/try_22 (0x17b/0x17f preheader coin flip).

## notes
real_math.h is shared with the RESERVED real_math unit. Compose with or wait for the Codex packet, and run the full 447-TU sweep. Integrator side items: the semantic_matches entries for jump-table under-credit (move_point 4752/226/8b2f29..., object_get_features_in_sphere 544/13/d5292109...).