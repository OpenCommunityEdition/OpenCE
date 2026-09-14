# `render_cameras.obj`: Opus5 100K wave D park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/render/render_cameras.c`, target `build/split/source/render/render_cameras.obj`.
- Compiler VC7 13.00.9254, object flags `/nologo /c /O2 /Oy- /Ob1` (config.json), gate
  `tools/campaign/gate.py ... --forbid-emitted-symbol _point_from_line3d`.
- Evidence: January target COFF (authority), HCEA `src/blam/render/render_*.c` (semantics), Stian build-2276
  `src/halo/render/render_cameras.c` (build_frustum lift), repo precedent for float-bit printing
  (`rasterizer_geometry.c`, `units.c`). No Ghidra/atlas output was prepared for this object.
- Worker notes with every measurement: `scratch/workersD/render_cameras.md`; scratch objects/sources under
  `scratch/workersD/render_cameras/`.

## Validated baseline (lane HEAD)

`== exact 14  residual 7  unwritten 0` — 2,896 / 10,256 padded code bytes exact. Data unchanged by this wave.

| residual | target padded/relocs | base | first divergence |
| --- | --- | --- | --- |
| `_render_camera_build_frustum` (parked) | 3376 / 113 | 3408 / 112, frame 0x84 vs 0x6c | `sub esp` |
| `_render_frustum_cube_view_fraction` (parked) | 1200 / 78 | 1200 / 78 | +0x11f slot of projection locals; negated inverse precomputed |
| `_render_camera_mirror` | 864 / 28 | 864 / 28 | +0x64 double literal `3f8999999999999a` vs `3f899999a0000000` |
| `_render_frustum_cube_visible` | 720 / 4 | 720 / 4 | +0xe2 dot-product operand order |
| `_render_camera_view_to_screen` (parked) | 480 / 30 | 480 / 30 | +0x176 viewport x0 load order/register |
| `_render_frustum_sphere_visible` (parked) | 432 / 6 | 432 / 6 | +0x140 return-0 block placement |
| `_render_camera_build_frustum_bounds` (parked) | 288 / 9 | 288 / 9 | +0x53 integer register choice |

## Accepted controls

No production source change. `git diff --stat -- source/render/render_cameras.c` is empty; final real-file gate is the
baseline line, all five parks equal `measurements.base`, owner census identical, fake scan 0 leads.

Object-level control: reordering all 21 definitions into January section order (helper first) leaves every function's
size, relocations and normalized hash unchanged — definition position is not a lever in this TU.

## Park-reopen proposal: `_render_frustum_cube_view_fraction` (strict EXACT in scratch)

Candidate `scratch/workersD/render_cameras/park_reopen_cube_view_fraction.c` (only this function differs from the real
file). January materialises `-inverse_z0` lazily inside the first edge expression (`fld inv; fchs; fstp tmp` after the
`z0*offset_x` and `x0*projection_x` leaves) and reuses the dead projection-local stack homes. The baseline's
precomputed `positive_inverse_z0/z1` locals force the negation at its own statement; plain `* -inverse_z0` is folded
into `fmul; fchs`. Spelling the factor `(-inverse_z0)` / `(-inverse_z1)` inside the MIN/MAX edge arguments (PAREN LAW,
worker brief section 10) is strict EXACT: 1200 bytes, 78 relocations, normalized SHA-256 prefix `bcc496e4a2fc55aa`.
The MIN/MAX macros duplicate each argument into both `?:` arms, so the parenthesised factor is an ordinary spelling of
`(edge numerator) * (-inverse_z)`; the change removes two locals and adds nothing. Whole-TU scratch gate 15 exact,
no sibling loss, other parks at base, owner census identical, guard passed, fake scan clean.

## Experiment matrix (scratch only)

