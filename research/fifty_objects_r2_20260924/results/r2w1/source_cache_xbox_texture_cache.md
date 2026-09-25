# source/cache/xbox_texture_cache

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\patches\\combined.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\patches\\01_tu_local_and_symbols.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\patches\\02_texture_cache_header_SHARED.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\patches\\03_bitmap_group_header_SHARED.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\patches\\04_config_status.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\patches\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\cand_P3.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\cand_P2_xbox_texture_cache.h",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\cand_FINAL.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\xbox_texture_cache\\tools\\hsweep.py"
]

## production_changes
All patches are in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\xbox_texture_cache\patches\. They are LF git-style diffs; apply them in order 01 to 04, or use combined.patch.

(1) 01_tu_local_and_symbols.patch:
- xbox_texture_cache.c: the three first-party renames. texture_cache_debug_block_name becomes texture_cache_name_block_proc, texture_cache_debug_bitmap_compare becomes compare, and texture_cache_build_hardware_format becomes texture_cache_initialize_hardware_format (prototype, definition and use of each). The stale _code_ header inventory is refreshed.
- #include "bitmaps/bitmaps_internal.h" replaces the TU-local bitmap_get_pixel_data_size prototype.
- #include "rasterizer/xbox/rasterizer_xbox.h" replaces `extern D3DDevice global_d3d_device;`, and flush now passes the pointer.
- config/symbols.json, 6 in-place row edits: 7239 _texture_cache_name_block_proc, 7242 _compare, 7250 _texture_cache_initialize_hardware_format, and "static": true on 7243-7245 (_IDirect3DDevice8_IsBusy@4, _IDirect3DDevice8_KickPushBuffer@4, _IDirect3DBaseTexture8_IsBusy@4).
- The full edited CRLF copy is patches/symbols.json. It is byte-identical to the config that produced the emulated split.
- After applying: csplit regen. Only source/cache/xbox_texture_cache.obj changes, as verified by diff -rq on the emulated split.

(2) 02_texture_cache_header_SHARED.patch (shared header):
- source/cache/texture_cache.h gains `void texture_cache_debug_render(void);`.
- NEW file source/cache/xbox_texture_cache.h: a focused owner header with the bitmap_format_to_d3d_format and bitmap_format_to_d3d_linear_format prototypes. It follows the math/real_math_planes.h precedent and its comment gives the reason.
- xbox_texture_cache.c drops its 3 local public prototypes and includes the new header.
- source/render/render_debug.c adds #include "cache/texture_cache.h", which fixes the implicit call at line 564.
- texture_cache.h consumers (14): source/bink/bink_playback.c, source/cache/cache_files_windows.c, source/cache/xbox_texture_cache.c, source/effects/decals.c, source/interface/hud.c, source/interface/hud_draw.c, source/interface/hud_messaging.c, source/interface/hud_nav_points.c, source/interface/hud_unit.c, source/interface/hud_weapon.c, source/interface/ui_widget.c, source/objects/object_lights.c, source/objects/widgets/lightning.c, source/rasterizer/xbox/rasterizer_xbox.c. After the patch, source/render/render_debug.c is a 15th.
- The new header's only includer is xbox_texture_cache.c.

(3) 03_bitmap_group_header_SHARED.patch (shared header):
- source/bitmaps/bitmap_group.h struct bitmap_data is corrected to HCEX. 0x16 `unsigned short pixel_data` becomes `short mipmap_pad`, 0x18 becomes `long pixels_offset`, 0x1C becomes `long pixels_size`, 0x20 becomes `long tag_index`, and 0x24 `void *bitmap_data_pointer` becomes `long cache_block_index`.
- Consumer renames in source/bink/bink_playback.c (615-617; `cache_block_index= NONE` replaces `bitmap_data_pointer= (void *)NONE`) and source/bitmaps/bitmaps.c (1149, 1153).
- xbox_texture_cache.c drops the TU-local struct bitmap_data (with its 8 verify typedefs) and the xbox_bitmap_group_prefix reserved-byte view. It includes bitmaps/bitmap_group.h, uses bitmap_group_get() and ->pixel_data.file_offset, and renames fields: ->tag_index x7, ->pixels_offset x1, ->pixels_size x4.
- Complete list of the 45 bitmap_group.h consumer TUs (no paths with spaces), in scratch/w/xbox_texture_cache/consumers_bitmap_group_h.txt: source/bink/bink_playback.c, source/bitmaps/{bitmap_drawing,bitmap_extract,bitmap_group,bitmap_utilities,bitmaps,bitmaps_quantitize,targa_file,tiff_file}.c, source/cache/predicted_resources.c, source/cutscene/cinematics.c, source/effects/{contrails,decals,particle_systems,particles,weather_particle_systems}.c, source/game/game_engine.c, source/interface/{hud,hud_draw,hud_messaging,hud_nav_points,hud_weapon,interface,overhead_map,ui_widget,virtual_keyboard}.c, source/main/main.c, source/objects/object_lights.c, source/objects/widgets/{antenna,glow,lightning}.c, source/rasterizer/{rasterizer_cinematics,rasterizer_swizzle,rasterizer_text}.c, source/rasterizer/xbox/{rasterizer_xbox,rasterizer_xbox_detail_objects,rasterizer_xbox_environment_fog,rasterizer_xbox_hardware_bitmaps,rasterizer_xbox_motion_sensor,rasterizer_xbox_screen_effect,rasterizer_xbox_transparent_geometry}.c, source/render/{render_contrails,render_sprite}.c, source/structures/structure_render.c, source/text/draw_string.c. After the patch, source/cache/xbox_texture_cache.c is a 46th.

