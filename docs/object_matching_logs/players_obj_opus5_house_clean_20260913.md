# `players.obj` Opus5 house-clean lane — 2026-09-13

## 1. Scope and provenance

- Translation unit `source/game/players.c`; target `build/split/source/game/players.obj`.
- Lane worktree `opus5-50k-house-clean-20260913`, pinned base `73e97eca5`.
- Compiler: repository XDK 3911 `CL.Exe` (VC7 13.00.9254), flags unchanged
  (`/O2 /Oy- /DDEBUG /Dxbox`); no `--cflag` used.
- Only `players.c` was edited. No header, config, `symbols.json`, park entry or
  tool changed.
- Evidence used: January target bytes/relocations/literals (authority 1), the
  January `ai.obj` target as a same-build control, standalone VC7 probes under
  `scratch/workers/players/probe/`, HCEA/Stian lifts for semantics only.
- Prior ledgers read in full: `players_obj.md`,
  `players_obj_fable_exact_packet_reconciliation_20260913.md`,
  `players_obj_january_safe_harvest_20260902.md`,
  `players_obj_jonas_residual_freshness_audit_20260830.md`,
  `players_obj_jonas_source_credibility_cleanup_20260830.md`,
  `docs/players_matching_notes.md`, and the Players rows in
  `opus_wave_canonical_reconciliation_20260902.md` and `config/parked.json`.

## 2. Validated baseline and result

| Gate | Result |
|---|---|
| Baseline (fresh, real file) | `== exact 54  residual 15  unwritten 1  (of 70 listed)` |
| Worker final (before lead review) | `== exact 56  residual 13  unwritten 1  (of 70 listed)` |
| **Admitted final** (real file, `--forbid-emitted-symbol _point_from_line3d`) | `== exact 55  residual 14  unwritten 1  (of 70 listed)` |

