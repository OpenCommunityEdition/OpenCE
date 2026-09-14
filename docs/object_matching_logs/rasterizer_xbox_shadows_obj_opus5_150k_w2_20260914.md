# `rasterizer_xbox_shadows.obj`: Opus5 150K house-clean lane, wave w2 (2026-09-14)

## Baseline

`python -B tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_shadows --all --forbid-emitted-symbol _point_from_line3d`
gave **15 exact, 3 residual, 0 unwritten**, and the guard passed. All three residuals are parked in
`config/parked.json`:

| function | January padded / relocs | real-file candidate | gate tag |
| --- | ---: | --- | --- |
| `__rasterizer_environment_shadow_begin` | 944 / 73 (sha16 `b6c4674070a5973f`) | 944 / 73 `709c6800cb61db41` | sha |
| `__rasterizer_environment_shadow_model_draw` | 688 / 48 (`366f4471c4717670`) | 688 / 48 `c81ae7e554a892d0` | sha |
| `__rasterizer_environment_shadow_draw` | 1472 / 146 (`ddc0ede010a6293c`) | 1472 / 146 `e24df5bd0c0b9df9` | sha |

Duplicate check:
- Ledgers read: `rasterizer_xbox_shadows_obj_jonas_lifecycle_trio_20260826.md`,
  `..._jonas_shadow_end_exact_boundary_20260830.md`, `..._opus_reconciliation_20260903.md`, and the three
  `parked.json` evidence entries.
- `git log --all` shows 7 commits touching the file.
- `branch_sweep` found no better body.
- The 100K handoff has no do-not-repeat item for this object.

The real file is **unchanged** in this wave, so `git diff --stat` is empty. Every experiment ran on scratch copies.

## Outcomes

### `__rasterizer_environment_shadow_draw`: PARK-REOPEN PROPOSED (scratch strict EXACT)

**Candidate:** `scratch/workers/rasterizer_xbox_shadows/cand_draw.c`. It is the current real file with only the draw
body changed.

**Change:** the three hand-written point·vector sums become Bungie's inline helper, for example
`dot_product3d((real_vector3d const *)&rasterizer_environment_shadows_globals.shadow_matrix.position, &...forward)`.
The same change covers `left`, and the `up` sum stored in `position_dot_up`.

**Evidence:**
- **January x87 stream.** All three dot products follow one uniform order: a z term, then a y term, then an add,
  then an x term, then an add. The row-4 integer loads are interleaved. The previous lane tried six hand-sum
  spellings, and every one produced a different association.
- **Bungie's own idiom.** `real_math.h:1325` has `plane3d_from_point_and_normal`:
  `dot_product3d((real_vector3d *)point, &plane->n)`.
- **In-repo uses.** `rasterizer_xbox.c:1065` and `rasterizer_xbox_transparent_geometry.c:1695` pass a
  point-as-vector cast to `dot_product3d` in the same way.
- **Argument order.** Both argument orders are strict EXACT. The position-first order was kept because it follows
  the helper idiom, so no expression order was chosen to steer the result.

**Measurements:**
- **Whole-TU gate:** 16 exact / 2 residual. All 15 baseline EXACT rows stay EXACT, and the guard passes.
- **Draw row:** 1472 / 146 / `ddc0ede010a6293c`, identical to January.
- **parkcheck:** begin OK, model_draw OK, draw DRIFT (it becomes exact).
- **Fake scan:** 0 leads.

**Owner census:** compared with base, the candidate emits one extra code COMDAT, `_dot_product3d`. January's object
does not have it. This belongs to the systemic header-inline surplus class: 68 of the 69 base objects that emit
`_dot_product3d` have no January copy. There is no data, BSS or COMMON change.

**Park criterion:** "authoritative January source/local records or a natural compatible-compiler donor explaining the
x87 scheduler choice". The new evidence refutes the park's premise. The residual was not a scheduler choice; it was
a missing genuine inline helper.

### `__rasterizer_environment_shadow_begin`: NOT LANDED (park evidence refined)

