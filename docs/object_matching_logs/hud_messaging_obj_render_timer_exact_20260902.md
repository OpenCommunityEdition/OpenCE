# `hud_messaging.obj` HUD timer renderer exact recovery (2026-09-02)

## Result

`_hud_render_timer` is byte-exact at **704 / 704 padded bytes**, with all **37
relocations** and the January normalized SHA-256
`4b605abb55b99074f56e746c71930da649b136bf7e01557467740b3f8577b20c`.
The focused object gate advances from **29 exact / 2 residual / 5 unwritten**
to **30 exact / 2 residual / 4 unwritten**, adding 701 meaningful and 704
padded matched code bytes without changing any inherited exact function.

## Reconstructed behavior

The retained ordinary C implements the complete timer and loading-time draw
path visible in January:

- it skips the timer-number path when the scripted HUD timer is hidden;
- builds the zero-filled absolute placement and the two-digit number element;
- resolves the HUD-digits tag through the typed
  `HUD_NUMBER_DEFINITION_GET` accessor and derives the signed glyph advance;
- uses the recovered HUD-corner enum to position left, right, or centered
  timer fields, with January's unreachable assertion for invalid values;
- copies the running or expired `hud_color_definition` from the authenticated
  `hud_globals.timer_definition` layout;
- latches the warning cutoff and expired reference time;
- uses `MAX`, `TICKS_PER_SECOND`, and `SECONDS_PER_MINUTE` for the clamped
  minute, second, and hundredths values;
- renders all three fields through the owner-declared `hud_draw_numbers`; and
- preserves January's optional loading-time `sprintf` and debug-string tail.

The runtime timer position is modeled as `point2d`; the number placement,
color, timer-tag, and HUD-global records use the HCEA database/PDB-confirmed
names and offsets. Compile-time assertions prove the 0x24 absolute placement,
0x24 relative placement, 0x54 number element, 0x64 HUD-number tag, and 0x360
HUD timer-definition offset.

## Evidence and source shape

The primary semantic donor is
`_research_cache/halocea/src/blam/interface/hud_render_timer.c`, cross-checked
against its database-confirmed headers for `hud_globals_definition`,
`hud_timer_definition`, `number_hud_element_definition`,
`hud_absolute_placement_definition`, `hud_placement_definition`,
`hud_number_definition`, and `hud_color_definition`. The independent Stian
January-PC and pastudan/raw lifts corroborate the complete branch, arithmetic,
copy, and call topology. January COFF remains authoritative for instruction
bytes, relocation identity, switch-table entries, assertion provenance, and
storage class.

The exact aggregate initializer naturally emits January's partial
zero-initialization sequence. One ordinary lexical scope around
`flash_cutoff` keeps the value's lifetime after the running-color structure
copy and naturally reproduces January's CX schedule. No volatile or register
forcing, barrier, pragma, assembly, raw address/offset, pointer pun, fake
dependency, forced inline, or nonsensical branch is present.

The public prototype now lives in `source/interface/hud_messaging.h`.
`source/interface/interface.c` includes that owner and no longer carries its
ad-hoc consumer declaration.

## Validation

- focused exact gate and direct normalized-section comparison: pass (704
  padded bytes, 37 relocations, identical normalized SHA-256);
- full HUD gate: `30 exact / 2 residual / 4 unwritten`;
- interface, HS, players, HUD, and game-engine consumer gates retain their
  prior exact counts;
- changed-file fake-match scan: zero review leads;
- production `ninja all_source`: pass;
- parked functions: 106 active / 0 stale / 0 invalid; and
- focused comparison/regression/park/fake-scan tests: 97 passed.
