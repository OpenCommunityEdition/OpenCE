# `players.obj` Opus5 100K consolidated lane — 2026-09-14

## 1. Scope and provenance

- Translation unit `source/game/players.c`; target `build/split/source/game/players.obj`.
- Lane worktree `opus5-100k-consolidated-20260914`, branch
  `opus/100k-consolidated-20260914` (base `c3e257e9a` merged with the fresh-graphs
  and PDB-IDA lanes). Worker claim `waveB:players`.
- Compiler: repository XDK 3911 `CL.Exe` (VC7 13.00.9254), flags unchanged
  (`/O2 /Oy- /DDEBUG /Dxbox`); no `--cflag`.
- Only `players.c` was edited (a 3-line hunk). No header, `config/symbols.json`,
  `config/parked.json`, build file or tool changed.
- Evidence: January target bytes/relocations (authority 1); January map atlas
  records for `players.obj` (dumped read-only to
  `scratch/workers/players/atlas_players.txt`; the prepared
  `scratch/atlas/players.obj.txt` and Ghidra output for this object did not exist);
  January PDB public symbols; HCEA `player_teleport_internal.c` for semantics only.
- Prior ledgers read in full: `players_obj.md`,
  `players_obj_fable_exact_packet_reconciliation_20260913.md`,
  `players_obj_january_safe_harvest_20260902.md`,
  `players_obj_jonas_residual_freshness_audit_20260830.md`,
  `players_obj_jonas_source_credibility_cleanup_20260830.md`,
  `players_obj_opus5_house_clean_20260913.md`.

## 2. Validated baseline and result

| Gate (real file, `--forbid-emitted-symbol _point_from_line3d`) | Result |
|---|---|
| Baseline at lane HEAD | `== exact 55  residual 14  unwritten 1  (of 70 listed)` |
| **Final** | `== exact 56  residual 13  unwritten 1  (of 70 listed)` |

Strict exact padded code bytes: 10,592 / 15,904 (final). The full `--all`
listing differs from baseline in exactly one row.

**Newly strict exact and landed (1 function):**

| Function | Padded | Relocs | Normalized SHA-256 (first 16) |
|---|---:|---:|---|
| `_player_examine_nearby_item` | 800 | 33 | `f1f67bf1b9c419df` |

Checks on `scratch/workers/players/final.obj`: emitted-symbol guard passed; no
baseline-EXACT row lost; the four Players parks equal `measurements.base`
(size/relocations/normalized SHA); code and data owner census identical to
`build/base/source/game/players.obj`; `tools/fake_match_scan.py` 0 leads.

Residual table (January / final — size / relocs / nSHA16):

| Function | January | Final |
|---|---|---|
| `_player_examine_nearby_unit` | 16 / 0 / `499f1f307c1cb989` | absent |
| `_player_examine_nearby_device` | 176 / 9 / `f95e7e1cc6a51fcf` | 160 / 9 / `65abfe19e8da82c5` |
| `_player_examine_nearby_objects` | 240 / 18 / `a08d292a1a63cf83` | 240 / 18 / `8a2202d94a99527c` |
| `_player_examine_nearby_vehicle` | 448 / 25 / `d68f377ed87e85d1` | 448 / 25 / `dcac787d00a4b5ec` |
| `_player_handle_powerup` | 240 / 14 / `84c28544d684b20d` | 256 / 14 / `6391d7e23fe8e68c` |
| `_player_handle_powerup_minor` | 160 / 9 / `27e1638518a37796` | 160 / 9 / `b6b1d0d35d027042` |
| `_player_set_action_result` | 240 / 5 / `ab7ce57f70ded3fe` | 240 / 5 / `bc9bed2cb0fa36f1` |
| `_player_teleport_internal` | 1296 / 61 / `8bb48b429649a3af` | 1312 / 62 / `6cf72b07a9eda1d6` |
| `_player_update_powerups` | 112 / 5 / `826a864ce09f2800` | 112 / 5 / `b5a7af2d83089af4` |
| `_players_update_after_game` | 816 / 52 / `af64f40efdb1be0c` | 800 / 52 / `6407fdea5b288e67` |
| four parks | unchanged | unchanged (locked) |

## 3. Accepted control (retained in production)

### `player_examine_nearby_item` (EXACT)

Hunk: the weapon block no longer reassigns `unit = unit_get(player->unit_index)`
before `unit_inventory_get_weapon`; the re-fetch is the argument itself:
`unit_inventory_get_weapon(player->unit_index, unit_get(player->unit_index)->unit.current_weapon_index)`.

Evidence: January reads `unit->unit.control_flags` through the unit pointer's
home `[ebp-0xc]` (+0x1bb) and then stores `weapon_count` into the same
`[ebp-0xc]` slot (+0x209). The `unit` variable's lifetime therefore ends at the
trigger test and the second `object_get_and_verify_type(unit_index, 3)` (+0x1dd)
is never assigned back to it. The dead reassignment kept `unit` live, gave
`weapon_count` EBX and spilled the item index (16 extra bytes). The inline
re-fetch is the same idiom already exact in this TU (`player_pseudo_kill`) and in
the accepted `players_update_before_game` control.

## 4. Experiment matrix (this lane; candidates under `scratch/workers/players/`)

