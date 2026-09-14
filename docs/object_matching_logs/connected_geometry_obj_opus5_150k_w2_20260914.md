# `connected_geometry.obj`: opus5 150K wave 2 (2026-09-14)

## Baseline and outcome

Real-file gate (with guard) at baseline: `exact 6 / residual 4 / unwritten 0`.

Nothing landed, and the real file is unchanged. Final checks:

- the final gate rows are identical to baseline;
- both parks (`_connected_geometry_find_or_add_vertex`, `_triangle_coplanar`) show no drift;
- the owner census matches base (the real_math inline COMDATs that appear only in our build were already there at base);
- the fake scan finds no leads.

## `_connected_geometry_find_or_add_edge` (240 target / 256 real): owner-blocked

**What January does:** the `[ebp-1]` orientation byte is written only in three places:

- forward match: set to 1;
- reverse match: set to 0;
- new edge: set to 1.

It is read at +0xBE. If the edge count is negative (an invalid dynamic array), the read happens with no write before it.

**Why the real file is bigger:** it initializes `forward = TRUE`, which adds a 4-byte store and pads the function to 256.

**Diagnostic (scratch only):** leaving `forward` uninitialized gives 240/7 and 105 instructions on both sides. The only
remaining difference is the order of the return blocks:

- January: [set-bit arm + ret] [return NONE] [clear-bit arm + ret];
- VC7: [set] [clear] [NONE].

**Layout shapes tried, none matching:** single-exit result, if/else statement, ternary return, early returns, flag set
before the triangle add, `else edge_index = NONE`, inverted if.

**Owner decision needed:** whether a latent indeterminate read, reachable only through an invalid dynamic array, may be
preserved. Current policy prohibits it. Even with that ruling, the layout residual above remains.

## `_connected_geometry_add_triangle` (416/14, sha only): not landed

**Residual:** January shrink-wraps EBX, placing `push ebx` at +0x23 after the triangle_index==NONE exit. VC7 pushes EBX
in the prologue. Everything else is identical.

**Inert shapes:**

- locals at function scope;
- `return triangle_index;` inside the if;
- bottom `return NONE;`;
- duplicate-scan locals scoped to their block;
- declaration initializers.

Diagnostic variants of both callees did not move the push either.

**Reopen criterion:** a measured VC7 shrink-wrap law.

## `_triangle_coplanar` (parked): not landed

**Correction to the park text:** the parked body differs from January by 10 instructions, not 2:

- the point0 and point1 dot sums are emitted z,y,x instead of January's y,z,x;
- the documented point2 x-term flip.

**Best scratch candidate:** `cg_SW_dp_tn_chain_after.c`, 4 differing instructions. It uses two fixes:

- the genuine helper `dot_product3d((real_vector3d const *)point, &plane->n) - plane->d`, which fixes all three point sums;
- declaring `plane = predicate_data` after the three point fetches. This is the x87 operand-order law (January loads ESI at +0xB7), and it fixes point2.

**Remaining residual:** only the facing dot order (January x,z,y; ours y,z,x).

**Facing shapes tried:**

- helper argument orders, nesting, `0<dot`, scoping, and all 12 explicit term permutations: none produce x,z,y;
- parenthesised or sequential facing: fixes the facing but flips every point sum (the reassociation order is coupled across the function);
- single-exit result: 72 differing instructions.

## `_connected_geometry_find_or_add_vertex` (parked): not landed

**Residual:** January loads point->y and point->z with fld before each getter call, spills them to one shared 4-byte slot,
and reloads with fsubr. That slot is a compiler spill temporary for an expression operand, not a named local.

**Shapes tried:**

| Shape | Result |
| --- | --- |
| inline getters | 176 bytes |
| component loop | 160 bytes |
| block-scoped `real` locals | 8-byte frame |
| `-GET + point` | 144 bytes |
| invented value helper (diagnostic only) | 176 bytes |

**Reopen criterion:** an authentic construct that evaluates the component before the call.

Probe scripts are in `scratch/workers/connected_geometry_rasterizer_geometry_etc/`.
