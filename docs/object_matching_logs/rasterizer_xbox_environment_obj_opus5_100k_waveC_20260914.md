# `rasterizer_xbox_environment.obj`: opus5 100K consolidated lane, wave C (2026-09-14)

## Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_environment.c`,
  target `build/split/source/rasterizer/xbox/rasterizer_xbox_environment.obj`.
- VC7 13.00.9254, repository flags `/O2 /Oy- /DDEBUG /Dxbox` with the checked-in
  XDK headers. The January split COFF is the byte authority.
- Lane `opus/100k-consolidated-20260914`, worker waveC. No Ghidra or map-atlas
  extract exists for this object.
- Evidence used:
  - the January COFF, through `tools.campaign.alndiff`;
  - a January-wide scan of every split object for x87 argument pushes
    (`scratch/workersC/rasterizer_xbox_environment/fldscan.py`);
  - a scan of all 37 January `_point_from_line3d` call sites;
  - an isolated VC7 lab (`lab.py`, `lab1.c`..`lab23.c`);
  - the HCEA dx9 `_rasterizer_environment_lightmap_draw_pp.c`, for semantics
    only;
  - the earlier environment ledgers, in particular
    `rasterizer_xbox_environment_obj_opus5_100k_20260914.md` (wave B).

## Validated baseline

Lane HEAD gate (point guard passed): **41 exact, 3 residual, 0 unwritten of 44**.
No data work in scope.

| Row | Target size/relocs/sha16 | Base size/relocs/sha16 | First divergence |
| --- | --- | --- | --- |
| `__rasterizer_environment_lightmap_draw` | 4016 / 248 / 7818e559325c1524 | 3984 / 246 / 02f716db92e8099f | wave B corrections not in production; after them, the `point_from_line3d` `t` pushes |
| `__rasterizer_environment_diffuse_light_begin` (parked) | 928 / 58 / 96f42a578f493250 | 928 / 58 / 6b3b2693e14dabc8 | stack-slot assignment (park) |
| `_rasterizer_environment_specular_spot_light_begin` (parked) | 960 / 64 / e0331f6b94a74f9c | 944 / 64 / d103843ee5f01bad | park |

## Accepted controls

None this wave. The production file is unchanged.

## FPU argument mechanism (measured)

The wave B candidate is 4000/248. It differs from January only where the `t`
argument of both `point_from_line3d` calls is pushed:

- January: `fld [ebp-4]; ... push ecx; fstp [esp]`.
- Ours: `mov ecx,[ebp-4]; push ecx`.

The pending-pop and register schedule that follows depends on that push, and
so does the push order in the first plasma `scale_vector3d` call.

Measured facts:

- **Where January uses this form.** Among single-float arguments, January
  pushes a pure `fld` → `fstp [esp]` only at these sites:
  - all 4 callers of `_fast_vector_intersects_sphere` (ai, players, collisions,
    bipeds);
  - 40 hs evaluate wrappers;
  - the two sites in `lightmap_draw`.
  
  Multi-float pushes through `SetVertexData*` and `IDirectSound*` also use it,
  but they are a separate pattern. In our tree, only hs produces the form, using
  a named local copy.
- **Other `point_from_line3d` callers.** Every other January caller that passes
  a stack `real` uses mov/push. The lightmap form is therefore not a property of
  the callee declaration or of a declined inline.
- **What triggers it.** VC7 emits the fld form only when the argument is an x87
  expression node that survives c1 and is reduced to a bare load in c2. Lab
  forms that produce it:
  - `-(-x)`, which also keeps two `fchs`;
  - an explicit same-type cast of an expression that c2 folds, for example
    `(real)(x*1.0)`, or `(real)(x*k)` where `k` is a constant local;
  - `(real)(p->field)`, which also reproduces the players pending-pop form
    `add esp,N; fstp [esp]`;
  - a `real` or `double` local copy of a pointer field;
  - a `double` copy of a local, with no call between the definition and the use;
  - an inlined helper that takes a `double` parameter.
- **Inline expansion here.** In this function the header-inline
  `point_from_line3d` is always expanded, even in cold-block probes. The inline
  frequency law therefore does not explain January's out-of-line call here.

## Experiment matrix (scratch only)

