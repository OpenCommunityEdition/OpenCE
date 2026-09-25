# source/interface/hud_nav_points

## decision
RETIRE

## est_probability
0.03

## route
None. _custom_render_nav_point differs only in scheduling. My dis_range at 0x520-0x5a8 shows January emits `push 0` (override_scale 0.0f) before `mov eax,[ebp-0x10]; mov ecx,[ebp-0x14]`, and ours hoists both loads first.

## why_new
Nothing new. The only known mechanisms that move a constant push ahead of region-head loads are a register conflict and a tail-merge IL block boundary. January has neither: no jump into +0x58c and no partner call.

## prior_negatives_checked
Round-1 fn2_hud_nav_points custom_render: more than 30 prior shapes, 19 lab diagnostics, the /Od 0x62fcf0-faithful od1, dummy-local oracles and /G5-/G7//Ot//Og. hud_nav_points_obj_* ledgers, including the exhaustive census and complete_renderer_fuzzy_closeout.

## notes
Even an exact body would still leave admission blockers (review4): consumer-local prototypes for objects.h, game_engine.h and units.h functions; opaque-span struct views hud_globals_definition and hud_nav_object_datum; float z_offset/vertical_offset. object_audit symbols: 0 differ.