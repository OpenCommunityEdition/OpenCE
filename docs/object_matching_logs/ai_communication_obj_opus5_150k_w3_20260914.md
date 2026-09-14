# `ai_communication.obj` — Opus5 150K house-clean lane, wave w3 (2026-09-14)

## Verdict

- Real file `source/ai/ai_communication.c` **unchanged** (`git diff --stat` empty). No row became strict
  EXACT in the real file; no park reopen is proposed.
- Every non-exact row is a LOCKED park with an exhausted shape budget, owner-blocked on the protected
  `units.c` (P1), or header-blocked and far from exact (P2).
- Fresh HEAD measurements strengthen two packets:
  - **P1** still closes `_ai_communication_find_actor_to_reply_to_player` (1104 padded) strict EXACT.
    The candidate is `scratch/workers/w3_ai_communication/p1.c` with the shadow header `shadow_p1/units.h`.
  - **P2** no longer has a side effect: every other row stays identical and every park has zero drift.
- The TU prototype cleanup changes no bytes in any of the 48 rows (below).
- Final real-file gate: `== exact 42 residual 6 unwritten 0`. This is identical to the baseline, the
  `_point_from_line3d` guard passed, all 4 parks have zero drift, the owner census matches `build/base`,
  and the fake scan found 0 leads.

## 1. Baseline (lane HEAD 210aa8f81)

`== exact 42 residual 6 unwritten 0`. The guard passed. This is identical to the cut-off run's baseline
at 71e4a5a77; the intervening commits touched only docs.

| Function | Target padded/relocs | Real file | Status |
| --- | --- | --- | --- |
| `_ai_communication_event` | 8064 / 336 | 7776 / 319 | zero-credit fuzzy residual |
| `_ai_communication_find_actor_to_reply_to_player` | 1104 / 65 | 1088 / 63 | owner-blocked (P1) |
| `_ai_communication_finished` | 1584 / 87 | 896 / 29 | PARKED |
| `_ai_communication_actor_talk_weight` | 912 / 24 | 912 / 24 | PARKED (sha) |
| `_ai_communication_get_player_rating` | 832 / 49 | 832 / 49 | PARKED (sha) |
| `_ai_communication_update_speech_timers` | 672 / 43 | 672 / 43 | PARKED (sha) |

`_ai_conversation_find_participant` (inherited from the 100K lane) and `_ai_conversation_begin` (w1 reopen)
are exact and were not touched.

## 2. Law detectors (scratch/w2/laws_w2.md) applied per row

| Row | Laws checked | Finding |
| --- | --- | --- |
| update_speech_timers | A7 (C4013 `error`), A5, A3, A10, declaration count | The `cseries/errors.h` include is byte-inert for the whole TU (e1/e2). The residual is a pure edi/esi/ebx web colouring: January {unit, priority reload, team_name} is edi, ours is esi/ebx. The team_name load position follows from that colouring. `team_name` alias folding is worse (w1 u3 / w3 u1). |
| actor_talk_weight | A10 (boolean initializers), A7 | 5 shapes in the cut-off run (tw1 short-return shadow as an ABI probe; tw2–tw5 declare+init/store forms). All stay `[sha]`, with siblings and parks unchanged. |
| get_player_rating | A22 dot canonicalisation / younger operand, A15, A11 | January emits k·k (value live from `fst`), then i·i, then j·j. A single-expression helper gives k,j,i (ours) and an unrolled accumulation gives i,j,k; no law yields k,i,j. The budget (5 shapes) was already spent. |
| finished | A3, A10, A8 | Under P1, January still puts unit_index in ebx and speech_priority in edi (`push edi; mov edi,ebx; mov bx,[team]` at each find_* call). Ours follows the callee's edi preference directly, so the friend and enemy find_global tails cross-jump: 1568/86 vs 1584/87. No source-visible lever; the budget was long exhausted. |
| find_actor_to_reply_to_player | A18 (value width) | January pushes `reply->animation_type` with `xor ecx,ecx; mov cx,[esi+6]`, deferred behind `push 0`. This is identical to the neighbouring short `vocalization_type`/`communication_priority` pushes, so the parameter is 16-bit (P1). |
| event | A6, A20, A1 | P2 prints account for 14 relocations. The remaining multiset gap is exactly the w1 inline, re-fetch and 2.0f items. Beyond those, 174 loose hunks remain after masking registers and slots: adjacent-store merging, slot ranking, block order and boolean materialisation. |

## 3. Measured candidates (all scratch, whole-TU gates)

