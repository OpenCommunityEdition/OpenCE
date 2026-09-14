# `rasterizer_xbox_water.obj` — opus5 150K lane, wave w2 (2026-09-14)

## Result

| item | value |
| --- | --- |
| real file baseline gate (guard passed) | exact 13 / residual 1 / unwritten 0 |
| real file final gate | unchanged: exact 13 / residual 1 / unwritten 0 (`git diff --stat` empty) |
| parked target | `_rasterizer_water_draw`, 2,240 padded / 171 relocations / sha16 `bc1ccd223540f3d7` |
| scratch candidate | `scratch/workers/rasterizer_xbox_water/park_reopen_candidate.c` |
| candidate measurement | **strict EXACT** 2,240 / 171 / `bc1ccd223540f3d7`, whole TU 14/14 exact, no alias |
| disposition | **park-reopen proposed** (orchestrator verifies and unparks in a dedicated commit) |

Nothing was landed in the real file, because the function is parked.

## Baseline and duplicate prevention

- Before this wave, the real file compiled to 2,320 bytes with 173 relocations and a `sub esp, 0x4c` frame. January's frame is `0x44`.
- Ledgers read:
  - `rasterizer_xbox_water_obj_jonas_leaf_wave.md`;
  - `..._jonas_frame_visibility_20260828.md`;
  - `..._opus_reconciliation_20260903.md` (do-not-repeat: pointer pun, pass collapse, aliases/volatile/asm);
  - `..._real_alpha_inline_reconciliation_20260903.md`;
  - the `config/parked.json` entry;
  - the 100K handoff.
- `git log --all` and `branch_sweep.py` found no exact body on any ref. The best historical blob is 13/1/0.
- The function is not in `build/semantic_report.json` or in the 100K admission manifest.

## Evidence and the five source-shape differences

The park classified the residual as "broad local/register and x87 scheduling". Alignment (`alndiff`/`relocdiff`) shows five specific source-shape differences instead. Everything before `+0x637` was already identical.

### 1. Third `PIN` evaluation calls `_dot_product3d` out of line

**January evidence.**
- The call is at `+0x816`: `push ecx` / `push offset global_window_parameters+0x14` / `call _dot_product3d`.
- `ecx = lea [edi+0x80]` is hoisted before the inline `magnitude3d`.
- Our body inlined all three evaluations.

**Established law.** The WORKER_BRIEF inline-frequency argument-kind clause, as measured in `rasterizer_xbox_transparent_geometry_obj_opus5_100k_waveC_20260914.md` section 5:
- `&global.member` never goes out of line;
- pointer-valued arguments do go out of line past a frequency threshold.

**First shape.** `struct render_camera const *camera = &global_window_parameters.camera;` with `-dot_product3d(&camera->forward, &group->plane.n)`. This is the verifier-accepted spelling from the transparent_geometry wave. It uses the genuine `struct rasterizer_window_begin_parameters` owner from `rasterizer.h`, which 10 other rasterizer TUs already use for this extern. It replaces the TU shadow struct `rasterizer_water_window_parameters`.

With this shape alone the call was still inline, so the block was one condition short. In-function probes, which are diagnostic only and not candidates:

| probe | change | effect on the call |
| --- | --- | --- |
| x_exp7 | extra `&&` on the magnitude test | moved out of line |
| x_exp9 | extra `&&` on the outer debug test | moved out of line |
| x_exp13 | 4th `&&` operand on the draw-before-fog condition | stayed inline |
| v7 | De Morgan inversion of that if/else | stayed inline (object sha identical) |
| x_exp15 | a dead boolean local | stayed inline |

**Second shape.** `draw_depth_only = TEST_FLAG(...) && !TEST_FLAG(...) && !TEST_FLAG(...); if (draw_depth_only)`. This reproduces the out-of-line call at January's exact address, with no other code change.

**New measured VC7 law (else-arm frequency).** The else arm of an N-operand `&&` if is hot (about 1 - 2^-N) and does not count toward the inline threshold. Testing a boolean local that holds the same chain makes the else arm a one-condition (1/2-frequency) block. Code layout is identical either way, because the single-use boolean is folded into the branch.

### 2. Vertex constants use a non-constant aggregate initializer

**January evidence.**
- `ripple_scale` is read once (`mov edx`; `mov eax, edx`), with no store to the address-taken array in between.
- `fcos` is evaluated before the eight zero stores.
- Ghidra `decomp.c:903-923` shows the same.

