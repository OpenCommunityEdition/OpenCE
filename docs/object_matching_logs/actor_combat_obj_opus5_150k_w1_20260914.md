# `actor_combat.obj`: update closes strict EXACT behind a units-safe ai_debug.h packet (opus5 150K lane, wave w1, 2026-09-14)

## Scope and provenance

- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`. Worker `w1:actor_combat`. The only file this worker could modify was
  `source/ai/actor_combat.c`. **It is unchanged** (`git diff --stat -- source/ai/actor_combat.c` is empty). No header, config,
  build or tool file was touched.
- Target: `build/split/source/ai/actor_combat.obj`. Compiler: XDK VC7 13.00.9254, repository flags via `tools/campaign/gate.py`.
- Evidence used:
  - January target COFF (disassembly, relocations);
  - HCEA PPC lifts `halocea-review/src/blam/ai/actor_combat_update.c:421-457` and `actor_start_burst.c:180-275` (semantics and
    operand topology only);
  - January `_ai_debug_render_actor` disassembly (checked for readers of `actor_debug_info +0x34..+0x5B`: none found);
  - punpckhdq `ai_debug.h` (same `__unknown1C[40]` gap, no names);
  - prior ledgers `actor_combat_obj_opus5_100k_20260914.md`, `actor_combat_obj_opus5_pdb_ida_20260914.md`,
    `actor_combat_obj_grenade_vehicle_semantic_withdrawal_20260905.md`, `actor_combat_obj_caller_closure_api_boundary_20260905.md`,
    and `config/parked.json`.
- Worker notes (per-function blocks): `scratch/workers/actor_combat.md`. Scratch candidates: `scratch/workers/actor_combat/`.

## Baseline and final (real file)

`python -B tools/campaign/gate.py source/ai/actor_combat --all --forbid-emitted-symbol _point_from_line3d`

| | result |
| --- | --- |
| baseline | `== exact 27  residual 7  unwritten 0 (of 34)`; point guard passed |
| final (real file, unchanged) | identical listing (diffed row by row); point guard passed; parks 4/4 undrifted; code/data owners identical to `build/base`; fake scan 0 leads |

No row is newly exact in the real file.

## Per-function outcomes

| function | January padded/relocs | outcome |
| --- | --- | --- |
| `_actor_combat_update` | 4672 / 146 | **scratch strict EXACT** with R2 + a units-safe P3 variant (W1/W2). Owner-blocked on the ai_debug.h packet. Not landed. |
| `_actor_start_burst` | 1968 / 67 | not landed; 5 new capped-branch shapes, none closer; evidence-exhausted tie family |
| `_actor_aim_grenade` | 544 / 22 (ours 576) | skipped; semantic-blocked (UB / point-as-velocity carrier needed) |
| `_actor_combat_find_nearby_target` (park) | 352 / 12 | scratch probe (inline `point_from_line3d` hypothesis) refuted; park unchanged |
| `_actor_combat_reaim_grenade` (park) | 448 / 16 | scratch probe (gravity as argument temp) inert; park unchanged |
| `_actor_combat_plan_grenade_trajectory` (park) | 288 / 5 | skipped; no new lever |
| `_actor_aim_projectile` (park) | 720 / 27 | skipped; no new lever |

## `_actor_combat_update`: the 2-instruction tie was source structure

### Starting point

The 100K lane's P3 simulation leaves exactly two instructions, reproduced here as `p3.c`:
- January at `+0xE06`: `fld [ebx+0x664]` (relative.i), then `fadd [edi]` (origin.x).
- Ours: `fld [edi]`, then `fadd [ebx+0x664]`.

For j and k, both January and ours load the origin first.

### New lever: a named pointer changes operand ranking

The asymmetry (i relative-first, j/k origin-first) also appears in the HCEA PPC lift (`actor_combat_update.c:451-457`). That is a different compiler, so the asymmetry is a property of the source tree, not an x86 register tie.

When one operand of the inline `add_vectors3d` is an indirection through a named pointer variable, component 0 becomes a plain `IND(var)` while components 1 and 2 become `IND(var+off)`. This changes VC7's commutative operand ranking for i only.

Measured matrix (update row only, all on the P3-emulation base):

| shape | result |
| --- | --- |
| e1 `add(&origin, add(&adj,&rel,&rel), &target)` | tie unchanged |
| e2 `add(add(&adj,&rel,&rel), &origin, &target)` | tie unchanged |
| s3a block-top `real_point3d *burst_origin` for all uses | `lea edi` moves before assert 640 (lea-edi/assert-640 mismatch); j/k reversed |
| s3c/s3d/s3e (s3a variants) | still the lea-edi/assert mismatch and/or j/k reversed |
| r1 nested-block `real_vector3d *burst_relative_position`, `add(&origin, rel, &target)` | tie unchanged |
| **r2 nested-block pointer; `add(&adj, rel, rel)` then `add(rel, &origin, &target)`** | **STRICT EXACT 4672/146** |
| va/vb/vc (pointer first in the first add; block-top pointer) | first add's i order flips |
| x1/x1r nested origin pointer (either order) | j/k reversed |
| x2 nested target pointer | tie unchanged |
| x3 named return of first add | tie unchanged |

R2 source block (replaces the two `add_vectors3d` calls after `burst_last_known_position`):

```c
			{
				real_vector3d *burst_relative_position = &actor->control.burst_relative_position;

				add_vectors3d(
					&actor->control.burst_adjustment,
					burst_relative_position,
					burst_relative_position);
				add_vectors3d(
					burst_relative_position,
					&actor->control.burst_origin,
					&actor->control.burst_target);
			}
