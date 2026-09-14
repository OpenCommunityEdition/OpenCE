# `rasterizer_xbox_environment.obj`: opus5 150K house-clean lane, wave w1 (2026-09-14)

## Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_environment.c`, target
  `build/split/source/rasterizer/xbox/rasterizer_xbox_environment.obj` (January split COFF is the byte authority).
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`. Worker-only measurements with `tools/campaign/gate.py`
  (PID objects), `alndiff`, `tinfo`, `scratch/parkcheck.py`, `tools/fake_match_scan.py`. No ninja, header or config edits.
- Prior ledgers read: `_codex_small_exact_20260902`, `_diffuse_texture_reconciliation_20260909`,
  `_lightmap_draw_fuzzy_20260912`, `_opus_reflections_reconciliation_20260909`, `_opus5_100k_20260914` (wave B),
  `_opus5_100k_waveC_20260914`; plus `sound_dsound_xbox_obj_opus5_100k_waveC_20260914.md` (paren law) and
  `ai_obj_opus5_100k_waveD_20260914.md`. The real file at base is byte-identical to the 100K lane final file.
- Worker notes and every scratch candidate: `scratch/workers/rasterizer_xbox_environment.md`,
  `scratch/workers/rasterizer_xbox_environment/`.

## Baseline (real file, point guard passed)

**41 exact, 3 residual, 0 unwritten of 44.**

| Row | Target size/relocs/sha16 | Base size/relocs/sha16 | Status |
| --- | --- | --- | --- |
| `__rasterizer_environment_lightmap_draw` | 4016 / 248 / 7818e559325c1524 | 3984 / 246 / 02f716db92e8099f | residual |
| `__rasterizer_environment_diffuse_light_begin` | 928 / 58 / 96f42a578f493250 | 928 / 58 / 6b3b2693e14dabc8 | PARKED |
| `_rasterizer_environment_specular_spot_light_begin` | 960 / 64 / e0331f6b94a74f9c | 944 / 64 / d103843ee5f01bad | PARKED |

## Final real-file gate

**42 exact, 2 residual, 0 unwritten of 44.** Only `__rasterizer_environment_lightmap_draw` changed (residual -> EXACT).
Point guard pass; both parks equal their `measurements.base` (parkcheck drift 0); owner census vs `build/base` and the
baseline gate object: no new or lost owners; `fake_match_scan.py`: 0 leads before and after.

## `__rasterizer_environment_lightmap_draw` — EXACT landed (credibility flag, see below)

Landed body = wave B's four measured structural corrections (switch cases in jump-table order; genuine XDK
`PS_TEXTUREMODES` for both `texture_modes` stores; all constant `pixel_shader` stores before the five
`real_*_to_pixel32` calls; stage-0 MAG/MIN/MIP filters written in each test-pattern arm) plus a restructured
self-illumination block:

```c
real intensity = 1.0f;
...
primary_off_scale = intensity*(1.0f - primary_value);
secondary_off_scale = intensity*(1.0f - secondary_value);
primary_on_scale = intensity*primary_value;
secondary_on_scale = intensity*secondary_value;
scale_vector3d(&primary_off_color, primary_off_scale, &primary_color);   /* then secondary */
point_from_line3d(&primary_color, &primary_on_color, primary_on_scale, &primary_color);   /* then secondary */
scale_vector3d(&plasma_on_color, intensity, ...);  scale_vector3d(&plasma_off_color, intensity, ...);
```

(the self-illumination locals now live in the `else` block that uses them).

Measured matrix (all in scratch):

| Shape | Result |
| --- | --- |
| v0 = wave C `cand0` (wave B corrections) | 4000 / 248 |
| p1: bare parens on the stack-local `t` argument | 4000 (paren law **inert** on a stack-local leaf) |
| c1: constant `intensity` factor + bare parens `(v*intensity)` | EXACT (bare parens: not admissible) |
| e0: c1 without parens | 4000 |
| d4: factor on `t` only, not on the off scales | 4016, sha differs (first plasma push order) |
| e2 / f1: named real temporaries holding `v*intensity` (and the off scales), no parens or casts | EXACT |
| f2 / g2n: the same temporaries **without** the factor | 4000 |
| **g2** (landed): locals in the else block, four assigned scale temporaries | **EXACT 4016 / 248 / 7818e559325c1524** |

Mechanism (refines the brief's real-temporary clause): a named `real` temporary whose expression c2 folds to a bare local
(`v*k`, `k` a constant `real` local) is pushed through the x87 when the callee's return value is discarded; a temporary
that is a plain copy of a local is copy-propagated and integer-copied.

**Credibility flag for the orchestrator/verifier.** Wave C rejected identity-factor forms (with casts/parens) as fake
arithmetic because no evidence supported a factor. The landed form needs no parens, casts, pragmas or header changes; the
factor itself remains. The argument for it is January-side: both plasma colours are passed through `_scale_vector3d` with the
constant `0x3f800000` (these are the only two constant-1.0 pushes among all 12 out-of-line January `_scale_vector3d` calls,
`callscan.py`), which is not how a literal colour copy is written; a single named self-illumination intensity of 1.0 applied to
all four pixel-shader colours explains three independent January-visible effects (the plasma constant pushes, the first plasma
push order, and the x87 `t` pushes). If this is still judged inadmissible, revert only this function's hunk; the object then
returns to 41/3/0 and wave B's corrections alone give 4000/248.

## `__rasterizer_environment_diffuse_light_begin` — PARK-REOPEN PROPOSED (strict exact in scratch)

- Park premise: "C2 assigns three coherent local basis vectors to different stack slots".
- New evidence: January stores the three gel basis vectors contiguously in `real_matrix3x3` member order (forward
  `[ebp-0x30]`, left `[ebp-0x24]`, up `[ebp-0x18]`): the slot order is the aggregate's layout, not independent ranking.
  January's cross product is forward x up (`left.i = up.k*fwd.j - up.j*fwd.k`), the handedness January also uses in
  `specular_spot_light_begin`; the parked body computed up x forward, a sign error in the retained fuzzy body.
- u1: `real_matrix3x3 gel_matrix` with the old argument order -> slots match, 6 operand blocks remain.
- u2: `cross_product3d(&gel_matrix.forward, &gel_matrix.up, &gel_matrix.left)` -> **strict EXACT 928 / 58 / 96f42a578f493250**.
- Candidate (current real file + only this body): `scratch/workers/rasterizer_xbox_environment/diffuse_light_begin_reopen_candidate.c`.
  Whole TU 42 -> 43 exact, only this row changes; point guard pass; spot park unchanged; census clean; fake scan 0.
  The same body applied to the HEAD file (without the lightmap landing) is also exact (`hu2.c`, 42 exact).

## `_rasterizer_environment_specular_spot_light_begin` — not landed (park locked), structural lead recorded

- January frame `sub esp,0x84` vs ours `0x74`. January slots: vertex constants `[-0x84,-0x34)`, unreferenced 4 B at `-0x34`,
  forward copy `-0x30`, cross result `-0x24`, up copy `-0x18`, unreferenced 12 B at `-0xc`: exactly
  `real_matrix4x3 {scale; forward; left; up; position}` with scale/position never written. January's cross product reads
  `light->forward` / `light->up` through pointers with the member copies interleaved.
- s1 cross from light fields: 944 (early block aligned). s2 separate position copy: 976 (rejected).
  **s3a `real_matrix4x3 matrix`, cross from `&light->forward, &light->up` into `&matrix.left`: 960 / 64, frame 0x84**
  (size, relocations and frame now equal), 15 small blocks: j/k cross-product operand load order inside the shared
  `cross_product3d` inline, a register rotation in the position copy, one `fxch`, and `set_pixel_shader` push placement.
  s3b (cross from matrix members) 16 blocks; s4 (cross before copies) 21; t2 (up copy first) 20; t3 (separate normalize) 15; t4 25.
- Classification: the 16-byte structural gap is closed in scratch (`s3a.c`); the remainder is operand-order/register
  scheduling tied to the shared real_math.h inline. Park not reopened; real file untouched.
- Reopen criterion: a natural cross-product spelling (or a consumer-audited owner change) that loads `up.i`/`up.j` first in the
  j/k terms; start from `scratch/workers/rasterizer_xbox_environment/s3a.c`.

## Rejected / do-not-repeat

- Bare parentheses on any argument (no genuine macro); `(real)` casts on `t`; `double` intensity; parens on a stack-local leaf (inert).
- Named temporaries without the factor (inert); separate `real_point3d` position copy in the spot light (not elided, +32 B).

## Proposals

- Orchestrator: unpark `__rasterizer_environment_diffuse_light_begin` with the candidate body (dedicated commit).
- Orchestrator/verifier: adjudicate the constant `intensity` factor in `lightmap_draw` (landed; revert hunk if rejected).
- Optional: refresh the spot-light park evidence to cite the `real_matrix4x3` frame finding and the closer `s3a` body.
