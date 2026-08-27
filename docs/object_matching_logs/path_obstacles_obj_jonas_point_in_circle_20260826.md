# `path_obstacles.obj`: exact point-in-circle helper (2026-08-26)

## Outcome

`_point_in_circle` is newly strict-exact. The wave adds one exact function and
59 meaningful code bytes. `path_obstacles.obj` advances from 4/16 to 5/16
exact functions and from 256 to 315 matched meaningful code bytes. It remains
incomplete; no completion label, semantic exception, adjudication, data
credit, or comparator waiver was added.

Source commit:

- `1837a3b8027399e4a1f3c4b97d068c236571f74e`
  (`Match path obstacles circle test`).

## Evidence and frozen baseline

The clean translation unit was frozen at commit
`5e48816ad69fa4250a20765db3411831a6d56a6b` in:

- `build/audit/path_obstacles_point_in_circle_baseline_20260826.json`;
- manifest SHA-256
  `5aede30bc843c9deb9e9aa6e88d7ea08cf7c2b88c16fe7dcd9f2339bd40cfbba`.

The January target is 59 meaningful / 64 padded bytes, zero relocations, and
normalized SHA-256
`24b7e293ca06795a9a7e45447fb86f2278b4f57e0d7f23d983d05159e5027f5f`.
The canonical ABI and outer predicate are fixed by
`source/math/real_math.h`.

No current rebuilt object other than the admitted result emits a same-name
`_point_in_circle` donor. The nested `_distance_squared2d` owner does exist in
the January `source/ai/action_vehicle.obj` target:

- 48 padded bytes;
- zero relocations;
- normalized SHA-256
  `e12f5936181ec0b4f78951a003a60fa0a613f8f5345003e293f2b3a4a180c4f2`.

The read-only target/base census is
`build/audit/find_distance_squared2d_evidence_20260826.py`, SHA-256
`a350f2d7afd65bf8308bedf17bea2f702f56d6061d8107ff338a1f3bbd1e3074`.
It found no current rebuilt standalone distance donor, so no claim is made
that the inner helper can be transferred as an ordinary wrapper.

The genuinely new topology evidence is the immediately preceding exact
`_point_in_sphere` wave. Its canonical outer radius predicate preserved a
locally renamed nested distance helper exactly, while the earlier
action-alert record proves a standalone distance-only wrapper collapses to a
different shorter form. The 2D candidate therefore retained the real outer
predicate rather than reopening the rejected wrapper topology.

## Candidate discipline

`distance_squared2d` and `point_in_circle` were locally macro-renamed while
the header was parsed. The ordinary definition retains the canonical outer
comparison and calls the parsed inline clone:

```c
boolean point_in_circle(
	real_point2d const *point,
	real_point2d const *center,
	real radius)
{
	return distance_squared2d_inline(point, center) <= (radius * radius);
}
```

The previous path-obstacles and action-alert ledgers were read completely.
None had compiled this 2D outer body. Claude's near-complete, finish-hs, and
untried residual censuses list the helper only as `target_only`, with no
candidate, experiment, or fixed point. No Claude branch changes this source,
and the normal `/O2 /Oy- /DDEBUG /Dxbox` compiler provenance is unchanged.

Exactly one normal VC7 candidate compile was spent. It was strict-exact on
the first shot. The preserved candidate is:

- `build/audit/path_obstacles_point_in_circle_first_current_tu_20260826.obj`;
- raw SHA-256
  `351c77cc878b6032318c64165dd50cef1c1c65860fee470767aee4a3ec012722`.

Direct hardened comparison reports `all_equal: true` for:

- `_point_in_circle`;
- `_point_in_sphere`;
- `_project_point3d`;
- `_set_real_point2d`;
- `_obstacles_new`.

The frozen pre-admission gate reports `_point_in_circle` as the sole
`newly_exact` function and `changed_nonexact: []`. Other findings are the
expected additive symbol/section inventory. There was no spelling retry,
manual expansion, wrapper variant, object permutation, compiler option,
allowlist, adjudication, comparator exception, synthetic anchor, or
byte-forcing form.

## Committed replay

The clean committed manifest is:

- `build/audit/path_obstacles_point_in_circle_committed_20260826.json`;
- commit `1837a3b8027399e4a1f3c4b97d068c236571f74e`;
- SHA-256
  `6bcd97c13202a5918a98a731108d1e3024a76eb04834388ccad35f775b713679`.

The generated object path was resolved inside the worktree, that single file
was deleted, and one rebuild was run from committed source. The replayed raw
object SHA-256 is
`c2603ac43cb6d4a31ca498218e385327793517da64dd96ef3d2aac7988944603`.
The committed manifest reports `ok: true`, no failures or warnings,
`changed_nonexact: []`, and all five accepted functions as `still_exact`.
Direct hardened comparison remains `all_equal: true`.

Pinned source and report evidence:

- source before/after SHA-256:
  `042b769d417fbc148890acf1685a8216856fe04fd23eeff389ea15ab06a19349` /
  `679b2ba35b038c460b1065c49dbc9b0fece4c1151714e9fc811807042276611c`;
- source blob before/after:
  `af5f70ea6fd0bdcbb4711e0f3d3dce2ec7357a7b` /
  `f455b37979e19871ded6ab8ffcc9f49643897b08`;
- canonical report:
  `347e56cbc75b6b043d624743c04b64e94c7a6f9548bc323e599c12b3da65b60b`;
- semantic report:
  `707deb9c7abe9a3b91e8aae8d834dd9cb8185f59435847477aa534345a43e15b`.

Supporting audits remain canonical:

- admission SHA-256:
  `f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`;
- parked-frontier SHA-256:
  `cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.

## Campaign gates

After regeneration:

- canonical code: 509,100/2,198,102 bytes and 4,219/11,060 functions;
- complete files: 377/833;
- Halo category: 496,186/1,770,166 code bytes and 4,052/7,574 functions;
- Halo complete files: 275/468;
- semantic audit: 470 units and 4,327 functions evaluated, 4,184 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,245 accepted
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
