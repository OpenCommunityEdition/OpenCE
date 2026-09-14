# `actor_looking.obj` — Opus5 100K consolidated lane, wave C (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_looking.c`; target `build/split/source/ai/actor_looking.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, repository `build.ninja` flags via
  `tools/campaign/gate.py`, whole-TU compiles of scratch candidates).
- Lane HEAD `7a3bbb9c8` (after checkpoint 4: actors.h look-graph packet and the complete look
  graph landing `ff1054197`).
- Evidence: January target COFF (byte/relocation decoding with `tools.campaign.alndiff`,
  `relocdiff`, `tinfo`), HCEA `src/blam/ai/actor_look_*.c` (semantics only), map atlas
  `scratch/atlas/actor_looking.obj.txt`.
- Read first: wave-A worker report `scratch/workers/actor_looking.md`, ledgers
  `actor_looking_obj_opus5_100k_20260914.md`, `_private_leaf_reconciliation_20260913.md`,
  `_direction_decoder_natural_reconstruction_20260906.md`, the two `config/parked.json` entries.
  `git log --all -- source/ai/actor_looking.c`: `ff1054197` is the newest body; no other worktree
  has an active report on this file.
- Worker notes: `scratch/workersC/actor_looking.md`; scratch candidates in `scratch/workersC/actor_looking/`.

## 2. Validated baseline and result

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| lane HEAD (baseline) | 9 | 7 | 0 |
| final real file (unchanged) | 9 | 7 | 0 |
| scratch park-reopen candidate `park_reopen_affect_movement.c` | 10 | 6 | 0 |

The real source file is not modified in this wave (no residual became strict EXACT in a landable
form; `_actor_look_affect_movement` is locked by its park and is proposed for reopening instead).
Final gate: guard passed; park drift PASS (decode 1616/99 `bea2ec69…`, affect 128/4 `d096436e…`);
owner census PASS; fake scan 0 leads.

Residual baseline (target padded / candidate size, relocations):

| function | target | candidate | first meaningful divergence |
| --- | --- | --- | --- |
| `_actor_look_update` | 4720 / 159 | 4688 / 159 | +0x186 load order; override-case slot coalescing (+0x6ed) |
| `_actor_look_decode_direction` (parked) | 1632 / 98 | 1616 / 99 | +0xbb: January inlines the first `valid_real_normal3d` |
| `_actor_look_find_random_vector` | 608 / 35 | 608 / 35 | +0x19 `xor bl,bl` + loop-top test (flag-condition `for`) |
| `_actor_look_idle_find_prop` | 608 / 20 | 608 / 20 | +0x8d `cmp ebx,[ai_debug+0x38]` vs `mov eax,[..]; cmp ebx,eax` |
| `_actor_look_valid_look_vector` | 240 / 6 | 240 / 6 | +0x6 inline normalize2d product order |
| `_actor_look_valid_aim_vector` | 128 / 3 | 128 / 3 | +0x6 inline normalize2d product order |
| `_actor_look_affect_movement` (parked) | 128 / 4 | 128 / 4 | +0x62 `mov eax,1` / store al vs constant stores |

## 3. Accepted controls

None new in production. Proposed (park-reopen, scratch only):

- `actor_look_affect_movement`: January's two return tails store constants
  (`mov byte [esi+0x505],1` / `mov byte [esi+0x505],0`); the park's body assigns the `&&` chain as a
  boolean expression, which VC7 materialises into `eax`. The if/else with `TRUE`/`FALSE` stores is the
  source form those constant tails decode to. First form tried: strict EXACT 128/4, normalized sha
  `5d7d253b…` == target; every other row unchanged; decode park unchanged. The park's reopen criterion
  (complete caller context) is met: `actor_look_update` and all five decode call sites are now written.

## 4. Experiment matrix (whole-TU gates; siblings and parks re-checked each run)

