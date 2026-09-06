# ActionObey shared swarm schema and enum-owner audit (2026-09-05)

## Disposition

The shared `swarm_component_datum` / `swarm_wander_control` schema is
authenticated strongly enough for one coherent owner-header trial.  The
natural owner is `source/ai/actors.h`: that header already owns
`swarm_datum`, `SWARM_COMPONENT_DATUM_SIZE`, `swarm_data`,
`swarm_component_data`, and the neighboring actor/swarm accessors.

The decisive evidence is direct DIA type information in both `HCEX.pdb` and
`HCEX_Release.pdb`, not the Fable donor's flat local view.  In particular,
`swarm_component_datum.flags` is a signed `short`, the byte at `+0x1B` is an
explicit member named `pad`, and the `+0x1C` tail is an anonymous union of
`wander` and `obey`.  `swarm_wander_control` has no named member at `+2`;
that two-byte interval is ordinary alignment before its vector.

This is a read-only/source-ownership result.  No production, donor, header,
configuration, build, or object file was changed or compiled by this audit.
The current root-owned ActionObey API/flush-leaf work was observed but not
modified or re-audited here.

## Direct PDB schema

Reproduction tool:

`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`

Primary type source:

`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX.pdb`

SHA-256: `f55cfe957da8079a62a26a6753f1ddbb700b067a66de0bd5db5aac182d985ff1`

Independent same-result PDB:

`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`

SHA-256: `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`

The relevant commands are `DIA2Dump.exe -sym <name> <pdb>`.  Both PDBs give
the same two principal layouts:

| type/member | offset | DIA type |
| --- | ---: | --- |
| `swarm_component_datum` | | size `0x40` |
| `identifier` | `0x00` | `short` |
| `flags` | `0x02` | `short` |
| `position` | `0x04` | `real_point3d` |
| `surface_index` | `0x10` | `long` |
| `combat_target_prop_index` | `0x14` | `long` |
| `attached_to_unit_ticks` | `0x18` | `unsigned char` |
| `ground_ticks` | `0x19` | `unsigned char` |
| `attack_delay_ticks` | `0x1A` | `unsigned char` |
| `pad` | `0x1B` | `unsigned char` |
| `wander` | `0x1C` | `struct swarm_wander_control` |
| `obey` | `0x1C` | `struct obey_individual_simple_control` |
| `swarm_wander_control` | | size `0x14` |
| `pause_ticks` | `0x00` | `unsigned char` |
| `move_ticks` | `0x01` | `unsigned char` |
| implicit alignment | `0x02..0x03` | no DIA member |
| `vector` | `0x04` | `real_vector3d` |
| `angle` | `0x10` | `float` |

The already-shared ActionObey arm in `actions.h` also agrees with the PDB:
`obey_individual_simple_control` is `0x24`, with bytes at `+0/+1`, signed
`short pause_timer` at `+2`, bytes at `+4/+5`, and anonymous
`directmovement` / `jump` arms at `+8`.  The direct-movement arm is `0x1C`
(`short facing +0`, vector `+4`, start position `+0x10`); the jump arm is
`0x0C` (`short delay_ticks +0`, reals `+4/+8`).  Those existing records need
no shape change.

`DIA2Dump -sym swarm_component_data HCEX.pdb` identifies the global as
`struct data_array *`, matching the existing `actors.h` declaration.

## Source-authentic owner shape

The following is the evidence-backed shape for `actors.h`.  The enumerator
count is deliberately omitted because the named `NUMBER_OF_*` symbol is not
present in either PDB and no current consumer needs it.

