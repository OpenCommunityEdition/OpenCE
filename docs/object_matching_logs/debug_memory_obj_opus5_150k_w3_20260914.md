# debug_memory.obj — Opus5 150K house-clean lane, wave w3 (2026-09-14)

## Outcome

`source/cseries/debug_memory.c` is **unchanged** (`git diff --stat` empty). Gate before and after: 17 exact / 1 residual / 0 unwritten,
guard passes, park drift 0, fake scan 0 leads. The only non-exact row, `_debug_realloc` (416 / 25), is parked; all work was scratch-only
and nothing is proposed for landing.

## Duplicate prevention

Read all eight `debug_memory_obj_*` ledgers. The park source is `jonas_fuzzy_closeout_20260901`; the do-not-retry list is in
`jonas_single_header_lifetime_rejection_20260830`. That list covers helper formal order, the predicate alone, the
separate-header/mutable-size/goto donor, and single-header file/line reuse plus accounting. Also read `laws_w2.md`.

## New evidence: A16, `&&` order from control flow

January's realloc-size argument lowers as `test edi,edi; jne use_size; test esi,esi; je use_size; xor eax,eax`. It tests **size
first**, then the pointer, and re-tests the pointer even on the path where the pointer is already known non-NULL.

- The park body spells `pointer != NULL && size == 0 ? 0 : allocation_size`. VC7 threads the known pointer test away and emits
  `mov eax,[ebp+0xc]; test eax,eax`.
- The size-first spelling `size == 0 && pointer != NULL ? 0 : allocation_size` is therefore target-proven.
- It also explains why January keeps the pointer live after the if-block (the `mov esi,[ebp+8]` reload).

## Shapes (scratch only)

| id | shape | result |
| --- | --- | --- |
| r1 | size-first ternary only | 416/25, 148 insns; realloc-size region instruction-identical; not exact |
| r2 | r1 + file/line parameter reuse | 400/25, frame 0xc (worse) |
| r3 | r1 + line reuse only | 416, frame 0xc (worse) |
| r4 | r1 + file reuse only | 400 (worse) |

All four keep the 17 exact siblings.

After r1 the remaining difference is register allocation:
- January enregisters the pointer in ESI from entry, while r1 leaves it in memory;
- January gives `header` a stack home (`[ebp-0xc]`, used on the NULL path), while r1 keeps it in ESI;
- the allocation file sits in EBX in January but has a stack home in r1.

## Reopen criterion

The park criterion is unchanged: explain the ESI/EDI/EBX entry lifetimes while keeping the 16-byte frame. Start any reopen from
`scratch/workers/w3_stack_walk_windows_profile_etc/r1.c`, whose size-first ternary is target-proven.