| Shape | Size / relocs / sha16 | Siblings | Kept |
| --- | --- | --- | --- |
| cand0 (wave B v2b) | 4000 / 248 / 43ac7853c0bb4168 | 41 E | scratch |
| `-(-v)` on `t` | 4016 / 248 / 43f4403151d72fb6 (4 extra `fchs`) | 41 E | diagnostic |
| `(real)(v * 1.0)` on `t` | 4016 / 248 / 933eb5e950cb6cc4 (first plasma push order) | 41 E | diagnostic |
| k1: `v*intensity`, `(1.0f - v)*intensity`, plasma `intensity` | 4000 / 248 / 28f3cd0a347f2990 | 41 E | reverted |
| k7: cast on `t` only, plasma `intensity` | 4016 / 248 / 933eb5e950cb6cc4 | 41 E | reverted |
| k16: `real intensity = 1.0f;`, `(1.0f - v)*intensity`, `(real)(v*intensity)` | **4016 / 248 / 7818e559325c1524 (strict exact)** | 42 E, parks OK, fakescan 0 | **not admissible** |
| k17: `(1.0f - v)*1.0`, `(real)(v*1.0)` | strict exact | 42 E | not admissible |
| k22: `double intensity = 1.0;` variant | strict exact | 42 E | not admissible |
| k21: `(real)` casts on every argument | 4016 / 248, sha differs | 41 E | reverted |
| header-inline `point_from_line3d` (noext_pfl) | 4048 / 246 | 41 E | diagnostic |
| cold-block probes (if / ?: / \|\| arms) | 4064 / 248, 4048 / 247 (still inlined) | - | diagnostic |
| `static real const` / `static double const` factor | 4048 / 254 (global loads) | - | reverted |

Every exact form needs two things:

- an identity factor (×1) on both scale arguments and both `t` arguments;
- an explicit cast on `t`.

No January, HCEA or Stian evidence supports such a factor. HCEA computes
`on*blend + off*(1-blend)` with no intensity term. These forms are fake
arithmetic under the house rules, so none was landed.

## Do-not-repeat list

All of the following still produce mov/push for the `t` argument:

- **Casts on a stack scalar:** `(real)`, `(float)`, `(double)` and
  `(real)(double)`. c1 drops them.
- **Arithmetic identities:** `x*1.0f`, `1.0f*x`, `x/1.0f`, `x+0.0f`, `x-0.0f`.
- **Implicit prototype conversion** of `x*1.0`, or of `x*k` with `k` typed
  `real`, `double`, `long` or `short`.
- **Local variable forms:**
  - const or constant-propagated float locals;
  - stack-variable copies in nested blocks;
  - struct or array members, whether plain, address-taken or copied;
  - dereferences through a pointer to a local.
- **Other expression wrappers:** unary plus, comma, `flag ? a : a`, identity
  inline helpers.
- **Declaration changes:**
  - `real const` parameter in the prototype;
  - `periodic_function_evaluate` returning `double`;
  - `/Op`.
- **Declined inlines:** a size-declined `__inline point_from_line3d`.
- **Interpolate helpers.** They also regroup the calls, and January's call order
  is grouped by operation: scale, scale, point, point.
- **Wave B's list.**

## Residual classification

- **Measured:**
  - wave B's four structural corrections;
  - the byte target, which is reachable when both scale arguments and both `t`
    arguments are c2-folded expressions and the `t` expressions keep an x87
    conversion node.
- **Inference:** the original source had some term or conversion on these four
  arguments that is not recoverable from current evidence.
- **Classification:** unresolved source form. It is not a register tie.
- **Parks:** unchanged. `fldscan` finds no x87 argument pushes in either parked
  function, so this lever does not apply to them. Neither reopen criterion is
  met.

## Reopen criteria

- **`lightmap_draw`:** reopen on evidence of a genuine factor or conversion in
  January's self-illumination colour computation. Examples:
  - an intensity or brightness term;
  - a `double`-typed intermediate;
  - an explicit cast in a compatible-build donor.
  
  Such evidence would make the scale and `t` arguments c2-folded expressions.
  Start from `scratch/workersC/rasterizer_xbox_environment/cand0.c`; k16, k17 and
  k22 show the target tree shape.
- **Parks:** as documented in `config/parked.json`.

## Disposition

- `__rasterizer_environment_lightmap_draw`: still residual, zero credit.
- Parked rows: unchanged and locked.
- Final real-file gate: 41 exact, 3 residual, 0 unwritten. The point guard
  passed and park drift passed. Owner census shows no change. `fake_match_scan`
  reports 0 findings.