**Lead admission correction (2026-09-13).** The independent adversarial review
REJECTED the `player_examine_nearby_vehicle` edit described in the worker notes
below: it introduced a `test_vehicle` alias duplicating the still-live `vehicle`
pointer (the prior reconciliation ledger explicitly rejects a duplicate
`vehicle_get`, and its reopen criterion — first-party source or an ordinary
legal-C form supported independently of matching output — is not met), and it
changed January's inline schedule by letting VC7 inline
`player_set_action_result` at the constant flip call site. That hunk was
**fully reverted**. `_player_examine_nearby_vehicle` is back to its baseline
`448/25 [sha]`, and `_player_examine_nearby_objects` (which was exact only
through the callee's changed private ABI) is back to its baseline residual.
Neither receives credit.

**Newly strict exact and admitted (1 function, 1,760 padded / 1,754 meaningful bytes):**

| Function | Padded / meaningful | Relocs | Normalized SHA-256 (first 16) |
|---|---:|---:|---|
| `_players_update_before_game` | 1760 / 1754 | 101 | `8c92baaaebf7177d` |

Rejected (reverted, zero credit): `_player_examine_nearby_objects` 240 / 233.

Checks on the final object: emitted-symbol guard passed; no baseline-EXACT row
lost; all four Players parks equal their `measurements.base`
(size/relocations/normalized SHA); code/data owner census identical to baseline;
string-literal census: the seven January-only assert literals are now owned and
no new candidate-only literal appeared; `tools/fake_match_scan.py` 0 leads (same
as baseline).

Residual table (target / baseline / final — size / relocs / nSHA16):

| Function | January | Baseline | Final |
|---|---|---|---|
| `_player_examine_nearby_unit` | 16 / 0 / `499f1f307c1cb989` | absent | absent |
| `_player_update_powerups` | 112 / 5 / `826a864ce09f2800` | 112 / 5 / `b5a7af2d83089af4` | unchanged |
| `_player_handle_powerup_minor` | 160 / 9 / `27e1638518a37796` | 160 / 9 / `b6b1d0d35d027042` | unchanged |
| `_player_examine_nearby_device` | 176 / 9 / `f95e7e1cc6a51fcf` | 160 / 9 / `65abfe19e8da82c5` | unchanged |
| `_player_set_action_result` | 240 / 5 / `ab7ce57f70ded3fe` | 240 / 5 / `bc9bed2cb0fa36f1` | unchanged |
| `_player_handle_powerup` | 240 / 14 / `84c28544d684b20d` | 256 / 14 / `6391d7e23fe8e68c` | unchanged |
| `_player_examine_nearby_objects` | 240 / 18 / `a08d292a1a63cf83` | 240 / 18 [sha] | unchanged (rejected closure reverted) |
| `_player_examine_nearby_vehicle` | 448 / 25 / `d68f377ed87e85d1` | 448 / 25 / `dcac787d00a4b5ec` | unchanged (rejected 464 / 26 variant reverted) |
| `_player_examine_nearby_item` | 800 / 33 / `f1f67bf1b9c419df` | 816 / 33 / `dd2e4f23cb671581` | unchanged |
| `_players_update_after_game` | 816 / 52 / `af64f40efdb1be0c` | 800 / 52 / `6407fdea5b288e67` | unchanged |
| `_player_teleport_internal` | 1296 / 61 / `8bb48b429649a3af` | 1312 / 62 / `6cf72b07a9eda1d6` | unchanged |

## 3. Accepted controls (retained in production)

### `players_update_before_game` (EXACT)

1. **Assert text restored from January literals.** The target owns seven
   `.rdata` strings our build did not: `action_index>=0 && action_index<NETWORK_GAME_MAXIMUM_PLAYER_COUNT`
   and six `(NONE == x) || ((x >= 0) && (x <= MAX))` /
   `(NONE == x) || ((x >= 0))` forms for `action->desired_*` and
   `control_data.*`. `match_assert` stringizes its expression, so these
   literals are January's source text; the expressions were respelled to
   stringize identically.
2. **Accessor re-fetch.** January has two `object_get_and_verify_type(unit_index, 3)`
   relocations; the second (+0x40C) follows `unit_delete_current_equipment`,
   reloads `player->unit_index` and reads `current_weapon_index` from its
   result. The source passes `unit_get(player->unit_index)->unit.current_weapon_index`
   to `unit_inventory_get_weapon`. VC7 does not CSE accessor calls; the accepted
   `player_examine_nearby_item` already re-fetches the unit before the same call.
3. **Direct `unit_control` calls.** January's join block computes
   `player->unit_index` before pushing the control-data pointer, reached by
   `lea eax,[ebp-0x68]; jmp` from the player-input branch and by fall-through
   after `lea eax,[ebp-0xa8]` from the inhibited branch, with the "no control"
   branch skipping the call. That is the cross-jumped tail of two identical
   direct calls; the `control_data_pointer` NULL-or-address variable (pushes the
   pointer first) was removed.

### `player_examine_nearby_objects` — REJECTED at admission review, reverted

The worker's analysis below is retained as research only. It is **not** in
production source.

Objects' body was already text-correct; its only defect was an ESI/EDI swap
caused by the private register ABI of `player_examine_nearby_vehicle` (January
passes the vehicle index in ESI). Callee register pressure decides that ABI.
The vehicle body now performs January's third vehicle-typed lookup as a
statement between the flipping-angle assignment and the orientation test
(`test_vehicle = vehicle_get(vehicle_index)`), proven by three
`object_get_and_verify_type(vehicle_index, 2)` calls at +0x1B/+0x59/+0xCB, the
up.k read from the second fetch's EAX while the flip branch uses the first
pointer, and the flipping angle being spilled across that call with `fcos`
applied afterwards. This reopens the reconciliation ledger's "duplicate
vehicle_get" rejection on January call-count evidence alone; nothing else from
the Fable donor (noinline, raw offsets, numeric enums) was taken.

Side effect recorded honestly: `_player_examine_nearby_vehicle` itself moves
from `448/25 [sha]` to `464/26 [size, relocs, sha]`; its instruction stream now
matches January except that VC7 inlines `player_set_action_result` at the
constant `_player_action_result_flip_vehicle` call site.

## 4. Experiment matrix (this lane)

| Function | Shape | Result | Kept |
|---|---|---|---|
| vehicle | inline `vehicle_get(..)->object.up.k > cosine(..)` | objects EXACT; vehicle 464/26, fcos before the call | superseded |
| vehicle | operand order / no flipping_angle local | byte-identical to previous row | no |
| vehicle | statement re-fetch `test_vehicle` | objects EXACT; vehicle x87 schedule equals January; 464/26 (flip inline) | landed by worker, **rejected by review and reverted** |
| vehicle (diag) | non-flip constant at flip site | no inline -> tail equals January except constant | diagnostic |
| vehicle (diag) | `player_set_action_result` defined after all callers | byte-identical | diagnostic |
| set_action_result | inline `distance3d(..) > distance3d(..)`, no locals | [sha]; first-sum order z²,y²,x² vs January z²,x²,y² | no |
| set_action_result | early `return;` instead of `set_action` flag | [sha]; vehicle flip inline unchanged | no |
| update_powerups | natural indexed loop, short index (do/for) | 96 [size], no strength reduction | no |
| update_powerups | natural indexed loop, long index (do/for, enum or NUMBEROF bound) | 96 [size]; lea + after-call index init as January, but `cmp esi,2` exit instead of homed down-counter | no |
| update_powerups | drop redundant `(short)` cast | byte-identical | no |
| handle_powerup | HCEA if/else ordering; braced else-if | 92 insns, unchanged | no |
| device | named `real radius` after camera call | [sha]; frame 0xc->0x10, integer move | no |
| item | remove / hoist `weapon_item_index` alias | byte-identical 816/33/`dd2e4f23` (alias inert) | no |
| before_game | assert literals | relocs still 100 | base for next |
| before_game | + nested unit re-fetch | 101 relocs, one 4-byte window | base for next |
| before_game | + direct unit_control calls | **EXACT** | **yes** |
| after_game | bind `struct scenario *scenario` for the block address | byte-identical | no |
| teleport_internal | single `match_assert(source_unit_index==NONE \|\| local_player_count()>1)` + one `&&` condition, removing the duplicated `biped_fix_position(position)` else and goto | relocs 61==61, size 1280!=1296, 421/428 insns | no (already-written residual stays fuzzy) |

Probe facts (standalone VC7, `scratch/workers/players/probe/`): a plain `real`
field argument is passed by `mov/push` regardless of `const` parameter, same-TU
definition, redundant cast, or an `__inline` callee that is not inlined; only a
named `real` object or FP expression emits `fld/fstp`. VC7 inlines a static
defined after its caller.

## 5. Do-not-repeat

- Alias placement of the item index in `player_examine_nearby_item` (inert).
- Named radius local in `player_examine_nearby_device`.
- `player_set_action_result` definition order; early-return form; inline distance
  expressions without locals.
- Natural indexed loops for `player_update_powerups` (short or long index, do/for,
  enum or NUMBEROF bound).
- Scenario-pointer binding in `players_update_after_game`.
- Everything in the prior Players do-not-repeat lists (barriers, volatile,
  noinline, copied helpers, widened powerup carriers, aggregate/home permutations
  in teleport).

## 6. Residual classification

- `_player_examine_nearby_unit`: owner/provenance. One-byte `ret` with zero
  in-TU callers against the `boolean (long, long)` ABI in `players.h`; no honest
  C body (fact). Not written.
- `_player_examine_nearby_device`: x87 argument materialization. Measured fact:
  January `ai.obj::_ai_test_line_of_fire` passes the same field via `fld/fstp` to
  `fast_vector_intersects_sphere` and via `mov/push` to `vector_intersects_pill3d`
  in one function. Inference: the cause is the January `fast_vector_intersects_sphere`
  declaration (real_math.h, header-owned).
- `_player_examine_nearby_vehicle`: baseline `[sha]` retained. The re-fetch
  variant exposes a constant-fold-driven inline of `player_set_action_result`
  at the flip site (measured by diagnostic) and was rejected.
- `_player_examine_nearby_objects`: private-ABI register swap driven by the
  vehicle callee; closes only with the rejected callee alias.
- `_player_set_action_result`: x87 operand order inside the header inline
  distance helpers (January's two distances use different sum orders).
- `_player_update_powerups`, `_player_handle_powerup`, `_player_handle_powerup_minor`:
  register family — January keeps the sign-extended powerup index in place and
  tests the 32-bit register in the inlined helper; ours CSEs the address or keeps
  the short. Only the rejected widened carrier has reproduced it.
- `_player_examine_nearby_item`: register — EBX goes to the item index in
  January and to `weapon_count` in ours; relocations identical.
- `_players_update_after_game`: owner — in-loop signed 4-bit read of
  `local_player_triggered_switch` (`shl al,4; sar al,4; cmp al,0xff`) against
  `byte :4` in `players.h`; plus a block-address selection difference.
- `_player_teleport_internal`: structural duplicate identified (shape above) plus
  the documented source-root/biped home lifetime residual.

## 7. Reopen criteria

- device: owner review of the January `fast_vector_intersects_sphere` prototype
  (would also target `_ai_test_line_of_fire`).
- vehicle / set_action_result: a legal-C `player_set_action_result` whose flip
  test does not collapse under a constant argument, with January corroboration;
  or an owner decision on the real_math.h distance/magnitude inline spelling.
- powerup family: first-party evidence of the helper parameter width or index
  carrier; measure all three callers together.
- item: evidence of `weapon_count`/item-index scope that changes callee-saved ranking.
- after_game: owner edit of the players_globals triggered-switch bitfield/sentinel.
- teleport_internal: start from the single-assert/single-else shape, then only
  with provenance for the January local homes.
- examine_nearby_unit: first-party stub source plus owner policy on a
  non-returning boolean stub.

## 8. Disposition

`players.obj` remains active and non-matching: 55/70 functions strict exact,
14 residual (4 parked, locked and unchanged), 1 target-only stub. Credit is
claimed only for `_players_update_before_game`. Do-not-repeat addition: the
`test_vehicle` alias / any second named vehicle pointer in
`player_examine_nearby_vehicle` (rejected twice: Fable reconciliation and this
lane's admission review); it also triggers a non-January inline of
`player_set_action_result`.
