# `rasterizer_frame_statistics.obj` opus5 150k wave 1 (2026-09-14)

## Verdict

**No production source change.** The real file keeps its baseline: `== exact 7 residual 3 unwritten 0`. The
`_point_from_line3d` guard passes, 2 parks have 0 drift, and the fake scan finds 0 leads. `git diff --stat` on the
unit is empty.

The draw residual is a C2 declaration-count / TU-state tie: VC7 picks the load order of commutative operands through
TU state. It is not a gap in the source topology. Both parks remain as they are.

## Baseline (real file, lane base 12f7375d4)

| Function | Target padded / relocs | Candidate | Tag |
| --- | --- | --- | --- |
| `_eat_my_shorts` (parked) | 32 / 0 | 16 / 0 | size, sha |
| `_rasterizer_frame_statistics_count_static_vertices` (parked) | 64 / 0 | 64 / 0 | sha |
| `_rasterizer_frame_statistics_draw` | 4176 / 322 | 4176 / 322 | sha |

These seven functions are inherited as exact and were not touched: initialize, begin, get_fps, fps_accumulate,
count_dynamic_vertices, end and dispose.

## `_rasterizer_frame_statistics_draw`

### What alndiff shows

The target and v0 both have 1178 instructions. Only three real hunks remain; every other hunk is a
defined-noncode literal naming artifact:

- +0xB0 / +0xA0 at 0x6e, 0x7e and 0x8d;
- +0xE4 / +0xD8 at 0x10f, 0x117 and 0x197.

In each hunk the loads for a commutative `lea` come in the opposite order.

### Measurements

All candidates are in `scratch/workers/rasterizer_frame_statistics/`.

1. **`eA`: signedness.** A scratch header turned every `unsigned long` counter into `long`, which is the donor
   7841c6ac3 typing. The same three hunks remain, so signedness is not the lever.
2. **`diag/`: declaration count (diagnostic only, never admissible).** Inserting k dummy declarations before the
   constants section changes *which* pairs are swapped, in a lottery pattern:
   - k = 0..8 for `typedef`, `extern` and prototypes;
   - k = 9..48 for `extern`.

   Five pairs are affected: vertices 0x1f/0x37, triangles 0x6e, primitives 0xca/0xd3, model 0x10f, and
   0x19a/0x1a8/0x1b1. No k gives zero hunks.
3. **Coupling to the parks.** Changing only a parked predecessor body also flips the lottery. Examples:
   - removing the unneeded `eat_my_shorts` forward prototype gives 9 hunks;
   - the result-variable comparator gives 6 hunks, on different pairs.

The 100K lane already showed that the private-statics split (d5/d6) does not move the draw. That result still holds
with get_fps exact.

### Classification and reopen

- **Classification:** class D. The tie is on commutative operand order, driven by TU declaration and symbol count. It
  is coupled to the two parked predecessor bodies.
- **Do not repeat:** any declaration, prototype, include or layout-assert add/remove/reorder search. Also do not choose
  park bodies to flip the draw. Both are steering; the 20260906 hold ledger already rejected them.
- **Reopen:** only with January's authentic declaration environment for this TU. Re-measure the draw whenever either
  park is reopened with authentic exact bodies.

## `_eat_my_shorts` (parked; scratch-only)

January's code is two epilogues with no setcc if-conversion: `mov ax,[first]; cmp ax,[second]; jbe; mov al,1; ret;
xor al,al; ret`. The extern sweep does not change it (k = 1..46), so it is not TU-state.

The previous lanes' three spellings are on the park record. This wave tried six more, in `ems/`:

| Shape | Spelling | Result |
| --- | --- | --- |
| e1 | `result = FALSE; if -> TRUE` | 32 B and branchy, but the zero store is hoisted before the compare and there is only one epilogue |
| e2 | result assigned in if / else | seta, 16 B |
| e3 | `?:` | sbb/neg, 32 B |
| e4 | `(boolean)` cast of the compare | not exact |
| e5 | two returns in if / else | seta, 16 B |
| e6 | no forward prototype | seta, 16 B |

- **Reopen:** January source, or a natural context that suppresses VC7's if-conversion while keeping both epilogues.

## `_rasterizer_frame_statistics_count_static_vertices` (parked; scratch-only)

The only difference is where `push esi` goes:

- **January:** the push comes after the `triangle_buffer` null `je`. That early exit targets a label past
  `pop esi; nop`.
- **Ours:** VC7 pushes before the `je`.

`get_fps`, which is exact in the same TU, shows that VC7 delays callee saves when the early-exit edge has its own
target block (its else arm). Five shapes were tried, in `csv/`:

| Shape | Spelling | Result |
| --- | --- | --- |
| s1 | switch (triangles, strip) | 48 B |
| s2 | early returns | 48 B, no esi |
| s3 | switch with default | 48 B |
| s4 | switch with returns | 48 B |
| s5 | strip arm returns | the baseline fixed point |

- **Reopen:** authentic topology that gives the `triangle_buffer`-null edge its own exit block. Re-measure the draw at
  the same time.

## Evidence notes

- The atlas decorates `@rasterizer_frame_statistics_get_fps@4` and `@rasterizer_frame_statistics_count_static_vertices@8`.
  January's split bytes read both parameter sets from the stack, and get_fps is exact as cdecl. The decoration is
  therefore not a usable ABI lever here.
- No header or config proposal.
