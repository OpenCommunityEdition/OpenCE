# rasterizer_xbox_models.obj - opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Translation unit: `source/rasterizer/xbox/rasterizer_xbox_models.c`. Target: `build/split/source/rasterizer/xbox/rasterizer_xbox_models.obj`.
- Compiler: XDK 3911 VC7 13.00.9254, repository flags.
- Worktree: `C:\halo-worktrees\opus5-150k-house-clean-20260914`, base `12f7375d4`. The real-file blob is unchanged since `7a1d0d91d`.
- Scratch: `scratch/workers/rasterizer_xbox_models/`. Worker notes: `scratch/workers/rasterizer_xbox_models.md`.
- Targets: the three locked parks. This wave re-investigated them in scratch only.
- Evidence used:
  - January target COFF;
  - a scratch csplit of `cachebeta.exe` with a proposed `symbols.json`. Of the 833 split objects, only this one differs from `build/split`;
  - VC7 probe compiles (`p1`-`p7`);
  - `HCEX.pdb` (SHIP, June 2011), read through `DIA2Dump -s` / `-sym`, plus a raw module-stream name scan;
  - HCEA reconstructions, used for names and topology only;
  - `config/symbols.json` inline-helper flags;
  - the waveC scratch tree (`opus5-100k-consolidated-20260914/scratch/workersC/rasterizer_xbox_models`).
- Previous ledgers read:
  - `opus5_100k_waveC`;
  - `opus_reconciliation_20260909`;
  - `transparent_geometry waveC`;
  - the `jonas_*` lifecycle/tint/wrapper ledgers;
  - the 100K handoff. It rejected the waveC submit reopen as "static names chosen to reproduce the name-hashed .bss layout".

## 2. Baseline (`gate.py --all --forbid-emitted-symbol _point_from_line3d`)

`exact 11, residual 3, unwritten 0`; guard passed.

| Function | January padded / relocs / sha16 | Candidate |
| --- | --- | --- |
| `__rasterizer_model_draw` | 5168 / 350 / `99d8b95f1ce01178` | 5168 / 348 / `23ca80471861b904` |
| `__rasterizer_model_transparent_geometry_submit` | 1296 / 83 / `7b84b560459ad2dd` | 1344 / 93 / `8fb7c26befa120e1` |
| `_rasterizer_model_draw_environment_shader` | 3104 / 235 / `d4c4ee8c3b174dad` | 3104 / 235 / `5179768ee4ad9fc8` |

## 3. Result

Nothing landed in the production file: `git diff --stat` is empty, the final gate is unchanged at 11/3/0, and there is no park drift.

A park-reopen candidate for `__rasterizer_model_transparent_geometry_submit` is **strict EXACT** under a proposed `symbols.json` split. It has 12/14 strict exact functions against the scratch split, all 11 siblings stay exact, and the whole January 0xBF `.bss` layout is reproduced symbol for symbol. It depends on new evidence (section 4) that refutes the premise of the earlier rejection.

## 4. New evidence

### 4.1 VC7 13.00.9254 `.bss` layout law (measured, probes p1-p7)

The compiler lays out a TU's `.bss` in three groups, in this order:

1. **Uninitialised (tentative) statics.** They come first, in name-hash order. The order is stable across different sets of names.
2. **Explicitly zero-initialised statics** (`= NULL`, `= FALSE`, `= {0}`). They follow, in **declaration order**.
3. **Function-scope statics.** They follow in definition order.

The waveC 13 tentative statics produced a layout that is entirely different from January's. The waveC candidate therefore never reproduced the hashed layout, and the rejection premise does not apply.

### 4.2 HCEX PDB authentication

The `HCEX.pdb` compiland `rasterizer_dx9_models.obj` lists these file statics, in this order:

1. `local_parameters`
2. `local_parameters_queued_flag`
3. `local_pixel_shader_dirty_flag` (in `.data`)
4. `local_model_effect_type`
5. `local_sky_flag`
6. `local_planar_fog_flag`
7. `local_environment_fog_screen_flag`
8. `local_do_not_change_z_stencil_states`

