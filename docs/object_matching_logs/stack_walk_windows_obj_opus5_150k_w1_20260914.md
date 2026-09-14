# `stack_walk_windows.obj` — opus5 150K house-clean lane, wave w1 (2026-09-14)

## Result

No landing. `source/cseries/stack_walk_windows.c` is unchanged, and `git diff --stat` for it is empty.

| gate | exact | residual | unwritten | guard |
| --- | ---: | ---: | ---: | --- |
| baseline | 9 | 7 | 0 | pass |
| final (real file) | 9 | 7 | 0 | pass |

- Meaningful exact bytes gained: **0**.
- No parks exist for this unit.
- The owner census is unchanged, because the file is unchanged.
- `tools/fake_match_scan.py` reports the same 2 inherited leads: the `walk_stack` EBP/ESP `__asm` captures.

The main new finding: **one unrecovered source construct blocks a 1,200-byte cluster.** The cluster is `is_valid_ebp` 32, `walk_up` 64, `walk_stack_context` 208, `walk_stack` 208 and `stack_walk_with_context` 688. All the other structural gaps in that cluster were solved in scratch.

## Evidence used

- January COFF `build/split/source/cseries/stack_walk_windows.obj`.
- January map atlas: `@walk_stack_context@16`.
- Ghidra decompilation.
- HCEX_Release.pdb, dumped with DIA2Dump `-sym` / `-lines`:
  - `static unsigned char is_valid_ebp()` and `static unsigned long walk_up()`;
  - the walker signatures, and the `stack_walk_with_context` locals `frame_number` / `addresses[0x40]`;
  - no automatic locals listed for `symbol_name_from_address`.
- HCEA reconstructions (`blam/game/is_valid_ebp.c`, `blam/memory/walk_*.c`, `symbol_name_from_address.c`, `stack_walk_with_context.c`), used for semantics only.

## Per-function outcomes

### `_is_valid_ebp` (32; residual sha) and `_walk_up` (64; residual sha)

January emits `mov r2,r1; and r2,3; test r2b,r2b`: a 32-bit AND followed by an 8-bit test.

- A capstone scan of every `build/split` object finds this sequence **only in this object**. There are 8 sites: `is_valid_ebp`, the inlined copy in `walk_up`, and 3 each in the walkers.
- The candidate emits `test al,3`.
- The 32-bit return (`mov eax,1` / `xor eax,eax`) is reproduced by `return !(walk_up_current_frame & 3) && walk_up_current_frame >= (unsigned long)old_ebp;`.

The following spellings all fold to `test al,3`. They were tried in whole-TU gates and in tiny-TU lab compiles with the unit's exact cflags.

**Condition and return shapes:**
- if / `||`;
- `&&` returns;
- a single-exit flag;
- `?:`;
- `switch`;
- `==TRUE` / `==FALSE` comparisons.

**Operator spellings:**
- `% sizeof`;
- a `TEST_FLAG` pair;
- commutative `3 & x`;
- `(x>>2)<<2`;
- `/4*4`;
- `(x+3)&~3`.

**Types, casts and storage:**
- `(boolean)`, `(byte)` and `(char)` casts;
- byte, char and unsigned locals;
- a pointer-typed frame global;
- int and byte bit-fields, including through a union;
- a const or variable mask.

**Other forms:**
- static and `__inline` helper functions;
- assignment inside the condition.

For comparison only (not admissible), compiler flags were also varied: `/Og-` and `/Od` give a 32-bit `test eax,eax`, and every other optimisation flag still folds.

**Key measurement (scratch only).** Any walker-side spelling that keeps a copy register for the `& 3` makes `_stack_walk_with_context` strict EXACT (688 / 49). It also makes both walkers 208 / 208 padded. The spelling used for that measurement, `(x & 3) < 1`, is not credible and was not proposed.

### `_walk_stack_context` / `_walk_stack` (208 target vs 176)

