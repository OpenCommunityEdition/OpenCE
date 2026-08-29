# `main.obj` window-count bounded rejection

## Result

`_main_get_window_count` was reconstructed and measured, but no tested typed-C
shape was strict exact. No implementation is retained in `source/main/main.c`;
that file's content hash is identical to HEAD after the experiment.

January is 61 meaningful bytes in a 64-byte function with five REL32 calls.
Its first two boolean calls share an early `return 1` block at `+0x12`; a false
cinematic result falls through to the lower clamp at `+0x18`. The clamp calls
the `short`-returning `_local_player_count` three times, compares AX against
one and four, and sign-extends the final AX result.

## Measured source shapes

The following ordinary-C forms were compiled with the unit's authoritative
VC7 flags and correct visible prototypes:

- separate early returns;
- a combined single-screen/cinematic guard;
- explicit lower and upper clamp branches;
- the project `PIN(local_player_count(), 1, MAXIMUM_WINDOWS)` macro;
- an explicit `else` around the clamp; and
- both `short` and `long` result signatures during ABI diagnosis.

The correct `short local_player_count(void)` declaration reproduced every
AX compare and the target's exact tail from the second count call onward.
However, a `long` result moved the shared `return 1` block after the first
count comparison, while a `short` result used an AX-width constant return.
The `PIN` spelling moved the common return block to the end of the function.
Thus the residue is isolated to compiler control-flow placement/result-width
selection, not guessed field layout, relocation identity, or an unknown
callee ABI.

This candidate is deliberately left unwritten rather than parked or credited.
Future work should begin from the measured block-layout/result-width conflict,
not repeat the guard and clamp spelling matrix above. No assembly, attributes,
pragmas, intrinsics, volatile/register forcing, undefined behavior, compiler
flag changes, object patching, or comparator exceptions were used.
