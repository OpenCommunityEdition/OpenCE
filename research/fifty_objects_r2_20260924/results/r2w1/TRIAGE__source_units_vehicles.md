# source/units/vehicles

## decision
OWNER

## est_probability
0.2

## route
The two functions need different things. (1) _update_alien_fighter_physics_new (1,088 B) is exact with the /Od-attested cross_product2d view casts (0x8f6cbf..0x8f6ccd; byte-inert strip; prefix layout) plus removal of the TU-wide REAL_MATH_EXTERNAL_POINT_FROM_LINE3D. vehicles is one of ruling #1's 17 objects. That emits _cross_product2d, which gets LNK2005 against actor_combat's NODUP copy: a dependency on the RESERVED Codex P1 packet. Re-ask the owner under ruling #6 once Codex reports. (2) _update_alien_scout_physics (2,464 B; frame 0x94 vs ours 0xa0) is exact only with two load-bearing aggregate 3D->2D view copies (structural owner packet §5, unanswered) and a leveling point_from_line2d that contradicts /Od 0x8f8b4a and works only through the count oracle. Admissible now at zero credit: `"static": true` for _update_turret_physics (symbols.json l.7052) plus a csplit regen, and review_vehicles/fallback_hunks12.patch (define removal plus /Od-attested pathfinding point_from_line3d calls; 37/2, provider link PASS).

## why_new
New since r1: the §5 view-copy question has still not been answered (grep of docs finds no ruling), and the fighter's remaining blocker has narrowed to the Codex-reserved actor_combat provider. The other inherited link failures (_interpolate_scalar, _object_get_bounding_sphere, _triple_product3d) no longer appear in the hint packet.

## prior_negatives_checked
r1 fn_vehicles_source_units_vehicles.md and REVIEW__source_units_vehicles.md (BLOCKING 1-3; with the /Od component leveling only a 2-instruction M8 leaf swap at +0x628 remains, and any +1 inline temp closes it = oracle, not source); claude_structural_owner_packet_20260923.md §5; Lane B RB4/held fighter; vehicles_obj.md.

## notes
Object completion also needs the invented helper debt removed: vehicle_cross/dot/triple_product3d_target (vehicles.c:1925-1949) and vehicle_dot_product3d_test (l.1382), used by the exact _update_human_boat_physics and _vehicle_preprocess_node_orientations. r1 boat1/boat2 /Od transcriptions leave 17 D-invariant blocks. The fighter alone is roughly 0.6 after P1 plus a ruling.