| Candidate | Change | Result |
| --- | --- | --- |
| d1.c | remove the 6 verbatim duplicate TU-local prototypes (actor_is_fighting, unit_test_speech, unit_speak, actor_perception_find_sense_position, actor_audibility_at_point, actor_handle_communication) | all 48 rows byte-identical; parks drift 0 |
| d2.c | d1 + replace the TU-local externs with `ai_runtime.h`, `actor_iterators.h`, `game/game_allegiance.h` | all 48 rows byte-identical; parks drift 0 |
| d3.c | v0 + `#include "game/game_allegiance.h"` only | byte-identical. The 100K note "include flips update_speech_timers" is stale. |
| e1/e2.c | `#include "cseries/errors.h"` (two positions) | byte-identical (C4013 gone) |
| r1.c | real + 100K `fn_reply_v3` body | reply 1088/65 `[size]`. The only instruction deltas are the two animation pushes. |
| p1h.c | shadow units.h short + six TU `short animation_type` params, no reply body | reply still `[size, relocs]`; finished park 896→912 |
| **p1.c** | r1 + shadow units.h short + TU statics short | **reply strict EXACT 1104/65 `be2605ffc0e7fb58`**; 43 exact / 5 residual. The talk_weight/get_player_rating/update_speech_timers parks are unchanged; finished drifts 896/29→912/29 (the expected re-measure). |
| p1f.c | p1 + waveD n5 finished body | finished 1568/86 `8a4b85701ff33941` (== w1 f1p) |
| f5.c | real + n5 finished body | 1568/86 `1ff5d84d98e4e130` (== w1 f1) |
| p2.c | d2 + shadow game_allegiance.h prototype + two print_allegiance blocks | event 7776/319 → 7952/333 `eb5fd189e87f56e6`; every other row identical; parks drift 0 |
| tw0–tw5.c | talk_weight shapes (cut-off run) | `[sha]` |

## 4. Orchestrator proposals

1. **P1 (re-submitted, fresh evidence).**
   - `source/units/units.h`: change `long animation_impulse` to `short animation_impulse` in both
     `unit_test_animation_impulse` and `unit_start_animation_impulse`. Update the `units.c`
     definitions (PROTECTED) and their static helper to match; the 100K units_probe2 measured units.obj 189/189.
   - In `ai_communication.c`: change the six `long animation_type,` parameters (3 prototypes and 3
     definitions of find_specific/find_global/actor_talk_weight) to `short`, and land the reply body
     from `p1.c`.
   - Re-measure the finished park (912/29 with the current body, or 1568/86 with n5).
   - Unlocks: find_actor_to_reply_to_player 1104 here, plus `_actor_unit_control` 784 and
     `_actor_action_try_to_dive` 720 per w2/waveD.
   - Consumers: the 66 files that include `units.h`.
2. **P2 (re-submitted; the side-effect note is now void).**
   - `source/game/game_allegiance.h`: add
     `short game_allegiance_get_incidents(short our_team_index, short other_team_index, short *incident_threshold);`
     (the definition is at game_allegiance.c:264).
   - In the TU: include `game/game_allegiance.h`, drop its three duplicate prototypes, and add the
     two print_allegiance blocks (`p2.c`). The result is zero credit (7952/333) with no park or sibling change.
   - Consumers: actors.c, actor_stimulus.c, ai.c, ai_script.c, encounters.c, aim_assist.c, game.c,
     game_allegiance.c, game_statistics.c, motion_sensor.c, damage.c.
3. **TU cleanup (R8, byte-inert).**
   - Apply d2: remove the 6 duplicate prototypes and replace the 6 consumer-local externs with
     `ai_runtime.h`, `actor_iterators.h` and `game/game_allegiance.h`.
   - Add `#include "cseries/errors.h"` for the C4013 at line 3295.
   - `sound_scripted_dialog_is_playing` has no owner header (it is defined in sound_manager.c, which
     has no header), so its local prototype stays until a header packet exists.
   - Measured: all 48 rows identical and parks drift 0.
4. **symbols.json linkage (A1, zero bytes).**
   - Add `"static": true` to `_actor_reset_idle_vocalization_timer` (file_offset 205552) and
     `_ai_communication_update_speech_timers` (206128).
   - Evidence:
     - both take a register argument (eax), so they use a private ABI;
     - neither has a January PDB public;
     - neither symbol appears in any other split object.
   - Out-of-tree csplit (`scratch/workers/w3_ai_communication/cfg` → `csout`): only
     `ai_communication.obj` changes (storage class 2→3 for exactly those two symbols); the other 832
     split objects are byte-identical.

## 5. Do-not-repeat (additions)

- update_speech_timers: errors.h include placement, prototype-cleanup/declaration-count variants
  (d1/d2/d3), the team_name fold.
- talk_weight: boolean initializer forms (tw2–tw5) and the short-return shadow.
- event: P2 alone does not change any other row; do not re-probe include placement.

## 6. Reopen criteria

- find_actor_to_reply_to_player: units.c released, then P1 applied (`p1.c` is ready).
- finished: P1 applied, plus a natural reason for unit_index in ebx.
- get_player_rating: a natural spelling or real_math.h evidence that gives k,i,j.
- update_speech_timers / talk_weight: January local records or a natural donor.
- event: P2 applied, then authentic inline/re-fetch/2.0f spellings and a slot-ranking pass.
