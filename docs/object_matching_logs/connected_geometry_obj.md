# connected_geometry: canonical reconciliation ledger

## 2026-10-01: triangle predicate exact

Reconciled against canonical `6bfd95673719d54b92d0b23f01dc9df4c0679768`.
The natural same-compiler donor is punpckhdq/halo commit
`5d7fddc37f76194e5d151ce829a1cf25ca821ce8`, inspected at upstream
`6a23ce979ac09f7ca032449ca205ceb82eac4b81`. This is a reconstruction,
not recovered Bungie source text.

`_triangle_coplanar` now uses the existing real-math helpers and ordinary
typed dynamic-array access. The donor's accessor macro was removed in a
pre-registered control; exactness survived. The plain 0.01f coplanarity
constant replaces the same existing literal. No shared header was changed.

Strict extent, bytes and all relocation identities match January:
380 meaningful / 384 padded bytes, 11 relocations, normalized SHA-256
`02acd868e062aee29df8e260c3fbd62221e3f2ced5e5f43573a1a288e6129d54`.
The new 48-byte `_plane3d_distance_to_point` select-any copy equals both
January's selected copy and the current decals provider. It earns no
duplicate credit; provider links pass in both orders.

The unit is 9/10 strict exact. `_connected_geometry_find_or_add_vertex`
remains residual and parked; its epsilon/grouping-macro diagnostic remains
held. The inherited January/current COMDAT-selection discrepancy for
`_plane3d_from_points` is unchanged. No object admission is claimed.

The triangle park is retired after fresh verification. Full checkpoint,
independent ownership/provider review and receipt pins are recorded in
[the two-function reconciliation](upstream_two_reconcile_20261001.md).
Earlier negative probes and reopen history remain in
`connected_geometry_obj_opus5_next150_n2_20260915.md` and the other
date-stamped connected-geometry records.
