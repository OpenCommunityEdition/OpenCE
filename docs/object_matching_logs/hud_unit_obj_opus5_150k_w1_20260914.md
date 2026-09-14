# `hud_unit.obj` renderer park re-investigation — opus5 150K lane, wave w1 (2026-09-14)

## Baseline

Real file `source/interface/hud_unit.c` at lane base `12f7375d4`:
**21 exact / 1 residual / 0 unwritten**. The `_point_from_line3d` guard passes.

The residual is `_hud_render_unit_interface`, which is parked:
- January: 3504 / 102, normalized SHA-256 `b82d575e…`;
- park base: 3472 / 102, `4071a4f0…`.

A structure-only diff, with registers and frame slots masked, measured 115 blocks / 229 instructions at base.
Nothing was landed, and `git diff -- source/interface/hud_unit.c` is empty.

## New evidence and structural corrections (scratch)

The 2026-09-04 ledgers ended with "no further residual spelling or allocation search". This wave used new
January-side evidence instead:
- the frame-slot census;
- store/call ordering;
- instruction order read against the HCEA statement order of `hud_render_unit_interface.c`.

Each item below was measured (`scratch/workers/hud_unit_messaging/e*.txt`, `ur*.obj`).

**Unit and HUD state setup**
1. Declare `unit` and `unit_definition` before `local_player_index`. January loads `[player+2]` after the
   unit tag lookup.
2. Call `initialize_hud_state(get_hud_state(player->local_player_index))`. January reloads `[player+2]` there.
3. Test `if (hud_unit && unit_hud_indices[unit_count] != NONE)`. January reads the HUD index after the null
   test.

**Shield loop**
4. Follow the HCEA statement order, with `boolean first_meter` and `boolean shield_fading`, and select the
   maximum as `shield_fading ? last : shield`. January's single `fcom` sets the flag and selects the maximum.
5. The shield `value_scale` and meter copy are statements after `game_engine_running()` and the
   reference-vitality selection.
6. `color[5]` is declared in a nested block after the copy. January's colour stores follow `rep movsd`.

**Health meter**
7. `value_scale` and the meter copy are statements. January copies before the cutoff comparisons.

**Auxiliary overlays**
8. Compute an `auxilary_panel` pointer before `game_engine_has_teams()`.
9. Declare `word overlay_type_flags`, matching January's `movzx ecx,si`.

**Auxiliary meters**
10. Remove the `real value` local and re-index `auxilary_values[meter->type]`. January shares one `fast_ftol`
    value slot; this change produced **frame 0x424 == January**.
11. Remove the `short *flash_time` pointers. January re-derives `&hud_state->auxilary_flash_time[type]` in each
    statement.
12. For an inactive meter, set the multiplayer flag first and then `SET_FLAG(flashing, TRUE)`. January ORs
    bit 0 last.

## Best candidate

`scratch/workers/hud_unit_messaging/hud_unit_candidate.c`:
- 3504 / 102, normalized SHA-256 `b521ab4612227514896133b7b776f80e9481dc395893fafb9ad21a0e293e13e3`;
- structure-only diff 33 blocks / 43 instructions;
- all 21 exact siblings unchanged;
- guard passes;
- owner census equal to the base object (the `_fast_ftol` COMDAT boundary is pre-existing);
- fake-match scan: 0 leads.

## Residual (not exact)

- **Shield loop `color[]` induction variable.** January strength-reduces `color[overcharge_index]` into an EBX
  pointer; VC7 keeps indexed addressing. As a knock-on, the shield `draw_flags` sits in EBX instead of in
  memory, and the frame is 0x420 against 0x424.
- **`health_meter` placement.** It is placed below the two 18-entry index arrays. Reference census is
  identical (7 refs / 104 B against 5 refs / 72 B), but the order is the opposite of January's.
- **Health colour.** The `min_color = max_color` store is forwarded per predecessor in January.
- **Registers.** Other scratch-register choices differ.

Classification: register-allocation / induction-variable selection / frame-order tie.

Allocation-level shapes tried, none closer:

| shape | result |
| --- | --- |
| flags via `if`/assign | 38 / 50 |
| flags via `SET_FLAG` | 38 / 50 |
| child comparison operand swap | 36 / 46 |
| `health_meter` at panel scope | 29 / 40, but 3520 B |
| `health_meter` at loop scope | 33 / 43, layout moved |
| inactive `draw_flags = FLAG(flashing)` initializer | 3536 B |

Authenticity notes for any future adoption:
- the nested bare block for `color[]` is driven by January's ordering;
- `SET_FLAG(draw_flags, _hud_draw_flashing_bit, TRUE)` is driven by January's ordering;
- review both against Bungie style.

## Proposal and reopen criterion

Optional: re-measure the park base with `hud_unit_candidate.c`. It is closer, with January's size and
relocation count, but carries no credit.

Reopen when a source or compiler lever is found that:
- makes VC7 select the `color[]` induction variable; and
- orders `health_meter` above the index arrays.
