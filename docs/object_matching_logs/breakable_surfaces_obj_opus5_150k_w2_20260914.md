# BreakableSurfaces (opus5 150K house-clean lane, wave w2, 2026-09-14): park solved in scratch, owner-blocked on point_from_line3d

Worker lane `opus/150k-house-clean-20260914` (HEAD `ea9175577`). Only `source/physics/breakable_surfaces.c` (and
`source/structures/structure_lens_flares.c`, separate ledger) were in scope. **No production source changed.**
`git diff --stat -- source/physics/breakable_surfaces.c` is empty. The final real-file gate matches the baseline row for row.

## 1. Baseline (real file, whole-TU gate, `_point_from_line3d` guard passed)

`== exact 11  residual 1  unwritten 0  (of 12 listed)`.

| Row | January (padded / relocs / sha16) | Real file |
| --- | --- | --- |
| `_breakable_surface_effect` (PARKED, `instruction-scheduling`) | 4032 / 117 / `510486a7d0a1f2fc` | 4032 / 117 / `678a8125d160643a`; 1156 vs 1155 instructions |

The aligned diff (`tools.campaign.alndiff`, `__real@` annotation artifacts ignored) has two regions:

- **R1** at `+0x37b/+0x389/+0x392`: `cross_product3d(&s_normal, &surface_plane.n, &t_normal)`.
  - January loads `surface_plane.n.k` / `.j` and multiplies by `s_normal.i` in the two `a->i` products.
  - Ours loads `s_normal.i` first.
- **R2** at `+0x97d..+0x9b5`: the t-direction grid offset.
  - January consumes the x87-held `jitter.y` in place (`fxch; fadd [t]`).
  - Ours keeps it (`fld [t]; fadd st(2)`) and pops it later with an extra `fstp st(0)`.

## 2. Duplicate prevention

Read:
- `breakable_surfaces_obj.md`, including:
  - E01-E23 and A01-A06;
  - the 2026-07-28 provenance audit;
  - the 2026-09-01 reconciliation.
- The memory notes on fast_ftol, the unit-local grouping, and the 20->4 correction.
- `claude-breakable-surfaces-20260830/research/breakable_surfaces_closeout/README.md`: about 65 cross/dot probes plus the c2dbg mapping.
- `git log --all` on the file.

What earlier work did and did not cover:
- Every earlier R1 probe varied the cross helper, the dot helper, callers, dataflow or `scale_vector3d`. None varied how the
  **projection block** reaches the plane normal.
- No earlier ledger attributes R2 to the COMDAT-avoidance macro.

## 3. New evidence

1. **HCEX.pdb line records** (`DIA2Dump -lines breakable_surface_effect`, SHIP PDB).
   - The per-surface setup is three single-statement lines:
     - 289 (0xB0 bytes): plane fetch;
     - 290 (0x40 bytes): projection axis;
     - 291 (0x20 bytes): projection sign.
   - The axis is therefore one statement, a helper call, not the open-coded ten-line `fabs` block. Multi-statement code
     elsewhere in the same function gets one record per line (for example lines 344-347).
2. **January's own `projection_from_vector3d` body is authenticated.**
   - It is strict EXACT in `decals.obj`: `source/effects/decals.c:827`, whose atlas record is `i decals.obj ?projection_from_vector3d@@...`.
   - The body is `if (k >= j && k >= i) return _z; else if (j >= i) return _y; else return _x;`.
   - Its `test ah,1` / `jne` compares are exactly the breakable compares.
   - The shared inline in `source/math/real_math.h:967` reads `if (k < j || k < i) return j < i; else return _z;`. That is a
     wrong reconstruction: it returns `_y` when `i` dominates, and its NaN compares differ. This explains why the origin
     mirror's helper call was rejected by bytes in the earlier ledger.
3. **Mechanism of R1** (lab `scratch/workers/structure_lens_flares_breakable_surfaces/lab1.c`-`lab3.c` plus the real TU).
   - For a commutative memory x memory product, VC7 `fld`s the younger operand.
   - Fields read **by name** earlier in the function rank older than fields reached only through pointers (inline-helper
     parameters or pointer locals).
   - Constant-index reads such as `n.n[_x]` behave like names. Pointer-alias and `n[axis]` reads behave like pointers.
   - The open-coded `fabs(surface_plane.n.i/j/k)` named reads age the plane fields and flip R1. Reading them through a
     pointer reproduces January exactly.
