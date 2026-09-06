# Bitmaps rasterizer-owner and stronger-packet audit (read-only support)

## Subsequent canonical trial

The exact lifecycle packet below was tested once after the Bitmaps publication
at `e50f98a194e0bac4d3f85c2fe2d8aec76b0b9f1c`. It regressed one 392-byte
Decals function and was completely restored, with all 3,632 runtime sections
across 48 consumers unchanged again. Do not repeat it as an untried proposal.
See `bitmaps_obj_lifecycle_owner_trial_rejected_20260905.md`. The stronger-11
bitmap packet remains admitted as nine credited gains; rebuild remains held.

## Conclusion

The genuine existing owner for the three hardware-bitmap lifecycle APIs is
`source/rasterizer/rasterizer.h`.  It is the public rasterizer boundary, already
forward-declares `struct bitmap_data`, groups APIs by their implementing
translation unit, and is already included by both the defining
`rasterizer_xbox_hardware_bitmaps.c` and the `rasterizer_text.c` caller.  The
current declarations in `rasterizer_text.h` are caller-owned, not
implementation-owned.  A newly invented narrow header has no January or HCEA
header provenance and would require an include edit in the Opus-reserved
hardware implementation to satisfy normal self-declaration discipline.

The smallest proper lifecycle owner packet is therefore:

1. Add a `prototypes/RASTERIZER_XBOX_HARDWARE_BITMAPS.C` group to
   `rasterizer.h` containing exactly:

   ```c
   boolean rasterizer_bitmap_new(
       struct bitmap_data *bitmap);
   void rasterizer_bitmap_changed(
       struct bitmap_data *bitmap);
   void rasterizer_bitmap_delete(
       struct bitmap_data *bitmap);
   ```

2. Remove `rasterizer_bitmap_new` and `rasterizer_bitmap_changed` from
   `rasterizer_text.h`.
3. Include `rasterizer/rasterizer.h` from `bitmaps.c`; remove its local
   `rasterizer_bitmap_changed` and `rasterizer_bitmap_delete` declarations;
   never add the Fable local `rasterizer_bitmap_new` facade.
4. Do not edit either rasterizer `.c` file.  Both relevant rasterizer TUs
   already include `rasterizer.h`.

This is a separate, held header wave.  It has no direct path collision with
Opus's reservation of `source/rasterizer/**/*.c`, but it causes 22 reserved
rasterizer objects to rebuild and therefore needs a frozen dependent sweep.

## Signature and ABI evidence

The canonical definitions are at
`source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c:155`, `:680`, and
`:694`; the misplaced declarations are at
`source/rasterizer/rasterizer_text.h:30-33`; and the local changed/delete
shadows are in `source/bitmaps/bitmaps.c` under its prototype block.

Direct `DIA2Dump.exe -sym` queries against `HCEX_Release.pdb` report each API
as a public function with one `struct bitmap_data *bitmap` parameter and
`uses custom calling convention: false`.  The immutable HCEA C extraction
independently gives `uint8_t` for `rasterizer_bitmap_new` and `void` for changed
and delete, agreeing with canonical `boolean`/`void`.  January i386 COFF uses
plain `_rasterizer_bitmap_*` decoration rather than stdcall `@4` decoration.
Together these prove the ordinary C/default-`__cdecl`, one-pointer ABI above.

Evidence identities:

- `DIA2Dump.exe`: SHA-256
  `5be3b890fb5c3b7c74b7c3ce7c275a70f48813c388d3611c41e69c0ac0626243`.
- `HCEX_Release.pdb`: SHA-256
  `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`.
- HCEA `rasterizer_bitmap_new.c`: SHA-256
  `b3e632003f522aa285b9261ee46fa95318278dcf951f808dfa5c9a679dc34593`.
- HCEA `rasterizer_bitmap_changed.c`: SHA-256
  `51c0b44b4e9b29b23f6a6b6881a60c44c2dbba33340119a46bd5235ee10b3791`.
- HCEA `rasterizer_bitmap_delete.c`: SHA-256
  `e2bb88391ca96fa595d43eb7278d5a46371f899399fe1a9de852fb6cf0c9c992`.
- Canonical `rasterizer.h`: SHA-256
  `5e7f9d72722ccff891460ced098a8a3fcf2403a52a617e3932baa8e356cb9726`.
- Canonical `rasterizer_text.h`: SHA-256
  `c35ba332ec843c3f616201d86559ede3877293eb826758b22101bfc667387e47`.
