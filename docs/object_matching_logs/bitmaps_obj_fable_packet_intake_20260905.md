# Bitmaps Fable packet intake (read-only support audit, 2026-09-05)

## Disposition

The committed Fable `source/bitmaps/bitmaps.c` packet contains **18 distinct strict object matches over canonical's existing 9**, totaling **4,736 padded bytes**. The object evidence is real, but the packet is **not admission-ready as one natural packet**. It lacks its cited bitmap header note, retains a C-local declaration for the foreign public `rasterizer_bitmap_new` API, assigns two indistinguishable static helper aliases by inferred order, and includes several functions whose exact bodies followed documented target-guided corrections (including eight variants for `bitmap_verify`).

The safe next review unit is therefore a frozen, staged source packet, not a wholesale cherry-pick. Keep protected `source/bitmaps/bitmap_utilities.c` and `source/bitmaps/bitmaps.h` untouched. First authenticate the missing public prototype in its narrow owner `source/bitmaps/bitmaps_mipmap.h`, resolve the genuine owner surface for `rasterizer_bitmap_new`, and separate the stronger first-pass subset from the explicitly tuned functions.

No source/header/build/config file was edited and no compile, gate, or park operation was run for this audit.

## Frozen identity and dirt

- Canonical: `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`
  - HEAD: `31a1927c951479106d0475ee99e1fb236a15e89d`
  - January target `build/split/source/bitmaps/bitmaps.obj`: 39,678 bytes, SHA-256 `6439102f24b216ed618a8c13e06d7c51b601a66214a6c17130ccb5586e41f042`
  - current canonical `build/source/bitmaps/bitmaps.obj`: 9,413 bytes, SHA-256 `de6ca5e983ea03c75cf0d7446207538825844c4e9de50765517e5287607e32c1`
- Fable: `C:/halo-worktrees/fable-50k-small-families-20260904`
  - HEAD: `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`
  - committed packet changes only `source/bitmaps/bitmaps.c` under `source/bitmaps` (`+543/-10`); no bitmap header is committed
  - committed/current bitmap source: 33,657 bytes, SHA-256 `ead2c4f5e80e5ff3668fcd9bcbe7aa759ec3b7f9d925ae4e72c4b43b4d331a0f`, git blob `4ab5acc7ae274687a63ccd7cbd424fe136c85aeb`
  - parent blob: `f9f114820d9028782cf0ccc359aa15ce91ae983b`; canonical has the same pre-packet content modulo line endings
  - final candidate object: 24,700 bytes, SHA-256 `dba76b32d4cf4c4292e299282b49788e34b0d12876ffd0544b7d6b3017531e25`
  - Fable target object: 39,730 bytes, SHA-256 `63435e13ed14ff4f0de291956f4a59849fe32bc529869c781de6a9709ed2eda7`
  - tracked Fable `source/bitmaps/` is clean. Other dirty Fable paths (`config/symbols.json`, AI/effects/interface/structures/tool sources, and scratch) are outside this committed bitmap packet and were not used as packet content.

The Fable report cites `scratch/hdr/bitmaps.md`, but exhaustive bitmap-named scratch inventory found no such file. This is a concrete missing provenance artifact, not merely an untracked header edit.

## Exact gains over canonical

Every row below is a hardened all-section match (`section_infos_equal=True`), including ordered symbolic relocations. Sizes are padded target sizes.

