# `collision_bsp.obj`: Opus5 100K wave D park re-investigation (2026-09-14)

Additive ledger. Wave D reopened the seven parks in this object in scratch only. Two parks
now match exactly and are proposed for reopening. The other five are unchanged in the real file.
Nothing was landed. `git diff -- source/physics/collision_bsp.c` is empty.

## 1. Scope and provenance

- Unit `source/physics/collision_bsp.c`, target `build/split/source/physics/collision_bsp.obj`.
- Lane `C:\halo-worktrees\opus5-100k-consolidated-20260914`, HEAD `548451f6d`. Real-file SHA-256
  `88e9204051204fcb5d658cd88bf433dffd9efb0bfe98895be868e07d49b6d01b` (unchanged).
- Compiler: VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox` (build.ninja recipe, unchanged).
- Evidence: January COFF (the byte authority) and `config/symbols.json`. HCEA `blam/physics/*` lifts
  and `headers/test_pill_data.h` were used for semantics only. No Ghidra or atlas file exists for
  this object.
- Prior ledgers read: `collision_bsp_obj_sphere_cluster_20260904.md`,
  `collision_bsp_obj_closest_point_fuzzy_20260904.md`,
  `collision_bsp_obj_jonas_closest_point_first_shot_20260829.md`,
  `collision_bsp_damage_owner_reconciliation_20260907.md`, and the collisions wave C FPU law ledger.
- `branch_sweep`: no history blob has more than 23 exact functions.

## 2. Validated baseline

Gate with the `_point_from_line3d` guard: **exact 23 / residual 7 / unwritten 0**. That is 5,568 of
10,544 padded code bytes. Data owners match `build/base`. Every residual is parked:

| Function | Target | Base | Gate tag |
|---|---|---|---|
| `_bsp2d_test_sphere_recursive` | 160/3 | 160/3 `bbd48caa` | sha, one operand pair |
| `_bsp2d_test_pill_recursive` | 272/6 | 272/6 `fbfc5617` | sha, one operand pair |
| `_bsp3d_test_sphere_recursive` | 800/22 | 800/22 `f0b2e672` | sha, 290 vs 287 insns |
| `_collision_bsp_test_vector_recursive` | 800/13 | 784/13 `76261579` | size |
| `_bsp3d_test_pill_recursive` | 1504/31 | 1472/28 `2811fc4e` | size, relocs |
| `_collision_surface_find_closest_point2d` | 560/11 | 560/11 `044384f2` | sha |
| `_collision_surface_test_sphere` | 880/13 | 848/13 `b8b40ff3` | size, packed SSE |

## 3. New evidence

1. **The COMDAT objection is refuted.** The bsp2d sphere park rejected a
   `plane2d_distance_to_point` call only because it emits a `_plane2d_distance_to_point` COMDAT
   that the split target lacks. But VC7 emits a select-any COMDAT for every *used* header
   `__inline`, even when every call is inlined. `build/base` already carries 20 such
   candidate-only COMDATs with zero out-of-line references to them. January's retained copy sits at
   file offset 1003456 (0xF4FC0) inside geometry.obj, which is linked well before this object
   (0x136C00). A split object can therefore never show it.
2. **The projection-sign helper is already emitted.** `projection_sign_from_vector3d` produces
   January's int-width compare materialisation (`mov eax,1 / xor eax,eax`). Its COMDAT is already in
   `build/base`, so the sphere ledger's objection no longer applies.
3. **January's third `_project_point3d` call** in `bsp3d_test_pill_recursive` projects the in-plane
   sweep vector into `&data->vector2d`. The same subsystem already uses this idiom in exact code:
   `collision_features.c:1012` passes `(real_point3d const *)&v3d` and `(real_point2d *)&v2d`.
4. **The packed SSE reduction is unique.** A byte scan of every code section under `build/split`
   finds the squared-distance form (`shufps 0x0e`, then `shufps 0x39` with `subps`) in exactly one
   function: `collision_surface_test_sphere`. `distance_squared3d` has about 65 other January
   callers, all scalar. The construct is therefore local to this object, and no header proposal
   can explain it.

## 4. Experiment matrix (all scratch; base = real file)

| ID | Shape | Result |
|---|---|---|
| s2a | `plane2d_distance_to_point(&node->plane, center2d)`, keeping the pointer local | same one-pair diff |
| s2b/s2d | manual helper text with or without the local | same diff (spelling inert) |
| **s2c** | `plane2d_distance_to_point(&node->plane, &data->center2d)`, local removed | **bsp2d_test_sphere EXACT** 160/3 `3d3e2ebc` |
| p2a/p2c/p2d | helper for distance0 plus `dot_product2d` for distance1 (either argument order) | distance1 operand flipped |
| p2b | helper for distance0, old manual distance1 | EXACT |
| **p2f** | helper for distance0 plus `n.i*v.i + n.j*v.j + distance0` | **bsp2d_test_pill EXACT** 272/6 `40f743a1` |
| combo1 | s2c + p2f | 25 exact / 5 residual; other parks unchanged; +1 COMDAT; fakescan 0 |
| t3a | `-plane3d_distance_to_point(plane, data->center)` + `projection_sign_from_vector3d(&plane->n, projection)` | bsp3d_test_sphere 287/287 insns; boolean width fixed; 3-insn y/z term order left |
| t3b | `point_from_line3d` (knowledge probe only, violates guard) | worse |
| t3c | inline t argument | same as t3a |
| t3d | `fabs` locals first | worse |
| t3e | named center pointer | worse |
| t3f | manual HCEA term order | worse |
| v1 | HCEA `if (result->t > t)` | predicate parity flips |
| v3 | straddle-first block order | worse |
| v4 | single function-bottom `return FALSE` | TRUE join fixed; else arm misplaced |
| **v5** | v4 + straddle-first `else if` + `<=` early FALSE | test_vector 800/13, 317/317 insns; one i-term operand pair left |
| v6 | dot before distance | worse |
| q1 | pill leaf `projection_sign_from_vector3d` | 1520/32 |
| **r1** | q1 + `negative_distance` + `project_point3d(&sweep_point, ..., (real_point2d *)&data->vector2d)` | bsp3d_test_pill 1504/31, 557/557 insns, sha-only |
| r2 | `point_from_line3d` (probe) | leaf term order and integer-load interleave remain |
| cp1 | swap `dot_product2d` arguments | identical bytes |
| cp2 | manual `point_from_line2d` (probe) | identical bytes |

Candidates:

- `scratch/workersD/collision_bsp/combo1.c` (SHA-256 `d55a4cb1...`): the park reopen.
- `scratch/workersD/collision_bsp/r1.c` (SHA-256 `dfd16230...`): combo1 plus the three closer bodies.

## 5. Do not repeat

- Operand or argument spelling of `dot_product2d`, `point_from_line2d`, and 3-term plane distances:
  VC7 canonicalises them (s2b, s2d, p2c, p2d, t3f, cp1, cp2).
- `point_from_line3d` as a lever in this object (t3b, r2): the guard forbids it, and it does not
  close the residual.
- Early `fabs` locals (t3d); a named center pointer (t3e); computing dot before distance (v6);
  HCEA's `result->t > t` predicate (v1).

## 6. Residual classification

| Function | Class |
|---|---|
| `bsp2d_test_sphere_recursive`, `bsp2d_test_pill_recursive` | exact with the helper; owner-policy question about a header-inline COMDAT |
| `bsp3d_test_sphere_recursive`, `collision_bsp_test_vector_recursive`, `bsp3d_test_pill_recursive` | scheduling: commutative term/operand order of reassociated x87 sums. Structure now target-exact in r1 (fact); cause unknown (inference: IR temp numbering) |
| `collision_surface_find_closest_point2d` | scheduling (re-confirmed) |
| `collision_surface_test_sphere` | source/topology: SSE intrinsic form local to this object; blocked by policy |

## 7. Reopen criteria

- bsp2d pair: an owner ruling that the `_plane2d_distance_to_point` header-inline COMDAT is admissible
  here. Then land combo1.
- The three near-miss parks: a natural context that produces January's y,z / i-term order. Start
  from r1.c.
- `collision_surface_test_sphere`: an owner ruling that admits a local SSE-intrinsic squared-distance
  form, or authenticated source.
- `collision_surface_find_closest_point2d`: as in the 20260904 ledger.

## 8. Disposition

No real-file change. The final gate (guard, `--out scratch/workersD/collision_bsp/final.obj`) is
still exact 23 / residual 7. Park drift passes. The owner census equals `build/base`. Fakescan finds
0 leads. The worker notes are in `scratch/workersD/collision_bsp.md`.
