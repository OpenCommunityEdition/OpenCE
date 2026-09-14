# `ui_widget.obj`: Opus5 100K consolidated lane (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/interface/ui_widget.c`, target `build/split/source/interface/ui_widget.obj`.
- Lane worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`, branch
  `opus/100k-consolidated-20260914` (base canonical `c3e257e9a` + fresh-graphs + pdb-ida merges).
- Compiler: VC7 13.00.9254 (XDK), repository flags; no flag, header, `config/symbols.json`
  or `config/parked.json` change.
- Evidence used: January object code/relocations; HCEA
  `src/blam/interface/{ui_widget_load_children_recursive,widget_instance_text_box_is_focused,
  widget_instance_process_one_event_recursive,draw_bitmap_in_rect,ui_widget_launch_widget,search_and_replace,
  widget_instance_render_column_list}.c` and full-blobs `render_ui_widgets_postgame.c`; Stian `ui_widget.c`
  (render_ui_widgets) for semantics only. The prepared January atlas and Ghidra outputs do **not** exist for
  this object (`scratch/atlas`, `scratch/ghidra/out` cover other objects only); the PDB has no public name
  inside `_ui_widget_globals_storage`.
- Prior ledgers read in full: `ui_widget_obj_opus5_house_clean_20260913.md`,
  `claude_ui_weather_aim_render_hs_reconciliation_20260908.md`,
  `ui_widget_obj_bitmap_reconstruction_20260902.md`, `ui_widget_obj_opus_pool_owner_reconciliation_20260904.md`.
  `branch_sweep.py` found no blob better than lane HEAD. No parked entry exists for this unit.
- Per-function worker blocks: `scratch/workers/ui_widget.md`.

## 2. Validated baseline and final

`python -B tools/campaign/gate.py source/interface/ui_widget --all --forbid-emitted-symbol _point_from_line3d`

| state | exact | residual | unwritten |
|---|---:|---:|---:|
| baseline (lane HEAD) | 89 | 13 | 0 |
| final | **90** | 12 | 0 |

Row-by-row gate diff: only `_ui_widget_load_children_recursive` changed (residual -> EXACT).
Data sections untouched.

Residuals at final (target vs candidate: size / relocs / normalized sha16):

| function | target | candidate | first divergence / class |
|---|---|---|---|
| `_display_error` | 896/70/10e4892e | 896/70/a5f8fc97 | ebx/edi swap of local_player_index vs widget_name; case-3 cross-jump layout |
| `_display_scenario_help` | 608/50/f1ee9996 | 608/50/14f3291f | callee-saved register of `string_index` (ebx vs edi) only |
| `_draw_bitmap_in_rect` | 1168/36/40224da2 | 1168/36/2f729aa9 | plasma block: hoisted address stores vs `map1_offset.y` x87 interleave |
| `_render_ui_widgets` | 560/28/69042d15 | 560/29/4871b941 | IV strength reduction + LFTR; vk-render block order |
| `_render_ui_widgets_postgame` | 384/6/07984422 | 384/7/63e26020 | IV strength reduction + LFTR only |
| `_search_and_replace` | 464/16/930c988d | 464/16/09e4a925 | zero pinning in prologue, lazy pushes, exit specialisation |
| `_ui_check_for_pause_game` | 672/50/9681ee2a | 640/47/7e8b117d | January keeps five `_error` tails un-cross-jumped |
| `_ui_widget_launch_widget` | 304/20/12b6096b | 304/20/d7bf791d | widget load position; parent-tag if/else pins -1 in ours |
| `_widget_instance_process_one_event_recursive` | 2016/66/b4a92b7c | 1984/65/b097358c | jump threading of `event_handled`, cross-jumped tab tails, widget home vs edi, dpad index displacement |
| `_widget_instance_render_column_list` | 176/2/3ce0dd36 | 160/2/84ece93d | private register ABI (mutual recursion with render_recursive) |
| `_widget_instance_render_recursive` | 752/27/3be047db | 752/27/ac995343 | frame-slot map; column_list call ABI |
| `_widget_instance_text_box_is_focused` | 64/0/e1af5f9c | 64/0/3656c22a | list-type arm order |

## 3. Accepted control (landed, strict EXACT)

| function | padded | relocs | normalized sha (target = final) |
|---|---:|---:|---|
| `_ui_widget_load_children_recursive` | 672 | 23 | `fb981d3a0eec9022` |

January (0x21e-0x23f): `cmp type,2; je Z; cmp type,3; je Z; test pass_unhandled; je EXIT; jmp LOOP; Z: zero list
parameters; LOOP: child search`. The not-pass path goes to the final common epilogue, so there is no early
`return`. HCEA reconstructs the same decision as a `wants_focus` flag. Landed source:

```c
boolean focus_a_child = FALSE;

