# Action-vehicle squared 2D-distance recovery (2026-08-27)

## Result

The first and only ordinary code-producing candidate makes
`_distance_squared2d` strict exact in `source/ai/action_vehicle.obj`. The
object advances from 5/17 to 6/17 exact functions and from 132/4,909 to
165/4,909 meaningful code bytes. It remains `NonMatching`; no data,
completion-label, semantic-exception, or parked-function credit is claimed.

All five inherited exact owners remain exact: `_action_vehicle_update`,
`_action_vehicle_flush_structure_indices`, `_point_from_line2d`,
`_distance2d`, and `_negate_vector2d`.

## Frozen target and provenance evidence

January defines an external cdecl `real(real_point2d const *, real_point2d
const *)` owner with 33 meaningful and 48 padded bytes. It loads the two
component differences, squares `x`, squares `y`, accumulates them, drops the
residual x87 value, and returns. The function has no relocation. Its byte body
is exactly the non-square-root prefix of the already accepted adjacent
`_distance2d` owner.

The canonical inline implementation in `source/math/real_math.h` supplies the
same signature and computes the squared magnitude of the vector from `a` to
`b`. The HCEA later-build source at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently spells the two
component differences and their sum of squares. Its source blob is
`c99e46c512b4f42215c6bf887b00b63d51084011`. HCEA supplies semantic and type
provenance only; January PC COFF remains the sole padded-byte authority.

The complete Claude campaign tree, Stian action-vehicle source and verification
metadata, all current ledgers, and all repository revisions were searched
before emission. None contains an earlier action-vehicle definition,
candidate artifact, or fixed-point record for this external owner. Mentions in
other object ledgers concern independent translation-unit copies and confer no
ownership here.

## Retained typed source and house-rule audit

The existing `actions.h` include brings in the canonical inline math header.
A translation-unit-local macro rename retains that inline spelling privately
while defining January's external owner from the canonical body:

```c
#define distance_squared2d distance_squared2d_inline
#include "actions.h"
#undef distance_squared2d

real distance_squared2d(
	real_point2d const *a,
	real_point2d const *b)
{
	real_vector2d v;

	return magnitude_squared2d(vector_from_points2d(a, b, &v));
}
```

Every parameter is on its own line, the C89 local declaration precedes the
statement, typed helpers and named fields carry the arithmetic, and the
function ends with an explicit value return. It contains no assembly,
`register`, `volatile`, optimizer barrier, pragma, intrinsic, attribute, raw
address, byte-offset access, pointer/integer reconstruction, pointer or union
pun, undefined behavior, synthetic anchor, object patch, comparator exception,
or byte forcing.

## Immutable artifacts and strict comparison

- Base commit: `9b60e35bab6620162e3e985960a693e9d48d7dce`.
- Pre-wave source blob: `8b35c87381770b48b7a6118555b6a60c61b54757`.
- Frozen retained source blob: `96eb18be20f7f1aa3e40e65063e9d65f5b57650c`.
- January split object SHA-256:
  `49F00A0115CECF578E51102FC33F665F876A4FAFCB78EE3B53520545406EF769`.
- Baseline candidate object SHA-256:
  `3502C791766695EDF523005BD35BD478CA266EA3930809B4D1A345C221A6DCC4`.
- First-shot object SHA-256:
  `0B473CDE87C91DB8E453A108F777B1138C5556CC44B31303F509AEA8B634B048`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1 with the unchanged natural
  `/O2 /Oy- /DDEBUG /Dxbox` flags and include graph.

The hardened comparator proves all 48 padded bytes and zero-relocation
identity exact. Target and candidate share normalized SHA-256
`e12f5936181ec0b4f78951a003a60fa0a613f8f5345003e293f2b3a4a180c4f2`.
The exact-set comparison reports only `_distance_squared2d` as newly exact,
preserves all five inherited exact owners, and reports no loss.

## One-shot discipline and validation

`git diff --check` and the production compiler's no-output `/Zs` check passed
before emission. Exactly one normal selected-object compiler edge produced the
retained artifact. No expression, helper spelling, declaration, include
topology, flag, or compiler retry occurred.

The complete graph and campaign gates pass:

- 377/833 complete objects overall and 275/468 for Halo;
- 510,990/2,198,102 meaningful code bytes and 4,239/11,060 functions overall;
- 498,076/1,770,166 Halo code bytes and 4,072/7,574 Halo functions;
- data unchanged at 1,836,762/4,176,062 overall;
- semantic audit: 470 units, 4,351 functions evaluated, 4,205 semantic exact,
  4,265 accepted exact, and zero unit errors;
- admission: zero candidates, contradictions, or revocations;
- parks: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass, with only the inherited pytest cache-permission
  warning.

No protected source, configuration entry, completion label, push, amend, or
history rewrite is involved.