| Candidate symbol | Canonical target identity | Padded bytes / relocations | Normalized hash |
|---|---|---:|---|
| `_bitmap_format_type_valid_width` | `_code_0006ba70` | 32 / 0 | `9c186b7da111114d12055376cee720362e4c2c187874effe3cced0da106d76f1` |
| `_bitmap_format_type_valid_height` | `_code_0006ba90` | 32 / 0 | `9c186b7da111114d12055376cee720362e4c2c187874effe3cced0da106d76f1` |
| `_bitmap_format_type_valid_depth` | `_code_0006bab0` | 48 / 0 | `75a481c693ac7721c67910f76259d485877d2484e0c7d6f0b181d96042d317b4` |
| `_bitmap_rebuild` | `_bitmap_rebuild` | 128 / 11 | `3e6fd87fe17484f181ec6e3f795d4584b2df5fcad4ee728fa39d08f32ecd94ac` |
| `_bitmap_mipmap_get_depth` | same | 144 / 9 | `9a864fce4b366f198a5f4b9cfdb1d2df5753fb5c12473a8c9865c746f5eb4ff4` |
| `_bitmap_mipmap_get_pixel_data_size` | same | 144 / 11 | `49991b5f859b2586d8376832cb0e9a9e9ed81950130a7cbf3de1a18c72f29457` |
| `_bitmap_mipmap_get_width` | same | 160 / 9 | `23d665ecb68ff9891e6bc9e06ceb67e57b1c33093164e19cd221c99ee4ecaf2f` |
| `_bitmap_mipmap_get_height` | same | 160 / 9 | `832b9bfe3bb3f5d40095d322069ed2ab8255ad6fe4cc5fee6d2ad48ba382f686` |
| `_bitmap_mipmap_get_pixel_count` | same | 176 / 12 | `ad3adfeb2cdfb5185c6b515b40fc6ee5a3bcb05f974319280aba43352ae988f4` |
| `_bitmap_mipmap_address` | same | 208 / 15 | `1f23ac477450722c3652ca7881f25452a7a8819f660f516d106b1c6385401d16` |
| `_bitmap_mipmap_get_row_pitch` | same | 224 / 19 | `2bd2fa900e7f0b7a6680b4aa66345616606e3d22df260c48fb9619ec422c7541` |
| `_bitmap_verify` | same | 352 / 10 | `46bb936937128d116aa5eef083323407df4caf6fce00ca85ff1ddcc546f5c49d` |
| `_bitmap_cube_map_new` | same | 368 / 23 | `c5813e5fab4c269cf106bc74a63892fdefa8a96715a08980e669ca0c78edcecd` |
| `_bitmap_2d_new` | same | 384 / 23 | `68479ae7014792a3394f7ecf09e4bf4d07fda2ef53575f98765c3e74f64723cd` |
| `_bitmap_3d_new` | same | 448 / 27 | `c0e0185511ef75dcb7ffd898be4d1b85dc28df147625aa74ea9117f4c77767b7` |
| `_bitmap_cube_map_address` | same | 528 / 33 | `43ae8a218be87e73ca8b99320a75588a3fa17e7431b1dcd5510174180d02ff63` |
| `_bitmap_2d_address` | same | 544 / 33 | `e05dbed80aad3c4fe4f6f38ea9cd93fee9962dac42e6bb59e57b630a6296853e` |
| `_bitmap_3d_address` | same | 656 / 37 | `f837a023753386d82cd1b0d4ace9d01fe337e4155945e08e31c4ada3a4452f86` |

The nine canonical baseline matches remain strict and all-section stable in the Fable candidate: `bitmap_type_get_string`, `bitmap_format_get_string`, `bitmap_changed`, `bitmap_delete`, `bitmap_byte_swap`, `bitmap_get_max_mipmap_count`, `bitmap_get_pixel_count`, `bitmap_get_pixel_data_size`, and `bitmap_format_get_bits_per_pixel`.

The final Fable accounting is 27 exact, 0 residual, 7 unwritten, or 5,600 padded exact bytes in total. Subtracting canonical's existing nine leaves precisely the 18 functions and 4,736 padded bytes above. The seven unwritten functions are not packet gains: `palette_find_closest_match`, `bitmap_format_to_a8r8g8b8`, four slice/face functions, and `bitmap_2d_get_pixel`.

## Linkage, sections, literals, and inherited owners

- The three validator helpers are static (`storage=3`); the other 15 new functions are external (`storage=2`). All are `type=32`, `value=0`, `.text` COMDAT NODUP (`selection=1`, flags `0x60501020`).
- There is no COMMON symbol in canonical baseline, Fable candidate, or target, and the packet adds no writable `.data`/BSS owner.
- Candidate runtime growth is exactly 44 COMDAT sections: 18 code and 26 `.rdata`. It also adds two debug sections. No baseline runtime section is lost.
- Of the 26 packet-added `.rdata` COMDAT ANY owners, 22 have exact target sections. Four are ordinary candidate-emitted/coalescible assertions absent as definitions from this target object: `bitmap->type==_bitmap_type_3d`, `bitmap->type==_bitmap_type_2d`, `bitmap->type==_bitmap_type_cube_map`, and `### ERROR unsupported bitmap type`. They are compiler/literal owners, not independent target data credit.
- `_global_vector_palette` remains strict-exact (1,116 padded bytes / 21 relocations; hash beginning `443063…`). Apparent symbol-table ownership movement does not change its bytes or ordered relocations.
- `_bitmap_verify` and `_bitmap_mipmap_get_pixel_count` transition undefined to defined. Three inherited functions (`bitmap_get_max_mipmap_count`, `bitmap_get_pixel_count`, `bitmap_get_pixel_data_size`) retain identical bytes/relocations; only their target-symbol metadata changes because calls become same-object definitions.
- Newly required undefined externals are `_error`, `_rasterizer_bitmap_new`, `_csmemset`, and `_debug_malloc`, each explained by genuine packet calls. There is no unexpected new code helper beyond the 18 listed functions.

