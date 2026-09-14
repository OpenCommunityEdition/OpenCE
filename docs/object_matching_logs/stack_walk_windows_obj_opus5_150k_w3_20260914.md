# stack_walk_windows.obj — Opus5 150K house-clean lane, wave w3 (2026-09-14)

## Outcome

`source/cseries/stack_walk_windows.c` is **unchanged** (`git diff --stat` empty). Gate before and after: 9 exact / 7 residual / 0 unwritten.
The guard passes, the unit has no parks, and the fake scan shows the same 2 inherited `walk_stack` EBP/ESP capture leads. Meaningful
exact bytes gained: 0.

## Duplicate prevention

Read: the w1 ledger and `scratch/workers/stack_walk_windows.md`, plus the abi_name, pdb_ida, loader_cluster_withheld and jonas ledgers,
and `laws_w2.md`. `_symbol_sort_proc` is inherited and was not touched.

## Walker cluster (1,200 B): `_is_valid_ebp` 32, `_walk_up` 64, `_walk_stack_context` 208, `_walk_stack` 208, `_stack_walk_with_context` 688

This is still blocked by the same unrecovered construct: January's frame-alignment test `mov r2,r1; and r2,3; test r2b,r2b`.

**New evidence: image-wide census.**
- A capstone scan of every January split object for `and r32,imm` followed within 3 instructions by `test r8,r8` on the same register (any
  mask) finds only 9 sites:
  - the 8 sites in this object;
  - one XDK library site, `xsaveapi.obj _XapiFillInSaveGameData@12` (`and eax,0x10; test al,al`).
- The same scan over every object this tree compiles (`build/base`) finds **zero** sites, so no exact reconstruction anywhere shows a
  source spelling for the construct.

**Lab probing** (tiny TU with the unit's cflags; mechanism probes, not landing shapes):
- A generated lab of 960 functions:
  - 12 integer types × cast positions (operand, result, mask) × {`& 3`, `% 4`};
  - × 8 test spellings × {direct, boolean local}.
- Relational forms `>0`, `>=1`, `<1` and `<=0`.
- Algebraic forms `x-(x&~3)`, `x-x/4*4`, `x^(x&~3)` and shifts.
- A static const mask and byte-typed switch lowering.
- 32-bit `long` result flags, for the A3(c) return-width detector.
- A flag sweep: /Os /O1 /Ox /Og- /Oi- /Ob0 /G5 /G6 /GL /Zi /Z7 /Op /Oa /Ow /Gy- /GF /GX /Gm /RTCc /GZ /Ge /Gs0 /J /Za /Ot /Oy.

Results:
- Every optimized form folds to `test al,3` or to a byte `and cl,3; cmp cl,1`.
- `/Og-` leaves the and/test pair unfolded, but as a 32-bit or `movzx` form and without global register allocation.
- Nothing reproduces the target.

**Reopen:** unchanged from w1. Find a credible spelling VC7 13.00.9254 `/O2` lowers to the construct, then apply it together with w1's
`m1base.c` caller edits and the L6 walker loops, and gate the whole TU.

## `_symbol_name_from_address` (176 / 9, sha)

This is a commutative load-order tie: January loads `stack_walk_globals.fixup` before the parameter. One new probe, sn8, gave the
TU-local struct field type `unsigned long fixup` (it holds `rva_base - (unsigned long)load_symbol_table`). It was byte-inert across all 16
rows. Exhausted: w1 sn1-sn7 plus sn8.

## `_load_symbol_table` (1568 / 93; candidate 1568 / 94)

The analysis started from w1's h4 flag loop (93 relocs, 1552). An alignment against January found three differences that w1 did not
record:
1. January emits the name-token-present edge as a fallthrough block. That block duplicates the `strncpy` argument pushes and jumps into the
   shared call.
2. January pushes each error literal in its predecessor block before a shared `push 2; call error; free_symbol_table`.
3. January pins zero in EDI across the loop header (`push edi` / `cmp eax,edi` / `mov [ebp+0xc],edi`), while `string_storage_used` has a
   stack home. Our build enregisters it in EDI.

Shapes:

| id | shape | result |
| --- | --- | --- |
| s1 | a separate `error()` and `free_symbol_table()` under each label | 1568 but 95 relocs; VC7 does not cross-jump calls |
| s2 | a dedicated `error_message` local instead of reusing `token` | 1552/93, identical to h4 |

Both were reverted. The residual is register allocation: the stack home of `string_storage_used`, which frees EDI for the zero pin, plus
the codegen block placements that follow from it.

**Reopen:** a lever for that stack home; carry h4 forward.

## Proposals

None. The w1 names-only `@walk_stack_context@16` note still stands and carries zero credit.
