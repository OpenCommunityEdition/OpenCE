# rasterizer_xbox_models.obj - opus5 100K consolidated lane, wave C (2026-09-14)

## 1. Scope and provenance

- Translation unit: `source/rasterizer/xbox/rasterizer_xbox_models.c`
- Target: `build/split/source/rasterizer/xbox/rasterizer_xbox_models.obj`
- Compiler: XDK 3911 VC7 13.00.9254, repository flags (`/O2 /Oy- /DDEBUG /Dxbox`).
- Worktree: `C:\halo-worktrees\opus5-100k-consolidated-20260914`, lane HEAD `7a3bbb9c8`.
- Evidence used: January target COFF (code, relocations); a scratch `csplit` of `cachebeta.exe` with a
  proposed `symbols.json` (only this object differs from `build/split`, 832 of 833 split objects are
  byte-identical); HCEA `src/blam/rasterizer/_rasterizer_model_transparent_geometry_submit.c` (global
  names and topology only). No Ghidra export or January map-atlas file exists for this object; the
  January atlas jsonl names only five functions here and no BSS symbols.
- Targets of this wave: the three locked parks (scratch-only re-investigation per brief section 9).

## 2. Validated baseline

`gate.py ... --all --forbid-emitted-symbol _point_from_line3d`: **exact 11, residual 3, unwritten 0**.

| Function | January padded / relocs / sha16 | Baseline candidate |
| --- | --- | --- |
| `__rasterizer_model_draw` | 5,168 / 350 / `99d8b95f1ce01178` | 5,168 / 348 / `23ca80471861b904` |
| `__rasterizer_model_transparent_geometry_submit` | 1,296 / 83 / `7b84b560459ad2dd` | 1,344 / 93 / `8fb7c26befa120e1` |
| `_rasterizer_model_draw_environment_shader` | 3,104 / 235 / `d4c4ee8c3b174dad` | 3,104 / 235 / `5179768ee4ad9fc8` |

The candidate object also emits `_dot_product3d`, `_plane3d_distance_to_point` and `_vector_from_points3d`
COMDATs that the target does not own (baseline property, unchanged).

## 3. Result

**Nothing landed in the production file** (all three residuals are locked parks; the only strict-exact
result requires a `config/symbols.json` split). Final real-file gate is unchanged: exact 11, residual 3.

Scratch park-reopen candidate `scratch/workersC/rasterizer_xbox_models/park_reopen_submit.c`, gated with
`coff_compare.section_infos_equal` against the scratch split `scratch/workersC/rasterizer_xbox_models/split/`:
**12/14 strict exact** - all 11 baseline-exact functions stay exact and
`__rasterizer_model_transparent_geometry_submit` becomes exact (1,296 / 83 / `7b84b560459ad2dd`).
Against the current, unsplit target the same candidate shows submit as `[reloc-identity]` only.

## 4. Accepted shapes in the park-reopen candidate (each decoded from January bytes)

1. **The BSS aggregate is split into separate statics.** January reloads `local_parameters` only after
   CALLs (7 loads); the aggregate candidate reloads it after every store through `group->` (17 loads,
   the entire +10 relocation gap). VC7 treats the address-taken aggregate
   (`&rasterizer_models_globals.immediate_transparent_geometry_group`) as aliased by pointer stores;
   separate statics are not. HCEA declares these as separate globals with these names. The split alone
   gives 1,296 / 83 and 383 / 383 instructions.
2. `vector_from_points3d(&camera.position, &group->centroid, &relative_centroid)` and
   `group->z_sort = -dot_product3d(&relative_centroid, &camera.forward)`: January reuses the
   `lea edx,[ebx+0x74]` centroid pointer for all three loads (inline helper parameter) and keeps the
   k, j, i product order.
3. `sort_filth->group_index = NONE` is stored before the two NULL reference stores (January `0x4e2`).
4. Declaration position: with 13 statics, submit is exact only when at least seven file-scope
   declarations precede `immediate_transparent_geometry_group` (declaration-count law). Measured:
   group declared after `local_parameters` gives a tie (zero in `edi`, NONE in `esi`); after
   `local_parameters_queued_flag` or `local_model_effect_type` gives exact; `queued_flag` first with the group
   sixth gives the tie. The candidate uses a natural grouping - model state set by `_rasterizer_model_begin`
   first, then transparent-submission caches. **This order is not evidence-authenticated**; the January
   offset order lands on the tie side.

## 5. Experiment matrix (scratch)

