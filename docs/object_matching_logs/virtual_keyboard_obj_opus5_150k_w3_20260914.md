# `virtual_keyboard.obj` — opus5 150K house-clean lane, wave w3 (2026-09-14)

## Baseline and result

The real file `source/interface/virtual_keyboard.c` at HEAD `403108e28` gates at **18 exact / 2 residual / 0 unwritten**, and the `_point_from_line3d` guard passes.

| function | January padded / relocs / sha16 | real file | outcome |
| --- | --- | --- | --- |
| `_virtual_keyboard_render_internal` | 1616 / 95 / `f52389e69e9dbb4f` | 1584 / 95 / `233bda966b7753db` | not landed; strict EXACT unreachable from C |
| `_virtual_keyboard_select` (parked) | 1088 / 123 / `28fa08bf4fb55546` | 1088 / 123 / `025bd53f5f2169bb` | not landed; park kept |

No source change landed, and `git diff --stat` is empty. Checks on the final real file:
- gate: 18 / 2 / 0;
- guard: passes;
- park drift: 0;
- owner census: 27 owners, the same as base (the three XDK D3D inline owners are pre-existing);
- fake-match scan: 0 leads.

Duplicate check:
- read ledgers fable_salvage_0902, jonas_one_shot_0828, jonas_freshness_0830, fresh_graphs_0914 and opus5_150k_w1_0914, plus the w1 notes `scratch/workers/virtual_keyboard.md`;
- `branch_sweep`: no blob is better (the best is `4d14975d`, 18 / 1 / 1);
- no w2 work exists on this object.

## `_virtual_keyboard_select` (parked; scratch only)

- **Starting point.** w1 `sa.c` uses `word buffer_size` for the default-arm memmove size and leaves 12 differing instructions (copy: `scratch/workers/w3_hud_messaging_virtual_keyboard/vs0.c`).
- **The remaining difference.** It is only where the shared left/right tail sits: `mov [cursor],eax; push 1; mov [first_key],bl; call ui_play_audio_feedback_sound`.
  - January keeps it in the **left** arm, and the right arm jumps **back** (`jmp 0x182`).
  - VC7 here keeps it after the right arm.
- **Law detectors.** None address cross-jump survivor choice:
  - A1: the virtual_keyboard globals are a measured no-effect control.
  - A3: there is already one `return TRUE`.
  - A9: the switch width is already equal.
  - A10, A13, A14 and A16 do not apply to the differing region.
- **Shape vs1, in the real TU.** Combined `case _vkey_left: case _vkey_right:` with `if (keycode == _vkey_left)`. There is no switch threading, it compiles to 291 instructions and is worse.
- **Lab probes.** A standalone TU with the same flags, in `scratch/workers/w3_hud_messaging_virtual_keyboard/lab/`:
  - `l2`: a replica that reproduces our layout.
  - `l3`: swapping the source order moves the layout to source order. In this lab TU, block layout follows source order, not case value (w1 reported case-value order in the real function; not re-measured there). The survivor is still the source-later arm, and the eax/ecx roles swap.
  - `l5`/`l6`: an extra leading call in either arm is inert.
  - `h1a`/`h1b`, `m2`/`m3`: IR-distinct but code-identical stores are inert.
  - `g1`: an explicit shared label reached by `goto` is byte-identical to `l2`.
- **Finding.** January's layout places the merged block directly after its first predecessor and branches backward from the second. That is not a reverse postorder of the merged CFG, which points to a late machine-level cross-jump over an unmerged layout. No legal source handle was found.
- **Reopen criterion.** The park criterion is unchanged. Reopen with `sa.c`'s `word buffer_size` plus a demonstrated source form whose arm tails stay distinct until after block layout.

## `_virtual_keyboard_render_internal` (not parked)

- **Alias confirmed.** January's loop-2 end bound `cmp ebx, _item_update_section+0` (relocation at +0x60c) is one past the 352-byte static `_keyboard_rect`: 3171128 + 352 = 3171480, which is the items.c profile section. VC7 necessarily emits `_keyboard_rect+352`, so every body stays `[reloc-identity]` at best.
- **No landing possible.** An already-written residual may land only when strict EXACT (brief §6), so this function cannot land. No shapes were spent.
- **A18 detector.** The `or edi,-1` pin is spent on assert NONE compares, `push edi` for `system_exit(-1)` and the `draw_string_set_draw_mode` style. edi is reused for the bitmap in between, so this is constant pinning, not a source variable.
- **A21.** The `wchar_t string[24] = {0}` block already matches.
- **Reopen criterion.** Comparator or owner credit for resolved-address-equal one-past-the-end relocations, then w1's criteria: a natural +0x3c key-IV spelling starting from `v1.c` / `s3/E001_y1000.c`, or a handle for the edi pin.

## Proposals

None for this object. The csplit-alias tooling item is recorded in the hud_messaging w3 ledger (P1). It would also be the prerequisite for any render_internal alias park.
