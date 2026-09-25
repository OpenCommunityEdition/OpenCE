# r2w1-xbox_texture_cache LEDGER (round 2, ADMISSION packet)

Worktree C:\halo-worktrees\claude-fifty-objects-r2-20260924, HEAD b7c1b7d9 (lane commit on f6d00a8c), tree clean at start.
Claim: source/cache/xbox_texture_cache CLAIMED (r2w1-xbox_texture_cache, "ADMISSION").
Read: WORKER_BRIEF_R2 (0-9), round-1 LEDGER/REVIEW (research/fifty_objects_20260925/w/{xbox_texture_cache,review_xbox_texture_cache}),
triage_rasterizer_a LEDGER P2/P7, hints.json packet (25/25 exact, audit FAIL(3) XDK wrapper storage 2/3).
Production source already contains the round-1 steal_memory named local (stolen_address).

## Baseline + evidence re-verification
- gate --all production (prod.obj): 25 exact / 0 residual / 0 unwritten (gate_prod.txt).
- object_audit production: all January-owned sections ok; FAIL(3) = storage 2/3 on the 3 XDK wrappers (audit_prod.txt).
- pdb_storage production: 3 disagreements, same 3 wrappers, PDB-public False (pdb_storage_prod.txt) -> January truth static.
- HCEX `DIA2Dump -type bitmap_data` re-run (hcex_bitmap_data.txt): 0x14 short mipmap_count, 0x16 short mipmap_pad,
  0x18 long pixels_offset, 0x1C long pixels_size, 0x20 long tag_index, 0x24 long cache_block_index, 0x28 void *hardware_format,
  0x2C void *base_address. January's own strings attest bitmap->cache_block_index ("texture->bitmap->cache_block_index==block_index"),
  bitmap->base_address, ->hardware_format, ->mipmap_count; nothing in January or the /Od build names 0x18/0x1C/0x20.
- Sept-2001 cachebeta xbe map (atlas 7eacac85, tier map) re-read: xbox_texture_cache.obj lists 22 names, constant delta
  0xE900 for EVERY function (delete 0x1ae410->0x1bcd10 ... get_hardware_format 0x1af060->0x1bd960). Names at the three
  invented statics: 0x1bcf60 _texture_cache_name_block_proc, 0x1bd0b0 _compare, 0x1bd220 _texture_cache_initialize_hardware_format.
  January's own atlas (4cc87b45, tier exact) independently lists _compare at 0x5ae7b0. The 4 XDK wrappers are unlisted
  in the Sept map (gap 0x40 between _compare end and _texture_cache_flush = 4 x 16-byte wrappers).
## Tooling: shadow-tree sweep (tools/hsweep.py)
- Copies source/+libs/ to s/ and compiles each of the 612 build.ninja cl TUs from cwd=s with the SAME relative path and
  cflags (xbox/include absolute) -> __FILE__ identical to production; canonical object compare (all sections except
  .debug$S: size/flags/bytes/relocations by symbol name, + symbol table).
- CONTROL: 612/612 compiled, 612/612 identical to build/base (sweep_control_vs_base.txt) -> sweep reproduces production.
- Transitive consumers from /showIncludes: cache/texture_cache.h = 14 TUs; bitmaps/bitmap_group.h = 45 TUs.
## Step A (TU-local + symbols.json) -- items (a), (g), (b), (e) + stale header comment
- EMULATED SPLIT: copied config/ -> emu/config, in-place CRLF line edits of symbols.json rows 7239 (->_texture_cache_name_block_proc),
  7242 (->_compare), 7243-7245 (+"static": true), 7250 (->_texture_cache_initialize_hardware_format); JSON parses; no other
  config file names the three old names. `build/tools/csplit.exe -i cachebeta.exe -p emu/config -o emu/split` exit 0;
  `diff -rq build/split emu/split` -> ONLY source/cache/xbox_texture_cache.obj differs (csplit deterministic, no other
  object references the renamed statics).
- tools/fncmp.py (per-function strict compare vs any split root) + tools/object_audit_emu.py (object_audit with EMU_SPLIT).
  Production vs emu split: 19 exact / 3 residual (callers: reloc names) / 3 unwritten (new names) -> as expected.
