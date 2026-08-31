# `real_math.obj` angular-helper restoration and point-cast rejection (2026-08-31)

## Result

One ordinary helper-form body from Claude/Fable commit `21f4fc550` is retained
as strict exact in the current canonical translation-unit context.  The object
advances from **79 exact / 5 residual** to **80 exact / 4 residual** functions
and remains correctly `NonMatching`.  A second byte-exact Fable body was
rejected because it crosses the campaign's authenticated no-punning boundary.

| Function | Padded bytes | Relocations | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_angular_accelerate_to_position` | 576 | 13 | `4cd16f09784becfa62651f9866549531697f9f4e15f38d9915e5dba587dd8f39` | new strict exact |
| `_point_in_triangle3d` Fable control | 432 | 9 | `f0c19ce34603faee28e30b9edb07783f1f591e36bdd06194f4eb273b212a140f` | exact bytes, rejected source |

## Retained source model

The retained angular change removes hand-expanded vector arithmetic and calls
the existing typed owners already declared in `real_math.h`:

- `dot_product3d`, `cross_product3d`, `scale_vector3d`,
  `subtract_vectors3d`, and `magnitude_squared3d` in the angular update.

The angular algorithm is unchanged: it clamps the desired rotation, computes
the rotation axis and speed, accelerates toward that axis, and rotates the
position.  Stian provenance corroborates this semantic topology.  No forced
inline, barrier, assembly, pragma, raw offset, dead expression, or fake branch
is present.

## Rejected exact point/triangle control

Fable's `_point_in_triangle3d` helper form is byte-exact at 432 bytes, nine
relocations, and the target hash shown above.  It achieves that result by
typing three point differences as `real_vector3d`, casting those vectors to
`real_point3d const *` for projection, and casting the projected points to
`real_vector2d const *` for cross products.

That source is not retained.  The 2026-08-26 Real Math ledger explicitly
forbids point/vector pointer casts merely because the unions share a layout;
the 2026-08-30 typed-boundary audit reaffirms the same safety boundary.
Authenticated HCEA source SHA-256
`56f0b98fa8a71c275f52533e469e1caa2f5f96b6b75957d5e1f5076aa6e35dd4`
uses direct scalar projection without those casts.  Exact bytes are therefore
insufficient evidence to admit the Fable control.  Production retains the
defined typed residual body.

## Validation boundary

- Focused gate: `_angular_accelerate_to_position` is strict exact with
  identical padded bytes, normalized hash, and ordered relocation identities;
  `_point_in_triangle3d` remains an honest residual.
- Whole-unit gate: `80 exact / 4 residual / 0 unwritten`, preserving all 79
  inherited exact functions.
- `tools/fake_match_scan.py` over the edited source: zero review leads.
- `git diff --check` over the edited source: pass.
- No header, symbol map, semantic exception, parked record, generated object,
  build edge, or target object changed in this Real Math lane.

The orchestrator owns the consolidated build and whole-board sweep.
