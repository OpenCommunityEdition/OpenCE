# ActionObey begin/perform owner prerequisites (read-only audit, 2026-09-05)

## Scope and disposition

This is the complete owner-prerequisite inventory for the frozen private
`action_obey_command_begin` and `action_obey_command_perform` source
proposals.  The published baseline inspected was
`c32247e7651d680dcd133d72b2fbf4e8009cc124`; concurrent root work after that
checkpoint is outside this audit.  No production source, header, build
configuration, target metadata, gate, or object was changed here.

The two source proposals remain research, not admitted code:

- `scratch/action_obey_command_begin_proposal_20260905.c`, SHA-256
  `630ee045a17f3a095929516cd524c1ee40a385eb08d75a2b133ed99ebcca5ead`;
- `scratch/action_obey_command_perform_proposal_20260905.c`, whose final hash
  is recorded at the end of this ledger.  The earlier performer SHA-256
  `23cb0320a7b64abb4111cb3269983f50b5ff3ca7287edb455349d04a9b737ce3`
  is retained as history: it used three unauthenticated `until_...` wait
  names and incorrectly assigned an adjacent `"always"` literal to the wait
  domain.  The corrected proposal uses the direct PDB names and preserves
  January's out-of-domain wait result of `TRUE`.

Root already exercised one coherent 86-consumer owner-header trial from the
6,233-strict checkpoint.  It lost the existing strict
`Units::_unit_preprocess_node_orientations` owner (6,233 -> 6,232) and was
fully restored without a name/order/source-shape retry.  The frozen evidence
is:

- `scratch/action-obey-command-owners-before-20260905.json`, SHA-256
  `34c077c0b3cbe43476e5e9dadc1a24fc22d29c462db23976b8ae1b8a45d8702d`;
- `scratch/action-obey-command-owners-after-20260905.json`, SHA-256
  `090874e7e335ae137a823e4de032de6122ca89bee8860bfd397fec166a08a404`;
- `scratch/action-obey-command-owners-restored-20260905.json`, SHA-256
  `23e6ff746d10fa75f5c23449f16903138281cac98ca62b54da20053adcd4523a`;
- stable before/restored reports, both SHA-256
  `f714aec1804b522fbe405ea393c82a21c79569f20a19a365a43a47a77b662642`.

Consequently, this ledger is an actionable future boundary, not a request to
repeat that failed trial.  The begin source itself remains frozen even where
this audit identifies better owner-constant spellings.

## Completeness verdict

The two fragments have exactly these missing shared prerequisites:

1. two complete actor constant domains in `source/ai/actors.h`;
2. twelve complete command-modifier domains and three complete scenario AI
   reference records in `source/ai/ai_scenario_definitions.h`;
3. the complete 0..13 unit-animation-impulse domain in
   `source/units/units.h`;
4. removal of the one current scenario-record shadow in
   `source/ai/action_alert.c` and the enumerator shadows listed below; and
5. direct inclusion of `units/unit_definitions.h` and
   `cutscene/recorded_animations.h` by `action_obey.c` when the begin body is
   integrated.

No additional public API is missing.  The private begin/perform functions,
`vehicle_possibility_qsort`, and
`action_obey_directmovement_update_facing` remain same-TU `static`
declarations.  `vehicle_possibility` remains a genuine private 8-byte record;
it is not a shared-header type.  The runtime vehicle-seat-desire enum is not
part of this packet: command validation uses the separate five-value
`ai_atom_vehicle_modifier` domain and forwards its short value through the
already-owned actor API.

## Exact actor-owner declarations

`source/ai/actors.h` is the genuine owner.  DIA reports both domains as
`enum <unnamed-tag>`, so use anonymous blocks and leave every datum backing
field as `short`:

```c
enum
{
	_actor_fire_target_none = 0,
	_actor_fire_target_prop = 1,
	_actor_fire_target_manual_point = 2,
	number_of_actor_fire_targets = 3,
};

enum
{
	_actor_combat_status_none = 0,
	_actor_combat_status_wary = 1,
	_actor_combat_status_investigate = 2,
	_actor_combat_status_definite = 3,
	_actor_combat_status_certain = 4,
	_actor_combat_status_clear_los = 5,
	_actor_combat_status_dangerous = 6,
	_actor_combat_status_visible = 7,
	NUMBER_OF_ACTOR_COMBAT_STATUS_LEVELS = 8,
};
```

