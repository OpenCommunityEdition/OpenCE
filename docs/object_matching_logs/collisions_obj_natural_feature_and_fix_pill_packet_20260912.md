# `collisions.obj` natural feature/fix-pill packet (2026-09-12)

## Result

This packet admits one new strict function and retains four complete, coherent
reconstructions as zero-credit fuzzy work:

| function | January padded bytes | result |
| --- | ---: | --- |
| `object_get_features_in_sphere` | 544 | strict exact; 533 meaningful bytes |
| `collision_test_vector_exit` | 400 | fuzzy; identical size and 6 relocations |
| `collision_get_features_in_sphere` | 512 | fuzzy; identical size and 25 relocations |
| `collision_fix_pill` | 672 | fuzzy; identical size and 28 relocations |
| `collision_fix_pill_nudge_collision` | 160 | fuzzy; candidate 144 bytes with 5 relocations |

The whole translation-unit gate remains at 9 exact, 4 residual, and 7
unwritten functions. The exact set includes every inherited match. The emitted
symbol guard confirms that this translation unit does not define
`point_from_line3d`.

## Source and naming evidence

The retained implementations and private names are independently supported by
the later HCEA reconstruction under `halocea-review/src/blam/physics/`:

- `object_test_point.c`
- `object_test_vector.c`
- `collision_test_vector_exit.c`
- `collision_get_features_in_sphere.c`
- `collision_fix_pill.c`
- `collision_fix_pill_nudge_collision.c`
- `clip_position_to_plane.c`
- `clip_velocity_to_plane.c`
- `clip_position_to_line.c`
- `clip_velocity_to_line.c`

January COFF remains the byte, ABI, and relocation authority. The HCEA files
are used only as semantic and naming evidence. Address-derived names were
replaced in `config/symbols.json`; no address-derived identifier is retained in
the reconstructed source.

`collision_fix_pill_offsets` is the 17-entry table recovered from HCEA's
`src/data/offsets.c`. Its values and order match that source. The collision
usage timer layout is translation-unit private and names the three observed
timer roles rather than retaining a BSS-address identifier.

## House-rule boundary

The four residual bodies use ordinary typed C, named flags and constants, and
the owning headers. They contain no volatile/register forcing, fake dependency,
compiler pragma, inline assembly, raw-offset access, or nonsensical branch.
They therefore remain useful source but receive **zero exact credit**. No more
source-shape tuning is warranted without new January-era source/compiler
evidence.

The scalar `COLLISION_POINT_FROM_LINE3D` expansion deliberately preserves this
translation unit's January inline schedule without selecting the external
`point_from_line3d` COMDAT. This is an emitted-symbol invariant, not a matching
claim.

## Remaining high-leverage work

`collision_move_point` remains unwritten at 4,752 padded bytes, together with
its four small private clipping helpers (336 padded bytes total). Their names
and semantics are known, but the helper register conventions are rooted in the
large caller. Reconstruct the caller and helpers as one packet; do not create
fake standalone callers merely to force private symbol emission.
