# `ui_widget.obj`: Opus5 house-clean lane (2026-09-13)

## 1. Scope and provenance

- Translation unit `source/interface/ui_widget.c`, target `build/split/source/interface/ui_widget.obj`.
- Lane worktree `C:\halo-worktrees\opus5-50k-house-clean-20260913`, branch
  `opus/50k-house-clean-20260913`, pinned base `73e97eca5`.
- Compiler: VC7 13.00.9254 (XDK), repository flags `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` plus include
  paths; no flag was changed or passed.
- Evidence: January object code, relocations and `.rdata` order; `config/symbols.json` linkage
  (both reload helpers `static: true`); HCEA `src/blam/interface/*.c` for semantics/signatures only.
- All prior `ui_widget_obj_*` ledgers and every ledger naming these functions were read first
  (in particular `claude_ui_weather_aim_render_hs_reconciliation_20260908.md`,
  `claude_remaining_donor_batch_20260908.md`, `ui_widget_game_data_profile_fuzzy_reconciliation_20260909.md`,
  `ui_widget_obj_bitmap_reconstruction_20260902.md`, `ui_widget_obj_opus_pool_owner_reconciliation_20260904.md`).
  No parked entry exists for this unit.

## 2. Validated baseline

`python tools/campaign/gate.py source/interface/ui_widget --all --forbid-emitted-symbol _point_from_line3d`

| state | exact | residual | unwritten | exact padded | residual padded | unwritten padded |
|---|---:|---:|---:|---:|---:|---:|
| baseline (HEAD) | 85 | 15 | 2 | 13,952 | 9,776 | 32 |
| final | **89** | 13 | 0 | **15,024** | 8,736 | 0 |

Fake-match scan: 0 review leads before and after. Data sections unchanged in owner set.

## 3. Accepted controls (landed, strict EXACT)

| function | padded / meaningful | relocs | normalized sha (target = final) |
|---|---:|---:|---|
| `_widget_instance_reload_recursive` | 16 / 1 | 0 | `499f1f307c1cb989...` |
| `_ui_widget_reload_by_tag` | 16 / 1 | 0 | `499f1f307c1cb989...` |
| `_widget_instance_initialize` | 496 / 493 | 18 | `2c4ba33b887e...` |
| `_ui_widget_load_by_name_or_tag` | 544 / 544 | 35 | `0015243618ef...` |

Net: +4 strict exact functions, +1,072 padded / +1,039 meaningful exact code bytes.

- **Reload helpers.** January emits both private 16-byte bare-`ret` COMDATs with no incoming
  relocation. VC7 discards an unused static, so January's source defined *and called* both, with
  every call inlined (vc7 static-COMDAT law; `progress_bar` precedent). The earlier de-credit was
  correct because the helpers had no caller; the canonical rejection was of making them public.
  This lane keeps them `static` and restores their callers in `event_handler_dispatch`: the tag
  flag `_event_handler_reload_self_bit` calls `widget_instance_reload_recursive(widget)` and the
  existing "failed to reload widget because event_handler->ui_widget_tag == NONE" branch becomes
  the else arm of `if (index != NONE) ui_widget_reload_by_tag(index);`, mirroring the neighbouring
  give-focus arm. Signatures follow HCEA (`widget_instance *`, `int tag_index`). The inlined empty
  calls emit nothing; `event_handler_dispatch` stays EXACT.
- **`widget_instance_initialize`.** The three header stores are written in `struct widget_instance`
  declaration order (`definition_tag_index` +0, `name` +4, `local_player_index` +8). Only two
  orders were measured.
- **`ui_widget_load_by_name_or_tag`.** Three structural corrections, each read from January:
  frame `sub esp,0x14` with the controller switch storing into the `local_player_index`
  parameter home (`mov [ebp+0x14],N`), so there is no `controller_index` copy; a single exit with
  `widget = NULL` (January returns a pinned zero web, `xor edi,edi` ... `mov eax,edi`, on the
  not-loaded path); and the not-loaded error as the `else` arm of `if (tag_index != NONE)`
  (January sinks that block after the out-of-memory epilogue). January's `.rdata` literal order
  (not-loaded string before out-of-memory string) now matches too. Behaviour is unchanged.

Checks for every landing: whole-TU gate with no baseline-exact row lost (row-by-row diff),
forbidden-emission guard, owner census (only two extra per-function `.debug$F` records), fake scan.

## 4. Experiment matrix (not landed; all reverted)

