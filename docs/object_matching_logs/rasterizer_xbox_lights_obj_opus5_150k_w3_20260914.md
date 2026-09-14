# `rasterizer_xbox_lights.obj` — opus5 150K house-clean lane, wave w3 (2026-09-14)

## Baseline and outcome

| item | value |
| --- | --- |
| unit | `source/rasterizer/xbox/rasterizer_xbox_lights` |
| baseline gate | 13 exact / 1 residual / 0 unwritten; `_point_from_line3d` guard passes |
| only non-exact row | `_rasterizer_sun_glow_draw` (2,352 padded, 174 relocations, **parked**) |
| final real-file gate | 13 exact / 1 residual / 0 unwritten (real file **unchanged**, park drift 0) |
| newly exact (real file) | none |
| scratch result | `_rasterizer_sun_glow_draw` **strict EXACT, 14/14**, proposed as a park reopen |

## Duplicate prevention

Read first: `_opus_reconciliation_20260903` (do-not-repeat: aliases / statement motion /
volatile / raw offsets / assembly used only to steer), `_opus5_150k_w2_20260914` (frame gap
solved by scoping in `cand_scoped.c`; 20+ single-expression dot spellings; accumulation loops;
micro repros m1-m18), `scratch/workers/rasterizer_xbox_lights.md`, `scratch/w2/laws_w2.md`
(A22 dot canonicalisation and younger-operand law) and the breakable_surfaces w2 ledger that
measured the younger-operand law. None of the w2 shapes wrote the direction vector without
naming its fields.

## New evidence: leaf-age law in the real TU

All probes are scratch copies of w2 `cand_scoped.c` (frame 0x98, occlusion CSE already exact),
changing only the brightness block. Harness: `scratch/workers/w3_rasterizer_xbox_lights_rasterizer_xbox_active_camouflage/lv.py`.

1. **Operand order.** VC7 `fld`s the younger leaf of a commutative memory x memory product.
   Any *named* reference to `direction.i/j/k` (named stores or named dot reads) creates old
   leaves, so the global forward vector is loaded first. That is every w2 shape (T3, T4, F1,
   L3, E8 re-confirm it). When the vector is written through an inline pointer parameter
   (`vector_from_points3d`) or a pointer local, and read only through a pointer local, all three
   products load the local first (E4, H4) — January's `fld [ebp-0x10]; fmul [gwp+20]` x3.
   Through `dot_product3d` parameters the offset-0 term differs: i global-first, j/k local-first
   (T1, T2, N3, N4, L1, L2, Q1, Q2).
2. **Term order.** A single-expression sum is emitted youngest term first (k,j,i) in every regime
   (M*, R*, S2, S6, Q*). Two-term splits confirm the rank is creation order (S4 -> j,i,k;
   S5 -> i,k,j). Unrolled loops either load the global first (G6, LQ1, LQ2) or do not unroll
   (LP1-LP4). January's i,j,k therefore proves a sequential accumulation.
3. **cos placement.** The late `fld qword; fcos; fxch st(1)` needs the sum to exist as a separate
   value feeding `PIN((cosine - cos(pi/4)) / (1 - cos(pi/4)), 0, 1)` (H8, G3, H9). An
   `intensity = (x - cos)/(1 - cos); PIN(intensity)` pair hoists `fcos` first (E6, E7, G2, G5).

## Candidate shapes (<= 5 at candidate level)

| # | shape | result |
| --- | --- | --- |
| 1 | `dot_product3d` regimes | i global-first; not exact |
| 2 | pointer reads, single expression | operand order exact, terms k,j,i |
| 3 | pointer reads, separate `cosine`, PIN over the ratio | `fcos` placement exact, terms k,j,i |
| 4 | **V1**: `real_vector3d *direction = vector_from_points3d(&camera_position, &position, &camera_to_light); normalize3d(direction);` three-statement `cosine` accumulation; `brightness = PIN((cosine - (real)cos(_pi / 4.0f)) / (1.0f - (real)cos(_pi / 4.0f)), 0.0f, 1.0f);` | **STRICT EXACT**, 14/14 |
| 5 | **W1**: V1 with `real_vector3d *direction = &camera_to_light;` and `direction->i = parameters->position.x - camera_position.x` stores (no helper) | **STRICT EXACT**, 14/14 |

Candidates (each = current real file + only the parked body replaced; CRLF preserved):

- `scratch/workers/w3_rasterizer_xbox_lights_rasterizer_xbox_active_camouflage/V1.c`
  (sha256 `069457152071b1ed276f97aca6b32d4bfdbd623de59049fadab290f0cc0028c5`)
- `scratch/workers/w3_rasterizer_xbox_lights_rasterizer_xbox_active_camouflage/W1.c`
  (sha256 `61bd4038a6faff6bd0a00df911e3b058a85f9962bf17bcfcaef77b41589eb844`)

The remaining body is w2's `cand_scoped.c` (brightness and occlusion block scoping,
block-local `occlusion_radius`, `projected_center` snapped in place).

## Checks (both candidates)

- whole-TU gate `== exact 14 residual 0 unwritten 0`; all 13 baseline-exact rows stay exact; guard passes;
- `scratch/parkcheck.py`: only `_rasterizer_sun_glow_draw` drifts, to the January sha16 `35e009239abac990`;
- `tools/fake_match_scan.py`: 0 leads (HEAD 0);
- owner census vs `build/base`: **W1 none**; **V1 adds one select-any `_vector_from_points3d` COMDAT**.
  That is the systemic header-inline surplus class (75 base objects already emit it), but laws_w2 C3
  still lists it as a pending owner ruling (`_compute_sound_obstruction`).

## Credibility disclosure

- The three-statement accumulation is the only measured spelling that yields January's term order.
  In-tree exact precedent uses the same accumulation form: `real_math.c` 826-831
  (`sphere_intersects_cone3d`), 1758-1760 and 2655-2657.
- V1's pointer is the helper's natural return value. W1's pointer local is used seven times
  (A5 counter-law: four or more uses is genuine) but points at a local vector.
- No cast, volatile, alias-only temporary, operand swap, fixed condition or header change is used.

## Proposals

- **park-reopen** `_rasterizer_sun_glow_draw` (2,352 B). Preferred: V1 if the orchestrator admits the
  `_vector_from_points3d` surplus-COMDAT census class; otherwise W1 (census-clean).
- **Law packet (for other lanes).** The leaf-age mechanism explains January's rare local-first x87 products. The
  byte scan in `scratch/workers/rasterizer_xbox_lights/scan_localfirst.py` found these still-residual functions:
  `lightning_submit`, `light_volume_render`, `rasterizer_lens_flares_draw`,
  `rasterizer_model_draw_environment_shader`, `sound_cache_debug_render`, `decal_new_from_collision` and
  `collision_debug_render`. For each, check whether a named store or read ages the local vector.

## Reopen criterion (if the reopen is refused)

Refusal on credibility grounds should state which part is inadmissible (the accumulation or the pointer).
A single-expression spelling producing i,j,k with local-first products would supersede both candidates.
