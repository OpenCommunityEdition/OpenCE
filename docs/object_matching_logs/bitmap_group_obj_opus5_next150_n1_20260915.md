# `bitmap_group.obj` fresh structural pass (Opus5 next-150K lane, wave n1, 2026-09-15)

## Result

`source/bitmaps/bitmap_group.c` is **unchanged**. `_bitmap_group_add_bitmap` is still a locked park and did not
become strict exact in scratch. There is no park-reopen proposal and no strict credit.

Baseline and final real-file gate, with the `_point_from_line3d` guard: **exact 5, residual 1, unwritten 0**, row
listings identical; `scratch/parkcheck.py` reports `parks 1 drift 0`; `tools/fake_match_scan.py` reports 0 leads;
`git diff --stat -- source/bitmaps/bitmap_group.c` is empty.

What this wave adds is a complete structural decode of the residual. Every earlier ledger described the gap as
"instruction schedule and control-flow layout" (the 2026-08 object ledger) or as "zero stores plus knock-on
differences" (the w2 ledger). Neither quantified it. It is now fully accounted for:

| Item | Bytes | Cause |
| --- | ---: | --- |
| two `xor edx,edx` (January +0x4d, +0x63) | 4 | the zero-pseudo pin (below) |
| `lea esp,[esp]` loop-alignment pad (January +0x229) | 7 | a CONSEQUENCE of the 4 bytes above: 0x225 + 4 = 0x229, and 0x229 + 7 = 0x230 is January's loop head |
| **total meaningful gap** | **11** | 995 (January, ends 0x3e3) vs 984 (ours, ends 0x3d8); padded 1008 vs 992 |

Nothing else is missing. The frame is identical, every one of the 59 relocations has the same type and target in
the same order, and the only remaining non-size difference is a size-neutral cold-block fall-through choice
(laws_w3 A43 class).

## Census

**Frame.** January `sub esp,0x34`; ours `sub esp,0x34`. Slot table, identical on both sides:

```
[ebp-0x34] signature   [ebp-0x30] width     [ebp-0x2e] height   [ebp-0x2c] depth
[ebp-0x2a] type        [ebp-0x28] format    [ebp-0x26] flags    [ebp-0x24] registration_point.x
[ebp-0x22] registration_point.y             [ebp-0x20] mipmap_count
[ebp-0x1c] pixels_offset                    [ebp-0x0c] hardware_format
[ebp-0x08] base_address
```

`struct bitmap_data new_bitmap_data` therefore occupies `[ebp-0x34, ebp-0x04)` (48 bytes) and `[ebp-0x04]` is the
one non-descriptor local: `pixels_end`, the spill home of EBX (January reloads it at +0x125 and accumulates into
EBX inside the loop at +0x33e).

Recycled parameter homes (laws_w3 A24 detector): January spills into `[ebp+0x0c]` (width), `[ebp+0x10]` (height)
and `[ebp+0x14]` (depth); ours into `[ebp+0x18]` (type), `[ebp+0x20]` (mipmap_count) and `[ebp+0x0c]`. This is a
consequence of which parameters the prologue preloads, which is itself a consequence of the zero-pseudo pin - it
is not an independent defect.

**Relocations.** 59 vs 59. `relocdiff --count-by-target --allow-structural` against the best scratch candidate
shows all 59 rows with identical type and identical target, in identical order; only the addresses differ, and
they differ by exactly three constants (-4 from +0xcd, -22 across the swapped cold block, -11 from +0x235 on).
The four diagnostics are emitted in January's order (cube, cube, `_0EA` three-argument "non-power-of-two",
`_0DN` two-argument "non power-of-two").

## Production defect found (relocation identity)

The **real, parked** body emits three `.rdata` assert-literal COMDATs January does not own, because its assert
expressions are written with spaces:

| real file today | January |
| --- | --- |
| `??_C@_0BD@POFKGBND@space_between?5?$DO?$DN?50` | `??_C@_0BB@PIAKKBNN@space_between?$DO?$DN0` |
| `??_C@_0EC@NCJMCPEA@?$CIbyte?5?$CK?$CJbitmap?9?$DObase_address?5?$DO?$DN?5` | `??_C@_0DO@OLLHEHHP@?$CIbyte?$CK?$CJbitmap?9?$DObase_address?$DO?$DN?$CIby` |
| `??_C@_0IA@IGIPKGHF@...` | `??_C@_0HO@CGGNING@...` |

Three of the shipped object's relocation targets are therefore wrong. The scratch candidate `ab_L5b.c` emits
exactly January's three. (The w2 ledger asserted this from the `.rdata` strings; it is now confirmed at the COFF
symbol level.)

## The residual, mechanised

January's prologue keeps EDX free and materialises the descriptor's zero constant in it twice:

```
35 mov dx,[ebp+0x18]   39 mov si,[ebp+0xc]   3d mov ax,[ebp+0x10]   41 mov cx,[ebp+0x14]   45 mov di,[ebp+0x1c]
49 mov [ebp-0x2a],dx   4d xor edx,edx        4f/53/57 word zero stores
5b mov dx,[ebp+0x20]   5f mov [ebp-0x20],dx  63 xor edx,edx         65/68/6b dword zero stores
6e mov edx,[ebp+8]     71 mov dx,[edx]       74 cmp dx,4            78.. signature/width/height/depth/format
```