- Canonical hardware implementation: SHA-256
  `23d3540bfe0e807a27d7f7dc34608cbdc4dc0c4379b609ecf78730bd3df0bc4e`.

## Exact lifecycle-header consumer closure

The read-only `ninja -t deps` census was taken with `.ninja_deps` SHA-256
`372b41eb84ea815f7b4fa17ef7ec7d7ca1f98d30b77aee65d429103ce01e356f`.
`rasterizer.h` has 47 existing base-object consumers.  Adding the direct
`bitmaps.c` include makes 48:

- 22 rasterizer objects: `common/rasterizer_common`, `rasterizer`,
  `rasterizer_debug`, `rasterizer_frame_statistics`, `rasterizer_lights`,
  `rasterizer_memory_pool`, `rasterizer_text`,
  `rasterizer_transparent_geometry`, `xbox/rasterizer_xbox`,
  `xbox/rasterizer_xbox_active_camouflage`, `xbox/rasterizer_xbox_decals`,
  `xbox/rasterizer_xbox_draw_primitives`, `xbox/rasterizer_xbox_dynavobgeom`,
  `xbox/rasterizer_xbox_environment`,
  `xbox/rasterizer_xbox_environment_fog`,
  `xbox/rasterizer_xbox_hardware_bitmaps`, `xbox/rasterizer_xbox_models`,
  `xbox/rasterizer_xbox_profile`, `xbox/rasterizer_xbox_screen_effect`,
  `xbox/rasterizer_xbox_transparent_geometry`,
  `xbox/rasterizer_xbox_water`, and `xbox/rasterizer_xbox_widgets`.
- 25 existing non-rasterizer objects: `ai/ai_debug`, `ai/ai_profile`,
  `cache/cache_files_windows`, `cseries/profile`, `cutscene/cinematics`,
  `effects/decals`, `game/game`, `game/game_engine_king`, `interface/hud`,
  `interface/terminal`, `interface/ui_widget`, `main/main`, `objects/damage`,
  `objects/object_lights`, `objects/widgets/light_volumes`,
  `objects/widgets/lightning`, `physics/collision_usage`,
  `render/render`, `render/render_contrails`, `render/render_debug`,
  `render/render_sprite`, `saved games/game_state`, `shell/shell`,
  `structures/structure_detail_objects`, and `structures/structure_render`.
- New direct consumer: `bitmaps/bitmaps`.

The two current `rasterizer_text.h` consumers (`rasterizer_text` and
`rasterizer_xbox`) are already members of the 47, so removing the misplaced
declarations adds no further unit.

## Mipmap pixel-count owner distinction

`source/bitmaps/bitmaps_mipmap.h` is the established narrow owner.  Adding
`bitmap_mipmap_get_pixel_count(struct bitmap_data *bitmap, short
mipmap_index)`, including the header from `bitmaps.c`, and removing the local
duplicate has exactly three Ninja consumers: `bitmaps/bitmaps`,
`rasterizer/rasterizer_swizzle`, and
`rasterizer/xbox/rasterizer_xbox_hardware_bitmaps`.

The later HCEA PDB and HCEA C extraction spell the pointer `const struct
bitmap_data *`.  That is useful provenance, but it is not January source/PDB
authority.  The retained non-const declaration is ABI-identical and preserves
the current non-const same-TU and sibling-mipmap convention; no qualifier
variant should be introduced for code generation.  The frozen owner-only
trial reports all three runtime-section sets unchanged and the full exact board
stable.

## Stronger-11 prerequisite and separation matrix

| Function group | Linkage in frozen object | Existing owner/prerequisite | Separation result |
|---|---|---|---|
| width/height/depth validators | private, storage 3 | local static declarations | Included with constructors; depth is uniquely owned, width/height remain uncredited aliases. |
| `bitmap_2d_new`, `bitmap_3d_new`, `bitmap_cube_map_new` | public, storage 2 | `bitmaps_internal.h`; `bitmap_verify` from `bitmaps.h`; allocation/memory/error APIs from existing cseries owners | No lifecycle dependency; exact while `bitmap_verify` remains an undefined external. |
| `bitmap_2d_address` | public, storage 2 | protected `bitmaps.h` already declares it | No header edit needed. |
| `bitmap_3d_address` | public, storage 2 | `bitmaps_internal.h` | No header edit needed. |
| `bitmap_cube_map_address` | public, storage 2 | `bitmaps_mipmap.h` | No header edit needed. |
| `bitmap_mipmap_address` | public, storage 2 | pre-existing duplicate declarations in `bitmaps_internal.h` and `bitmaps_mipmap.h` | Duplicate-owner debt is inherited; do not broaden this packet to clean it. |
| `bitmap_mipmap_get_row_pitch` | public, storage 2 | `bitmaps_mipmap.h`; calls existing/unwritten `bitmap_mipmap_get_width` | Exact with the callee left as an ordinary undefined external. |
| `bitmap_rebuild` (not in stronger-11) | public, storage 2 in January/Fable final | no current cross-TU caller or bitmap header; calls `rasterizer_bitmap_new` and `rasterizer_bitmap_changed` | Separately admissible only after the lifecycle owner wave; it is the sole stronger-tier function that introduces `rasterizer_bitmap_new`. |

