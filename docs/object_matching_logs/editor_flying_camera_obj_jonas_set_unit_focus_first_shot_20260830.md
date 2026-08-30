# `editor_flying_camera.obj` unit-focus first-shot boundary (2026-08-30)

## Result

The first and only natural typed candidate for
`source/camera/editor_flying_camera.obj::_editor_camera_set_unit_focus`
reached January's complete 112-byte boundary and emitted the same ten
relocation destinations, but its control-flow order and normalized code are
not strict exact. The candidate was inverse-reverted without a source-shape,
branch, declaration, or pointer-lifetime retry.

| Measurement | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful / padded bytes | 112 / 112 | 112 / 112 |
| Relocations | 10 | 10 |
| Normalized SHA-256 | `814efca3fe23ba03691d332d34781b13bffdd8f2d8815db78fba22dfa70dfdcf` | `94197e2f9d2ed3c5ca2a1606a28e598373504302e27e3e03c19d79b106e529e8` |

`editor_flying_camera.obj` remains 10/21 exact functions and 656/5,184
strict padded bytes. No source, header, data owner, configuration, semantic
exception, parked entry, completion label, compiler control, or production
object change is retained. This additive ledger is the lane's only tracked
change, and nothing is pushed.

## Authority and provenance

This isolated lane began at canonical commit
`1c6f96dc088dbd19d61a3cf2fe2bbba5bde9c981`. The January executable has
SHA-256
`4CC87B45F721270392A96F1674ED2B5CD4A7BB4355FAEAB4531D1CF1884D9520`;
the split `editor_flying_camera.obj` has SHA-256
`5E0066E8F28B6C2C79E9E546075922B263342DEA5A50D9EF84E8818792D99945`.
The unchanged Microsoft VC7 13.00.9254.1 compiler has SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`
and uses the ordinary `/O2 /Oy- /DDEBUG /Dxbox` edge.

The authenticated HCEA donor is commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`. Its
`src/blam/camera/editor_camera_set_unit_focus.c` is Git blob
`42ef227291aa33c794d647e36ef16a309da17140`, payload SHA-256
`9AF9B251F73CD8DDE47B5A4C229DDD6613F8A6A374B9307E7E86C9E081B37844`.
It independently fixes the public 32-bit object-index argument, persistent
focus-index store, no-camera early exit, zero-vector assignment for `NONE`,
and three component differences between the camera position and the focused
object's bounding-sphere center.

January's typed PC topology independently fixes the remaining ABI facts:

- `_data_002dcc28+4` is the existing `long unit_focus` field;
- `_bss_0031d438+0x18` is the existing `struct flying_camera *camera`;
- `_bss_0031d438+0x20` is a complete `real_vector3d unit_offset`;
- repository macro `object_get(unit_index)` naturally calls public
  `_object_get_and_verify_type` with `_object_mask_all == -1`; and
- `struct object_datum::object.bounding_sphere_center` is naturally at
  `+0x50`, while `struct flying_camera::position` begins at zero.

The repository acceleration playbook, matching methodology, COFF and campaign
tooling documentation, all four pre-existing Editor Flying Camera ledgers,
applicable Claude doctrine, owner source/header, callable dependencies,
reachable Git history, and local donor/reference trees were audited before
mutation. No repository-root `AGENTS.md` or `CLAUDE.md` applies. All-ref
history and the registered owner-source census found no prior definition,
measured candidate, parked entry, or descriptive freeze for this function.

## Frozen January packet

The target caches the camera pointer in ESI, stores the focus index, and exits
when no camera exists. The ordinary object branch falls through first; it
calls `object_get`, subtracts the three bounding-center components, and stores
the vector. The `NONE` branch is a later tail that copies
`*global_zero_vector3d`.

The exact target relocation schedule is:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `+0x09` | `DIR32` | `_bss_0031d438 + 0x18` |
| `+0x10` | `DIR32` | `_data_002dcc28 + 4` |
| `+0x1F` | `REL32` | `_object_get_and_verify_type` |
| `+0x30` | `DIR32` | `_bss_0031d438 + 0x20` |
| `+0x3C` | `DIR32` | `_bss_0031d438 + 0x24` |
| `+0x49` | `DIR32` | `_bss_0031d438 + 0x28` |
| `+0x50` | `DIR32` | `_global_zero_vector3d` |
| `+0x58` | `DIR32` | `_bss_0031d438 + 0x20` |
| `+0x61` | `DIR32` | `_bss_0031d438 + 0x24` |
| `+0x69` | `DIR32` | `_bss_0031d438 + 0x28` |

The candidate declared no storage. All grouped data and BSS owners remained
external and received no credit.

## Sole frozen candidate