```c
enum
{
	_swarm_component_attacking_in_melee_bit = 0,
	_swarm_component_attached_to_unit_bit,
	_swarm_component_wander_bit,
	_swarm_component_obey_bit,
	_swarm_component_obey_desire_jump_bit,
};

struct swarm_wander_control
{
	byte pause_ticks;
	byte move_ticks;
	real_vector3d vector;
	real angle;
};

struct swarm_component_datum
{
	short identifier;
	short flags;
	real_point3d position;
	long surface_index;
	long combat_target_prop_index;
	byte attached_to_unit_ticks;
	byte ground_ticks;
	byte attack_delay_ticks;
	byte pad;
	union
	{
		struct swarm_wander_control wander;
		struct obey_individual_simple_control obey;
	};
};
```

Owner assertions should lock, at minimum:

- `sizeof(swarm_wander_control)==0x14`, vector `+4`, angle `+0x10`;
- `sizeof(swarm_component_datum)==0x40`, flags `+2`, position `+4`,
  surface `+0x10`, target prop `+0x14`, the four bytes `+0x18..+0x1B`, and
  both union arms `+0x1C`; and
- the already-proven composed access
  `obey.directmovement.vector==+0x28`.

The retained HCEA generated header spells an explicit `pad[2]` in
`swarm_wander_control`.  That is a reconstruction convenience, not a named
member recovered by DIA, and must not be promoted into the owning schema.

## Signed flags: independent January corroboration

January `_code_00005350` (`action_obey_individuals_iterate`, `0x130` padded)
zeros exactly `0x24` bytes at component `+0x1C`, then loads the word at `+2`
and emits:

```text
xor eax,eax
mov ax,word ptr [esi+2]
and eax,0xfffffffb
or eax,8
mov word ptr [esi+2],ax
```

This is the component's `wander`-clear / `obey`-set operation.  The frozen
Fable measurement records that `short flags` reproduces the
`and eax,0xfffffffb` encoding while its prior unsigned `word` spelling emits
the different `and eax,0xfffb` form.  That code observation corroborates the
direct PDB `short`; it is not being used as a spelling-search instruction.

The committed Fable source must therefore not contribute its flat
obey-only component record, even though that local record also uses `short`.
Its source SHA-256 is
`067b4900d5494c5341ca609337f0d220dedd4f95778a935d601427de96575a59`.

## Enum owners and provenance tiers

### `actors.h`: component flags

Direct `HCEX.pdb` constants authenticate all five spellings and values:

| enumerator | value |
| --- | ---: |
| `_swarm_component_attacking_in_melee_bit` | 0 |
| `_swarm_component_attached_to_unit_bit` | 1 |
| `_swarm_component_wander_bit` | 2 |
| `_swarm_component_obey_bit` | 3 |
| `_swarm_component_obey_desire_jump_bit` | 4 |

These belong with `swarm_component_datum.flags` in `actors.h`.
`NUMBER_OF_SWARM_COMPONENT_FLAGS` is absent from the PDB and unused by the
candidate; do not add it merely because a later extracted DB header has it.

### `actions.h`: simple-control flags

Direct `HCEX.pdb` constants authenticate all five spellings and values:

| enumerator | value |
| --- | ---: |
| `_obey_simple_directmovement_bit` | 0 |
| `_obey_simple_directmovement_update_continuously_bit` | 1 |
| `_obey_simple_jump_bit` | 2 |
| `_obey_simple_jump_jumped_bit` | 3 |
| `_obey_simple_jump_targeted_bit` | 4 |

They belong beside `obey_individual_simple_control.simple_control_flags` in
`actions.h`.  `NUMBER_OF_OBEY_SIMPLE_FLAGS` is absent from the PDB and unused,
so the proposed owner packet need not invent it.

### `bipeds.h`: runtime biped flags

The PDB defines `_biped_datum.flags` as `unsigned long` and directly
authenticates the complete six-name set:

| enumerator | value |
| --- | ---: |
| `_biped_airborne_bit` | 0 |
| `_biped_slipping_bit` | 1 |
| `_biped_absolute_movement_bit` | 2 |
| `_biped_no_collision_bit` | 3 |
| `_biped_movement_passes_through_bipeds_bit` | 4 |
| `_biped_limp_body_physics_active_bit` | 5 |