Direct `HCEX_Release.pdb -sym` results authenticate every value enumerator
and number.  The lower-case `number_of_actor_fire_targets` spelling is the
database spelling; an upper-case replacement is not authenticated.  The
terminal counts are domain/database corroboration rather than separately
emitted PDB constants.  `actor_control_data.current_fire_target_type` at
`+0x1A0` and the actor-state combat-status fields remain `short`; declaring a
four-byte C enum field would corrupt the authenticated layout.

Conflicting current declarations to remove in a coherent future packet:

- fire-target `none`/`prop` from the mixed blocks in
  `source/ai/actor_combat.c` and `source/ai/actors.c`;
- the full combat-status block in `source/ai/actors.c`;
- the partial combat-status blocks in `source/ai/actor_stimulus.c`,
  `source/ai/actor_type_infection.c`, and `source/ai/actor_type_flood.c`;
- only `_actor_combat_status_certain` from the mixed anonymous block in
  `source/ai/ai_communication.c` (the following
  `_ai_information_none = 0` remains an explicit reset); and
- the differently named sole alias
  `_action_charge_combat_status_clear_line_of_sight = 5` in
  `source/ai/action_charge.c`, changing its one consumer to
  `_actor_combat_status_clear_los`.

The complete conflict/use census and direct offset proof are preserved in
`scratch/action_obey_actor_enum_owner_packet_20260905.md`.

## Exact command-modifier owner declarations

All of these belong in `source/ai/ai_scenario_definitions.h`.  Use anonymous
blocks: DIA exposes the source domains as anonymous enums, and command
storage remains `short atom_modifier`.

```c
enum
{
	_ai_atom_go_to_modifier_stop_at_point = 0,
	_ai_atom_go_to_modifier_keep_moving = 1,
	NUMBER_OF_AI_ATOM_GO_TO_MODIFIERS = 2,
};

enum
{
	_ai_atom_look_modifier_idle_aim = 0,
	_ai_atom_look_modifier_idle_turn_around = 1,
	_ai_atom_look_modifier_idle_look = 2,
	_ai_atom_look_modifier_force_facing = 3,
	_ai_atom_look_modifier_force_aim_weapon = 4,
	NUMBER_OF_AI_ATOM_LOOK_MODIFIERS = 5,
};

enum
{
	_ai_atom_animation_mode_modifier_noncombat = 0,
	_ai_atom_animation_mode_modifier_asleep = 1,
	_ai_atom_animation_mode_modifier_combat = 2,
	_ai_atom_animation_mode_modifier_panic = 3,
	NUMBER_OF_AI_ATOM_ANIMATION_MODE_MODIFIERS = 4,
};

enum
{
	_ai_atom_crouch_modifier_disable = 0,
	_ai_atom_crouch_modifier_enable = 1,
	NUMBER_OF_AI_ATOM_CROUCH_MODIFIERS = 2,
};

enum
{
	_actor_atom_grenade_modifier_toss = 0,
	_actor_atom_grenade_modifier_lob = 1,
	_actor_atom_grenade_modifier_bounce = 2,
	NUMBER_OF_AI_ATOM_GRENADE_MODIFIERS = 3,
};

enum
{
	_ai_atom_vehicle_modifier_any_non_driver = 0,
	_ai_atom_vehicle_modifier_gunner = 1,
	_ai_atom_vehicle_modifier_passenger = 2,
	_ai_atom_vehicle_modifier_driver = 3,
	_ai_atom_vehicle_modifier_any_seat = 4,
	NUMBER_OF_AI_ATOM_VEHICLE_MODIFIERS = 5,
};

enum
{
	_ai_atom_animate_modifier_relative_movement = 0,
	_ai_atom_animate_modifier_absolute_movement = 1,
	_ai_atom_animate_modifier_absolute_movement_no_collision = 2,
	_ai_atom_animate_modifier_no_interpolation_relative_movement = 3,
	_ai_atom_animate_modifier_no_interpolation_absolute_movement = 4,
	_ai_atom_animate_modifier_no_interpolation_absolute_movement_no_collision = 5,
	NUMBER_OF_AI_ATOM_ANIMATE_MODIFIERS = 6,
};

enum
{
	_ai_atom_action_modifier_berserk = 0,
	_ai_atom_action_modifier_surprise_front = 1,
	_ai_atom_action_modifier_surprise_back = 2,
	_ai_atom_action_modifier_evade_left = 3,
	_ai_atom_action_modifier_evade_right = 4,
	_ai_atom_action_modifier_dive_forward = 5,
	_ai_atom_action_modifier_dive_back = 6,
	_ai_atom_action_modifier_dive_left = 7,
	_ai_atom_action_modifier_dive_right = 8,
	_ai_atom_action_modifier_vehicle_woohoo = 9,
	_ai_atom_action_modifier_vehicle_scared = 10,
	NUMBER_OF_AI_ATOM_ACTION_MODIFIERS = 11,
};

enum
{
	_ai_atom_targeting_modifier_enable = 0,
	_ai_atom_targeting_modifier_disable = 1,
	NUMBER_OF_AI_ATOM_TARGETING_MODIFIERS = 2,
};

enum
{
	_ai_atom_initiative_modifier_enable = 0,
	_ai_atom_initiative_modifier_disable = 1,
	NUMBER_OF_AI_ATOM_INITIATIVE_MODIFIERS = 2,
};

enum
{
	_ai_atom_wait_modifier_alerted = 0,
	_ai_atom_wait_modifier_visible_enemy = 1,
	_ai_atom_wait_modifier_told_to_advance = 2,
	NUMBER_OF_AI_ATOM_WAIT_MODIFIERS = 3,
};

enum
{
	_ai_atom_die_modifier_normal = 0,
	_ai_atom_die_modifier_silent = 1,
	NUMBER_OF_AI_ATOM_DIE_MODIFIERS = 2,
};
```

