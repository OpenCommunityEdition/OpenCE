# `bitmaps_quantitize.obj` fresh structural pass (Opus5 next-150K lane, wave n1, 2026-09-15)

## Result

`source/bitmaps/bitmaps_quantitize.c` is **unchanged**. `_row_dither` is still a locked park and did not become
strict exact in scratch. There is no park-reopen proposal and no strict credit.

Baseline and final real-file gate, with the `_point_from_line3d` guard: **exact 2, residual 1, unwritten 0**, row
listings identical; `scratch/parkcheck.py` reports `parks 1 drift 0`; `tools/fake_match_scan.py` reports 0 leads;
`git diff --stat -- source/bitmaps/bitmaps_quantitize.c` is empty.

## Census

The park is classified `instruction-scheduling`. The census confirms that and adds the missing mechanism.

**Frame.** January `sub esp,0x24`; ours `sub esp,0x24` - identical, no slot gap. Every `[ebp-N]` reference is the
same on both sides: `-4` the long copy of `pixel_index`, `-8` its word copy, `-0xc..-0x20` the `original[]` and
`quantized[]` byte arrays, `-0x10` the `source - next_source` bias, `-0x14` `pixel_count - 1`, `-0x18` the
`channel_index` home, `-0x1c` the `channels_remaining` home, `-0x24` `source_after_pixel`.

**Relocations.** target 0, ours 0; nothing to census. There are no strings or literals in this function.

**Instruction stream.** 177 vs 177, identical except for one three-operation permutation in the tail of the third
(error-distribution) `do..while`:

```
January: +0x178 inc eax     +0x179 add ebx,2   +0x17c add ecx,2
ours   : +0x178 add ebx,2   +0x17b inc eax     +0x17c add ecx,2
```

`eax` = `channel_index` (homed at `[ebp-0x18]`), `ebx` = `minimum_channel`, `ecx` = a compiler-generated
induction variable equal to `&next_source[channel_index + CHANNEL_COUNT]` (its uses are `[ecx-0x10]` =
`next_source[ci-4]`, `[ecx-8]` = `next_source[ci]`, `[ecx]` = `next_source[ci+4]`; the current row rides the same
IV as `[edx+ecx]` with `edx = source - next_source` formed in the preheader at +0xb3/+0xb5). The preheader is
byte-identical to January's.

## Shapes (scratch only)

| Shape | Change | Result |
| --- | --- | --- |
| `rd_d1.c` | laws_w2 A5 (hand strength-reduction removal): delete the `minimum_channel` cursor and index `minimum_error[channel_index]` directly | 170 instructions, `sub esp,0x20` - **loses a frame slot**. REFUTED on structural grounds: January's frame proves the explicit cursor |
| `rd_d2.c` | swap the two tail increments to `channel_index++; minimum_channel++;` | 177/177; order becomes `inc eax; add ecx,2; add ebx,2`, reproducing the park's recorded negative exactly at this HEAD |
| `rd_d3.c` | give the next row its own cursor (`short *next_channel = next_source;`, accesses `next_channel[-CHANNEL_COUNT] / [0] / [CHANNEL_COUNT]`, three explicit tail increments in January's order) | 496 bytes (+32) / 185 instructions, frame 0x28, and the `source - next_source` bias hoists out of the preheader to function entry - strictly worse |

The park evidence already records the declaration, loop-tail, pointer, dependency, barrier, pragma and
diagnostic-assembly families as exhausted; none was repeated. The prohibited levers (barriers, pragmas,
artificial dependencies, `volatile`/`register` forcing, inline assembly, byte patches) remain rejected.

The obvious w3-era candidate - rewriting an inner loop as a `for` with a comma increment clause (laws_w3 A25 and
its admission note) - is **not applicable** here: all three inner loops have a compile-time-constant trip count of
`CHANNEL_COUNT`, and the VC7 unroller do-while law says a small constant-trip `for`/`while` is unrolled and
folded while a `do..while` is not. January's bytes show an unrolled-free loop, so the `do..while` spelling is
proven and the increment clause is unavailable.

## New measured facts

1. **The explicit `minimum_channel` cursor is frame-proven.** Removing it in favour of
   `minimum_error[channel_index]` shrinks `sub esp,0x24` to `0x20`. The A5 "pointer walk to direct indexing"
   lever is refuted here structurally, not by scheduling.
2. **Generated-IV placement law (measured on both statement orders).** VC7 emits the compiler-generated
   induction-variable update IMMEDIATELY AFTER the increment of the index variable it is derived from, and emits
   the explicit source increments in source statement order:
   - `minimum_channel++; channel_index++;` -> `add ebx,2; inc eax; add ecx,2`
   - `channel_index++; minimum_channel++;` -> `inc eax; add ecx,2; add ebx,2`
   January's `inc eax; add ebx,2; add ecx,2` therefore requires the `ecx` IV **not** to be a child of
   `channel_index`'s increment. It is unreachable by permuting the two source statements, and both permutations
   are now measured at this HEAD.
3. **An explicit third cursor is structurally worse.** The only source form that would give three explicit
   increments in January's order costs 32 bytes and moves the bias computation out of the loop preheader.

## Checks

- final real-file gate (guard on): `exact 2 residual 1 unwritten 0`, row listings identical to baseline;
- `scratch/parkcheck.py source/bitmaps/bitmaps_quantitize build/base/...`: `parks 1 drift 0`;
- siblings `_bitmap_quantitize` (592) and `_bitmap_quantitize_read_row` (32) stay EXACT in every candidate gate;
- `branch_sweep`: 6 unique historical blobs, all strictly worse than the current file (exact 1 or 0); no donor;
- `tools/fake_match_scan.py source/bitmaps/bitmaps_quantitize.c`: 0 review leads;
- `_point_from_line3d` guard passed in every gate;
- `git diff --stat -- source/bitmaps/bitmaps_quantitize.c`: empty.

## Proposals

None. No header or configuration packet is needed and no park re-baseline is warranted (no candidate is closer
than the parked body).

## Reopen criterion (refined)

Replaces "a genuinely new lawful scheduling control":

> A source form in which the `&next_source[channel_index + CHANNEL_COUNT]` induction variable is **not** derived
> from `channel_index`'s increment, while `minimum_channel` stays an explicit cursor (frame-proven) and the
> `source - next_source` bias stays in the loop preheader - or original statement provenance. Statement-order
> permutation of the two existing increments is now exhaustively refuted: both orders are measured, and the
> generated IV follows its parent's increment in each.
