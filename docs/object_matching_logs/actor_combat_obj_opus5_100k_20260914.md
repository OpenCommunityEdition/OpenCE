# `actor_combat.obj`: burst caller graph landed; five private owners and two COMDATs exact (2026-09-14)

## Scope and provenance

- Lane `opus/100k-consolidated-20260914` (worker `waveA:actor_combat`), lane HEAD `b8cd50113`; only `source/ai/actor_combat.c`
  was modified. No header, `config/`, tool or build file was touched.
- Compiler: XDK VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox` (repository flags from `build.ninja`).
- Target: `build/split/source/ai/actor_combat.obj`.
- Names: January debug map atlas (`scratch/atlas/actor_combat.obj.txt`) for the six private owners, already renamed in
  `config/symbols.json` by `b8cd50113` (`"static": true`); PDB public `_actor_combat_update`.
- Prerequisite header `12e295220`: `weapons.h` declares `weapon_aim` and `weapon_estimate_time_to_target`.
- Source body: replay of the PDB/IDA lane diagnostic packet (`actor_combat_obj_opus5_pdb_ida_20260914.md`,
  `C:/halo-worktrees/opus5-pdb-ida-15k-20260914/scratch/workers/actor_combat/u3.c`), with every scratch-only declaration replaced
  by an admissible mechanism or removed (below). Enumerator names: HCEA DB-verified headers
  (`halocea-review/src/headers/actor_definition_flags2_flags.h`, `actor_special_fire_mode.h`, `actor_special_fire_situation.h`,
  `actor_combat_status.h`). HCEA/Stian give semantics only.

## Validated baseline and final state

`python -B tools/campaign/gate.py source/ai/actor_combat --all --forbid-emitted-symbol _point_from_line3d`

| | functions | exact padded code | data owners |
| --- | --- | --- | --- |
| baseline (HEAD) | exact 20 / residual 5 / unwritten 9 (34) | 2,496 / 12,416 B | 10/26 exact (217/721 B) |
| final (real file) | exact 27 / residual 7 / unwritten 0 (34) | 3,424 / 12,416 B | 26/26 exact (721/721 B) |

Newly strict-EXACT (no alias): `_actor_combat_enable_special_fire_situation` 144/2, `_actor_combat_get_firing_parameters` 160/6,
`_actor_start_pause` 176/11, `_actor_start_first_burst_delay` 208/8, `_actor_combat_allow_special_fire_situation` 176/9,
`_fast_ftol` 32/0, `_random_boolean` 32/2 (+928 padded bytes). All 20 baseline-EXACT rows remain EXACT.

| Residual | January size / relocs / sha16 | final | first divergence / class |
| --- | --- | --- | --- |
| `_actor_combat_update` | 4672 / 146 / `e37ad38594fffad1` | 4576 / 145 / `40a46028101422bc` (FUZZY) | +0x87A: alignment debug snapshot omitted (owner blocker P3) |
| `_actor_start_burst` | 1968 / 67 / `cf16e6e39069e23e` | 1968 / 67 / `5a8b7075e61a99a1` (FUZZY) | +0x117 frame-slot coloring; backend tie |
| `_actor_aim_grenade` | 544 / 22 / `e0f88f129bff43bf` | 576 / 22 unchanged | semantic-withdrawn (vehicle lifecycle proof required) |
| 4 parked rows | unchanged | park drift PASS 4/4 | locked |

## Accepted controls (retained source shapes)

- Five private helpers: PDB-lane shapes (single-exit flags, nested `get_firing_parameters(actor, actor_combat_get_firing_variant_definition(actor), ...)`,
  orphan flag latched before `fire_immediately = TRUE`, strafing collateral count `>= 3`). Exact first compile in this lane.
- `_fast_ftol`, `_random_boolean`: natural header-inline COMDATs of `actor_start_burst`/`actor_combat_update`; no wrapper or rename.
- The TU-local `sine`/`cosine`/`tangent` wrappers and their `#define ... _inline` renames were removed: the three rows stay EXACT as
  natural COMDATs of `actor_start_burst`'s `cosine`/`sine`/`tangent` uses.
