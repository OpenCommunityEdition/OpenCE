# `lruv_cache.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, baseline `ae10935da`.
Production source was NOT modified. `git diff --stat -- source/memory/lruv_cache.c` is empty.

## State

| measurement | baseline | final |
| --- | --- | --- |
| gate | exact 17 residual 1 unwritten 0 | exact 17 residual 1 unwritten 0 |
| `_lruv_debug_to_file` | 464 padded / 20 relocs / `8b2776c4666f2276` | unchanged |
| target | 464 padded / 458 meaningful / 20 relocs / `fb7c7a22cacb060e` | — |

Park `_lruv_debug_to_file` [register-allocation]: OK, drift 0.

## Census

- **Frame:** `sub esp,0x14` on both sides. `[ebp-1]` (locked), `[ebp-8]` (block_index) and `[ebp-0xc]`
  (page_count) are identical; only the `stream` and `age` cells are SWAPPED (January stream `[ebp-0x10]`
  / age `[ebp-0x14]`, ours the reverse).
- **Relocations:** 20/20, same targets, same order.
- **Code:** the body is an instruction-for-instruction match under a 3-cycle permutation of the
  callee-saved registers — January (cache = EDI, stream/block = ESI, accumulator = EBX) versus ours
  (cache = EBX, stream/block = EDI, accumulator = ESI). The prologue push order (January
  `push esi; push edi` + shrink-wrapped `push ebx` at 0x31; ours `push ebx; push edi` + `push esi`)
  follows first definition and is a consequence of the cycle, not a cause.
- Only two non-register differences exist, both at the `block_name_proc` join:
  (i) January interleaves `push ecx` between the two field loads of
      `page_index = block->first_page_index + block->page_count` (0x110-0x117); same five instructions.
  (ii) January materialises stream, page_count and block_name in registers in BOTH arms and converges
      with `jmp 0x16a` (`je 0x15f` sends the NULL case away); ours hoists two of the three loads above
      the branch and needs one extra `jmp`. Net 176 our instructions versus 173, at the same 464 bytes.

## Shape (1) — and it settles an open question

**L1, goto-free join.** Delete both `goto`s; assign `block_name = ""` in each of the three paths
(the `block_index == NONE` arm, the inner `else` arm, and `if (!block_name) block_name = "";` after
`block_name_proc`). Result: **464 padded but 22 relocations** (`[relocs 22!=20]`) — worse.

January references the empty-string literal exactly ONCE, so the source contains exactly one
`block_name = "";` statement, and the retained `output_hole:` / `output_block:` label topology is
January-correct. VC7 does not CSE the three `""` address loads, so every multi-assignment spelling costs
+2 relocations. This is new evidence, not a tie verdict.

The 2026-08-25 one-shot already proved the unsigned-`age` lifetime and the `MIN` cast removal
object-neutral; nothing in this wave changes that.

## Disposition

`NonMatching`, 17/18, park retained, no credit claimed.

Reopen criterion (2026-08-25 wording retained): January-authoritative source/local records, a strict
donor, or a legal lifetime fact that directly changes the FIRST callee-saved divergence at +0x6.
ADDED do-not-repeat: goto-free / multi-assignment `block_name` spellings — refuted by relocation count.