These belong next to `_biped_datum.flags` in `bipeds.h`.
`NUMBER_OF_BIPED_FLAGS` is not returned by DIA and is unnecessary.

The existing header's `_biped_limping_bit=0` is not an authentic alias:
`DIA2Dump -sym _biped_limping_bit` returns no symbol, while bit 0 is directly
named `_biped_airborne_bit`.  It is also not unused.  Current source has five
uses in `units.c` (lines 2364, 4219, 4302, 6547, and 10051 at audit time) and
one in `players.c` (line 2111).  The strongest semantic anchors select
`_unit_state_melee_airborne` and `_unit_state_dying_airborne` when the bit is
set.  A coherent owner repair must rename those six uses to
`_biped_airborne_bit`, remove the wrong placeholder, and not retain it as a
second same-value name.

### `ai_scenario_definitions.h`: command-list flags and atom modifiers

`ai_command_list_definition.flags` is directly `unsigned long` at `+0x20`.
The PDB authenticates exactly these five command-list names:

| enumerator | value |
| --- | ---: |
| `_ai_command_list_allow_initiative_bit` | 0 |
| `_ai_command_list_allow_targeting_bit` | 1 |
| `_ai_command_list_disable_looking_bit` | 2 |
| `_ai_command_list_disable_communication_bit` | 3 |
| `_ai_command_list_disable_falling_damage_bit` | 4 |

January `action_obey_command_list_setup` independently reads masks
`1/2/4/8`, and `action_obey_individual_begin` reads mask `0x10`.
`_ai_command_list_manual_structure_bsp_bit=5` and
`NUMBER_OF_AI_COMMAND_LIST_FLAGS=6` occur in later extracted DB/Demon
material but are not returned by either PDB and are unused by the evidenced
eleven-function donor subset.  They must not be included in this PDB-grounded
packet.

`ai_command_definition.atom_modifier` is directly a signed `short` at `+2`.
The PDB authenticates only
`_ai_atom_loop_modifier_until_told_to_advance=1`; January command-end compares
the word against 1.  The donor's `_ai_atom_loop_modifier_always=0` and count
are not needed and have no direct PDB result, so omit them.

The direct-movement facing domain is the one remaining naming caveat.
January `_code_00005f70` proves a signed-short read at simple-control `+8`, an
unsigned range check for values `0..3`, and these behaviors:

- 0: copy the base facing;
- 1: negate the base facing;
- 2: store the normalized perpendicular;
- 3: negate that perpendicular;
- any other value: leave the output unchanged.

This authenticates the values and semantics of the donor spellings
`_ai_atom_move_facing_forwards/backwards/left/right`, and the natural owner is
the atom-modifier domain in `ai_scenario_definitions.h`.  It does **not**
authenticate those exact source spellings: DIA returns no such names and the
retained HCEA function explicitly records that its enum oracle found none.
If descriptive reconstructed enum names are allowed with an explicit
provenance comment, those four donor names are semantically credible.  If an
original-name requirement applies, the private direct-movement/update pair
remains withheld; raw integers or a TU-private enum are not acceptable
workarounds.

## Duplicate/facade removal inventory

One coherent shared-owner trial must address all current production views,
not only the donor file:

