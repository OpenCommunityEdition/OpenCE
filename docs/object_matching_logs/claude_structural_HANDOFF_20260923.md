# Structural 40K lane handoff (2026-09-23)

- Worktree: `C:\halo-worktrees\claude-structural-20260923`
- Branch: `claude/structural-40k-20260923` (local only, **not pushed**)
- Base: `ea84554a` (latest `origin/jonas/exact-pilots`, includes the A/B/C/D
  reconciliation and D3D8 intake)
- Machine-readable record: `claude_structural_manifest_20260923.json`
- Owner decisions: `claude_structural_owner_packet_20260923.md`

## Result

Target was 40,000 new meaningful source-driven bytes (stretch 50,000). The
verified gain is **14,150**, well short. The remaining structural pool is
dominated by owner-held forms (see "Why the target was not reached").

| Category | Functions | Padded | Meaningful |
|---|---|---|---|
| **New source-driven strict exact** | **15** | **14,272** | **14,150** |
| Already-credited csplit relocation aliases (config only) | 8 | 6,816 | 6,752 |
| Zero-credit fuzzy source landed | 1 | 0 | 0 |
| Data / scorer-only changes | 0 | 0 | 0 |
| **Stable-verdict regressions** | **0** | | |

Measured against the frozen baseline:

| Measure | Before | After |
|---|---|---|
| Halo objdiff code ledger | 1,524,233 / 1,770,166 (7,388 fns) | 1,538,383 (7,403 fns), +14,150 |
| Stable exact rows | 7,537 | 7,560 |
| Parks (active / stale / invalid) | 124 / 0 / 0 | 109 / 0 / 0 |
| Admission audit (candidates / contradicted / rejected / revoked) | 42 / 0 / 10 / 0 | 48 / 0 / 10 / 0 |
| pytest | 1,152 passed / 5 skipped / 26 subtests | unchanged |

The alias rows were already scored by the semantic ledger, so they move
strict verdicts, not the objdiff ledger.

## Commits

| Batch | Commit | Content | Credit |
|---|---|---|---|
| b1 | `6eb9cb76` | aim projectile, obstacle avoidance, actor perception ×2, display_error, text-box focus, grenade HUD, HUD messaging (+2 relocs.json lines) | +8,459 meaningful |
| b2 | `275a0225` | 7 csplit neighbour-symbol aliases (relocs.json) | already credited |
| b3 | `cfa20176` | active camouflage, render particles, shadow begin (2 disclosed per-site rule-24 casts) | +4,101 |
| b4 | `1df575be` | `_render_debug_profile` alias | already credited |
| b5 | `c789bec0` | ai_debug render-actor scoping + real sine/cosine defect fix | zero (fuzzy) |
| b6 | `70e1a15b` | by-value `uncompress_int32_to_real_vector3d` (+ `players_debug_render` prototype), magazine reload, triangle buffer | +1,590 |
| docs | this commit | handoff, manifest, owner packet | 0 |

Every batch passed `scratch/lane/batch_gate.py`:
- full ninja
- stable diff with zero regressions
- parked validation, 0 stale / 0 invalid
- admission audit, 0 contradicted / 0 revoked
- per unit: fake-scan and punning-cast deltas, and the selected-provider link check (new surplus only)
- pytest and `git diff --check`

The final consolidated diff against the frozen baseline gains 23 rows /
21,088 padded bytes with 0 regressions. The 16 changed `.c` files have 0
fake-scan leads.

## Priority targets from the brief