- Out-of-line real-math calls in `actor_combat_update` (January REL32 `_real_random_range`, `_distance_squared3d`, `_distance3d`,
  `_vector_from_points3d`, `_point_from_line3d` x2, `_magnitude_squared3d` x3): the TU's existing per-TU rename mechanism
  (`normalize2d`/`normalize3d` precedent) extended with `#define distance3d actor_combat_distance3d_inline` and external
  prototypes after the `#undef` block; the TU's own inline call sites use the `actor_combat_*_inline` names.
- TU-local complete enum copies with disclosure comments: `actor_definition.flags2` bits (copies in actor_firing_position.c,
  actor_moving.c, actor_stimulus.c, encounters.c), special-fire mode/situation (action_charge.c), combat-status ladder
  (actors.c, action_obey.c, actor_perception.c).
- Authenticity rewrites verified byte-identical to the PDB-lane form: the discarded `TAG_BLOCK_GET_ELEMENT(&triggers, 0)` became an
  unused `trigger` local; `real_vector3d const *gun_offset` + cast became a plain pointer; the alignment threshold literal
  `0.17526217f` is spelled `(0.97f - 0.70710677f)/1.5f` and folds to the same `__real@3e3377ed` (linear ramp from cos 45 degrees at
  range 0 to 0.97 at 1.5 world units).
- `actor_combat_update` omits January's alignment debug snapshot (see blocker P3), with a disclosure comment. Nothing else is
  omitted; every other assert anchor, call and store is present.

## Experiment matrix (this lane)

| candidate | change | update | start_burst | whole TU |
| --- | --- | --- | --- | --- |
| `u3.c` | PDB packet replay, real names | 4672/146 sha (2 insns) | 1968/67 sha | 27/7/0 |
| `v1.c` | admissible declarations; snapshot omitted | 4576/145 | 1968/67 sha | 27/7/0 |
| `v2.c` | unused `trigger` local | identical `40a46028` | same | 27/7/0 |
| `v3.c` | non-const `gun_offset` | identical | same | 27/7/0 |
| `v4.c` | threshold constant expression | identical | same | 27/7/0 |
| `v5.c` = landed | disclosure comment | identical | same | 27/7/0 |
| `vD.c` | v0 + scratch view struct (diagnostic only) | 4672/146 sha (2 insns) | same | 27/7/0 |
| `vP3.c` | landed + proposed ai_debug.h text inlined at its include position (simulation) | 4672/146 sha (2 insns) | same | 27/7/0, parks PASS |
| `vP3a.c` | burst_target via inline `point_from_line3d(..., 1.0f, ...)` | identical 2-insn tie | same | 27/7/0 |

All candidates: point guard PASS, park drift PASS, fake scan 0 leads.

## Do-not-repeat

- update burst_target i-component load order (`fld rel.i; fadd origin.x` in January, j/k origin first): textual operand order is
  canonicalized (vP3a identical to add_vectors3d form); plus the PDB lane's 4 probes. Backend tie.
- start_burst: PDB lane's 9 tie probes (operand flips, trig local placement/CSE, declaration order, weapon-block scoping, `*=` forms).
- A shadow/view struct over `actor_debug_info` is inadmissible; do not land it.

## Residual classification

- `actor_combat_update`: ownership/header (fact: 14 debug stores + `game_time_get` at +0x87A..+0x959 need `actor_debug_info` field
  names); with the fields (simulated) it is a one-load-order x87 scheduling tie (fact: 2 of 1334 instructions).
- `actor_start_burst`: scheduling/frame coloring (fact: same size/relocs; 53/618 instructions; slot permutation
  [ebp-0x10/-0x14/-0x18] and January keeping `limit*1.5` live on x87 for the `origin_radius` store). Classification as a tie is
  inference from exhausted spellings.

## Owner census

