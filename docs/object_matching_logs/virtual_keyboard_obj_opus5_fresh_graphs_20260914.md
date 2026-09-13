# `virtual_keyboard.obj` renderer reconstruction — opus5 fresh-graphs lane, 2026-09-14

## 1. Scope and provenance

- Translation unit `source/interface/virtual_keyboard.c`, January target
  `build/split/source/interface/virtual_keyboard.obj`.
- Compiler: VC7 13.00.9254 (`xbox/bin/vc7/CL.Exe`), unit flags from
  `build.ninja`: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` plus the standard include
  list. No flag was changed.
- Lane `opus/30k-fresh-graphs-20260914`, pinned base `c3e257e9a` plus
  orchestrator commit `47d9dd0da`.
- Evidence used, in authority order: the January COFF bytes and relocations
  (full Capstone listing kept in `scratch/workers/virtual_keyboard/target_render.txt`),
  the Ghidra 12.1.2 decompilation (`scratch/ghidra/out/virtual_keyboard.obj.decomp.c`,
  lines 319-559), the January map atlas, `config/symbols.json` (static
  `_virtual_keyboard_render_internal`, static `_keyboard_rect` at file offset
  3171128), and the January assertion literals. HCEA's
  `virtual_keyboard_render_internal.c` and `virtual_keyboard_render_text_box.c`
  were read for names only. They describe a later build that splits a text-box
  helper out and has no key grid, so they were not used as a donor.

## 2. Validated baseline

| State | Exact / residual / unwritten | Exact padded bytes |
| --- | --- | --- |
| Lane HEAD | 18 / 1 / 1 | 2,400 of 5,104 |
| After this lane | 18 / 2 / 0 | 2,400 of 5,104 |

| Function | January size / relocs / normalized SHA-256 | Candidate | Status |
| --- | --- | --- | --- |
| `_virtual_keyboard_render_internal` | 1616 / 95 / `f52389e69e9dbb4f8cbeaf41e8ad788704fec98dc5fbff916156df5285960425` | 1584 / 95 / `233bda966b7753db3fb797c9633098487a9b22c678679f43f81d223eb3d89e9d` | fuzzy landed, zero credit |
| `_virtual_keyboard_select` | 1088 / 123 / `28fa08bf…e141` | 1088 / 123 / `025bd53f…65d9` (unchanged) | parked, untouched |

The candidate matches the target from the prologue up to the `or edi,0xffffffff`
at January offset `0x3e`, which is the first meaningful difference. The
`sub esp,0x74` frame and the ebp slot set are the same on both sides.

## 3. Accepted controls (retained in production)

- **Data:** `static rectangle2d keyboard_rect[NUMBER_OF_VIRTUAL_KEYS]` holds
  January's 44 `{y0,x0,y1,x1}` rectangles, decoded byte-for-byte from `.data`
  section 35. Rows 0-35 are the configurable keys and 36-43 are
  done/shift/caps/symbols/backspace/left/right/space.
- **Tag layout:** `virtual_keyboard_definition` is now `font_tag`,
  `background_bitmap_tag`, `special_key_labels_string_list_tag`, then `keys`.
  `virtual_keyboard_key` ends with four bitmap tag references: unselected,
  selected, active and sticky. The `font_tag` spelling comes from January's
  assert string. Offsets 0x0c, 0x1c, 0x2c, 0x34 and key +0x1c/+0x2c/+0x3c/+0x4c
  are the ones January dereferences.
- **Block scoping:** the background, caption, text-box, caret and key blocks each
  own their locals. With every local at function scope the frame was 0x80.
  Block scoping reproduces January's 0x74 frame, where the rectangles and dword
  homes overlap across sibling blocks.
- **Colors:** `text_color` fields are assigned and then
  `caption_color = text_color;`. This gives January's store/reload pattern
  (`mov [ebp-0x34],1.0; mov eax,[ebp-0x34]; mov [ebp-0x44],eax`). A chained
  scalar assignment folds to immediates instead.
- **Loops:**
  - The second loop continues the same `key_index`
    (`for (; key_index < NUMBER_OF_VIRTUAL_KEYS; key_index++)`). January keeps
    the `cmp esi,0x2c` pre-test, so the counter is not reinitialised.
  - The switch is on `key_index`: an 8-entry table starting at `_vkey_done`,
    with done, backspace, left, right and space sharing one arm.
  - `bitmap_index = NONE` before the switch lets the `default` assert skip the
    bitmap fetch, as January's `jmp 0x5a3` does.
- **Caret walk:** the loop tests the pointer (`while (character)`), not the
  character. January's `add ebx,2; jne` proves this. The walk ends through the
  `font_get_character_by_ascii_code` NULL break when it reaches the terminator.
  A source comment records this.
- **Private ABI:** `virtual_keyboard_get_character((word)key_index)` reproduces
  January's `esi`-register call. The casts compile to the same code as the
  uncast form but avoid two C4244 warnings.
- **Linkage:** the prototype and definition are now `static`, matching
  symbols.json. `_virtual_keyboard_render` stays exact.
- **Local type:** `struct font_character` is defined locally with the same
  layout already repeated in `draw_string.c` and `rasterizer_text.c`, because
  `font_group.h` only forward-declares it. See proposal A below.

## 4. Experiment matrix

| Shape | Size / relocs | Result |
| --- | --- | --- |
| v1 all locals at function scope | 1600 / 95, frame 0x80 | reverted |
| v2 block scoping, caret rectangle nested in `if (bitmap)` | 1584 / 95, frame 0x70 | reverted |
| v3 caret rectangle at caret-block scope | 1584 / 95, frame 0x74, slot set equal | kept |
| v4a `caption_color = text_color` struct copy | 1584 / 95, colour prologue equal | kept |
| v5t switch arms as ternaries | byte-identical to v4a | inert |
| e1 `key_index` at function scope | byte-identical | inert |
| e3 no `key` pointer, `keys[key_index].field` | one more diff block | reverted |
| c1 caret stores ordered x0, x1, y0, y1 | 1584 / 95, 4 fewer differing lines | **landed** |
| c3 caret `bitmap` declared before `height` | slot census identical | inert |
| s2 case order shift/caps/symbols before the done group | worse | reverted |
| v4b argb `NONE` instead of `0xffffffff` | identical | inert |
| hyp `long style` in an out-of-tree `draw_string.h` copy | identical | inert; no header proposal |

All 18 exact siblings stayed exact in every compile. `_virtual_keyboard_select`
kept 1088 / 123 / `025bd53f…` each time.

## 5. Do-not-repeat list

- Do not re-adapt the HCEA 360 renderer. It is a different program shape.
- Chained colour assignment, ternary switch arms, `key_index` scope, argb
  spelling, caret declaration order and the `style` parameter type are all
  measured inert.
- Do not reinitialise `key_index` for the second loop. January keeps the
  runtime pre-test.
- Do not use a keepalive, forced inlining, or register/volatile steering to buy
  the `edi` pin.

## 6. Residual classification

1. **Ownership/relocation (measured fact):** January's loop-2 bound relocation
   is spelled `_item_update_section+0`. That is one past the end of the 352-byte
   `_keyboard_rect` (3171128 + 352 = 3171480, the next symbols.json entry). VC7
   necessarily emits `_keyboard_rect+352`. This is the known
   `csplit-relocation-alias` class, so strict EXACT cannot be reached from C.
2. **ABI/register (measured fact, cause unknown):** January keeps -1 in `edi`
   from the prologue to the second `draw_string_set_draw_mode` and spends it 7
   times. Our build materialises immediates. This belongs to the same family as
   the recorded zero-register pinning residual.
3. **Block placement / tail merge (measured fact, inference on cause):** we
   merge the three sticky-bitmap arms and place the caps/symbols arms after
   `ret`. January keeps three `mov eax,[esi+0x10]; jmp` arms. The key-pointer IV
   base is +0x3c in January and +0x2c in ours, consistent with January still
   having three sticky loads at IV-selection time. This accounts for the 32-byte
   size gap.
4. **Scheduling:** January reloads `width` on the caret-walk skip edge
   (`xor esi,esi; mov si,[ebp-0xc]`), and the height and caret-bitmap homes are
   swapped (-0x10 and -0x18).

The relocation multiset matches everywhere except the alias above, the
January-undefined `bitmap` literal COMDAT, the jump-table internal offset, and
the unit-wide COMMON versus `.bss` spelling of `virtual_keyboard_globals`.

## 7. Reopen criteria

Reopen only for one of these:

- a demonstrated source handle that makes VC7 pin -1 in a callee-saved register;
- a demonstrated source handle that stops the sticky-arm tail merge;
- a comparator or csplit change that credits resolved-address-equal
  one-past-the-end relocations.

Without the third, the best possible outcome is a `csplit-relocation-alias` park.

## 8. Disposition

`_virtual_keyboard_render_internal` is a complete, house-rule-reviewed ordinary-C
body landed as a zero-credit fuzzy residual. It is not strict exact.
`_virtual_keyboard_select` remains parked and untouched.

Orchestrator proposals:

- **A.** Move the complete `struct font_character` into `source/text/font_group.h`
  and delete the three `.c`-local copies. Consumers:
  - `game/game_engine.c`
  - `interface/hud_messaging.c`
  - `interface/terminal.c`
  - `interface/virtual_keyboard.c`
  - `main/main.c`
  - `rasterizer/rasterizer_text.c`
  - `text/draw_string.c`
  - `text/font_group.c`
- **B.** If the build requires it, add a `config/parked.json` entry for
  `_virtual_keyboard_render_internal`:
  - target 1616 / 95 / `f52389e6…0425`
  - base 1584 / 95 / `233bda96…9e9d`

Final checks:

- The emitted-symbol guard for `_point_from_line3d` passes.
- Parked-sibling drift passes.
- The owner census adds no candidate-only owner except the January-undefined
  `bitmap` literal.
- `tools/fake_match_scan.py` reports 0 leads.
- `CL /Zs /W3` adds no warnings.
