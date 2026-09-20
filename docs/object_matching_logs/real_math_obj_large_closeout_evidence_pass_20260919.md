# `real_math.obj` authenticated closeout (2026-09-19)

## Result

`source/math/real_math.obj` is now **84/84 strict exact**. The packet closes
the last two rows without losing any inherited exact row anywhere in the full
build:

| function | meaningful / padded bytes | relocations | normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_sphere_intersects_triangle3d` | 607 / 608 | 6 | `aeba141c144e34c93f00af61c50ae3eb925542f20acd170eb8fe1693784fc1be` |
| `_pill_intersects_triangle3d` | 828 / 832 | 12 | `ca0e4598c4accbab9165891b02bde1eb72d9c723a022eb788f0a86a12658e57c` |

Packet gain: **+2 exact functions, +1,435 meaningful bytes, +1,440 padded
bytes**. The accepted ledger moved from **7,546 / 1,451,389 meaningful /
1,503,803 padded** to **7,548 / 1,452,824 meaningful / 1,505,243 padded**.

## Authenticated source correction

- The same-revision `halo_cache_symbols.exe` debug build identifies
  `triple_product3d` at `0x0056df00`; its `/RTC` local record and disassembly
  prove the ordinary nested source shape: a local `real_vector3d c`, followed
  by `dot_product3d(cross_product3d(a, b, &c), n)`.
- Same-revision `/RTC` records authenticate the triangle-local sets
  `v,e0,e1,e2,n` and `e0,e1,e2,v,n,p`. January object disassembly supplies
  the branch topology and signs. HCEA PDB/source corroborates the signatures
  and ordinary-C semantics.
- The out-of-line January owners independently authenticate the two other
  real-math header corrections: `rotate_vector2d` in `path_obstacles.obj` and
  the conjunction spelling of `valid_real_vector3d_axes2` in
  `bored_camera.obj`; both donors remain strict exact.
- Two missing owner-header declarations complete the authentic declaration
  schedule: `convex_hull3d_verify(void)` in `geometry.h` and
  `path_state_estimated_distance(...)` in `path.h`. Both definitions are
  public and already live in their owning translation units.
- The external `decals.c` owner uses the same nested `triple_product3d`
  spelling. `_decals_delete_permanent_from_cluster` remains exact at 400
  padded bytes / 29 relocations with SHA-256
  `b3eeb427eeef1e02f793286b1011a5b89822e041f0b442759259255e1117220`.

The paired owner-prototype corrections are required together. Removing either
reopens an independent VC7 reload-order tie in `geometry.obj` or `units.obj`.
The final coherent packet preserves all canaries: `_bitmap_copy`,
`_bitmap_2d_uncompress_from_mipmap`, `_get_edge_vertex`,
`_unit_preprocess_node_orientations`, and the game-engine race pair.

## Verification

- Focused gates: `real_math` 84/84; the decals owner and all named canaries
  strict exact; `path` remains 29/32 (no debit).
- Full `ninja all_source progress semantic_progress`: all 482 units compile;
  report and semantic report pass with zero unit errors. `progress` stops only
  on nine already-owned stale/invalid park records from concurrent work.
- Stable verdict diff against
  `scratch/ten-objects-before-20260919.json`: 18 concurrent gains / 10,819
  padded bytes, **0 regressions**. Exactly the two real-math rows above belong
  to this packet.
- Park audit: 180 active, 7 stale, 2 invalid; none is a regression. Admission
  audit: 0 contradicted / 0 revoked; `real_math` is a new ownership-audit
  candidate.
- Fake scan on the five packet files reports only the pre-existing,
  independently evidenced decals math-assembly block; no new lead.
- `_point_from_line3d` guards pass for the changed source owners. The existing
  separately recorded `units.obj` surplus-owner veto is unchanged.
- `git diff --check` passes. No configuration file was changed by this packet.

## Files

- `source/math/real_math.c`
- `source/math/real_math.h`
- `source/effects/decals.c`
- `source/math/geometry.h`
- `source/ai/path.h`

No commit or push was made.
