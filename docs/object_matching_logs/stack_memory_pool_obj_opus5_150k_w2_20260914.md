# `stack_memory_pool.obj` — opus5 150K house-clean lane, wave w2 (2026-09-14)

## Baseline

Gate: 31 exact / 2 residual / 0 unwritten, with the point guard passing and no parks. The two residuals:

| Function | January padded / relocs / sha16 | Real file |
| --- | --- | --- |
| `_stack_memory_pool_allocate` | 640 / 36 / `00ddbe408b35e153` | `c49ac9850cc6a1eb` |
| `_stack_memory_pool_resize_block` | 192 / 13 / `cc68570f2905b455` | `4d1bc2fd72947f08` |

The real file is unchanged.

## `_stack_memory_pool_allocate`: not landed, new evidence recorded

**New evidence (assert-anchor line order).** January's nonfatal `display_assert` lines appear in this order:

| Line | Message |
| --- | --- |
| 0x35F | "no more unsused master pointers" |
| 0x370 | — |
| 0x37D | — |
| 0x394 | — |
| 0x39F | "allocation from memory pool failed" |
| 0x3A4 | "invalid size" |

So both failure diagnostics follow the list-insertion code in the source text. They are else arms at the bottom of a nested single-exit body, not the early returns the current source uses. The 82-experiment full-admission ledger and the 2026-09-13 closeout never tried this topology.

Two shapes were tried:
1. Nested single-exit body with a `space_available` flag set from `free_space != NULL`: 656/36. The flag is not jump-threaded (`setne al`).
2. The same body with `if (!free_space) space_available = FALSE;`: 640/36, with a residual identical to the current real body.

The line-order-faithful spelling is therefore code-equivalent to the current early returns. The remaining gap is independent of return topology: January uses `test edi,edi` / `xor eax,eax; cmp ebx,eax` at entry and returns the invalid size with `mov eax,esi`. Two observations about that gap:
- January's ESI zero is a variable, not the pinned constant, and it reaches only the invalid-size return.
- The earlier carriers (exp 076/077) did not reproduce it.

**Reopen criterion:** identify the variable whose NULL value reaches the invalid-size return through ESI without constant propagation, within the nested body. Scratch: `scratch/workers/player_profile_files_windows_etc/smp_v1.c` and `smp_v2.c`.

## `_stack_memory_pool_resize_block`: skipped (exhausted)

The only difference is January reloading `reference` from [ebp+8] twice. Experiments 024–027, 040–041 and 060–061 already covered the reuse/grow/return and alias families, there was no new evidence, and no shape was spent.
