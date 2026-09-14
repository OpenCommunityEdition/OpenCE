# items.obj: `_item_update` park closed in scratch (opus5 150K lane, wave w3, 2026-09-14)

Worker scope: `source/items/items.c` only. **No production change**: `git diff --stat -- source/items/items.c` is
empty. The strict-exact candidate depends on a `_point_from_line3d` COMDAT owner ruling (brief C3 family), so it is
reported, not landed.

## Baseline and final real-file gate

| gate | result |
| --- | --- |
| baseline (guard on) | `== exact 17 residual 1 unwritten 0`; `_item_update` 2352/110 residual `[sha]`, parked |
| final (guard on) | identical rows; guard passes; park drift 0; owner census = build/base |

Target `_item_update`: 2352 padded / 110 relocations / normalized SHA-256 `78237a8528e5d6a8...`.
Parked real body: `3daa5a96ffd38c2e...`. The w2 best scratch body (`8495836d...`) differed in 9 instructions at
+0x390, recorded as an "x87 sum term-order tie" in the angular-velocity projection.

## Duplicate prevention

All earlier `items_obj*.md` ledgers (including `items_obj_opus5_150k_w2_20260914.md` and its 5 spent shapes),
`scratch/workers/items.md`, the parked.json entry (base still the 3daa5a96 body: the w2 re-baseline proposal was not
applied), `scratch/w2/laws_w2.md` A22, and `structure_lens_flares_obj_opus5_150k_w2_20260914.md`.

## New evidence: the tie premise is wrong

Laws applied as detectors: **A22 YOUNGER OPERAND** (fields read by name rank older than fields reached only through
an inline helper's pointer parameters) and the structure_lens_flares w2 **group N** result (hand-expanded
`point_from_line3d` scalar updates flipped January's later dot-product term order from j,k,i to k,j,i).

`_item_update` has the same defect. The nudge after `collision_test_line`,

```c
new_position.x += collision.plane.n.i * 0.05f;
new_position.y += collision.plane.n.j * 0.05f;
new_position.z += collision.plane.n.k * 0.05f;
```

is a hand expansion of the header inline `point_from_line3d(&new_position, &collision.plane.n, 0.05f, &new_position)`
(real_math.h:1077, `result->x = v->i*t + p->x`; same bytes at +0x1bb..+0x1e5). The by-name reads age `n.i/j/k`, and
VC7 then orders the later inlined `dot_product3d(&item->object.angular_velocity, &collision.plane.n)` k,j,i instead
of January's j,k,i. January's other `point_from_line3d` use (+0x57e, inside the reflection else-arm under
`collision_test_point`) is an out-of-line call that January items.obj references **undefined**.

The file's current `#define point_from_line3d items_point_from_line3d_inline` plus a local external prototype exists
only to avoid emitting that COMDAT, and it is what forced the hand expansion.

## Shapes (scratch only; parked real body never edited)

| # | file | shape | result |
| --- | --- | --- | --- |
| 1 | `i1.c` | nudge through the TU alias `items_point_from_line3d_inline(...)` | item_update EXACT, guard passes, but emits a new `_items_point_from_line3d_inline` COMDAT (census fail). Manual inlining: diagnostic only |
| 2 | `i2.c` | natural header inline: remove the rename, `#undef` and local prototype; nudge = `point_from_line3d(...)` | **18/18 EXACT**. VC7 keeps +0x57e out of line by itself. Emits `_point_from_line3d` (guard FAIL) |
| 3 | `i3.c` | 2 + house formatting (one parameter per line) | EXACT |
| 4 | `i4.c` | 3 + reflection block `new_position = collision.point; velocity.i += collision.plane.n.i * reflection;` (x3), replacing the parked interleaved, operand-swapped component stores | **EXACT (preferred)** |
| 5 | `i5.c` | 4 with the point copy after the velocity update | EXACT (store order not visible; 4 kept) |

Checks on `scratch/workers/w3_weapons_items/i4.c`:

- All 17 baseline EXACT rows stay EXACT; the only row change is `_item_update` residual -> EXACT.
- relocdiff: 0 differing rows out of 110.
- Guard fails only on `_point_from_line3d`. The emitted COMDAT is 48 bytes, sha `9b763841f8519177`, identical to
  January's action_charge.obj copy.
- parkcheck: `_item_update` drifts exactly to the target hash, as expected for a reopen.
- Owner census vs build/base: the only NEW owner is `_point_from_line3d` (.text COMDAT, referenced undefined by January).
- fake scan: 0 leads.

i4 also removes the parked body's artificial layout carriers (`union item_update_scratch`, `union item_update_work`,
`struct item_update_storage`) and the `(real_vector3d *)&candidate` casts, using the natural block-scoped locals from w2.

## Proposals

- **Owner ruling (C3 `_point_from_line3d` COMDAT family, +2,352 padded):** if the systemic select-any
  `_point_from_line3d` COMDAT is admitted for objects whose January code inlines the helper at one site and calls it
  out of line at another, unpark `_item_update` and apply `scratch/workers/w3_weapons_items/i4.c` to
  `source/items/items.c` in one dedicated commit. Consumers: items.c only. The only config change is removing the
  parked.json entry.
- **Law (for WORKER_BRIEF / laws):** a residual recorded as a canonical x87 sum term-order "tie" should first be checked
  for earlier hand-expanded inline helpers that read the same fields by name (A22 aging). Two confirmed instances:
  structure_lens_flares (group N) and items `_item_update`. A22 also lists observer and triangle_coplanar ties as
  candidates.

## Reopen criterion

An owner ruling admitting `_point_from_line3d` COMDAT emission in items.obj; then land i4.c unchanged.