The PDB directly authenticates the go-to values, all three wait values, and
the exact value names used by begin for crouch enable, targeting enable,
initiative enable, and silent death.  It also authenticates look values 1..4
and every animate/action value.  The retained database headers below
authenticate the complete look domain (including idle-aim 0), animation-mode,
grenade, vehicle, animate, and action domains.  Paired opposite values and
terminal counts for the two-valued domains, plus the go-to/wait counts, are
complete-domain corroboration from the retained Demon header; do not describe
those counts as separately emitted PDB symbols.

There is no wait `always` member.  The adjacent January `"always"` literal is
`_ai_atom_loop_modifier_always = 0`; the already-owned loop value
`_ai_atom_loop_modifier_until_told_to_advance = 1` remains unchanged.  The
already-admitted reconstructed move-facing block also remains unchanged and
is not another prerequisite domain.

The frozen begin proposal still spells five comparisons numerically.  A
future semantic application should use the genuine names without treating
that as a source-shape experiment:

- go-to `== 1` -> `_ai_atom_go_to_modifier_keep_moving`;
- crouch `== 1` -> `_ai_atom_crouch_modifier_enable`;
- targeting `== 0` -> `_ai_atom_targeting_modifier_enable`;
- initiative `== 0` -> `_ai_atom_initiative_modifier_enable`;
- die `== 1` -> `_ai_atom_die_modifier_silent`.

No exact-name local shadows of these twelve domains exist in canonical
source.  The present `ai_scenario_definitions.h` loop and move-facing blocks
must not be duplicated.

## Exact scenario-reference records

These are complete tag-block element records and belong in
`source/ai/ai_scenario_definitions.h`, adjacent to the other command-list
schema.  Direct `HCEX_Release.pdb -type` output authenticates every field
name, field type, offset, and total size; none of the fields below is a
speculative tail name.

```c
struct ai_animation_reference_definition
{
	char animation_name[TAG_STRING_LENGTH + 1];
	struct tag_reference animation_graph;
	unsigned long unused[3];
};

struct ai_script_reference_definition
{
	char script_name[TAG_STRING_LENGTH + 1];
	unsigned long unused[2];
};

struct ai_recording_reference_definition
{
	char recording_name[TAG_STRING_LENGTH + 1];
	unsigned long unused[2];
};

typedef char ai_animation_reference_definition_size_assert[
	sizeof(struct ai_animation_reference_definition) == 0x3C ? 1 : -1];
typedef char ai_script_reference_definition_size_assert[
	sizeof(struct ai_script_reference_definition) == 0x28 ? 1 : -1];
typedef char ai_recording_reference_definition_size_assert[
	sizeof(struct ai_recording_reference_definition) == 0x28 ? 1 : -1];
```