| function | shape | result |
| --- | --- | --- |
| affect_movement | if (…&&…&&decode) { flag = TRUE; } else { flag = FALSE; } (a1) | EXACT; parked, proposed |
| decode_direction | `if (result && !valid…)` + separate `if (result)` assert (d1) | identical sha |
| decode_direction | LEARNING PROBE ONLY: unconditional top-level check (d2, semantically wrong) | helper inlined, 1632/100 |
| decode_direction | `if (!result) return result;` then top-level check (d3) | 1600/100, still out of line |
| decode_direction | `boolean valid = valid_real_normal3d(direction); if (!valid)` (d4) | identical sha |
| decode_direction | `if (result != FALSE)` (d5); `valid_real_normal3d(direction) == FALSE` (d6) | identical sha |
| update | `face_exactly` initialised from the compare, not copied from `change_facing` (u1) | 4704/159 (+16) |
| update | u1 + cosine assignments after the boolean initialisers (u2) | 4704, January store order |
| update | u2 + `secondary_aim_valid` at block scope (u3) | identical sha (inert) |
| update | u2 + priority declarations before cosines (u4) | identical sha (inert) |
| update | u2 + `facing_free` before `looking_free` (u5) | 4704, January load order at +0x186 |
| valid_aim_vector | j-then-i 2D copy (a2) | i*i first like January, loads j->esi / i->eax; [sha] |
| find_random_vector | wave-A for+break/do/while forms re-measured (f3,f4,f5,f6,f7,f10,f11) | for+break: January CFG/layout but base_vector leaves EAX (major/minor lose exact); do/while: 544/34 |

## 5. Do-not-repeat list

- decode_direction: result-gating spellings d1, d3–d6 all leave the diagnostic guard out of line;
  do not retry `&&`/nested/early-return/named-local/`!= FALSE`/`== FALSE` variants.
- update: declaration order and block scope of locals are inert (u3, u4); keep u1 and the store order
  of u2/u5 in any future attempt.
- aim/look: ten spellings across two waves; the copy-order lever only swaps which of (load order,
  multiply order) matches.
- find_random_vector: all measured loop keywords/break/return forms; see wave-A matrix.
- find_prop: operand order, `||`/if-else validity, `(real)game_time` already measured in wave A.

## 6. Residual classification

- decode_direction: **inline-frequency boundary** (fact: moving the guard to unconditional top level
  inlines it and restores 1632 bytes; every result-gated form measured keeps the call). Inference:
  January's guard sits at a frequency no measured natural gate reaches.
- update: **register/slot allocation tie after structural fixes** (fact: identical relocation
  multiset; January keeps `secondary_aim_valid` in `[ebp-7]` and gives `ebx` to the
  `&desired_facing_vector` CSE inside the secondary switch; ours holds it in `bl`, swapping
  `ebx`/`edi` roles and the `aiming_cosine`/`primary_priority` slots).
- find_random_vector: **register-ABI tie** (for+break matches the CFG but not the register-parameter choice).
- find_prop: **register scheduling** (one load/compare pair).
- aim/look: **x87 evaluation-order tie** inside the header inline `normalize2d`.

## 7. Reopen criteria

- affect_movement: orchestrator unpark + land `scratch/workersC/actor_looking/park_reopen_affect_movement.c`.
- decode_direction: a natural result-gated spelling weighted at function-top frequency (evidence of a
  macro/loop wrapper or different control structure in January's source for the diagnostic).
- update: a source reason for `secondary_aim_valid` to stay in memory (extra January use/def of it, or of
  the desired-facing pointer, in the secondary switch); start from `u5.c`.
- aim/look, find_prop, find_random_vector: new January source/topology evidence only.

## 8. Disposition

Still active. No new exact credit landed in the real file this wave. One strict-EXACT park-reopen
proposal (`_actor_look_affect_movement`, 128 padded / 122 meaningful / 4 relocations). Parks unchanged.
