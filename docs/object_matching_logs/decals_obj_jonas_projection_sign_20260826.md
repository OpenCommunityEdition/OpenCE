# `decals.obj`: exact projection-sign helper (2026-08-26)

## Outcome

`_projection_sign_from_vector3d` is newly strict-exact. The wave adds one
exact function and 85 meaningful code bytes. `source/effects/decals.obj`
advances from 5/33 to 6/33 exact functions and from 191/15,295 to
276/15,295 matched meaningful code bytes. It remains incomplete; no
completion label, semantic exception, adjudication, data credit, comparator
waiver, or parked entry was added.

Source commit:

- `5af65facfd5425ab112d6dc07ac282fdb5716abb`
  (`Match decals projection sign helper`).

## Frozen evidence

The clean translation unit was frozen at commit
`9487411e13749a665fd060d00b8f39e101d90ee2` in:

- `build/audit/decals_projection_sign_baseline_20260826.json`;
- manifest SHA-256
  `531e828faef9d0e9c2bd19532b080ad44b798f399060b422be8d3c2e6886fe96`.

The January target is 85 meaningful / 96 padded bytes, with normalized
SHA-256
`f77ab71bc5d794bb0e656fe362726c2b7531f9f4e751550acdd68e359216a5a4`
and five relocations:

- assertion path string at `0x1b`;
- projection-range expression string at `0x20`;
- `_display_assert` / `_system_exit` at `0x25` / `0x2c`;
- `__real@00000000` at `0x3f`.

The canonical ABI, assertion line 848, range predicate, and indexed sign test
are fixed by `source/math/real_math.h`. The tree-wide donor census in
`build/audit/find_decals_helper_donors_20260826.py` proves two independent
same-name exact donors:

- `source/physics/breakable_surfaces.obj`;
- `source/physics/collision_features.obj`.

Both donors match the target's complete padded bytes, normalized hash, and
all relocation identities, addresses, types, destinations, and addends.
Claude's `near-complete`, `finish-hs`, and `untried` residual censuses list
this helper only as target-only and contain no candidate or experiment. The
current matching methodology, house rules, preceding decals ledger, current
source, and normal `/O2 /Oy- /DDEBUG /Dxbox` compiler provenance were read
before the candidate.

## Candidate and evidence-directed correction

`projection_sign_from_vector3d` was locally macro-renamed while
`math/real_math.h` was parsed. The ordinary definition retains the canonical
range assertion and returns the indexed component's comparison with zero.

The first emitted candidate had the exact 96-byte normalized code hash and
all five relocation addresses and types, but it was not strict-exact. The
source had inserted spaces around `>=` and `<=`, so `match_assert` owned the
wrong string symbol:

- candidate: `projection >= _x && projection <= _z`;
- January: `projection>=_x && projection<=_z`.

That candidate is preserved as:

- `build/audit/decals_projection_sign_first_current_tu_20260826.obj`;
- raw SHA-256
  `15628182a998d92a583a4f78cec2d69c8a6c6ef721b4df2e878789e54e9878d2`.

The hardened comparison supplied genuinely new, relocation-specific
evidence, so one correction changed only the stringified assertion token
spacing. The corrected candidate is:

- `build/audit/decals_projection_sign_corrected_current_tu_20260826.obj`;
- raw SHA-256
  `00688c272048f41e4936393e464b7dbd1280c1b91bf98fd38a00a884ac2c79ae`.

It is strict-exact: 96/96 padded bytes, all five relocations, and the exact
normalized hash. Direct hardened comparison reports `all_equal: true` for
the new helper, both admitted plane helpers, `_decals_dispose`, and
`_decal_new_from_media_collision`.

The frozen pre-admission gate reports `_projection_sign_from_vector3d` as
the sole `newly_exact` function and `changed_nonexact: []`. Its hard findings
are the expected additive function/symbol inventory, three newly owned RDATA
symbols, and the associated context-evidence refresh for the two plane
helpers. Direct comparison proves those inherited code bodies did not change;
the clean committed manifest below freezes the new ownership.

No ungrounded body retry, wrapper trial, object permutation, compiler flag
change, allowlist, adjudication, comparator exception, alignment directive,
intrinsic, synthetic anchor, or byte-forcing expression was used.

## Committed replay

The committed manifest is:

- `build/audit/decals_projection_sign_committed_20260826.json`;
- commit `5af65facfd5425ab112d6dc07ac282fdb5716abb`;
- SHA-256
  `efbbbb43804809be336a41ba6434c01b57609f12322430f989cbe7aafc0060e0`.

The generated `build/base/source/effects/decals.obj` was resolved inside the
worktree, deleted by literal path, and rebuilt once from committed source.
The replayed raw object SHA-256 is
`fe73d267c59b9e1cb37919c0014c0d67fcc2398d66b917ab52e5a57ff36e03da`.
The committed manifest reports `ok: true`, no failures or warnings,
`changed_nonexact: []`, and all six accepted functions as `still_exact`.
Direct hardened comparison remains `all_equal: true`, including the exact
assertion-string relocation.

Pinned source and report evidence:

- source before/after SHA-256:
  `095bf9701c8da196e84b836b98da937312b4bd3bf91e8302d5261a18f9df426a` /
  `a0e5d94e3e99a6d7a0577bee5546af2ca089cf787df1f1ea9af6f4eb9277dd5a`;
- source blob before/after:
  `0a4e2e50f0fe8021695e493ff47de7d03d5d9ccc` /
  `774d1477c6fba42a3a4ecb91f9335c883f584f5d`;
- canonical report SHA-256:
  `f34dbf27e248b36615b71fc02104bbfda3014c97a7f1b2899d92a2f8e9983770`;
- semantic report SHA-256:
  `4abe4fd7e0edb994095af3f0aab9525b53f356482345b5972e79810a5e563ece`.

Supporting audits remain canonical:

- admission SHA-256:
  `f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`;
- parked-frontier SHA-256:
  `cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.

## Campaign gates

After regeneration:

- canonical code: 509,260/2,198,102 bytes and 4,222/11,060 functions;
- complete files: 377/833;
- Halo category: 496,346/1,770,166 code bytes and 4,055/7,574 functions;
- Halo complete files: 275/468;
- semantic audit: 470 units and 4,330 functions evaluated, 4,187 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,248 accepted
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
