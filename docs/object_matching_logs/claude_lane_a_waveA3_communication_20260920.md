# Lane A wave A3 — the missing communication diagnostic graph

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, on top of wave A2 (`8110e554`).

**New strict bytes: 0. New strict functions: 0. Regressions: 0.**
Zero exact credit; this is the largest *structural* recovery in the lane.

## `_ai_communication_finished`

| | baseline | after |
|---|---:|---:|
| section size | 912 | **1,568** (January 1,584) |
| relocations | 29 | **86** (January 87) |
| instructions | 305 | **482** (January 484) |
| objdiff | 42.12% | **91.44%** |
| gap | 672 bytes, 58 relocations | **16 bytes, 1 relocation** |

The unit census is unchanged at `exact 44 residual 4 unwritten 0` — no sibling
moved in either direction.

### How the gap was found

A per-function string-literal relocation diff across **all 34 Lane A residuals**
(`scratch/orch/fn_string_diff.py`, keyed on the content-derived MSVC mangled name
so nothing is lost to name truncation) established three things at once:

1. **No Lane A unit emits a string absent from the whole January image** — a
   strong source-fidelity result for the whole portfolio.
2. **32 of 34 residuals have exactly matching string sets**, so their divergences
   are provably not missing diagnostics.
3. This function references **zero** string literals where January references
   **sixteen** — by far the largest purely structural gap in the 79,657-byte
   portfolio, and invisible to the objdiff percentage alone.

The full evidence packet, including the recovered texts and the January
disassembly windows, is
[claude_lane_a_communication_diagnostics_packet_20260920.md](claude_lane_a_communication_diagnostics_packet_20260920.md).

### What January's bytes supplied

- The sixteen exact texts, read out of January's `.rdata`.
- The frame: `sub esp, 0x650` against our old `sub esp, 0x50`, with the message
  buffer at `[ebp-0x450]`.
- The asymmetric build idiom, which is January's and not a mistake: the **first**
  write is plain `sprintf` into the local buffer; every later append is
  `csstrcat(buffer, csprintf(temporary, ...))`.
- The assert, complete: `push 1 / push 0x9e5 / push <file> / push "!reply_to_player"
  / call _display_assert / push -1 / call _system_exit / add esp,0x14`, written
  back as `match_assert` with January's file and the hex line literal `0x9E5`
  that is this file's own convention (every existing `match_assert` here uses hex).
- Every debug guard resolved to a **real named field** of the existing
  `struct ai_debug_state`, counted from its own
  `offsetof(struct ai_debug_state, print_speech) == 0x93` assert:

  | January operand | field |
  |---|---|
  | `_ai_debug:143` | `ai_debug.print_communication` (guards every append) |
  | `_ai_debug:132` | `ai_debug.communication_focus_enable` |
  | `_ai_debug:133` | `ai_debug.communication_random_disabled` |
  | `_ai_debug:100` | `ai_debug.communication_focus_vector`, via `BIT_VECTOR_TEST_FLAG` |

**No new type, header, name or declaration was required.** Every symbol the body
needs was already declared: `csstrcat`, `strupr` and `temporary` in `cseries.h`,
`game_connection` in `game.h`, `dialogue_get_vocalization_name` in
`dialogue_definitions.h`.

### Source review

Idioms were checked against the tree before admission rather than assumed:
`(short)(long)` float-to-short conversion appears in 9 files including this one at
:2158; the NaN-preserving `!(a < b)` form appears 33 times; this file uses
`if (ptr && ...)` 16 times and `ptr != NULL` zero times; `match_assert` line
literals in this file are hex. The landed body also replaces a bare `30.0f` with
`TICKS_PER_SECOND` at the delay conversion, per `docs/common_constants.md`.

Fake scan on the changed file: **0 findings**. Point guard: **PASS**.

## Rejected from the same worker's candidate

`_ai_communication_event`: writing the is-reply `player_rating` as the literal
`2.0f` instead of the named `communication_player_rating_low_priority` moves the
relocation count **319 → 318** against January's 336 and the instruction count
**2280 → 2278** against 2353 — structurally *away* on both counts, for a 2-region
cosmetic gain. The campaign-wide rejected-hypotheses file recorded this as "no such
spelling probe was compiled"; it is now compiled and **negative**. Not landed.

## Also landed: `ai_debug.c` boolean test spelling

`source/ai/ai_debug.c:2281`, `if (...flee.has_approach_point>0)` →
`if (...flee.has_approach_point)`. `has_approach_point` is a `boolean`
(`source/ai/actions.h:190`), so comparing it with `> 0` is both unidiomatic and
badly spaced. Measured: section size stays at January's exact 24,976, instruction
count stays 6,829, real differing regions **1,452 → 1,451**. It is the only one of
the four ai_debug candidates that is not a regression.

## `config/parked.json`

One entry re-baselined — `ai_communication:_ai_communication_finished` —
measurements refreshed and evidence extended with the mechanism and the new
16-byte / 1-relocation reopening condition. No park retired, none unrelated
touched. 184 active / 0 stale / 0 invalid.

## Verification

    ninja -j4 all_source progress semantic_progress
      -> Validated parked compiler ties: 184
      -> halobetacache 1,444,193 / 1,770,166 (7,292 / 7,574)   [unchanged]
    stable_verdicts diff vs frozen baseline: gained 1 / 224 bytes / regressions 0 (exit 0)
    accepted ledger: +1 function / +214 meaningful, regressions 0
    parks 184/0/0 | admission 19/0/8/0 | source/ai fake scan 2 (both inherited)
    point guard PASS | git diff --check clean