**Rejected shapes.** Two-statement stores re-read the field. Chained `a = b = scale` and `b = a` both went through the x87.

**Shape.** A scoped block holding `real vertex_constants[3][4] = {{scale, scale, (real)cos(angle)*vel*time, (real)sin(angle)*vel*time}, {0...}, {0...}};`. The block is byte-identical.

**Precedent.** Non-constant aggregate initializers appear in EXACT `_fast_vector_intersects_sphere` (`real_math.c:417`) and `_object_set_position` (`objects.c:3201`).

### 3. Combiner stage 0/1 setup is inside both arms

**January evidence.** `mov al, [esi+0x28]` (the `water->flags` fog test) is read *before* the `rgb_outputs[0]`, `rgb_inputs[0]` and `rgb_inputs[1]` stores. A store to the global `pixel_shader` cannot be crossed by a pointer read (store/call-order law). The stores are therefore in both arms, and VC7 hoists the identical prefix.

**Shape.** The full per-configuration stage setup in each arm. The block is identical.

### 4. Mipmap LOD bias bit transfer uses a local union

**January evidence.**
- No `_csmemcpy` relocation.
- The frame is 8 bytes smaller (frame-size law: there is no separate bits slot).
- The negated bias is stored with `fstp [ebp+8]` (parameter-home reuse).
- The DWORD is read at the argument push, *after* `mov edx, 0x10`. The read is therefore part of the argument expression, not a prior statement.
- `D3D8Types.h:609` documents `D3DTSS_MIPMAPLODBIAS` as a float carried in the DWORD argument.

**Shapes measured.**

| form | result |
| --- | --- |
| intrinsic `memcpy` (the `#undef memcpy` precedent) | frame and slot correct, but the load comes before `mov edx, 0x10` (one-instruction order difference) |
| pointer pun | exact, but **not proposed** because it was rejected earlier |
| `union { real value; unsigned long bits; } mipmap_lod_bias;` | **exact** |

**Precedent for the union.** The same local union is used in EXACT `_error_heap` (`path_obstacle_avoidance.c:334`) and in `render_cameras.c:1174`. The union is implementation-defined in C89, defined in C99 TC3/C11, and documented by MSVC. The representation copy is the API's semantic requirement; it is not a codegen device.

### 5. Rejected: `cosine()` / `sine()`

These are also exact, but they emit new `_cosine` and `_sine` COMDAT owners that `build/base` lacks. The candidate keeps `(real)cos` / `(real)sin`.

## Admission checks on the candidate

| check | result |
| --- | --- |
| whole-TU strict gate | 14/14; the only row change is draw residual → EXACT; all 13 baseline-exact rows unchanged |
| `_point_from_line3d` guard | passed |
| `scratch/parkcheck.py` | only drift is the reopened park itself, now at the target hash |
| owner census vs `build/base` | no new or missing owners |
| data sections vs baseline candidate | identical, except the debug object-path stamp (gate artifact) |
| `tools/fake_match_scan.py` | 0 leads (HEAD: 0 leads) |
| house rules | typed owner struct, `real` / `boolean`, `TEST_FLAG`, `match_assert` unchanged, explicit returns; no volatile, pragma, asm, raw offsets, fake conditions, or dead code |

## Park criterion

The park reads "Reopen only for authoritative January source/local records or a natural same-compiler donor explaining the lifetime schedule". Its premise of unidentified scheduling is refuted. Every difference was a source shape, each proven by January code and relocation facts and explained by an established reusable law:
- the inline-frequency argument-kind clause;
- store/call order;
- the frame-size gap;
- the new else-arm frequency measurement.

No header or config text is needed beyond removing the `parked.json` entry in the unpark commit.

**Policy item for the orchestrator.** Shape 4 is the local-union bit transfer (with precedent), not the rejected pointer pun.

## Candidate diff summary (vs the real file)

- File level:
  - removes `struct rasterizer_water_window_parameters` and its `verify_*` typedef;
  - changes the extern to `struct rasterizer_window_begin_parameters`.
- Function locals:
  - adds `boolean draw_depth_only`;
  - replaces `real mipmap_lod_bias` and `unsigned long mipmap_lod_bias_bits` with the union;
  - moves `vertex_constants` into a scoped block with an initializer.
- Body:
  - condition hoisted into `draw_depth_only`;
  - stage 0/1 combiner stores moved into both fog arms;
  - `camera` local in the tint block;
  - `mipmap_lod_bias.value` / `.bits` replace the `csmemcpy` call.
