# `hud_messaging.obj` park re-investigation — opus5 150K lane, wave w1 (2026-09-14)

## Baseline

Real file `source/interface/hud_messaging.c` at lane base `12f7375d4`:
**33 exact / 3 residual / 0 unwritten**. The `_point_from_line3d` guard passes.
All three residual rows are parked in `config/parked.json`:

| function | January padded / relocs | park base | gate tag |
| --- | --- | --- | --- |
| `_hud_messaging_update` | 2928 / 137 | 2880 / 136 | size, relocs, sha |
| `_scripted_hud_set_objective` | 160 / 8 | 160 / 8 | sha |
| `_scripted_hud_get_timer_ticks` | 80 / 2 | 80 / 2 | sha |

The parks are locked, so this wave changed no real source. All work is in
`scratch/workers/hud_unit_messaging/`, and `git diff -- source/interface/hud_messaging.c` is empty.

## Outcomes

### `_scripted_hud_get_timer_ticks` — park-reopen proposed (strict EXACT in scratch)

- Candidate: `scratch/workers/hud_unit_messaging/o5.c`. This is the current real file plus only this body
  and the objective body below.
- Result: 80 / 2, normalized SHA-256 `c28cd50e0150845564921ef4ab5bb445c5a9cb58ac319a84461460fa53686468`,
  which equals January.

**Lever: the single-exit law.** The park's only difference was January's separate `mov eax,-1` epilogue
against our `or ax,ax`. The park kept an early `return NONE`. January instead assigns a single `short ticks`
result through an if / else-if / else chain and returns it once at the bottom:
- the hidden timer returns 0;
- the `NONE` sentinel returns `NONE`;
- a paused timer returns the stored ticks;
- a running timer returns `(short)(ticks + reference_time - game_time_get())`.

VC7 duplicates the small constant-result return block, which produces January's `mov eax,-1` epilogue.

Shapes tried:

| shape | result |
| --- | --- |
| early return kept | residual (the park) |
| `long` result variable | residual |
| single exit, old casted expression | EXACT |
| single exit, natural expression | EXACT |
| single exit, natural expression plus explicit `(short)` truncation (chosen) | EXACT |

Park criterion: "a natural same-compiler donor explaining the full-width sentinel rematerialization". The
single-exit law, measured in earlier lanes, is that explanation.

### `_scripted_hud_set_objective` — park-reopen proposed (strict EXACT in scratch)

- Candidate: same `o5.c`.
- Result: 160 / 8, normalized SHA-256 `a83e487b78b95cb689279b7de9dfe2a3078ce9e67c5809282fdd7a6b3ebc6a54`,
  which equals January.

January reads `fade_ticks` at `[eax+0x11e]`, then `add eax,0x100` and reads `up_ticks` at `+0x1c`. That is one
materialized `&hud_globals->messaging.objective_color` base. The same TU's `hud_messaging_update` already
declares `struct hud_color_definition *objective_color = &hud_globals->messaging.objective_color;` and reads
`objective_color->custom.objective.up_ticks / fade_ticks`.

Using that owner idiom produces January's bytes exactly: a block-top `objective_color` pointer, direct
`hud_messaging_globals` stores, and `up_ticks + fade_ticks`.

The earlier pointer trial that the park rejected also kept the `hud`/`globals` copy locals and the scalar
`up_ticks`/`fade_ticks` locals. Negatives:
- pointer assigned after the objective store;
- no locals, either operand order.

### `_hud_messaging_update` — not landed; normalized bytes identical, csplit relocation alias remains

- Candidate: `scratch/workers/hud_unit_messaging/MF2.c` (copy: `hud_messaging_candidate.c`), built on `o5.c`.
- Result: 2928 / 137 with normalized SHA-256 `26a1376422e3742f9d65a8d83241919e1e370769a936ade9c9b859829d7d1b7b`.
  This equals January.
- The gate tag is only `[reloc-identity]`. Relocations `+0x4f7` and `+0x510` are spelled
  `_global_hud_anchor_names+2` by csplit and `_button_mappings-18` by VC7. They are the same image address:
  `3034168 + 2 = 3034188 - 18`.

The park's "load/register schedule" description was wrong: the gap was structural. Every change below is
evidence-led:

**Frame and locals**
- `sub esp,0x2a0` against our `0x2c8`. The item `formatted` buffer is `wchar_t[256]`, not 276. With that
  change the slot census equals January's.
- State-message locals are block scoped: `preferences`, `state_color`, `pixel_color`, `line_bounds` and
  `line_cursor`.
- January keeps a `state_message` pointer (`&datum->state_message` at `[ebp-0x30]`) for the custom-icon
  `is_text_flags` and `info[]` reads.

**Calls and control flow**
- The objective colour is `pixel32_to_real_argb_color(get_flash_color(objective_color, objective_start_time), &state_color)`.
  The start time is computed first, because January pushes `&state_color` after the time arithmetic.
- `hud_get_font_index()` is called. VC7 inlines it into the exact bytes of the previous hand-expanded `?:`,
  which removes manual inlining.
- Branch layout is `data > _icon_right_stick`, `!show_hud_help_text`, and `string_index != NONE`.
- The help colour tests `!TEST_FLAG(reverse)` first.

**Store and evaluation order**
- `line_bounds` and `message_bounds` are stored in the order `y0, x0, x1, y1`. All 24 orders were measured;
  x1 before y1 is required for the 2928 size.
- The split-screen `line_top` is `screen_point.y - split_screen_hud_message_offset + spacing * line_height`
  (integer part first).
- In the message loop, the `text_color` copy comes between `game_time_get()` and `elapsed`.

**Message loop**
- A plain `for` loop replaces the `if (maximum_message_count > 0)` wrapper. The rotated guard is
  `cmp word [ebp-0x3c],0`.
- The item arm is `!= NONE` and comes first. The `message_offset` if/else runs before the item tag lookup.
- `short string_index` and `short value_scale = MAX(...)` are proven by January's 16-bit add and `movsx`.
- The formatted string gets its own draw call.

Rejected:
- a uniform `state_message->state_message` (2944 / 138);
- `render_state_text` calls for the three expanded text draws (out of line, 2768 / 131).

Checks:
- all other rows unchanged;
- guard passes;
- the owner census matches the base object (63 owners, no candidate-only owner against the target, no COMMON);
- fake-match scan: 0 leads (HEAD: 0).

The fail-closed csplit-relocation-alias validator (`tools/parked_functions.py`) cannot admit this function
today. `_button_mappings`, `_render_state_text` and `_render_state_bitmap` each appear twice in
`config/symbols.json` (TU statics of other objects), so `image_symbol_addresses` drops them as ambiguous.

## Proposals

1. Unpark `_scripted_hud_get_timer_ticks` and `_scripted_hud_set_objective` using `o5.c`: 240 padded bytes,
   strict EXACT, no sibling or park drift for the third park.
2. Re-class `_hud_messaging_update` as `csplit-relocation-alias` with base 2928 / 137 / `26a13764…` using
   `MF2.c`. This needs per-unit resolution of duplicate TU-static names in the validator. Otherwise re-measure
   the unclassified park with the closer body.

## Reopen criteria

`_hud_messaging_update`: tooling that resolves TU-local static names to their own unit's image address, or an
owner ruling on csplit-alias credit.
