# `path_obstacles.obj`: point setter and 2D rotation fixed point (2026-08-26)

## Outcome

One ordinary helper is newly strict-exact:

- `_set_real_point2d`.

The first frozen pair also measured `_rotate_vector2d`, but that sibling was
nonexact and was pruned without tuning. The retained wave adds one exact
function and 19 meaningful code bytes. `path_obstacles.obj` advances from
1/16 to 2/16 exact functions and from 21 to 40 matched meaningful code bytes.
It remains incomplete; no completion label, semantic exception,
adjudication, data credit, or comparator waiver was added.

Source commit:

- `e99443d4770b8acd07a42cdf65a25d455a025fb7`
  (`Match path obstacles point setter`).

## Frozen evidence

The untouched translation unit was frozen at commit
`691f1b025ae2e228e873c5c68a3feb2266d3b562` in:

- `build/audit/path_obstacles_direct_2d_pair_baseline_20260826.json`;
- manifest SHA-256
  `50de0f17b3f8a76adfd139439ad2c0ccb5b213440d353b2b1e918b89543d05ea`.

The two direct canonical targets are:

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_set_real_point2d` | 19 / 32 | 0 | `0e79e6de615e917fce520dffd169117cbd2822eae1e7ce0a1685fe3a780b25e6` |
| `_rotate_vector2d` | 42 / 48 | 0 | `9a9864975806d5697fb568f3ea1aadee7f934119b5ebcd0b4aad5edb307aec3a` |

The canonical bodies and ABIs are fixed by `source/math/real_math.h`.
The read-only census script
`build/audit/find_path_obstacles_helper_donors_20260826.py`, SHA-256
`eeea013ea722d0816f2444773e25abd5e2a8683c15b1765d2f8022d18b61a9af`,
found five exact same-name donors for `_set_real_point2d`:

- `source/math/real_math.obj`;
- `source/physics/breakable_surfaces.obj`;
- `source/physics/collision_bsp.obj`;
- `source/physics/collision_features.obj`;
- `source/structures/structure_lens_flares.obj`.

Direct hardened comparison against `real_math.obj` independently proves all
32 padded bytes and the empty relocation inventory. No current rebuilt object
emits `_rotate_vector2d`; its evidence was limited to the simple canonical
header body, target section, and zero-relocation ownership.

The same census found exact donors for `_project_point3d` and
`_point_in_sphere`, but those helpers were excluded before freezing the wave.
`_project_point3d` has ten relocations, two assertion paths, a projection-map
addend topology, and a nested point setter. `_point_in_sphere` nests
`distance_squared3d`. `_point_in_circle` likewise nests a distance helper and
has no current emitted donor. They require separate topology evidence and were
not bundled into this direct-body pair.

## Prior and Claude documentation audit

The existing local record
`docs/object_matching_logs/path_obstacles_obj_jonas_new_leaf_20260821.md` was
read in full. It covers only `_obstacles_new`; it contains no candidate or
fixed-point evidence for either helper in this wave.

The Claude residual censuses in the near-complete, finish-hs, and untried
worktrees all list every remaining `path_obstacles` owner only as
`target_only`. They record no candidate, residual measurement, source
experiment, or fixed point for `_set_real_point2d` or `_rotate_vector2d`.
No reachable Claude branch changes `source/ai/path_obstacles.c`.

The Claude copies of `docs/matching_methodology.md` and
`docs/shape_transfer_executor_strategy.md` are byte-identical to the current
files. Claude's flag-provenance census confirms the normal
`/O2 /Oy- /DDEBUG /Dxbox` profile and contradicts `/QIfist` as a whole-TU
alternative. No option or compiler change was considered.

## One-shot result and pruning

Both inline identifiers were locally macro-renamed across the include block,
then their canonical ordinary definitions were appended after the accepted
constructor in target order. Exactly one normal VC7 candidate compile was
spent for the frozen pair.

The preserved first combined candidate is:

- `build/audit/path_obstacles_direct_2d_pair_first_current_tu_20260826.obj`;
- raw SHA-256
  `867d31b16e7368f100ebc2d30622f5020ae2e47fe425111069616bbd4e8b1b71`.

Its results were:

- `_set_real_point2d`: strict exact on the first shot;
- `_rotate_vector2d`: equal 48-byte padded size and zero relocations, but
  candidate normalized SHA-256
  `58f40bb118ff928d858eaa95d3b90a4f661add7e86967e059d5a5bf5693e6310`
  versus January
  `9a9864975806d5697fb568f3ea1aadee7f934119b5ebcd0b4aad5edb307aec3a`.

The frozen gate correctly reported the setter as `newly_exact`, the inherited
constructor as `still_exact`, and the rotation helper in `changed_nonexact`.
The rotation macro and body were therefore removed immediately. No operand
reordering, expression spelling, declaration, cast, wrapper, compiler flag,
or layout variant was tried.

The pruning build was not a new source hypothesis: it only removed the
rejected later function and re-established the already measured setter-only
state. The preserved retained object is:

- `build/audit/path_obstacles_set_real_point2d_retained_current_tu_20260826.obj`;
- raw SHA-256
  `aa89609fd7330c6d4ff3bfeb75e322e807e0784804d28b9849e25cd826c75aba`.

Direct comparison then reports `all_equal: true` for `_set_real_point2d` and
`_obstacles_new`; the frozen gate reports exactly the setter as
`newly_exact`, the constructor as `still_exact`, and
`changed_nonexact: []`.

## Committed replay

The clean committed manifest is:

- `build/audit/path_obstacles_set_real_point2d_committed_20260826.json`;
- commit `e99443d4770b8acd07a42cdf65a25d455a025fb7`;
- SHA-256
  `d185a4b36fde93416294ffe9df4099cfed6d02e1059c3bf3c961edc94bd66ab1`.

The generated object path was resolved inside this worktree. That single file
was deleted and rebuilt once from committed source. The replayed raw object
SHA-256 is
`0d82469cc3f4583d7d3da792b349379651bfc1aedad02448fb26685facb1da76`.
The post-replay manifest reports `ok: true`, no failures or warnings,
`changed_nonexact: []`, and both accepted functions as `still_exact`. Direct
hardened comparison remains `all_equal: true`.

Pinned artifacts:

- January split object:
  `8d8a2106cce26a6c271939867ccb6088d9a132ac4f7ddf8cd7c749489e329bc3`;
- source before/after SHA-256:
  `b081af2f1ec53ebb00bcfd72d95f915802c1009207c8d5c514d1dfc7a7b62443` /
  `c0789964cd07eb0daf6b736eaf023267252cc475238ca4a49693621973199f9b`;
- source blob before/after:
  `849800446379dbc746721be28bde0b9e9df22c63` /
  `5e6a1885587998ebcebcaff0b6e6cf679dde3199`;
- canonical report:
  `f4385ad1531a411b05e6c98983f234c3df16a064ad85ed345fd8df27c8381ae3`;
- semantic report:
  `5b713064fd4e0620ef1f5e9041c35afe17b0e9f244745c196eee057a11b7d7c2`;
- VC7 compiler:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- hardened comparator:
  `411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`;
- regression gate:
  `d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`.

Supporting audits:

- admission SHA-256:
  `f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`;
- parked-frontier SHA-256:
  `cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.

## Canonical progress and gates

After regeneration:

- canonical code: 508,825/2,198,102 bytes and 4,216/11,060 functions;
- complete files: 377/833;
- Halo category: 495,911/1,770,166 code bytes and 4,049/7,574 functions;
- Halo complete files: 275/468;
- semantic audit: 470 units and 4,324 functions evaluated, 4,181 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,242 accepted
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

## House-rule and reopen audit

Every parameter is on its own line and the retained function ends with an
explicit value return. The source is C89 and contains no assembly, `register`,
`volatile`, barrier, pragma, intrinsic, attribute, raw address,
pointer/integer reconstruction, pointer or union pun, undefined behavior,
synthetic anchor, or byte-forcing form. It performs no tag or object access.

Do not retry the canonical `_rotate_vector2d` body. Reopen it only with new
evidence that explains the equal-size, zero-relocation instruction-order or
x87-stack difference—an authenticated ordinary definition, exact donor, or
grounded compiler-topology result—not with operand/statement spelling churn.