| Step | Change | Submit size / relocs | Result |
| --- | --- | --- | --- |
| v0 | production copy | 1,344 / 93 | 388 vs 383 instructions |
| s1 | 13 statics (offset order) | 1,296 / 83 | 7 slot diffs |
| s2 | + helper spellings + NONE-first filth | 1,296 / 83 | zero/NONE register swap only |
| s3 | + HCEA-inverted camouflage if/else | same sha as s2 | inert |
| s4 | chained `previous= next= NONE` | worse (store order reversed) | reverted |
| s5 | first_triangle_index/lightmap order | same sha as s2 | inert |
| r2 / c2 / cand | declaration order, see 4.4 | 1,296 / 83 | **exact** |
| d1 | model_draw: separate subtract/point_from_line3d with rgb casts | draw 5,184 / 349, frame 0xb0 | subtract inlined; new COMDAT |
| d2 | model_draw: nested subtract in point_from_line3d argument | identical to d1 | inert |
| e1 | env shader: vector_from_points3d, plane3d_distance_to_point, argb perpendicular/parallel, rgb cc0_error | 71 -> 41 differing blocks | not exact; flips submit tie back |
| e_a1/e_a2 | `(boolean)TEST_FLAG` / raw `flags & FLAG()` for ALPHATESTENABLE | same sha as e1 | inert |
| e_b1..b4 | detail_map_scale chained / array copy / early / direct field | never January's `mov edx; mov ecx,edx` | reverted |
| e_c1 | `dot_product3d(&forward, &camera_to_model)` | same sha as e1 | inert |

## 6. Do-not-repeat

- Transparent submit: camouflage if/else inversion, chained NONE assignment, zero-store ordering
  (all inert or worse); the residual after the split is only the declaration-count register tie.
- Model draw: any statement placement of `subtract_vectors3d` - it stays inline (straight-line block);
  nested-call spelling is byte-identical. The inline frequency law does not apply here.
- Environment shader: the operand order of `dot_product3d`, the `(boolean)` cast and the raw-flag spellings
  for ALPHATESTENABLE, and chained or array-copy detail-scale stores.

## 7. Residual classification

- **Transparent submit** (measured): ownership/data-layout gap (address-taken BSS aggregate), fully closed
  by the split plus two decoded helper spellings. The remaining sensitivity is a declaration-count
  register tie (inference: symbol-table order drives constant register assignment).
- **Model draw** (measured): January calls `_subtract_vectors3d` and `_point_from_line3d` on
  `real_rgb_color` storage in a straight-line block, while `vector_from_points3d` and `dot_product3d` in the
  same arm are inlined. `_subtract_vectors3d` is defined only in January `action_charge.obj` (the COMDAT copy
  csplit attributes there) and referenced here. So this needs a non-inline declaration in this TU plus
  representation casts (prohibited by house rule). A further 57 address-normalized blocks remain: the plasma
  sub-struct fold, the seed/phase slots, and the colour/constant slot layout.
- **Environment shader** (measured): 41 blocks after the decoded helper spellings. These are the alpha-test
  temp register, the downstream push registers, the detail_map_scale integer-copy path, the fog-density term
  order and the cc0 pixel register. Inference: mostly register/scheduling ties, plus one unexplained
  integer-copy source form.
- **Research note** (measured): VC7 lays out a TU's uninitialised statics in `.bss` in a name-dependent
  (hash) order that is independent of declaration order. Renaming one static moves it, and reversing the
  declaration order does not. January's 0xBF-byte layout therefore encodes January's real static names.

## 8. Orchestrator proposals

1. `config/symbols.json` (edit in place, do not re-serialize): replace
   `{ "file_offset": 4611432, "flags": 0, "name": "_rasterizer_models_globals", "static": true },`
   with the 13 lines in `scratch/workersC/rasterizer_xbox_models/symbols_json_proposal.txt`. These are
   `_transparent_geometry_cached_animation` +0, `_transparent_geometry_cached_lighting` +4,
   `_transparent_geometry_cached_node_matrix_count` +8, `_transparent_geometry_cached_node_matrices` +12,
   `_immediate_transparent_geometry_group` +16, `_local_parameters` +0xB0, `_local_parameters_queued_flag` +0xB4,
   `_local_model_effect_type` +0xB8, `_local_sky_flag` +0xBA, `_local_planar_fog_flag` +0xBB,
   `_local_environment_fog_screen_flag` +0xBC, `_local_do_not_change_z_stencil_states` +0xBD, and
   `_local_reported_too_many_transparent_geometry_groups` +0xBE, all `"static": true`. Consumers: only this TU,
   and the scratch split shows no other object changes.
2. Park reopen for `__rasterizer_model_transparent_geometry_submit`, in the same commit as proposal 1.
   Land `park_reopen_submit.c`, unpark submit, and re-measure the `__rasterizer_model_draw` park, which drifts
   to 5,168 / 348 / `89b0d7ea77a71d69` under the split. Disclose the declaration-order sensitivity from 4.4.

## 9. Reopen criteria

- Model draw: an owner ruling that admits the rgb-to-vector/point casts for the self-illumination lerp, plus a
  narrow `REAL_MATH_EXTERNAL_SUBTRACT_VECTORS3D` switch; then the plasma sub-struct and slot blocks.
- Environment shader: a decoded source form for the detail-scale integer copy and the fog-density term
  order. Any change must re-gate submit because of the declaration-count sensitivity.

## 10. Disposition

The production source is untouched (`git diff --stat` is empty) and all three parks stay locked with no drift.
Submit is strict exact only in the scratch packet that depends on the symbols.json split. No credit is claimed.
