"""Build f1.c (the corrected Q4 source packet) from w2.c (W2's packet applied to HEAD)."""
F = 'scratch/campaign/workers/F1/'
src = open(F + 'w2.c', encoding='latin-1', newline='').read()
assert '\r' not in src

old_typedef = (
    "typedef char verify_lens_flare_reset_overrun_stays_in_first_parameters_record[\n"
    "\tsizeof(struct lens_flare_occlusion_test_results) == 0x22 &&\n"
    "\tsizeof(struct rasterizer_lens_flare_submit_parameters) >= sizeof(struct lens_flare_occlusion_test_results) ? 1 : -1];\n")
new_typedef = (
    "typedef char verify_lens_flare_reset_overrun_record_sizes[\n"
    "\tsizeof(struct lens_flare_occlusion_test_results) == 0x22 &&\n"
    "\tsizeof(struct rasterizer_lens_flare_submit_parameters) == 0x28 ? 1 : -1];\n")
assert src.count(old_typedef) == 1
src = src.replace(old_typedef, new_typedef)

old_comment = src[src.index("\t/* BUG (preserved for exact matching): January clears one record"):]
old_comment = old_comment[:old_comment.index("\t */\n") + len("\t */\n")]
new_comment = """\t/* BUG (preserved for exact matching): January clears MAXIMUM_LIGHTS_PER_MAP+1 records here
\t * (push 0x7722 = 897*0x22), one more than local_lens_flare_occlusion_test_results holds (0x7700 bytes),
\t * so it also zeroes the 0x22 bytes that follow the array in this file's .bss. In January's image and in
\t * our object those are the first bytes of local_lens_flare_parameters[0], definition through
\t * lens_flare_index (+0x00..+0x21); compressed_window_index, compressed_light_scale and
\t * internal__occlusion_pixels are not touched.
\t * verify_lens_flare_reset_overrun_record_sizes only checks the record sizes; C cannot check which
\t * object follows the array. tools/test_rasterizer_lights_reset_overrun_layout.py checks that on the
\t * built object and on January's split (neighbour at exactly +0x7700, at least 0x22 bytes long, nothing
\t * else in the span); if it fails, this length must not stay.
\t * Lifecycle, from January's code: local_lens_flare_count is zeroed below with no read in between;
\t * every read of a parameters record uses an index below local_lens_flare_count; and the count only
\t * grows in rasterizer_lens_flare_submit, which copies the whole 0x28-byte record before anything reads
\t * it. So the zeroed bytes are not read before they are overwritten, provided that: all users run on
\t * one thread; nothing called from the lens flare loops re-enters this file (this function is also the
\t * hs command rasterizer_lights_reset_for_new_map, so it runs whenever scripts or the console do, not
\t * only at map load); no non-local exit leaves the count raised without the copy; and code outside this
\t * file reaches the records only through the pointers these loops pass down during the call. The code
\t * does not prove those conditions.
\t * A corrected build should clear sizeof(local_lens_flare_occlusion_test_results).
\t */
"""
src = src.replace(old_comment, new_comment)
open(F + 'f1.c', 'w', encoding='latin-1', newline='').write(src)
print('ok')
