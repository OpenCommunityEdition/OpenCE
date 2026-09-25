# source/interface/virtual_keyboard

## decision
RETIRE

## est_probability
0.03

## route
None. _virtual_keyboard_select: the tail-merge survivor is the source-last arm in a flat switch. January's single `add eax,-0x24; cmp eax,7; ja` dispatch rules out every END-as-jump wrapper, because each one adds a compare (s3b). _virtual_keyboard_render_internal: six independent residual classes (-1 web split, key-IV base, loop-2 rotation, caret reload, font-header reload, csplit reloc alias).

## why_new
Nothing new. /Od 0x68ef60 confirms select's arm order. /Od 0x68fa00/0x68fdb0 are a later redesign with no key grid.

## prior_negatives_checked
Round-1 fn_virtual_keyboard select (e1-e30 labs, corpus survivors, s1/s3a-d, count oracle 1-32 inert) and render_internal (lab_pin1-5, r1, lab_style). virtual_keyboard_obj_* ledgers (fresh_graphs, 150k w1 and w3, residual one-shot, freshness audit). My alndiff confirms the select tail placement and the 0x2fa register swap (fixed only by the 'word buffer_size' lever, which must not land while the function is non-exact).

## notes
Zero-credit storage debt: 4 symbols are January-static but external in ours (free_space_in_text_buffer, get_current_character, _virtual_keyboard_globals, _virtual_keyboard_key_layout). The relocs.json alias retarget _item_update_section -> _keyboard_rect+352 is needed once render_internal is exact.