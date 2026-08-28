# `ai_communication.obj` actor, notify, and looking wave

Date: 2026-08-28
Owner: Jonas / Codex
Status: partial object, strict gains admitted

## Scope and baseline

This wave was limited to `source/ai/ai_communication.c`. It starts from the
published lifecycle/direct-PC result at commit
`dc0de62fcadd6fe08b7f104d4ba742a970c2399c`: 14 exact, 0 residual, and 34
unwritten functions, covering 1,648 of 25,696 padded code bytes. The inherited
typed `.rdata`, `.data`, and `.bss` sections were frozen acceptance boundaries.

The work deliberately treated the two real private-call relationships as
atomic clusters:

- `actor_communication_update` -> `code_000322f0`
- `ai_communication_notify` -> `code_000324b0` -> the two looking helpers

No dead emission anchor, ABI annotation, compiler directive, or shared-header
change is retained. Units and Vehicles were not read or modified by this wave.

## Accepted results

Six functions became strict target-owner COFF matches:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_code_000318c0` | 176 | 6 | `dbe71297aed0a7bec087b4326a2e815f12b372f4868f1155903e98899bf28200` |
| `_code_00031970` | 80 | 2 | `cd1be95162edc276747fbdf178c3268b9db01d3c89446b09f270987f73f291ce` |
| `_code_000322f0` | 208 | 9 | `1905fa59615dbbc091a07cae6b108f8ab4aa612d902f438081223d278443756e` |
| `_actor_communication_update` | 240 | 9 | `4e6e4b9977451136d14ba3947b0981434d2536d2ad617fae9d18e673e5404314` |
| `_code_000324b0` | 128 | 4 | `2551a091aea86b3f1fb59369e49137f98c485274d2123691f70d9340e6886a5e` |
| `_ai_communication_notify` | 528 | 23 | `418686ae93416f524c7294cb09bd4672390c827f45a8e340dfd266c177d9cba1` |

Final strict code result: 20 exact, 0 residual, and 28 unwritten
functions; 3,008 of 25,696 padded bytes. The net gain is six functions,
1,360 padded bytes, and 53 relocations with no inherited exact-function loss.
The exact-set audit reports only generic `.text` bookkeeping as removed when
the new COMDAT sections appear; all 14 named inherited functions remain exact.

The rebuilt data owners remain independently strict exact:

| Owner section | Bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| main `.rdata` | 6,336 | 36 | `7a24cfa7b71aa91b354208ee7a7dd45663c0c37e1025af5f87fbe5835bd17dbc` |
| main `.data` | 276 | 69 | `d489001ff88ed206b56f44bd0789a8332510f33bf40a312a7b2e5d60c3fe3e03` |
| target-owned `.bss` | 16 | 0 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

## Source-shape findings

- `_code_000318c0` needs the shared `look_at_point` label. The typed direction
  local is not blanket-cleared; only the target-selected union member is
  written. `_code_00031970` likewise has no invented empty-direction helper or
  zero-fill.
- `_code_000322f0` needs distinct block-local minimum/maximum delay values and
  a real `real_seed_random_range` call in each combat branch. Sharing those
  locals preserved size and relocations but produced a four-stack-home
  scheduling residual.
- `_actor_communication_update` preserves the nested outer gates, the
  `timer > 0 && --timer == 0` topology, block-local initialized sound and
  vocalization values, one typed 0x30-byte speech-item clear, and packet
  initialization at the embedded `ai` member.
- `_code_000324b0` is exact only in its genuine static caller chain. Notify
  supplies its private `ECX`/`EAX`/stack entry shape; it in turn supplies the
  private call shapes for both looking helpers. The isolated public-ABI form
  was 144/4 versus the 128/4 target and was correctly rejected.
- `_ai_communication_notify` needs a single-case allegiance `switch`, an
  explicit parent-object-index temporary, the negated
  `distance_squared3d(...) > 900.0f` spelling, and the medium-volume assignment
  after speaker team/location preparation. These ordinary-C levers reproduce
  the target branch topology, argument schedule, and constant lifetime.

## Audit artifacts

Generated objects remain local evidence and are not committed.

- Look-pair baseline:
  `build/audit/ai_communication_look_pair_baseline_20260828.obj`, raw SHA-256
  `45B597F82F70204FC06DDD2C44F08082BF11E87C84D4D6FC2A624E9D4307229D`.
- Temporary anchored look proof:
  `build/audit/ai_communication_look_pair_exact_anchored_20260828.obj`, raw
  SHA-256
  `173316DAE014BABB09775C894B3B533BFB5D07BB991186F1C80CAE31086AE9C4`.
  The non-emitting proof anchor was removed; the real notify chain now emits
  both helpers.
- Actor first shot:
  `build/audit/ai_communication_actor_update_first_shot_20260828.obj`, raw
  SHA-256
  `C99B8B94D3843344FBAB08D80BEFEFFA0229DD8C5A5B550149573DFBBADB1070`.
- Exact actor/reset cluster:
  `build/audit/ai_communication_reset_actor_exact_cluster_20260828.obj`, raw
  SHA-256
  `971AFF09D6F1D766D5E72A667848FEE41EC83BB03307BF27F5F3A1E68B319970`.
- Notify baseline:
  `build/audit/ai_communication_notify_baseline_20260828.obj`, raw SHA-256
  `697B23F8AE69304755F59105FA46288928739D9103417F84C0E5D8A30F47AF04`.
- Notify first shot:
  `build/audit/ai_communication_notify_first_shot_20260828.obj`, raw SHA-256
  `79057CC6F7EF885608AAE5AF062006CBAA438CB05308FA6661F1DD4DF3AF0CCC`.
  The three private helpers were exact; notify retained a same-size,
  same-relocation structural residual.
- Final six-function integration:
  `build/audit/ai_communication_actor_notify_looking_exact_20260828.obj`, raw
  SHA-256
  `B6DFD00F0AB6833CA9257400A94FF867073EFC31AD3E11F21DD60B9B7D704FFB`.

## Validation

- The strict unit gate reports 20 exact, 0 residual, and 28 unwritten
  functions. Hardened direct comparison reports all six additions equal.
- Complete `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` targets pass.
- Semantic audit: 470 units, 4,621 functions evaluated, 4,490 semantic exact,
  4,517 accepted exact, and 0 unit errors.
- Object admission: 0 candidates, 0 contradicted, and 0 revoked.
- Parked audit: 13 active, 0 stale, and 0 invalid.
- Tooling tests: 205 passed.
- Strict campaign board: 271/619 objects, 4,471/8,246 functions, and
  601,255/1,922,669 padded bytes.
- `git diff --check` and the added-line prohibited-token scan are clean.

## Provenance and Claude/tooling doctrine applied

The source and evidence were reconciled against all relevant active Claude and
campaign documentation before implementation:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halo/CLAUDE.md`
- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `tools/campaign/README.md`
- `docs/object_matching_logs/ai_communication_data_obj_codex_checkpoint.md`
- `docs/object_matching_logs/ai_communication_obj_jonas_code_wave.md`
- `docs/object_matching_logs/ai_communication_obj_jonas_lifecycle_wave2.md`
- `docs/object_matching_logs/ai_communication_obj_jonas_lifecycle_reply_wave3_20260828.md`

