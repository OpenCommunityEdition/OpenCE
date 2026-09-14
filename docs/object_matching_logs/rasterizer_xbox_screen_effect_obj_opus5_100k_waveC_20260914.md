# `rasterizer_xbox_screen_effect.obj` - opus5 100K consolidated lane, wave C (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_screen_effect.c`, target
  `build/split/source/rasterizer/xbox/rasterizer_xbox_screen_effect.obj`.
- Compiler: XDK VC7 13.00.9254 `CL.Exe /nologo /c /O2 /Oy- /DDEBUG /Dxbox` (project defaults, no override).
- Provenance: January target COFF (instructions, relocation order, `.rdata` assertion/error texts, assert line
  literals); HCEA `rasterizer/dx9/rasterizer_screen_effect_set_texture_transforms.c` (later build, semantics only);
  HCEX.pdb (Jun 2011, SHIP) function-scope locals of `rasterizer_screen_effect_set_texture_transforms` read with
  `DIA2Dump -sym` (names/types only: `union real_vector2d default_scale`, `struct bitmap_data viewport_map`,
  `float[8][4] vsh_constants__texanim`, `stage0_size`, `stage1_size`, `viewport_size`, `reference_viewport_size`).
  No Ghidra or map-atlas file exists for this object.
- Started from wave B's best candidate `scratch/workers/rasterizer_xbox_screen_effect/v5.c` (ledger
  `rasterizer_xbox_screen_effect_obj_opus5_100k_20260914.md`). Worker notes: `scratch/workersC/rasterizer_xbox_screen_effect.md`.

## 2. Validated baseline (lane HEAD)

`== exact 10  residual 3  unwritten 0` (13 functions); data owners unchanged by this wave (owner census clean).

| Function | Target size / relocs / sha16 | Base size / relocs / sha16 | Status |
|---|---|---|---|
| `__rasterizer_screen_effect` | 3888 / 290 / d67a06f0c52e6846 | 3872 / 290 / 073549d7cf4ac30e | PARKED (locked) |
| `__rasterizer_screen_flash` | 1344 / 118 / f1d3e84eebfa355c | 1344 / 118 / 74b68c67df01d6b9 | PARKED (locked) |
| `_rasterizer_screen_effect_set_texture_transforms` | 2128 / 86 / c24f18204f9cfada | 2080 / 86 / 4b0ee325d2bf5c81 | residual |

## 3. Accepted controls (landed)

`_rasterizer_screen_effect_set_texture_transforms` is now strict EXACT (2128 / 86 / c24f18204f9cfada). Retained shape:

1. Wave B's structural decode (v1-v5): private ABI with `parameters` in EAX (asserts 47/48 first, body in a nested
   block so the stub bitmap initializer follows the asserts), stub-bitmap `width/height` subtrahends, window-bounds
   centre sums `x1 + x0`.
2. **Per-stage scale chosen by a struct-valued conditional expression**:
   `convolution_scale = TEST_FLAG(convolution_bitmap->flags, _bitmap_linear_bit) ? default_scale : vector2d_reciprocal(&convolution_size);`
   (four stages). Evidence: same-CL lab (`scratch/workersC/rasterizer_xbox_screen_effect/lab/lab2-6.c`) shows VC7 flds the
   operand of `local * struct_field` whose value was created later and uses the older one as the memory operand; if/else
   statement defs make every scale field older than the in-branch radius copy (`fld radius; fmul scale`), the conditional
   expression does not (January `fld scale; fmul radius` at all eight blur sites). HCEA selects the scale on the same flag.
3. **Named `real random` local** in the video-pass jitter, assigned before each component's add. January's
   `call real_seed_random; fld noise_scale; fmul st(1) ... fstp st(0)` is the kept-copy pattern of a value with its own
   variable (scan of every January `real_seed_random` site: `seed_random_sites.txt`; same pattern at `units::unit_drop_item`
   and `sound_manager::sound_new_impulse`).
4. Locals renamed to the HCEX names `default_scale` and `viewport_map` (normalized hash unchanged).

## 4. Experiment matrix (whole-TU gate; every sibling stayed exact)

