# `source/game/game_engine_ctf.obj` — Opus5 150K wave 2 park re-investigation (2026-09-14)

## Result

No source change was made; `source/game/game_engine_ctf.c` is byte-identical to the lane base.

| gate | exact | residual | unwritten | park drift |
| --- | ---: | ---: | ---: | --- |
| baseline | 38 | 1 | 0 | — |
| final (real file) | 38 | 1 | 0 | 0 of 1 |

The point guard passes and the fake-match scan finds 0 leads.

## `_ctf_engine_weapon_update` (parked, unclassified) — exhausted, no reopen

**Measurements.** Target and candidate agree on size, relocations and instruction count; only the normalized hash differs.

| | size | relocations | instructions | normalized sha16 |
| --- | ---: | ---: | ---: | --- |
| January target | 576 | 40 | 188 | `0c8e2649895a583167` |
| current candidate | 576 | 40 | 188 | `bff168bc4a51e834` |

All 78 differing byte positions are in the idle-return block (`+0x13A..+0x19A`).

**Ledgers read.**
- `game_engine_ctf_obj_codex_closeout_20260903` (91 trial objects).
- `small_family_residual_fixed_point_reaudit_20260913`.
- The Fable and Jonas CTF ledgers.
- The parked.json criterion.
- The HCEA reconstruction. It is a later 360 build, which computes the opposing team inline in the call.

**What January does in the idle-return block.**
1. It sign-extends the owner team into EDI (`movsx`).
2. It computes the opposing team into EBX.
3. Only then does it test the handled flag.
4. After the block it reloads `weapon_index` into EDI (`mov edi,[ebp+8]`) for `ctf_reset_flag`'s EDI private ABI.

**Probes (two, scratch only).** No new compiler law applies, so these only confirm the recorded fixed point.
- **P1 (`long` team):** reproduces January's `movsx edi`. The opposing team is still computed lazily, after the flag test. `weapon_index` becomes cached in EBX from the prologue onward, which moves the zero register to EDI and causes 93 diff lines.
- **P2 (`long` team, opposing team computed inline in the call):** identical to P1.

The park and its criterion are unchanged.
