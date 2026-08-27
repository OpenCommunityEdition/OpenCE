# `path_obstacles.obj`: exact point projection (2026-08-26)

## Outcome

`_project_point3d` is newly strict-exact. The wave adds one exact function and
143 meaningful code bytes. `path_obstacles.obj` advances from 2/16 to 3/16
exact functions and from 40 to 183 matched meaningful code bytes. It remains
incomplete; no completion label, semantic exception, adjudication, data
credit, comparator waiver, or compiler-control device was added.

Source commit:

- `61c79f46bc00941aa42d38f223357da9c12dac01`
  (`Match path obstacles point projection`).

## Frozen evidence

The untouched translation unit was frozen at commit
`715d4d7245ae46cd6f91a3e2f2c5ea305f5b65b7` in:

- `build/audit/path_obstacles_project_point3d_baseline_20260826.json`;
- manifest SHA-256
  `0eea3b94ff7daa344c32e3f5aa168d12dad6a382a91ef161ff39072a195d8e40`.

The January target is 143 meaningful / 144 padded bytes, with normalized
SHA-256
`32b0c09154fd8d60152a1e5f61f0d6fcb42b12ecd5dee1e13bf882586257d6e4`
and ten relocations. The complete relocation topology is:

- assertion path string at `0x1c`;
- projection-range expression string at `0x21`;
- `_display_assert` / `_system_exit` at `0x26` / `0x2d`;
- assertion path string at `0x4b`;
- sign expression string at `0x50`;
- `_display_assert` / `_system_exit` at `0x55` / `0x5c`;
- `_global_projection3d_mappings+2` at `0x72`;
- `_global_projection3d_mappings+0` at `0x79`.

The first two path-string relocations resolve to the same owned RDATA symbol.
The expression strings are exactly the tokens stringified by the canonical
`match_assert` calls:

- `projection>=_x && projection<=_z`;
- `~(sign&~1)`.

Spacing those expressions differently would change owned string bytes, so the
canonical token spelling was retained intentionally rather than treated as a
formatting variation.

## Donors and source provenance

The canonical direct body and ABI are fixed by
`source/math/real_math.h`. Three current same-name donors independently match
the target's entire padded body and every relocation identity, address, type,
destination, and addend:

- `source/physics/collision_bsp.obj`;
- `source/physics/collision_features.obj`;
- `source/structures/structure_lens_flares.obj`.

The donor census is recorded by
`build/audit/find_path_obstacles_helper_donors_20260826.py`, SHA-256
`eeea013ea722d0816f2444773e25abd5e2a8683c15b1765d2f8022d18b61a9af`.
`breakable_surfaces.obj` emits a same-name but nonidentical section and was not
used as evidence. The three exact donors agree with each other and with the
January action-obstacles owner, so the transfer is same-name, same-ABI, and
relocation-complete rather than a masked-shape inference.

The prior local records
`path_obstacles_obj_jonas_new_leaf_20260821.md` and
`path_obstacles_obj_jonas_point_setter_20260826.md` were read in full. The
first covers only the constructor. The second explicitly deferred this helper
because its assertion, mapping-addend, and nested-setter topology required an
independent evidence wave; it records no candidate compile for this body.

The Claude near-complete, finish-hs, and untried residual censuses list
`_project_point3d` only as `target_only`, with no candidate, experiment, or
fixed point. No reachable Claude branch changes `path_obstacles.c`. The Claude
methodology and shape-transfer documents are byte-identical to the current
copies, and the flag census confirms the normal
`/O2 /Oy- /DDEBUG /Dxbox` profile. No alternate compiler or option was used.

## One-shot candidate

`project_point3d` was locally macro-renamed across the header include. The
ordinary definition then copies the canonical body directly, including the
two exact assertion token sequences and the nested call to the already parsed
`set_real_point2d_inline` clone. This preserves the donor's nested inline
topology while giving the January owner its required ordinary symbol.

Exactly one normal VC7 candidate compile was spent. It was strict-exact on the
first shot. The preserved first candidate is:

- `build/audit/path_obstacles_project_point3d_first_current_tu_20260826.obj`;
- raw SHA-256
  `4bf12ff6ab7ab53230c78ef6486b91b0bd652960f8ec06b977b1ecdb4144350d`.

Hardened comparison reports `all_equal: true` for:

- `_project_point3d`;
- `_set_real_point2d`;
- `_obstacles_new`.

The frozen pre-admission gate reports `_project_point3d` as the sole
`newly_exact` function and `changed_nonexact: []`. Its hard findings are the
expected additive function/symbol ownership and the three newly emitted
assertion RDATA owners. Direct comparison proves the two inherited code bodies
remain exact; the clean committed manifest below freezes the new ownership.

There was no spelling retry, wrapper trial, object permutation, compiler flag
change, allowlist, adjudication, comparator exception, alignment directive,
intrinsic, synthetic anchor, or byte-forcing expression.

## Clean committed replay

The committed manifest is:

- `build/audit/path_obstacles_project_point3d_committed_20260826.json`;
- commit `61c79f46bc00941aa42d38f223357da9c12dac01`;
- SHA-256
  `879ce62c4c04246a2dea86e6408d78bd015466f9afcd748d301eeb69df9d0a58`.

The generated object path was resolved inside the authoritative worktree.
That single file was deleted and rebuilt once from committed source. The
replayed raw object SHA-256 is
`347a7369c2641900ff89db1e3f941d4a5299f253250ff99389bb16aba683035c`.
VC7 debug records are nondeterministic, so raw hashes are not the equality
oracle.

The post-replay committed manifest reports `ok: true`, no failures or
warnings, `changed_nonexact: []`, and all three functions above as
`still_exact`. Direct hardened comparison again reports `all_equal: true`,
including all ten relocations.

Pinned artifacts:

- January split object:
  `8d8a2106cce26a6c271939867ccb6088d9a132ac4f7ddf8cd7c749489e329bc3`;
- source before/after SHA-256:
  `c0789964cd07eb0daf6b736eaf023267252cc475238ca4a49693621973199f9b` /
  `6562040fb6279716ab7745114f0f43e8f827d2cd6ffa8ab95d39a5a2e62ae106`;
- source blob before/after:
  `5e6a1885587998ebcebcaff0b6e6cf679dde3199` /
  `bd704f729daea83a2a39b0dbdfd12e4ace396f57`;
- canonical report:
  `1f6dead689e6313ebb07c581ff5fb77ed3ca14ca1042a23c253d58f53c9dff98`;
- semantic report:
  `0acada77aad6a214d35c3be248855713dcfc9b030ebde7a221129b31e6dc440d`;
- VC7 compiler:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- hardened comparator:
  `411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`;
- regression gate:
  `d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`.

Supporting audits remain canonical:

- admission SHA-256:
  `f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`;
- parked-frontier SHA-256:
  `cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.

## Campaign gates

After report regeneration:

- canonical code: 508,968/2,198,102 bytes and 4,217/11,060 functions;
- complete files: 377/833;
- Halo category: 496,054/1,770,166 code bytes and 4,050/7,574 functions;
- Halo complete files: 275/468;
- semantic audit: 470 units and 4,325 functions evaluated, 4,182 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,243 accepted
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
synthetic anchor, or byte-forcing form. It performs no tag or object access,
so the typed-access rules are not implicated.
