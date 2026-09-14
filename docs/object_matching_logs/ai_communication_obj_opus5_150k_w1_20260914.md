# `ai_communication.obj` — Opus5 150K house-clean lane, wave w1 (2026-09-14)

## Verdict

- Real file `source/ai/ai_communication.c` **unchanged** (`git diff --stat` empty). Every non-exact row
  was either a LOCKED park, owner-blocked (P1), or header-blocked zero-credit fuzzy (P2).
- **Park-reopen proposal, strict EXACT in scratch:** `_ai_conversation_begin` 1744 padded /
  1735 meaningful / 54 relocations, normalized SHA-256
  `4ac91071ef1ee4f6e17df958f06ddfcb727ef821e855baceca2e77f32c94cb12` == target.
  Candidate `scratch/workers/ai_communication/park_reopen_begin.c` (= current real file + the begin body
  only; diff `park_reopen_begin.diff`). Whole-TU scratch gate `== exact 42 residual 6`, no sibling change.
- Final real-file gate: `== exact 41 residual 7 unwritten 0`, `_point_from_line3d` guard passed, all five
  parks equal `measurements.base`, owner census identical to `build/base`, fake scan 0 leads.

## 1. Baseline (lane HEAD 12f7375d4)

`== exact 41 residual 7 unwritten 0` (guard passed).

| Function | Target size/relocs | Real file | Status |
| --- | --- | --- | --- |
| `_ai_communication_event` | 8064 / 336 | 7776 / 319 | zero-credit fuzzy (wave A) |
| `_ai_communication_find_actor_to_reply_to_player` | 1104 / 65 | 1088 / 63 | owner-blocked (P1) |
| `_ai_communication_finished` | 1584 / 87 | 896 / 29 | PARKED |
| `_ai_conversation_begin` | 1744 / 54 | 1648 / 53 | PARKED |
| `_ai_communication_actor_talk_weight` | 912 / 24 | 912 / 24 | PARKED (sha) |
| `_ai_communication_get_player_rating` | 832 / 49 | 832 / 49 | PARKED (sha) |
| `_ai_communication_update_speech_timers` | 672 / 43 | 672 / 43 | PARKED (sha) |

`_ai_conversation_find_participant` is inherited exact from the 100K lane and was not touched.

## 2. `_ai_conversation_begin` — park reopen (new lever: one function-scope loop counter)

Starting point: waveD's b19 body (1744/54, 96.3% aligned), classified there as a frame-allocation-priority
tie (`found_alternate` at `[ebp-0x16]` vs January `[ebp-0x1d]`, mask `-0x20` vs `-0x24`, frame `0xb4` vs `0xb8`).

Evidence:

- Full slot census including decimal displacements: identical sets except the one byte; January's `-0x10`
  cell is the spilled loop counter of the alternate loop **and** of the final `can_begin` loop
  (`mov [ebp-0x10],eax` at +0x17e and +0x691), plus `nearest_player_distance`.
- January's own assertion literal `??_C@_0JB@OPDEEEOP` (145 bytes) reads
  `(conversation->dialogue_indices[index] >= 0) && (conversation->dialogue_indices[index] < MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT)`,
  so the final loop's counter is named `index`.

Measured shapes (from b1 = real file + b19 body):

| Shape | Result |
| --- | --- |
| rename `found_alternate` (4 names) | slots unchanged (names are not a tie-break) |
| s1 `nearest_prop_distance` hoisted to the visible block | unchanged |
| s3 `unit` pointer at the `can_begin` block | worse |
| s5 final loop reuses the function-scope counter (`participant_index`) | January slot census, sha equal, only the assert literal differs |
| s6 separate function-scope `short index` for the final loop | `[sha]` (must be one counter) |
| **s7 one function-scope `short index` for every loop** | **strict EXACT** |

