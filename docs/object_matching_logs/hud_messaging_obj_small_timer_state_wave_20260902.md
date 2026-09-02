# `hud_messaging.obj` small timer/state wave (2026-09-02)

## Result

Starting from the verified queue-cluster packet at `45914cc3f`, this packet
recovers two more January functions and advances the focused object gate from
**26 exact / 0 residual / 10 unwritten** to **28 exact / 0 residual / 8
unwritten**.  The new strict matches contribute **269 meaningful / 288 padded
bytes**.

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_scripted_hud_set_timer_time` | `115 / 128` | `3` | `d2d65da4e3ebf60fece545a76b903e162cb46b127ff739eda3a31cb905cefb98` |
| `_hud_set_state_message` | `154 / 160` | `5` | `5632722593fc1dfb9e7c3043a3fae67b35258e5d984527679df23694a3947d07` |

All 26 inherited exact functions remain exact.

## Reconstruction and provenance

The timer function follows the independently preserved HCEA/Stian behavior:
convert minutes and seconds to 30 Hz ticks, enable and unpause the timer,
capture the current game time, and clamp its corner to the valid range.  The
ordinary typed source keeps a local timer owner across related accesses and
reloads it after the external time call, reproducing January's three ordered
relocations naturally.  This is the semantic owner/lifetime topology visible
in the donor and not a raw-address transcription.

The state-message function follows the HCEA message-table topology and the
January disassembly.  It suppresses state changes while help text is active,
uses the named HUD-message tag reference, obtains the typed definition through
`HUD_MESSAGE_TEXT_DEFINITION_GET`, and obtains an in-range message through
`TAG_BLOCK_GET_ELEMENT`.  An out-of-range request becomes `NONE` before the
single common validity assignment.  The expanded translation-unit-private
message definitions use semantic field names and preserve the January sizes;
they are deliberately not placed in a widely included header, avoiding VC7's
definition-position sensitivity.

The touched public timer declaration now lives in the owning
`interface/hud_messaging.h` and was removed from `hs.c`.  The source uses
project `real`, `word`, `boolean`, `NONE`, `TRUE`, `FALSE`, and `PIN` types and
constants.  It contains no address-named symbols, raw byte offsets, assembly,
forced inlining, volatile/barrier controls, fake dependencies, inert
expressions, or nonsensical branches.

An ordinary typed reconstruction of `_scripted_hud_get_timer_ticks` was also
examined.  It produced a credible 80-byte residual at a documented VC7
register/scheduling boundary, but it is not retained in this exact checkpoint;
the function remains unwritten pending a separately evidenced fuzzy park.
The authenticated `compare_messages` private body remains dead-stripped until
its real `hud_messaging_update` caller is reconstructed and is not credited.

## Validation

- `tools/campaign/gate.py source/interface/hud_messaging`: `28 exact / 0
  residual / 8 unwritten`.
- Header-consumer gates after moving the public declaration:
  - `hs.obj`: `445 exact / 3 residual / 0 unwritten`;
  - `hud.obj`: `19 / 3 / 0`;
  - `players.obj`: `54 / 15 / 1`;
  - `game_state.obj`: `24 / 0 / 0` (direct production-edge build and strict
    per-function comparison; the focused gate helper cannot parse Ninja's
    escaped space in this unit path).
- `tools/fake_match_scan.py --fail-on-findings` on all three changed source and
  header files: zero review leads.
- Focused comparison/regression/parking/scanner tests: `97 passed`.
- `git diff --check`: pass.
- A repository-wide `ninja` invocation stops before compilation because this
  isolated worktree has no `cachebeta.exe`; all affected production edges were
  therefore validated with the focused gate/direct compile checks above.

No parked entry, semantic exception, completion label, target-data change,
artificial private-function anchor, or large `hud_messaging_update`
reconstruction is part of this packet.