```

Meaning: the relative burst offset advances by the per-tick adjustment, then target = relative offset + origin.

**Verifier caveat (not hidden).** Exactness needs both the nested-block named pointer and the operand order `(relative, origin)`; r1, with origin first, is the tie. With the pointer present, VC7 follows textual order for component 0, so January's i order is read as the textual order. This must be judged against the brief's rule on expression-order steering. It is reported, not claimed admissible on bytes alone. Precedent for the idiom in the same object is `actor_aim_projectile`'s block-scoped `real_point3d const *burst_target = &actor->control.burst_target;`.

### P3 header proposal: re-measured, and the flat form is inadmissible

Consumer sweep method (`scratch/workers/actor_combat/p3sweep.py`, `hsweep.py`):
- Covers all 25 `ai_debug.h` consumers; no header includes it.
- Copy A is a snapshot; copy B redirects the direct include to a shadow header.
- Both copies are gated with `--source`, and every function record (size, relocs, normalized sha) is compared.
- Results: `scratch/workers/actor_combat/p3sweep/results.txt`, `results_w1.txt`, `results_w2.txt`.

| header variant | consumers changed | detail |
| --- | --- | --- |
| P3 flat (100K ledger text) | **1/25** | `units::_unit_preprocess_node_orientations` changes (units 189 -> 188, the protected sentinel) |
| W1 named `struct actor_debug_burst_alignment` + member | 0/25 | — |
| W2 tag-less nested struct member | 0/25 | — |

With W1 or W2 plus R2 (`w1c.c`, `w2c.c`), `actor_combat` gates as follows:
- `_actor_combat_update` is strict EXACT 4672/146; whole TU `exact 28 residual 6 unwritten 0`.
- All 27 baseline EXACT rows stay exact; point guard passes; parks 4/4 unchanged.
- Census: no new code or data owner vs `build/base`; data 26/26 exact (721 B). Fake scan: 0 leads.

## ORCHESTRATOR-PROPOSAL P3' (supersedes P3 of the 100K ledger)

**Header `source/ai/ai_debug.h`, `struct actor_debug_info`.** Replace `char __unknown1C[40];` (+0x34..+0x5B) with the W2 text:

```c
	struct
	{
		long time;
		boolean aligned;
		boolean aligned_immediately;
		real_vector3d weapon_vector;
		real_vector3d aim_vector;
		real threshold;
		real alignment;
	} burst_alignment;
