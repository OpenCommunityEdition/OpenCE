# `ui_widget.obj`: Opus5 150K house-clean lane, wave w3 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/interface/ui_widget.c`; target `build/split/source/interface/ui_widget.obj`.
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, wave w3 (second pass).
- Compiler VC7 13.00.9254 (XDK 3911), repository flags. No source, header, `config/*.json` or flag change was
  made; the real file is byte-for-byte unchanged (`git diff --stat -- source/interface/ui_widget.c` is empty).
- Evidence: January object code, relocations and layout; `scratch/ghidra/out/ui_widget.obj.decomp.c`;
  `scratch/atlas/ui_widget.obj.txt`; PDB publics; HCEA `src/blam/interface/*.c` (semantics only).
- Prior work read: every `ui_widget_obj_*` ledger (in particular `_opus5_150k_w1_`, `_opus5_100k_`,
  `_opus5_house_clean_`), `scratch/workers/ui_widget.md` (w1 notes), `scratch/w2/laws_w2.md`, the C4013/W3
  census (`scratch/w3/c4013.txt`, `scratch/w3/warn/source_interface_ui_widget.txt`: no C4013/C4133 in this TU).
- A previous w3 run was cut off without report blocks; its scratch probes (pe1-pe3, pg1/pgA/pgq1, deA, lwA, st1,
  probe1) were recovered, re-measured where needed and are recorded below.
- Per-function worker blocks: `scratch/workers/w3_ui_widget.md`. Scratch candidates: `scratch/workers/w3_ui_widget/`.
- No parked entry exists for this unit. Excluded (do-not-spend): `_render_ui_widgets`, `_render_ui_widgets_postgame`.

## 2. Baseline and final

`python -B tools/campaign/gate.py source/interface/ui_widget --all --forbid-emitted-symbol _point_from_line3d`

| state | exact | residual | unwritten |
|---|---:|---:|---:|
| baseline | 91 | 11 | 0 |
| final (real file unchanged) | 91 | 11 | 0 |

Row-by-row diff: identical. Guard passed. Park drift: no parks. Owner census (final.obj vs `build/base`): no
NEW/LOST owners, no COMMON. Fake scan: 0 leads.

## 3. Law detector sweep (w2 laws A1-A22 over the 11 residuals)

| law | result |
|---|---|
| A1 static linkage | `ui_widget_globals_storage` extern->static (st1): every function byte-identical. `ui_plasma_effect_color` has a PDB public. No candidate. |
| A3 single exit | display_error: success-flag instead of the early `return` (dz1) byte-identical; search_and_replace nested guards (sr3a) byte-identical. |
| A5 alias removal | render_recursive: nesting `bitmap_group_get` into the element call (rr3a) is refuted by January's push order (0x40 pushed after tag_get). |
| A6 frame layout | render_recursive: slot census recorded (sec. 4); no scope/type defect found. |
| A7 prototypes | no C4013/C4133 in this TU; C4244 long->short only at `ui_play_audio_feedback_sound(audio_feedback)` (stores are dword in January, so `long` is right). |
| A10 conditional initializer | text_box_is_focused `focused = TRUE` initializer (tb3a) worse; display_error NONE initializer placement is real (de2, below). |
| A17 bare validation call | render_recursive `sequence` local -> bare `TAG_BLOCK_GET_ELEMENT` statement (rr3b) byte-inert. |
| A18 value facts | process_one_event close-loop IV is `long` (January `inc esi; cmp esi,4`); PIN (not MIN/MAX) in display_error proven by branch shape (de9, dz2). |
| A20 declaration-order stores | draw_bitmap_in_rect non-address stores already in field order; residual is `lea` hoisting. |
| others (A2, A4, A8, A9, A11-A16, A19, A21) | no match in the residual rows. |

## 4. Per-function outcomes

### `_display_error` (896): structural gap closed in scratch, not landed

January facts read from the bytes:

1. `error_local_player_index = NONE` is materialised inside `if (local_player_index != NONE)` (0xa5), and the NONE
   path jumps straight to the main-menu test (0xdb).
2. The switch default (assert + epilogue) stays in source position after case 4 (0x148). VC7 moves a
   `return;` default to the end of the function (measured, de2 vs de5).
3. Case 3 hoists `mov al,[modal]` and jumps into both case 2's and case 4's ternary tests (0x121-0x12a): two
   cross-jumped duplicates, not a fallthrough.
4. Colouring: widget_name/error index in edi, local_player_index in ebx.

Shapes (scratch, starting from the house_clean BEST = PIN + `top_widget_tag_index`, 102 differing lines):

