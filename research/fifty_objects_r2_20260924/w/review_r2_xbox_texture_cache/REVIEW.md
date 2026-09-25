# Adversarial review: source/cache/xbox_texture_cache (r2w1 OBJECT_COMPLETE_CANDIDATE)

**Verdict:** approved only with the amendment. The packet as submitted is not admissible. With the reviewer's
one-line fix it is.

- Apply `scratch/w/review_r2_xbox_texture_cache/combined_amended.patch`. It is the worker's combined.patch plus the
  removal of an invented pad member.
- Equivalently, apply the worker's 01..04 in order, then `05_remove_invented_pad_member.patch`.
- `06_optional_page_protection_constants.patch` is optional. It is byte-identical and applies after the amended packet.

Tree: worktree HEAD b7c1b7d9 (source/config/tools == f6d00a8c). `git status` stayed clean throughout. Every probe is in
LEDGER.md in this directory.

## Blocking finding (fixed in the amendment)

**B1: invented pad member in a TU-private struct.** `struct xbox_texture_cache_texture` keeps `byte reserved006[2];`
between `boolean used` (at 5) and `struct bitmap_data *bitmap` (at 8).
- The HCEX PDB analogue `texture_datum` has no pad. Its fields are identifier, request_index, available@4,
  postprocessed@5, bitmap@8 and texture@0xC (`DIA2Dump -type texture_datum` in hcex_texture_datum.txt).
- Natural alignment already puts `bitmap` at 8.
- Brief section 9 lists "pad members in a TU-private struct" as an admission blocker even when every byte matches.
- Round-1 admissions removed exactly this class: the player_profile `byte pad[3]` and the game_engine netgame_goal pads.

The construct predates this packet, but OBJECT_COMPLETE admits the whole TU.

The fix deletes the line. Measured result:
- gate (emulated split): 25/25 EXACT, and the point_from_line3d guard passes.
- The production-path compile is canonically identical to the worker's candidate object in every section, relocation
  and symbol-table entry. object_audit PASS, provider_link PASS, and the /W3 warnings are unchanged.
- The verify typedefs still hold: loaded 4, used 5, bitmap 8, hardware_format 0xC, size 0x20.

## Independent re-verification of the worker's claims (all reproduced)

- **Patches.**
  - `git apply --check` at HEAD: combined, 01 and 04 exit 0. 02 and 03 alone fail by design; they depend on 01.
  - Sequential 01->04 (non-repo git apply on LF HEAD blobs) equals combined.
  - The result equals the worker's sF tree and cand_P3.c.
  - Its symbols.json equals patches/symbols.json byte for byte and differs from config only on rows 7239, 7242-7245
    and 7250.
- **Names** (atlas; scripted check):
  - The 7eacac85 Sept-2001 map lists 21 xbox_texture_cache.obj code names. The manifest says 22; it is 21, plus the
    4 unlisted XDK wrappers, for 25. All 21 land on the patched names at a constant delta of 0xE900.
  - January's own 4cc87b45 exact tier has 12 names, including `_compare` at 0x5ae7b0. All match.
- **HCEX bitmap_data** (re-dumped): 0x16 short mipmap_pad, 0x18 long pixels_offset, 0x1C long pixels_size, 0x20 long
  tag_index, 0x24 long cache_block_index. This equals the patched bitmap_group.h exactly.
- **Emulated split:**
  - Own copy of config with the patched symbols.json and config.json, then csplit (rc 0).
  - `diff -rq build/split` shows only xbox_texture_cache.obj differs.
- **Strict gate:**
  - gate.py was run literally in a gate root (copies of build.ninja, tools, the candidate source tree, xbox/include and
    the emulated-split object), with `--source source/cache/xbox_texture_cache.c --all --forbid-emitted-symbol
    _point_from_line3d`.
  - Result: exact 25, residual 0, unwritten 0.
  - The per-function table also shows storage equal on all 25, including the XDK wrappers at 3/3.
