# `cinematics.obj` — opus5 150K house-clean lane, wave w2 (2026-09-14)

## Outcome

This wave did not change `source/cutscene/cinematics.c`. The real-file gate before and after is identical: `== exact 16 residual 1 unwritten 0`. The `_point_from_line3d` guard passes, `_cinematic_render` shows no park drift, and the fake scan reports 0 leads.

## `_cinematic_render`: not landed

January is 1,280 bytes with 57 relocations, sha `a89dcee38e6a2396`. The candidate is also 1,280 / 57, but its sha differs.

A fresh alndiff of the current real file shows the residual is now only **4 instructions** at +0x45d..+0x468. The stack-slot rotations recorded by the 20260831 ledger no longer appear. The remaining difference is in the argument to `rasterizer_text_set_shadow_color((alpha << 24) | (title->shadow_color & 0x00FFFFFF))`:

```
January:   shl eax,24 ; and ecx,0xffffff ; or eax,ecx ; push eax
candidate: and ecx,0xffffff ; shl eax,24 ; or ecx,eax ; push ecx
```

New evidence checked: HCEX.pdb (`DIA2Dump -sym cinematic_render`) lists only the stack aggregates `rectangle2d bounds` (twice) and `real_argb_color text_color`. That gives names only, with no scalar or scope information.

Five shapes were tried. All of them left the same 4-instruction residual:

| shape | change |
| --- | --- |
| c1 | drop the `(pixel32)` cast |
| c2 | swap the `\|` operands |
| c3 | parenthesise as the XDK `RGBA_SETALPHA` macro does |
| c4 | inline `PIN(...)` into the shift (the bitmap_utilities idiom) and drop the `shadow_alpha` local |
| c5 | put the result in a named `pixel32 shadow_color` local |

VC7 orders the two `|` operands the same way regardless of source order, casts or grouping. The residual is a tie in which register holds the OR result and which operand is evaluated first (class D). The body stays parked.

The separate whole-object blocker is unchanged: the candidate TU still emits its own `_fast_ftol` COMDAT (see the 20260831 ledger).

**Reopen criterion:** a same-compiler donor in which VC7 uses the shift operand as the OR accumulator for `(long << 24) | (mem & mask)`.
