# Action-vehicle 2D-distance recovery (2026-08-27)

## Result

The first and only ordinary code-producing candidate makes `_distance2d`
strict exact in `source/ai/action_vehicle.obj`. The object advances from 4/17
to 5/17 exact functions and from 95/4,909 to 132/4,909 meaningful code bytes.
It remains `NonMatching`; no data, completion-label, semantic-exception, or
parked-function credit is claimed.

The four inherited exact owners, `_action_vehicle_update`,
`_action_vehicle_flush_structure_indices`, `_point_from_line2d`, and
`_negate_vector2d`, remain exact.

## Frozen target and provenance evidence

January defines an external cdecl `real(real_point2d const *, real_point2d
const *)` owner with 37 meaningful and 48 padded bytes. It subtracts the two
point components, squares and adds them on the x87 stack, applies `fsqrt`, and
returns the result. The function has no relocation.

The canonical inline implementation in `source/math/real_math.h` supplies the
same signature and computes the magnitude of the vector from `a` to `b`. The
HCEA later-build source at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently spells the same two
component differences and square root. Its source blob is
`3757f43b014e35fe4945b9b678c7857cec58613b`. HCEA supplies semantic and type
provenance only; January PC COFF remains the sole padded-byte authority.

The complete Claude campaign tree, Stian action-vehicle sources and
verification metadata, all current ledgers, and all repository revisions were
searched before emission. None contains an earlier action-vehicle definition,
candidate artifact, or fixed-point record for this owner. The immediately
preceding point-from-line ledger explicitly excluded it to preserve the present
independent one-shot boundary.

`_distance3d` and `_distance_squared2d` remained outside this wave. Their
different x87 stack shapes require separate frozen evidence and production
shots.

## Retained typed source and house-rule audit

The existing `actions.h` include brings in the canonical inline math header.
A translation-unit-local macro rename retains the inline spelling privately
while allowing January's external owner to be defined from the canonical
body:

```c
#define distance2d distance2d_inline
#include "actions.h"
#undef distance2d

real distance2d(
	real_point2d const *a,
	real_point2d const *b)
{
	real_vector2d v;

	return magnitude2d(vector_from_points2d(a, b, &v));
}
```

Every parameter is on its own line, the C89 local declaration precedes the
statement, typed helpers and named fields provide the arithmetic, and the
function ends with an explicit value return. It contains no assembly,
`register`, `volatile`, optimizer barrier, pragma, intrinsic, attribute, raw
address, byte-offset access, pointer/integer reconstruction, pointer or union
pun, undefined behavior, synthetic anchor, object patch, comparator exception,
or byte forcing.

## Immutable artifacts and strict comparison

- Base commit: `608181dec91c7d657c503d864c115354ed9946a7`.
- Pre-wave source blob: `d6af877431ebe5ae8d02c961bb091aa2f6d17683`.
- Frozen retained source blob: `8b35c87381770b48b7a6118555b6a60c61b54757`.
- January split object SHA-256:
  `49F00A0115CECF578E51102FC33F665F876A4FAFCB78EE3B53520545406EF769`.
- Baseline candidate object SHA-256:
  `4A2BBD6AFD7DB99DC2636DE3AD3F590B0B52A3D969D450347FB3F0B366BC75F1`.
- First-shot object SHA-256:
  `5901C77965196428278E8290250BAB1B493581466B74A2FF60D5CF70CDEE0285`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1 with the unchanged natural
  `/O2 /Oy- /DDEBUG /Dxbox` flags and include graph.

The hardened comparator proves all 48 padded bytes and zero-relocation
identity exact. Target and candidate share normalized SHA-256
`3c78450b16a81d94946b346e70e6b6c0b6d43ce353c74c1a18cd64d8e37eff9d`.
The exact-set comparison reports only `_distance2d` as newly exact, preserves
all four inherited exact owners, and reports no loss.

## One-shot discipline and validation

`git diff --check` and the production compiler's no-output `/Zs` check passed
before emission. Exactly one normal selected-object compiler edge produced the
retained artifact. No expression, helper spelling, declaration, include
topology, flag, or compiler retry occurred.

The complete graph and campaign gates pass:

- 377/833 complete objects overall and 275/468 for Halo;
- 510,957/2,198,102 meaningful code bytes and 4,238/11,060 functions overall;
- 498,043/1,770,166 Halo code bytes and 4,071/7,574 Halo functions;
- data unchanged at 1,836,762/4,176,062 overall;
- semantic audit: 470 units, 4,350 functions evaluated, 4,204 semantic exact,
  4,264 accepted exact, and zero unit errors;
- admission: zero candidates, contradictions, or revocations;
- parks: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass, with only the inherited pytest cache-permission
  warning.

No protected source, configuration entry, completion label, push, amend, or
history rewrite is involved.
