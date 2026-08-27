# Action-vehicle point-from-line recovery (2026-08-27)

## Result

The first and only ordinary code-producing candidate makes
`_point_from_line2d` strict exact in `source/ai/action_vehicle.obj`. The object
advances from 3/17 to 4/17 exact functions and from 60/4,909 to 95/4,909
meaningful code bytes. It remains `NonMatching`; no data, completion-label,
semantic-exception, or parked-function credit is claimed.

The three inherited exact owners, `_action_vehicle_update`,
`_action_vehicle_flush_structure_indices`, and `_negate_vector2d`, remain
exact.

## Frozen target and provenance evidence

January defines an external cdecl
`real_point2d *(real_point2d const *, real_vector2d const *, real,
real_point2d *)` owner with 35 meaningful and 48 padded bytes. It computes the
two components in order as `t*v + p`, stores them to the result point, leaves
the result pointer in `EAX`, and returns. The function has no relocation.

The canonical inline implementation in `source/math/real_math.h` independently
supplies the exact signature, parameter order, named fields, arithmetic order,
and return value. Its source blob is
`39a62c3da5b6a9003dfeb420ed554d1718596541`. January PC COFF remains the sole
padded-byte authority.

The complete Claude campaign tree, HCEA and Stian action-vehicle sources and
metadata, all current ledgers, and all repository revisions were searched
before emission. None contains a prior action-vehicle definition, candidate
artifact, or fixed-point record for this owner. The existing path-structure
ledgers mention the same canonical inline helper only as a spelling option in
other translation units; they neither define nor claim this external owner.

The adjacent `_distance2d` and `_distance3d` leaves were deliberately excluded
from this wave because their nested magnitude helpers introduce separate x87
stack-scheduling evidence. This wave froze only the direct arithmetic leaf.

## Retained typed source and house-rule audit

The existing `actions.h` include brings in the canonical inline math header.
A translation-unit-local macro rename retains that inline owner under a private
name while allowing January's external function to be defined naturally:

```c
#define point_from_line2d point_from_line2d_inline
#include "actions.h"
#undef point_from_line2d

real_point2d *point_from_line2d(
	real_point2d const *p,
	real_vector2d const *v,
	real t,
	real_point2d *result)
{
	result->x = t*v->i + p->x;
	result->y = t*v->j + p->y;

	return result;
}
```

Every parameter is on its own line, the implementation uses named typed
fields, and the function ends with an explicit value return. It contains no
assembly, `register`, `volatile`, optimizer barrier, pragma, intrinsic,
attribute, raw address, byte-offset access, pointer/integer reconstruction,
pointer or union pun, undefined behavior, synthetic anchor, object patch,
comparator exception, or byte forcing.

## Immutable artifacts and strict comparison

- Base commit: `dccc548cc21c1dfc6a3ab8e1083888794141e506`.
- Pre-wave source blob: `18146aae0ffe9fe419384a27c204a04aa7d276f0`.
- Frozen retained source blob: `d6af877431ebe5ae8d02c961bb091aa2f6d17683`.
- January split object SHA-256:
  `49F00A0115CECF578E51102FC33F665F876A4FAFCB78EE3B53520545406EF769`.
- Baseline candidate object SHA-256:
  `128B7726809D6A1814973851CB71E5F5B1023F34D4A012AD297482FA1B3EDF5D`.
- First-shot object SHA-256:
  `4A2BBD6AFD7DB99DC2636DE3AD3F590B0B52A3D969D450347FB3F0B366BC75F1`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1 with the unchanged natural
  `/O2 /Oy- /DDEBUG /Dxbox` flags and include graph.

The hardened comparator proves all 48 padded bytes and zero-relocation
identity exact. Target and candidate share normalized SHA-256
`d90e91bc9340636a11d9fe06c53bd87fad47428860899fd41c08c65be2cbe273`.
The exact-set comparison reports only `_point_from_line2d` as newly exact,
preserves all three inherited exact owners, and reports no loss.

## One-shot discipline and validation

`git diff --check` and the production compiler's no-output `/Zs` check passed
before emission. Exactly one normal selected-object compiler edge produced the
retained artifact. No expression, statement order, include topology, flag, or
compiler retry occurred.

The complete graph and campaign gates pass:

- 377/833 complete objects overall and 275/468 for Halo;
- 510,920/2,198,102 meaningful code bytes and 4,237/11,060 functions overall;
- 498,006/1,770,166 Halo code bytes and 4,070/7,574 Halo functions;
- data unchanged at 1,836,762/4,176,062 overall;
- semantic audit: 470 units, 4,349 functions evaluated, 4,203 semantic exact,
  4,263 accepted exact, and zero unit errors;
- admission: zero candidates, contradictions, or revocations;
- parks: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass, with only the inherited pytest cache-permission
  warning.

No protected source, configuration entry, completion label, push, amend, or
history rewrite is involved.