Ours hoists `group->type` into EDX at +0x3d, so the zero has nowhere caller-saved to live and VC7 spends the zero
already sitting in EBX (`pixels_end`), including on the `group` assert: January `test eax,eax` at +0x0c, ours
`cmp eax,ebx`.

**New measured law - ZERO-PSEUDO PIN.** Under this toolchain, as soon as the function contains ANY zero-valued
store into the descriptor, VC7 forms ONE zero pseudo, pins it in the first free callee-saved register, coalesces
it with the function's entry-initialised zero local, and then also spends it on the `group != NULL` assert
(`cmp eax,<zeroreg>`), never `test eax,eax`. The pin is register-independent (EBX in three probes, EDI in two)
and survives every local placement, declaration order, local type and initialiser position that still reproduces
January's proven frame. w2's X1-X9 (delete zero stores) plus this wave's z1/z2/z3/p1 (move, retype and duplicate
the zero local) are thirteen independent probes of the same mechanism.

## Shapes (scratch only; 5 of the structural budget)

| Shape | Change | Result |
| --- | --- | --- |
| `ab_L5b.c` | rebuild of the w2 best body (single exit, short fetch index, January assert literals) spliced into the current real file | 992 / 59 / `79586c828ad0b2d4`; reproduces w2 exactly; all 59 relocation targets correct |
| `ab_z1.c` | declaration order: `long pixels_end = 0;` before `struct bitmap_data new_bitmap_data;` | byte-identical to L5b; declaration order is inert here |
| `ab_z2.c` | `long pixels_end;` + `pixels_end = 0;` after the descriptor initialisation (w2 `Pa` re-measured) | 992, but the frame layout breaks - descriptor moves to `[ebp-0x30..ebp)` and `pixels_end` to `[ebp-0x34]`; the zero pins into EDI. Refuted by January's slot table |
| `ab_z3.c` | `long pixels_end;` + `pixels_end = 0;` immediately after the assert (w2 `SE2` re-measured) | 992, frame preserved, but the zero still pins - now into EDI (`xor edi,edi` at +0x0c, `cmp eax,edi`) and the `pixels_end` store sinks to +0x5c. Worse |
| `ab_p1.c` | `struct bitmap_data *previous_bitmap = NULL;` hoisted to function scope (w2 `P1` re-measured) | frame grows to 0x38 and the zero still pins in EBX. Refutes "a pointer NULL is a separate zero pseudo" |

Families inherited as spent and not repeated: blanket `csmemset` clear, separate `flags` local, removal of the
entry-live `pixels_end`, `long` return type, per-return `short` casts, signature-store position, `register`,
combined predicate with `goto`, failure/success branch inversions, single shared failure return, `group_type`
local.

## Checks

- final real-file gate (guard on): `exact 5 residual 1 unwritten 0`, row listings identical to baseline;
- `scratch/parkcheck.py source/bitmaps/bitmap_group build/base/...`: `parks 1 drift 0`;
- owner census of the `ab_L5b` candidate against `build/base`: zero candidate-only and zero base-only owners;
- `tools/fake_match_scan.py source/bitmaps/bitmap_group.c`: 0 review leads;
- `_point_from_line3d` guard passed in every gate;
- `git diff --stat -- source/bitmaps/bitmap_group.c`: empty.

## Proposals

**Park re-baseline (zero strict credit; NOT a reopen).** Replace the parked `_bitmap_group_add_bitmap` body with
`scratch/workers/n1_bitmap_group_bitmaps_quantitize/ab_L5b.c` and re-baseline the `config/parked.json` entry to
992 / 59 / `79586c828ad0b2d4`. It is strictly closer in the R11 sense: identical size and relocation count, but it
removes three wrong `.rdata` relocation targets from the shipped object, restores January's single-exit
epilogues and short fetch index, and its 59 relocations are target-identical to January's in order. The w2 ledger
already asked for this; it is re-verified at this HEAD (owner census clean, siblings all still exact).

No header or configuration packet is needed.

## Reopen criterion (refined)

Replaces "a legal-C control that stops VC7 routing the descriptor's zero stores through a zero-valued
callee-saved variable":

> A legal-C, house-rule-admissible form that keeps January's frame (`sub esp,0x34`, descriptor at
> `[ebp-0x34..ebp-0x04)`, `pixels_end` at `[ebp-0x04]`) **and** makes VC7 rematerialise the descriptor's zero
> constant twice in a caller-saved register instead of pinning one zero pseudo. The single-instruction detector is
> January's `test eax,eax` at +0x0c versus our `cmp eax,<zeroreg>`. Everything else in the function is already
> correct, and the 7-byte loop-alignment pad follows automatically from the 4-byte prologue delta.

The 1,424-byte / 152-relocation tag-definition `.data` cluster (`_global_bitmap_reference`,
`_global_bitmap_reference_optional`, `_bitmap_pixel_data`, `_color_plate_data`, `_bitmap_group`) remains absent
and uncredited; it is independent of this function and was not touched by this wave.