| shape | change | differing lines |
|---|---|---:|
| de1 | `widget_stack = local_player_index == NONE ? 0 : local_player_index;` + unconditional assert 2168 | 90 (registers now January's) |
| de2 | de1 + NONE assigned inside the if; test `(local_player_index == NONE \|\| error_local_player_index == NONE) && !we_are_at_the_main_menu` | 88 (prologue identical) |
| de3 | de2 + case 3 falls into case 2 | 88 |
| de4 | de2 + case 3 if/else | 912/71 [size] |
| de5 | de2 + default `widget_name = NULL; break;` + rest of the block inside `if (widget_name)` | 79 (default in place) |
| de6 | de5 + case 3 falls into case 2 | 78 |
| de7 | de5 with every ternary spelled if/else | byte-identical to de5 |
| **de8** | **de5 + case 3 `if (first_local_player == TRUE) half; else qtr; break;`** | **37: whole switch/join/default layout identical** |
| dr1/dr2/dr3 | de8 reverting widget_stack / NONE placement / top_widget_tag_index | 57 / 39 / 47 (all three changes needed) |
| dz1 | de8 + success flag instead of early `return` | byte-identical |
| de9, dz2 | de8 with `MIN(MAX(0,e),39)` / `MIN(MAX(e,0),39)` | 43 / 43 (PIN proven) |
| dz3, dz4 | de8 with explicit `== FALSE` compares | byte-identical |

The 37 remaining lines are one mechanism. January pins constant 0 in ecx from the PIN compare to the final
auto-close stores:

- `xor ecx,ecx; cmp bx,cx`;
- `cmp word [pause_game_time_count],cx`;
- a re-xor at 0x33c after the calls;
- `mov [esi+0x1c],ecx; mov [esi+0x20],ecx`.

Ours uses `test bx,bx`, an immediate compare and `xor eax,eax`. Every other function stays byte-identical in de8;
fake scan 0 leads; census clean. Best candidate: `scratch/workers/w3_ui_widget/BEST_display_error_de8.c`
(+ `.diff` against the real file). Not landed: the function is not strict exact.

### `_widget_instance_process_one_event_recursive` (2016): not landed

- pe1, from the previous w3 run: `long controller_index` in the close-if-controller loop, as January's 32-bit
  compare proves. Size 1968.
- pe2: pe1 plus the button match as an if. Size 1968.
- pe3: pe1 plus every handler match spelled `if (cond) handler_matches = TRUE;`. Size 1952. This reproduces
  January's flag join in the dispatch loop (`cmp; jmp J; J: jne skip`, no `sete`); only the handler_index home
  differs there.
- pe4, this run: removing the redundant `!event_handled` from the children-updown condition is byte-identical.

Remaining, analysed but no shape spent:

- January does not thread `event_handled` at the leftright entry (0x319).
- January keeps `audio_feedback` memory-only.
- January registers `definition` in esi in region 1.
- January reloads `widget` into edi around every tab call.

Ours threads the tab tails, cross-jumps them and pins zero in esi.

### `_widget_instance_render_recursive` (752): not landed

January frame:

- `alpha_modifier` and the ftol temp share [-4];
- `bounds` at [-0xc];
- `clipped` at [-0x14];
- `alpha` at [-0x18];
- `bitmap` at [-0x1c];
- `clip` lives in the widget parameter home [ebp+8].

Ours puts `alpha` in [ebp+8] and registerises the `clip` phi.

Shapes:

- rr3a (nested accessor) is refuted by push order.
- rr3b (bare A17 statement instead of the never-read `sequence` local) is byte-inert. Use that form whenever the
  function is next landed.

### Other residuals

- `_widget_instance_text_box_is_focused`: tb3a (`focused = TRUE` initializer) worse.
- `_search_and_replace`: sr3a nested guards inert. It is the same zero-pin class as display_error, with
  replacements' 0 held in eax at entry.
- `_ui_check_for_pause_game`: only the esi/edi swap is left (w1 `BEST_pause_game_p1.c`, 672/50). No new evidence,
  so it was not spent.
- `_ui_widget_launch_widget`: only the -1 constant pin in the parent-tag diamond is left, after the definition
  move. Budget exhausted; not spent.
- `_display_scenario_help`: an ebx/edi tie with identical instruction streams; not spent.
- `_draw_bitmap_in_rect`: January stores `&map1_offset` only after the `map1_offset.y` fstp; ours hoists that
  `lea`. Backend store scheduling; not spent.
- `_render_ui_widgets`, `_render_ui_widgets_postgame`: excluded.

## 5. Measured laws for the brief (orchestrator proposals, no config change)

1. **Default-return placement.** A `return;` inside a switch `default:` is moved to the end of the function.
   A `default: x = NULL; break;` whose later `if (x)` guard is threaded keeps the default block in source
   position. Once that block stays, case-body layout follows source order, and the cross-jump survivor follows
   layout (display_error de2 -> de5 -> de8).
2. **Duplicate versus fallthrough.** A case that contains full if/else copies of both neighbours' selections is
   emitted as a hoisted head load plus jumps into both neighbours' tests (January display_error 0x121). A
   fallthrough into the next case does not produce this (de5/de6 vs de8).
3. **Pre-fold uses drive colouring.** `v = c ? 0 : p; assert(range(v));` and `if (c) v = 0; else { v = p; assert; }`
   fold to the same CFG, but the ternary form changed callee-saved colouring of `p` to January's (dr1 57 vs
   de8 37).
4. **Initializer placement.** An in-branch assignment moves a constant materialisation (`or edi,-1`) into that
   branch, while a declare+init materialises it at block top. Admissible only when the guard keeps every read
   defined (de2).

## 6. Reopen criteria

- **display_error**: start from `BEST_display_error_de8.c`. It needs a lever that makes VC7 hold constant 0 in
  ecx across PIN -> pause assert -> auto-close stores. Before landing, a credibility review of the
  `if (widget_name)` wrapper with the NULL default, and of the `(local_player_index == NONE || ...)` guard.
- **process_one_event_recursive**: start from pe3's dispatch-loop if-form plus the long close-loop IV. It needs a
  lever against leftright threading of `event_handled`.
- **search_and_replace**: the same zero-web lever as display_error, with eax as the register.
- **render_recursive**: a January-evidenced spelling that keeps `clip` in memory at the join.
- **pause_game, launch_widget, display_scenario_help, text_box_is_focused, draw_bitmap_in_rect**: as recorded
  in the w1 and house_clean ledgers.
- The `dpad_event_times` .bss split proposal remains held under the owner ruling on invented .bss split names;
  not re-proposed.

## 7. Disposition

Nothing landed; no newly exact functions. All 11 residuals stay active, with no parks and no fuzzy credit.
