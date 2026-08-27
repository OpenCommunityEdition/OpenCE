# `decals.obj`: exact plane helper pair (2026-08-26)

## Outcome

`_plane3d_negate` and `_plane3d_distance_to_point` are newly strict-exact.
The wave adds two exact functions and 75 meaningful code bytes.
`source/effects/decals.obj` advances from 3/33 to 5/33 exact functions and
from 116/15,295 to 191/15,295 matched meaningful code bytes. It remains
incomplete; no completion label, semantic exception, adjudication, data
credit, comparator waiver, or parked entry was added.

Source commit:

- `fea3bd0b50592c9995e89c726d0b6bf0015c757d`
  (`Match decals plane helpers`).

## Frozen evidence

The clean translation unit was frozen at commit
`365497d923ebb932dd89c6039b69f47ed0393544` in:

- `build/audit/decals_plane_pair_baseline_20260826.json`;
- manifest SHA-256
  `b8f4433f1c339678caa3f0b51fd8bfd758964d5e5d2a19ffde12b4f69b038b29`.

The read-only donor census is:

- `build/audit/find_decals_helper_donors_20260826.py`;
- SHA-256
  `1c306e7aebf6e9f3409f3c6831bbb646b42027a04791cc3fd0238b47edfdbb2`.

It measured all eight nearby target-only real-math COMDATs against every
same-name rebuilt object. Only this pair had independently exact donors:

| function | January extent | relocations | normalized SHA-256 | exact donors |
|---|---:|---:|---|---|
| `_plane3d_negate` | 41 meaningful / 48 padded | 0 | `60392482a07c013f93b88ca13741bbde11eb04529befb7377d6704adbbfa5dc1` | `bsp3d.obj`, `collision_bsp.obj`, `collision_features.obj`, `units.obj` |
| `_plane3d_distance_to_point` | 34 meaningful / 48 padded | 0 | `084ae3079dcf600c296c5564f85d95268126224a8992aaf3eb314f60a259a852` | `breakable_surfaces.obj`, `bsp3d.obj`, `collision_features.obj`, `scenario.obj` |

The other six helpers were excluded before compilation. Their available
same-name donors were nonexact or absent: `_projection_from_vector3d`,
`_projection_sign_from_vector3d`, `_project_point2d`, `_triple_product3d`,
`_plane2d_from_points`, and `_plane3d_from_point_and_normal`.

Claude's `near-complete`, `finish-hs`, and `untried` branches were read before
the wave. Their `research/residual_frontier.json` files agree that the decals
math helpers were target-only; they contain no prior helper candidate or
tuning history. Their sole old decals residual was the disposal ownership
alias that the current semantic layer has since accepted. The current
matching methodology, house rules, prior decals history, current source, and
normal `/O2 /Oy- /DDEBUG /Dxbox` compiler provenance were also checked.

## Candidate discipline

Both public names were locally renamed while `math/real_math.h` was parsed,
then emitted as ordinary C definitions. `_plane3d_negate` retains the
canonical four field negations. `_plane3d_distance_to_point` spells out the
three named scalar products and plane-distance subtraction in the canonical
order; it does not use the header's record-pointer cast.

The first compiler invocation stopped during parsing because this formerly
minimal translation unit had not included the math type definitions. It
emitted no candidate object. Adding the canonical math header corrected only
that declaration issue and did not alter either frozen function body.
Exactly one normal code-producing VC7 candidate compile was then spent, and
both functions were strict-exact on that first emitted object.

The preserved first candidate is:

- `build/audit/decals_plane_pair_first_current_tu_20260826.obj`;
- raw SHA-256
  `a2edd2152eb62c5bf2162ae10fea7a8f01f983c7c2bca833a6e915ef270c0834`.

Direct hardened comparison reports `all_equal: true` for both new functions
and the two inherited functions that need no ownership adjudication:
`_decals_dispose` and `_decal_new_from_media_collision`. The frozen
pre-admission gate reports the pair as the only `newly_exact` functions and
`changed_nonexact: []`; its symbol-set finding is the expected additive
inventory. `_decals_dispose_from_old_map` remains accepted through the
existing resolved BSS-ownership equivalence.

No body retry, spelling sweep, hand expansion, object permutation, compiler
option, allowlist, new adjudication, comparator exception, synthetic anchor,
or byte-forcing form was used.

## Committed replay

The committed manifest is:

- `build/audit/decals_plane_pair_committed_20260826.json`;
- commit `fea3bd0b50592c9995e89c726d0b6bf0015c757d`;
- SHA-256
  `a53696cdfe960bf64da35c02d6a72b80c6b8c8bf26933286de25a41ed3d71cd8`.

The generated `build/base/source/effects/decals.obj` was resolved inside the
worktree, deleted by literal path, and rebuilt once from committed source.
The replayed raw object SHA-256 is
`9a562df6d323beb71ca2320b6ad7c2db569b86c71a3b943ef19406f20803d7fb`.
The committed manifest reports `ok: true`, no failures or warnings,
`changed_nonexact: []`, and all five accepted functions as `still_exact`.
Direct hardened comparison of the four raw-comparable functions remains
`all_equal: true`.

Pinned source evidence:

- source before/after SHA-256:
  `35b9cf4ad9fd0752f64e155a7d25b5f49607523a8f2840e9deb9549e14622af7` /
  `095bf9701c8da196e84b836b98da937312b4bd3bf91e8302d5261a18f9df426a`;
- source blob before/after:
  `fa34dc7daf4b4b1ae72032edef0f8538bd087f79` /
  `0a4e2e50f0fe8021695e493ff47de7d03d5d9ccc`;
- canonical report SHA-256:
  `63a273d43e86c0f33a668eea8943a8df48f58735dee507c5570f3da9cdf35560`;
- semantic report SHA-256:
  `7b027b69365b6aa88d364327e218e87d4dd3340f66c345321e751ad8bd1de593`.

Supporting audits remain canonical:

- admission SHA-256:
  `f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`;
- parked-frontier SHA-256:
  `cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.

## Campaign gates

After regeneration:

- canonical code: 509,175/2,198,102 bytes and 4,221/11,060 functions;
- complete files: 377/833;
- Halo category: 496,261/1,770,166 code bytes and 4,054/7,574 functions;
- Halo complete files: 275/468;
- semantic audit: 470 units and 4,329 functions evaluated, 4,186 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,247 accepted
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

Every parameter is on its own line and both functions end with explicit value
returns. The retained source is C89. It contains no assembly, `register`,
`volatile`, barrier, pragma, intrinsic, attribute, raw address,
pointer/integer reconstruction, pointer or union pun, undefined behavior,
synthetic anchor, or byte-forcing form. It performs no tag or object access.
