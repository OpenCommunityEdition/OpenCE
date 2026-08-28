# `ai_communication.obj` public conversation wave

Date: 2026-08-28
Owner: Jonas / Codex
Status: partial object, strict gains admitted

## Scope and baseline

This wave is limited to `source/ai/ai_communication.c` and starts from the
published actor/notify/looking result at commit
`0399271f76735b62cfd6430a40f25acda6b066b0`: 20 exact, 0 residual, and 28
unwritten functions, covering 3,008 of 25,696 padded code bytes. The inherited
typed `.rdata`, `.data`, and `.bss` owners are frozen acceptance boundaries.

The admitted package contains only the three public conversation cleanup
functions. The private line-begin/line-perform/driver research remains in an
isolated worktree until every genuine static dependency is exact. No residual
private body, proof wrapper, emission anchor, ABI annotation, compiler
directive, shared-header change, or unrelated worktree edit is retained.

## Accepted results

Three functions became strict target-owner COFF matches:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_ai_conversation_finish` | 400 | 25 | `1f14f58d062862199eddcdc427609f2ef684c4d19247e93238458a1d92750ad5` |
| `_ai_conversation_actor_deleted` | 208 | 7 | `d98b8f190389358a4cf9427a3974e2336dfbba3d4a4cd5d908393ae1730c2097` |
| `_ai_conversation_unit_died` | 368 | 12 | `3c985dcce18e39946e0b636a57aa222506b7f51674897881953b00d167071e8e` |

Final strict code result: 23 exact, 0 residual, and 25 unwritten functions;
3,984 of 25,696 padded bytes. The net gain is three functions, 976 padded
bytes, and 44 relocations. Exact-set comparison reports no inherited loss and
exactly the three functions above as newly exact.

The rebuilt data owners remain independently strict exact:

| Owner section | Bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| main `.rdata` | 6,336 | 36 | `7a24cfa7b71aa91b354208ee7a7dd45663c0c37e1025af5f87fbe5835bd17dbc` |
| main `.data` | 276 | 69 | `d489001ff88ed206b56f44bd0789a8332510f33bf40a312a7b2e5d60c3fe3e03` |
| target-owned `.bss` | 16 | 0 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

## Source-shape findings

- `_ai_conversation_finish` needs two deliberately distinct datum-pointer
  materializations: the first remains live for participant cleanup while the
  second supplies the recent-conversation definition index. The recent slot
  is a short, the count update uses a long maximum temporary, and the
  participant bitmask is assigned at the head of each loop iteration.
- `_ai_conversation_actor_deleted` uses the typed 0x64-byte conversation view,
  a short participant index, and the direct stop-on-death flag branch. The
  deletion path clears the participant bit and actor slot, then advances the
  active line when the deleted actor was its speaker.
- `_ai_conversation_unit_died` needs a function-scope short participant index,
  an explicit zero-count precheck followed by `do`/`while`, and the direct
  `unit_index` parameter throughout. This gives VC7 the January 0x18-byte
  frame, keeps the unit index in EDI, and spills the participant index at the
  target stack home.
- The typed conversation, recent-conversation, and scenario-definition views
  expose only corroborated offsets. Tag-block access remains through
  `TAG_BLOCK_GET_ELEMENT`, actor access remains through `actor_get`, and every
  function has an explicit terminal return.

## Audit artifacts

Generated objects remain local evidence and are not committed.

- Isolated clean public package:
  `build/audit/ai_communication_conversation_public_exact_20260828.obj`, raw
  SHA-256
  `EA39DAE8637288505B5BB1D141B522631C8025A359698AE95A597362530607A9`.
- Final integration on top of the published actor/notify/looking wave:
  `build/audit/ai_communication_conversation_public_integration_exact_20260828.obj`,
  raw SHA-256
  `21DA7D63AC3D2A534D73F964D09E8369687B75E20DDF76EDA6FFB6DBD19CDEDC`.

## Validation

- The strict whole-unit gate reports 23 exact, 0 residual, and 25 unwritten
  functions. Hardened direct comparison reports all three additions equal.
- Exact-set comparison reports 20 inherited exact functions retained, no
  losses, and exactly the three public conversation functions newly exact.
- Complete `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` targets pass.
- Semantic audit: 470 units, 4,624 functions evaluated, 4,493 semantic exact,
  4,520 accepted exact, and 0 unit errors.
- Object admission: 0 candidates, 0 contradicted, and 0 revoked.
- Parked audit: 13 active, 0 stale, and 0 invalid.
- Tooling tests: 205 passed.
- Strict campaign board: 271/619 objects, 4,474/8,246 functions, and
  602,231/1,922,669 padded bytes.
- `git diff --check` and the added-source prohibited-token audit are clean.

## Provenance and Claude/tooling doctrine applied

The source and evidence were reconciled against the active Claude and campaign
documentation before implementation:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halo/CLAUDE.md`
- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `tools/campaign/README.md`
- `docs/object_matching_logs/ai_communication_data_obj_codex_checkpoint.md`
- `docs/object_matching_logs/ai_communication_obj_jonas_code_wave.md`
- `docs/object_matching_logs/ai_communication_obj_jonas_lifecycle_wave2.md`
- `docs/object_matching_logs/ai_communication_obj_jonas_lifecycle_reply_wave3_20260828.md`
- `docs/object_matching_logs/ai_communication_obj_jonas_actor_notify_looking_wave4_20260828.md`

Authenticated semantic donors remain HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` and Stian Halo commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`. Claude-history commit
`f9782194dea35c5c71528acc29310e3a211d3c5b` remains evidence for the later
`ai_communication_get_player_rating(long unit_index)` correction; that shared
prototype change is intentionally outside this wave. Donors supply semantic
names and candidate topology, while the January Halo CE Xbox COFF remains the
only admission authority.

## Withheld research and next bounded cluster

The private conversation driver has measured exact bodies for
`_code_000322c0`, `_ai_conversation`, `_ai_conversation_update`, and
`_code_00031e80`, but it is not admissible until the genuine static
`_code_00032030` dependency is also strict. The speech research has an exact
`_code_000316e0` body under a genuine-shaped proof caller and a nearly exact
`_code_00032530`; neither is retained through a proof-only caller. Continue
these atomic caller clusters in isolation and publish only a zero-residual
production translation unit.
