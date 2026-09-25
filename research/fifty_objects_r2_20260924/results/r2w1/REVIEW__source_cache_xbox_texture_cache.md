# REVIEW source/cache/xbox_texture_cache

approve: True

amended_patch: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_xbox_texture_cache\combined_amended.patch

## per_object
[
 {
  "unit": "source/cache/xbox_texture_cache",
  "approve": true,
  "issues": "Approve only with the amendment: combined_amended.patch, or 01-04 plus 05_remove_invented_pad_member.patch. It removes the invented `byte reserved006[2]` pad from xbox_texture_cache_texture. HCEX texture_datum has no pad, and brief section 9 makes a pad member in a TU-private struct an admission blocker. The removal is byte-inert: 25/25 EXACT, object canonically identical, object_audit PASS against the emulated split, pdb_storage 0, surplus IDENTICAL, provider_link PASS. 06 (page-protection constants) is optional."
 },
 {
  "unit": "source/bitmaps/bitmap_group.h consumers (bink_playback, bitmaps + 44 others)",
  "approve": true,
  "issues": "The HCEX field correction is exact. All 46 consumers are byte-identical in the 612-TU sweep, and no park moves."
 },
 {
  "unit": "source/cache/texture_cache.h (+render_debug.c include) and new source/cache/xbox_texture_cache.h",
  "approve": true,
  "issues": "texture_cache.h gets the debug_render prototype only. The mappers go in a focused owner header (precedent: real_math_planes.h). The 3-prototype form's regression of decals and hud_weapon was reproduced. All 15 consumers are identical. render_debug loses its C4013. The packet composes cleanly with the pending owner-queue render_debug and object_lights packets."
 }
]

## checks
Everything ran independently in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2_xbox_texture_cache (REVIEW.md, LEDGER.md P0-P15). HEAD is b7c1b7d9, whose source equals f6d00a8c. git status stayed clean throughout.

(1) Patches:
- git apply --check at HEAD: combined, 01 and 04 exit 0. 02 and 03 fail on their own by design because they depend on 01.
- Applying 01->04 in sequence gives the same result as combined. That result matches the worker's sF tree and cand_P3.c after LF normalisation.
- My symbols.json equals patches/symbols.json byte for byte. It differs from config only on rows 7239, 7242-7245 and 7250.

(2) Names, checked by script:
- The Sept-2001 map in atlas 7eacac85 lists 21 xbox_texture_cache.obj names. The worker said 22; it is 21, plus 4 unlisted XDK wrappers, for 25. All 21 land on the patched symbols.json names at a constant delta of 0xE900.
- January's own atlas 4cc87b45 (exact tier) has 12 names, including _compare at 0x5ae7b0. All match.

(3) HCEX: I re-ran DIA2Dump on bitmap_data. It matches the patched bitmap_group.h exactly: mipmap_pad, pixels_offset, pixels_size, tag_index, cache_block_index.

(4) Emulated split: I made my own config copy and ran csplit (rc 0). diff -rq against build/split shows only xbox_texture_cache.obj differs.

(5) gate.py, run literally in a gate root (copies of build.ninja, tools, the candidate source tree, xbox/include and the emu split object), with --source source/cache/xbox_texture_cache.c --all --forbid-emitted-symbol _point_from_line3d:
- The guard passed. Result: exact 25, residual 0, unwritten 0.
- The per-function table shows storage equal on all 25; the XDK wrappers are now 3/3.

(6) Unmodified copies of scratch/tools scripts against the emu split:
- object_audit PASS: 42 January-owned sections ok and 46/46 symbols. Surplus is unchanged. The 3 XDK tables are also surplus in 64 objects that are already Matching.
- pdb_storage: 0 disagreements. Production has 3, all XDK wrappers.
- surplus_identity: 2 COMDATs IDENTICAL.
- provider_link: PASS on 19 rows in both orders.
- report.json: code 3457/3457 and data 6344/6344.