The exact offsets are animation name `+0`, tag reference `+0x20`, unused
dwords `+0x30`; script/recording name `+0`, unused dwords `+0x20`.

The only current declaration conflict is the complete local
`struct ai_animation_reference_definition` at
`source/ai/action_alert.c:76`.  It must be removed so that the already-present
`#include "ai_scenario_definitions.h"` supplies the owner.  Its local
`long unused[3]` has the right width but the PDB owner type is
`unsigned long`.  There is no current script- or recording-reference shadow.
The `scenario` structure's corresponding members remain `struct tag_block`;
no scenario prefix or retyping is required.

## Exact unit-animation-impulse owner declaration

`source/units/units.h` is the genuine public constant owner.  Again use an
anonymous enum and keep the existing public function parameters as `long`
and the private mapper's parameters as `short`:

```c
enum
{
	_unit_animation_impulse_berserk = 0,
	_unit_animation_impulse_signal_move = 1,
	_unit_animation_impulse_signal_attack = 2,
	_unit_animation_impulse_signal_warn = 3,
	_unit_animation_impulse_surprise_front = 4,
	_unit_animation_impulse_surprise_back = 5,
	_unit_animation_impulse_evade_left = 6,
	_unit_animation_impulse_evade_right = 7,
	_unit_animation_impulse_dive_front = 8,
	_unit_animation_impulse_dive_back = 9,
	_unit_animation_impulse_dive_left = 10,
	_unit_animation_impulse_dive_right = 11,
	_unit_animation_impulse_vehicle_celebrate = 12,
	_unit_animation_impulse_vehicle_panic = 13,
	NUMBER_OF_UNIT_ANIMATION_IMPULSES = 14,
};
```

Direct `HCEX_Release.pdb -sym` queries return every value enumerator with
exactly these names and values.  The terminal count 14 is supported by the
complete January range/switch and database domain, not a separately emitted
PDB constant.  The immutable HCEA header's older `GUESS` prose predates this
direct PDB check and does not downgrade the now-authenticated value names.

Current canonical has no local value-name shadow.  Remove only
`NUMBER_OF_UNIT_ANIMATION_IMPULSES = 14` from the mixed block near
`source/units/units.c:708`; retain `_unit_seat_unknown8_bit = 8`.
For a complete semantic migration, replace the raw 0..13 cases in both
switches of private `code_00198e40` and the raw 12..13 range tests in private
`code_00198fd0` with these owner constants.  The PDB authenticates
`code_00198e40` as
`static short unit_animation_impulse_get_index(short, short *)`; that private
symbol rename is useful metadata cleanup but is not an API prerequisite and
must not be put in a header.

## Prototype and include boundary

The corrected performer needs no prototype or type beyond the four owner
domains it names: go-to, wait, fire-target, and combat-status.  Its callees
are already declared by their genuine headers.

The begin proposal needs no additional shared type/prototype beyond the
modifier domains, unit impulse, and three reference records above.  Two
direct-owner includes are required when it is placed into the real TU:

- `#include "units/unit_definitions.h"` for the
  `unit_definition_get(index)` owner macro and complete
  `struct unit_definition` dereference;
- `#include "cutscene/recorded_animations.h"` for
  `boolean recorded_animation_play(long unit_index, short animation_index)`.

Preserve the existing direct `#include "math/real_math.h"`; it owns the
normalization/vector APIs, including the accepted 3D-prefix-as-2D convention.
`actor_move_keep_moving_past_destination(long)` is already correctly owned by
`actors.h` and matches the definition in `actor_moving.c`.  Do not add any
caller-local extern declarations.

All other required owners are already present: `actions.h`, `actors.h`,
`actor_definitions.h`, `ai_communication.h`, `props.h`, `objects.h`,
`scenario.h`, `units.h`, `bipeds.h`, `hs.h`, and the recorded-animation,
object-definition, player, data, and C runtime owners selected in the natural
candidate skeleton.

## Evidence and provenance

