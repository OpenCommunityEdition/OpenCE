# Lane A wave A5 — the `print_allegiance` incident report

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, on top of wave A4 (`e99467da`).

**New strict bytes: 0. New strict functions: 0. Regressions: 0.** Zero exact credit.

## `_ai_communication_event`

| | baseline | after |
|---|---:|---:|
| section size | 7,776 | **7,856** (January 8,064) |
| relocations | 319 | **328** (January 336) |
| size gap | 288 bytes short | **208 bytes short** |
| relocation gap | 17 short | **8 short** |

Unit census unchanged at `exact 44 residual 4 unwritten 0`.

The first of January's two `print_allegiance` debug blocks is restored:

    if (ai_debug.print_allegiance)
    {
        console_printf(
            FALSE,
            "incident between teams %s and %s: %s, %s",
            global_game_team_names[subject_team],
            global_game_team_names[cause_team],
            betrayal ? "betrayal" : "accident",
            observed ? "observed" : "unobserved");
    }

placed exactly where January's bytes put it — after the betrayal/hostility
resolution and before the `if (observed)` allegiance-incident call. The block was
identified by the same string-relocation audit that drove wave A3: January's
`_ai_communication_event` references seven strings we did not
(`"betrayal"`, `"accident"`, `"observed"`, `"unobserved"`, `"still holds"`, and
the two format strings), plus `_console_printf` twice, `_global_game_team_names`
twice, `_ai_debug+149` twice.

`ai_debug.print_allegiance` is a real named field of the existing
`struct ai_debug_state`. The change adds `#include "game/game_allegiance.h"` to
`ai_communication.c` — the **genuine owner header** of `global_game_team_names`
(`game_allegiance.h:44`). That is house rule 9 satisfied, not circumvented: no
prototype was placed in the consumer, and no header was edited.

## Still deferred

January's **second** `print_allegiance` block calls
`game_allegiance_get_incidents`, which is declared in **no header in the tree**.
Its genuine owner is `source/game/game_allegiance.h`. That half remains a
**deferred header prerequisite** and was not attempted; no consumer-local
prototype was written. It accounts for a further `_game_allegiance_get_incidents`
reference, `"still holds"`, the `"allegiance %s, %d incidents (threshold %d)"`
format, and the `NONE`-to-999 threshold substitution.

## Verification

    ninja -j4 all_source progress semantic_progress
      Validated parked compiler ties: 184
      halobetacache 1,444,193 / 1,770,166 (7,292 / 7,574)   [unchanged]
    stable_verdicts diff vs frozen baseline: gained 1 / 224 bytes / regressions 0
    accepted ledger: +1 function / +214 meaningful, regressions 0
    parks 184/0/0 | admission 19/0/8/0 | source/ai fake scan 2 (both inherited)
    fake scan on the changed file: 0 | git diff --check clean

`_ai_communication_event` is not parked, so `config/parked.json` is untouched by
this wave.