The source refined the already authenticated camera-global envelope with a
named `real_vector3d unit_offset` at `+0x20`, compile-time named-field layout
checks, the public prototype, and the natural body below:

```c
void editor_camera_set_unit_focus(
	long unit_index)
{
	struct flying_camera *camera;
	struct object_datum *unit;

	camera = bss_0031d438.camera;
	data_002dcc28.unit_focus = unit_index;
	if (!camera)
		return;

	if (unit_index == NONE)
	{
		bss_0031d438.unit_offset = *global_zero_vector3d;
	}
	else
	{
		unit = object_get(unit_index);
		bss_0031d438.unit_offset.i =
			camera->position.x - unit->object.bounding_sphere_center.x;
		bss_0031d438.unit_offset.j =
			camera->position.y - unit->object.bounding_sphere_center.y;
		bss_0031d438.unit_offset.k =
			camera->position.z - unit->object.bounding_sphere_center.z;
	}

	return;
}
```

Candidate source/header Git blobs were
`796c5fd5d2ad76e8a0102679a9748e596f338065` and
`3aed0dfdde5299dce43a6e5f06ee791afcd238ae`. Their transient working-file
SHA-256 values were respectively
`0A46D6ED5E12FCD7CB96E196B9FC71F1837B00254581F3DCA92F5FDD70AB034E`
and
`4F8FC8DB5B4F5933794D971EF03C5ECC73B136F6A64C29A5D46BE530074429F7`.
VC7 `/Zs` passed before the one code-producing adjudication compile. The
candidate object had raw SHA-256
`F9E3EB9B34F0D6B0E53D4B74E79B0F2090EC6B2AB663420A03F12CAD73A7862E`.

The candidate used readable typed C89 only. It contained no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, barrier, raw address or
offset access, pointer/integer reconstruction, representation pun, undefined
behavior, byte forcing, compiler/tool exception, or comparator waiver.

## Fixed boundary and reopening condition

The first 26 bytes, including camera caching and the first two relocations,
match January. The remaining structural boundary has two coupled parts:

1. HCEA's natural `if (unit_index == NONE)` spelling keeps the zero-vector
   branch first in VC7 output. January places the ordinary object branch first
   and moves the zero-vector copy to the tail, so the eight remaining
   relocation addresses and order differ.
2. After the first component subtraction, January advances the returned
   object pointer by `0x50` and accesses the remaining center components at
   `+4` and `+8`. The candidate retains the object base and addresses them at
   `+0x54` and `+0x58`. That is a source-level subobject-pointer lifetime/CSE
   topology absent from the authenticated donor, not a type or layout error.

Do not repeat this body, reverse the branch merely from the observed bytes,
introduce an artificial center pointer, or vary declaration order. A future
one-shot may reopen only if newly authenticated PC source or equivalent
independent ordinary-C topology fixes both the branch form and center-pointer
lifetime before compilation. No tuning or second candidate was produced here.

## Restoration and gates

The source and header were inverse-restored to their exact pre-lane Git blobs
`473a74da467d7074b43445df7722ba045730fd37` and
`131ee7526499b50255d10783a0d6964c845f77a8`. The candidate symbol is absent.
The final restored owner object has raw SHA-256
`39BCCDB4B1161919C49B8B63E899ACBF1B7EDF3CD7AA5C6A4A043AA5ED587762`.
The whole-object exact-set comparison reports zero lost and zero newly exact
sections; all ten inherited exact functions remain strict. The isolated
campaign gate reports ten exact, zero residual, and eleven unwritten functions.

The complete `halobetacache_build`, `libcmt_build`, `progress`, and
`semantic_progress` graph passes. Semantic output reports 470 units, 4,950
evaluated functions, 4,833 semantic-exact functions, 4,843 accepted-exact
functions, and zero unit errors. Strict progress is 384/833 objects,
4,804/11,060 functions, 609,995/2,198,102 meaningful code bytes, and
1,856,050/4,176,062 data bytes. Halo progress is 282/468 objects,
4,637/7,574 functions, and 597,081/1,770,166 meaningful code bytes.

Protected `_unit_preprocess_node_orientations` remains exact at 1,920 padded
bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reports zero candidates, contradictions, and revocations. Parked
validation reports 12 active, zero stale, and zero invalid entries. Pytest
passes all 212 tests. `git diff --check` passes.

`build/report.json` is 1,600,388 bytes with SHA-256
`1EC44DD8B0CBDE37AA82F84EA6CED758FFA611ADD74E6FDB77535A2CD93982FE`.
`build/semantic_report.json` is 3,289,061 bytes with SHA-256
`91463BAAC7970260EB520E071CF77CBADCAC80EACE4D8C7DF2C625AB0445B4B2`.
