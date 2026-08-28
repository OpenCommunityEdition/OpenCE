# `ai_communication.obj` lifecycle and direct-PC reply wave

Date: 2026-08-28
Owner: Jonas / Codex
Status: partial object, strict gains admitted

## Scope and baseline

This wave was limited to `source/ai/ai_communication.c`. It preserved the nine
strictly exact functions already present on `jonas/exact-pilots`, the exact typed
`.rdata`/`.data`, and the established C89/house-rule source policy.

The frozen parent candidate was
`build/audit/ai_communication_baseline_20260828.obj` with raw SHA-256
`5FA91DDC2BC03FC80CCFFA9AD63A610A7BDC51BA5E5377D7D154983FE25F9DE5`.
Its strict code result was 9 exact, 0 residual, and 39 unwritten functions,
covering 656 of 25,696 padded bytes.

## Accepted results

Five functions became strict target-owner COFF matches:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_ai_communication_initialize` | 336 | 29 | `2a0d1e6049ab773094d295952cf00425ee2d84452011404940c70599e566a638` |
| `_ai_communication_initialize_for_new_map` | 272 | 21 | `f3a382a8698bccce76a1415a6487ea6779dd518e2b5c1a18c36fccefab296ade` |
| `_code_000314c0` | 144 | 6 | `e35850961c850ce32f18cf6ab351e82454cd62d335c47126bd8392363b3162c8` |
| `_code_00031570` | 64 | 2 | `9ddca24f8cfe0a334cfed40e1c35152e5b191098f0503e167837c2e451cb1ded` |
| `_code_000315b0` | 176 | 10 | `f4ec1ebcacbca6a94912de54536009e935ec0792d563d3494eb2427d207dd3bf` |

Final strict code result: 14 exact, 0 residual, and 34 unwritten
functions; 1,648 of 25,696 padded bytes. The net gain is five functions and
992 padded bytes with no inherited exact-function regression.

The new 16-byte target-owned `.bss` is also strict exact: zero relocations,
flags `0xc0300080`, normalized SHA-256
`374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb`.
Its typed owners are the dialogue count at offset 0, dialogue-events pointer at
offset 4, reply count at offset 8, and reply-events pointer at offset 12.

The inherited main `.rdata` remained strict exact at 6,336 bytes and 36
relocations (SHA-256
`7a24cfa7b71aa91b354208ee7a7dd45663c0c37e1025af5f87fbe5835bd17dbc`).
The inherited `.data` remained strict exact at 276 bytes and 69 relocations
(SHA-256
`d489001ff88ed206b56f44bd0789a8332510f33bf40a312a7b2e5d60c3fe3e03`).

## Source-shape findings

- The exact initializer uses the two 16-bit event-count globals themselves as
  counters. VC7.1 scalar-replaces them into `CX`, reproducing the target's
  16-bit increments, loop-entry jumps, alignment, and downstream mapping-loop
  schedule. A local short counter compiled to the wrong 32-bit lifetime.
- The new-map initializer uses the authenticated status layout
  `last_time_spoken` at +0 and `disable_until_time` at +4, while assigning
  disable first to reproduce the target store order. It ends with
  `data_make_valid`, not the stale PC donor's `data_delete_all`.
- `_code_000314c0` requires a preinitialized result, predicate assignment, and
  one explicit terminal return to retain the target `BL` lifetime and three
  epilogues.
- `_code_000315b0` requires direct reads from both typed actor target fields;
  synthetic prop-index locals changed the target register schedule.

## First-shot evidence

- Lifecycle first shot:
  `build/audit/ai_communication_lifecycle_first_shot_20260828.obj`, raw
  SHA-256
  `667461618EE397E229F5BD7C29C0E9A00C1E9A90C4E32086E3D26C48488E3EF1`.
  Initialize was 320/29 residual; initialize-for-new-map was 240/19 residual.
- Direct-PC first shot:
  `build/audit/ai_communication_direct_pc_first_shot_20260828.obj`, raw
  SHA-256
  `FDB3A6937E2AED48BEB5A27AAAC292B4607A1C424419232377706A24FB573D35`.
  `_code_00031570` was exact immediately; `_code_000314c0` and
  `_code_000315b0` were residual.
- First all-five exact text:
  `build/audit/ai_communication_first_exact_text_20260828.obj`, raw SHA-256
  `C98E3D1EC2495491950922D91755674A50A6518B37523728D859297EB5542A06`.
- Final exact code and BSS:
  `build/audit/ai_communication_lifecycle_direct_pc_final_20260828.obj`, raw
  SHA-256
  `4FA7AF0988F544104AB86B516D2E2B58F817538C616E9BDADD29605ED8A401E4`.

These generated audit objects remain local evidence; their hashes and measured
results are recorded here instead of committing build output.

## Validation

- `halobetacache_build`, `libcmt_build`, `semantic_progress`, and `progress`
  completed successfully.
- The final whole-unit campaign gate reports 14 exact, 0 residual, and 34
  unwritten functions.
- Exact-set comparison reports no inherited exact-function loss.
- Semantic audit: 470 units, 4,615 functions evaluated, 4,484 semantic exact,
  4,511 accepted exact, and 0 unit errors.
- Object admission: 0 candidates, 0 contradicted, and 0 revoked.
- Parked audit: 13 active, 0 stale, and 0 invalid.
- Tooling tests: 205 passed.
- `git diff --check` and the added-line banned-token scan are clean.

## Provenance

- `CLAUDE.md` in the active Halo research tree
- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `tools/campaign/README.md`
- `docs/object_matching_logs/ai_communication_data_obj_codex_checkpoint.md`
- `docs/object_matching_logs/ai_communication_obj_jonas_code_wave.md`
- `docs/object_matching_logs/ai_communication_obj_jonas_lifecycle_wave2.md`
- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
- Stian Halo commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`
- Claude-history commit `f9782194dea35c5c71528acc29310e3a211d3c5b`
  for the later `ai_communication_get_player_rating(long unit_index)` type
  correction; that shared-header change was intentionally not mixed into this
  wave.

HCEA is the semantic/type authority, the Stian PC reconstruction is supporting
control-flow evidence, and the Halo CE Xbox target COFF is final authority.

## Do not retry without new evidence

The prior measured failures remain parked as source-shape hypotheses:
`_code_00031390`, `_code_00031400`, `_code_00031470`, `_code_00031660`,
`_code_000316a0`, `_actor_communication_team`, and
`_ai_conversation_status`.

## Next bounded wave

The next authenticated, genuinely untried actor/looking cluster is 832 padded
bytes and 30 relocations:

- `_code_000318c0` (176/6)
- `_code_00031970` (80/2)
- `_code_000322f0` (208/9)
- `_actor_communication_update` (240/9)
- `_code_000324b0` (128/4)

For `_code_00031970`, target evidence forbids an added
`direction_get_empty`/zero-fill. For `_actor_communication_update`, preserve the
single 0x30-byte typed item clear and call
`ai_communication_packet_new(&speech_item.ai)`.
