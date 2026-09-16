# `bitmap_utilities.obj` — next-150K lane, final implementation wave tierB (2026-09-16)

## Outcome

Nothing landed. `source/bitmaps/bitmap_utilities.c` is unchanged (`git diff --stat` is empty).

Baseline and final real-file gate (with the `_point_from_line3d` guard): **exact 43, residual 5, unwritten 0**, the
same row for row. The guard passes. `scratch/parkcheck.py` reports 5 parks with 0 drift. The owner census matches
`build/base`. The fake-match scan finds 0 leads.

| function | January padded / meaningful / relocs | status | result |
| --- | --- | --- | --- |
| `_bitmap_2d_sharpen` (Tier B row B3, the only function in scope) | 1152 / 1151 / 29 | parked | NOT-LANDED: the implied type change is refuted; the loop-form fix removes the width lens; register allocation remains, so the work stopped |
| `_bitmap_2d_alpha_bleed`, `_bitmap_2d_smooth`, `_bitmap_3d_smooth`, `_bitmap_sharpen` | — | parked | out of scope for this wave |

Worker notes, with the census, every measurement and the per-function block: `scratch/workers/tB_bitmap_utilities.md`.
Scratch files are in `scratch/workers/tB_bitmap_utilities/`:
- `v1.c`, `v2.c` and `v3.c` are the three shapes;
- `d1.c` is the keystone diagnostic;
- `mk.py` builds a candidate;
- `o_*.txt` and `aln_*.txt` hold the disassembly and alignments.

## Assignment and duplicate prevention

The orchestrator assigned research-campaign row B3.
- **Evidence given:** "F3 DECLARED-TYPE: cells -8/-12/-16 read at 2 and 4 bytes by us, 4 only by January".
- **Implied change:** declare `y`, `byte_index` and `row_size`/`next_y` as `long`.
- **Second label on the row:** F6 = TIE. That label only means no earlier wave closed the row. It does not identify a defect.

Read before the census:
- all nine `bitmap_utilities_obj_*` ledgers;
- the w1 worker notes (s5a/s5f/s5g, the 417-order sweep) and the w3c worker notes (four byte facts, no shape);
- laws_w2, laws_w3, and the research campaign (sections 2-5).

`git log --all` shows 14 commits; the newest is 723ce4dd3. `branch_sweep.py` finds no better blob. The row was still
non-exact at baseline: 1184/29, sha `8a988645f8b5734c`.

## Census (written before any shape)

- **tinfo.** January is 1152 padded (1151 meaningful, 371 instructions), 29 relocations, sha `38d28fa8cc7ad114`.
  The HEAD park is 1184/29.
- **Frame.** `sub esp,0x3c` on both sides. The slot-by-slot table is in the worker notes. At HEAD, January's cells
  -8/-0xc/-0x10 hold `previous_row`, `current_row` and a pointer IV, read only as dwords. Ours hold the short
  `wrapped_left` IV, `y` and `wrapped_right`.
- **Relocations by target.** Counts are equal for every target:
  - calls: `bitmap_verify`, `display_assert` ×4, `system_exit` ×4, `bitmap_get_pixel_data_size`, `debug_malloc`,
    `bitmap_2d_address` ×3, `bitmap_mipmap_address`, `csmemcpy`, `debug_free`, `error`;
  - literals: the file string ×6, and each of the four assert/diagnostic strings once.

  No call or global is missing or extra.
- **Registers.**
  - January: EBX holds `bitmap` from entry. ESI holds `negative_table` (private ABI). EDI holds `y` at the row head,
    then `byte_index`, with `push edi` shrink-wrapped to +0xf1. `temporary_pixels` and every row pointer stay in memory.
  - HEAD park: EDI holds `bitmap`, then `current_row`. EBX holds `temporary_pixels`, then `previous_row`.

## Lens re-verification

The F3 byte fact reproduces: `frameslot.py` on the HEAD object reports cells -8/-12/-16 as DECLARED-TYPE. **The implied
change is contradicted by January's own bytes.** January's short variables live in EDI and are provably 16-bit:
- `test di,di` at +0x100;
- `cmp di,4` at +0x240;
- `cmp di,cx` at +0x25a, +0x33a and +0x42a;
- `cmp di,word ptr [ebp-0x20]` at +0x415;
- `movsx eax,di` at +0x269 and +0x346.

Shape 1 applied the implied `long` declarations. It reaches size 1152, but it emits 32-bit `test edi,edi` and `cmp ecx,4`
and shrinks the frame to 0x38, so the size match is a coincidence.

The width delta is a symptom. HEAD hoists `short row_size = 4 * bitmap->width`, so `row_size + byte_index` becomes an
induction variable homed as a short. January re-reads `bitmap->width` on every iteration: a byte store through
`destination_row` may alias it. January's wrapped index is therefore a per-iteration `movsx` temporary homed as a dword.

## Shapes (3 of 5)