Authoritative local artifacts:

- PDB:
  `../../research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`,
  SHA-256
  `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`;
- secondary PDB:
  `../../research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX.pdb`,
  SHA-256
  `f55cfe957da8079a62a26a6753f1ddbb700b067a66de0bd5db5aac182d985ff1`;
- DIA reader: `../../research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`,
  SHA-256
  `5be3b890fb5c3b7c74b7c3ce7c275a70f48813c388d3611c41e69c0ac0626243`;
- January `action_obey.obj`: current split artifact SHA-256
  `13a4e47de2c79a3122033539284d148cae052f5f008f084a3d15862dda844368`;
  the earlier frozen source-body audit recorded SHA-256
  `0e4f889a35b97c44f6ae91e4906b547bd64f2ff12b9f399e02b46f02b69633fb`
  before later semantic target-metadata regeneration;
- retained full-domain header:
  `../../research/players_update_cluster_research/upstream/demon/source/ai/ai_scenario_definitions.h`,
  SHA-256
  `d650fce71f3f8f1cc9e1c5e02b350bb56b65b8de385ef1a7d93257410de2106c`.

Immutable HCEA database extracts, used as corroborative reconstructions rather
than original source:

```text
33fb4dd6bdc543a755d068186d18f472e679bcfd12835ffa020866dbef33db27  ai_atom_animation_mode_modifier.h
b9cf105849bd637b2c1b7011578ddb204f67e35ac5dce696eb6217a12323e412  actor_atom_grenade_modifier.h
586f4f8473f15d34370598a74ae3637a4d377ca37b938398de892ca0633deed8  ai_atom_vehicle_modifier.h
66b289cef8e9af8e497f552bdbf2a149b0396d6814800f0db1d7d82f73aa0570  ai_atom_look_modifier.h
df5ed09edc01002f3568b353cdf5ae24784a294b8ddcc92d9b6ccf3c9171851c  ai_atom_animate_modifier.h
1919399bdc4cf367a73e1e71d7c17abdf5a2c5b481b40b97ea6e020362c832fc  ai_atom_action_modifier.h
6d367a2adacf78a8c89b7e8a2cc566572e5fd0419fe92fd979affcd5abe74020  unit_animation_impulse.h
c34d83224cf95b071a9de847bdd1a98e63bde836ae6d622beb3351a990b2da72  ai_animation_reference_definition.h
16f251ee304111c333f39f2a495da1b65dcd91d154a151592a8f2f81ac63330f  ai_script_reference_definition.h
b96e456964ea532b65afac96fbf4360f8cee954d51beb0ab374127a0ced4f68d  ai_recording_reference_definition.h
```

All ten files are under
`../../research-cache/halocea-full-blobs-20260830/src/headers/`.

Final supporting scratch revisions:

```text
630ee045a17f3a095929516cd524c1ee40a385eb08d75a2b133ed99ebcca5ead  action_obey_command_begin_proposal_20260905.c
1a9392214a2ca5be3eb0f214d38cc0c67f3d18ddabaa339df27165c7726c774b  action_obey_command_perform_proposal_20260905.c
46e78d3c070638fab00d3f0c5334088b91146f92224330a5964ee4b358d8cd57  action_obey_actor_enum_owner_packet_20260905.md
1e554e3d6f43775d645302ffb4d0eb297e7e65d51983a938b006282f836a23af  action_obey_command_begin_proposal_issues_20260905.md
```

## Provenance limits and final boundary

- Enum tag names are not PDB-authenticated; use anonymous enums.
- PDB proves the value enumerators described above.  Terminal counts and the
  non-emitted counterparts in two-valued domains are domain-completion
  evidence from the retained source/database and January range behavior.
- The three scenario-reference records are stronger: every field name/type,
  offset, and size is direct PDB evidence.  No field is a speculative facade.
- HCEA `src/blam` bodies and extracted headers are binary reconstructions, not
  recovered first-party source.
- The frozen begin's numeric boolean comparisons are known owner-name debt,
  not an invitation to edit or compile another variant now.
- The failed 86-consumer owner trial is the current integration blocker.
  Reopen only with a genuinely changed shared-header/consumer context, not
  spelling, enum-tag, declaration-order, include-order, or compiler-steering
  experiments.