```

Alternative W1, equally clean in the sweep: a file-scope `struct actor_debug_burst_alignment { ... };` placed immediately before `struct actor_debug_info`, plus member `struct actor_debug_burst_alignment burst_alignment;`. W2 adds no file-scope declaration and is preferred. Layout is identical to `char[40]` (4-byte alignment, pad at +0x3A..+0x3B); `last_projectile_aiming_time` stays at +0x5C, and the exact `actor_aim_projectile` writes are unchanged in the sweep.

**Evidence.** January `_actor_combat_update` makes these writes:
- `+0x87A`: `game_time_get` -> `[debug+0x34]`.
- When `_actor_definition_start_firing_before_aligned_bit` is set: `+0x38 = 1`, `+0x39 = 1`.
- Otherwise:
  - weapon vector -> `+0x3C`;
  - current fire-target aim vector -> `+0x48`;
  - alignment -> `+0x58`;
  - threshold -> `+0x54`;
  - `+0x38 = alignment >= threshold`;
  - `+0x39 = 0`.

**Naming.** No January reader exists (checked `_ai_debug_render_actor`), so the names are inferred from the write site. The neighbouring fields are named `burst_*`.

**Why not the flat P3.** It regresses the protected units sentinel in an isolated shadow test. The nested snapshot is one coherent "burst alignment snapshot" record; the verifier must still judge whether the grouping is genuine design or a regression dodge.

**Consumers (25).** Measured 0 changed rows each with W1/W2:
- source/ai: actions.c, action_charge.c, action_obey.c, action_uncover.c, action_vehicle.c, actors.c, actor_combat.c, actor_firing_position.c, actor_looking.c, actor_moving.c, actor_perception.c, actor_stimulus.c, ai.c, ai_communication.c, ai_debug.c, ai_script.c, encounters.c, path.c.
- source/game/players.c, source/main/debug_keys.c, source/objects/objects.c, "source/saved games/game_state.c", source/structures/structure_visibility.c.
- source/units: bipeds.c, units.c, unit_dialogue.c.

**Accumulation.** Isolated tests do not compose with other header packets. After applying, re-gate `source/units/units --fn _unit_preprocess_node_orientations` and `python -m tools.parked_functions` on the live tree.

**Replay in `source/ai/actor_combat.c` after P3' lands.**

1. Replace the disclosure comment and the `if (!TEST_FLAG(...start_firing_before_aligned_bit))` opening with:

```c
				fire = TRUE;
				actor->control.aiming_at_fire_target = TRUE;
				actor_debug_info->burst_alignment.time = game_time_get();
				if (TEST_FLAG(actor_definition->flags, _actor_definition_start_firing_before_aligned_bit))
				{
					actor_debug_info->burst_alignment.aligned = TRUE;
					actor_debug_info->burst_alignment.aligned_immediately = TRUE;
				}
				else
				{
```

2. Keep the threshold, weapon_vector, alignment and hold_burst_start code as landed, then append inside the else:

```c
					actor_debug_info->burst_alignment.weapon_vector = weapon_vector;
					actor_debug_info->burst_alignment.aim_vector = actor->control.current_fire_target_aim_vector;
					actor_debug_info->burst_alignment.alignment = alignment;
					actor_debug_info->burst_alignment.threshold = alignment_threshold;
					actor_debug_info->burst_alignment.aligned = alignment >= alignment_threshold;
					actor_debug_info->burst_alignment.aligned_immediately = FALSE;
```

3. Apply the R2 block above.

Scratch candidate `scratch/workers/actor_combat/w2c.c` (sha256 `1f679183…6139`) is the current real file plus exactly these three edits, with the header include pointed at the shadow `inc/ai_debug_w2.h`. Expected result: 28/6/0, with `_actor_combat_update` EXACT (+4,672 padded).

## `_actor_start_burst`: capped branch re-tested

January's capped branch keeps `limit*1.5` on the x87 as the source of the `origin_radius` store:

```
fld [ebp-0x18]; fld origin; fdivr st(1); fmul return; fstp return; fstp origin
```

Ours copies it through an integer register:

```
fdiv origin; mov ecx,limit; mov origin,ecx
```

The extra reference plausibly flips the `[ebp-0x14]/[ebp-0x18]` slot ranking. The rotation products keep cos/sin until explicit pops.

| shape | result (alndiff differing lines; base 106) |
| --- | --- |
| b_a1 HCEA statement order (return_radius, origin_radius, timer) | 189: January's fdivr form appears, but limit stays on the x87 across the compare |
| b_a2 return_radius, timer, origin_radius | 110 |
| b_n1 named `maximum_burst_radius` in the outer if | 106 (identical) |
| b_n2 n1 + `return_radius *= maximum_burst_radius/origin_radius` | 106 (identical) |
| b_n3 n1 + HCEA order | 189 |

The residual is unchanged inside the P3'+R2 context. Classification: x87 value-lifetime scheduling tie family, inferred from exhausted spellings.

Reopen when a spelling spills limit*1.5 before the compare yet sources the origin store from the x87, or authoritative locals appear. An untested idea for the rotation products: the named-pointer ranking lever found for update. This is not a closure path while the capped branch remains.

## Parks (scratch-only probes; real bodies untouched)

- `find_nearby_target`: inline `point_from_line3d` form (via the TU's existing renamed inline, so no `_point_from_line3d` COMDAT), in two variants f1/f2. Byte-identical to base, so the inline-parameter-lifetime hypothesis is refuted.
- `reaim_grenade`: g1, gravity passed as the `?:` argument temp. Frame 0x30 and slots identical. January places the gravity value inside the dead `aim_vector2d` slot (`[ebp-0xc]`).
- `plan_grenade_trajectory`: January stores aim_gravity in the enregistered `grenade_target` parameter home `[ebp+0x10]`. No new lever; 0 shapes.
- `aim_projectile`: the aggregate-copy interleave at debug +0x98 differs. The named-pointer lever does not apply; 0 shapes.

No park-reopen proposal.

## `_actor_aim_grenade`

Unchanged. Exactness requires the shared point/vector working slot, which means an uninitialized read or a position-as-velocity carrier (see the 20260905 withdrawal). Both are prohibited; no lifecycle proof is available.

## Reopen criteria

- `_actor_combat_update`: P3' (W2 or W1) lands in `ai_debug.h`; then apply the replay above (expect strict EXACT). If the verifier rejects the R2 operand order, the function stays 4672/146 with a 2-instruction tie under P3'. Land that fuzzy only as a structurally complete body.
- `_actor_start_burst`: see above.
- `_actor_aim_grenade`: authenticated release-time `vehicle_index == NONE` invariant.
- Parks: unchanged criteria in `config/parked.json`.
