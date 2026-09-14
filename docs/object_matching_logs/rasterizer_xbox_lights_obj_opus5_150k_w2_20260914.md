# `rasterizer_xbox_lights.obj` — opus5 150K house-clean lane, wave w2 (2026-09-14)

## Baseline and outcome

| item | value |
| --- | --- |
| unit | `source/rasterizer/xbox/rasterizer_xbox_lights` |
| baseline gate | 13 exact / 1 residual / 0 unwritten; `_point_from_line3d` guard passes |
| only non-exact row | `_rasterizer_sun_glow_draw` (2,352 padded, 174 relocations, **parked**) |
| final real-file gate | 13 exact / 1 residual / 0 unwritten (real file **unchanged**) |
| newly exact | none |
| park drift | 0 (real-file object matches the park's base hash `dafe3a30c86a8480…`) |

The parked renderer was re-investigated in scratch only. No source landed. The work
refutes part of the park's premise: the 0x98/0xA4 frame gap is structural (frame-size
law), and it is closed in scratch with natural lexical scoping. What remains is a
VC7 canonical-ordering difference in one dot-product statement.

## Duplicate prevention

- Ledgers read:
  - `rasterizer_xbox_lights_obj_opus_reconciliation_20260903.md` (E01 only; do-not-repeat
    covers local aliases, statement motion, volatile, raw offsets and assembly used to steer
    the frame or x87 schedule);
  - `rasterizer_lights_obj_corona_rotation_and_draw_frontier_20260909.md` (declaration only);
  - the opus5 100K handoff.
- `git log --all`: 93e2f032e, 3ada67f86, bb0d690a3, e830efbd7.
- `branch_sweep`: only history blobs (13/1/0 and 0/0/14). No donor body exists.

## Evidence (January split COFF)

**Frame census.** January `sub esp,0x98`. The baseline candidate has 0xA4.

- January shares `ebp-0x10..-8` between the brightness `direction` and `projected_center`.
  The snapped `floor(x+0.5)` is stored back into `projected_center.x`'s slot, and no store
  to `.y` occurs.
- January shares `ebp-0x38..-0x2c` between `uncompressed_direction` (lea `ebp-0x34`),
  `projected_axes` (lea `ebp-0x30`) and `screen_bounds` (lea `ebp-0x38`).
- The candidate instead shares `direction` with `projected_axes` and `screen_bounds`. It
  gives `uncompressed_direction` its own 12 bytes (`ebp-0xa4`), which is exactly the gap.

**Occlusion radius.** January loads `occlusion_radius` once:
`fld [eax+0x10]` … `fld dir; fmul st(1)` ×3. It then pushes the field by integer copy and
pops the CSE (`fstp st(0)`) just before the call. The baseline multiplies by
`[eax+0x10]` three times.

**Brightness dot product.** January evaluates `dir.i*fwd.i`, `dir.j*fwd.j`, `dir.k*fwd.k`
in that order, with the local loaded first. It evaluates `cos(pi/4)` only after the sum
(`fld qword; fcos; fxch st(1); fsub st(1)`).

This `fld [ebp-x]; fmul [global]` ascending pattern appears once in the whole January
corpus, found by a byte scan of `build/split`. Every other function where January loads a
local before a data global is currently non-exact:

- `lightning_submit`
- `light_volume_render`
- `rasterizer_lens_flares_draw`
- `rasterizer_model_draw_environment_shader`
- `sound_cache_debug_render`
- `decal_new_from_collision`
- `input_abstraction_update`
- `collision_debug_render`

## Shapes (scratch only; `scratch/workers/rasterizer_xbox_lights/`)

