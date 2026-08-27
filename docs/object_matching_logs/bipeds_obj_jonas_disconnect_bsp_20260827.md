# Biped structure-BSP disconnect recovery (2026-08-27)

## Result

The first and only ordinary code-producing candidate makes
`_biped_disconnect_from_structure_bsp` strict exact in
`source/units/bipeds.obj`. The object advances from 8/51 to 9/51 exact
functions and from 57/24,400 to 97/24,400 meaningful code bytes. It remains
`NonMatching`; no data, completion-label, semantic-exception, or
parked-function credit is claimed.

All eight inherited exact functions remain exact:
`_animation_left_foot_frame_index`, `_animation_right_foot_frame_index`, the
four biped lifecycle no-ops, `_biped_delete`, and
`_biped_stop_melee_attack`.

## Frozen target, type, and provenance evidence

January defines an external `void(long)` cdecl owner with 40 meaningful and
48 padded bytes. It obtains a verified biped through object mask `1`, then
stores `NONE` to dword offsets `+0x430`, `+0x434`, and `+0x448`. Its sole
relocation is a zero-addend `REL32` at function offset `+0x0A` to
`_object_get_and_verify_type`.

Current typed source independently authenticates the entire access path:

- `biped_get(index)` in `source/units/bipeds.h` wraps
  `object_get_and_verify_type(index, _object_mask_biped)`;
- `struct biped_datum` exposes the named nested fields
  `support_surface_index`, `pathfinding_surface_index`, and
  `last_pathfinding_surface_index` at the three target offsets;
- the shared `NONE` sentinel has the target all-bits-set value.

The complete registered Claude campaign corpus, Stian source, HCEA
later-build reconstruction, and current ledgers were searched before source
emission. The readable later-build sources agree that this owner retrieves a
typed biped and invalidates those three structure/pathfinding surface fields.
They provide semantic, type, and field-name provenance only; January PC COFF
remains the sole padded-byte and relocation authority.

Three other small absent biped owners were excluded before the selected
compile. `_biped_preprocess_node_orientations` and
`_biped_approximate_surface_index` require private helper register ABIs not
expressible by their visible C declarations. `_biped_place` requires canonical
scenario unit/biped datum types absent from the current tree, so no
source-local guessed layout was introduced.

## Retained typed source and house-rule audit

```c
void biped_disconnect_from_structure_bsp(
	long biped_index)
{
	struct biped_datum *biped;

	biped = biped_get(biped_index);
	biped->biped.support_surface_index = NONE;
	biped->biped.pathfinding_surface_index = NONE;
	biped->biped.last_pathfinding_surface_index = NONE;

	return;
}
```

The implementation uses the canonical typed object-access macro instead of a
raw `object_get` cast, places its parameter on its own line, keeps its C89
declaration before statements, uses named fields rather than byte offsets,
and ends with an explicit no-value return. It contains no assembly,
`register`, `volatile`, optimizer barrier, pragma, intrinsic, attribute, raw
address, pointer/integer reconstruction, pointer or union pun, undefined
behavior, synthetic anchor, object patch, comparator exception, or byte
forcing.

## Immutable artifacts and strict comparison

- Base commit: `2df4a22314ccc6050545d1f22e68b900078d3abf`.
- Pre-wave source blob: `33e62b2bbb9db68c714070e1b000636a38cff645`.
- Frozen retained source blob: `4cf6734fc4c5fff7247e71dc6251ee012dc6900a`.
- January split object SHA-256:
  `07AF03F457191E94D8D00C0921A056974A9CC4618279FFC09C91C2BCD0BB3721`.
- Baseline candidate object SHA-256:
  `5C478C399E58BDC740CAB5211F170B906648B2EB2B4D2B20D7695899A9DCB163`.
- First-shot object SHA-256:
  `BAEA99AE9459C208CD2514841CA6A6E57BED9095D2A3D414D1F000B88AEE426E`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1 with the unchanged natural
  `/O2 /Oy- /DDEBUG /Dxbox` flags and include graph.

The hardened comparator proves all 48 padded bytes, relocation count,
address, type, destination identity, addend, and ownership exact. Target and
candidate share normalized SHA-256
`c232ecc7bc22493c9fdefeb4e0c495395ad64d64b30d5c07b53647e5aa448410`.
The exact-set comparison reports only
`_biped_disconnect_from_structure_bsp` as newly exact, preserves all eight
inherited exact functions, and reports no loss.

## One-shot discipline and validation

`git diff --check` and the production compiler's no-output `/Zs` check passed
before emission. Exactly one normal selected-object compiler edge produced
the retained artifact. No type, macro, field, statement order, flag, or
compiler retry occurred.

The complete graph and campaign gates pass:

- 377/833 complete objects overall and 275/468 for Halo;
- 510,860/2,198,102 meaningful code bytes and 4,235/11,060 functions overall;
- 497,946/1,770,166 Halo code bytes and 4,068/7,574 Halo functions;
- data unchanged at 1,836,762/4,176,062 overall;
- semantic audit: 470 units, 4,347 functions evaluated, 4,201 semantic exact,
  4,261 accepted exact, and zero unit errors;
- admission: zero candidates, contradictions, or revocations;
- parks: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass, with only the inherited pytest cache-permission
  warning.

No protected source, configuration entry, completion label, push, amend, or
history rewrite is involved.
