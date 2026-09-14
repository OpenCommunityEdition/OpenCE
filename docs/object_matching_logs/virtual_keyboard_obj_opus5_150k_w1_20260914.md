# `virtual_keyboard.obj` — opus5 150K house-clean lane, wave w1, 2026-09-14

## 1. Result

No source change landed. `source/interface/virtual_keyboard.c` is byte-identical to the lane
base (`12f7375d4`); `git diff --stat` is empty.

| Gate | Exact / residual / unwritten |
| --- | --- |
| Baseline (real file) | 18 / 2 / 0 |
| Final (real file) | 18 / 2 / 0 |

Checks on the final real file: `_point_from_line3d` guard passes, parked-sibling drift 0
(`_virtual_keyboard_select` 1088 / 123 unchanged), fake-match scan 0 leads. There is no owner
census delta because the source did not change.

| Function | January padded / relocs / sha16 | Real file | Outcome |
| --- | --- | --- | --- |
| `_virtual_keyboard_render_internal` | 1616 / 95 / `f52389e69e9dbb4f` | 1584 / 95 / `233bda966b7753db` | not landed (residual) |
| `_virtual_keyboard_select` (parked) | 1088 / 123 / `28fa08bf4fb555466` | 1088 / 123 / `025bd53f5f2169bb` | not landed (park kept) |

Evidence came from four sources:
- the January COFF bytes and relocations;
- the Ghidra decompilation (`scratch/ghidra/out/virtual_keyboard.obj.decomp.c`, lines 470-560);
- the four earlier ledgers for this object;
- the fresh-graphs do-not-repeat list.

`branch_sweep` found no blob where either function is exact.

## 2. `_virtual_keyboard_render_internal`

### 2.1 Answer to the orchestrator question

This is **not** only the relocation alias. The loop-2 end bound is spelled `_item_update_section+0` in
January. That address is one past the 352-byte `_keyboard_rect` (3171128 + 352 = 3171480), so this is
the `csplit-relocation-alias` class. The current body also differs in real code:
- 32 bytes of size (1584 against 1616);
- about 229 aligned instructions in an alias-normalized diff.

### 2.2 New measured lever: where the bitmap fetch sits

January's loop 2 has these features:
- three separate sticky arms, each `mov eax,[esi+0x10]; jmp`;
- one shared fetch block that stores into `edi` and sits before the default assert;
- the per-key bitmap held in callee-saved `edi`.

What moves this:
- **Fetch after the switch.** Every spelling measured 1584 bytes: any branch polarity, ternaries, or
  case order. VC7 cross-jumps the three sticky tails into one.
- **Fetch inside each case.** `if (bitmap_index != NONE) bitmap = bitmap_group_get_bitmap_from_sequence(...)`
  followed by `break;` gives January's 1616-byte envelope. The sticky arms stay unmerged, the bitmap moves
  to a callee-saved register, and the cases are laid out in source order. Scratch body:
  `scratch/workers/virtual_keyboard/v1.c` (1616 / 95 / `9a367fc92361d9dc`). All 18 siblings stay exact
  and the park does not drift.
- **Closest switch structure.** The done-group arm fetches per leaf and the shift/caps/symbols arms test
  `if (layout != key_index)` first. Scratch body: `scratch/workers/virtual_keyboard/s3/E001_y1000.c`
  (1600 / 95 / `a67d43dc445053cd`). Its switch region is structurally identical to January's except for
  the key-pointer IV base.

This meets the earlier ledger's reopen criterion "a demonstrated source handle that stops the sticky-arm
tail merge". It does not reach exactness on its own.

### 2.3 New measured VC7 law: key-pointer IV base

In a switch where every arm reads different `key->field` offsets, one IV is created and its displacement
is fixed. Across more than 200 generated probes, only the **source-last arm** decides that displacement:
- it is the leaf reached by the conditional-jump target of that arm's first inner `if`;
- `if (!flag)` is canonicalised to `if (flag)` first;
- the form of the other arms is inert.

Measured on the symbols arm:

| Symbols-arm form | IV base |
| --- | --- |
| `layout ==`, then `last_event == select` | +0x2c |
| `layout ==`, then `last_event != select` | +0x3c |
| `layout !=` (any inner form) | +0x1c |

January uses +0x3c. That needs the `!=` select form, and that form breaks the arm merge layout January shows.
No single natural spelling satisfied both constraints.

### 2.4 Rejected or inert shapes

- Fetch after the switch, in all polarity, ternary and order combinations: 1584, sticky arms merged.
- Splitting the done group into two identical case arms: inert.
- `keys[key_index].field` with no `key` local: inert.
- Loop-1 `key_bounds` declared at loop scope: byte-identical.
- Mixed per-leaf and per-arm fetch forms other than §2.2: structurally worse. The generator is
  `scratch/workers/virtual_keyboard/gen2.py` / `gen3.py`, and scores are in `s/` and `s3/`.

### 2.5 Residual classification (after the §2.2 lever)

1. Key-IV base selection (+0x1c against +0x3c), which follows the IR order of the source-last arm.
2. Loop-2 register rotation (rect/key/bitmap), label home `-0xc` against `-0x18`, the switch-index copy
   `mov ecx,eax`, and loop rotation.
3. The `edi = -1` pin from the prologue to the second `draw_string_set_draw_mode`. This belongs to the
   zero-register pinning family.
4. Caret block: `width` reload on the pre-test edge, and the height/bitmap homes swapped.
5. Loop 1: font-header reload, and the order of the rect copy against argument pushes.
6. The `_item_update_section` relocation alias, which cannot be repaired from `config/`.

### 2.6 Reopen criterion

Reopen for either of these:
- a natural switch spelling that gives January's +0x3c key IV while keeping the per-arm-fetch merge layout
  (start from `v1.c` or `s3/E001_y1000.c`);
- a demonstrated handle for the `edi = -1` pin.

Even with a perfect body, strict credit is impossible. The best outcome is a `csplit-relocation-alias`
park, which requires equal size, normalized bytes and resolved relocations.

## 3. `_virtual_keyboard_select` (parked; scratch only)

An alias-normalized aligned diff leaves only two real differences.

1. **Default-arm memmove size association.** January emits `sub esi,eax; lea edx,[esi+ecx-2]`; ours
   emits `sub ecx,eax; lea edx,[ecx+esi-2]`. Declaring the local as `word buffer_size` fixes it. That
   matches the `word buffer_size` field it copies, which makes it the natural type. Scratch body:
   `scratch/workers/virtual_keyboard/sa.c` (1088 / 123 / `ff65b27bb63875dc`). Inert alternatives:
   - an `unsigned long` local;
   - a reordered size expression;
   - a `sizeof(wchar_t) *` free-space spelling.
2. **Left/right arm shared-tail placement.** The shared tail is the cursor store plus
   `ui_play_audio_feedback_sound(_ui_audio_feedback_cursor)`. January keeps it in the left arm and
   branches the right arm back to it; VC7 here keeps it in the right arm. All of these were inert:
   - `!= L'\0'`;
   - a reversed pointer compare;
   - braces;
   - swapping the source order of the two cases (emission follows case value);
   - all 16 pre/post/compound increment and decrement spellings.

The park is not reopened: the function is still not exact. Its criterion is unchanged. Reopen together
with the `word buffer_size` lever once a source handle for the left-arm tail placement exists.

## 4. Proposals

None. No header or config change is needed, and no park reopen is proposed.
