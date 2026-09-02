# `connected_geometry.obj`: Fable salvage and house-rule reconciliation (2026-09-02)

## Outcome

This lane audited Fable snapshot `dd2eaca29` against pinned canonical
`fca0dd5d7103a7cb331106c61a86dc26e8c84356`, preserved the raw import as
`7d7229efd`, and then reconciled it into the strongest source-plausible
version under the campaign house rules.

The result retains all six canonical strict-exact functions and replaces the
three unwritten add-triangle owners with typed, coherent C.  Those three
owners remain fuzzy and are not added to strict totals.  The warning flag and
read-only data now match January exactly.

| Measure | Canonical baseline | Raw Fable snapshot | Reconciled | Delta from baseline |
| --- | ---: | ---: | ---: | ---: |
| Strict-exact functions | 6 / 10 | 6 / 10 | 6 / 10 | 0 |
| Strict meaningful code bytes | 894 | 894 | 894 | 0 |
| Strict padded code bytes | 944 | 944 | 944 | 0 |
| Strict data bytes | 0 / 137 | 137 / 137 | 137 / 137 | +137 |
| TU fuzzy similarity | 60.38571% | 91.121864% | 97.219604% | +36.833894 points |
| Unwritten functions | 3 | 0 | 0 | -3 |

The newly reconstructed target scope is 835 meaningful / 848 padded code
bytes.  This is an honest fuzzy-source gain, not a byte-exact-code claim.

## Address-normalized zero-regression set

The canonical baseline object and the reconciled object both compare equal to
January for the same six function identities and addresses:

| January address | Function | Meaningful / padded bytes | Normalized SHA-256 |
| ---: | --- | ---: | --- |
| `0xF2CD0` | `_connected_geometry_new` | 43 / 48 | `d85f2504e2ffcdd236b7e29ae2beda66e0f3ad33a7b3cef3bc6292946d4200c1` |
| `0xF2D00` | `_connected_geometry_delete` | 79 / 80 | `e74305d7f27a6a8ad73684af18843186b6e4f76971a75f30828609519dbb1c7f` |
| `0xF2D50` | `_connected_geometry_group_recursive` | 197 / 208 | `4ca7c404a14987765c235196243b2a9fe8f75f38e0ca4a47aa9270679117bd08` |
| `0xF2FD0` | `_plane3d_from_points` | 172 / 176 | `44d64972b09abae673273d594bfffe68ee1058ecbe300c3eb0f20fe51f97cddf` |
| `0xF33A0` | `_connected_geometry_add_intermediate_triangle` | 114 / 128 | `cc2602c2c2ac3bcf96b17b3301d9f50337c39cc318fcd07682a07c25cf172a67` |
| `0xF3420` | `_connected_geometry_group_coplanar` | 289 / 304 | `4135a273f037ad0dad9f2a5b7de863ed03749b945fe4de3894104387695e431a` |

`coff_compare.py` reports `all_equal: true` for this set against both the
frozen baseline and the final candidate.  The comparison is keyed by January
function identity/address rather than by list position.

## Reconciled source and ownership

The raw snapshot duplicated point, vector, plane, edge, triangle, and geometry
records inside the translation unit and carried local inline math helpers.
The reconciled source instead uses the natural subsystem owners:

- `tool/connected_geometry.h` for the geometry records and public API;
- `math/real_math.h` types and helpers through that owner header;
- a new `tool/error_geometry.h` for the externally owned
  `error_geometry_triangle` prototype.

The code uses `real`, `real_point3d`, `real_plane3d`,
`NUMBER_OF_EDGES_PER_TRIANGLE`, and `SET_FLAG`.  Private functions and the
one-shot global now have semantic names:
`connected_geometry_find_or_add_vertex`,
`connected_geometry_find_or_add_edge`, `triangle_coplanar`, and
`warned_about_duplicate_triangles`.  The last is marked static in the split
symbol description.

All retained functions use multiline signatures, explicit returns, typed
records, and ordinary defined C.  The lane retained no inline function, inline
assembly, `volatile`, `register`, raw-offset access, fake dependency, codegen
pragma, or inert expression.  The final object does not emit a
`point_from_line3d` COMDAT, preserving the January inline schedule.

## Evidence

The Stian donor independently reconstructs the same private cluster in
`src/halo/main/main.c`:

- `FUN_00103600` is the three-component epsilon vertex lookup/append helper;
- `FUN_001036c0` is the unordered edge lookup/append helper with bit 31 as
  traversal direction;
- `FUN_00103860` is the five-argument triangle builder and duplicate scan;
- `FUN_001037b0` and `FUN_00103a00` corroborate the plane and coplanar paths.