- A1 = renames (prototype, definition, use of each: name_block_proc passed to lruv_debug_to_file, compare passed to
  qsort_4byte, initialize_hardware_format called from start_loading_bitmap) + header comment (3 new names + 6 stale
  _code_ placeholders -> the names symbols.json already carries). emu: 25/25 EXACT.
- A2 = A1 + `#include "bitmaps/bitmaps_internal.h"` (genuine owner of bitmap_get_pixel_data_size), local prototype dropped.
  emu: 25/25 EXACT (repeats round-1 lab F1 on the renamed TU).
- A3 = A2 + `#include "rasterizer/xbox/rasterizer_xbox.h"` (owner of `extern D3DDevice *global_d3d_device`), consumer-local
  `extern D3DDevice global_d3d_device;` dropped, flush passes the pointer. emu: 25/25 EXACT; flush still 4 relocs, no
  _global_d3d_device reference (dead wrapper pThis) -> byte-inert as predicted by tinfo.
- object_audit_emu A3 vs emu split: **OBJECT AUDIT: PASS** (46 January symbols, 0 differ; all January-owned sections ok);
  surplus unchanged (drectve, 10 literals, 3 XDK tables, 4 __real, _add_vectors3d/_dot_product3d).
  All gates: emitted-symbol guard (_point_from_line3d) passed.
## Step B (item c) -- texture_cache.h prototypes: AS SPECIFIED IT REGRESSES
- B = A3 .c minus its 3 local public prototypes + texture_cache.h gains debug_render + both format prototypes.
  Shadow sweep of the 14 texture_cache.h consumers vs control (sweep_B_vs_control.txt, $L label names normalised):
  xbox_texture_cache 25/25 EXACT vs emu split; 11 consumers identical; **decals::_decals_delete_permanent_from_cluster
  EXACT -> residual** (400 B) and hud_weapon::_render_weapon_hud bytes move (already residual). => (c) as specified is
  NOT landable (declaration-count law; decals sentinel is the same one recorded in declaration-count-cleanup memory).
- LAB ORACLE countscan (tools/countscan.py; dummies never landed), extra prototypes in texture_cache.h:
  f1,f2 (void protos) identical; f3,f4 break decals; f5..f8 break decals+hud_weapon; p1 (one 2-param proto) identical;
  p2,p3 break both; x1..x3 (extern data) identical.
  Genuine subsets: D (debug_render only) ALL IDENTICAL; F1 (one format proto) ALL IDENTICAL; D+F1, D+F2 break decals;
  F1+F2 breaks both. => texture_cache.h can take exactly ONE of the three without a regression.
## Step P2 (item c, landable form): debug_render -> texture_cache.h; format mappers -> focused owner header
- Precedent: math/real_math_planes.h / matrix_math.h focused owner headers (declaration-count-cleanup memory; same decals
  sentinel). texture_cache.h is the platform-neutral interface (sound_cache.h sibling declares sound_cache_debug_render);
  the two mappers return D3DFORMAT codes and have NO caller outside xbox_texture_cache.obj in January (undefined-symbol
  census over every build/split object: only render_debug references _texture_cache_debug_render; nobody references
  _bitmap_format_to_d3d_format / _linear_format).
- P2 = A3 + .c drops its 3 local public prototypes + includes new cache/xbox_texture_cache.h (2 format prototypes);
  texture_cache.h + `void texture_cache_debug_render(void);`.
  Shadow sweep (14 consumers): 13/13 other consumers IDENTICAL to control; xbox_texture_cache 25/25 EXACT vs emu split.
## Step P3 (items d+f): bitmap_group.h corrected to HCEX + TU switched to the owner struct
- bitmap_group.h: 0x16 `unsigned short pixel_data` -> `short mipmap_pad`; 0x18 `unsigned long pixels_offset` -> `long`;
  0x1C `unsigned long pixel_data_size` -> `long pixels_size`; 0x20 `unsigned long bitmap_tag_index` -> `long tag_index`;
  0x24 `void *bitmap_data_pointer` -> `long cache_block_index` (all HCEX; cache_block_index also January-assert-attested).