Current local-call census and the independent January undefined-reference
census agree on all public members.  The January callers are:

- mipmap address: `bitmap_extract`, `bitmap_utilities`, `rasterizer_swizzle`,
  hardware bitmaps;
- row pitch: `xbox_texture_cache`, `rasterizer_swizzle`;
- 2D new: `bitmap_extract`, `bitmap_utilities`, `tiff_file`, `main`,
  `rasterizer_text`;
- 3D new: `bitmap_extract`, `bitmap_utilities`;
- cube new: `bitmap_extract`, `bitmap_utilities`;
- 2D address: `bitmap_drawing`, `bitmap_extract`, `bitmap_utilities`,
  `bitmaps_quantitize`, `targa_file`, `tiff_file`, `rasterizer_text`,
  `rasterizer_xbox`;
- 3D address: `bitmap_extract`, `bitmap_utilities`;
- cube address: `bitmap_extract`, hardware bitmaps; and
- rebuild: none.

The existing C-local shadows in `targa_file.c`, `tiff_file.c`, and `main.c`
are outside this packet.  The Fable donor's local rasterizer declarations are
also excluded.

## Independent stronger-11 object result

The immutable audit inputs are baseline
`c94745ea1e0496d44923ba34302071e44b02bbdf0089a0ba8934d4211309192b`,
first scratch candidate
`25a902f1bc176bf445454895ecf2b7b17417bfa84722ae7fb0a4cf23ae30918e`,
final actual object
`dcbf76839ef0a07669a29d5269efd2dcaf9db728bf290910bde460a859588313`,
and final target
`95c1f3f022bef7a5828ccb64ea526e809ef76cc6f9c2844573279297909eea0a`.

The independent audit proves:

- all nine baseline functions and all 40 baseline sections are unchanged;
- first versus final has the same complete section set and every non-debug
  section is exact (only `.debug$S` differs);
- growth is exactly 11 code COMDATs, 24 rdata COMDATs, and two `.debug$F`
  sections, with no lost section, COMMON, BSS, new writable runtime owner, or
  `point_from_line3d` symbol;
- the eight public functions plus private depth validator are strict target
  exact, including symbol storage/type/value, section flags `0x60501020`, and
  NODUP selection 1;
- width and height are byte/relocation exact against the two generic target
  sections, but remain uncredited because both are 32 bytes, have zero
  relocations, are mutually indistinguishable, and their target aliases retain
  unresolved external storage;
- 20 of 24 new rdata owners are direct target-object matches; the four
  candidate-object-only ANY literals are the three bitmap-type assertions and
  unsupported-type assertion, all ordinary pooled owners found elsewhere;
- new undefined symbols are exactly `_bitmap_mipmap_get_width`, `_csmemset`,
  `_debug_malloc`, and `_error`, each explained by genuine calls; and
- the only config change is `0x6BAB0` from `_code_0006bab0` to private/static
  `_bitmap_format_type_valid_depth`.

Audit artifacts:

- `scratch/bitmaps_stronger_natural_independent_object_audit_support_20260905.py`
  — SHA-256
  `07e155d8aedbe7a2410e68da0843fa21c732bbfb0b7cf97d816f92995bb3032d`.
- `scratch/bitmaps_stronger_natural_independent_object_audit_support_20260905.json`
  — SHA-256
  `e2644eefdf90588d530a84bb0f76f4737640c3e84a6e6460f46b6daaa40b1bb3`.

Disposition: stronger-11 has no all-section or owner blocker.  Credit is nine
functions; the two ambiguous private sections are real emitted code but not
individually authenticated target owners.  Rebuild and the 48-unit lifecycle
header migration remain a separate wave.