## Helper identity caveat

Canonical intentionally retains generic target names `_code_0006ba70`, `_code_0006ba90`, and `_code_0006bab0`. Depth is uniquely distinguishable: its 48-byte body differs and `bitmap_verify` relocates to `_code_0006bab0`.

Width and height are each 32 bytes, have no relocations, are byte-identical, and are not independently called from another target section. The Fable mapping `ba70=width`, `ba90=height` follows source/assertion order. That is a plausible inference, but not a unique object-level bijection. Do not import those two `config/symbols.json` aliases as independently authenticated facts.

## Genuine API owner and caller closure

`source/bitmaps/bitmaps_mipmap.h` is the narrow existing owner. It already declares mipmap address, cube-map address, width, height, depth, pixel-data-size, and row-pitch, but omits the public function implemented by this packet. The source-authentic declaration is:

```c
long bitmap_mipmap_get_pixel_count(
    struct bitmap_data *bitmap,
    short mipmap_index);
```

It belongs after depth and before pixel-data-size in `bitmaps_mipmap.h`. A future staged patch should include that header from `bitmaps.c` and remove the C-local declaration. It must not broaden `bitmaps.h`.

Direct current consumers of `bitmaps_mipmap.h` are:

- `source/rasterizer/rasterizer_swizzle.c`
- `source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c`

Adding the owner TU gives an initial three-TU header blast; a complete admission still needs the ordinary all-section/COMMON dependent sweep.

January undefined-reference caller census for the 15 external additions:

| Function | January cross-TU callers |
|---|---|
| `bitmap_rebuild` | none |
| `bitmap_mipmap_get_depth` | `rasterizer_swizzle`, `rasterizer_xbox_hardware_bitmaps` |
| `bitmap_mipmap_get_pixel_data_size` | same two |
| `bitmap_mipmap_get_width`, `bitmap_mipmap_get_height` | `bitmap_utilities`, `rasterizer_swizzle`, `rasterizer_xbox_hardware_bitmaps` |
| `bitmap_mipmap_get_pixel_count` | none (same-TU call from `bitmap_get_pixel_count`) |
| `bitmap_mipmap_address` | `bitmap_extract`, `bitmap_utilities`, `rasterizer_swizzle`, `rasterizer_xbox_hardware_bitmaps` |
| `bitmap_mipmap_get_row_pitch` | `xbox_texture_cache`, `rasterizer_swizzle` |
| `bitmap_verify` | `bitmaps_quantitize`, `bitmap_extract`, `bitmap_group`, `bitmap_utilities`, `rasterizer_swizzle` |
| `bitmap_cube_map_new` | `bitmap_extract`, `bitmap_utilities` |
| `bitmap_2d_new` | `bitmap_extract`, `bitmap_utilities`, `tiff_file`, `main`, `rasterizer_text` |
| `bitmap_3d_new` | `bitmap_extract`, `bitmap_utilities` |
| `bitmap_cube_map_address` | `bitmap_extract`, `rasterizer_xbox_hardware_bitmaps` |
| `bitmap_2d_address` | `bitmaps_quantitize`, `bitmap_drawing`, `bitmap_extract`, `bitmap_utilities`, `targa_file`, `tiff_file`, `rasterizer_text`, `rasterizer_xbox` |
| `bitmap_3d_address` | `bitmap_extract`, `bitmap_utilities` |

The target census is object authority; some target callers remain unwritten in current source. Existing local shadows in `tiff_file.c`, `targa_file.c`, and `main.c` should not be swept into this packet.

`bitmap_rebuild` is externally linked but has neither a current declaration nor a current/January cross-TU caller. Its eventual public/internal owner is unresolved and need not be invented for this isolated implementation.

