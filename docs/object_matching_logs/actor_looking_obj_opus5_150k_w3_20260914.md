# `actor_looking.obj` — Opus5 150K house-clean lane, wave w3 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_looking.c`; January target `build/split/source/ai/actor_looking.obj`
  (VC7 13.00.9254, repository flags through `tools/campaign/gate.py`).
- Only scratch copies were edited. The real file is unchanged (`git diff --stat -- source/ai/actor_looking.c` empty).
  No header or config edits.
- Read first: the worker brief, `scratch/w2/laws_w2.md`, house rules, methodology, and every actor_looking ledger
  (`_opus5_150k_w1`, `_opus5_100k`, `_opus5_100k_waveC`, `_direction_decoder_natural_reconstruction`,
  `_private_leaf_reconciliation`, `_validity_owner_boundary`, `_jonas_two_body`, `_jonas_secondary_look_research`).
  Also read the w1 worker notes and the 150K handoff. No w2 ledger exists for this object.
- This is a resumed w3 run. The first w3 session was cut off after measuring decode candidates in scratch without
  reporting them. Those files were re-verified, and the proposal below was rebuilt from the current real file.
- Worker notes: `scratch/workers/w3_actor_looking.md`. Scratch: `scratch/workers/w3_actor_looking/` (resume run in `r3/`).

## 2. Baseline and result

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (real file) | 11 | 5 | 0 |
| final real file | 11 | 5 | 0 |
| park-reopen candidate `r3/pr1.c` | **12** | 4 | 0 |

- **Final real-file gate:** identical to the baseline, and the `_point_from_line3d` guard passed.
- **Park drift:** 0 (decode 1616/99).
- **Owner census:** PASS.
- **fake_match_scan:** 0 leads.
- **Newly exact in the real file:** none.

## 3. Park reopen proposal: `_actor_look_decode_direction` (1632 padded / 1624 meaningful / 98 relocations)

Candidate: `scratch/workers/w3_actor_looking/r3/pr1.c`. It is the current real file with only the decode body changed.

- **Decode row:** strict EXACT, normalized sha `4f3714fab510f0c4…` == target.
- **Every other row:** identical to the baseline gate. The four remaining residuals keep their normalized hashes:
  find_prop `b145a6e8`, update `472c1784`, aim `4a0b82e0`, look `bae54ff4`.
- **Checks:** guard passed; owner census PASS; fake scan 0 leads. The only park is decode itself, and its drift is
  the reopen.

### Changes and evidence

Three independent January facts support the change. None comes from an equivalent-spelling search.

1. **Case order.** Tinfo relocations of both seven-entry jump tables show the target block before the prop block.
   The codegen-levers "jump-table order = source case order" law applies.

   | Table | Index 0 (movement) | Index 1 (prop) | Index 2 (target) |
   | --- | --- | --- | --- |
   | dispatch (+0x620) | 0x7B | 0x1E3 | 0x16E |
   | diagnostic (+0x63C) | 0x321 | 0x4B5 | 0x394 |

   The source order is therefore movement, target, prop, point, vector, danger, object. The 2026-09-06
   reconstruction had used enum order. The swap alone moves the size (1600/97), so the block order is structural.
2. **The first normal check is not `valid_real_normal3d`.**
   - January has no call at +0xBB. It inlines `valid_realcmp` over the squared magnitude and keeps that value on
     the x87 stack.
   - The line-529 assert still calls `valid_real_normal3d` out of line.
   - Wave C d1–d6 and w1 dd1 proved that every `valid_real_normal3d` spelling at this depth stays out of line
     (frequency, not budget).
   - The cold diagnostic computes the printed magnitude from the retained value (`fstp st(0); fsqrt`, +0x307/+0x309).
   - The candidate spells this intent: compute the squared magnitude once, test it, and reuse it for the printed
     magnitude.
     `real magnitude_squared = magnitude_squared3d(direction); if (!valid_realcmp(magnitude_squared, 1.0f))
     { real magnitude = square_root(magnitude_squared); …`.
   - Also strict EXACT but not proposed: `valid_realcmp(magnitude_squared3d(direction), 1.0f)` with
     `magnitude3d(direction)` (`c1.c`). It hand-expands the helper body (house rule 13).
