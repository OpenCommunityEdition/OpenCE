# Fable Geometry clean-pair intake ledger (2026-09-06)

## Intake disposition

The donor's four claimed strict functions total 752 padded / 731 meaningful bytes. The independently admissible packet is the two first-compile, source-authentic functions below: 384 padded / 369 meaningful bytes. Geometry moves from 12/30 strict functions before the packet to 14/30 after it.

| Function | January offset | Padded / meaningful | Donor status | Disposition |
|---|---:|---:|---|---|
| `_convex_hull2d_perimeter` | `0xF5BB0` | 96 / 92 | exact after expression/helper variants | held; prior canonical first-shot ledger also records a natural mismatch, and the donor exact form brings helper-shape provenance concerns |
| `_convex_hull3d` | `0xF7450` | 272 / 267 | exact on first compile | admit with owner schema |
| `_convex_hull3d_test_point` | `0xF7560` | 112 / 102 | exact on first compile | admit with owner schema |
| `_convex_hull3d_test_vector` | `0xF75D0` | 272 / 270 | exact after helper variants | held; donor form emits `_plane3d_distance_to_point` and `_dot_product3d` helpers absent from the January Geometry object |

No perimeter, vector, residual geosphere, helper, or filler source is in the clean pair.

## Source and owner proof

- Frozen pair source: `scratch/geometry-fable-owner-pair-20260906/files/source/math/geometry.c`, SHA-256 `20c7baa58e7e1e4eaa93be5f7ddbf752c76684b2e8d61a8f5b3ccde0c9db0990`. Relative to the frozen before source, it has one 65-line insertion at the former line index 507 and defines only `convex_hull3d` and `convex_hull3d_test_point`.
- Frozen owner header: `scratch/geometry-fable-owner-pair-20260906/files/source/math/geometry.h`, SHA-256 `04f7b7b7d027c0621d93410abd856c9cb42b6268164ad8db4a78647d91c5411f`. It is byte-identical to the schema-only header and adds exactly three records plus the four public prototypes for begin, expand, wrapper, and point test.
- Release-PDB evidence: `C:/halo-worktrees/fable-50k-small-families-20260904/scratch/w_geometry_pdbtypes.txt`, SHA-256 `578b3216c79e737b8a6aced6028682a2ec929929a7d9f29cda70e46a4d3d75e2`, lines 1126854-1126878. Layouts are `vertex3d` 12 bytes (`extant+0`, `point_index+2`, `edge_index+4`, `user_data+8`), `edge3d` 32 bytes (`extant+0`, arrays at `+4/+C/+14`, `user_data+1C`), and `surface3d` 28 bytes (`extant+0`, plane `+4`, `edge_index+14`, `user_data+18`). Canonical `boolean` is the authentic unsigned-byte type.
- January and config evidence make `convex_hull3d_begin` and `convex_hull3d_expand` public external functions (COFF type 32, storage 2; config entries are non-static at `0xF6770` and `0xF6D40`). The clean wrapper therefore calls authentic APIs, not private facades. Their canonical bodies remain unwritten, so this packet is partial credit and does not claim a complete canonical link closure.
- The wrapper preserves the January assertion order and lines 2284-2287, calls begin once, then expand for signed-short indices `[0, point_count)`, returns false on either failure, and otherwise returns true.
- The point test uses the source-authentic explicit plane expression. The exact object confirms a 28-byte surface stride and field accesses at `extant+0`, normal `+4/+8/+C`, and distance `+10`; its only relocation is the existing `_global_convex_hull3d_epsilon` owner.

Direct include closure is exactly six units: Geometry plus `game_engine_king`, `breakable_surfaces`, `leaf_map`, `structure_lens_flares`, and `structure_visibility`. The schema-only objects for all six are byte-identical to before after zeroing only the COFF timestamp. In the pair build, all five non-owner consumers remain timestamp-normalized byte-identical. The owner retains every inherited function and COMMON record and adds exactly the two functions plus four assertion literals.

## Object and literal ownership

