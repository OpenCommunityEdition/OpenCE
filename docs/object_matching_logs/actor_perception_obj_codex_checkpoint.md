# `actor_perception.obj` Codex checkpoint

This is a new Codex-owned checkpoint. It does not replace or modify any prior
Actor Perception or Claude-authored log.

## Scope and disposition

- Integration base: `188aca95` (`Recover dead camera helper sections`).
- Target unit: `source/ai/actor_perception`.
- Strongest source donors audited: `fbd85f9f`, `42358a66`, `8d137f80`, and
  `99a8c5e8`; the retained source blob descends from the ordinary-C 33/44
  closeout at `b2779184ab5f504c0495e4c681719d0dfd36559f`.
- The runtime reconstruction delta is `source/ai/actor_perception.c`. A later
  corrective pass changes one `actors.h` prototype from `void` to `boolean` to
  match the recovered ABI. Historical shared-layout edits to `actors.h`,
  `props.h`, `ai_profile.h`, `ai_debug.h`, and `real_math.h` were deliberately
  not imported. Required January layouts remain TU-private typed views and
  access wrappers.
- `config/config.json` remains `NonMatching`. No park entry is added.
- The frozen objects `vehicles`, `ai_debug`, `ai_script`, `actions`, and
  `units` have no source, config, or log delta from this lane. The sole shared
  prototype correction is proven byte-neutral for all five below.

## Strict gain

The authoritative base had 22/44 strict-exact functions, 5,039 meaningful
bytes, and 5,184 padded bytes. The checkpoint has 33/44 strict-exact
functions, 10,532 meaningful bytes, and 10,752 padded bytes. This is a net
gain of 11 functions, 5,493 meaningful bytes, and 5,568 padded bytes, with no
lost baseline-exact function.

| Newly exact function | Meaningful | Padded | Relocations |
|---|---:|---:|---:|
| `_actor_audibility_at_point` | 567 | 576 | 18 |
| `_actor_compute_prop_target_weight` | 928 | 928 | 21 |
| `_actor_perception_create_orphan_from_friend` | 772 | 784 | 49 |
| `_actor_visibility_at_point` | 864 | 864 | 32 |
| `_arctangent` | 13 | 16 | 0 |
| `_code_0001dc00` | 233 | 240 | 6 |
| `_code_0001f470` | 122 | 128 | 0 |
| `_code_0001f4f0` | 100 | 112 | 2 |
| `_code_00020210` | 484 | 496 | 21 |
| `_code_00020780` | 519 | 528 | 11 |
| `_prop_position_refresh` | 891 | 896 | 25 |

The fail-closed non-code boundary is 36/76 target sections and 813/2,002
target bytes strict-exact, comprising one DATA section and 35 RDATA sections.
The 40 target-only RDATA sections (1,189 bytes) are literals owned solely by
the nine omitted residual implementations. They remain absent rather than
justifying nonexact code or artificial data forcing. The candidate also has
27 candidate-only sections (376 bytes: three DATA, 23 RDATA, and one
DIRECTIVE); they receive zero target credit. The target/candidate symbol
inventories are 369/284 entries. The target object SHA-256 is
`19704943ab15e64d0bebdfa0688a5779f3efca899bf8631f1f529a995da379e4`;
the candidate object SHA-256 is
`9466b765928e76bf47e625ce1377be349aaa20f8c4d9e5a335b655b20a057da4`.

## Residual census

Nine independently nonexact bodies are fail-closed and absent from the
candidate. Their original target sections remain available to the split-link
path; no placeholder or candidate body receives credit.

| Absent function | Target padded | Target relocations |
|---|---:|---:|
| `_actor_situation_combat_status_update` | `0x140` | 7 |
| `_actor_situation_update` | `0x4F0` | 28 |
| `_actor_perception_friend_prop_is_attacking` | `0x1B0` | 16 |
| `_actor_perception_aiming_vector_test_blockage` | `0x190` | 10 |
| `_code_00020990` | `0x5C0` | 50 |
| `_actor_emotion_update` | `0x680` | 38 |
| `_prop_status_refresh` | `0xE60` | 91 |
| `_code_00023290` | `0x970` | 86 |
| `_actor_perception_update` | `0x1270` | 170 |

Two nonexact bodies must remain as natural callers so VC7 emits four exact
static functions. They are explicitly unaccepted and receive no admission,
park, or configuration credit.

| Necessary caller | Padded T/B | Relocs T/B | Candidate hash | Exact static callees preserved |
|---|---:|---:|---|---|
| `_code_0001f560` | `0x4F0/0x4F0` | `28/28` | `d5d6dc16f7ce3f4b6058ba7de1f6e19777132e760e2201c37a5c622f7452b699` | `_code_0001f470`, `_code_0001f4f0` |
| `_code_000228b0` | `0x6D0/0x6D0` | `46/46` | `15890e02c1357a89ee3aa8dfc5c90e4a82f1794bcf6e4506f58479623e51673f` | `_code_0001dc00`, `_code_00020780` |

