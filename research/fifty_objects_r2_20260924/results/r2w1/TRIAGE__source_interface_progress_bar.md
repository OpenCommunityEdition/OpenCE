# source/interface/progress_bar

## decision
RETIRE

## est_probability
0.03

## route
None for _draw_gravy_layer (January name per the atlas: draw_layer_int), 352 vs 336. Frame (0x1c) and the 22-relocation multiset are identical. The whole gap is January keeping rect corners as live values with dead spill homes, while VC7 forwards rect.* memory operands in ours.

## why_new
Nothing new at the byte level. I checked the atlas: the January-anchored jsonl names only draw_layer_int. Other builds show draw_layer and this_is_awful but give no January user for the PDB-static draw_gravy_screen_layer, which January emits with no referencing relocation in either tree. That fails reopen criterion (d).

## prior_negatives_checked
progress_bar_obj_opus5_next150_n1: 6 shapes (named corners, pointer alias, nested block, rotation, aggregate copy, inlined helper). Round-1 wave1: about 25 more perturbations, including definition order, K&R/void* prototypes, casts, arrays, unions, __inline, register, dead stores and D3DDevice direct calls. The lab-only no-summary route has no source cause.

## notes
Zero-credit admission debt for later: 24 D3D wrapper copies at progress_bar offsets (e.g. 855312, 856416) lack "static": true in symbols.json. Every other object's copies carry it (symbols.json 5591/5594/...), and the PDB has no publics for them, so the fix is in-place flags plus csplit regen. Also 5 Halo functions are PDB-static but external in ours, and there are atlas renames (draw_layer_int, do_convoluation_coords, draw_fade_layer, progress_bar_create_mask_texture) and the owner-gated _code_000d16d0 rename.