| Function | Shape | Result | Kept |
|---|---|---|---|
| item | inline unit re-fetch replacing the dead `unit =` reassignment | **EXACT** 800/33 | **yes** |
| teleport_internal | v2: single `match_assert(source_unit_index==NONE \|\| local_player_count()>1)`, one `&&` condition, one else, `best.k = 0` after the forward/up fallback, block-scoped search locals, block-scoped `forward` vector for the tail, `break` out of the trigger loop | 1280/61, 428==428 insns, frame 0x78 vs 0x7c, sha `b6f405cf3f9cf9e1` | no (residual) |
| teleport_internal | v3: loops pass `source_unit_index` instead of the root index | byte-identical to v2 | no |
| teleport_internal | v4: HCEA separate line-of-sight local | 1248/61, 412 insns (worse) | no |
| teleport_internal | v5: all locals at function scope | frame 0x84 (worse) | no |
| teleport_internal | v6: separate short trigger-loop index | 1280/61, frame 0x78 | no |
| teleport_internal | v7: adjustment index scoped to the search block | identical to v6 | no |
| teleport_internal | v8/v9: `player_unit_index` declared first / last | identical to v6 | no |
| after_game (measurement only, shadow header by absolute include) | signed `char :4` triggered-switch field | 832/52 | no |
| after_game (measurement only) | + NONE compares, no `(byte)` casts | 816/52 [sha], block-address window only | no |
| after_game (measurement only) | + `struct scenario *scenario` local for the trigger block | **EXACT** | proposal |
| after_game (measurement only) | block-scoped `tag_block` pointer instead | [sha] | no |
| after_game (measurement only) | enum `_local_player_triggered_switch_none = NONE`, enum names kept in .c | **EXACT** (preferred proposal) | proposal |
| after_game (measurement only) | `(char)` cast on the compare | [sha] | no |

## 5. ORCHESTRATOR-PROPOSAL — `players_update_after_game`

Header `source/game/players.h` (two lines, declaration count unchanged):

```c
	_local_player_triggered_switch_none = NONE,     /* was 0xF */
			char local_player_triggered_switch : 4;  /* was byte */
```

Paired `players.c` hunk (full candidate:
`scratch/workers/players/proposal_after_game_players.c`): declare
`struct scenario *scenario;` instead of `struct tag_block *bsp_switch_trigger_volumes;`,
assign `scenario = global_scenario_get();`, use
`scenario->bsp_switch_trigger_volumes.count` and
`&scenario->bsp_switch_trigger_volumes`, and drop the two `(byte)` casts on
`player->local_player_index`.

Evidence: January +0x1db `shl al,4; sar al,4; cmp al,0xff` then
`movsx dx,al; cmp dx,[esi+2]` is a signed 4-bit field compared against NONE and
the short local player index; the ticks nibble update and `> 0xC` test match an
unsigned `byte :4` (already exact under the proposal). January +0x184
`lea ecx,[eax+0x39c]; mov eax,[ecx]` is the scenario-pointer spelling. Under the
proposal the function is strict EXACT, every other Players row is unchanged
(56 exact) and the four parks do not drift. The field and enum are used only by
`players.c`, but `players.h` has 72 direct includers; the orchestrator must
rebuild and diff the whole board per function before admitting it.

## 6. Do-not-repeat

- Everything in the prior Players do-not-repeat lists.
- teleport_internal: loop argument choice (source vs root index), HCEA
  line-of-sight local, function-scope locals, separate trigger index, adjustment
  index scope, `player_unit_index` declaration order.
- after_game with the current unsigned header: any .c-only spelling (the NONE
  sentinel is wrong for an unsigned nibble; the scenario local alone stays [sha]).
- device: named radius local / volatile (prior lanes).

## 7. Residual classification

- `_player_teleport_internal` (measured): the duplicated else is a genuine
  structural defect in production; the v2 candidate removes it and reaches equal
  instruction count and relocations. Remaining (inference): register/home
  allocation — January keeps `source_unit_index` in memory in the entry region,
  gives ESI to the root index and EDI to `player_unit_index` in the loops and
  spills the root pointer (`[ebp-0xc]`) and scenario (`[ebp-0x10]`); ours gives
  ESI to the root pointer / loop index, keeps `source_unit_index` in ESI and
  reuses its parameter home for `scenario`, saving one frame slot. Plus x87
  commutative order (January squares j before i in both magnitudes). Not landed
  (residual rule).
- `_players_update_after_game`: owner/header (signed nibble), proven exact under
  the proposal.
- `_player_examine_nearby_device`: x87 argument materialization. Measured:
  January uses `fld/fstp` for object radii in players, bipeds and ai callers and
  `mov/push` for `data->radius` in collision_bsp; no exact function in the tree
  reproduces the `fld/fstp` form for a plain field.
- `_player_examine_nearby_vehicle` / `_objects`: January's second `vehicle_get`
  before the up.k test requires a second pointer statement (owner-rejected twice);
  objects follows the private ABI.
- `_player_set_action_result`: x87 stack order in the first `distance3d`.
- powerup family: register/carrier (see house-clean ledger).
- `_player_examine_nearby_unit`: target-only `ret`; new fact — no January atlas
  record and no PDB public symbol at 0x000aa180 (name only from a later-build KB).

## 8. Reopen criteria

- teleport_internal: start from `scratch/workers/players/tele_v2.txt`; reopen only
  with evidence that changes the register weights of `source_unit_index` / the root
  pointer (a January-attested extra use or scope) or the x87 operand-order rule.
- after_game: owner approval of the players.h proposal, then land the paired hunk.
- device: an exact tree function passing a plain real field via `fld/fstp`.
- vehicle/objects, set_action_result, powerup family, examine_nearby_unit: as in
  `players_obj_opus5_house_clean_20260913.md`.

## 9. Disposition

`players.obj` remains active and non-matching: 56/70 functions strict exact,
13 residual (4 parked, locked, unchanged), 1 target-only stub. Credit is claimed
only for `_player_examine_nearby_item`. One header proposal would close
`_players_update_after_game`.
