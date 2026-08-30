# `geometry.obj` perimeter first-shot boundary

## Result

This bounded strict lane starts from canonical commit
`f88ac08d028e19ea7dcb65f563745b2a09b6840d` and closes one natural
authenticated candidate for `_convex_hull2d_perimeter`. The candidate missed;
it was removed in full without a spelling, declaration, expression-order, or
compiler-control retry. Production source is byte-identical to the baseline,
the seven inherited exact siblings remain exact, and this ledger grants no
function, byte, data, or object credit.

| Evidence | January target | First natural candidate |
| --- | ---: | ---: |
| Meaningful / padded code | `92 / 96` | `97 / 112` |
| Ordered relocations | `0` | `0` |
| Normalized SHA-256 | `71292feb41a8e5d57e328751240458cb0b9678237d26b7f8be4ba7abab7f4b0e` | `c5a37b83a10cb000d982e6d0ced4caab96e081c2edd16c7fa3979bd4b2d549d9` |
| Normalized differing padded bytes | | `95` |

The empty ordered relocation sequences agree exactly. Complete padded bytes do
not, so the function is not retained.

## Provenance and frozen candidate

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/math/geometry.obj` SHA-256:
  `1afce0545c1e6d766152acf21abd191958962382ec390cfa831e056bae63e451`.
- `config/symbols.json` SHA-256:
  `16620d6518733c0c46652aee80e1baaecdd9cbca6dc1832fe52d332ce17af821`.
  It independently names `_convex_hull2d_perimeter`; January fixes the body at
  RVA `0xF5BB0` with ordinary cdecl arguments `(short, real_point2d const *)`
  and a real return in `ST(0)`.
- Stian's independently recovered PC body is commit
  `e6a61d3a587bbb66100c534037b4bc7bb306e2dc`, source blob
  `904c8bc81918342510096a4f6ea2e41c78a16998`. Its documented VC71 comparison
  fixes the unusual source topology: seed the closing edge with the x term
  first, then accumulate consecutive edges with the y term first.
- Clean HCEA semantic donor commit:
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`; donor blob
  `f1e83136d26ceaa6a6b5031a9e4928a2f714088d`, file SHA-256
  `e6f324adfcf620f0c8d202b4a4302e94f08d8e9bdaf60cfb41414118b1c84d99`.
  It independently supplies the closed-hull edge traversal and typed
  `real_point2d` parameter meaning.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, using the repository's unchanged XDK 3911 command and flags.

The one and only production compile used readable C89 with a `real` perimeter,
an `unsigned short` remaining-edge countdown, named `.x`/`.y` field access,
the existing typed `square_root` helper, the Stian-authenticated closing/loop
expression order, and a natural pointer-advance `do` loop. A matching public
prototype was included in that same shot. No raw storage, casts, register
qualifiers, intrinsics, pragmas, assembly, attributes, volatile devices, or
address reconstruction were used.

## Fixed boundary

January starts the closing edge by loading `x`, derives the last vertex with a
single `lea`, then loads `y`. The natural named-field candidate instead loads
`y`, derives the end pointer, then loads `x`. More importantly, January forms
the loop countdown as `dec ecx; movzx ecx,cx`, decrements it again before the
x87 body, and reuses those preserved flags at the back edge. VC7 compiled the
frozen candidate as `lea ecx,[edx-1]`, a six-byte 16-bit subtraction, an
explicit `test cx,cx`, and a different x87 stack schedule. This expands the
meaningful body by five bytes and moves it into the next 16-byte padded bucket.

These are structural source/backend differences, not an admissible last-byte
choice. Do not retry the same donor topology, named-field expansion,
countdown type, pointer-advance loop, or expression order. Reopen only from
newly authenticated January PC source or compiler provenance that predicts
both the closing-edge load order and exact countdown/flag lifetime. Source
permutations, code-generation controls, undefined behavior, and byte forcing
remain out of bounds.

## Source restoration and validation

The rejected object had SHA-256
`35a1158cf1402aa4f768c92c546bc51a2e4c3eb125f833b20f868bb3fee16247`.
Both production edits were inverse-reverted immediately. Final source blobs
and SHA-256 values are:

| Production file | Git blob | SHA-256 |
| --- | --- | --- |
| `source/math/geometry.c` | `6640c797bfbd2537630d442f78b55526b748de35` | `a45d4f82e2aa843bde8d5a0e11607573dff9cc6cd22cec00d25b1a0e3a0496ec` |
| `source/math/geometry.h` | `4026bb46f1583e99670c95367b84912df8b4143e` | `115e72ff2fa7f043eb6927126b51d69eb313d2d1f66a074e66064911ded1e8f3` |

The clean restored-object replay confirms that the rejected symbol is absent
and all seven inherited exact Geometry functions remain strict. The complete
validation packet passes:

- `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress` complete successfully;
- semantic report: 470 units, 4,918 functions evaluated, 4,801 semantic
  exact, 4,811 accepted exact, 131 hidden exact, and zero unit errors;
- progress: 384/833 objects, 4,772/11,060 functions,
  606,426/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes;
- admission audit: zero candidates, contradictions, and revocations;
- parked audit: 12 active, zero stale, and zero invalid;
- complete test suite: 212 passed;
- protected Units sentinel: 1,920 padded bytes, 87 ordered relocations, and
  strict normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `build/report.json` SHA-256:
  `b9cb28ea0bd32f8729fd7b241e83a6ab71e01b56f44350abc3796bc63ec844fb`;
- `build/semantic_report.json` SHA-256:
  `0f782f197d2e92d9837d8252e43b074b43bf1f2386da21a8d7211c81553ec542`.
