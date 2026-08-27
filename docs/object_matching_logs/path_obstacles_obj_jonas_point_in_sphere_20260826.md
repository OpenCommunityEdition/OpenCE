# `path_obstacles.obj`: exact point-in-sphere helper (2026-08-26)

## Outcome

`_point_in_sphere` is newly strict-exact. The wave adds one exact function and
73 meaningful code bytes. `path_obstacles.obj` advances from 3/16 to 4/16
exact functions and from 183 to 256 matched meaningful code bytes. It remains
incomplete; no completion label, semantic exception, adjudication, data
credit, or comparator waiver was added.

Source commit:

- `626670091de82c0e3eb0f116c853befaf6bf1ce0`
  (`Match path obstacles sphere test`).

## Frozen evidence

The clean translation unit was frozen at commit
`162281efa9e3326b7b718b416bb060ebb16c94b3` in:

- `build/audit/path_obstacles_point_in_sphere_baseline_20260826.json`;
- manifest SHA-256
  `e439380d14ff297abe8502f873df6f33cdfabcd2f41638b65fea8e082263da9e`.

The January target is:

- 73 meaningful / 80 padded bytes;
- zero relocations;
- normalized SHA-256
  `8ae676f3f017b22262b85225640cbca7684ccde45ce18dffb00adec11238b92d`.

The canonical body and ABI are fixed by `source/math/real_math.h`. Four
current same-name donors independently compare `all_equal: true` against the
target:

- `source/objects/objects.obj`;
- `source/physics/breakable_surfaces.obj`;
- `source/physics/collisions.obj`;
- `source/physics/physics.obj`.

The donor census is recorded by
`build/audit/find_path_obstacles_helper_donors_20260826.py`, SHA-256
`eeea013ea722d0816f2444773e25abd5e2a8683c15b1765d2f8022d18b61a9af`.
Direct hardened comparisons against `objects.obj` and `collisions.obj`
independently prove all 80 padded bytes and the empty relocation inventory.

## Nested-inline topology

The canonical function is an outer radius comparison around
`distance_squared3d`. The prior action-alert fixed-point record proves that an
ordinary wrapper which only returns a locally renamed
`distance_squared3d_inline` is not the January 64-byte standalone distance
COMDAT: VC7 collapses that wrapper to 48 bytes. That rejected topology was not
reopened here.

Instead, this wave preserves the real donor context. Both
`distance_squared3d` and `point_in_sphere` are locally macro-renamed while the
header is parsed, and the ordinary outer function directly retains the
canonical comparison:

```c
boolean point_in_sphere(
	real_point3d const *point,
	real_point3d const *center,
	real radius)
{
	return distance_squared3d_inline(point, center) <= (radius * radius);
}
```

This supplies the nested distance operation with the same outer comparison
and radius expression present in every exact donor. It is an ordinary direct
body, not a synthetic caller or standalone distance wrapper.

The previous path-obstacles ledgers were read completely. None compiled or
measured this helper. Claude's near-complete, finish-hs, and untried residual
censuses list it only as `target_only`; no Claude branch supplies a candidate
or changes `path_obstacles.c`. The shared methodology documents and normal
`/O2 /Oy- /DDEBUG /Dxbox` flag provenance remain unchanged.

## One-shot result

Exactly one normal VC7 candidate compile was spent. The function was
strict-exact on the first shot. The preserved candidate is:

- `build/audit/path_obstacles_point_in_sphere_first_current_tu_20260826.obj`;
- raw SHA-256
  `73b07df56432b520570e76152b30d693fe4557eccfd69c341202c1d17366e29f`.

Direct hardened comparison reports `all_equal: true` for:

- `_point_in_sphere`;
- `_project_point3d`;
- `_set_real_point2d`;
- `_obstacles_new`.

The frozen pre-admission gate reports `_point_in_sphere` as the sole
`newly_exact` function and `changed_nonexact: []`. Its remaining findings are
the expected additive symbol/section ownership. No spelling retry, wrapper
variant, hand expansion, object permutation, flag change, allowlist,
adjudication, comparator exception, synthetic anchor, or byte-forcing form
was used.

## Committed replay

The clean committed manifest is:

- `build/audit/path_obstacles_point_in_sphere_committed_20260826.json`;
- commit `626670091de82c0e3eb0f116c853befaf6bf1ce0`;
- SHA-256
  `66274f2b31807abab7bb49dcc0889ca830d0412ff9c3165ad02dcf91a35695ad`.

The generated object path was resolved inside the worktree, that single file
was deleted, and one rebuild was run from committed source. The replayed raw
object SHA-256 is
`58a572db2bc468184cce3c61d25e660e840d04e4c0e71d176465997cd687bf35`.
The committed manifest reports `ok: true`, no failures or warnings,
`changed_nonexact: []`, and all four accepted functions as `still_exact`.
Direct comparison remains `all_equal: true`.

Pinned source evidence:

- source before/after SHA-256:
  `6562040fb6279716ab7745114f0f43e8f827d2cd6ffa8ab95d39a5a2e62ae106` /
  `042b769d417fbc148890acf1685a8216856fe04fd23eeff389ea15ab06a19349`;
- source blob before/after:
  `bd704f729daea83a2a39b0dbdfd12e4ace396f57` /
  `af5f70ea6fd0bdcbb4711e0f3d3dce2ec7357a7b`;
- canonical report:
  `8525a84ff6b5655f6af4d7ed84a2153ad9f2d78dbf44df5a5aff0d4291cf188a`;
- semantic report:
  `f438fc65dea9439a0030507f01191731b187037b8303541547f7ad94d02c09b9`.

Supporting audits remain canonical:

- admission SHA-256:
  `f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`;
- parked-frontier SHA-256:
  `cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.

## Campaign gates

After regeneration:

- canonical code: 509,041/2,198,102 bytes and 4,218/11,060 functions;
- complete files: 377/833;
- Halo category: 496,127/1,770,166 code bytes and 4,051/7,574 functions;
- Halo complete files: 275/468;
- semantic audit: 470 units and 4,326 functions evaluated, 4,183 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,244 accepted
  exact, and zero unit errors.

Repository gates:

- object admission: zero candidates and zero revocations; only the inherited
  `shell_xbox` completion-label contradiction remains;
- parked validation: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass;
- Halo and libcmt dry graphs: no work;
- protected-path touches: zero;
- deleted tracked paths: zero;
- no push.

## House-rule audit

Every parameter is on its own line and the function ends with an explicit
value return. The retained source is C89. It contains no assembly, `register`,
`volatile`, barrier, pragma, intrinsic, attribute, raw address,
pointer/integer reconstruction, pointer or union pun, undefined behavior,
synthetic anchor, or byte-forcing form. It performs no tag or object access.
