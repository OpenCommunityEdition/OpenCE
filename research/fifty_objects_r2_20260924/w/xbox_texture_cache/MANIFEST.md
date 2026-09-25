# MANIFEST: source/cache/xbox_texture_cache (r2w1-xbox_texture_cache, ADMISSION)

Base: worktree HEAD b7c1b7d9 (lane docs commit; `git diff f6d00a8c b7c1b7d9 -- source config tools` is empty), so the
packet applies to canonical f6d00a8c. Tip: no commits (the worker never mutates git). Evidence: LEDGER.md in this directory.

## What the packet does

| item | change | files |
|---|---|---|
| (a) | `"static": true` on symbols.json rows 7243-7245 (`_IDirect3DDevice8_IsBusy@4`, `_IDirect3DDevice8_KickPushBuffer@4`, `_IDirect3DBaseTexture8_IsBusy@4`) | config/symbols.json |
| (g) | first-party names: row 7239 `_texture_cache_name_block_proc`, 7242 `_compare`, 7250 `_texture_cache_initialize_hardware_format`, plus the .c definitions, prototypes and uses; stale `_code_` header inventory refreshed | config/symbols.json, xbox_texture_cache.c |
| (b) | `#include "bitmaps/bitmaps_internal.h"` replaces the TU-local `bitmap_get_pixel_data_size` prototype | xbox_texture_cache.c |
| (e) | `#include "rasterizer/xbox/rasterizer_xbox.h"`. The consumer-local `extern D3DDevice global_d3d_device;` is dropped, and flush passes the `D3DDevice *` | xbox_texture_cache.c |
| (c) | `texture_cache_debug_render` goes into cache/texture_cache.h. The two D3D format mappers go into a NEW focused owner header, cache/xbox_texture_cache.h. The 3 local public prototypes are dropped from the .c. render_debug.c includes texture_cache.h, which fixes its implicit declaration | texture_cache.h, xbox_texture_cache.h (new), xbox_texture_cache.c, render_debug.c |
| (d)+(f) | bitmap_group.h `struct bitmap_data` corrected to HCEX: 0x16 `short mipmap_pad`, 0x18 `long pixels_offset`, 0x1C `long pixels_size`, 0x20 `long tag_index`, 0x24 `long cache_block_index`. The consumers bink_playback.c and bitmaps.c are renamed to match. The TU drops its local `struct bitmap_data` and the `xbox_bitmap_group_prefix` reserved-byte view, and uses `bitmap_group_get()` and `->pixel_data.file_offset` | bitmap_group.h, bink_playback.c, bitmaps.c, xbox_texture_cache.c |
| status | config.json line 403 `NonMatching` becomes `Matching` | config/config.json |

## Deviation from the triage spec, with its cause (measured)

Item (c) as specified adds all 3 prototypes to cache/texture_cache.h. That regresses
`decals::_decals_delete_permanent_from_cluster` from EXACT to residual (400 B) and moves the residual
`hud_weapon::_render_weapon_hud`. The mechanism is the declaration-count law. countscan.py is a lab oracle only:
- any 1 of the 3 genuine prototypes is inert
- any 2 break decals
- all 3 break decals and hud_weapon

So texture_cache.h takes only `texture_cache_debug_render`. That is the sibling pattern: sound_cache.h declares
`sound_cache_debug_render`. The two mappers are Xbox-only D3D helpers with no caller outside this object anywhere in
January (undefined-symbol census over every build/split object). They go into a focused owner header, following the
math/real_math_planes.h precedent, whose comment states the reason. The new file must be accepted by the integrator.

## Source evidence per changed construct

- Names (g): the 2001-09-25 cachebeta xbe linker map (atlas 7eacac85, tier map) lists all 22 xbox_texture_cache.obj
  code names with a constant delta of 0xE900 against January:
  - `_texture_cache_name_block_proc` 0x1bcf60 (Jan 0x1ae660)
  - `_compare` 0x1bd0b0 (Jan 0x1ae7b0). January's own atlas 4cc87b45 also has it at exact tier, 0x5ae7b0
  - `_texture_cache_initialize_hardware_format` 0x1bd220 (Jan 0x1ae920)
- Storage (a): cachebeta.pdb publics list decorated stdcall names (e.g. `_D3DResource_IsBusy@4`) but not these 3
  wrappers, so they are file-static. Ours was already 3. Precedent: the rasterizer_xbox_decals admission. The renamed
  statics are absent from the publics as well.
- bitmap_data (d): HCEX.pdb `DIA2Dump -type bitmap_data` gives the exact list above (hcex_bitmap_data.txt).
  January's own assert string `texture->bitmap->cache_block_index==block_index` attests the 0x24 name. The old header
  field `void *bitmap_data_pointer` at 0x24 was wrong; bink stored `(void *)NONE` there, which now reads
  `cache_block_index= NONE`.
- global_d3d_device (e): the owner declaration is rasterizer_xbox.h:164 `extern D3DDevice *global_d3d_device`. tinfo
  for `_texture_cache_flush` shows 4 relocs and none to `_global_d3d_device`, because the wrapper pThis is dead. The
  change is byte-inert, as measured.
- bitmap_get_pixel_data_size (b): owner is bitmaps/bitmaps_internal.h.

## Target vs candidate (emulated split = csplit of the edited symbols.json; cand_FINAL.obj sha256 bf1dbf20...)

All 25 functions are EXACT, with padded size, normalized sha, relocation count and storage equal. The full table is in
fn_table_FINAL.txt. Examples: steal_memory 224/16 relocs 7ffbda42; debug_render 784/46 ce7f0ad9; get_hardware_format
496/41 087db447; initialize_hardware_format 336/17 0cd7b038; `_compare` 48/2 e4a70834; name_block_proc 48/3 68491468.

