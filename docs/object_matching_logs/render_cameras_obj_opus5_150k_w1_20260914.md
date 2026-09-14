# `render_cameras.obj`: Opus5 150K house-clean wave w1 (2026-09-14)

## Scope and baseline

- Translation unit `source/render/render_cameras.c`, target `build/split/source/render/render_cameras.obj`,
  lane `opus5-150k-house-clean-20260914` at base `12f7375d4`.
- Baseline gate (`--forbid-emitted-symbol _point_from_line3d`): `== exact 15  residual 6  unwritten 0`.
- Non-exact rows: `_render_camera_build_frustum` 3376 (parked; 3408/112 vs 3376/113),
  `_render_camera_build_frustum_bounds` 288 (parked), `_render_camera_mirror` 864, `_render_camera_view_to_screen`
  480 (parked), `_render_frustum_cube_visible` 720, `_render_frustum_sphere_visible` 432 (parked).
- Inherited exact, not touched: `_render_frustum_cube_view_fraction` (100K wave D reopen).
- Ledgers read first: wave D 20260914 (and its worker notes), frustum natural reconstruction 20260913, Jonas cube
  fixed point / sphere boundary 20260829, sphere diameter 20260903, narrow owner pair 20260909, Fable owner trial
  20260906. `branch_sweep`: no blob beats the current file. The `_pi` float-literal header proposal is still not
  landed in this lane.
- Worker notes with every measurement: `scratch/workers/render_cameras.md`; scratch sources/objects/alignments in
  `scratch/workers/render_cameras/`.

## Outcome

No production source change. `git diff --stat -- source/render/render_cameras.c` is empty. Final real-file gate is
the baseline line, guard passed, all four parks at their recorded measurements (`parkcheck`: drift 0), owner census
unchanged by construction (file identical to HEAD), fake scan 0 leads. Exact credit delta: 0.

## Per-function results

| function | result | key evidence |
| --- | --- | --- |
| `_render_camera_mirror` | not landed | Only defect after wave D's proven `0.0125f` literal: refraction z row consumes the x87 offset (`fmul n.k`) where January keeps it (`fld n.k; fmul st(1) ... fstp st(0)`). Nine probes: default-init, function-scope, shared variable, duplicated tail, duplicated rows, `== 0` polarity, nested scope and in-place accumulation. None moved it. The reflection branch's single-definition value is kept in every probe, and the refraction phi is consumed in every probe. |
| `_render_frustum_cube_visible` | not landed | Residual is only the commutative term/operand order of four inlined `plane3d_distance_to_point` dot products. New atlas fact: `render_frustum_build_point_flags` is an inline/COMDAT function in January (`i` record). Measured `__inline` calls, however, stay out of line under `/Ob1`. The 512-byte/1-relocation result shows January open-codes the loop. `long` index unrolls the loop (1856); `if/SET_FLAG` loses the ?: lowering (688); a single `|` expression is worse (736). |
| `_render_frustum_sphere_visible` (park) | not reopened | Single-exit / return-block-specialisation law tested (sv4-sv8). Specialisation turns on all-or-nothing: separate epilogues with 32-bit loads, or none. January specialises only the 0 path; 2/1 share a 16-bit phi exit. |
| `_render_camera_view_to_screen` (park) | not reopened | **vs4**: `long viewport_x0` declared before `viewport_height` gives January's exact instruction order. Only a 3-register rotation remains (y0/y1/x0 = edx/ecx/eax in January). |
| `_render_camera_build_frustum_bounds` (park) | skipped (exhausted) | Window-y rank/register tie; `(real)` cast inert. |
| `_render_camera_build_frustum` (park) | scratch progress | **bf2**: the projection else-branch is written as `set_real_vector3d(&view_plane.n, 0.0f, 0.0f, 1.0f);` instead of the chained field stores. This reproduces January's x87 phi forwarding of `n.i` and `n.k`, the `mov [n.j],0` store and the 113th relocation (`fld 0.0`). The relocation count is now equal (3376/113). bf1 (separate field statements) is inert, so the inline helper's parameter temporaries are the lever. 57 x87 rank hunks remain: cross-product k row, far bounds, corner z store timing, midpoint, projection `z_far` dup and fabs load order, and warning-check operand/id placement. |

## Proposals

- **Header (carried forward, unchanged):** change `source/math/real_math.h` `#define _pi ((real)M_PI)` to a float
  literal (`3.14159265359f`). Witnesses are January `__real@40490e38` (render_cameras) and `__real@3ecf817a`
  (render_sprite); see the wave D ledger. Necessary but not sufficient for build_frustum.
- **Names-only (no credit):** January map atlas `00174fc0 exact render_cameras.obj _render_camera_warn_once`. Rename
  the static `render_camera_check_warning_condition` (a PC-corpus name) to `render_camera_warn_once`:
  - Change `config/symbols.json` line 6283 to `"_render_camera_warn_once"`, keeping `"static": true`.
  - Rename the prototype, the definition and the 22 calls in `render_cameras.c`.
  - Proof: `scratch/workers/render_cameras/rn1.c` gated with
    `--alias render_camera_warn_once=render_camera_check_warning_condition` keeps exact 15 / residual 6.
  - The calls inside the parked build_frustum body change text only.

## Do-not-repeat (additions)

- mirror:
  - default-initialised or function-scope offset;
  - one variable shared by both branches;
  - rows or tail duplicated into branches;
  - `== 0.0f` polarity;
  - nested scope;
  - in-place offset accumulation.
- cube_visible:
  - `__inline` build_point_flags calls (not inlined under `/Ob1`);
  - `long` index;
  - statement `SET_FLAG` form;
  - single `|` expression.
- sphere_visible:
  - single-return forms, with a `||` chain + result, nested positive ifs, or `?:` result;
  - one `return 0` + `?:` return;
  - return 0/2/1 statements.
- build_frustum: `n.i = 0; n.j = 0; n.k = 1` as separate statements (inert); fabs operand swap (canonicalised).
- build_frustum_bounds: `(real)` cast on the window height.

## Reopen criteria

- mirror: a measured VC7 rule for when a phi-joined x87 value is kept rather than consumed at its last use (keep
  `0.0125f`).
- cube_visible: a measured ranking rule for inlined dot-product sums inside a strength-reduced loop.
- sphere_visible: a same-compiler sibling with known source that specialises exactly one constant return path.
- view_to_screen: register evidence for x0 in eax, starting from vs4.
- build_frustum: the `_pi` header landed and the remaining x87 rank ties explained, starting from bf2. The scratch
  body contains a diagnostic `_pi` override and is evidence only.