The January call graph, element sizes, relocation destinations, signed/short
loop counters, warning string, and data references independently corroborate
that mapping.  HaloCEA's
`src/blam/structures/plane3d_from_points.c` agrees on the typed point/plane
contract and the degenerate-plane `NULL` result.  January remains authoritative
where donor source and target instruction order differ.

The Marathon source was also searched narrowly rather than treated as a
drop-in donor.  `marathon2/shapes.c::find_or_add_color` and
`marathon2/motion_sensor.c::find_or_add_motion_sensor_entity` establish that
`find_or_add_*` is authentic Bungie-family naming for index-returning linear
lookup/append helpers.  Marathon's indexed map topology uses endpoints and
lines and contains identity-based contains-then-append loops, but it has no
direct analogue for coordinate welding, unordered edge creation, duplicate
triangles, arbitrary-plane coplanar grouping, or a plane from three points.
It therefore informs naming/style only; it does not override the January and
Stian evidence for this object's semantics.

## Honest residuals and parks

| Function | Target / candidate padded bytes | Relocations | Objdiff | Disposition |
| --- | ---: | ---: | ---: | --- |
| `_connected_geometry_find_or_add_vertex` | 192 / 192 | 8 / 8, exact addresses and destinations | 91.02597% | Formally parked |
| `_connected_geometry_find_or_add_edge` | 240 / 256 | 7 / 7, same destinations but candidate addresses `+4` | 85.57143% | Honest fuzzy; not formally parked |
| `_connected_geometry_add_triangle` | 416 / 416 | 14 / 14, same destinations; first call is `+1` | 98.37838% | Honest fuzzy; not formally parked |
| `_triangle_coplanar` | 384 / 384 | 11 / 11, exact addresses and destinations | 99.87755% | Existing canonical park retained |

The vertex helper has the same 80 decoded instructions and complete relocation
packet as January.  Its residual is x87 lifetime and final copy-register
selection: January carries `point.y`/`point.z` over getters while VC7 spills
the same values through a four-byte local.  Its final target/candidate hashes
are respectively
`c21c80fafbc4175df12584cdd83cf153b979272feeab86c9354405901df8253a` and
`e42aac4dc8c567a7fd1f8072038969e00f4fce226c654ca16451e47d20c9afc0`.

The edge helper deliberately initializes the direction byte.  January leaves
that automatic byte indeterminate only on the invalid negative-edge-count
path; reproducing that read would violate the no-undefined-behavior rule.
The lawful initialization accounts for its larger frame and shifted
relocations, so it is retained as fuzzy source but does not qualify for a
formal park.

The triangle builder now matches January's 416-byte envelope, 158 decoded
instructions, and relocation destinations.  Only the first call relocation is
one byte later and the residual is confined to the callee-save `EBX`
lifetime/push schedule.  Its target/candidate hashes are respectively
`2207c1a6a0dc6790fb1dc54aeb46852933f3e0d6efb28bffeb65cf011d2c7958` and
`99b62ac3cc8fbcc510843d7201338fbdedf6318914f7990121a15b4781638db2`.

The existing coplanar park remains unchanged; its only residual is the already
documented dependency-free x87 operand choice.

## Exact data

The final semantic report records 137 / 137 target data bytes exact:

- `.bss`: 1 / 1, the static duplicate-warning flag;
- `.rdata`: 136 / 136, including the warning text and coplanar epsilon.

The duplicate-warning flag's normalized hash is
`6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`;
the warning string's normalized hash is
`40b21ff5aaf29ea03e76e6fb09c82ddac7de9ddf93df71b2253a62c94a2b8c43`.

## Verification

- isolated gate: `exact 6, residual 4, unwritten 0`;
- isolated targeted Ninja build of `connected_geometry.obj`: pass;
- address-normalized baseline/final strict-set sweep: six unchanged exact,
  zero regressions;
- semantic report: 97.219604% TU similarity, 137 / 137 data bytes exact;
- parked audit: two active for this unit, zero stale, zero invalid;
- object-admission audit: zero candidates, contradictions, rejections, or
  revocations;
- fake-match scan of the touched source/header pair: zero review leads;
- targeted tooling tests: 103 passed;
- prohibited-source and `point_from_line3d` COMDAT scans: clean;
- `git diff --check`: clean apart from Git's local LF/CRLF advisory.

Per lane isolation requirements, this salvage did not run shared full Ninja,
modify canonical, or push.  Canonical integration must rerun the full-tree
build and exact-set regression sweep after applying the two commits.