Authenticated semantic donors were HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` and Stian Halo commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`. Claude-history commit
`f9782194dea35c5c71528acc29310e3a211d3c5b` remains evidence for the later
`ai_communication_get_player_rating(long unit_index)` correction; that shared
prototype change was intentionally not mixed into this wave. HCEA supplies
semantic names and types, donor PC source supplies supporting topology, and
the Halo CE Xbox target COFF remains final authority.

## Do not retry without new evidence

The prior measured failures remain parked as source-shape hypotheses:
`_code_00031390`, `_code_00031400`, `_code_00031470`, `_code_00031660`,
`_code_000316a0`, `_actor_communication_team`, and
`_ai_conversation_status`.

Do not reintroduce a standalone external form of `_code_000324b0`, private ABI
annotations, or a dead helper-emission anchor. Do not hoist the two reset range
calls back into one shared call without new compiler evidence.

## Next bounded wave

The next highest-leverage donor-backed cluster is the 1,088-byte speech pair:

- `_code_000316e0` (416/23), normalized SHA-256
  `25863d0d4a8dfd0ea6e6a8ac6d2ca0f0f7db7ec5765802b6764ee1ad4eade286`
- `_code_00032530` (672/43), normalized SHA-256
  `91999fda75815c6638478824f3efa53ee6717572f365530362cbec00e78a50ac`

`_code_00032530` must retain the now-exact static reset helper in its real
caller cluster. The conversation core is the next independent 2,368-byte
donor-backed wave; the 48-byte private line-end helper must instead be worked
later with `ai_conversation_update`, not as a standalone leaf.
