# `actor_combat.obj`: safe vector-math pair (2026-08-26)

## Outcome

Two ordinary vector-math functions are newly strict-exact:

- `_cross_product2d`;
- `_add_vectors3d`.

The wave adds two exact functions and 61 meaningful code bytes.
`actor_combat.obj` advances from 9/34 to 11/34 exact functions and from 388
to 449 matched meaningful code bytes. It remains incomplete; no completion
label, semantic exception, adjudication, data credit, or comparator waiver was
added.

Source commit:

- `ce4deae9351850e081358a89e06cecf5e7410646`
  (`Match actor combat vector math wrappers`).

## Frozen evidence

The untouched translation unit was frozen at commit
`6561917c6702d8c713da89fa43fe0cc0f016e04e` in:

- `build/audit/actor_combat_safe_math_pair_baseline_20260826.json`;
- manifest SHA-256
  `a66b3dfa373c004b0895cfe879a0a1031a3a60d1a8b0e45a83357ab54935a68b`.

The target functions have these authenticated January fingerprints:

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_cross_product2d` | 23 / 32 | 0 | `eb266b2f8e53f8f6b73e6f1a45dded79e2aa4e6876daad3cad14df884b469e3e` |
| `_add_vectors3d` | 38 / 48 | 0 | `0290de9b10b36dc72d220008d9db37d21620115f1018e725f81d8875c95b1cd8` |

The canonical bodies and ABIs are fixed by `source/math/real_math.h`. Current
rebuilt objects independently provide exact same-name donors:

- `_cross_product2d`: `geometry.obj`, `real_math.obj`,
  `collision_bsp.obj`, and `collision_features.obj`;
- `_add_vectors3d`: `device_machines.obj`, `real_math.obj`,
  `collisions.obj`, `collision_features.obj`, and `units.obj`.

Direct comparison against both `real_math.obj` and `collision_features.obj`
proves all 32 and 48 padded bytes. Both functions have zero relocations, so
there is no unresolved callee, owner, or addend hypothesis. This is a
same-name, same-ABI transfer rather than a masked-shape guess.

## Claude documentation and tooling audit

All locally reachable Claude records that name this object or the applicable
matching tools were reconciled before candidate selection.

- The Claude residual census at
  `C:/halo-worktrees/claude-near-complete-objects-20260816/research/residual_frontier.json`
  lists both functions only as January `target_only` sections. It records no
  candidate, residual measurement, source experiment, or fixed point for
  either function. Its SHA-256 is
  `db87717092d2e7c71160619e9df25f0b940f1762725c81f8bd2d6ece77fbde0d`.
- No reachable `claude/*` branch changes `source/ai/actor_combat.c`; the
  current actor-combat history consists of the existing Jonas/Codex leaf,
  residual, and trigonometry waves.
- The Claude copies of `docs/matching_methodology.md` and
  `docs/shape_transfer_executor_strategy.md` are byte-identical across the
  near-complete, finish-hs, and untried worktrees and to the current files.
  Their SHA-256 values are respectively
  `3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`
  and
  `072ee7b2b3a2900e401bec7f69bff475f57639456d9c6270d2ea51189aaefa83`.
- Claude's `docs/house_rules.md`, SHA-256
  `9294b81dc2e1c92d75ffd2f9f289287131bdba3564ba95bb054b08189781bfe1`,
  warns that vector-add operand order is contextual. Here the January
  sections, canonical header lvalue topology, and multiple exact same-name
  donors all agree on the retained component expressions; no caller-context
  inference is used.
- Claude's flag-provenance census confirms this TU uses the normal
  `/O2 /Oy- /DDEBUG /Dxbox` profile and rejects `/QIfist` as a whole-TU
  alternative. No flag change was considered or used.
- The Claude campaign README identifies
  `tools/coff_compare.section_infos_equal` as the acceptance gate. The
  current hardened comparator and whole-TU regression manifest were used
  directly; neither the old in-memory edit harness nor an alias rename was
  used.

The same-name census also exposed `_valid_real`, `_valid_realcmp`, and
`_valid_real_normal3d`. They were excluded before freezing the candidate:
their canonical implementation reaches `valid_real`'s pointer-punning float
representation test, which violates the campaign's no-undefined-behavior and
no-punning rules. No candidate compile was spent on them.

## Candidate discipline

The two inline identifiers are locally macro-renamed across the include block,
then ordinary out-of-line definitions are appended after the already accepted
trigonometry wrappers. `actor_combat.c` has an empty private-code tail, so the
append preserves every existing function section index. The macros only avoid
duplicate C definitions; they do not alter optimization, calling convention,
or emitted instructions.

The one normal VC7 candidate compile was strict-exact for both functions on
the first attempt. There was no object permutation, compiler-flag change,
allowlist, adjudication, comparator exception, object patch, intrinsic,
alignment directive, synthetic anchor, or byte-forcing expression.

The preserved first candidate is:

- `build/audit/actor_combat_safe_math_pair_first_current_tu_20260826.obj`;
- raw SHA-256
  `09e63027a73e79ffd974357b06944fb9ad08cb2dc721b2addadc92a90a2e37f9`.

## Exact comparison and frozen gate

The hardened comparator reports `all_equal: true` for both additions and all
nine previously accepted functions:

- `_cross_product2d`;
- `_add_vectors3d`;
- `_sine`;
- `_cosine`;
- `_tangent`;
- `_actor_combat_fire_wildly`;
- `_actor_combat_disable_bursts`;
- `_actor_firing_blindly`;
- `_actor_combat_currently_firing_burst`;
- `_actor_get_weapon_definition`;
- `_actor_combat_get_firing_variant_definition`.

The frozen gate reports exactly the pair as `newly_exact`, all nine inherited
functions as `still_exact`, and `changed_nonexact: []`. Its sole hard finding
is the expected additive symbol inventory from defining the two functions.

## Clean committed state and replay

The clean committed manifest is:

- `build/audit/actor_combat_safe_math_pair_committed_20260826.json`;
- commit `ce4deae9351850e081358a89e06cecf5e7410646`;
- SHA-256
  `b6eca88f54554349208d52185f9e33c39404953611bf3f55ddc85a10f88b0c19`.

Its pre-rebuild and post-rebuild checks return `ok: true`, no failures, no
warnings, no changed nonexact functions, and all eleven functions above as
`still_exact`.

The generated object path was resolved inside this worktree. That single file
was deleted and rebuilt once from the committed source. Raw base-object hashes
were:

- before replay:
  `09e63027a73e79ffd974357b06944fb9ad08cb2dc721b2addadc92a90a2e37f9`;
- after replay:
  `8bfcd383fadf5074ac8768364e26200a0cc786dd8525a4b5ae257a060d402c09`.

VC7 debug/build records are nondeterministic, so raw object hashes are not the
equality oracle. The post-replay manifest is green and direct hardened
comparison remains `all_equal: true`.

Pinned artifacts:

- target split object:
  `ed7cf731b085a4a3422cdc5e87ecafb1033fce0a705666d03a3b2f81618423c6`;
- source file:
  `03598a18979ec6640fdffc8e53e382710e625b8072df026d41bdca5e5376fcb4`;
- source blob before/after:
  `53d4905fea21f46164367030e76e64b40db104d1` /
  `c600965feadfbd2a5c290f1a9c5edbb71b4630bd`;
- canonical report:
  `3fb2be08d6eb74d6c522c9ea038a65725a1347b0d603d7a2f5d374edb445d253`;
- semantic report:
  `7ab9a6345ca45ac99073fb7682e28937546cbce9fbf96c877e2efc13d8076362`;
- VC7 compiler:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- shape-atlas generator:
  `bdae62ac955b22fcb787b62ea34e4d841295a2290e4ae9170677870a1699d365`;
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

- canonical code: 508,730/2,198,102 bytes and 4,213/11,060 functions;
- complete files: 377/833;
- Halo category: 495,816/1,770,166 code bytes and 4,046/7,574 functions;
- Halo complete files: 275/468;
- semantic audit: 470 units and 4,321 functions evaluated, 4,178 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,239 accepted
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