**New facts:**
1. **Assert text.** January's assert-expression literals are `"shadow_color->red >=0.0f && ..."`,
   `"shadow_color->green>=0.0f && ..."` and `"object_bounding_radius>0.0f"`. The parked body stringizes
   `red >= 0.0f`, `green >= 0.0f` and `object_bounding_radius > 0.0f`, which gives three candidate-only string
   COMDATs. Any future reopen must carry this spelling fix.
2. **The helper resolves the store residual.** Using `-dot_product3d(...) * inverse_radius` for rows 0 and 1 removes
   the park's "first residual" completely. January's five constant-row stores now sink in exactly January's
   positions, and both rows get January's z, y, add, x, add term association.

**Remaining difference:** one commutative fld/fmul operand transposition in a single term, 2 instructions. January
loads the position component first in all six terms.

**Shapes tried** (every one 944 / 73; the second column lists the terms still transposed):

| shape | terms still transposed |
| --- | --- |
| `dot_product3d(position, vector)`, const or non-const cast | row0 x/y/z and row1 y/z |
| `dot_product3d(vector, position)`, const or non-const cast | row1 x only |
| `scale_vector3d` + `real_plane3d vertex_constants[5]`, position first | row0 x only |
| `scale_vector3d` + `real_plane3d vertex_constants[5]`, vector first | 5 terms |
| `real_plane3d`-typed explicit stores, either order | 2 terms |

Diagnostics that were never candidates for landing:
- mixed argument order per row;
- `inverse_radius * v` store operand order;
- +1 to +11 typedefs and +1 to +10 prototypes before the function.

All of them are inert or only move the transposed term. The odd term is keyed per row (value-number order), and
declaration count does not affect it.

**Classification:** class D x87 operand tie.

**Best candidates:** `scratch/workers/rasterizer_xbox_shadows/b2.c` and `p_pv.c`.

**Reopen when:** a natural spelling keeps inline `dot_product3d` and also loads all six position components first,
or January local/type records for `vertex_constants` become available.

### `__rasterizer_environment_shadow_model_draw`: NOT LANDED (park evidence refined)

**Baseline difference:** two transpositions. January loads `detail_map_scale` into eax, runs the x87 multiply,
then stores `vertex_constants[0].i`. January's `fstp [0].j` also comes before `lea edx`. Ours stores early.

**Evidence:**
- **Active camouflage.** `rasterizer_xbox_active_camouflage.c:615` computes a named `detail_map_v_scale` local
  first. January's instructions in that region are identical to ours.
- **Models.** `__rasterizer_model_draw+0x12bd` has the same January shape. Our `rasterizer_xbox_models.c`
  reconstruction shows the same early-store defect there.

**Shapes tried:**

| shape | result |
| --- | --- |
| m1: named local computed first | ONE transposition left: `fld [esi+0xec]` before the eax load; store and fstp/lea match |
| m2: j-statement first | worse |
| m4: `real vertex_constants[3][4]` | inert |
| m5: `j = v_scale * vertex_constants[0].i` | worse, 203 instructions |
| m6: separate u/v locals | worse, 201 instructions |
| m7 (diagnostic): local assigned after the i-store | same as baseline |
| m8 (diagnostic): m1 with the operands commuted | identical to m1, so operand order is inert |

**Classification:** class D scheduler tie.

**Best candidate:** `scratch/workers/rasterizer_xbox_shadows/m1.c`.

**Reopen when:** a spelling keeps m1's local-first schedule and also loads `detail_map_scale` before the multiply,
or January local records become available.

## Final state

**Real-file gate (guard):** 15 exact / 3 residual / 0 unwritten, the same as baseline.

**Other checks:**
- parkcheck on a fresh compile of the real file: 0 drift.
- Fake scan of the real file and the best scratch candidates: 0 leads.
- No header or config edits, and no orchestrator header proposals.

## House-rule notes

- The draw candidate adds no volatile, register, pragma, assembly, barrier, raw offset, forced inline or
  expression-order steering. The helper is Bungie's `real_math.h` inline, and the cast is Bungie's own
  point-as-vector idiom.
- The candidate-only `_dot_product3d` COMDAT is disclosed above for the orchestrator's owner-census decision.