(7) Symbol census over split, base and candidate: no other object defines or references any renamed or storage-flipped name.

(8) Shadow sweep (my own rsweep.py, all 612 TUs):
- The control tree is 612/612 identical to build/base.
- Candidate vs control: 611 identical. Only xbox_texture_cache differs, through the renamed symbols.
- Consumer lists from /showIncludes match the worker's: texture_cache.h goes from 14 to 15 consumers, bitmap_group.h from 45 to 46, and the new header has 1.
- I reproduced the lab that puts all three prototypes in texture_cache.h. It changes decals (.text 400) and hud_weapon (.text 2624). The debug_render-only form is identical.
- The pending owner-queue packets (render_debug 05+06, object_lights 01+07) were applied to both sides. The amended packet still changes only xbox_texture_cache across all 612 TUs.

(9) CL /Zs /W3 warnings are identical, except that render_debug loses the C4013 for texture_cache_debug_render.

(10) Strip tests on pre-existing constructs:
- The bitmap_group named local, when inlined, stays EXACT.
- The point->vector casts in render_inverse_transform_screen_point, when removed, stay EXACT (the helper and debug_render). They are type-only; the construct is canonical Wave 12, commit 336fecdb.

(11) Amendments:
- A (remove the pad member) and B (page-protection constants) each give 25/25 EXACT.
- Both are canonically identical to the worker's candidate object in every section, relocation and symbol-table entry.
- The amended object passes object_audit and provider_link.
- combined_amended.patch: git apply --check exits 0 at HEAD. Applied, it reproduces the tested tree. 05 applied after 01-04 gives the same tree.

## issues
APPROVED ONLY WITH THE AMENDMENT. The packet is not admissible as submitted.

BLOCKER B1 (fixed in amended_patch): the TU-private struct xbox_texture_cache_texture keeps an invented pad member, `byte reserved006[2];`, between `boolean used` (offset 5) and `struct bitmap_data *bitmap` (offset 8).
- The HCEX analogue texture_datum (identifier, request_index, available@4, postprocessed@5, bitmap@8, texture@0xC) has no pad.
- Natural alignment already puts bitmap at 8.
- Brief section 9 lists pad members in a TU-private struct as an admission blocker even when bytes match.
- Round 1 removed this same class (player_profile byte pad[3], game_engine netgame_goal pads).
- The line predates this packet, but OBJECT_COMPLETE admits the whole TU.
- Removing it is byte-inert: 25/25 EXACT, and the object is canonically identical to the candidate, so object_audit and provider_link PASS.

INTEGRATION: apply combined_amended.patch, or the worker's 01..04 followed by scratch/w/review_r2_xbox_texture_cache/05_remove_invented_pad_member.patch. Add the new file source/cache/xbox_texture_cache.h. Regenerate the split with csplit before gating.

NON-BLOCKING:
(1) Optional 06_optional_page_protection_constants.patch changes XBOX_TEXTURE_CACHE_PROTECTION=0x404 to PAGE_READWRITE | PAGE_WRITECOMBINE. That is the XDK spelling physical_memory_map.c uses for the same memory. It is byte-identical.
(2) The TU-local bitmap enums remain. Canon now accepts them: bitmaps.c and bitmap_utilities.c were admitted Matching on 2026-09-23 and bitmap_extract.c on 2026-09-20, all with the same copies. The 20260904 veto predates that.
(3) TU-private type and field names differ from HCEX (texture_datum / texture_cache_debug_options_definition). This is an owner preference only.
(4) bitmap_d3d_format_tables[2][18] is a descriptive, pre-existing interpretation.
(5) A name-keyed stable sweep will show the 3 renames as 3 losses and 3 gains, because scratch/campaign/gate_baseline has the old names.
(6) Re-sweep after composing with other round-2 header packets. The decals and hud_weapon sentinels are one declaration away.
(7) Manifest erratum: the Sept map lists 21 xbox_texture_cache names, not 22.
