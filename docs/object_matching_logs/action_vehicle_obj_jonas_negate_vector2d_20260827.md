# Action-vehicle vector-negation recovery (2026-08-27)

## Result

The first and only ordinary code-producing candidate makes
`_negate_vector2d` strict exact in `source/ai/action_vehicle.obj`. The object
advances from 2/17 to 3/17 exact functions and from 35/4,909 to 60/4,909
meaningful code bytes. It remains `NonMatching`; no data, completion-label,
semantic-exception, or parked-function credit is claimed.

Both inherited exact owners, `_action_vehicle_update` and
`_action_vehicle_flush_structure_indices`, remain exact.

## Frozen target and provenance evidence

January defines an external cdecl `real_vector2d *(real_vector2d const *,
real_vector2d *)` owner with 25 meaningful and 32 padded bytes. It loads the
two source floats in order, negates each with x87 `fchs`, stores them to the
corresponding result fields, leaves the result pointer in `EAX`, and returns.
The function has no relocation.

The canonical inline implementation in `source/math/real_math.h` independently
supplies the exact signature and named-field semantics. The HCEA later-build
source at commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently performs the same
two component negations and returns the result pointer. Its source blob is
`2c138a6375cc6957f91a3af3756ff5309f12518b`. HCEA supplies semantic and type
provenance only; January PC COFF remains the sole padded-byte authority.

The complete Claude campaign tree at
`0e1fc785b7f5fb8c8d6fec8cae667005aa663957`, Stian sources and verification
metadata, all current object ledgers, and all repository revisions were
searched before emission. They contain no earlier action-vehicle definition,
candidate artifact, or fixed-point record for this owner. The broader Claude
and Stian action-vehicle reconstructions concern larger actor-action bodies and
do not alter this standalone math leaf's signature or ownership.

## Retained typed source and house-rule audit

The existing `actions.h` include brings in the canonical inline math header.
A translation-unit-local macro rename keeps that inline implementation
available as `negate_vector2d_inline` while allowing January's external owner
to be defined naturally:

```c
#define negate_vector2d negate_vector2d_inline
#include "actions.h"
#undef negate_vector2d

real_vector2d *negate_vector2d(
	real_vector2d const *a,
	real_vector2d *result)
{
	result->i = -a->i;
	result->j = -a->j;

	return result;
}
```

Every parameter is on its own line, the implementation uses the named typed
fields, and the function ends with an explicit value return. It contains no
assembly, `register`, `volatile`, optimizer barrier, pragma, intrinsic,
attribute, raw address, byte-offset access, pointer/integer reconstruction,
pointer or union pun, undefined behavior, synthetic anchor, object patch,
comparator exception, or byte forcing.

## Immutable artifacts and strict comparison

- Base commit: `21ab835f9748f2423d8c6a77c0da99f2b42eebc3`.
- Pre-wave source blob: `21e056ab7bf30cb7f746e8a610ca15146a4785ae`.
- Frozen retained source blob: `18146aae0ffe9fe419384a27c204a04aa7d276f0`.
- January split object SHA-256:
  `49F00A0115CECF578E51102FC33F665F876A4FAFCB78EE3B53520545406EF769`.
- Baseline candidate object SHA-256:
  `E87C78FD8A92B66B61018183132ED60DD1CEE3AAB8CCF80FF471F9B8818CF276`.
- First-shot object SHA-256:
  `128B7726809D6A1814973851CB71E5F5B1023F34D4A012AD297482FA1B3EDF5D`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1 with the unchanged natural
  `/O2 /Oy- /DDEBUG /Dxbox` flags and include graph.

The hardened comparator proves all 32 padded bytes and zero-relocation
identity exact. Target and candidate share normalized SHA-256
`5a29b0bda878f7b2a152c71c0056cd52ba442629e5177fb7f7617ac83bc3eed3`.
The exact-set comparison reports only `_negate_vector2d` as newly exact,
preserves both inherited exact owners, and reports no loss.

## One-shot discipline and validation

`git diff --check` and the production compiler's no-output `/Zs` check passed
before emission. Exactly one normal selected-object compiler edge produced the
retained artifact. No field spelling, statement order, include topology,
flag, or compiler retry occurred.

The complete graph and campaign gates pass:

- 377/833 complete objects overall and 275/468 for Halo;
- 510,885/2,198,102 meaningful code bytes and 4,236/11,060 functions overall;
- 497,971/1,770,166 Halo code bytes and 4,069/7,574 Halo functions;
- data unchanged at 1,836,762/4,176,062 overall;
- semantic audit: 470 units, 4,348 functions evaluated, 4,202 semantic exact,
  4,262 accepted exact, and zero unit errors;
- admission: zero candidates, contradictions, or revocations;
- parks: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass, with only the inherited pytest cache-permission
  warning.

No protected source, configuration entry, completion label, push, amend, or
history rewrite is involved.
