# `hud_messaging.obj` renderer caller closure (2026-09-08)

## Result

This packet reconstructs the complete `hud_messaging_update` graph rather than
tuning another isolated residual.  The unit moves from 30 exact / 2 residual /
4 unwritten January owners to **33 exact / 3 residual / 0 unwritten** owners.
Three ordinary C helpers become strict exact:

| function | meaningful bytes | padded bytes |
| --- | ---: | ---: |
| `_compare_messages` | 47 | 48 |
| `_render_state_text` | 128 | 128 |
| `_render_state_bitmap` | 387 | 400 |
| **total** | **562** | **576** |

`_hud_messaging_update` is a complete, typed **fuzzy** reconstruction and
receives zero exact credit.  January is 2,928 padded bytes with 137
relocations and normalized SHA-256
`26a1376422e3742f9d65a8d83241919e1e370769a936ade9c9b859829d7d1b7b`;
the retained candidate is 2,880 padded bytes with 136 relocations and
normalized SHA-256
`d5d32a3ce4d44e6e87ed3daa1d1ce28cf1a746bd53ae0b678bf7ec99a955d815`.
The production objdiff percentage is recorded in `config/parked.json` after
the aggregate full build.

## Evidence and reconstruction

January target code, assertion strings, relocations, data placement and
function boundaries are authoritative.  The independent HCEA reconstruction
at `scratch/_halocea-reference-20260907/src/blam/interface/` supplies the
semantic control-flow map.  Every target dependency was checked against the
January relocation stream; later-build behavior was not assumed when January
disagreed.

The retained renderer covers objective/help/custom state messages, text and
bitmap elements, input-button remapping, split-screen placement, timed item
messages, fading and expiration.  It uses named HUD enums, `real` and the
existing typed tag macros.  Exact-coordinate assertion wrappers preserve the
January file/line payloads.  Target evidence requires two calls to
`global_scenario_get` on the scenario-name path and distinct raw-text and
item-text draw call sites; both are represented directly instead of being
compiler-steered away.

The target data directly identifies `button_mappings`, `time_code_time`, and
`time_code_stop_time`.  The adjacent four-byte value 17 is named
`split_screen_hud_message_offset`, consistent with both its two January uses
and the cross-build source.  It is target-static in `config/symbols.json`.

The remaining renderer delta is not missing high-level behavior.  Size and
relocation comparison leaves a 48-byte / one-relocation difference dominated
by VC7's duplicate `hud_msg_def` load and stack/register schedule around the
split-screen line-top calculation.  Further expression perturbation would be
low-leverage compiler steering, so the body is parked for future source/local
evidence.

## House-rule and gate checks

- No private `code + address` or global `bss + address` names were introduced.
- Typed tag access, named enum constants and project scalar/color/geometry
  types are used throughout.
- `get_flash_color` is declared in its genuine `hud_draw.h` owner.
- No forced inline, volatile/register steering, raw offset, fake dependency,
  undefined behavior, pointer representation pun, inline assembly or
  nonsensical branch is retained.
- The isolated full-unit gate reports 33 exact / 3 residual / 0 unwritten.
- The `_point_from_line3d` emitted-symbol guard and changed-source fake-match
  scan pass.

Final production build, rename-stable sweep, park audit, semantic totals and
tool-suite results are recorded in the aggregate reconciliation checkpoint.