- January target: `scratch/geometry-fable-owner-pair-20260906/files/build/split/source/math/geometry.obj`, SHA-256 `9a223d12be60d64dfed8930aeb8366a33b92c82a693a341b7e10beef0682740c`.
- First clean candidate: `scratch/geometry-fable-clean-pair-first-20260906.obj`, SHA-256 `3b4b1a60c4536730da1374725d5905046621dc79fd8c3315a3e85046b463af94`.
- Frozen actual: `scratch/geometry-fable-owner-pair-20260906/source/math/geometry.obj`, SHA-256 `c3ab4ebdedeb476cc64bc4a0e8db747c05616fe4d7e121e75ee5b8863725171c`.
- `_convex_hull3d`: 272 bytes, 18 relocations, normalized SHA-256 `d9c64e0f0e98d708c3746dfb07260bc5d359100baa561935645f7d9779dc114b` in all three objects.
- `_convex_hull3d_test_point`: 112 bytes, one relocation, normalized SHA-256 `bebd453d4592cbd7e4a7e64228c863bd6049caba61f8c5bd76a6542cf79b7568` in all three objects.

A fail-closed scan parsed exactly 572 base objects and 833 split objects. All 12 definitions of the four exact assertion literals have identical bytes and COMDAT selection 2 (ANY); there is no selection-1/NODUP veto:

- Actual Geometry owns all four.
- Canonical competitors: `source/objects/widgets/lightning.obj` and `source/render/triangle_strips.obj` own `vertices`; `source/render/render_debug.obj` and `source/tool/error_geometry.obj` own `points`.
- January competitors: target Geometry owns `vertices`, `edges`, and `surfaces`; target ErrorGeometry owns `points`.
- The four exact split-side undefined references (`Geometry:points`, `lightning:vertices`, `render_debug:points`, `triangle_strips:vertices`) each have matching selection-2 owners. This establishes literal safety only; it does not change the pending canonical begin/expand implementation status.

## Runtime differential

Oracle: `scratch/geometry_fable_clean_pair_runtime_differential_20260906.py`, SHA-256 `874a92dac1108a9d9e9b282cb14092333bfb39f0f9cf0a247bde303865a76604`.

Replay command from the canonical root:

```text
python scratch/geometry_fable_clean_pair_runtime_differential_20260906.py
```

Result: PASS / exit 0. Sixteen cases execute independent January, first-candidate, and frozen-actual bytes with zero semantic failures and zero target mismatches:

- Wrapper (8): begin failure, expand failure, success, zero points, and null assertion for each of points/vertices/edges/surfaces.
- Point predicate (8): zero surfaces, nonextant skip, inside, outside, exact epsilon boundary, NaN, positive infinity, and mixed extant records.
- State/ABI: full modeled geometry memory, exact external-call order/arguments/stack depth, caller frame, EAX/AL trace, ESP, return address, direction flag, and EBP/EBX/ESI/EDI.
- Seven negative controls prove detection of external argument/order, AL, ESP, callee-saved register, modeled state, and caller-frame mutations.

The wrapper stubs only the genuine public begin/expand APIs with deterministic returns. Fatal paths stop after authentic `display_assert` followed by modeled noreturn `system_exit(-1)`. No behavior of the still-unwritten begin/expand algorithms is claimed.

## Primary donor evidence

- `C:/halo-worktrees/fable-50k-small-families-20260904/scratch/reports/geometry.md`, SHA-256 `68abc43fee12b99380e83487da81c35c9be34a1e4602743574188bd3db19bb70`.
- `C:/halo-worktrees/fable-50k-small-families-20260904/scratch/facts/geometry.md`, SHA-256 `2277486a86b107a5f1abce784a4efc7114543e1d5efa08da0f54bd76597ed15c`.
- Frozen manifests: `scratch/geometry-fable-owner-before-20260906.json` (`5a236f...`), `scratch/geometry-fable-owner-schema-20260906.json` (`d77471...`), and `scratch/geometry-fable-owner-pair-20260906.json` (`b5aa38...`).
