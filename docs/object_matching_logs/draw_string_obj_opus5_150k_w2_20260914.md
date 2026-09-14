# `draw_string.obj` park re-investigation (Opus5 150K house-clean lane, wave w2, 2026-09-14)

## Result

`source/text/draw_string.c` is **unchanged**. All three remaining residuals are locked parks. None of them became
strict exact in scratch, so this wave has no park-reopen proposal and adds no strict credit.

| Function | Target padded / relocs | Real file (park base) | Best scratch this wave | Blocker |
| --- | --- | --- | --- | --- |
| `_bitmap_draw_character` | 944 / 25 | 912 / 25 | 944 / 25, one 4-instruction block | two addends of one sum evaluated in the wrong order |
| `_parse_string` | 624 / 30 | 624 / 30 | 624 / 30, 189/189 instructions, esi/edi swap only | callee-saved register coloring |
| `_bitmap_draw_string` | 304 / 9 | 240 / 9 | not attempted | owner ruling: the exact form reproduces a NULL dereference |

Baseline and final real-file gate, with the `_point_from_line3d` guard: **exact 25, residual 3, unwritten 0**.
- The row listings are identical.
- No park drifts (`scratch/parkcheck.py`).
- The owner census matches `build/base`.
- The fake-match scan finds 0 leads.
- `git diff --stat` is empty.

## Ledgers and duplicate checks

Ledgers read:
- `draw_string_obj_jonas_hcea_one_shot_20260828`
- `draw_string_obj_full_reconstruction_20260902`
- `draw_string_obj_tab_stops_opus_reconciliation_20260903`
- `draw_string_obj_opus5_100k_waveD_20260914`
- the 100K handoff
- the 100K worker notes `opus5-100k-consolidated-20260914/scratch/workersD/draw_string.md`, including bdc1-bdc10, ps1-ps10 and bds1-bds3

Other checks:
- `git log --all -- source/text/draw_string.c`: no other exact body.
- `branch_sweep`: 10 blobs, none better than the current one.

Worker notes are in `scratch/workers/draw_string_bitmap_group.md`. Scratch sources are in
`scratch/workers/draw_string_bitmap_group/`.

## `_bitmap_draw_character` (NOT-LANDED)

**Starting point.** Wave D's best body (bdc6) reproduces at 944/25 on the current file, with 8 diff blocks. January
keeps `x0` in edx across both asserts, including the odd `x0+dx` spill through `[ebp-0xc]`. bdc6 keeps `y0` there
instead.

**New lever: the row guard.** bdc6 had an unguarded up-counting loop.
- Changes made:
  - restore the guarded down-counter `for (row = dy; row > 0; row--, y++, y0++)`;
  - wrap it in `if (dy > 0)`, the single-exit form. The HCEA/real-file early return `if (dy <= 0) return;` compiles
    byte-identically.
- Effect:
  - the allocator switches to January's choice;
  - the `x0+dx` spill appears exactly as in January;
  - only one 4-instruction block at +0xae..+0xcc remains.

**The remaining block.** January evaluates `x0 << encoding_shift` into esi, then `y0 * stride` into ecx, and loads
`base_address` into eax. VC7 here evaluates the product into esi first and adds the base as a memory operand.

**Rejected shapes** (all 944/25):

| Shape | What changed | Result |
| --- | --- | --- |
| C2, C4, C6, C8, C9 | every operand order and association of the sum | identical to C (canonicalised) |
| CA | parenthesised index | identical to C |
| CW | width-first stride | identical to C |
| CY | product first | identical to C |
| V3 | `&pixels[...]` source pointer | identical to C |
| S1, S2 | function- or block-scope `destination`/`source` | identical to C |
| C1 | no guard | back to 8 blocks |
| C3 | named `short bits_per_pixel` | worse: the call's `add esp` is no longer deferred |
| L2 | guard plus up-counter | worse: prologue register swap |
| V2 | source pointer before destination | worse: 928 bytes |
| O1 | named `destination_offset` | worse |

Best candidate: `scratch/workers/draw_string_bitmap_group/dsL1.c`, which is the current file plus `bdcL1.body.txt`.
Measurement: 944/25, sha16 `9a18e06eaa4fc003`.

**Reopen criterion:** a legal-C control that orders the shl and mul addends of a canonicalised pointer sum under VC7
13.00.9254. The park's measured evidence could be refreshed to "944/25, one block" with this body.

## `_parse_string` (NOT-LANDED)

**Wave D's claim was incomplete.** Wave D recorded the real-file body (ps5) as a pure esi/edi swap. It is not:
- at +0x1cf January has `je 0x208` where ps5 has `jne 0x1ea`;
- the result and character stores straddle `pop edi`.

**January's predicate.** Decoding January's control flow gives `(!A||B) && (!C||D) && E`. A redundant second
`test bx,bx` follows can_end-TRUE. In words: an ASCII character must be in `can_end_words`, a non-ASCII character
must not be in `cannot_end_words`, and the next character must not be in `cannot_begin_words`.

**Shapes tried:**

| Shape | Form | Result |
| --- | --- | --- |
| P1 | else-if chain | 704 bytes: result stores are not cross-jumped |
| P2 | implication predicate: `((character & 0xFF00) \|\| character_in_pattern(character, can_end_words)) && (!(character & 0xFF00) \|\| !character_in_pattern(character, cannot_end_words)) && !character_in_pattern(next_character, cannot_begin_words)` | 624/30, 189/189; the polarity block is gone and only the esi/edi coloring of `state` and `&state->string_index` remains |
| T1 | P2 without the pointer local | identical swap |
| T2 | P2 reading `state->string_index` | identical swap |

Best candidate: `scratch/workers/draw_string_bitmap_group/dsP2.c`, which is the current file plus `psP2.body.txt`.

**Reopen criterion:** unchanged (same-compiler evidence of what decides the coloring). Any reopen should start from
the P2 predicate.

## `_bitmap_draw_string` (OWNER-BLOCKED)

Re-verified on the January bytes. After `test ebx,ebx; jne`, the code reads `[ebx]`, `[2]`, `[4]` and `[6]` with no
relocations: the NULL `bounds` rectangle is dereferenced.

Brief section 5 forbids reproducing undefined-behaviour original bugs, so no shape was attempted. The handoff already
lists this as an owner ruling.

**Reopen criterion:** an owner ruling that admits a UB-preserving `BUG (original)` body.

## Proposals

No orchestrator proposals and no park-reopen proposals.
