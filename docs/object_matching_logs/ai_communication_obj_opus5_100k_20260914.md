# `ai_communication.obj` — Opus5 100K consolidated lane (2026-09-14)

## Verdict

- Strict exact: **40 / 48 before, 40 / 48 after** (no new exact credit).
- `_ai_communication_event` (structurally incomplete residual) replaced by a
  complete January-debug-build reconstruction, landed as **zero-credit fuzzy**:
  size 4624 -> 7776 (target 8064), relocations 111 -> 319 (target 336),
  aligned-equal instructions 19.1% -> 51.3%, relocation-multiset L1 distance
  241 -> 21.
- `_ai_communication_find_actor_to_reply_to_player` is **strict exact under
  ORCHESTRATOR-PROPOSAL P1** (animation-type short ABI; header + protected
  `units.c`); not landed because the change needs a header, a protected file
  and a park re-measure.
- Six parks untouched and drift-free.

## 1. Scope and provenance

- Unit `source/ai/ai_communication.c`, target `build/split/source/ai/ai_communication.obj`.
- VC7 13.00.9254 (XDK 3911), `/O2 /Oy- /DDEBUG /Dxbox` per `build.ninja`.
- Evidence: January COFF disassembly with resolved string literals
  (`scratch/workers/ai_communication/event_jan_r.txt`), January assertion
  anchors, Ghidra 12.1.2 split-object decompilation (hypothesis only), January
  map atlas, HCEA `ai_communication_event.c` /
  `ai_communication_find_actor_to_reply_to_player.c` (semantics only, later
  release build without diagnostics).

## 2. Validated baseline (lane HEAD)

`== exact 40  residual 8  unwritten 0` (guard `_point_from_line3d` passed).

| Function | Target size/relocs | Base size/relocs | Tag |
| --- | --- | --- | --- |
| `_ai_communication_event` | 8064 / 336 | 4624 / 111 | size, relocs, sha |
| `_ai_communication_find_actor_to_reply_to_player` | 1104 / 65 | 1088 / 63 | size, relocs, sha |
| `_ai_communication_finished` (parked) | 1584 / 87 | 896 / 29 | size, relocs, sha |
| `_ai_conversation_begin` (parked) | 1744 / 54 | 1648 / 53 | size, relocs, sha |
| `_ai_conversation_find_participant` (parked) | 2048 / 85 | 2032 / 85 | size, sha |
| `_ai_communication_actor_talk_weight` (parked) | 912 / 24 | 912 / 24 | sha |
| `_ai_communication_get_player_rating` (parked) | 832 / 49 | 832 / 49 | sha |
| `_ai_communication_update_speech_timers` (parked) | 672 / 43 | 672 / 43 | sha |

## 3. Accepted controls (landed)

`_ai_communication_event` final: 7776 / 319 / normalized
`091a23246d7eaa8018bb2fe5cd899d7774d9b3bd112262a831a0e4186e9885ab`
(target `7a16e8d71ff2a5b3...`). Source shapes and their evidence:

- Assertion strings name January's locals: `dialogue_index`,
  `communication_priority`, `communication_team`, `subject_actor`,
  `protagonist_actor`, `total_possibility_weight`, `selected_possibility`,
  `preselected_reply_actor_index`; identifiers
  `NUMBER_OF_AI_COMMUNICATION_TYPES/PRIORITIES`,
  `_ai_communication_priority_none`,
  `MAXIMUM_COMMUNICATION_POSSIBILITIES` (overflow error text). Line anchors
  0x34E, 0x4C3, 0x594 (NULL diagnostic default), 0x62B, 0x62E, 0x639, 0x6B0,
  0x6F2, 0x76E, 0x7A7. The TU-local candidate struct was renamed
  `ai_communication_possibility` with field `preselected_reply_actor_index`
  (used only by this function); three TU-local enum constants were added.
- Speech timer tables: per-team shout/talk/chatter ticks and seconds
  (`* (1.0f / TICKS_PER_SECOND)`), `speech_disabled_reason` cleared to NONE,
  priority-none slots forced disabled, priorities 1..5 evaluated. The minimum
  tolerance test reads the slot's not-yet-written delay: **BUG (original)**,
  proven by the store order at January +0x813 / +0x84D; commented in source.
- Debug-vector suppression (`communication_suppress_vector` sets
  `dialogue_index = NONE` on local games), ignore/focus vectors,
  `print_communication_player`.
