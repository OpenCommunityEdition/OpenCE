# `path.obj` next-150K lane, final implementation wave tierB (B1 `_path_state_traverse`, 2026-09-16)

Worker `tB_path`. Unit `source/ai/path.c`; target `build/split/source/ai/path.obj` (January 2002 Xbox debug,
VC7 13.00.9254). Scope: exactly one function, the research campaign's Tier B row B1 `_path_state_traverse`
(2,093 meaningful bytes, PARKED, class unclassified). Notes: `scratch/workers/tB_path.md`; candidates, generators
and tools: `scratch/workers/tB_path/`.

## 1. Outcome

**Nothing landed. `git diff --stat -- source/ai/path.c` is empty.** The parked body stays locked; no row became
strict EXACT, so there is no park-reopen proposal and no header/config proposal.

Final real-file gate (`gate.py source/ai/path --all --forbid-emitted-symbol _point_from_line3d`):
`== exact 28 residual 4 unwritten 0`, guard passed, all 32 rows identical to the baseline listing;
`scratch/parkcheck.py`: parks 4 drift 0; `tools/fake_match_scan.py source/ai/path.c`: 0 leads; owner census
unchanged (file not edited).

Best scratch body: `scratch/workers/tB_path/s3.c` (function fragment `frag_s3.c`). It removes every
structural difference: frame, byte cells, local inventory, control flow, block layout and register assignment all
equal January. What is left is an x87 term-order tie in three blocks plus the strength-reduced edge-cursor bias.
Per owner requirement 3 the wave stopped there.

## 2. Census (owner requirement 1, measured at HEAD 6d6529d98 on the w3c `t1.c` start point)

- tinfo: January 2096 padded / 65 relocs / sha 3e33ddd8b81a7b51, 652 instructions; t1 663 instructions, 131 alndiff hunks.
- Frame: January `sub esp,0x85c` vs ours `0x864`. The full slot table is in the notes. The decisive cells:
  - January has one address-taken float at `[ebp-0x30]`. It receives the `path_attractor_weight` out-parameter, the MIN result and 0.0f. Ours has two objects, `distance_to_attractor` and `closest_approach_to_attractor`.
  - January has three byte booleans: `result` at `[ebp-1]`, `passable` at `[ebp-2]` and `reported_cost_overflow` at `[ebp-3]`.
  - January keeps `new_node_index` in memory `[ebp-0x10]`, a cell it shares with `adjacent_surface_index`, and keeps `heap_location` in BX.
  - January has no initializer stores for `distance_to_destination` or `new_node_index`.
- Relocations by target: 65 = 65. Every call, string and global is equal. The only difference is the literal label class (`symbol:` vs `defined-noncode:`), the csplit artifact present on every exact row.
- Both quoted lenses re-verified and HOLD:
  - **F3 DECLARATION.** 0x85c vs 0x864, byte cell -1, and the address-taken object at -0x30.
  - **F6 SOURCE-DEFECT-OPEN.** Entry stores `[ebp-1]=1` and `[ebp-3]=0`, and `[ebp-2]=0` behind `jne`. The two dword stores in ours are initializers January lacks.
- HCEX.pdb SHIP (`DIA2Dump -sym path_state_traverse`) independently lists `closest_approach_to_attractor` as a stack float and has no `distance_to_attractor`.
- Prior art: waveD `b0` (100K lane worktree, read-only) already carried the booleans, the single attractor object, the cost join and the `<=` result tail. Re-gated here it measures 2080/65, 658 instructions, 95 hunks, frame equal. So the implied change alone does not close.

## 3. Shapes (4 of 5 spent)

| shape | change (evidence) | result |
| --- | --- | --- |
| S1 | b0 + exhausted-search test moved to the loop top (January places the exhausted block at the loop exit ahead of the break blocks, with no re-test; HCEA has the same loop-top NONE test) + diameter squared inside the `&&` right operand (January `fadd st0,st0` after the 16.0f compare) + `edge_length` used by both PIN bounds (January stores the fsqrt before the dot; a single-use named sqrt is forwarded) + `new_node_index = node_index` before the heap asserts (January store @0x5c1 between `cmp bx,1` and `jl`) | 2096/65 [sha], 660 insns, 57 hunks; tail and projection block byte-identical, new_node_index in `[ebp-0x10]` |
| S2 | S1 + `boolean passable` declared before `adjacent_surface_index` (store order `[ebp-2]` then `[ebp-0x10]`) + post-loop `if (node_index != NONE) {found} else if ... alloc ... overflow` | 2128 [size], 665, 62; store order and block order now January's, but VC7 does not thread the found (break) edge past the post-loop NONE test (+2 insns) |
| **S3** | S2 with the probe loop handling both exits in place, found first: `while (TRUE) { node_index = hash[slot]; if (node_index != NONE) { ...found...; break; ... } else { alloc / overflow; break; } }` | **2112 [size], 657, 61; structure = January** (alloc falls into the join, found and overflow deferred after `path_heap_insert`, no re-test); every frame cell equal in width and refs; frameslot `PERMUTATION / NOT-A-DECLARATION`; siblings unchanged; fakescan 0 |
| S4 | S3 with the `edge` alias replaced by `edges[edge_index].` (laws_w3 A33) | 2112, frame 0x860 (+4), 661, 106 hunks, bias unchanged: rejected, the alias is genuine |