The 32-byte gap comes from three things:
- the private ABI;
- the extra callee-saved `ebx`;
- the three test sites that follow from the `& 3` construct.

The target ABI is `ecx` = ignore_levels, `edx` = routine_addresses, then number_of_levels and levels_dumped on the stack.

The target ignore loop is `test ecx,ecx; je; dec ecx; je; loop … dec ecx; jne`. Among seven loop spellings (L1, L3–L9), **only** `if (ignore_levels) { while (--ignore_levels) { walk_up(); } }` reproduces it. With that spelling, the walkers differ only by the `& 3` form and the loop-alignment padding that follows from it.

A `__fastcall` declaration, as the atlas decoration suggests, changes only the COFF name. The body bytes are identical.

### `_stack_walk_with_context` (688 / 49 target vs 672 / 51)

These caller-side edits are credible and are all target-proven:

- **Instruction bytes.** Four locals in the context block: `instruction`, then `instruction & 0xFF`, `(instruction >> 8) & 0xFF`, `(instruction >> 16) & 0xFF` and `instruction >> 24`, computed before the register error calls (store/call order). This restores the `sub esp,0x108` frame and the context block.
- **Final loop.** `if (!error_stream) error(...) else fprintf(...)`, with `routine_addresses[frame_number]` used directly and the symbol-name `?:` inline in the call argument. This gives one call site each, and relocs become 49.
- **"Mat" loop.** `routine_address = routine_addresses[frame_number] + *(long *)(routine_addresses[frame_number] - sizeof(long));` is exact.

With these edits (scratch `scratch/workers/stack_walk_windows/m1base.c`), the only remaining difference is the walker call ABI, and that depends on the `& 3` construct. The edits were not applied, because an already-written residual may land only when it is strict exact.

### `_symbol_name_from_address` (176 / 9; residual sha)

The only difference is the load order of `fixup + fake_address`: January loads the global first. Seven shapes were inert or worse:
- operand swap;
- assignment before or after `csstrcpy`;
- `long` address or parameter;
- `fake_address +=`;
- a TU prototype.

This is classified as a commutative load-order tie.

### `_load_symbol_table` (1568 / 93 target vs 1568 / 94)

January's header-scan loop is not rotated, so the `fgets` call is not duplicated. A flag loop reproduces the header loop exactly and fixes the relocation count (93 = 93):

```c
while (!found_symbols_section)
{
	/* fgets or error; "Lib:Object" sets the flag; else "Timestamp" check */
}
```

The size is then 1552 against 1568, so it was not applied. The remaining gap is the zero-register pin: January keeps `edi` = 0 through the symbol loop and then reuses it for `rva_base`. The two 256-byte arrays also sit in opposite slots. Array declaration order is inert (confirmed again). The scratch source is `h4base.c`.

## Rejected / not-landed shapes (do not repeat)

- Every `is_valid_ebp` spelling listed above.
- `symbol_name_from_address` load-order shapes sn1–sn7.
- Header-loop `for (;;)` and `do … while (TRUE)`.
- Array declaration permutations.
- Walker loops L1, L3, L4, L5, L7, L8, L9.

## Proposals

Names only, zero credit, optional. The atlas records `@walk_stack_context@16`, but `config/symbols.json:1676` names it `_walk_stack_context`. Rename the symbol only if the source adopts `static void __fastcall walk_stack_context(...)`. The measured bytes are unchanged.

## Reopen criteria

1. **Walker cluster.** Find a credible spelling that VC7 13.00.9254 lowers to `and r32,3; test r8,r8`. Then apply together: that spelling, the L6 loop in both walkers, and the `m1base.c` caller edits. Gate the whole TU.
2. **`load_symbol_table`.** Find a source lever for the zero-register pin or the array slot order, and carry the h4 flag loop forward.
3. **`symbol_name_from_address`.** Find new provenance for how the address is computed.