| function | shape | result |
| --- | --- | --- |
| cube_view_fraction | cv1 inline `* -inverse_zN` | 1200/78 not exact (negation folded) |
| | cv2 `real positive_inverse_z0 = -inverse_z0;` at declaration | 1184 (computed before asserts) |
| | **cv3 `* (-inverse_zN)`** | **strict EXACT** |
| sphere_visible | sv1 `return (...) ? 2 : 1;` | identical to base |
| | sv2 `if (...) return 2; return 1;` | 448 (three duplicated epilogues) |
| | sv3 single `||` reject chain with assignments | identical to base |
| view_to_screen | vs1 plain named height/width reals (no int locals) | identical to base (cleaner equivalent body) |
| | vs2 `rectangle2d const *viewport` binding | identical |
| | vs3 width declared first | identical |
| build_frustum_bounds | fb1 y rows `- wb.y1 - wb.y0` | identical (VC7 canonicalises) |
| | fb2 eight long locals (HCEA) | worse (ebx, frame 0xc) |
| mirror | m1 `< 0.0125f` | literal identity fixed; one 2-insn x87 keep/consume hunk remains |
| | m2 rows `position + n*offset` | identical to m1 |
| | m3 rows via inline point_from_line3d (diagnostic) | identical to m1 |
| | m4 single-def `?:` offset | worse |
| cube_visible | cb1 direct `&cube_vertices[vertex_index]` args | dot orders reshuffle, not exact |
| build_frustum | bf1-bf24 (see notes) | best bf15: 3376 bytes, frame 0x6c exact, 112/113 relocs, 60 hunks |

## Build-frustum structural findings (scratch progress, not landable)

Measured facts reproduced in `scratch/workersD/render_cameras/park_progress_build_frustum.c` (contains a diagnostic
`_pi` override, so it is evidence only): lexical blocks for the basis vectors, the six-plane normal/plane pair, and the
far-corner `real_rectangle2d` + corner/midpoint view points reproduce January's 0x6c frame exactly; the FOV diagnostic
reads the float bits directly (`*(long *)&camera->vertical_field_of_view`); `far_x_scale = -(inverse * z_far)` locals;
chained `world_bounds.x0 = world_bounds.x1 = world_vertices[0].x`; direct `world_vertices[vertex_index]` indexing in the
AABB loop; `((half_bounds_width / viewport_height * viewport_width) * field_of_view_tangent)` grouping.

**Header blocker (ORCHESTRATOR-PROPOSAL):** January `__real@40490e38` for `_pi - _real_epsilon` is a float-precision
fold; `#define _pi ((real)M_PI)` folds in double precision (`40490e37`). A float literal (`3.14159265359f`) reproduces
`40490e38`; `((real)(float)M_PI)` and `/Op` do not. Independent witness: `render_sprite.obj`
`angle*angle/((_pi/2)*(_pi/2))` is January `__real@3ecf817a` (float pi) vs our `3ecf817b`. A whole-board split/base
scan of one-ulp `__real@` mismatches finds only these two pi-derived constants plus one unrelated. Proposed text:
`#define _pi 3.14159265359f` in `source/math/real_math.h`; requires a full-board rebuild and per-function diff.

## Do-not-repeat

- Reordering function definitions; declaration-order permutations of build_frustum/view_to_screen locals.
- Operand respellings of commutative products/sums (VC7 canonicalises): far corner, midpoint, fabs terms, window
  subtraction order, dot products inside cube_visible.
- sphere_visible reject/return respellings (sv1-sv3) and the earlier Jonas list.
- mirror refraction offset `?:` single definition; inline point_from_line3d (forbidden symbol anyway).

## Residual classification

- cube_view_fraction: solved in scratch (park-reopen proposed).
- build_frustum: header constant blocker (proven) plus x87 rank/phi scheduling (unresolved): January forwards
  `view_plane.n.i` and `n.k` as x87 phis in the projection else-branch (`fld 0.0` = the 113th relocation), cross
  product k-row, far/midpoint product operand order, warning dot-product operand order.
- mirror: x87 keep-vs-consume of the refraction phi on the last row (scheduling); literal proven `0.0125f`.
- cube_visible: commutative rank lottery of inlined dot products (scheduling).
- view_to_screen, build_frustum_bounds: integer register/CSE-placement ties.
- sphere_visible: return-block layout / 16-bit if-conversion tie.

## Reopen criteria

- build_frustum: `_pi` float-literal header landed AND a law for which struct fields VC7 forwards as x87 phis at an
  if/else join.
- mirror: a form whose refraction offset survives the last row; keep `0.0125f`.
- cube_visible: a measured ranking rule for inlined dot products in a strength-reduced loop.
- sphere_visible: a sibling with known source lowering to `mov ax,imm; jcc exit; mov ax,imm` plus inline return block.
- view_to_screen / build_frustum_bounds: new register-allocation evidence (not respelling).

## Disposition

No new exact credit in the real file. One park-reopen proposal (1,200 padded bytes) and one header proposal (`_pi`).
Object remains NonMatching at 14/21.
