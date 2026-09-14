# `render_cameras.obj`: Opus5 150K house-clean wave w3 (2026-09-14)

## Scope and baseline

- Translation unit `source/render/render_cameras.c`, target `build/split/source/render/render_cameras.obj`,
  lane `opus5-150k-house-clean-20260914` (HEAD `210aa8f81`). Second pass after w1; no w2 ledger exists for this object.
- Baseline gate (`--forbid-emitted-symbol _point_from_line3d`): `== exact 15  residual 6  unwritten 0`, guard passed.
- Non-exact rows: `_render_camera_build_frustum` 3376 (parked; 3408/112 vs 3376/113), `_render_camera_build_frustum_bounds`
  288 (parked), `_render_camera_mirror` 864, `_render_camera_view_to_screen` 480 (parked), `_render_frustum_cube_visible`
  720, `_render_frustum_sphere_visible` 432 (parked). Inherited exact, not touched: `_render_frustum_cube_view_fraction`.
- Read first: every `render_cameras_obj_*` ledger (w1, wave D, natural frustum reconstruction, Jonas leaf / typed
  projection / adjusted FOV / cube fixed point / sphere boundary, sphere diameter, narrow owner pair, Fable owner trial,
  prototype owner split), the w1 worker notes and scratch sources, `scratch/w2/laws_w2.md` (A1-A22, R1-R12, D), and the
  ai w2 x87 split-enregistration research lead (lists build_frustum).
- A previous w3 run had been cut off after writing only a preamble; its leftover scratch (bf4/bf5/bf5a) was re-measured.
- Worker notes: `scratch/workers/w3_render_cameras.md`; scratch sources/objects in `scratch/workers/w3_render_cameras/`.

## Outcome

No production source change. `git diff --stat -- source/render/render_cameras.c` is empty. The final real-file gate
equals the baseline line, the guard passed, all four parks are at their recorded measurements (`parkcheck` drift 0),
the owner census is unchanged by construction, and the fake scan reports 0 leads. Exact credit delta: 0.

## Law detectors applied (w2 A-laws)

| function | detector | finding |
| --- | --- | --- |
| `_render_frustum_sphere_visible` (park) | A3(c) return width | January's 2/1 loads are 16-bit (`mov ax,2; jnp exit; mov ax,1`), so they are return-statement constants rather than a short result variable (our `result = 2/1` emits `mov eax`). Block order [1-block][exit] implies `return 1` precedes `return 2`. |
| `_render_camera_build_frustum` (park) | A4 helper routing | `set_real_point3d(&view_point, x, y, -camera->z_far)` for the four far corners reproduces January's corner store timing: 57 -> 48 alignment hunks. |
| `_render_camera_build_frustum` (park) | A11 explicit cast | Removing `(real)` from `projection_scale` is byte-identical (inert). |
| `_render_camera_mirror` | A11, A10, A4, x87 phi | No reconstruction cast is present. A10 = w1 m2 (worse) and A4 = wave D m3 (identical). The x87 phi law does not apply. |
| `_render_frustum_cube_visible` | A22 dot canonicalisation | January mixes i,j,k (plane 0), i,k,j (plane 1) and k,j,i (planes 2/3) in one loop body; A22 does not explain it. |
| `_render_camera_view_to_screen` (park) | A3, A10 | The result phi and specialised epilogues are already identical; only a 3-register rotation remains. |
| `_render_camera_build_frustum_bounds` (park) | A11 | The only cast is the required int->real conversion (fidiv proves an int divisor); w1 fb3 was inert. |
| all | A7 C4013 census | `scratch/w3/c4013.txt` has no render_cameras entries. |

## Per-function results

- **sphere_visible (park, not reopened).**
  - Shape S1 (`sv_s1.c`): the base plane tests followed by `if (!(d0 < nr && ... && d5 < nr)) { return 1; } return 2;`.
    It measures 448 [size], with three specialised epilogues and the zero block still at the end, the same structure as
    w1 sv8.
  - A3(c) explains the load width but not January's inline zero-block placement or the shared 2/1 exit.
- **build_frustum (park, scratch progress).**
  - bf4 reproduces w1 bf2 (3376/113, 57 hunks).
  - bf5 (corners and midpoint through the helper) gives 50 hunks.
  - **bf5a (corners only) gives 48 hunks.**
  - bf6 (bf5a without the `(real)` cast) is identical to bf5a.
  - All of these carry w1's diagnostic `_pi` override, so they are evidence only.
  - Remaining clusters:
    - operand order in the first `cross_product3d(&camera->forward, &camera->up, ...)`, where both operands are
      camera fields;
    - the far-bounds product;
    - the midpoint `inverse * half_z` operand order;
    - in the projection, the `z_far` CSE dup and the fabs term order;
    - vertex-vs-plane fld order in the warning dot products, plus `mov eax,id` / `fld [esi+8]` placement.
- **mirror (not landed).**
  - No shape was compiled: every law-derived candidate had already been measured.
  - Mechanism note: the reflection branch's single-definition, block-local `mirror_scale` is popped lazily at scope end.
    Every multi-definition or `?:` refraction spelling (including the 100K m4, re-diffed) is consumed at its last use,
    whereas January keeps it.
- **cube_visible, view_to_screen, build_frustum_bounds:** skipped as exhausted; no law yields a new shape.

## Proposals (carried forward, unchanged)

- Header: `source/math/real_math.h` `#define _pi ((real)M_PI)` -> `#define _pi 3.14159265359f` (wave D / w1 evidence:
  January `__real@40490e38` in render_cameras and `__real@3ecf817a` in render_sprite). Necessary for build_frustum's
  size/relocation equality, but not sufficient.
- Names only, no credit: symbols.json line 6283 `_render_camera_check_warning_condition` -> `_render_camera_warn_once`
  (atlas `00174fc0 exact render_cameras.obj _render_camera_warn_once`), plus the 24-site rename in render_cameras.c
  (w1 `rn1.c` proof).

## Do-not-repeat (additions)

- sphere_visible: `return 1` before `return 2` with a negated `&&` classification (S1). This makes 4 return-shape
  families measured at 448.
- build_frustum: removing the `(real)` cast on projection_scale (inert); midpoint through set_real_point3d (worse than
  corners only).
- mirror: single-def `?:` offset (re-confirmed as consuming).

## Reopen criteria

- build_frustum: `_pi` header landed AND a rank law for same-base memory x memory products (camera cross product,
  warning dot products). Start from `scratch/workers/w3_render_cameras/bf5a.c`.
- sphere_visible: a same-compiler sibling with known source where two 16-bit constant returns share the exit while
  exactly one constant return path is specialised.
- mirror: a measured VC7 rule for when a phi-joined x87 variable is popped at scope end rather than consumed; keep
  `0.0125f`.
- cube_visible: a ranking rule explaining mixed dot-product term orders inside one strength-reduced loop body.
- view_to_screen / build_frustum_bounds: register-allocation evidence (not respelling).