- **object_audit (emu):**
  - PASS: 42 January-owned sections ok (15 strings, _bitmap_d3d_format_tables 144, .bss 5656, 25 .text) and 46/46
    symbols.
  - Surplus is unchanged from production: .drectve, 10 literals, 3 XDK tables, 4 __real, add_vectors3d/dot_product3d.
  - The 3 XDK tables are surplus in 64 already-Matching objects (census over build/base), so that is precedent.
- **pdb_storage (emu):** 0 disagreements; production has 3 (the wrappers). The mappers and debug_render are
  cachebeta publics, so their external storage is correct.
- **surplus_identity:** 2 COMDATs IDENTICAL.
- **provider_link:** SELECTED-PROVIDER LINK PASS on 19 rows, both orders.
- **report.json:** code 3457/3457 and data 6344/6344 today.
- **Name census** (split, base and candidate objects): no other object defines or references a renamed or
  storage-flipped name. render_debug alone references `_texture_cache_debug_render`, as UNDEF.
- **Shadow sweep** (own rsweep.py, 612 build.ninja TUs):
  - The control tree is 612/612 identical to build/base.
  - Candidate vs control: 611 identical; only xbox_texture_cache differs, through the renamed statics.
  - Consumer lists from /showIncludes equal the worker's lists: texture_cache.h 14 -> 15, bitmap_group.h 45 -> 46, and
    the new header 1.
  - decals, hud_weapon, units, bink_playback, bitmaps and bitmap_group are all byte-identical, so no park can move.
- **Focused-header justification reproduced.** Putting debug_render and both mapper prototypes in texture_cache.h
  changes decals (.text 400, decals_delete_permanent_from_cluster) and hud_weapon (.text 2624). The debug_render-only
  form is identical.
  - Precedent: math/real_math_planes.h (6c7ab19d), which uses the same comment pattern.
  - sound_cache.h declares the sibling sound_cache_debug_render.
- **Composition** with the pending owner-queue render_debug/object_lights packets (05+06, 01+07): with them applied to
  both sides, the amended packet still changes only xbox_texture_cache across all 612 TUs.
- **/W3:** the warning sets are identical, except that render_debug loses the C4013 for texture_cache_debug_render.
- **Strip tests on the unusual pre-existing constructs:**
  - The `bitmap_group` named local, inlined away, stays EXACT, so it is not load-bearing.
  - The point->vector casts in render_inverse_transform_screen_point, removed, keep the helper and debug_render EXACT.
    They are type-only. The construct is canonical Wave 12 (336fecdb, A30 class).

## Non-blocking notes (owner or integrator)

1. **Optional patch 06.** It changes `XBOX_TEXTURE_CACHE_PROTECTION = 0x404` to `PAGE_READWRITE | PAGE_WRITECOMBINE`.
   That is the XDK spelling that physical_memory_map.c's XPhysicalAlloc uses for this same memory, and that
   game_state_xbox.c uses. It is byte-identical and 25/25.
2. **TU-local bitmap type/format/flag enums** remain. The Matching objects bitmaps.c, bitmap_utilities.c (admitted
   2026-09-23) and bitmap_extract.c carry the same TU-local copies, so this is no longer a whole-object veto in canon.
   The 20260904 ledger's veto predates that.
3. **TU-private type and field names differ from HCEX.** HCEX has texture_datum{request_index, available,
   postprocessed} and texture_cache_debug_options_definition{display_cache_graph, display_cache_list}. The descriptive
   names are allowed, but the owner may prefer the HCEX names. The Xbox datum layout differs from the PC one (it
   embeds D3DBaseTexture), so the field mapping is inference.
4. **`bitmap_d3d_format_tables[2][18]`** is a descriptive, pre-existing interpretation of January's one 144-byte table
   (base and +0x48). Its data credit is already landed.
5. **Integration.**
   - Regenerate the split (csplit) before gating.
   - A name-keyed stable sweep will show 3 renames as 3 lost and 3 gained functions. scratch/campaign/gate_baseline
     still uses the old names, so use a rename-aware diff.
   - Re-sweep after composing with any other round-2 header packet, because isolated tests do not compose. The decals
     and hud_weapon sentinels are one declaration away.
   - The new file source/cache/xbox_texture_cache.h must be added.
6. **Manifest erratum.** It says "22 code names" in the Sept map; the count is 21.
