# source/physics/breakable_surfaces

## decision
OWNER

## est_probability
0.35

## route
Two owner rulings stand between this object and completion. B1: land the /Od-attested named-local body of real_local_random in real_math.h. It adds one declared name to every includer, which loses _rasterizer_frame_statistics_draw (4,176 B, exact only at today's declaration count) and _bitmap_copy. B2: extend the bsp3d TU-local j+k association helper precedent to _breakable_surface_plane_distance, a 32 B static surplus that January's image does not have. Separately, an admissible zero-regression sub-packet is ready and has NOT landed at f6d00a8c: (1) delete the hand-copied breakable_surface_get_plane_from_designator (breakable_surfaces.c:54) and call bsp3d_get_plane_from_designator; (2) drop the per-TU '/Ow','/QIfist' override at config.json line 520, which makes the six helper COMDATs identical to January's selected copies; (3) add "static": true to symbols.json line 5362 (_breakable_surface_effect) and line 23116 (_globals, file_offset 4550316), then run a csplit-only regen. Packet: research/fifty_objects_20260925 w/breakable_surfaces_audit2/patches.

## why_new
Nothing new removes B1 or B2. New today: the /Od build shows January's source had no header-body defect behind B2. /Od 0x7a0ca0 calls plane3d_distance_to_point(&s_plane/&t_plane, vertex_point) at 0x7a10bc, 0x7a10e8, 0x7a1119 and 0x7a1135, the same statement shape as ours. /Od 0x56d580 is dot_product3d(point,&plane->n)-d, and /Od 0x42dd80 is a left-associated i+j+k. Both are identical to real_math.h, so the j+k association is context-keyed codegen, not a spelling difference we can fix. B1 shares its victim with collisions (see the cross-cutting item).

## prior_negatives_checked
r1 wave1 audit_physics_rast_physics_breakable_surfaces.md; r1 wave2 breakable_surfaces_source_physics_breakable_surfaces.md: offsets -2..+5 all break frame_statistics_draw except 0; the only duplicate-prototype compensators are CRT, stack_walk (-2, still broken) and bitmap_delete (-2); the B24 outer-paren control was negative. Also breakable_surfaces_obj_opus5_150k_w2_20260914.md, breakable_surfaces_projection_provider_20260923.md, and owner queue 'held: breakable_surfaces B1'.

## notes
12/12 functions exact; data complete. Object audit fails only on 2 storage rows, fixed by the symbols.json edit above. The remaining link failure is _real_local_random against the effects copy (header body pop ecx vs January add esp,4), which is B1. real_math is RESERVED for the Codex packet, so any real_math.h change must wait for or compose with that report. Ledger: scratch/w/triage_objects_physics/LEDGER.md.