| file | current debt | owner-coherent action |
| --- | --- | --- |
| `source/ai/actors.c` | partial `swarm_component_datum` with unsigned `word flags` and `unknown_tail`; local accessor macro; two local layout typedefs | remove the partial type; consume the owner type; relocate/remove redundant asserts; use the shared typed accessor if installed |
| `source/ai/actor_type_infection.c` | local component/wander types (`word flags`, synthetic named `word pad`); local component and obey-simple enums; local biped-airborne enum; local accessor and layout typedefs | remove duplicate types/enums/asserts and consume `actors.h`, `actions.h`, and `bipeds.h` owners |
| `source/ai/actor_perception.c` | 0x40 prefix facade `actor_perception_swarm_component_view`; local accessor; raw `TEST_FLAG(...,1)` | remove facade/assert, use `swarm_component_datum`, and name bit 1 `_swarm_component_attached_to_unit_bit` |
| `source/units/bipeds.c` | local bit 0 / bit 5 enum | remove it after the complete owner enum lands |
| `source/game/player_control.c` | local `_biped_airborne_bit=0` enum | remove it after the owner enum lands |
| `source/units/units.c`, `source/game/players.c` | six uses of wrong `_biped_limping_bit` name | migrate all six to direct-PDB `_biped_airborne_bit` |
| prospective Fable `source/ai/action_obey.c` body | flat obey-only component, local component/simple/biped/command/modifier enums, local accessor | do not import those definitions; consume the owning headers and shared full-union type |

`actor_type_infection.c` also copies the field into a local
`word component_flags`.  DIA proves the **record field** is signed; it does
not recover that local variable's source type.  Do not retune this local as
part of the ownership packet without January function evidence—the header
correction alone removes the false record declaration.

Because `actors.h` already owns `swarm_get`, `swarm_try_and_get`, and the
`swarm_component_data` declaration, a single typed `swarm_component_get`
macro there is the natural source-owner cleanup for the three current local
macros and the future ActionObey caller.  Macro ownership is a source-design
inference (PDBs do not retain macros), not a recovered original symbol; root
may instead retain identical local macros, but the perception facade's return
type still must be corrected.

## Trial boundary and dependency scope

This audit found 43 direct lexical includers of `actors.h` (42 C files plus
`ai_debug.h`), 27 direct includers of `actions.h`, 13 of
`ai_scenario_definitions.h`, and 13 of `bipeds.h`.  These are only source-text
counts, not a substitute for the build dependency graph; the root-owned
trial must freeze and compare every actual dependent object and all inherited
strict/residual owners.

The first coherent trial should therefore be one ownership packet:

1. install the PDB-authenticated full schema and five component bits in
   `actors.h`;
2. remove the three production type facades and migrate perception's raw bit;
3. install the five PDB-authenticated obey-simple bits in `actions.h`;
4. install the six PDB-authenticated biped bits in `bipeds.h`, remove three
   local enum bodies, and repair all six wrong bit-0 consumer names;
5. install only the PDB-authenticated command-list bits 0..4 and loop-until
   value 1 in `ai_scenario_definitions.h`;
6. decide explicitly whether the four behavior-authenticated but
   source-name-unproven move-facing enumerators are acceptable; and
7. only then compile the unchanged natural ActionObey donor-body subset under
   real owner headers, with a complete dependent-object/runtime census.

No enum counts, manual-BSP bit, flat union, synthetic wander pad, raw bit
numbers, prefix views, or TU-private fallback enums are part of this
recommendation.

## Evidence read

- `scratch/fable_action_obey_intake_20260905.md`, SHA-256
  `587bda8055043f26957944b40939f0a7e9a831957c8cace07e0d3c007238eef6`;
- current `actors.h/.c`, `actor_type_infection.c`, `actor_perception.c`,
  `actions.h`, `ai_scenario_definitions.h`, `bipeds.h/.c`,
  `player_control.c`, `units.c`, and `players.c`;
- the committed Fable `action_obey.c` at
  `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`, its saved
  `scratch/reports/action_obey.md`, and its frozen exactness notes;
- complete January disassemblies for `_code_00005350`,
  `_action_obey_command_list_setup`, `_code_00005290`, `_code_00004fb0`, and
  `_code_00005f70` from `build/split/source/ai/action_obey.obj`;
- direct DIA queries against both PDBs for all types and enumerators recorded
  above; and
- retained HCEA generated headers/functions and Demon command-schema source,
  used only as corroboration where explicitly identified, never as January
  original-source proof.