- Separate requirements string (early filters) and main debug string (late
  rejections and candidates), "NONE" tail only when not every usage failed a
  requirement (or focus is on).
- Protagonist switch subject/cause/friend/enemy with an unreachable default
  (`match_vassert(..., FALSE, NULL)`; `_comm_protagonist_target` is not handled
  in January; the table has no such rows). Find-actor flags built with
  `SET_FLAG`; encounter search passes
  `DATUM_INDEX_TO_ABSOLUTE_INDEX(subject_encounter_index)` (January
  `and eax, 0xffff`).
- Reply path remaps `communication_priority` (not `speech_priority`) through
  `communication_player_speaking_priorities`; delays computed as separate play
  and notification delays, each plus the candidate delay.
- Enemy-status block order `!subject_actor` / `!subject_encounter` / encounter
  (January block layout).
- Speech item fields stored individually (January has no clear), reply path
  calls started, notify and finished.
- Alignment normalization written with `square_root` and `_real_epsilon`
  (January's `__real@3f1a36e2e0000000`); calling `normalize2d` emitted four
  candidate-only real_math COMDAT owners and was rejected.

Admission checks (final real-file object): all other 47 rows identical to
baseline; guard passed; all six parks equal `measurements.base`; owner census
adds only literal COMDATs January references undefined (`NONE`, `never`,
`player`, `__real@3d088889`, `__real@3f1a36e2e0000000`) and drops three
base-only owners; `tools/fake_match_scan.py` 0 leads; `CL /Zs /W3` warning
count unchanged (14); `git diff --check` clean.

## 4. Experiment matrix

| Candidate | Function | Result | Kept |
| --- | --- | --- | --- |
| reply v1 structural if-chain | find_actor_to_reply | 1104/65, 2 sites | no (not exact) |
| reply `disable_until_time > time` | find_actor_to_reply | cmp form, worse | no |
| reply `time < disable_until_time` | find_actor_to_reply | worse | no |
| reply v3 `long disable_time` local | find_actor_to_reply | timer site exact; animation movsx only | no |
| reply `word animation_type` field | find_actor_to_reply | movzx, not January | no |
| v4probe short animation ABI + units.h probe | find_actor_to_reply | **EXACT**, 41/48; finished park 896->912 | proposal P1 |
| units_probe (unit_test_animation_impulse short) | units.obj | 188/189 | evidence |
| units_probe2 (+ code_00198fd0 short) | units.obj | 189/189 | evidence |
| actions_probe (short prototype) | actions.obj | try_to_dive size 704->720 (=target) | evidence |
| include game_allegiance.h only | all | update_speech_timers park sha flips | no |
| event v6..v10 (see worker report) | event | 7744/320 -> 7776/319 | v10 form landed |
| allegiance_probe (P2 prints) | event | 7952/333, 55.1%; speech_timers park flips | proposal P2 |

## 5. Do-not-repeat

- Timer difference in find_actor_to_reply: `>`/`<` compare spellings are
  worse; the named local is the January shape.
- `word` struct field for animation_type (movzx) is not January; the ABI is a
  16-bit parameter.
- Adding `#include "game/game_allegiance.h"` alone flips the
  update_speech_timers esi/edi tie (declaration context); do not try include
  placement variants.
- `normalize2d` in ai_communication_event emits out-of-line COMDAT owners.

## 6. Residual classification

- find_actor_to_reply_to_player: ABI/header (animation parameter width across
  `units.h`, `units.c` private, and three TU-local private prototypes). Fact.
- ai_communication_event: (a) header/owner — print_allegiance blocks need
  `global_game_team_names` and an undeclared `game_allegiance_get_incidents`
  (13 relocations); (b) compiler auto-inlining of `actor_communication_team` and
  `ai_communication_look_secondary_at_unit` at this call site (inference from
  identical bodies inline in January); (c) danger-zone actor re-fetch; (d)
  immediate 2.0 player rating; (e) register/slot scheduling.

## 7. Reopen criteria

- P1 applied: splice `scratch/workers/ai_communication/fn_reply_v3.c` and the
  short prototypes; expect find_actor_to_reply_to_player strict exact and
  re-measure the finished park.
- P2 applied: add the allegiance prints
  (`scratch/workers/ai_communication/allegiance_probe.c`) and re-measure the
  update_speech_timers park.

## 8. Disposition

Object still partial (40/48). No new Matching credit. Event is an honest
zero-credit fuzzy body; the other residuals remain parked or proposal-blocked.