- Data: object_audit reports all 17 January-owned data sections ok. That covers 15 string literals,
  `_bitmap_d3d_format_tables` (144) and `.bss _texture_cache_debug_bitmaps` (5656). report.json already shows data at
  6344/6344.
- Symbols: object_audit_emu PASS, 46/46 January symbols (storage, section, offset). pdb_storage_emu: 0 disagreements.
  Production today: FAIL(3) / 3 disagreements.
- Surplus: unchanged from production and already accepted in round 1. It is .drectve, 10 pooled literals, 3 XDK
  tables, 4 `__real` and `_add_vectors3d`/`_dot_product3d`. surplus_identity: both COMDATs IDENTICAL.
  provider_link, both orders: PASS on 19 rows.
- Census: no other split or base object defines or references any renamed or storage-flipped name.

## Blast radius (shadow-tree sweep, tools/hsweep.py)

- Control: all 612 build.ninja TUs compiled from a copy of source/ with production flags and relative paths. All 612
  are identical to build/base, which shows the harness reproduces production.
- Consumers:
  - texture_cache.h has 14: bink_playback, cache_files_windows, xbox_texture_cache, decals, hud, hud_draw,
    hud_messaging, hud_nav_points, hud_unit, hud_weapon, ui_widget, object_lights, lightning, rasterizer_xbox
  - the new xbox_texture_cache.h has 1: xbox_texture_cache
  - render_debug.c's include affects render_debug only
- bitmap_group.h has 45 consumers (consumers_bitmap_group_h.txt). The paths include no spaces. They are:
  bink_playback, bitmap_drawing, bitmap_extract, bitmap_group, bitmap_utilities, bitmaps, bitmaps_quantitize,
  targa_file, tiff_file, predicted_resources, cinematics, contrails, decals, particle_systems, particles,
  weather_particle_systems, game_engine, hud, hud_draw, hud_messaging, hud_nav_points, hud_weapon, interface,
  overhead_map, ui_widget, virtual_keyboard, main, object_lights, antenna, glow, lightning, rasterizer_cinematics,
  rasterizer_swizzle, rasterizer_text, rasterizer_xbox, rasterizer_xbox_detail_objects, rasterizer_xbox_environment_fog,
  rasterizer_xbox_hardware_bitmaps, rasterizer_xbox_motion_sensor, rasterizer_xbox_screen_effect,
  rasterizer_xbox_transparent_geometry, render_contrails, render_sprite, structure_render, draw_string.
  After the patch, xbox_texture_cache also consumes it.
- Per step:
  - P2: 13 of 13 other consumers identical
  - bitmap_group.h alone: 45 of 45 identical
  - P3: 47 of 47 identical
- FINAL (every patch together, all 612 TUs): 611 identical. Only xbox_texture_cache differs, through the renamed
  symbols. No parked body moves. The touched units' parks (`_bitmap_group_add_bitmap`,
  `_bink_query_analog_controller_buttons`) are in byte-identical objects.

## House-rule review of changed lines

- Genuine first-party names.
- Prototypes are in owner headers.
- There is no consumer-local extern or prototype for another file's function.
- The typed tag accessor `bitmap_group_get` replaces the raw `tag_get('bitm')`.
- The genuine owner struct replaces the TU-local duplicate and the reserved-byte view.
- One cast is removed (`(void *)NONE` on a long field).
- No filler, pragma or decoration.
- The strip test is not applicable, because every change is byte-inert by measurement.

## Tests run

- gate.py for A1-A3, with the `_point_from_line3d` emitted-symbol guard passing.
- fncmp and object_audit_emu against the emulated split.
- pdb_storage_emu, surplus_identity_emu and provider_link.
- hsweep control/P2/C1/P3/P3all/FINAL.
- `git apply --check` of combined, 01 and 04 at HEAD.
- GNU patch applied 01 to 04 in sequence to LF HEAD blobs. The result matched combined and matched the tested shadow
  tree.

pytest and ninja were not run; they are integrator-only.

## Credit

- No new function or data bytes. Code is already 3457/3457 and data 6344/6344.
- The credit is whole-object admission: +1 Matching object.
- 0 zero-credit parks.

## Held items / owner notes (not blocking by measurement, flagged for the owner)

1. TU-local bitmap type/format/flag enums and the d3d format-table enum. No owner header defines them. bitmaps.c,
   bitmap_utilities.c, bitmap_extract.c, bitmap_group.c and bink_playback.c carry their own copies. This is
   board-wide bitmap-schema debt, and the 20260904 ledger calls it a veto class. Reopen with a board-wide bitmap-enum
   owner packet in bitmap_group.h that removes every TU copy at once. January assert strings attest
   `NUMBER_OF_BITMAP_FORMATS`, `_bitmap_type_2d` and `_bitmap_cached_bit`.
2. `texture_cache_debug_bitmaps` keeps its descriptive name. The map atlas is code-only, and HCEX has only the PC
   cache. Reopen if a data-name source appears.
3. `compare` keeps its `struct bitmap_data *` parameters and the `(boolean (*)(long, long))` cast at the
   `qsort_4byte` call. This predates the packet and is exact. The actor_firing_position and structure_render compare
   procs take `(long, long)` instead. An owner may prefer that spelling; it was not measured.
4. Other units still carry consumer-local prototypes of this file's functions: cache_files.c (open/close), hs.c
   (flush) and hardware_bitmaps.c (bitmap_delete). That debt belongs to those units, not to this object.
   Any further texture_cache.h prototype needs its own sweep: two added void prototypes were inert, but debug_render plus either format prototype broke decals.