3. **Ternary polarity from arm layout** (an A16-class control-flow fact).
   - In January's prop diagnostic, both `?:` share one diamond, and the fall-through (true) arm is the NULL case:
     `test eax,eax; jne +0x4d2; mov ecx," (invalid)"; jmp; +0x4d2: mov eax,[eax+4]; mov ecx,""`.
   - `prop ? "" : " (invalid)"` makes the non-NULL arm the fall-through, and the function stays `[sha]`
     (`c2.c`/`p3.c`). Inverting only the owner ternary is also `[sha]` (`p4b.c`).
   - January therefore wrote the NULL guard first: `!prop ? " (invalid)" : ""` and `!prop ? 0 : prop->owner_actor_index`.
   - Same-subsystem January precedent, strict EXACT: `encounters.c` `_encounter_update_follow` (1920) prints
     `!current_squad_definition ? "<none>" : current_squad_definition->name`.
   - The movement `moving ? "" : "not "` ternary is unchanged.

**Park criterion.** The park asks for "authentic source/type/complete-caller-context evidence, not equivalent-spelling
or emission-policy searches". Facts 1–3 above are January code/relocation facts. The caller context is also complete:
update and affect_movement have both been written since the park was created. The preserved movement x,z,z bug
comment and the failed-prop owner-0 behaviour are unchanged.

### Measured decode shapes (w3 total, both sessions)

| Shape | Result |
| --- | --- |
| s1: case swap only | 1600/97 |
| p1/p2: named magnitude_squared (p2 also hoists `prop`) | 1648/100 |
| p3: swap + magnitude_squared | 1632/98 [sha] |
| p4b: p3 + owner ternary inverted | [sha] |
| c2: swap + `valid_realcmp(magnitude_squared3d())` | [sha] |
| **p4a = pr1: swap + magnitude_squared + both ternaries** | **EXACT (proposed)** |
| c1: swap + `valid_realcmp(magnitude_squared3d())` + `magnitude3d` + both ternaries | EXACT (not proposed, rule 13) |

## 4. Other residuals (law detector pass, no shapes spent)

| Function | Classification | Law detector result | Reopen |
| --- | --- | --- | --- |
| `_actor_look_update` 4688 vs 4720 | register/slot allocation | A1/A3/A5/A6/A7/A8/A9/A10/A12/A13/A16/A20 give no specific lever (see note 1) | a source reason for `secondary_aim_valid` to stay in memory; start from `scratch/workers/actor_looking/v2.c` (4704) |
| `_actor_look_valid_aim_vector` | x87 sum-term order inside inlined `normalize2d` | A22 YOUNGER OPERAND considered (see note 2) | a January-side reason the copies are not forwarded into the inline |
| `_actor_look_valid_look_vector` | same, plus the final `dot_product2d` operand order (see note 3) | A22 would need an R3 commutative swap (inadmissible) | same lever as aim; land `p_look_l2.c` only together with it |
| `_actor_look_idle_find_prop` | one load/compare scheduling at +0x8d (first debug check) | A3 inert (w1); A7 `#include "game/game.h"` for the C4013 `game_time_get` is byte-inert across the whole TU | new January topology evidence |

Notes:

1. **`_actor_look_update`.** The frame is equal (0x58), the relocation multiset is identical, and v2 has the same
   instruction count as January. All 66 register-normalised blocks are allocation differences, for example ebx
   carrying the desired-facing CSE versus `secondary_aim_valid`.
2. **`_actor_look_valid_aim_vector`.** January's i→ecx, j→eax load order is forced because eax holds the pointer
   parameter, so it is not source-order evidence.
3. **`_actor_look_valid_look_vector`.** At +0xb9 January computes `looking.i*attempted.i` first and then
   `attempted.j*looking.j`.

## 5. Do-not-repeat

- **decode:** the table in section 3. Do not reopen the result-gated `valid_real_normal3d` family.
- **update:** everything listed in the w1 ledger, plus the cut-off w3 u_s1–u_s4 (scope moves), m1 (`minor_valid`
  flag), m2a (default-first aiming-speed switch) and m2b (inverted aiming-speed if). All are inert or 4688.
- **find_prop:** the a7 include probe (inert).

## 6. Proposals

- **park-reopen:** `_actor_look_decode_direction`, candidate `scratch/workers/w3_actor_looking/r3/pr1.c` (section 3).
- **Hygiene note (zero credit, no header change):** `source/ai/actor_looking.c` could include `game/game.h` for
  `game_time_get` (C4013 at HEAD). It is measured byte-inert, so it was not landed.