For `_rasterizer_model_transparent_geometry_submit` it lists `Static Local unsigned char warned` and `Local real_plane3d zero_plane`.

Declared as zero-initialised statics in this order, followed by the function static `warned`, these names land at exactly January's offsets: B0, B4, B8, BA, BB, BC, BD and BE.

This gives January-side corroboration of both the names and the order. January's assert literals `local_parameters` and `local_model_effect_type==...` independently authenticate two of the names.

### 4.3 The five unnamed statics are separate scalars

These are the four transparent-geometry cached pointers plus the immediate group.

- **Aggregate forms fail.** Two shapes were measured: v1 kept the 0xB0 aggregate, and v2/v4 used a 0x10 aggregate holding only the four cached pointers. Both reload the cached members after every `group->` store, while January hoists all four loads.
- **No name record survives.** `HCEX.pdb` has no record for any of the five. The raw module stream for submit contains only the parameters, `zero_plane` and `warned`.
- **Names used.** The candidate keeps the production descriptive names: `transparent_geometry_cached_animation`, `_lighting`, `_node_matrix_count`, `_node_matrices`, and `immediate_transparent_geometry_group`.
- **Order recovered.** Declaring them zero-initialised before `local_parameters` reproduces offsets 0/4/8/C/10 by declaration order alone. The names influence neither the code nor the layout.

### 4.4 No declaration-order steering

v6 declares the statics first and v7 declares them after the `local_*` group. Both are EXACT for submit. The waveC declaration-count tie disappears with the zero-initialised form.

### 4.5 Two decoded body edits are still required

These are waveC findings, re-measured here:
- `vector_from_points3d` + `-dot_product3d` for `z_sort`;
- `sort_filth->group_index = NONE` stored before the two NULL references.

Without them, submit is residual (t1/t2/t3).

## 5. Experiment matrix (scratch; strict check against the matching scratch split)

| ID | Shape | Submit | Other |
| --- | --- | --- | --- |
| v1 | 0xB0 aggregate + authenticated zero-init `local_*` + `warned` + `zero_plane` + body edits | 1296/83 `b55a59b4` | cached reloads + zero/NONE register tie |
| v2 / v4 | 0x10 aggregate (cached) + separate group, declared after / before | `07da7e29` | cached reloads |
| v3 / v5 | 5 separate tentative statics, after / before | EXACT | `.bss` layout wrong (lighting at B0) |
| **v6** | 5 separate zero-init statics declared first | **EXACT** | **full January `.bss` layout**; `park_reopen_submit.c` |
| v7 | 5 separate zero-init statics declared after `local_*` | EXACT | layout differs (proves no order steering) |
| t1-t3 | v6 without the z_sort and/or filth edits | residual | edits required |
| e1 | v6 + waveC environment-shader spellings | EXACT (no longer flips) | env 41 real blocks (v6: 71) |
| e2 / e3 | + unit-local `__inline set_real_vector4d` for vertex (/ specular) constants | flips to `9aab703e` | env 3120 bytes, candidate-only COMDAT; reverted |
| e4 | HCEA `base_map_transform[1] = base_map_transform[0]` | EXACT | env sha identical to e1 (canonicalised) |
| e5 | detail scale as a direct field read twice | EXACT | env 3120, integer path but reload, not January's copy |

## 6. Per-function outcomes

- **`__rasterizer_model_transparent_geometry_submit`: PARK-REOPEN-PROPOSED.** Candidate `scratch/workers/rasterizer_xbox_models/park_reopen_submit.c`, 1296 / 83 / `7b84b560459ad2dd`.
  - All 11 baseline-exact siblings are strict EXACT against `split_sep5`.
  - The point guard passed.
  - Owner census: the `.bss`/`.data` owners are exactly the 13 split names plus `_local_pixel_shader_dirty_flag`, with section sizes 191/1 equal to the target. There are no other new owners relative to `build/base`.
  - The fake scan reports 0 leads.
  - The environment-shader park does not drift.
  - The draw park drifts to 5168 / 348 / `89b0d7ea77a71d69`. The drift is inherent to the split; draw moves from 290 to 286 aligned blocks against the split target and remains residual.