if (widget->type == _ui_widget_type_spinner_list ||
	widget->type == _ui_widget_type_column_list)
{
	widget->parameters.list.selected_index = 0;
	widget->parameters.list.last_list_tab_direction = 0;
	focus_a_child = TRUE;
}
else if (TEST_FLAG(definition->flags, _widget_pass_unhandled_events_to_children_bit))
{
	focus_a_child = TRUE;
}
if (focus_a_child)
{
	struct widget_instance *child;
	for (...) { ... }
}
```

Behaviour is unchanged: list widgets always focus a child, and other widgets do so only if they pass unhandled
events to children. The prior ledger reported "single-exit focus_a_child flag: byte-identical". That form kept
the negative `else if (!pass)` test. What matters here is the positive `else if (pass) focus_a_child = TRUE` arm,
plus scoping the child loop inside `if (focus_a_child)`. Checks passed: whole-TU gate (no exact row lost), the
point guard, the owner census (identical to `build/base`, no COMMON), fake scan (0 leads) and park drift (no
parks).

## 4. Experiment matrix (all reverted unless stated)

| function | shape | result |
|---|---|---|
| load_children_recursive | `if (list \|\| pass) { if (list) zero; loop }` | 233 insns, inner re-test half-threaded |
|  | HCEA flag, positive else-if arm, loop inside `if (flag)` | **EXACT, landed** |
| process_one_event_recursive | 1-D `dpad_event_times[16]` + linear index | -8 still folded, unchanged |
|  | `(unsigned long)` button index | unchanged |
|  | probe: `dpad_event_times` as its own symbol | both sites become January's `lea [..*4-8]` + symbol:0; proves separate BSS owner (not landable, see proposal) |
|  | drop redundant `!event_handled` from children-updown block | byte-identical |
| draw_bitmap_in_rect | `map_offset[i]` right after `map[i]` (both planes) | diff 16 -> 25 |
|  | `map_offset[1]` right after `map[1]` | diff 21; showed early pointer stores follow source order |
|  | `-f()*201.0f` single statement | 1152 [size] |
|  | `-(f()*201.0f)` | fmul before fchs, diff 19 |
| text_box_is_focused | HCEA ternary init + do-while + if/else arms | diff 6 -> 3, only list-type arms swapped (saved `scratch/workers/ui_widget/BEST_text_box_is_focused_t1.c`) |
|  | same with `?:`, inverted `!= &&`, `\|\|` expression | identical to previous row |
|  | same with two-case switch | TRUE-first but movsx range check, 31 insns |
|  | `if (==spinner) T; else if (==column) T; else F` | diff 5 |
| render_ui_widgets_postgame | `short` IV | frame 0x50, SR kept + second IV |
|  | `!widget_index` | identical |
|  | probe: `widget_globals` as own symbol | still strength-reduced |
| launch_widget | if/else parent tag with `parent` local | 320 [size], -1 pinned |
|  | no `parent` local + if/else | 320 |
|  | no `parent` local + `?:` | 320 |
| search_and_replace | HCEA guard `if (!string \|\| !*string) return replacements;` | 480 [size] |
|  | same with `return 0;` | 480 |
| display_error | case 3 `if (first) half else qtr; break;` | 912/71 (matches prior ledger) |
| render_recursive | `fast_ftol(alpha * 255.0f)` | diff 62 -> 61, same slot map |
|  | `real alpha` declared after `clipped` | diff 64 |

## 5. Do-not-repeat

- Everything in the prior ledger's section 5, plus the rows above.
- `dpad_event_times` index spellings: the missing `-8` displacement is an ownership fact, not an expression spelling.
- IV strength reduction in the render loops: not caused by storage ownership, `short` IV or `!index`.
- text_box_is_focused: `||`, `?:`, inverted `&&` and if/else all compile to the same arm order.

## 6. Residual classification (measured vs inferred)

- Measured: `dpad_event_times` is a separate January BSS owner at storage+0x870. VC7 will not fold a negative
  displacement below its own symbol, and the probe reproduces January's instruction exactly.
- Measured: load_children_recursive January control flow = flag form (landed).
- Inferred (tie classes): display_scenario_help / display_error / launch_widget / search_and_replace are
  register or constant-pinning ties. draw_bitmap_in_rect and render_recursive are store/slot scheduling.
  The two render loops are IV-SR selection. pause_game and process_one_event are cross-jump/threading.
  render_column_list is a private-ABI decision driven by the compile order of a recursion cycle.

## 7. Reopen criteria

- process_one_event_recursive: land the `dpad_event_times` owner split below, then find a source feature
  that blocks threading of `event_handled` between the two children-tab blocks.
- render loops: a feature suppressing IV strength reduction for a `long` 0..3 index used in `== 0`.
- text_box_is_focused: a TRUE-first arm form without a range check (start from the saved best).
- render_column_list/render_recursive: evidence of VC7's recursion-cycle compile order.
- pause_game: a lever against cross-jumping identical `_error` call tails.

## 8. ORCHESTRATOR-PROPOSAL

Split `_ui_widget_globals_storage` (`config/symbols.json` file_offset 4538944) so that +0x870 (16 x
`unsigned long`, 0x40 bytes) is its own static BSS owner. The .c would then define
`static unsigned long dpad_event_times[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS][NUMBER_OF_DPAD_DIRECTIONS];` in place
of the `#define dpad_event_times ui_widget_globals_storage.dpad_event_times` alias and the struct tail member.
Evidence: January `lea eax,[edx+ecx*4-8]` / `[eax*4 + .bss:2160]` at 0x94 and 0x738, reproduced only when the
array is its own symbol. Name: reconstruction name; no PDB/HCEA name. Consumers: `source/interface/ui_widget.c`
only. Measure with `static` on both sides (object-size/linkage law). Unlocks 2 instructions; it is not
sufficient for exact on its own.

## 9. Disposition

One function newly strict EXACT and eligible for credit (`_ui_widget_load_children_recursive`,
672 padded bytes). The 12 residuals stay active, not parked, with no fuzzy credit.
