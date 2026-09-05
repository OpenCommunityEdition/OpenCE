# Projection owner correction: current-context recheck (2026-09-04)

## Scope

This is a scratch-only recheck against published canonical commit
`ebba3978e494f0f91af355d35746e3dad1436ea6`. It tests the exact natural
`projection_from_vector3d` correction preserved in commit
`9633a0b4cca5519b46dde3ecb4df1694b419092b` and documented in
`real_math_projection_from_vector3d_correction_20260904.md` at that commit.
It is not a new expression, declaration-order, or compiler-control search.

The corrected helper selects `_z` when the absolute k component is at least
both other components, `_y` when j is at least i, and `_x` otherwise. The
published helper's `return j < i` instead reverses the x/y dominance result
and does not preserve the target's unordered-comparison path. January's
80-byte, zero-relocation helper in Decals is the byte authority, with hash
`70d35416dc81990b367872a8cb7c8b2326f1c79396a84350e27f326f4bdda8dd`.

The shared header was changed once, the four focused translation units were
compiled to independent scratch objects, and the header was restored with
the inverse source patch. No Ninja/build object, configuration, park entry,
or production source change is admitted by this check.

## Results

| Consumer | Current-context outcome |
| --- | --- |
| Decals | 14 exact / 2 residual / 17 unwritten. The inherited 400-byte `decals_delete_permanent_from_cluster` exact still becomes residual. No other strict owner is lost. |
| Game Engine | 169 exact / 11 residual / 0 unwritten; no strict gain or loss. The earlier `populate_statistic_buffer` regression does not recur in the current genuine header context. |
| Units | All 189 exact functions remain strict; no gain or loss. |
| Leaf Map | 9 exact / 2 residual / 14 unwritten; gains the 176-byte `leaf_face_get_vertex3d` owner with no strict loss. The forbidden `_point_from_line3d` emission guard passes. |

The Decals target/published hash for the affected owner is
`b3eeb427eeef1e02f793286b1011a5b89822e041f0b442759259255e1117220a`;
the corrected-header candidate hash is
`6fb830e33513eb040f0ff7cb46cc9167d30a96893dedb145370b416426025ffd`.
The newly exact Leaf Map owner has target/candidate hash
`5b81d3746a42006830ced8346e16daa118c568fdbdae1ec0eaa26dc3096603ac`.

Scratch artifacts, relative to the canonical worktree:

- `scratch/projection-current-decals-20260904.obj`
- `scratch/projection-current-game-engine-20260904.obj`
- `scratch/projection-current-units-20260904.obj`
- `scratch/projection-current-leaf-map-20260904.obj`

## Decision and limitations

The old blocker has narrowed, not disappeared. Because the Decals regression
still occurs, this check did not repeat the full shared-header consumer build
or the earlier Bitmap Group park audit. It therefore does not certify those
other consumers, owned data, or all parks as stable.

The correction remains preserved outside canonical. The Leaf Map private
caller family must not be imported using the known-wrong projection body,
and its scratch exact gain receives no published byte credit. Conversely,
the ordinary Decals loop must not be rewritten through equivalent spellings
merely to recover the historical allocator result. Reopen with genuine
source/type/caller evidence that changes the affected context, or as an
explicitly reviewed semantic correction with its collateral effects fully
measured; do not repeat this same diagnostic without new evidence.

Canonical admitted delta from this recheck: **zero bytes, zero functions,
zero objects**. The source diff for `source/math/real_math.h` is empty after
restoration; any remaining Git modified marker is line-ending/index-only.
