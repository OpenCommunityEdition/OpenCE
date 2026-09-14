# `ui_widget.obj`: Opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/interface/ui_widget.c`; target `build/split/source/interface/ui_widget.obj`.
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/150k-house-clean-20260914`, base `12f7375d4`.
- Compiler: VC7 13.00.9254 (XDK 3911) with the repository flags. No flag, header, `config/symbols.json` or
  `config/parked.json` change was made.
- Evidence used:
  - January object code, relocations, COMDAT section order and `.rdata` literal order.
  - The January map atlas `scratch/atlas/ui_widget.obj.txt` and the Ghidra decompilation
    `scratch/ghidra/out/ui_widget.obj.decomp.c`. Both are new for this object; the 100K lane recorded them as absent.
  - January assert line anchors, read as a source-order proxy.
  - HCEA `src/blam/interface/*.c`, for semantics only.
- Prior work read before starting:
  - ledgers `ui_widget_obj_opus5_100k_20260914.md`, `ui_widget_obj_opus5_house_clean_20260913.md`,
    `claude_ui_weather_aim_render_hs_reconciliation_20260908.md` and `ui_widget_obj_opus_pool_owner_reconciliation_20260904.md`;
  - the 100K lane worker notes and the 50K evidence-tree `BEST_*` bodies.
- `branch_sweep.py` found no blob better than HEAD. No parked entry exists for this unit.
- Excluded by the orchestrator and not touched: `_render_ui_widgets`, `_render_ui_widgets_postgame`.
- Per-function worker blocks: `scratch/workers/ui_widget.md`.

## 2. Baseline and final

`python -B tools/campaign/gate.py source/interface/ui_widget --all --forbid-emitted-symbol _point_from_line3d`

| state | exact | residual | unwritten |
|---|---:|---:|---:|
| baseline (HEAD) | 90 | 12 | 0 |
| final | **91** | 11 | 0 |

Only one row changed in the row-by-row diff: `_widget_instance_render_column_list` went from residual to EXACT.

The other checks all passed:

- the `_point_from_line3d` guard;
- the owner census, which is identical to `build/base` with no COMMON;
- the fake-match scan, with 0 leads;
- park drift (there are no parks);
- `git diff --check`.

## 3. Landed: `_widget_instance_render_column_list` (176 padded, 2 relocations, strict EXACT)

Before this change the body was already instruction-identical. The mismatch was the calling convention:

- in our build VC7 gave this static function a private register ABI, with `widget` passed in EDI (160 bytes);
- January passes `widget` on the stack and saves ebx/esi/edi (176 bytes).

### Measured VC7 compile-order law (5 probes)

A static function in a call-graph cycle keeps the standard ABI only if the backend compiles it after its partner.
C2 enters each strongly connected component from the **last-defined** function that reaches it; in effect this is a
reverse-definition-order DFS. The back edge into that entry node is dropped. The partner is therefore compiled first,
and the entry node, compiled last, keeps the standard ABI.

The table shows which build is EXACT for each placement of the column_list definition, relative to the cycle
`{render_recursive, render_column_list}` and the callers `render_ui_widgets_postgame` and `render_ui_widgets`.

| probe | where the column_list definition sits | result |
|---|---|---|
| v1 | unchanged; a prototype was added before render_recursive's | 160, no change |
| v2 | after render_recursive, before postgame | 160 |
| v4 | between postgame and render_ui_widgets | 160 |
| **v3** | **after render_ui_widgets** | **176, EXACT (landed)** |
| v5 | before process_ui_widgets | 176, EXACT |

The prototype and definition-order probes from earlier ledgers were byte-identical because `render_ui_widgets` was
still defined last. The atlas corroborates the result: January's register-ABI statics are decorated as fastcall
(`@widget_instance_text_box_is_focused@4`, `@get_icon_type@4`, `@column_list_update@8`,
`@widget_instance_find_by_tag_index_recursive@8`), while `_widget_instance_render_column_list` appears plain.

The source change moves the unchanged definition after `render_ui_widgets`. A file-local static prototype sits beside
the other render prototypes; it is required because render_recursive now calls the function before its definition.
No logic changed.

## 4. Not landed

### `_ui_widget_launch_widget`: compile-order prerequisite proven

The function belongs to the SCC {launch_widget, load_by_name_or_tag, widget_instance_initialize,
event_handler_dispatch, go_back_to_previous, ui_widget_delete, delete_children_recursive,
load_children_recursive}. In January's COMDAT order launch_widget is the entry node and is compiled last. January's
assert anchors 5380/5392 place its definition after process_ui_widgets (644), process_one_event_recursive (3067–3477),
event_handler_dispatch (3721) and draw_string_and_hack_in_icons (4341).

Moving the definition to the end of the file (scratch `v6.c`) reproduces January's SCC emission order. The widget
load now matches January, and every other function stays byte-identical. One difference remains: January writes the
parent tag as an if/else, `je; mov ebx,[ecx]; jmp; or ebx,-1`. Every explicit spelling tried makes VC7 pin −1 into a
register:

- if/else;
- `?:`;
- declare+init;
- no `parent` local;
- the parent tag passed as a call argument.

None was landed, because the move alone is not exact.

### `_ui_check_for_pause_game`: size and relocation gap closed in scratch

Measured mechanism: VC7 cross-jumping merges identical `push 2; call _error; add esp,8` tails into the block that
**falls through** into the common exit.

- In January, `ui_widgets_close_all()` falls into the exit (0x206 → 0x20b), so none of the five `_error` tails merge.
- In our build, the default case's full-screen load/error falls into the exit, so four tails merge into two call
  sites.

Writing the default as `if (!widgets_active) { load / error } else { ui_widgets_close_all(); }`, on top of the prior
BEST loop form, gives **672 padded bytes and 50 relocations**, matching the target. Only an esi/edi colouring swap
remains, between the controller index and `local_player_count`. The best candidate is
`scratch/workers/ui_widget/BEST_pause_game_p1.c`.

Colouring probes:

- These were inert: dropping the `pressing_controller_index` initialiser, moving declarations, a function-scope IV,
  and discarding the unused `network_game_data`/`machine_index` results.
- Handling after the loop with `if (controller_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS)` and a short copy reproduces
  January's colouring. It also leaves an unthreadable `cmp esi,4; jge` on the found path, which January does not have.
- A flag form, or the handling inside the loop, brings the swap back.

House-rule note for any future landing: the real file's `network_game_data` and `machine_index` locals are unused.
Discarding the call results instead is byte-identical.

### `_display_error`

New evidence comes from literal COMDAT order, which is the reverse of code emission. Only the halfscreen pair
differs, which shows a different cross-jump survivor:

- January keeps **case 2's** halfscreen ternary. Case 3 hoists `mov al,[modal]` and jumps into case 2's and case 4's
  tests.
- Our build keeps case 3's copy.

January also places the default (assert + return) in source position, while ours goes to the end of the function.
None of these probes changed the output; case bodies are laid out by case value, not source order:

- `default:` first: byte-identical;
- `case 2` moved last: byte-identical;
- a case 3 → case 2 fallthrough form: identical layout.

### Other residuals

- **`_widget_instance_process_one_event_recursive`:** the relocation gap is exactly one `tab_to_previous` call.
  - January does not jump-thread `event_handled` from the children-updown tail into the leftright block. It tests the
    flag at runtime at 0x319, 0x3e8 and 0x45d, so the block tails stay different and unmerged.
  - Our build threads, proves `audio_feedback == none`, and cross-jumps both blocks into one tail.
  - No source lever was found. The `dpad_event_times` BSS split proposal is carried forward.
- **`_widget_instance_render_recursive`:** the 100K lane's reopen criterion (fix column_list's ABI) is now met, but the
  diff is unchanged, so the slot map is independent of that ABI.
  - January keeps `clip` in the widget parameter home, in memory at the `if (clip_rect)` join.
  - Our build registerises the phi.
  - Probes: `clipped` scoped inside the if, which changed nothing; `alpha *= pulse`, which is worse.
- **`_search_and_replace`:** January's zero initialiser reaches the return in EAX before the lazy pushes. Early-return
  forms (`return 0`, `return replacements`) are constant-propagated and grow the function to 480 bytes.
- **`_display_scenario_help`:** only a register preference differs (`string_index` in ebx vs edi). Passing the
  controller call directly as an argument changes evaluation order.
- **`_widget_instance_text_box_is_focused`:** only the order of the boolean store arms differs. Probe: a default
  `focused = FALSE` store, which is worse.
- **`_draw_bitmap_in_rect`:** the plasma parameter store schedule was decoded against `rasterizer.h:145`. January
  stores `map_offset[1]` in source position; our build hoists it together with the `plasma_fade` copy. No shape was
  spent.

## 5. Rejected shapes (do not repeat)

- **render_column_list:** a prototype alone; a definition before `render_ui_widgets`.
- **launch_widget,** after the definition move:
  - if/else, `?:`, declare+init `= NONE`, no `parent` local;
  - the parent tag as a call argument.
- **pause_game,** on top of p1:
  - no initialiser; declaration order; function-scope IV;
  - discarded results (inert);
  - post-loop index test (unthreaded compare); flag form; handling inside the loop.
- **display_error:** default first; case 2 last; case 3 → case 2 fallthrough.
- **render_recursive:** scoped `clipped`; `alpha *=`.
- **search_and_replace:** early-return guards (480 bytes).
- **display_scenario_help:** no `local_player_index` local.
- **text_box_is_focused:** default-FALSE store.

## 6. ORCHESTRATOR-PROPOSAL

1. **Config, carried from the 100K lane and still unlanded.** Split `_ui_widget_globals_storage` (`config/symbols.json`
   file_offset 4538944) so that +0x870, 0x40 bytes, is its own static BSS owner `dpad_event_times`. The .c would then
   define `static unsigned long dpad_event_times[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS][NUMBER_OF_DPAD_DIRECTIONS];` in place
   of the alias and the struct tail member.
   - Evidence: January `lea eax,[edx+ecx*4-8]` / `[eax*4+.bss:2160]`.
   - Consumer: `ui_widget.c` only.
2. **Measured law, for the brief's §9.** Compile order in call-graph cycles: an SCC is entered from its last-defined
   reaching function, and that entry node keeps the standard ABI. A static that January shows with stack arguments,
   while we give it a register ABI, can be fixed by its definition position. Read January's order from COMDAT section
   order and assert anchors; no body change is needed.
3. **Measured law, for the brief's §9.** Cross-jump fallthrough: VC7 merges identical call tails only into the block
   that falls through into the shared successor. A January object with more un-merged identical tails than ours
   usually has a different fallthrough predecessor there; this is often a flipped if/else polarity.

## 7. Reopen criteria

- **launch_widget:** land the definition move together with a parent-tag spelling that VC7 does not pin. Start from
  scratch `v6.c`.
- **ui_check_for_pause_game:** start from `BEST_pause_game_p1.c`. A natural feature is needed that gives the
  controller-index web colouring priority without an unthreaded post-loop compare.
- **display_error:** a form that makes case 2's halfscreen ternary the cross-jump survivor. The observable check is
  that `nonmodal_half` sorts before `modal_half` in the literal order.
- **process_one_event_recursive:** a source feature that blocks threading of `event_handled`, measured together with
  the BSS split.
- **render_recursive:** a `clip` spelling that keeps the join in memory.
- **search_and_replace, display_scenario_help, text_box_is_focused, draw_bitmap_in_rect:** as recorded in the prior
  ledgers.

## 8. Disposition

One function is newly strict EXACT and eligible for credit: `_widget_instance_render_column_list`, 176 padded bytes.
The 11 remaining residuals stay active; none are parked and none carry fuzzy credit.
