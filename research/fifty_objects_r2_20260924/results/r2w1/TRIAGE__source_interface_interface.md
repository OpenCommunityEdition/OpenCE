# source/interface/interface

## decision
OWNER

## est_probability
0.9

## route
Apply round-1 production.patch + symbols_static.patch (research/fifty_objects_20260925/results/wave1/fn_interface_source_interface_interface.md, REVIEW approve=True). The six profile_* globals become external, as cachebeta publics show. interface_get_weapon_hud_index, render_debug_profile and interface_splitscreen_render become static, as HCEX and the absence from cachebeta publics show. The result is 18/18 strict, and object_audit passes after csplit regen. The owner must pick one of two header variants. Variant A removes the interface.h prototype and adjudicates a declaration-count tie loss in _rasterizer_frame_statistics_draw (4176 B). Variant B keeps the extern prototype over the static definition (C4211 at /W4).

## why_new
Nothing new at the byte level. I checked the reopen criterion (c) arithmetic against January import tables. The genuine missing interface.h prototypes are interface_dispose and interface_dispose_from_old_map (imported by January game.obj) and interface_draw_fullscreen_overlays (render.obj). All three are weight-2 void prototypes. interface_get_rgb_color is PUBLIC but has no January importer. The profile_* globals have no importer in any other object, so there is no header-extern evidence. -3 + 2a + 5b = 0 has no solution, so no genuine packet balances the removed prototype.

## prior_negatives_checked
Round-1 LEDGER: net -3..+44 oracle, where only 0 keeps rfs_draw; C4211/C89 analysis; review items (a)-(d), including 'do not apply the semantic_matches item 5'. interface_obj_* ledgers. Production object_audit FAIL(7) = the weapon_hud_index section + 6 profile_* storage rows; the patch is not landed.

## notes
This is an owner pick, not more compiler work. Variant B has zero blast radius. On landing, retire the parked entry source/interface/interface:_interface_get_weapon_hud_index in the same change and edit symbols.json lines in place.