| # | shape | evidence | size/relocs | aligned target insns | verdict |
| --- | --- | --- | --- | --- | --- |
| 0 | HEAD park | — | 1184/29 | 154/372 | baseline |
| 1 | `long y, next_y, row_size, byte_index` (implied change) | research B3 | 1152/29 sha | 157 | refuted by widths (see above); reverted |
| 2 | loop forms from the bytes, HEAD term order kept | see list below | 1200/29 sha | 184 | prologue becomes January's `push ebx; mov ebx,[ebp+8]`; loop forms match; wrapped-term order reversed; reverted |
| 3 | shape 2 + natural row-major kernel order | January loop 1 emits `pW cW nW`; the HEAD (HCEA decompiler) order emits `nW cW pW`; w1 measured that row-major reproduces January | 1184/29 sha `7e1a852d964151c9` | 209 | best; scratch only |

Byte facts behind the loop forms in shape 2:
- **Loop 1 has no `row_size` local.** January compiles it as a compare loop that re-reads the width on every
  iteration (+0x195..+0x247). The source is `for (byte_index = 0; byte_index < 4; byte_index++)` with
  `short wrapped_left = 4 * bitmap->width + byte_index;`.
- **The loop 2 bound is a short local.** January compares it at 16 bits (`cmp di,cx`, +0x25a) and counts the loop with
  a 16-bit trip count (`movzx ecx,cx`, +0x288). The source is
  `for (last_byte_index = 4 * bitmap->width - 4; byte_index < last_byte_index; byte_index++)`.
- **Loop 3 reuses that bound plus 4.** January adds 4 and stores it back to the same cell (+0x337/+0x33d), and loop 3
  re-reads the width for `wrapped_right` (+0x370). The source is
  `for (last_byte_index += 4; byte_index < last_byte_index; byte_index++)` with
  `short wrapped_right = byte_index - 4 * bitmap->width;`.

HCEA's `bitmap_2d_sharpen.c` independently shows the `(4*width-4)+4` bound.

Each shape was gated as a whole TU. All 43 exact rows stayed exact, and parkcheck drift was limited to the target.

### What remains in shape 3

Every hunk was read:
- The row header computes the same values in the same order.
- The loop bodies use January's instruction forms, and loop 1's term order is identical.
- On shape 3's object, `frameslot.py` now reports **WIDTH-SELECTION / NOT-A-DECLARATION**: all data widths are equal.
  The B3 lens is resolved.

The remaining difference is **which web owns EDI**. In January, EDI holds `y` and then `byte_index`. In ours, EDI holds
`temporary_pixels` before and after the loop, `previous_row` across calls 2-3, and the pointer IVs inside the loops.
Two consequences follow from that assignment:
- `y` is spilled, with an extra rotation `jmp`;
- in loops 2 and 3, the IV representative is ours `dest+bi` instead of January's `prev+bi` with a `dest-prev` store
  offset.

This is register allocation. Following owner requirement 3, the work stopped. No law in laws_w2/w3 or the research
campaign names a source construct that moves callee-saved web ownership:
- M8 governs x87 leaves;
- H3 governs block-head reload order;
- F3 part 0 is a statistical use-count ranking with no source lever.

HCEX.pdb has no `bitmap_2d_sharpen` (or `bitmap_sharpen`/`bitmap_2d_smooth`) symbol, so there is no local-list evidence.

## Diagnostic: w3c keystone hypothesis

w3c proposed that a correct `_bitmap_2d_sharpen` might keep `_bitmap_2d_uncompress_from_mipmap` exact when
`_bitmap_2d_smooth` is reopened.

`d1.c` combines w3c `m4.c` (the 2d_smooth reopen) with the shape-3 sharpen block. It gives 43 exact:
`_bitmap_2d_smooth` becomes EXACT and uncompress becomes residual. That is identical to `m4.c` re-measured at this HEAD.
**A structurally corrected but non-exact sharpen does not hold the coin.** The hypothesis remains untested only for an
exact sharpen body.

## Corrections for the research campaign ledger

- **B3 implied change:** `long` types are REFUTED (32-bit ops and a 0x38 frame; January's y/byte_index are provably
  16-bit).
- **B3 mechanism:** HEAD's hoisted `row_size` and the loop forms are the real defect behind the F3 DECLARED-TYPE
  verdict. Fixing them (shape 3) changes the verdict to NOT-A-DECLARATION and leaves a register-allocation residual.
  Reclassify the row to Tier C / register-allocation.

## Reopen criterion

Either of these would reopen the row:
- a measured law, or January-side evidence, that names why `y`/`byte_index` rather than
  `temporary_pixels`/`previous_row` win EDI in VC7's allocator;
- an authoritative local list for this function.

Start from `scratch/workers/tB_bitmap_utilities/v3.c`. Do not re-spend these:
- declared-type (`long`) changes;
- kernel term-order sweeps (w1);
- `row_size` hoisting variants.
