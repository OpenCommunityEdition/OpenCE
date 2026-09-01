# `structure_lens_flares.obj` authenticated fuzzy closeout (2026-09-01)

## Outcome

`source/structures/structure_lens_flares.obj` is retained as a credible fuzzy
reconstruction rather than falsely labelled complete. Three of its four
January functions are strict exact:

- `_compare_temp_markers`: 32/32 padded bytes, zero relocations;
- `_cluster_index_from_point`: 96/96 padded bytes, three relocations;
- `_structure_lens_flares_place`: 16/16 padded bytes, two relocations.

The remaining `_build_structure_lens_flares` body is 99.29274% in objdiff.
Target and candidate are both 4,336 padded bytes, contain 1,327 decoded
instructions, and have 156 relocations. The residual changes instruction
scheduling and local/register lifetimes; several relocation positions move
with that schedule, so equal relocation count is not overstated as positional
equality. All 1,144 bytes of target-owned data are exact.

## Source and name provenance

The retained control-flow and local-lifetime topology comes from the Fable
witness documented in:

- `structure_lens_flares_obj_fable_witness_reconstruction_20260829.md`;
- `structure_lens_flares_obj_fable_two_byte_frontier_20260830.md`;
- `structure_lens_flares_obj_jonas_fable_uniform_boundary_20260830.md`.

That witness records roughly 15,000 compile probes and independently explains
the scoped `real ds_x` temporary, real-plane projection topology, rotated lens
flare search, and late `offset` assignment. The earlier long comparator name
was explicitly an invented descriptive label. The September 2001 cachebeta
linker-map artifact in the local Halo symbol atlas instead authenticates
`compare_temp_markers`, followed by `build_structure_lens_flares` and
`structure_lens_flares_place` in the same compiland. Its +0x80 and +0x10F0
spans match the January comparator-to-build and build-to-place boundaries.
The canonical symbol is therefore `_compare_temp_markers`.

## House-rule reconciliation

The reconstruction uses project `real`, point, vector, and plane types; named
shader-type constants; correct connected-geometry structures and prototypes;
and the geometry-owned `convex_hull2d` prototype. No address-derived private
name, raw BSS name, wrong `fi4` cast, fake dependency, volatile compiler
steering, barrier, undefined behavior, assembly, or nonsensical branch is
retained.

The January line update is deliberately written as three scalar assignments.
Calling `point_from_line3d` produced a candidate-only helper COMDAT, violating
the standing inline-schedule rule. The final candidate contains no
`point_from_line3d` symbol or section. It does contain the 32-byte shared
`fast_ftol` COMDAT selected by the recovered helper calls; this is allowed
authenticated helper code but receives no object-completion credit.

Prototype ownership required one compiler-sensitive exception. Putting the
correct `structure_lens_flares_place(void)` prototype in the broad
`structures/structures.h` header regressed protected `units.obj` from 189/189
to 188/189 because VC7 allocation changes with declaration position. An
old-style empty-parameter declaration avoided the regression but was rejected
as an incorrect prototype. The dedicated natural owner header
`structures/structure_lens_flares.h` holds the typed build/place prototypes,
is included by the implementation and `hs.c`, removes the use-site prototype,
and preserves both protected objects.

## Measurements

Hardened `tools/coff_compare.py` records for
`_build_structure_lens_flares`:

| artifact | padded size | relocations | normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | 4,336 | 156 | `40aa8e486e52fe258065ae9cc9bd892679b15fd7845008cc0ebe1651788cf33b` |
| retained candidate | 4,336 | 156 | `57d6f033f379373528ec40ed132b8af7db73b6f1a087be9ddc3ad2f30d55fc46` |

The function is recorded in `config/parked.json` as an
`instruction-scheduling` residual. Reopen it only for authoritative January
source/local records or a natural same-compiler scheduling donor.

## Verification

- `python tools/campaign/gate.py source/structures/structure_lens_flares --all`:
  3 exact, 1 residual, 0 unwritten;
- `python tools/campaign/gate.py source/units/units --all`:
  189 exact, 0 residual, 0 unwritten;
- `python tools/campaign/gate.py source/hs/hs --all`:
  unchanged at 445 exact and 3 pre-existing residuals;
- `python -m tools.parked_functions --report scratch/structure_full_report.json`:
  the new record is active, with zero stale or invalid records.

No completion credit is claimed for `structure_lens_flares.obj`; the closeout
preserves the best authenticated source while making its residual explicit.
