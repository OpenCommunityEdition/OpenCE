> Research-only: zero new exact credit; no production changes.

# Fog owner primary-proof audit — 2026-09-20

**Disposition: hold the 21-static proposal. No new source shape, symbol/config change, gate, or exact credit.** The raw January evidence confirms the fog module's 661-byte private storage contribution and its references. Fresh evidence does not authenticate 21 separate C declarations. The Sept14 w1 result remains a compiler/alias hypothesis, not a newly proved original identity partition. This packet is a negative boundary, not coverage progress.

Scope: `source/rasterizer/xbox/rasterizer_xbox_environment_fog.c`, baseline HEAD `11746616760fffd6066354ffb7e5e0077eac06e6`. Production and configuration are unchanged. IDA_AUTOMATION_UNAVAILABLE; read-only raw PDB/PE and DIA inspection used the authorized fallback. No compiler, Ninja, split generation, source candidate, or configuration candidate was run.

## Evidence newly collected

`owner-proof.json` records hashes, raw record bytes, every reference, component offsets, consumer inventory, and all 24 exact controls. `owner_proof.py` reproduces the read-only audit directly from supplied originals and existing objects.

- January `cachebeta.pdb`, SHA-256 `8480f0c44fc7b5acba5775c02053d1a62c6794ab8d646661106985cbe46d7bc5`: module index120, stream129, `\halo\objects\halobetacache\rasterizer_xbox_environment_fog.obj`. Its 140-byte symbol stream consists only of S_OBJNAME (0x0009) and S_COMPILE2 (0x1013). There are no data, procedure-local, lexical-block, source-file, or line records in this module. Parsing the original global-symbol stream finds no symbol within the range, or the surrounding 64 bytes.
- Its original DBI section contribution is section20, offset0x1AF050, RVA/file offset0x465AD0, size0x295 (661), flags0xC0400080, module120. Raw record: `1400000050f01a0095020000800040c0780000000000000000000000`. This is one **module contribution**, not a record declaring a single C object. It contains no individual component boundaries.
- The original PE has 77 HIGHLOW base relocations whose stored addresses fall in VA0x865AD0..0x865D64 inclusive. All 77 relocation sites belong to code contributions from this same fog module. All 77 correspond to the existing reconstructed target object's fog-owner relocations. No references from another raw provider module were found. Base relocations prove referenced addresses, not the storage duration, declaration identity, complete extent, or type of each destination.
- Fresh HCEA DIA `-m`, `-g`, and full `-compiland` output was collected. HCEA contains `rasterizer_dx9_environment_fog.obj`, not this Xbox implementation. Screen initialize/model-begin are 8-byte stubs; screen window begin/end, dispose, screen begin/draw/end, wind-get-vector, model-submit/end are 4-byte stubs. Fog-module file statics are `eye_distance_to_fog_plane` (float), `planar_eye_density` (float), and a header's `anisoFilterMap`. None describes the January 21-component allocation.
- The global-table hit `local_environment_fog_screen_flag` is **not** owned by HCEA fog: fresh `rasterizer_dx9_models.obj` records it as a file-static unsigned char at RVA0x246C40F. Its ownership is preserved in `hcea-models-compiland.txt`. Treating the reconstructed `blam_data_globals.h` global list as a fog declaration donor would misattribute it.
- The supplied `haloleak2024/halo1` filename census found fog assets, not an original Xbox fog C implementation, object, library, or PDB. Existing upstream/reconstructed copies are not first-party declaration evidence. No broader claim of source absence is made.

## Complete component map

These are the **current source's semantic components**, checked against all original raw relocation destinations. Extents/types here describe the reconstruction and semantic array use, not recovered January type records. No original static declaration name was recovered. The two names `local_fog_eye_density` and `local_fog_pass` are independently present in January assertion literals. Exact spelling of the remaining names is not required by the user, but that permission does not prove declaration boundaries.