(4) 04_config_status.patch: config/config.json line 403, xbox_texture_cache.c NonMatching becomes Matching.

No parks to retire; the unit has none.

Verification:
- git apply --check exits 0 at HEAD for combined, 01 and 04. HEAD's source equals f6d00a8c.
- GNU patch applying 01 to 04 in sequence reproduces combined.patch and the tested shadow tree byte for byte.

## evidence
Baseline production: gate 25/25 EXACT; object_audit FAIL(3) (the XDK wrappers are storage 2 in the split, 3 in ours); pdb_storage shows 3 disagreements (not PDB-public, so static).

First-party names:
- The Sept-2001 cachebeta xbe map (atlas 7eacac85, tier map) lists all 22 xbox_texture_cache.obj code names at a constant delta of 0xE900 from January. It gives _texture_cache_name_block_proc at 0x1bcf60, _compare at 0x1bd0b0 and _texture_cache_initialize_hardware_format at 0x1bd220.
- January's own atlas 4cc87b45 lists _compare at 0x5ae7b0, exact tier.

bitmap_data layout: HCEX `DIA2Dump -type bitmap_data` gives the field list above. January's assert string attests `texture->bitmap->cache_block_index`.

Sweep tool (hsweep.py): it compiles all 612 build.ninja TUs from a shadow copy with production flags and relative paths. The control run reproduced build/base 612/612 identical.

Item (c) as specified regresses:
- It moved decals::_decals_delete_permanent_from_cluster from EXACT to residual and moved hud_weapon::_render_weapon_hud (declaration-count law).
- Lab oracle countscan (never landed): any 1 of the 3 genuine prototypes is inert; debug_render plus either format prototype breaks decals; the two format prototypes together break decals and hud_weapon.
- Hence the redesign: debug_render goes into texture_cache.h and the mappers into a focused header. The mappers have no January caller outside this object (undefined-symbol census over build/split).
- The P2 sweep: 13/13 other consumers identical.

Other sweeps:
- bitmap_group.h HCEX correction with the 2 consumer renames: 45/45 consumers identical.
- P3: 47/47 identical.
- render_debug.c including texture_cache.h: identical.
- FINAL (all patches together, all 612 TUs): 611 identical; only xbox_texture_cache differs, through its renamed symbols. No park moves: the bitmap_group and bink_playback parks are in byte-identical objects.

FINAL candidate against the emulated split (csplit of the edited symbols.json; only the texture cache object changes):
- 25/25 EXACT, with size, normalized sha, reloc count and storage equal (fn_table_FINAL.txt).
- object_audit PASS: 17 January-owned data sections ok and 46/46 symbols.
- pdb_storage: 0 disagreements.
- surplus_identity: 2 COMDATs IDENTICAL.
- provider_link: PASS in both orders on 19 rows.
- A symbol census found no other object defining or referencing the 9 changed names.

Files:
- Ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\xbox_texture_cache\LEDGER.md
- Manifest: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\xbox_texture_cache\MANIFEST.md

## blockers
None by measurement. For the owner and integrator:
- The new file source/cache/xbox_texture_cache.h needs acceptance.
- TU-local bitmap type/format/flag enums remain. This is board-wide schema debt shared with bitmaps.c, bitmap_utilities.c, bitmap_extract.c, bitmap_group.c and bink_playback.c; no owner header exists, and the 20260904 ledger calls it a veto class. It needs an owner ruling or a board-wide enum-owner packet.
- texture_cache_debug_bitmaps keeps its provisional name. The map atlas is code-only and HCEX has only the PC cache.
- The existing `(boolean (*)(long, long))compare` cast at the qsort_4byte call is exact but a style question.
- Isolated sweeps do not compose with other lanes' header packets. Re-run the decals sentinel and parks on the live tree after applying.

## reopen_criteria
If the integrator rejects the new focused header, the fallback is to delete the two format-mapper prototypes outright. They are defined before their only in-TU use and have no external caller. Placing them in texture_cache.h is measured to regress decals and must not be used.

For the TU-local enums: they are cleared by an owner ruling that accepts TU-local tag enums until a board-wide bitmap-enum packet exists, or by that packet itself.

The packet must be re-validated if another lane lands a texture_cache.h or bitmap_group.h change first.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\xbox_texture_cache\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\xbox_texture_cache\LEDGER.md