The Fable source adds a local declaration for `rasterizer_bitmap_new`. That is a foreign public facade. Current definitions live in `source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c`; the only shared declaration found for `rasterizer_bitmap_new`/`rasterizer_bitmap_changed` is misleadingly housed in `source/rasterizer/rasterizer_text.h`, while delete has no common owner. This owner gap must be adjudicated rather than perpetuating another C-local prototype.

The type/format/flag constants added by Fable are TU-private. Other TUs also duplicate subsets, and no authenticated shared enum owner currently exists. HCEA corroborates their values, but they should not be promoted into protected `bitmaps.h` as part of this intake.

## Natural/provenance classification

The final exact object is not a retained first-natural candidate. Both scratch `ng_bitmaps.obj` and `w_bitmaps.obj` already contain all 27 exact bodies, while `w_bitmaps_v1` through `v9` source artifacts demonstrate experimentation without a frozen first-natural object/inventory.

Specific conflicts with a clean natural claim:

- Canonical `docs/object_matching_logs/bitmaps_obj_jonas_max_mipmap_leaf_20260821.md` froze earlier rejection evidence for width, height, depth, and pixel-count. The Fable report records corrected `MAX` polarity for width/height/depth and corrected call order for pixel-count; these reuse already measured boundaries.
- Pixel-data-size was first 160 bytes and corrected after a prototype issue.
- `bitmap_verify` was measured across eight variants, exceeding the worker's stated bounded-variant rule. It is an exact reconstruction, not a natural first candidate.
- The report's batch heading “first compile + one bounded correction” is therefore not a reliable function-level provenance description.

The strongest reported tier, without elevating worker prose to proof, is the 12-function / 3,600-padded-byte subset with no recorded second-try marker: the three validators, rebuild, mipmap-address, row-pitch, three constructors, and three address functions. Address trio is explicitly reported exact on first compile. The six remaining functions (width, height, depth, pixel-count, pixel-data-size, and verify) total 1,136 padded bytes and carry explicit correction/tuning concerns.

HCEA sources support semantics, types, and names, and Stian supports later Xbox disassembly reconstruction; neither is authentic January PC source authority. They cannot erase the preserved canonical rejection history.

## Safe staged next action for the owner

1. Freeze the source fragment and inventories before adapting it.
2. Trial only the missing `bitmap_mipmap_get_pixel_count` declaration in `bitmaps_mipmap.h`, include that owner header in `bitmaps.c`, and remove the local duplicate. Blast the two existing consumers plus owner, then run the full all-section/COMMON dependent check.
3. Resolve `rasterizer_bitmap_new` in a genuine rasterizer owner surface; do not add another foreign C-local facade.
4. Keep `bitmaps.h` and `bitmap_utilities.c` untouched. The prior selective-donor ledger records that broad `bitmaps.h` expansion changed protected Units (189 to 188).
5. Review the stronger 12-function tier separately from the six target-guided functions. Do not use the final 27-exact object as first-natural evidence.
6. Retain generic aliases for the indistinguishable width/height helpers unless another direct symbol source authenticates their address-to-name ordering.
7. Leave the seven unwritten residuals and the separate `bitmap_drawing.c`/`tiff_file.c` pending lanes outside this packet.

## Evidence read

- Fable `scratch/reports/bitmaps.md` — SHA-256 `d8a0b71d81cf1bfb7edd4aea6dc5006612e0e7f5cf44b96f80bb42d45cbdce5f`
- Fable `scratch/facts/bitmaps.md` — SHA-256 `4712f9f4e9c85eb02e88e35a60cf14230b4f9b07969b5b011544e126bbc1addd`
- Canonical `docs/object_matching_logs/fable_50k_small_families_interrupted_lane_intake_20260905.md` — SHA-256 `788f18e1b6303604e8be6ca8f82838f0f813e59d926073c3b18d14fd7fc07125`
- Canonical `docs/object_matching_logs/bitmaps_obj_jonas_max_mipmap_leaf_20260821.md` — SHA-256 `d70b30…`
- Canonical `docs/object_matching_logs/bitmap_utilities_obj_selective_donor_reconciliation_20260831.md` — SHA-256 `c62522…`
- Current canonical/Fable bitmap sources and headers, Fable commit diff, scratch object inventories, January target symbol/relocation census, and HCEA/Stian bitmap source evidence.
