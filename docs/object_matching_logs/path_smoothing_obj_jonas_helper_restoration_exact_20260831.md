# `path_smoothing.obj` avoidance-helper restoration (2026-08-31)

## Result

The ordinary helper form recovered in Claude/Fable commit `21f4fc550` closes
`_find_avoidance_point` without changing its ABI, declarations, callers, or
configuration.  The object advances from **3 exact / 3 residual** to
**4 exact / 2 residual** functions and remains correctly `NonMatching`.

| Function | Padded bytes | Relocations | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_find_avoidance_point` | 288 | 6 | `00f44b3323d2a668374b4c4f3ecb541c790dfb458d40b3e194c8b3e30b462c6d` | new strict exact |

## Source evidence and naturalness

The retained body replaces four open-coded point subtractions and one
open-coded 2-D cross product with the existing typed math operations:

```c
vector_from_points2d(center, &tangent_points[0], &center_to_tangent[0]);
vector_from_points2d(center, &tangent_points[1], &center_to_tangent[1]);
cross = cross_product2d(&center_to_tangent[0], &center_to_tangent[1]);
```

This is the direct geometric operation expressed by the variables and agrees
with the recovered `real_point2d` compiland signature.  The calls are separate
statements, so their source evaluation order is clear.  No helper body is
copied into this translation unit, and no address, barrier, assembly,
`volatile`, dead dependency, fake branch, or compiler directive is added.

The prior natural-frontier ledger measured the residual as only two commuted
x87 operand bytes.  The Fable control matrix showed that factor-swapping the
open-coded expression was inert, whereas restoring the real helper definitions
selected January's schedule.  This is a source-model correction, not an
arithmetic respelling retained only to steer the compiler.

## Validation boundary

- PID-isolated focused gate: `_find_avoidance_point` is strict exact at
  288 bytes and six relocation records.
- Whole-unit gate: `4 exact / 2 residual / 0 unwritten`, preserving all three
  inherited exact functions.
- `tools/fake_match_scan.py` over the edited source: zero review leads.
- `git diff --check` over the edited source: pass.
- No header, symbol map, semantic exception, parked record, generated object,
  build edge, or target object changed in this lane.

The orchestrator owns the consolidated build and whole-board sweep.