| Offset | Extent | Current semantic component | Raw refs |
|---:|---:|---|---:|
| 0x000 | 2 | cached_node_matrix_count, short | 2 |
| 0x004 | 4 | cached_node_matrices, matrix pointer | 2 |
| 0x008 | 208 | previous_camera_matrix[4] | 3 |
| 0x0D8 | 1 | local_environment_fog_screen_model_flag | 6 |
| 0x0D9 | 1 | local_environment_fog_screen_flag | 5 |
| 0x0DC | 8 | local_fog_screen_layer_bitmap_indices[4] | 3 |
| 0x0E4 | 48 | local_fog_screen_layer_colors[4] | 5 |
| 0x114 | 4 | local_fog_eye_density | 5 |
| 0x118 | 2 | local_fog_pass | 6 |
| 0x120 | 304 | windows[4], stride0x4C | 5 |
| 0x250 | 4 | opaque_model_submit_parameters | 4 |
| 0x254 | 4 | opaque_model_count | 8 |
| 0x258 | 4 | fog_screen_active[4] | 6 |
| 0x260 | 32 | last_frame_index[4] | 2 |
| 0x280 | 2 | atmosphere_dominant_warning_count | 2 |
| 0x282 | 1 | reported_bad_animation_index | 2 |
| 0x284 | 4 | model | 4 |
| 0x288 | 1 | model_parameters_cached | 3 |
| 0x28C | 4 | cached_lighting | 1 |
| 0x290 | 4 | cached_animation | 1 |
| 0x294 | 1 | reported_too_many_opaque_models | 2 |

The 19 currently modeled padding bytes are offsets0x002–003, 0x0DA–0DB, 0x11A–11F, 0x25C–25F, 0x283, and 0x289–28B. No raw reference in this census lands in them. The last referenced byte is0x294, matching the contribution end. The JSON lists each relocation site and within-component addend; nothing is hidden behind a changed comparator or rebased candidate symbol.

## Interpretation and prior-work boundary

The current ordinary aggregate emits 664 bytes; January's contribution has661. That independently establishes that the current normally aligned singleton is not a literal reproduction of the original complete allocation. It supports reopening the ownership question. It does **not** force exactly21 independent C objects: multiple aggregates, separate address-taken arrays plus groups of non-address-taken values, and other partitions remain possible. This audit does not test any of them.

Sept14 w1 P1 already demonstrated that splitting all21 fields changes alias assumptions and yields exact screen-active; model-submit also needs the documented declaration-order stores. Its scratch split target reported26/27 exact, with no data credit. Those old measurements are retained as historical evidence only. Repeating them would not add the missing original declaration proof.

The fresh target/base aligned disassemblies preserve the two relevant machine-code facts: January moves the frame-index comparison before the indexed active-byte clear and loads the active byte before storing eye density; model-submit hoists cached count/lighting loads across group stores. These observations support restricted aliasing. They do not uniquely recover all21 storage identities. No source or config probe is justified under the current request for a new primary fact beyond that known hypothesis.

Read prior histories: Sept03 exact reconciliation; Sept09 screen-end reconciliation; Sept12 four-function re-audit; Sept13 window-owner reconciliation; Sept14 opus5 100K and 150K w1. The prior synthetic `_rasterizer_environment_fog_screen_globals` name was explicitly a reconstruction of a private range with no public PDB name. Neither it nor the21 proposed replacements is an original symbol merely because csplit emits it.

## Consumers, controls, and remaining evidence

A fresh inventory of all833 existing split objects finds the synthetic owner only in the fog object. The production C/H source inventory likewise finds its owner only in the fog C file. The independent raw PE provider census above corroborates TU privacy without relying on that reconstructed symbol name.

Read-only comparison of the existing target/base objects gives24 exact controls and3 residuals, matching the old baseline. `owner-proof.json` lists each control's name, size, relocation count, full normalized hash, and candidate descriptor. They include both window leaves, initialization/disposal, model-end, fog begin/draw/end, screen draw/end, model-begin, wind-update/get-vector, vector4d setter, random boolean, and nine D3D wrappers. No checks were rerun by compiling unchanged source.

Any future authentic split must preserve all24 exact owners, every helper/COMDAT owner, COMMON entries, and `_point_from_line3d` ownership; maintain every raw January destination and addend; explain data contribution boundaries without padding tricks or claiming nonexistent data equality; and demonstrate no other object changed under a complete split inventory. Source/header and symbol changes must describe an independently supported source identity, never redefine the target to fit code. Historical potential is507+277=784 meaningful bytes; **this audit earns zero**.

Concrete missing evidence: an original Xbox full-data-symbol/type artifact or actual owner declarations; alternatively, another independent first-party source record that establishes separate identity for the relevant address-taken arrays and scalar groups. HCEA DX9 stub topology cannot supply it. Stop at this boundary and retain current production.