| function | shape | result |
|---|---|---|
| `_widget_instance_text_box_is_focused` | if/else instead of `a||b` expression | byte-identical |
|  | HCEA do-while + `focused = parent ? ... : TRUE` + break | best: only the inner list-type arm order differs |
|  | two-case `switch (ancestor->type)` | movsx range check, 31 insns |
| `_widget_instance_render_column_list` | function-scope locals, compound `for` | identical 160/2 |
|  | definition moved after `widget_instance_render_recursive` + private prototype | object byte-identical |
|  | prototype also moved after render_recursive's | byte-identical |
| `_render_ui_widgets_postgame` | HCEA `render_widget` eligibility flag | identical IV strength reduction |
|  | no `widget` local | identical |
|  | `short widget_index` | much worse (frame 0x50) |
| `_render_ui_widgets` | single exit nested in `!bink` | byte-identical |
|  | `if (!virtual_keyboard_active()) {...} else virtual_keyboard_render();` | 178/178 insns, only IV strength reduction left |
| `_ui_check_for_pause_game` | loop only finds controller, handling after `if (pause_pressed)` | 640/47, short IV compare |
|  | solo `case 2` before `case 0: case 1:` | byte-identical |
|  | above + `long controller_index` IV, `short pressing_controller_index` | best: loop identical, error tails still cross-jumped |
| `_ui_widget_load_children_recursive` | zero block + `if (list || pass_unhandled)` | 232 insns |
|  | single-exit `focus_a_child` flag | byte-identical |
|  | inverted list test | 241 insns |
| `_ui_widget_launch_widget` | ternary parent tag | 320 [size], -1 pinned |
|  | explicit if/else | same as ternary |
|  | no `parent` local | byte-identical |
| `_search_and_replace` | split `replacements = 0;` | byte-identical |
| `_display_scenario_help` | single exit `widget_name = NULL` | 624 [size] |
|  | `scenario_name` scoped in the if | byte-identical |
| `_display_error` | `top_widget_tag_index` computed before dialog test | load region identical |
|  | + `PIN(error_code, 0, NUMBER_OF_ERROR_CODES - 1)` | best: 299/299 insns |
|  | + case 3 explicit if/else | 912/71 worse |

Best-known shapes are preserved in the lane scratch directory
(`scratch/workers/ui_widget/BEST_*.txt`) but not landed: they are rewrites of already-written
residuals and none is strict exact.

## 5. Do-not-repeat

- `text_box_is_focused`: `==||==` vs if/else vs switch for the list-type test.
- `render_column_list`: definition/prototype order, loop spelling (COMDAT order is not definition order).
- `render_ui_widgets(_postgame)`: eligibility flag, removing the `widget` local, `short` IV,
  nesting early returns alone.
- `ui_check_for_pause_game`: switch case order.
- `load_children_recursive`: the three focus-tail spellings above.
- `launch_widget`: ternary/if-else parent tag, removing `parent`.
- `search_and_replace`: declaration/initialiser splits (zero-pinning class).
- `display_scenario_help`: single-exit rewrite, `scenario_name` scope.
- Never force emission of the reload helpers by removing `static`.

## 6. Residual classification (13 rows)

| function | class | measured fact |
|---|---|---|
| `_widget_instance_text_box_is_focused` | selection/layout | inner list-type arm FALSE-first vs January TRUE-first |
| `_widget_instance_render_column_list` | private ABI | body instructions identical; January keeps `widget` on the stack, VC7 promotes it to EDI |
| `_render_ui_widgets_postgame` | selection | loop IV strength-reduced + LFTR (extra reloc) vs January's index loop |
| `_render_ui_widgets` | selection | same IV defect; vk-render block order fixable |
| `_ui_check_for_pause_game` | scheduling | five unmerged `error` tails in January vs three cross-jumped in ours |
| `_ui_widget_load_children_recursive` | layout | inline duplicated epilogue for the `!pass_unhandled` exit |
| `_ui_widget_launch_widget` | scheduling | widget load position; NONE materialisation for parent tag |
| `_search_and_replace` | register | zero-register pinning |
| `_display_scenario_help` | register | EBX vs EDI for `string_index`, everything else equal |
| `_display_error` | register/layout | EBX/EDI swap, default-case placement, zero pinning |
| `_draw_bitmap_in_rect` | scheduling | plasma parameter-block store order |
| `_widget_instance_render_recursive` | slot assignment | inline-scope slot sharing ([ebp-4] reuse) differs |
| `_widget_instance_process_one_event_recursive` | scheduling | less jump threading/CSE/cross-jumping in January (unmerged tab-call tails) |

## 7. Reopen criteria

- IV strength reduction (both render_ui_widgets functions): a source feature that keeps a 0..3
  `long` index used twice as a subscript and once in `== 0` from being strength-reduced.
- Cross-jumping (`ui_check_for_pause_game`, `process_one_event_recursive`): a proven source lever
  or evidence of extra January source complexity.
- `display_error`: start from the best shape (`top_widget_tag_index` + `PIN`); needs a reason for
  the default-case placement.
- `render_column_list`: a lever that demotes a register-promoted pointer parameter; re-measure
  with any `render_recursive` change (mutual recursion).
- `draw_bitmap_in_rect`: January-concordant order of the plasma parameter assignments.

## 8. Disposition

Four functions are strict exact and eligible for credit. The 13 residuals stay active,
not parked, with no fuzzy credit. No header, config, symbols, compiler flag or other translation
unit was touched. No ninja, configure, commit or test run was performed in this lane.