- Code: every target owner present; new candidate-only code COMDATs are the systemic header-inline class only
  (`_actor_combat_real_random_range_inline`, `_random`, `_real_random`; same class as the baseline extras `_cross_product3d`,
  `_dot_product3d`, `_square_root`, `_actor_combat_*_inline`), emitted because the new bodies genuinely inline those helpers.
- Data: every new data owner is January-defined (12 assert/console strings, 4 literals) or a literal January references undefined
  (`__real@38d1b717`, `3c8efa35`, `3d088889`, `3e800000`, `3f19999a`, `41f00000`).

## ORCHESTRATOR-PROPOSAL P3 (source/ai/ai_debug.h, `struct actor_debug_info`)

Replace `char __unknown1C[40];` (+0x34..+0x5B) with:

```c
	long burst_alignment_time;
	boolean burst_aligned;
	boolean burst_aligned_immediately;
	real_vector3d burst_weapon_vector;
	real_vector3d burst_aim_vector;
	real burst_alignment_threshold;
	real burst_alignment;
```

Evidence: January `_actor_combat_update` +0x87A `game_time_get` -> `[debug+0x34]`; +0x88C/+0x890 bytes `+0x38`/`+0x39` = 1/1 when
`_actor_definition_start_firing_before_aligned_bit`; else weapon vector -> `+0x3C`, current aim vector -> `+0x48`, `fst` alignment
-> `+0x58`, threshold -> `+0x54`, `+0x38 = alignment >= threshold`, `+0x39 = 0`. Layout is implicit-padding identical (0x3A..0x3B pad).
Names are inferred from the write site (the header already uses `burst_*` for its +0x10..+0x33 neighbours); `field_34`... placeholders
in the header's existing style are an equally valid choice. Simulation (`vP3.c`): update 4672/146 sha-only, all other rows unchanged.
Not simulated in other TUs (a struct member split changes no file-scope declaration count, but run the full-tree sweep).
Consumers (25): ai/actions.c ai/action_charge.c ai/action_obey.c ai/action_uncover.c ai/action_vehicle.c ai/actors.c
ai/actor_combat.c ai/actor_looking.c ai/actor_moving.c ai/actor_perception.c ai/actor_stimulus.c ai/ai.c ai/ai_communication.c
ai/ai_debug.c ai/ai_script.c ai/encounters.c ai/path.c game/players.c main/debug_keys.c objects/objects.c
"saved games/game_state.c" structures/structure_visibility.c units/bipeds.c units/units.c units/unit_dialogue.c.

Replay after P3 (replace the disclosure comment and the `if (!TEST_FLAG(...))` opening in `actor_combat_update`):

```c
				fire = TRUE;
				actor->control.aiming_at_fire_target = TRUE;
				actor_debug_info->burst_alignment_time = game_time_get();
				if (TEST_FLAG(actor_definition->flags, _actor_definition_start_firing_before_aligned_bit))
				{
					actor_debug_info->burst_aligned = TRUE;
					actor_debug_info->burst_aligned_immediately = TRUE;
				}
				else
				{
					/* threshold, weapon_vector, alignment, hold_burst_start as landed */
					actor_debug_info->burst_weapon_vector = weapon_vector;
					actor_debug_info->burst_aim_vector = actor->control.current_fire_target_aim_vector;
					actor_debug_info->burst_alignment = alignment;
					actor_debug_info->burst_alignment_threshold = alignment_threshold;
					actor_debug_info->burst_aligned = alignment >= alignment_threshold;
					actor_debug_info->burst_aligned_immediately = FALSE;
				}
```

## Reopen criteria

- `actor_combat_update`: P3 lands -> apply the replay block (expect 4672/146, 2-insn tie).
- `actor_start_burst`: new evidence for the capped-branch spelling (a named maximum-radius value that is itself the store source).
- `actor_aim_grenade`: authenticated release-time vehicle_index lifecycle invariant (ledger 20260905).

## Disposition

Active. Seven rows strict EXACT and eligible; `actor_combat_update` and `actor_start_burst` are zero-credit fuzzy bodies that are the
genuine January emitters of the five private statics and two COMDATs; four parks locked and undrifted.