| Shape | Size / relocs / sha16 | Result |
|---|---|---|
| v5 (wave B best) | 2112 / 86 / 670f5c43594c95aa | 10 diff sites |
| t1: v5 + conditional-expression scale selection | 2112 / 86 | 8 blur sites fixed; 2 jitter sites left |
| ja: t1 + `real_local_random()` inline in the expression | 2112 / 86 | inert |
| jd: t1 + random as first operand | 2112 / 86 | inert |
| jb: t1 + named `real random` (per component) | 2128 / 86 / c24f18204f9cfada | EXACT |
| jb2: jb with `real_local_random()` | 2128 / 86 EXACT | rejected: emits new COMDAT owner `_real_local_random` |
| jb3: `random_i` / `random_j` | EXACT | equivalent, not retained |
| jc: `real_vector2d noise_offset` field stores | EXACT | equivalent, not retained |
| c2 (landed): jb + HCEX local names | 2128 / 86 / c24f18204f9cfada | EXACT |

Park re-investigation (scratch only, real parked bodies untouched):

| Shape | `__rasterizer_screen_effect` | Result |
|---|---|---|
| landed file | 3872 / 290 / 40f164b5 | drift from base = callee ABI call site only |
| p_f1: line-584 else arm as `match_vassert(..., FALSE, "### ERROR non-convolution effect tried to render more than 2 passes")` | 3872 / 290 / 40f164b5 | sha-neutral, January literal |
| p_f14: + assert texts `pass==0`, `pass==1`, `combiner_count<=...`, overbright text without spaces | 3872 / 290 / 40f164b5 | sha-neutral, literal owners become January's |
| p_f124: + pass==1 arm as two complete if/else arms | 3872 / 290 / f386d3d3 | arm order matches January, register churn (worse) |

## 5. Do-not-repeat list

- Everything in wave B's list (operand spellings, radius scope/hoist, casts, `constants[k][0]`).
- `real_local_random()` in this TU: emits an extra `_real_local_random` COMDAT owner; keep
  `real_seed_random(get_global_local_random_seed_address())`.
- For the parked caller: arm-order restructuring alone (p_f124) churns the zero-register allocation.

## 6. Residual classification

- `_rasterizer_screen_effect_set_texture_transforms`: closed (strict EXACT).
- `__rasterizer_screen_effect` (parked): measured - January keeps `parameters` in `[ebp+8]`/esi with a zero register in ebx
  saved late (0x1fc), ours pins `parameters` in ebx from 0xab; literal-text defects F1/F4 are real but sha-neutral.
  Class: ABI/register (zero-register pinning). Inference: the remaining allocation depends on source structure not yet
  identified.
- `__rasterizer_screen_flash` (parked): untouched, no drift.

## 7. Reopen criteria

- `__rasterizer_screen_effect`: start from `scratch/workersC/rasterizer_xbox_screen_effect/p_f124.c`; reopen only with
  evidence explaining January's `parameters`/zero-register allocation (e.g. the source statement that spills `parameters`
  to its argument slot before the pixel-shader section).
- `__rasterizer_screen_flash`: unchanged from its park record.

## 8. Disposition

- Newly exact: `_rasterizer_screen_effect_set_texture_transforms` (2128 padded bytes). Final real-file gate
  `== exact 11  residual 2  unwritten 0`; `_point_from_line3d` guard passed; owner census clean; fake-match scan 0 leads.
- Required in the same commit (config, orchestrator): refresh `config/parked.json` `__rasterizer_screen_effect`
  `measurements.base` to size 3872, relocations 290,
  `normalized_sha256 40f164b599db7d921763294f3bdc6207a2841eca8bea40037b2f2e770131bf7b` - the only change is the caller's
  call site moving to January's `mov eax,reg; call` private ABI. Optional sha-neutral park-body literal fix: p_f14.c.

## Orchestrator admission note (checkpoint 5)

The verifier rejected the first landing: its `random` local shadowed a helper name, and the blur-branch products were reordered away from the HEAD/HCEA `x_scale * convolution_radius` spelling. The verifier-corrected candidate (`random_value`, original operand order) was landed instead. It is still strict exact (2,128 padded). In the same commit, the parked caller `__rasterizer_screen_effect` was re-measured, with no body change: its call site follows the callee's January EAX ABI (3872/290, sha 40f164b5, 93.17924 percent), and evidence was appended. Full build: 0 regressions, parks clean.