The strict function states are therefore 33 `STRICT_EXACT`, two `NONEXACT`,
and nine `ABSENT_IN_BASE`. The historical donor already exhaustively measured
the ordinary-C topology, declaration, Boolean, scalar/vector, x87-expression,
and allocator families for the residuals. They should be reopened only with
new January source or layout evidence, an independently corroborated donor,
or a defined-C dependency control not represented by that prior matrix.

## House and verification audit

- The corrective implementation delta is ordinary typed C: no assembly,
  `volatile`, force-inline controls, optimizer pragmas, barriers, undefined
  aliasing, raw byte-offset access, or byte forcing.
- Parameters are vertically formatted and every function has an explicit
  return. Generic object access and swarm datum access are confined to typed
  TU-local wrappers; tag access uses subsystem definition wrappers.
- The shared-header localization preserves the 33/44 strict result from the
  historical donor while avoiding any frozen-object dependency blast radius.
- A forced XDK rebuild and repository-wide semantic report evaluate 3,739
  candidate functions across 470 units, find 3,602 semantic-exact and 3,666
  accepted functions, and report zero unit errors.
- Ordinary objdiff reports 27 ordinary exact functions, 8,182 matched code
  bytes, four matched data bytes, and 31.891361% fuzzy similarity. The strict
  oracle additionally proves two ordinary false negatives and four local
  static functions, producing the authoritative 33/44 count.
- Full `halobetacache_build`, `libcmt_build`, semantic, progress, admission,
  and parked-function gates pass. Admission finds no new completion candidate;
  the parked manifest remains three active, zero stale, and zero invalid, with
  no Actor Perception entry.
- The repository test suite passes 179/179. A forced clean post-commit
  same-path regression reports all 33 accepted functions still exact, no
  changed nonexact function, and no code, non-code, symbol, or environment
  failure.

## Corrective ABI and typed-access audit

The integrated checkpoint initially hid an incompatible declaration by
renaming `actor_perception_create_orphan_from_friend` around the `actors.h`
include. That workaround is removed. The January target returns its Boolean in
`AL` on both epilogues, the HCEA/PDB-derived implementation declares an integer
result and documents the same success/failure contract, and the only current
cross-TU caller ignores the result. The shared declaration and definition are
now consistently `boolean`.

Three exact functions no longer overlay an `actor_datum` or `prop_datum` with
an incompatible local structure type:

- `actor_compute_prop_target_weight` uses the typed shared datums plus
  TU-local semantic aliases for the January slots at `0x54` and `0x6E`;
- `actor_visibility_at_point` uses `actor_datum::input` directly, including
  the looking-vector basis at `0x18C`, `0x198`, and `0x1A4`;
- `actor_audibility_at_point` uses the typed January combat-status alias at
  `0x6A`.

All alias offsets are compile-time asserted. No repeated raw cast, raw byte
offset, union type punning, or declaration-hiding macro remains on these
paths. `code_0001f560` also ends in an explicit `return;`; its expected
nonexact candidate hash remains
`d5d6dc16f7ce3f4b6058ba7de1f6e19777132e760e2201c37a5c622f7452b699`.

The corrective forced rebuild preserves all 33 strict-exact functions. In
particular, the four affected accepted functions retain these hashes:

| Function | Normalized SHA-256 |
|---|---|
| `_actor_compute_prop_target_weight` | `cc39cbeb7ca214fd9f760fb91ae820e3f26cd3ab85d73a6eee5794932aa553ae` |
| `_actor_visibility_at_point` | `d1687fb1a1d95f86b95a22f973c938b4ec8e36a002d7301e27300c7219fc5e02` |
| `_actor_audibility_at_point` | `1e789d092b7979d3870590f7b48001bda265029b4266ef404fd3e2cf1a543beb` |
| `_actor_perception_create_orphan_from_friend` | `b115575eebdb5f0f670b05461b6b4b95f5967a37334257c9e984c9d1b3eee5a4` |

A same-path old-declaration/new-declaration A/B force-rebuilt the 26 unaffected
direct `actors.h` consumers plus the two additional frozen units. After
normalizing only the COFF timestamp field, all 28 objects are byte-identical;
this includes `vehicles`, `ai_debug`, `ai_script`, `actions`, and `units`.

The corrective full Halo and libcmt builds pass. The repository-wide semantic
report evaluates 3,763 functions across 470 units, finds 3,629 semantic-exact
and 3,693 accepted functions, and reports zero unit errors. Admission remains
zero candidates and zero revocations with only the established `shell_xbox`
contradiction; parks remain three active, zero stale, and zero invalid. All
179 tooling tests pass.