Operand-normalized diff of S3 vs January leaves only three items:
1. **Edge-cursor bias.** The strength-reduced base is `[ebp-0x840]` (+0x1c) where January has `[ebp-0x844]` (+0x18). It changes displacements only.
2. **x87 term order in three blocks.**

   | block | January | ours |
   | --- | --- | --- |
   | `magnitude_squared3d` squares | i,k,j | i,j,k |
   | t numerator dot | k,i,j | k,j,i, plus one extra `fxch` |
   | `distance3d(&cheapest_node->entry_point, &entry_point)` squares | k,j,i | i,k,j |

   The destination `distance3d` block already matches.
3. **Size.** The +2 bytes of the extra `fxch` make one `jne` near and push the probe-loop head into the 1..8 alignment window (`lea esp,[esp]`). That is the entire 2112 vs 2096 padded gap.

**mod8.py, run last on S3.** Verdict INSTRUMENT-PERTURBS. The per-block census (`x87runs.py` over `mod8_work` d0..d8) shows:
- The magnitude block is D-insensitive.
- The dot never reaches January's k,i,j.
- The two `distance3d` sites move with the same period, so no uniform count fixes both (D=2/4/6 fix the linear site and break the destination site).
- The bias is D-insensitive.

The x87 residual is therefore not a single IL-local count defect. The remaining spellings are helper or argument permutations with no January role evidence (R3/R15, research section 4.4 retired). The wave stopped (owner requirement 3).

## 4. New measured facts (for laws ledgers)

- **Loop-top break block ordering.** `while (TRUE) { x = call(); if (x == NONE) { ...; break; } ... }` makes VC7 duplicate the call and test at the bottom (rotation). It places the NONE handler first among the loop's exit blocks, ahead of later `break` blocks, and those blocks skip it. The `while (x != NONE) {...} if (x == NONE && ...)` spelling instead re-tests `cmp bx,-1` on every break path.
- **Break edges are not jump-threaded.** VC7 threads a loop exit only on the edge whose last compare decided the value. A `break` from later in the body still passes a post-loop `if (node_index != NONE)` test (S2).
- **Block layout follows fall-through.** A block reached only by a forward jcc is deferred to the next hole after an unconditional jmp, in IL order. An if/else arm falls into the statement after the if. An arm that ends in `break` inside a `while (TRUE)` whose body continues in IL order does not fall through (S1 vs S2/S3).
- **A named single-use real is forwarded.** VC7 forwards a single-use `real edge_length = square_root(x)` into its use (b0). January's early `fsqrt; fstp [mem]` therefore implies two uses.
- **A33 does not generalize.** Indexed access instead of a 20-use struct pointer alias costs a frame cell here (S4).

## 5. Reopen criterion

Reopen only with a January-evidenced mechanism for:
- the three x87 square/dot term orders (the two `distance3d` expansions must fall into different key classes);
- the strength-reduced edge-cursor bias (+0x18, `edge_vector.j`).

Start from `scratch/workers/tB_path/s3.c`. Its bytes equal January outside these blocks and the alignment pad they cause.

Do not repeat:
- A33 indexed access (S4);
- uniform dead-local counts (mod8 D=1..8);
- the post-loop `if (node_index != NONE)` dispatch (S2);
- b0's NONE-first probe body (S1 layout);
- `real diameter` declared before the if;
- a single-use `edge_length` plus a `margin` local;
- the `distance_to_destination = 0.0f` initializer of the t1 park base (January has no store). S3 keeps `short new_node_index = NONE;`, which emits no store because VC7 threads every NONE path, as January does.

Review note for any future landing:
- **`distance_to_destination` has no initializer.** This is byte-proven. Both reads sit under `state->destination_valid`, which the loop never writes. It is not a runtime uninitialized read, but the reviewer should confirm.
- **Repeated expressions are deliberate.** S3 repeats `(pathfinding_radius * 2.0f)` and `pathfinding_radius / edge_length` instead of naming them. That is what the bytes require.