| Target | Outcome |
|---|---|
| `_ai_debug_render_actor` (24,976) | Genuine lexical-scoping and buffer-type fixes (RTC + January slots). The frame, 114/114 slots and the 1,905-entry relocation multiset now match, with 6,472/6,662 instructions equal. It also fixes a real defect: the vertical sine and cosine arrays were swapped. Landed as zero-credit fuzzy (b5). The remaining ~43 width-normalised bytes are x87 operand staging and one tail merge, with no admissible lever. |
| `_ai_communication_event` (8,064) | Fuzzy, 97 → 74 differing regions. The first diagnostic block was re-derived, the enemy_status polarity fixed, and the incident constants restored. `game_allegiance_get_incidents` is already declared in `game_allegiance.h`. The body needs the `allegiance_incident_type` enum moved from game_allegiance.c into `game_allegiance.h` (12-includer sweep; not landed). The rest is an authentic uninitialised `play_type` read (held) plus a normalize2d that fails the provider link. |
| `_collision_debug_render` (4,192) | The three globals are proven: contribs.json module 140 owns `.bss` 4550336, not COMMON, and the /Od build shows three separate arrays. Exact against a scratch re-split, but only with NODUP-class helper emission plus a descriptively named `.bss` split, so it is held (owner packet §1). |
| `_update_alien_scout_physics` (2,464) | The 12-byte frame hypothesis is confirmed: a 2D copy and the hover `left` slot. The fix needs 6 new aggregate view casts plus an inline `_point_from_line3d`, so it is held (§1, §5). |
| Held Lane B bodies (limp noodle, infection, fighter; 7,920) | Still strict exact on this tree, but every one fails the new selected-provider link (NODUP providers), so they are held. |

## New laws and tools

1. **NODUP provider law.** Our build defines 47 shared helpers as ordinary
   functions in their provider objects (e.g. action_charge.c hand-writes
   `point_from_line3d`), so any new SELECT_ANY copy fails a VC7 link
   (`scratch/lane/comdat_selection.py`, `provider_link.py`). A shadow
   experiment showed that replacing action_charge's hand copy with the header
   inline keeps action_charge identical and makes emitters link-clean. That is
   owner decision §1.
2. **Scratch harvest.** Other lanes' `_gate_*.c` probes and finished
   candidates were re-gated against today's tree (`scratch/lane/harvest.py`).
   This produced the aim-projectile and obstacle-avoidance landings. It also
   exposed probes that only match with dummy `m8_probe` locals (rejected) and
   several held forms.
3. **Header count interplay.** A genuine prototype change can flip a distant
   name-count tie. The by-value geometry prototype flipped
   `_rasterizer_frame_statistics_draw` until the genuinely missing
   `players_debug_render` prototype was restored. Both are independently
   evidenced, and the combination was swept with zero regressions.
4. **Aggregate view-copy strip test.** For `x = *(real_vector2d *)&v3d`, the
   castless alternative is a field copy. action_charge's field copy is exact
   (cast byte-inert); actor_looking's is not (owner §5).

## Why the target was not reached

After three waves (64 worker runs, ~16.3M subagent tokens) and two harvest
passes, the open Halo rows went from 189 (253,864 B) to 166 (232,776 B).
The remaining rows fall into these groups:
- **Owner-held (strict exact variants exist):** provider/NODUP helper
  emission (§1), PDB-typed volatile (§2), realcmp placement (§3), authentic
  January bugs (§4), aggregate view copies (§5), and the §7 items (P7
  lightmap, the unreachable-default read, the squaring macro, a named
  condition boolean). That is **about 47K bytes of strict-exact-under-ruling
  rows**: Lane D's re-measured 17.2K plus this lane's ~29.7K.
- **Diagnosed backend ties** (x87 operand order, register or slot
  permutations, cross-jumps). Each has worker evidence that five admissible
  shapes do not move it.

Owner rulings, not more waves, are the lever for the next 20–40K.

## Where to look next

- Owner packet §1–§7, each with measured, ready variants and their file
  paths.
- Sound fuzzy leads not landed (listed in the manifest):
  - observer, 6 instructions left, needs semantic review
  - main_update_time, 5 rows left
  - flag_update, slot displacements only
  - ui_check_for_pause_game
  - ai_communication_event
- Shadow worktree `C:\halo-worktrees\claude-structural-shadow-20260923`
  (detached, clean) exists for header sweeps. It can be removed with
  `git worktree remove`.

## Reproduce

```
python scratch/lane/batch_gate.py check final scratch/lane-before.json <units...>
python -m tools.campaign.stable_verdicts diff scratch/lane-before.json scratch/lane-after-b6.json
python scratch/lane/provider_link.py <unit> <obj> --baseline=<pre-change obj>
```

`scratch/` is untracked. It holds the baseline snapshot, per-batch gate logs,
the worker directories and the tools above.
