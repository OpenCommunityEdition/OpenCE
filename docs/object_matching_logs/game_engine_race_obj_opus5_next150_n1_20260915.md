# `game_engine_race.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

## Result

No source change. `source/game/game_engine_race.c` is byte-identical to the lane
baseline `ae10935da`; `git diff --stat` for the file is empty.

| gate | exact | residual | unwritten | point guard | park drift |
| --- | ---: | ---: | ---: | --- | --- |
| baseline (HEAD) | 35 | 1 | 0 | pass | — |
| final (real file) | 35 | 1 | 0 | pass | 0 of 1 |

`tools/fake_match_scan.py`: 0 review leads.

## `_race_engine_get_score` (parked, unclassified) — register-web rotation

Target 224 padded / 215 meaningful / 4 relocations; candidate 224 / 4, `[sha]`
only, 92 instructions on both sides. Frames are equal (frame-less, same
`push ebx/esi/edi`), and the relocation census by target is identical
(`_player_data`, `_datum_get`, `_race_globals+136`, `_race_globals+68`). There is
no structural gap.

### The whole residual, value by value

| value | January | ours |
| --- | --- | --- |
| `lap_bit_vector[team]` | EDX (volatile), loaded at `+0x2d` BEFORE the callee-saved pushes | ESI, after the pushes |
| `laps` (`movsx` of the short field `+0xc2`) | EDI, read at `+0x37` between the bit-vector load and the count loop | EBX, read at `+0x2b` before the team index |
| inlined `count_bits_32` result | ESI | EDI |
| bit mask | EBX | EAX |
| unrolled bit induction variable | EAX | EDX |
| team index | ECX (`mov ecx,[eax+0x20]`, keeps the player pointer alive in EAX) | EAX (`mov eax,[eax+0x20]`, kills the player pointer) |

The permutation is anchored by one decision: January loads the team index into
ECX so that EAX still holds the player datum for the `laps` read, and therefore
places the bit vector in a volatile register before any callee-saved push. Ours
consumes EAX for the team index and must take callee-saved ESI.

### Shapes measured (5, scratch only)

1. `rs_r1.c` — swap the two statements (count first, laps second). Strictly
   closer in part: `mov ecx,[eax+0x20]` and the EBX bit mask now match January,
   but the `laps` read sinks past the whole unrolled loop to `+0xc6`, where
   January keeps it live in EDI across the loop. Still `[sha]`.
2. `rs_r2.c` — same plus swapped declarations: byte-identical to r1.
3. `rs_r3.c` — name the argument (`long lap_flags = race_globals.lap_bit_vector[...]`)
   and call `count_bits_32(lap_flags)` in the return: **96 bytes**. Naming the
   argument stops `count_bits_32` from being inlined and unrolled. This refutes
   the "January named the bit vector" hypothesis and reproduces the failure the
   2026-09-02 ledger recorded.
4. `rs_r4.c` — named argument plus named count: 96 bytes, same cause.
5. `rs_r5.c` — `laps` local with the count inlined into the return expression:
   byte-identical to HEAD, re-proving the fixed point.

### Reopen criterion

A source form that reads `laps` AFTER the bit-vector element but BEFORE the
inlined count loop — January's exact interleave — without naming the helper
argument (which kills the inline). Neither statement order achieves it:
laps-first reads it too early (and kills the player pointer), count-first sinks
it past the loop.

## Checks

- Final real-file gate with the `_point_from_line3d` guard: 35 / 1 / 0, unchanged.
- `scratch/parkcheck.py source/game/game_engine_race`: parks 1, drift 0.
- `tools/fake_match_scan.py source/game/game_engine_race.c`: 0 leads.
