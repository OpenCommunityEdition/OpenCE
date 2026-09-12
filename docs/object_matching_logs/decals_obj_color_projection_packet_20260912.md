# `decals.obj`: color packing and projection packet (2026-09-12)

## Outcome

This packet restores two previously unwritten January functions with natural,
typed source:

| function | meaningful | padded | relocs | result |
|---|---:|---:|---:|---|
| `_real_a_rgb_color_to_pixel32` | 260 | 272 | 14 | strict exact |
| `_decal_projection_create` | 668 | 672 | 14 | strict exact |

The packet therefore adds **928 meaningful / 944 padded exact code bytes** and
two strict function owners.  All inherited exact Decals owners remain exact.

## Reconstruction evidence

January's `_real_a_rgb_color_to_pixel32` section establishes both assertion
sites (`bitmaps_inlines.h` lines 243 and 244), the call to
`valid_real_rgb_color`, channel order, and the complete x87 stack schedule.
The implementation uses the same stack-local `255.0f` scale and in-memory
32-bit `FISTP` pattern already authenticated for `real_alpha_to_pixel32` in
`source/bitmaps/bitmaps_inlines.h`.  Its small inline-assembly block is limited
to that directly evidenced low-level color/math packet; the validation and
control flow remain ordinary C.

January's `_decal_projection_create` section establishes the private ABI,
matrix and extent copies, plane construction, dominant-axis selection, four
projected corners, texture axes, and reciprocal determinant.  The final exact
source uses the project's established math vocabulary:

- `plane3d_from_point_and_normal`;
- `projection_from_vector3d` and `projection_sign_from_vector3d`;
- `project_point3d` for each of the four extent corners;
- `vector_from_points2d` for the texture axes; and
- `cross_product2d` for the determinant.

The January target disassembly is authoritative.  The HCEA
`decal_projection_create` reconstruction and Pastudan's raw January decompile
were used only as independent semantic corroboration.  Replacing a first-pass
manual expansion of the texture-vector math with the proper project helpers
removed the remaining x87 scheduling differences without compiler steering.

## Isolated grouped gate

The final PID-safe whole-TU gate was:

```text
python -B tools/campaign/gate.py source/effects/decals --all \
  --forbid-emitted-symbol _point_from_line3d \
  --out scratch/decals_color_projection_packet_final.obj
```

It reports 26 exact, five residual, and two unwritten target owners.  Both new
sections match January in padded size, meaningful size, normalized bytes, and
all relocation identities.  Their normalized SHA-256 values are respectively
`50c2f830176fb14c624b5177983c19aba8c82b5e07eecf7a205b4149afe1e917`
and
`ff6617f9b07e46d2c4ce1a5e9a2ee77e668a91f2f3ee389d89a9e948ea14a4db`.
The forbidden `_point_from_line3d` emitted-symbol guard passes.

`tools/fake_match_scan.py` reports only the expected inline-assembly review
lead described above.  There are no raw-address names, `volatile` qualifiers,
pragmas, fake dependencies, forced-inline annotations, or untyped tag/object
accesses in the packet.  `decal_projection_create` is correctly defined and
declared `static` in its owning translation unit.  Signatures follow the
multiline house style and the void function ends with an explicit `return;`.

## Inline-helper ownership boundary

The integrated owner audit found four select-any math bodies that January does
not emit from `decals.obj`. The ordinary
`REAL_MATH_EXTERNAL_PROJECT_POINT3D` include schedule removes
`project_point3d` and its transitive `set_real_point2d` body without changing
any of the 26 exact target functions. `vector_from_points2d` and
`cross_product2d` cannot be externalized without changing the exact projection
caller, while ordinary `static __inline` and `extern __inline` forms still emit
their bodies. Forced inline was tested only as a rejected scratch probe and is
not retained.

The two remaining helper copies receive no bytes or function credit and are
registered as explicit whole-object admission rejections. The natural typed
`decal_projection_create` body remains independently valid exact-function
progress; this ledger does not claim whole-object Matching or link closure.

## `render_debug_decals` semantic-exact boundary

The complete public debug renderer is accepted for **427 meaningful code
bytes** in the semantic-exact tier, but is deliberately not labelled strict.
January and the candidate have identical 432-byte code sections, identical
normalized code SHA, and the same 27 relocation offsets, types, counts, and
resolved addresses. The fail-closed symbolic comparator reports one identity
difference at section offset `0x53`: January's final address is named
`_debug_decals`, while the candidate's source expression is represented as
`_decal_geometry-24`. Those expressions resolve to the same target address
because the loop computes the predecessor vertex with the authentic
`(base + count - 1)` indexing form and the two globals are adjacent.

`debug_decals` is independently established by the public Decals declaration
and the HS external-global registration, and its January address is now named
in `config/symbols.json`. No alias cast, fake dependency, or source rewrite is
used to hide the COFF ambiguity. The strict comparator remains fail-closed;
the function receives ordinary semantic credit under the existing
same-code/same-final-relocation-address policy and is not a fuzzy park.

## Public-header blast-radius audit

The related public `render_debug_decals(void)` declaration now lives in
`source/effects/decals.h`.  PID-isolated pre/post objects were compared for all
ten direct C includers.  Across the nine non-concurrent includers, all 363
emitted function sections and exact verdicts were unchanged.  The concurrent
`object_lights` owner independently confirmed its final gate with the
prototype present: all 35 inherited exact functions were preserved and the
`_point_from_line3d` guard passed.  The header change therefore has no observed
definition-position regression.

The root integration subsequently passed the full Ninja build, repository-wide
stable sweep, and header-consumer checks. Aggregate publication evidence is in
the sixth 10K batch ledger.