- Compile census of the 45 bitmap_group.h consumers with the corrected header: only bink_playback.c (615-617) and
  bitmaps.c (1149, 1153) name renamed fields. Consumer edits: bink `pixels_size`, `tag_index`, `cache_block_index= NONE`
  (was `bitmap_data_pointer= (void *)NONE`); bitmaps.c `pixels_size` x2. Sweep C1: 45/45 consumers IDENTICAL.
- TU switch (P3): include bitmaps/bitmap_group.h; delete TU-local struct bitmap_data (+ its 8 verify typedefs) and the
  xbox_bitmap_group_prefix reserved-byte view (+ verify); `struct bitmap_group *bitmap_group = bitmap_group_get(index)`
  (same statement position), `bitmap->pixels_offset += bitmap_group->pixel_data.file_offset;`; field renames
  ->tag_index x7, ->pixels_offset x1, ->pixels_size x4. 
- Sweep P3 (48 TUs = union of both headers' consumers): 47/47 identical, xbox_texture_cache 25/25 EXACT (emu split).
- FULL SWEEP P3all (all 612 build TUs from the combined tree): **611 identical, only xbox_texture_cache differs**
  (renamed symbols / relocations to them). object_audit_emu (P3 obj vs emu split): **PASS**, 46/46 symbols, 20 surplus rows.
- Claims taken for edited dependencies: source/bink/bink_playback, source/bitmaps/bitmaps, source/bitmaps/bitmap_group (header owner).
## render_debug implicit declaration (item c tail)
- LAB RD: render_debug.c + `#include "cache/texture_cache.h"` (P2 header) -> render_debug.obj IDENTICAL to control.
  Claimed source/render/render_debug and folded into patch 02 (fixes the C4013 implicit call at render_debug.c:564).
## FINAL combined tree (P1+P2+P3+render_debug)
- Full sweep 612 TUs: 611 identical to control (== production build/base), only xbox_texture_cache differs (renamed
  statics). No parked body can move (parks in touched units: bitmap_group::_bitmap_group_add_bitmap,
  bink_playback::_bink_query_analog_controller_buttons -> both objects byte-identical). units.c sentinel identical.
- cand_FINAL.obj (sha256 bf1dbf20...) vs emu split: 25/25 EXACT with storage equal (fn_table_FINAL.txt);
  object_audit_emu PASS (all 17 January-owned data sections ok incl. _bitmap_d3d_format_tables and .bss; 46/46 symbols);
  pdb_storage_emu 0 disagreements; surplus_identity 2 COMDATs IDENTICAL; provider_link (both orders) PASS, 19 rows.
- No other split/base object defines or references any of the 9 changed names (census), so the storage flips/renames
  cannot orphan an external reference.
## Patches (patches/, LF, git-style)
- 01_tu_local_and_symbols.patch (xbox_texture_cache.c A3 + symbols.json 6 rows), 02_texture_cache_header_SHARED.patch
  (texture_cache.h, NEW cache/xbox_texture_cache.h, .c prototype moves, render_debug.c include),
  03_bitmap_group_header_SHARED.patch (bitmap_group.h HCEX, bink_playback.c, bitmaps.c, .c struct switch),
  04_config_status.patch (config.json NonMatching -> Matching), combined.patch; symbols.json full CRLF copy.
- `git apply --check` combined/01/04 at HEAD b7c1b7d9 (source == f6d00a8c): exit 0. GNU patch sequential 01->04 on an LF
  copy of the HEAD blobs == combined == the tested shadow tree sF (byte compare after LF normalisation).
## Incident note
- I ran `python -B -m tools.parked_functions --help` before reading the script (brief section 7). Checked immediately:
  `git status` shows no tracked modification; the script only writes with --output. No effect.
- Claims released (5 units) with outcome OBJECT_COMPLETE_CANDIDATE; MANIFEST.md written. Tree: no tracked modifications.
- Cleanup: removed intermediate shadow trees sB/sC/sLab/sP2/sP3/sRD and lab sweep/cs_* objects; kept s (pristine), sF (final tested tree), sweep/{control,B,C0,C1,P2,P3,P3all,FINAL,RD}.