- **`_rasterizer_model_draw_environment_shader`: NOT-LANDED.** Four new shapes (e2-e5) were tried.
  - The remaining 41 blocks are register/x87 scheduling sites keyed on symbol numbering: alpha-test parameter homing, argument registers, the detail-scale register copy, the fog-density term order, and the pixel-register permutation.
  - Finding: the zero-initialised packet makes the waveC e1 spellings compatible with an exact submit.
- **`__rasterizer_model_draw`: SKIPPED-EXHAUSTED.** No new evidence for the `subtract_vectors3d` / `point_from_line3d` colour-cast blocker. The `HCEX.pdb` locals only confirm the retained local types.

## 7. Orchestrator proposals

**1. `config/symbols.json`** (edit in place; do not re-serialise). Replace the line
`{ "file_offset": 4611432, "flags": 0, "name": "_rasterizer_models_globals", "static": true },`
with these 13 lines (the file uses CRLF):

```
{ "file_offset": 4611432, "flags": 0, "name": "_transparent_geometry_cached_animation", "static": true },
{ "file_offset": 4611436, "flags": 0, "name": "_transparent_geometry_cached_lighting", "static": true },
{ "file_offset": 4611440, "flags": 0, "name": "_transparent_geometry_cached_node_matrix_count", "static": true },
{ "file_offset": 4611444, "flags": 0, "name": "_transparent_geometry_cached_node_matrices", "static": true },
{ "file_offset": 4611448, "flags": 0, "name": "_immediate_transparent_geometry_group", "static": true },
{ "file_offset": 4611608, "flags": 0, "name": "_local_parameters", "static": true },
{ "file_offset": 4611612, "flags": 0, "name": "_local_parameters_queued_flag", "static": true },
{ "file_offset": 4611616, "flags": 0, "name": "_local_model_effect_type", "static": true },
{ "file_offset": 4611618, "flags": 0, "name": "_local_sky_flag", "static": true },
{ "file_offset": 4611619, "flags": 0, "name": "_local_planar_fog_flag", "static": true },
{ "file_offset": 4611620, "flags": 0, "name": "_local_environment_fog_screen_flag", "static": true },
{ "file_offset": 4611621, "flags": 0, "name": "_local_do_not_change_z_stencil_states", "static": true },
{ "file_offset": 4611622, "flags": 0, "name": "?warned@?1??_rasterizer_model_transparent_geometry_submit@@9@9", "static": true },
```

Consumers: only this TU. The scratch csplit changes no other object.

Disclosure on names:
- **Authenticated (8):** the seven `local_*` names and `warned`, by `HCEX.pdb` plus January's layout; two of them also by January asserts.
- **Descriptive, not authenticated (5):** the four cached-pointer names and `immediate_transparent_geometry_group`. These are the existing production field names; they have no surviving record and influence neither the bytes nor the layout.

**2. Park reopen** for `__rasterizer_model_transparent_geometry_submit`, in the same commit as proposal 1:
- land `park_reopen_submit.c`, which is the current real file plus the static structure and the submit body edits;
- unpark submit;
- re-baseline the `__rasterizer_model_draw` park at `89b0d7ea77a71d69`.

## 8. Reopen criteria

- **Environment shader.** Two things must hold in the same TU state after proposal 1 has landed:
  - a decoded source form that produces January's `mov edx,[scale]; mov ecx,edx`;
  - the j,k,+,i fog-density order.
- **Model draw.** An owner ruling that admits the rgb-to-vector/point casts, plus a narrow external `subtract_vectors3d` switch.

## 9. Do-not-repeat (additions)

- Aggregate representations of the submit caches: any struct-typed global is treated as aliased by `group->` stores.
- Choosing static names by `.bss` hash order. The zero-initialised declaration-order form makes this unnecessary.
- A unit-local `set_real_vector4d` inline for the environment shader constants: it adds a declaration, flips ties, and emits a COMDAT.
- The array-copy and direct-field detail-scale spellings. They are canonicalised, or give a reload instead of a copy.
