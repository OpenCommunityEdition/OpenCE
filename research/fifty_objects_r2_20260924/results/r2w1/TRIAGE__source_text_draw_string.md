# source/text/draw_string

## decision
OWNER

## est_probability
0.85

## route
Owner ruling on the January-bug class for _bitmap_draw_string: January's inverted `if (!bounds)` test followed by bounds-> reads, a NULL dereference. Round-1 cand1/cand2 give 28/28 strict, and the /Od-attested valid_format flag form is byte-inert. Separately, the ownership packet can land now at zero credit: ownership.patch + symbols_json.patch split the invented _draw_string_globals into four HCEX-attested file statics with = { 0 } ordering. With both, final_full.c passes the full audit against a simulated split.

## why_new
Round 1 already added the /Od corroboration: fn 0x8b2090 has the same inverted test and the RTC names adjusted_bounds/adjusted_clip. I verified here that the ownership packet has not landed: symbols.json line 23279 is still _draw_string_globals, and pdb_storage shows 1 disagreement.

## prior_negatives_checked
The park evidence says no safe expression yields the absolute reads at 0..6 without a relocation. Round-1 fn_draw_string wave1 (both units). draw_string_obj_* ledgers, including opus5_next150_n1 and lane D group J, list it as an owner ruling.

## notes
This is the held January-bug class from brief sec.8 (NULL dereference). Report only; do not land without approval. The bug body must carry a BUG (original) comment.