The waveD premise ("no visible source trace") is refuted: the counter's scope and name were the trace.
Body review: semantics identical to the park body (the participant-missing / not-ready flags give the same
`can_begin`/`keep_trying` outcomes); the looking-at loop advances the iterator once before testing the found
flag (January's fourth `data_iterator_next` relocation), which is harmless; explicit returns, typed accessors,
`TEST_FLAG`/`SET_FLAG`, and `match_assert` with January file, line and text are all kept.

Checks: the other 41 EXACT rows are identical; point guard passed; the other four parks match base; owner census
has no new or lost owners; fake scan 0 leads.

## 3. Other outcomes

- `_ai_communication_get_player_rating` (not landed): the only delta is the x87 order at +0x9b (January k²,i²,j²;
  ours k²,j²,i²). New shapes: `distance_squared3d` after or before `vector_from_points3d` (848, worse) and
  `dot_product3d(&vector,&vector)` (identical). Together with waveD p1/p2 that is 5 shapes, so the budget is spent.
  The slot census is identical.
- `_ai_communication_update_speech_timers` (not landed): 40 register-role diffs (January unit edi /
  time esi / priority edi; ours esi / edi / bx). Scoping shapes: `communication_team` at function scope was
  identical; folding `team_name` was worse. This is consistent with the park's declaration-context history.
- `_ai_communication_actor_talk_weight` (skipped, exhausted): the push schedule of the perception-knowledge result
  plus the `cause|subject` load order. `actor_get_perception_knowledge` (EXACT) returns via `mov eax,3`, which gives
  no ABI evidence. No new lever.
- `_ai_communication_finished` (not landed; exactness also needs P1):
  - Real file + waveD n5 = 1568/86 (`1ff5d84d98e4e130`).
  - The TU-local short-animation probe drifts the talk_weight park, so the units.h/units.c P1 is required.
  - The root delta is unit_index in ebx (January) vs edi (ours). As a result our friend and enemy
    `find_global` push sequences are identical and cross-jump (86 vs 87 relocations).
  - Function-scope `speech_priority` / `speaker_actor` / `unit` were all byte-identical.
- `_ai_communication_find_actor_to_reply_to_player`: OWNER-BLOCKED on P1, because units.c is protected.
- `_ai_communication_event` (not landed, zero credit). A fresh relocation multiset gives the gap:
  - P2 allegiance prints: 13 relocations.
  - January inlines `actor_communication_team` at the protagonist team site.
  - January re-fetches the subject actor in the danger look case (assert 0x6B0).
  - January stores an immediate 2.0f for the reply `player_rating`.
  - The frame is 0x126c vs 0x1264.

## 4. Orchestrator proposals

1. **park-reopen** `_ai_conversation_begin`: unpark and land the body from
   `scratch/workers/ai_communication/park_reopen_begin.c`.
2. **P1**, re-submitted unchanged from `ai_communication_obj_opus5_100k_20260914.md`: short `animation_impulse`
   in units.h and units.c (protected), then short statics in this TU, then the `fn_reply_v3` body, then a
   re-measure of the finished park.
3. **P2**, re-submitted: `short game_allegiance_get_incidents(short our_team_index, short other_team_index, short *incident_threshold);`
   in `source/game/game_allegiance.h`, plus the include and the print_allegiance blocks in event. Consumers are
   listed in the worker notes. The include flips the update_speech_timers park's load pair.
4. **park-remeasure** (zero credit, unchanged from waveD): the finished body n5, 1568/86.

## 5. Do-not-repeat

- begin: do not try local renames, `nearest_prop_distance` scope, or `unit` block scope; the single `index`
  counter is the lever.
- get_player_rating: do not retry the `distance_squared3d` / `dot_product3d` magnitude spellings.
- finished: do not retry function-scope `speech_priority` / `speaker_actor` / `unit`.
- update_speech_timers: do not retry `communication_team` scope or the `team_name` fold.

## 6. Reopen criteria

- get_player_rating: a natural spelling giving k,i,j order.
- update_speech_timers / talk_weight: January local records or a natural donor.
- finished and find_actor_to_reply_to_player: P1 applied, then a natural reason for unit_index in ebx.
- event: P2 applied, then authentic spellings for the inline, re-fetch and immediate-rating sites.
