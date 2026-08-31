# AI Communication: authenticated Claude small trio

Date: 2026-08-31

Unit: `source/ai/ai_communication`

Source donor: Claude campaign commit
`e02971dfdba111d5481c5b7d7b6fc38a9e90d155`

## Result

Three previously unwritten functions were manually ported from the donor and
re-gated in the current canonical translation unit.  The large donor commit
was not cherry-picked.

| January function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_actor_communication_team` | 64 | 3 | `e381bc05013c03f5e5b56f8cc1dbeba503ed14c30c28872daebeb67c00afbda6` |
| `_ai_conversation_status` | 272 | 6 | `39f1ffa6d82dc98ef2bf97366e49ccbe328b33b9cf789d9f949406aa616af9bc` |
| `_ai_conversation_new` | 304 | 17 | `5a0b98904215778ab0c49562dad691bd01ca3ace2fbd634cb0d10a8cfe547042` |

The strict gain is 640 padded bytes and three functions.  The unit moves from
33 exact / 0 residual / 15 unwritten to 36 exact / 0 residual / 12 unwritten.

## Name, linkage, and source provenance

Claude's temporary source called the third function `code_00031d50`.  That
address label was not admitted.  January's exact symbol atlas maps
`0x431D50` directly to `_ai_conversation_new` in
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`.
The same atlas maps the other two names, and January's PDB independently
authenticates `actor_communication_team` and `ai_conversation_status`.

January COFF gives all three symbols storage class 2 and type `0x20`, so all
three retain external linkage.  Their declarations now live in the owner
header `source/ai/ai_communication.h`; the foreign consumer-local declaration
of `ai_conversation_status` was removed from `source/ai/ai_script.c`.

Three HCEA maps corroborate `ai_conversation_new`.  The corresponding HCEA
source reconstructions independently reproduce all three algorithms:

- `research-cache/halocea-full-blobs-20260830/src/blam/ai/actor_communication_team.c`
- `research-cache/halocea-full-blobs-20260830/src/blam/ai/ai_conversation_status.c`
- `research-cache/halocea-full-blobs-20260830/src/blam/ai/ai_conversation_new.c`

Pastudan independently corroborates the conversation-status topology.
Marathon has no applicable analogue for this cluster.

The authenticated actor race bit names come from HCEA's recovered
`actor_race` enum.  The full enum was placed in its semantic owner,
`source/ai/actor_types.h`; Claude's invented `_actor_race_*` aliases were not
copied.  The source uses `_race_human_bit` and `_race_covenant_bit` through
the shared `TEST_FLAG` macro.

## Semantic review

`actor_communication_team` maps human and Covenant race flags to the two
dialogue communication teams and returns `NONE` for every other race.

`ai_conversation_status` takes the highest live state for the requested
scenario conversation, then consults the newest matching recent-conversation
record when no live datum exists.

`ai_conversation_new` allocates a conversation datum and, for a scripted
request when the pool is full, evicts the selected existing datum before
initializing the replacement.  Its predicate
`scripted < overwrite_scripted || creation_time < overwrite_creation_time`
looks unusual, but the same behavior appears independently in the later HCEA
implementation.  It is retained as an original cross-version quirk rather
than "corrected" into more conventional selection logic.

No volatile access, forced inline/noinline attribute, fake dependency,
barrier, dead branch, undefined-value carrier, inline assembly, or
address-named symbol remains in this packet.  The scoped fake-match scanner
reports zero review leads.

## Current-header and regression verification

Claude's archived exact source used an older transitive `actions.h`, so its
old object alone was not accepted as proof.  The packet was rebuilt after the
semantic rename and all current header edits.

- Fresh configured Ninja build: pass.
- Semantic audit: 470 units scanned, 5,048 functions evaluated, 4,941
  semantic exact, zero unit errors.
- `ai_communication` gate: 36 exact / 0 residual / 12 unwritten.
- Explicit trio gate: all three strict exact.
- `actor_types` gate: 13 exact / 0 residual / 0 unwritten.
- `action_converse`, `actions`, `ai_debug`, and `ai_script`: no status changes
  from their known residual sets.
- Strict board: 4,929 / 8,245 functions and 689,783 / 1,922,413 padded bytes.
- Parked ledger: 27 active / 0 stale / 0 invalid.
- Test suite: 222 passed.
- `git diff --check`: pass.

The actor-race header caused the expected actor-type includers to rebuild;
the full semantic sweep found no regression in any of them.
