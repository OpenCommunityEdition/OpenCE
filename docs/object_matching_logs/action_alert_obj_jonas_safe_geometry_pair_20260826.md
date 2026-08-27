# `action_alert.obj`: safe geometry pair (2026-08-26)

## Outcome

Two ordinary geometry helpers are newly strict-exact:

- `_vector_from_points3d`;
- `_magnitude_squared3d`.

The wave adds two exact functions and 76 meaningful code bytes.
`action_alert.obj` advances from 3/12 to 5/12 exact functions and from 101
to 177 matched meaningful code bytes. It remains incomplete; no completion
label, semantic exception, adjudication, data credit, or comparator waiver was
added.

Source commit:

- `020449c3b3518fd6bbc1dfbe739ab3a15762befa`
  (`Match action alert geometry wrappers`).

## Frozen evidence

The untouched translation unit was frozen at commit
`fe2e8631136da957ce6fe286fde456a480cbc2ba` in:

- `build/audit/action_alert_safe_geometry_pair_baseline_20260826.json`;
- manifest SHA-256
  `769e1bda19900e12f7c3622d1e4f6632debc1ef84b5fcc6fe42ca72ce10c089d`.

The target functions have these authenticated January fingerprints:

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_vector_from_points3d` | 38 / 48 | 0 | `905f56dbe4597d2bacc201456ffc33464b166e0b6392dda24a1a3e109b637aa2` |
| `_magnitude_squared3d` | 38 / 48 | 0 | `0d04579096eeab3a1c45f87774e1eb20ce4856dbac219f624a855e8c9ae99725` |

The canonical bodies and ABIs are fixed by `source/math/real_math.h`.
Direct comparison against the exact same-name definitions in
`build/base/source/math/real_math.obj` proves all 48 padded bytes of each
function. The repository-wide same-name census also found 25 exact donors for
`_vector_from_points3d` and 33 for `_magnitude_squared3d`. Both target
sections have zero relocations, so there is no unresolved callee, owner, or
addend hypothesis. This is a same-name, same-ABI transfer rather than a
masked-shape guess.

`_distance_squared3d` was deliberately excluded. Its target section is 49
meaningful / 64 padded bytes with zero relocations and normalized SHA-256
`f42fa39f289bfa50174aa61e77b345b092df8329d46f43f88ca127227ad1e9c8`,
but its canonical source nests another helper. That makes definition order and
inline topology a separate evidence question; it was not bundled into this
independent two-function wave.

## Claude documentation and tooling audit

All locally reachable Claude records that name this object or the applicable
matching tools were reconciled before candidate selection.

- The Claude residual census at
  `C:/halo-worktrees/claude-near-complete-objects-20260816/research/residual_frontier.json`
  lists the three geometry functions only as January `target_only` sections.
  It records no candidate, residual measurement, source experiment, or fixed
  point for the selected pair. The file SHA-256 is
  `db87717092d2e7c71160619e9df25f0b940f1762725c81f8bd2d6ece77fbde0d`.
- No reachable `claude/*` branch changes `source/ai/action_alert.c`. The
  current history contains the prior Jonas/Codex prefix, control, and random
  range recovery only.
- The Claude copies of `docs/matching_methodology.md` and
  `docs/shape_transfer_executor_strategy.md` are byte-identical across the
  near-complete, finish-hs, and untried worktrees and to the current files.
  Their SHA-256 values are respectively
  `3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`
  and
  `072ee7b2b3a2900e401bec7f69bff475f57639456d9c6270d2ea51189aaefa83`.
- Claude's `docs/house_rules.md`, SHA-256
  `9294b81dc2e1c92d75ffd2f9f289287131bdba3564ba95bb054b08189781bfe1`,
  requires ordinary C, canonical vector expressions, and evidence before a
  compile. The retained component expressions are the canonical header forms
  and are corroborated by exact same-name donors.
- Claude's flag-provenance census confirms this TU uses the normal
  `/O2 /Oy- /DDEBUG /Dxbox` profile and contradicts `/QIfist` as a whole-TU
  alternative. No flag change was considered or used.
- The Claude campaign README identifies
  `tools/coff_compare.section_infos_equal` as the acceptance gate. The
  current hardened comparator and whole-TU regression manifest were used
  directly; no in-memory edit harness, alias adjudication, or comparator
  exception was used.

The earlier local record
`docs/object_matching_logs/action_alert_obj_jonas_prefix_recovery.md` was also
read in full. Its one-shot covered setup, control, the two flush helpers, and
update; only control survived. It did not compile or measure either helper in
this wave, so no fixed point was reopened.

## Candidate discipline

The two inline identifiers are locally macro-renamed across the include block,
then ordinary out-of-line definitions are appended after the already accepted
random-range wrapper. `action_alert.c` has an empty private-code tail, so the
append preserves every existing function section index. The macros only avoid
duplicate C definitions; they do not alter optimization, calling convention,
or emitted instructions.

The one normal VC7 candidate compile was strict-exact for both functions on
the first attempt. There was no object permutation, compiler-flag change,
allowlist, adjudication, comparator exception, object patch, intrinsic,
alignment directive, synthetic anchor, or byte-forcing expression.

The preserved first candidate is:

- `build/audit/action_alert_safe_geometry_pair_first_current_tu_20260826.obj`;
- raw SHA-256
  `76977841b7e1bac6c02bf3ca86ca56785c5d47a6f3d55bda7019847d270e0ad4`.

## Exact comparison and frozen gate

The hardened comparator reports `all_equal: true` for both additions and all
three previously accepted functions:

- `_vector_from_points3d`;
- `_magnitude_squared3d`;
- `_action_alert_begin`;
- `_action_alert_control`;
- `_real_random_range`.

The frozen gate reports exactly the pair as `newly_exact`, all three inherited
functions as `still_exact`, and `changed_nonexact: []`. Its sole hard finding
is the expected additive symbol inventory from defining the two functions.

## Clean committed state and replay

The clean committed manifest is:

- `build/audit/action_alert_safe_geometry_pair_committed_20260826.json`;
- commit `020449c3b3518fd6bbc1dfbe739ab3a15762befa`;
- SHA-256
  `940551915655b88e2affb548bb9e62d687ec109d672b4d02dde0a79cd85af98f`.

Its post-rebuild check returns `ok: true`, no failures, no warnings, no
changed nonexact functions, and all five functions above as `still_exact`.

The generated object path was resolved inside this worktree. That single file
was deleted and rebuilt once from the committed source. Raw base-object hashes
were:

- before replay:
  `76977841b7e1bac6c02bf3ca86ca56785c5d47a6f3d55bda7019847d270e0ad4`;
- after replay:
  `c7646f7851aa760a7de32c2b52f9b391137eae2ede49cb028cb74740e9810acb`.

VC7 debug/build records are nondeterministic, so raw object hashes are not the
equality oracle. The post-replay manifest is green and direct hardened
comparison remains `all_equal: true`.

Pinned artifacts:

- target split object:
  `bb7a70aae881cdf5672ae48c3b9c5b7d0cdcc9a30a90730672f04b71fa1cab5f`;
- source file before/after:
  `84e4f2c3591554c6d34dc0701173bbfb11fb137ff4053a571cf3a55041d252ad` /
  `0682c4b9fbf896d8434a1a8f6f78b355c2fabf20669601be3d25f64541fee93b`;
- source blob before/after:
  `01c9af4f179f4fc7e3792413c7a18cf3d7899c5a` /
  `8de8debb9ac7108acc7352c53e36e305f75d06f7`;
- canonical report:
  `932d130576c91043f603dec2712ad9afb5dc9bfc98a1d5a991af3ebf482ca4f6`;
- semantic report:
  `33c07f434de44fc536e8ba0ddbf98b3d4eb0b9056fe243e84dceed5ec6721bde`;
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

- canonical code: 508,806/2,198,102 bytes and 4,215/11,060 functions;
- complete files: 377/833;
- Halo category: 495,892/1,770,166 code bytes and 4,048/7,574 functions;
- Halo complete files: 275/468;
- semantic audit: 470 units and 4,323 functions evaluated, 4,180 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,241 accepted
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
`volatile`, barrier, pragma, explicit intrinsic, attribute, raw address,
pointer/integer reconstruction, pointer or union pun, undefined behavior,
synthetic anchor, or byte-forcing form. These functions perform no tag or
object access, so the typed-access rules are not implicated.