| # | shape | result | decision |
| --- | --- | --- | --- |
| 1 | Math-helper spellings on the unscoped body: `dot_product3d`, `PIN`, inline `point_from_line3d` (p2, forbidden COMDAT), radius local at function scope (p3/p4) | frame stays 0xA4; the radius CSE appears but is consumed early | reverted |
| 2 | Lexical block scoping (qA–qF): uncompressed vector in a block; separate brightness and occlusion blocks; `projected_center.x/y` snapped in place | qD: **frame 0x98 and a slot census identical to January**; alndiff blocks 113 → 94 | scratch |
| 3 | qD plus block-local `real occlusion_radius` (r2); tidy spelling `cand_scoped.c` (declare+initialise, `PIN`, block-local `intensity`) | 2352/174, sha16 `38102c57976bc00d`; the radius schedule matches exactly (same bytes as inline `point_from_line3d`); only the dot/cos order remains | scratch; best credible candidate |
| 4 | More than 20 single-expression spellings of the dot and cos: operand and term order, `dot_product3d` either way round, locals, split statements, `+=`/`-=`/`/=`, no casts, `cosine()`, double arithmetic, external `normalize3d` | VC7 canonicalises all of them to k,j,i with `fcos` hoisted first (inert) | reverted |
| 5 | Accumulating the dot in a constant-trip loop (VC7 unrolls it), L_loopA; micro repros m8/m9 | January's i,j,k order **and** the late `fcos`/`fxch st(1)` reproduced. Residual: 3 instructions, the product operand order (January `fld local; fmul global`) | diagnostic only; not proposed as credible source |

The operand-order probes each put one hypothesis in isolated micro TUs. The local was
never loaded first:

- pointer, copy, `const` or nested-struct globals;
- block-scope extern and declaration order;
- symbol weight and aliasing;
- source operand order and removing `normalize3d`.

VC7 does load the local first for the i term of `rasterizer_lens_flares_draw` (reproduced
by our build of that TU). There it survives removing `normalize3d` and swapping the
`dot_product3d` arguments, so it is context-driven, not an address-taken effect.

## Measured observations (reusable)

1. **Frame-size law confirmed on a park.** The previous ledger read the sun-glow frame gap
   as an unclassified local/x87 tie. It was a scope structure. Block-local vectors let VC7
   pack the stack as January did.
2. **CSE lifetime follows scope.** A block-local `real` holding a field keeps its x87 copy
   alive through a later direct-field argument push outside the block. January's
   `fstp st(0)` before the call is reproduced, with the same bytes as inline
   `point_from_line3d`.
3. **Dot-product canonicalisation.** VC7 13.00.9254 reorders every single-expression
   `a.i*b.i + a.j*b.j + a.k*b.k` (with or without `- C` for a CSE `C`) to k,j,i, with `C`
   hoisted first. An unrolled constant-trip accumulation produces i,j,k with `C` after the
   sum. Micro evidence: `scratch/workers/rasterizer_xbox_lights/micro/m1.c`, `m8.c`, `m9.c`.

## Checks

- Real file untouched: `git diff --stat` is empty, 13/1/0, guard passes, park drift 0, owner
  census unchanged vs `build/base`, 0 fake-scan leads.
- Scratch candidates (`cand_scoped.c`, `r2.c`, `L_loopA.c`): 13 siblings stay exact, guard
  passes, 0 fake-scan leads. Park drift is expected there (scratch-only edits to the parked
  body).

## Proposals

No header or config needs. The orchestrator may re-baseline the park's evidence text
(parked.json is orchestrator-owned; hashes unchanged):

> Frame gap refuted as a tie (2026-09-14 w2): lexical block scoping of the brightness vector
> and of the uncompressed/occlusion vectors, plus snapping projected_center in place,
> reproduces January's 0x98 frame, slot census and occlusion-radius CSE lifetime
> (scratch/workers/rasterizer_xbox_lights/cand_scoped.c, 2352/174, sha16 38102c57976bc00d).
> The residual is VC7's canonical order of the brightness dot product: January emits i,j,k
> with the local loaded first and cos(pi/4) after the sum.

## Reopen criterion

Reopen when a natural spelling of the brightness dot product makes VC7 emit January's
`fld [ebp-0x10]; fmul [gwp+20]; fld [ebp-0xc]; fmul [gwp+24]; faddp; fld [ebp-8]; fmul [gwp+28]; faddp; fld qword; fcos; fxch st(1)`
on top of `cand_scoped.c`, or when January evidence of that statement's form appears. The
same mechanism probably blocks the other non-exact local-first functions listed above, so
solving one should be tried on all of them.