4. **R2 needs genuine inline expansion of `point_from_line3d` at both grid sites.** None of these reproduce it:
   - either site alone;
   - the production macro with a do/while or a bare block;
   - any binding order, or a decl/init split;
   - no distance binding, or no pointer bindings;
   - axis-pointer or position-pointer forms.

   Direct statements with a block-scoped `real distance` do fix R2, but they flip the else-arm s/t distance term order.

## 4. Shapes (scratch copies of the current real file, whole-TU gate)

| Shape | File | Result |
| --- | --- | --- |
| S1: pointer-normal projection block, macro kept | `bs_q1.c`, `bs_q2.c` | 4032/117; R1 gone, R2 left (4/3 lines); 11/12 |
| S2: S1 + `point_from_line3d` at the two grid sites | `bs_p1.c`, `bs_p2.c` | **STRICT EXACT, 12/12**; guard fails |
| S2b: S2 + `point_from_line3d` at the velocity sites | `bs_r1.c` | STRICT EXACT, 12/12; guard fails |
| S3: `projection_axis = projection_from_vector3d(&surface_plane.n);` with the January body emulated TU-locally, + grid `point_from_line3d` | `bs_B0.c` | **STRICT EXACT, 12/12**; guard fails |
| S4: COMDAT-free direct grid statements | `bs_q2d1.c`, `bs_e1.c`, `bs_e2.c`, `bs_q2h1.c`, `bs_q2h2.c`, `bs_q2d5a.c`, `bs_q2d5b.c` | 4 / 8 / 8 / 6 / 6 / 4 / 8 lines |
| S5: macro structure variants | `bs_m3.c`-`bs_m8.c`, `bs_q2m3.c`-`bs_q2m8.c` | inert, or 4016 bytes |

Checks on `bs_p2.c`:
- Siblings stay 11/11 EXACT.
- `scratch/parkcheck.py` reports drift only because the park becomes target-exact (`510486a7…`).
- Owner census vs `build/base`: exactly one new owner, `_point_from_line3d`. `bs_B0.c` also emits `_projection_from_vector3d`.
- `tools/fake_match_scan.py`: 0 leads, the same as HEAD.

## 5. Outcome

**NOT LANDED: OWNER-BLOCKED.**
- The park has a strict-exact, house-rule-clean body except for the explicit `_point_from_line3d` no-emission rule.
  This is the same class as `trigger_create_projectiles`, `decal_new_from_collision` and `actor_move_update`.
- R1 is solved by a new lever. R2 has no COMDAT-free natural spelling found.

### Orchestrator proposals

- **A (owner ruling).** Add `_breakable_surface_effect` (4,032 B) to the `point_from_line3d` policy list. The header-free exact
  body is `bs_p2.c`, three hunks against HEAD:
  - `real_vector3d const *normal = &surface_plane.n;` at the top of the while body;
  - `fabs(normal->i/j/k)` in the projection block, and `projection_sign_from_vector3d(normal, projection_axis)`;
  - `point_from_line3d(&position, &s_plane.n|&t_plane.n, …)` at the two grid sites.
- **B (header body fix, `source/math/real_math.h:967`).** Replace the `projection_from_vector3d` body with the January
  `decals.c:827` body, which declares i/j/k with `fabs` and returns `_z`/`_y`/`_x`.
  - Direct callers:
    - `decals.c`: has its own definition via rename, so it is unaffected;
    - `real_math.c:1006`;
    - `collision_features.c`: 3 sites;
    - `leaf_map.c`: 4 sites;
    - `structures.c:373`.
  - TU-local emulation (`emu_proj.py`):
    - collision_features stays 20/20;
    - real_math stays 80/4;
    - structures stays 10/7;
    - **leaf_map goes 16 -> 17, because `_leaf_face_get_vertex3d` (176 B) becomes EXACT.**
  - Every TU includes `real_math.h`, so this needs the full build. It changes body text only; the declaration count is unchanged.
  - With B landed, the authentic breakable spelling is S3 (the single-statement helper call HCEX line 290 attests). S3 still needs ruling A.

### Reopen criterion

Reopen when either:
- the owner admits `point_from_line3d` emission for inline-expanding TUs (S2/S3 are then exact as measured); or
- a COMDAT-free natural spelling reproduces the inline-expansion node order at both grid sites.
