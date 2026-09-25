# source/bitmaps/bitmap_group

## decision
RETIRE

## est_probability
0.05

## route
None for _bitmap_group_add_bitmap. Round-1 cand.c is 1008/59 relocations with 346/346 instructions, and 4 instructions differ (which operand becomes the destination of the commutative add at the two pixel_data.address sites).

## why_new
Nothing new. No /Od or HCEX copy exists, and the PC lift was already used.

## prior_negatives_checked
bitmap_group_obj* ledgers (8, including cache_enum_reaudit and codex_add_bitmap_fuzzy); round-1 wave2 (operand order, casts, shadow field types, name-count, loop scoping; stop rule); park 33.

## notes
Still-pending zero-credit hygiene: static postprocess_bitmap/delete_bitmap plus symbols.json lines 1224-1225, the own-header include, and a single-section semantic_data_matches entry for _global_bitmap_reference (1,424 B, resolved-equal; integrator-applicable under brief s4.5). A park re-baseline is needed if the round-1 body lands.