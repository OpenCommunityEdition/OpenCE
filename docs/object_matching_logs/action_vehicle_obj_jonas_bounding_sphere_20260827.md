# Action-vehicle bounding-sphere recovery (2026-08-27)

## Result

The first and only ordinary code-producing candidate makes
`_object_get_bounding_sphere` strict exact in
`source/ai/action_vehicle.obj`. The object advances from 6/17 to 7/17 exact
functions and from 165/4,909 to 294/4,909 meaningful code bytes. It remains
`NonMatching`; no data, completion-label, semantic-exception, or
parked-function credit is claimed.

All six inherited exact owners remain exact: `_action_vehicle_update`,
`_action_vehicle_flush_structure_indices`, `_point_from_line2d`,
`_distance_squared2d`, `_distance2d`, and `_negate_vector2d`.

## Frozen target and provenance evidence

January defines an external cdecl
`void(long, real_point3d *, real *)` owner with 129 meaningful and 144 padded
bytes. It retrieves a typed all-mask object, asserts non-null `center` and
`radius` outputs with canonical `objects.h` line metadata 535 and 536, copies
the named three-float bounding-sphere center, copies the radius, and returns.

Its nine relocations are fully fixed: `REL32` to
`_object_get_and_verify_type`; two references to the canonical objects-header
path literal; references to the `center` and `radius` assertion literals; and
two ordered `REL32` pairs to `_display_assert` and `_system_exit`.

The canonical inline implementation in `source/objects/objects.h`
independently supplies the exact signature, typed `object_get` access path,
assertion strings and line numbers, named fields, copy order, and explicit
terminal return. The HCEA later-build source at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently corroborates the
typed object datum and sphere-field semantics; its source blob is
`3bed3193ca7ec40b5f6c81955fc1452b7b5daaf9`. HCEA omits January's assertions
and is semantic/type provenance only; January PC COFF remains the sole padded
byte and relocation authority.

The complete Claude campaign tree, Stian sources and verification metadata,
all current object ledgers, and all repository revisions were searched before
emission. None contains a prior action-vehicle candidate or fixed-point record
for this owner. The existing global and HUD copies authenticate the shared
contract but do not claim this translation unit's COMDAT ownership.

The adjacent `_action_vehicle_begin` and `_action_vehicle_control` owners were
excluded before emission because the 2026-08-21 action-vehicle ledger already
preserves their natural HCEA-backed misses and requires new January evidence
before any retry.

## Retained typed source and house-rule audit

A translation-unit-local macro rename preserves the canonical inline owner
under a private name while allowing January's external COMDAT to be defined
from the same typed body:

```c
#define object_get_bounding_sphere object_get_bounding_sphere_inline
#include "actors.h"
#undef object_get_bounding_sphere

void object_get_bounding_sphere(
	long object_index,
	real_point3d *center,
	real *radius)
{
	struct object_datum *object = object_get(object_index);

	match_assert("..\\objects\\objects.h", 535, center);
	match_assert("..\\objects\\objects.h", 536, radius);

	*center = object->object.bounding_sphere_center;
	*radius = object->object.bounding_sphere_radius;

	return;
}
```

Every parameter is on its own line, the C89 declaration precedes statements,
the canonical typed object-access macro replaces any raw access cast, the body
uses named fields, and the void function ends with an explicit return. It
contains no assembly, `register`, `volatile`, optimizer barrier, pragma,
intrinsic, attribute, raw address, byte-offset access, pointer/integer
reconstruction, pointer or union pun, undefined behavior, synthetic anchor,
object patch, comparator exception, or byte forcing.

## Immutable artifacts and strict comparison

- Base commit: `6d23d0f7bc976fcd24480411ee6e4878a758d8df`.
- Pre-wave source blob: `96eb18be20f7f1aa3e40e65063e9d65f5b57650c`.
- Frozen retained source blob: `6f689f90311fc56f1f3c0d58c593df81390efdca`.
- January split object SHA-256:
  `49F00A0115CECF578E51102FC33F665F876A4FAFCB78EE3B53520545406EF769`.
- Baseline candidate object SHA-256:
  `0B473CDE87C91DB8E453A108F777B1138C5556CC44B31303F509AEA8B634B048`.
- First-shot object SHA-256:
  `4A44DE94FA7BC403290B54809F4DC08EAD97D2DC6DC50EC57724626599A96CE4`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1 with the unchanged natural
  `/O2 /Oy- /DDEBUG /Dxbox` flags and include graph.

The hardened comparator proves all 144 padded bytes, all nine relocation
addresses, types, destinations, addends, and ownership exact. Target and
candidate share normalized SHA-256
`65d07f3bd1140a75fa9122ef9ec31f739c4a744a117752b38f576b2493457fef`.
The exact-set comparison reports only `_object_get_bounding_sphere` as newly
exact, preserves all six inherited exact owners, and reports no loss.

## One-shot discipline and validation

`git diff --check` and the production compiler's no-output `/Zs` check passed
before emission. Exactly one normal selected-object compiler edge produced the
retained artifact. No assertion spelling, macro, field, declaration, include
topology, flag, or compiler retry occurred.

The complete graph and campaign gates pass:

- 377/833 complete objects overall and 275/468 for Halo;
- 511,119/2,198,102 meaningful code bytes and 4,240/11,060 functions overall;
- 498,205/1,770,166 Halo code bytes and 4,073/7,574 Halo functions;
- data unchanged at 1,836,762/4,176,062 overall;
- semantic audit: 470 units, 4,352 functions evaluated, 4,206 semantic exact,
  4,266 accepted exact, and zero unit errors;
- admission: zero candidates, contradictions, or revocations;
- parks: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass, with only the inherited pytest cache-permission
  warning.

No protected source, configuration entry, completion label, push, amend, or
history rewrite is involved.
