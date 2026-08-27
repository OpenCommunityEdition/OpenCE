# Action-vehicle 3D-distance fixed point (2026-08-27)

## Result

Two separately frozen, evidence-backed waves for `_distance3d` were cleanly
rejected. Both candidates preserve all five inherited exact functions in
`source/ai/action_vehicle.obj`, match January's 64-byte padded extent, and
contain zero relocations, but neither matches the target's x87 instruction
schedule. All candidate source was removed and the exact pre-wave source blob
was restored.

The object therefore remains at 5/17 exact functions and 132/4,909 meaningful
code bytes. No function, data, completion-label, semantic-exception, or
parked-function credit is claimed.

## Frozen target and provenance evidence

January defines an external cdecl `real(real_point3d const *, real_point3d
const *)` owner with 51 meaningful and 64 padded bytes. It loads the three
component differences, squares `x`, then `y`, accumulates those two, squares
and accumulates `z`, exchanges the result into the expected x87 slot, applies
`fsqrt`, drops the two residual stack values, and returns. It has no
relocation. Its normalized SHA-256 is
`4cdb6852c1705b5f4af050b829c52d1342a122dccb39f2291f477c6829908b84`.

The canonical inline implementation in `source/math/real_math.h` supplies the
fixed signature and nested vector/magnitude semantics. The independent HCEA
later-build source at commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
spells three scalar differences followed by the left-associated sum of their
squares. Its source blob is
`0918df559cfdfb35e458f74fc6ebd1592f9eda97`. HCEA is semantic and source-shape
provenance only; January PC COFF remains the sole padded-byte authority.

The complete Claude campaign tree, Stian action-vehicle source and verification
metadata, every current ledger, and all repository revisions were searched.
None provides another January-authenticated source shape or a prior attempt for
this owner.

## Wave one: canonical nested helper

The first frozen wave used the canonical inline body under a source-local macro
rename:

```c
real distance3d(
	real_point3d const *a,
	real_point3d const *b)
{
	real_vector3d v;

	return magnitude3d(vector_from_points3d(a, b, &v));
}
```

It emits the same loads, three differences, 64-byte padded extent, and zero
relocations, but squares `z`, then `x`, then `y`. Its normalized SHA-256 is
`01d7809ae796fc6e9a4a6c2ca73f1e156770bdcff33dd17fc212c50d7279a841`.
The immutable candidate object SHA-256 is
`995D0A95996A30F24DFA5D09CD6CDE66D1AC8429AD61F14F1B0C93929C95003C`.

The emitted x87 order and the independent HCEA scalar spelling constituted
genuine new evidence, so the canonical source was restored before opening a
separate second wave.

## Wave two: donor-derived scalar topology

The second frozen wave used the HCEA scalar topology while retaining current
typed names and the canonical square-root helper:

```c
real distance3d(
	real_point3d const *a,
	real_point3d const *b)
{
	real dx = b->x - a->x;
	real dy = b->y - a->y;
	real dz = b->z - a->z;

	return square_root(dx*dx + dy*dy + dz*dz);
}
```

It also emits the same loads, differences, extent, and zero-relocation shape,
but squares `z`, then `y`, then `x`. Its normalized SHA-256 is
`21014f2a01d8cc834186684d7f719a227bf489bc06c34d4890f9b4a862f356f5`.
The immutable candidate object SHA-256 is
`245CE326C975C10738CA9B8E74CF7ECD07E7A6C419C31DA7029C8A667FA55B61`.

No further retry is justified by current evidence. Reopen this owner only with
a new January-era source donor, compiler intermediate evidence, or a lawful
source topology independently explaining the target's exact x87 ownership and
drop order.

## Immutable baseline and restoration

- Base commit: `6076c6b4953ded466b8f280fc5177999de537d2e`.
- Pre-wave and restored source blob:
  `8b35c87381770b48b7a6118555b6a60c61b54757`.
- Canonical-wave source blob:
  `3dba79474fb4e09b49778ac577f78c27e08654b6`.
- Scalar-wave source blob:
  `1ce486341aed567b5b8b77198d74156e28bf5387`.
- January split object SHA-256:
  `49F00A0115CECF578E51102FC33F665F876A4FAFCB78EE3B53520545406EF769`.
- Frozen baseline candidate object SHA-256:
  `5901C77965196428278E8290250BAB1B493581466B74A2FF60D5CF70CDEE0285`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1 with the unchanged natural
  `/O2 /Oy- /DDEBUG /Dxbox` flags and include graph.

Each wave passed `git diff --check` and a no-output `/Zs` parse before its one
normal code-producing compile. Neither used assembly, `register`, `volatile`,
optimizer barriers, pragmas, intrinsics, attributes, raw addresses,
pointer/integer reconstruction, pointer or union puns, undefined behavior,
synthetic anchors, object patches, comparator exceptions, or byte forcing.

After restoration, the complete exact-set comparison again contains exactly
`_action_vehicle_update`, `_action_vehicle_flush_structure_indices`,
`_point_from_line2d`, `_distance2d`, and `_negate_vector2d`, with no lost or
new owner.

## Validation

The restored complete graph and campaign gates pass at the unchanged campaign
totals:

- 377/833 complete objects overall and 275/468 for Halo;
- 510,957/2,198,102 meaningful code bytes and 4,238/11,060 functions overall;
- 498,043/1,770,166 Halo code bytes and 4,071/7,574 Halo functions;
- data at 1,836,762/4,176,062 overall;
- semantic audit: 470 units, 4,350 functions evaluated, 4,204 semantic exact,
  4,264 accepted exact, and zero unit errors;
- admission: zero candidates, contradictions, or revocations;
- parks: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass, with only the inherited pytest cache-permission
  warning.

No protected source, configuration entry, completion label, push, amend, or
history rewrite is involved.
