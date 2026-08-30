# `render_cameras.obj` sphere-visibility boundary (2026-08-29)

## Result

`_render_frustum_sphere_visible` received one frozen, readable, typed C89
candidate compile.  It is **not** strict and the production source is fully
restored.  The candidate reproduced the target's 432-byte padded extent and
all six ordered relocation addresses, types, destinations, and addends, but
its final classification expression lowered differently.

| Measurement | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful bytes | 418 | 428 |
| Padded bytes | 432 | 432 |
| Ordered relocations | 6 | 6 |
| Normalized SHA-256 | `2bdb31a0158b5a23314020bcccb8931e8bcc74c2b357d56305c687c16b91f949` | `11ceb60ad36c1cafbd90f9c5edb039a97663081224091d212e3a9aee637de493` |

The complete frozen candidate object SHA-256 was
`5c9381794357618fca79188cc21bd97b71cc4eee6caecaee01adb1b2d474e152`.
This owner remains unwritten.  Do not retry this topology through comparison
reversal, Boolean regrouping, return spelling, declaration order, or local
renaming.  A future shot requires independently preserved January-compatible
source evidence for the final partial/full classification expression.

## Provenance and selection

January `source/render/render_cameras.obj` is authoritative.  Its complete
object SHA-256 is
`c0194e912ef3b7e9ab6e4ffc9db57c527bd2fc9b011bc00e00c0ddc793cb6974`.
The target owner is 418 meaningful / 432 padded bytes with normalized SHA-256
`2bdb31a0158b5a23314020bcccb8931e8bcc74c2b357d56305c687c16b91f949`.

The clean typed semantic donor is HCEA
`src/blam/render/render_frustum_sphere_visible.c` at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, Git blob
`6b141b95ab829f1741c2da4d9ac00adce7384da4`, and file SHA-256
`e90320dd4c508e6f86b796a398737244ef015c10b0b83c8f8d07e3644a30bd74`.
The preserved Stian/Pastudan histories independently attest the signature and
target control flow, but supply no admissible natural PC C body: Pastudan's
owner is a naked assembly transcription and is evidence only, never a source
candidate.

All applicable Render Cameras ledgers were read before selection.  The three
functions rejected by the earlier leaf wave and the residuals fixed by the
typed projection wave were excluded.  A complete history and ledger search
found no prior candidate for this owner.

The function was link-closed.  Its six relocations all call the public typed
`_plane3d_distance_to_point` owner at exact addresses 156, 189, 218, 247, 276,
and 302.  No private helper, data owner, literal, assertion, or register ABI is
involved.

## Frozen candidate

The sole emitted body used the existing `struct render_frustum`,
`real_point3d`, `real_rectangle3d`, `real_plane3d`, and `real` definitions.
It performed the natural sequence fixed by HCEA and January:

1. six axis-aligned sphere/world-bounds rejection tests;
2. six ordered plane-distance calls, retaining distances for planes 0, 1, 2,
   3, and 5 while testing plane 4 without retaining it;
3. rejection when any signed plane distance exceeds the radius; and
4. partial/full classification against the negated radius.

The source used named fields and the existing typed math interface.  It
contained no assembly, `volatile`, `register`, pragma, intrinsic, compiler
attribute, optimizer barrier, raw address, raw offset, pointer/integer
reconstruction, representation pun, undefined behavior, synthetic anchor, or
byte-forcing device.  A non-emitting production-argument `/Zs` check passed
before the one object-producing compile.

## Fixed boundary

The first 349 padded section bytes are identical.  That exact prefix includes
all six bounds tests, all six plane calls, all six post-call radius tests, all
six relocation records, and the radius negation.  Only 49 padded bytes differ,
from offset `0x15D` through `0x1AB`.

The first difference is the immediate byte of the first final x87 condition:

- January lowers the comparison as `test ah, 5` followed by `jp` to a shared
  partial-result block;
- the natural donor topology lowers it as `test ah, 1` followed by `je`, with
  a separate early partial return.

January repeats its `test ah, 5` / `jp` form for the first four retained
distances, then selects 2 or 1 after the fifth comparison with one shared
epilogue.  The candidate repeats `test ah, 1` conditions, emits a separate
early epilogue, and then selects 1 or 2 after the last comparison.  This is a
source Boolean-orientation/control-flow mismatch, not a relocation, ABI,
layout, arithmetic, or dependency mismatch.  No comparison or return-shape
tuning was attempted after the miss.

## Restoration

The inverse-revert is byte exact:

- `source/render/render_cameras.c` restored Git blob:
  `c5ca98582be6f66fca4ad442d35c150a826f3d79`;
- `source/render/render_cameras.h` restored Git blob:
  `0eff66c2da0472ead9b2ff787e0121686a919c9d`.

The focused restored-object gate reports four exact, zero residual, and
seventeen unwritten functions.  All four inherited exact code owners remain
strict.  This additive ledger is the only retained change from the shot.

The full validation packet passes:

- `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress` complete successfully;
- semantic report: 470 units, 4,918 functions evaluated, 4,801 semantic
  exact, 4,811 accepted exact, 131 hidden exact, and zero unit errors;
- progress: 384/833 objects, 4,772/11,060 functions, 606,426/2,198,102 code
  bytes, and 1,856,050/4,176,062 data bytes;
- admission audit: zero candidates, contradictions, and revocations;
- parked audit: 12 active, zero stale, and zero invalid;
- test suite: 212 passed;
- protected Units sentinel: 1,920 padded bytes, 87 ordered relocations, and
  strict normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `build/report.json` SHA-256:
  `b9cb28ea0bd32f8729fd7b241e83a6ab71e01b56f44350abc3796bc63ec844fb`;
- `build/semantic_report.json` SHA-256:
  `0f782f197d2e92d9837d8252e43b074b43bf1f2386da21a8d7211c81553ec542`.
