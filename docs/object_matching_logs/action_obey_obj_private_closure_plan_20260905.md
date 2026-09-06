# Fable `action_obey` private-closure intake (2026-09-05)

Read-only/source-plan audit. No production, donor, build, configuration, or generated-object file was changed or compiled. This note does not change the earlier intake accounting: the initial Fable report started at 9 exact / 0 residual / 18 unwritten, and the reported donor `+11` remains donor evidence rather than admitted campaign progress. This audit admits zero functions.

## Conclusion

The two missing routines can be restored as a coherent, genuine same-translation-unit closure:

```c
static boolean action_obey_command_begin(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control);

static boolean action_obey_command_perform(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control);
```

January proves that these are private register-ABI functions, not public ABI. `_code_000060c0` receives the complex-control pointer in EAX and unit index in ECX; actor index, command-list index, and simple-control pointer remain stack arguments. `_code_000071a0` receives unit index in EAX and its other four arguments on the stack. Those allocations are consistent with VC7 optimizing the ordinary source signature above after it sees the same-TU callers. The audit aliases should name the private sections, but production source must not declare either body non-static or manufacture an external relocation.

Once both bodies exist, the retained `action_obey_individual_perform` has a real private callee closure and can continue to support public `action_obey_perform`. There is no missing third private body behind either routine. The remaining preconditions are authentic shared schema and owner prototypes, not an ABI invention.

## Primary evidence

- January PC object: `build/split/source/ai/action_obey.obj`, SHA-256 `0e4f889a35b97c44f6ae91e4906b547bd64f2ff12b9f399e02b46f02b69633fb`.
  - `_code_000060c0`: 4,320 padded bytes, 167 relocations, normalized SHA-256 `5b56fdd8b88a4456f893740a2f972a21d114b032c636f6b90c9d86e0114fd640`.
  - `_code_000071a0`: 1,408 padded bytes, 51 relocations, normalized SHA-256 `afc109dc8ddf7e3f4ee6ed60f606b1e597702f063a81488fb2f8fd24f3713b07`.
- Semantic donor, read in full:
  - `research-cache/halocea-full-blobs-20260830/src/blam/ai/action_obey_command_begin.c`, SHA-256 `4373d536adedb3dbdadfeaf62cfd1369ba2d1c53e92acae8477b6837a0a9461f`.
  - `research-cache/halocea-full-blobs-20260830/src/blam/ai/action_obey_command_perform.c`, SHA-256 `bd22ee99d261de2f8b1db20ca548bb2158031a7e0d907bdfd78a9813842bd531`.
  - `research-cache/halocea-full-blobs-20260830/src/blam/ai/action_obey_individual_perform.c`, SHA-256 `05d10cbc643b6b1b3eee5fe725bb257188a9521ea09df5deeb1370775532dade`.
  - `research-cache/halocea-full-blobs-20260830/src/blam/ai/action_obey_directmovement_update_facing.c`, SHA-256 `3295935a5a1602fe4cdbfba17bf2762734699ef1e1bc981aae5c346a6bdcb042`.
- Retained PC source is not a full donor: Stian's `src/halo/ai/action_obey.c` is zero bytes; Punpckhdq retains the January symbol/literal manifest only; Pastudan history commit `ba72a7d7` contains only a raw-float `cross_product3d` reconstruction. It independently corroborates that math operation, but supplies neither private body.
- Fable source reviewed: `C:/halo-worktrees/fable-50k-small-families-20260904/source/ai/action_obey.c`, SHA-256 `067b4900d5494c5341ca609337f0d220dedd4f95778a935d601427de96575a59`.

HCEA is an Xbox/PPC semantic donor. January PC disassembly, relocation destinations, literals, assertion strings, record strides, and register/stack use take precedence wherever the two diverge.

## Natural `command_begin` source plan

Start by resolving the typed actor, command list, current command, point block, actor definition, and actor variant. Return false if `current_command_index` is outside the command count. Dispatch on the 28 `ai_atom_type` values and use the existing simple/complex control fields:

1. `pause`: convert seconds to ticks (`parameter1 * 30`).
2. `go_to` / `go_to_and_face`: validate point indices; set destination point/surface/keep-moving state; call `actor_move_to_point`; optionally call `actor_move_keep_moving_past_destination`; for the facing form, set the typed facing point.
3. `move_direction`: sample actor body position or `object_get_origin`; for a point command call `vector_from_points3d` then `normalize3d`, otherwise use `vector3d_from_angle`; set the facing mode, halt the actor when it owns the unit, and set direct-movement flags.
4. `look`, `look_random`, `look_player`, `look_object`: resolve a command point, active prop, nearest player, or named object; use `random_range(low, high + 1)` and `real_random_range` for the random look; construct a complete local `direction_specification` directly and call `actor_look_secondary`; store the duration in ticks. Do **not** import HCEA's one-argument/variadic `direction_get_empty` declaration: January has no such relocation and writes the local type/object/point fields directly.
5. `animation_mode`, `crouch`, `shoot`, `grenade`: validate domains and populate complex control. Grenade setup takes the grenade type from the actor variant, calls `unit_add_grenade_type_to_inventory`, and initializes trajectory/target/timer state.
6. `vehicle`: iterate `_object_mask_vehicle`, keep at most 16 typed `vehicle_possibility` records, compute distance, `qsort` with the already genuine same-TU `vehicle_possibility_qsort`, and try candidates through `actor_action_try_to_enter_vehicle` using the ordinary empty string literal. The record is TU-private; no public header is owed.
7. `running_jump`, `targeted_jump`: reject the actor's mounted unit case; set jump bits/timers/velocity. January uses `dot_product3d` for the non-actor unit's facing/velocity test.
8. `script`, `animate`, `recording`: call `hs_wake_by_name`; resolve the animation reference/default object animation graph and call `unit_start_user_animation`; update typed biped runtime flags; or resolve/play a recorded animation.
9. `action`, `vocalize`: map modifiers to animation impulses/communication types, or fill a real `unit_speech_item`, initialize its `ai_information_packet`, and call `unit_speak`.
10. `targeting`, `initiative`, `wait`, `loop`, `die`, `move_immediate`, `set_radius`, `teleport`: update the named flag/state fields. Teleport filters with `_object_mask_biped`, reads `biped_definition.biped.flags`, uses full 3D facing for flying/climb-anything bipeds and a typed 2D facing otherwise, then calls `object_set_position`, `object_reset`, `object_compute_node_matrices_recursive`, `actor_input_sample_position`, and `actor_move_halt`.

The PC-only debug epilogue must also be retained. When `ai_debug` is enabled, January formats `<no encounter>` or `encounter/squad`, calls `action_obey_describe_command(..., temporary, 256)`, and reports `"%s: %s #%d%s: %s"` with either `""` or `" FAILED"`. HCEA returns immediately after the switch and does not contain this epilogue.

PC-shape corrections relative to HCEA include ordinary `random_range` / `real_random_range`, `vector_from_points3d`, `distance_squared3d`, and `dot_product3d` calls; direct construction of the look-direction record; the PC debug epilogue; the January empty-string literal; and typed biped/object/tag access. HCEA's seed calls, manually expanded vector/distance products, `nullptr`, raw object header casts, and its `direction_get_empty` facade are semantic hints only.

## Natural `command_perform` source plan

Resolve the typed command and actor, default `done` to true, and use the January switch grouping:

- `pause` and all look atoms, plus `move_immediate`: finish when `pause_timer == 0`.
- `go_to` / `go_to_and_face`: for the actor's own unit with complex control, test `actor_path_at_destination`; apply keep-moving tolerance/velocity logic; wait ten ticks while the actor is still moving; when destination facing is active, assert at source line 983 that the current atom is `_ai_atom_go_to_and_face`, normalize a 3D or 2D vector to the facing point, compare against `0.98400003f`, then halt on completion.
- `move_direction`: obtain actor body position or object origin, form a vector from the stored start position, and compare its dot product with the commanded travel distance.
- Immediate atoms (`animation_mode`, `crouch`, `vehicle`, `script`, `targeting`, `initiative`, `loop`, `die`, `set_radius`, `teleport`) return true.
- `shoot`: require the actor unit and complex control; compare the manual fire target with the ordered point (`distance_squared3d < 0.25f`); otherwise set pause from `actor_variant_definition.ranged_combat.first_burst_delay_upper_bound * 30`, floored at 60.
- `grenade`: inspect `unit.grenade_throw_state` after start, or call `actor_combat_plan_grenade_trajectory` and depress the trigger when the unit is not busy.
- Jump atoms: require the jump flag, use actor `input.in_midair` or `unit_flying_through_air`, and clear the timer after the jumped bit and airborne state coincide.
- `animate`, `recording`, `action`, `vocalize`: inspect `unit.animation.state`, `recorded_animation_controlling_unit`, `complex_control->play_action`, or `unit.speech.current.priority` respectively.
- `wait`: preserve the January three-way combat-status/told-to-advance ladder.

Use `vector_from_points3d`, `distance_squared3d`, `magnitude_squared3d`, `dot_product3d`, `normalize3d`, and `normalize2d` as the typed math vocabulary instead of HCEA's manually expanded products. January's relocations authenticate every non-inlined call.

## Inline-math ownership and true callers

The four currently exact out-of-line bodies are authenticated `source/math/real_math.h` inline operations, but the present macro-renaming plus hand-appended definitions are a temporary facade. Preserve their current exact credit until the real callers are present, then remove the four include-time renames and four manual definitions atomically and let ordinary header ownership materialize them. A normal compile must prove all four sections remain exact before that cleanup is retained.

| Target section | January | True source caller | PC evidence |
| --- | --- | --- | --- |
| `_random_range` | 32 bytes, 2 relocs | `action_obey_command_begin`, `look_random` | call relocation at private-body `+0x25f`; body calls `get_global_random_seed_address` and `seed_random_range` |
| `_vector_from_points2d` | 32 bytes, 0 relocs | `action_obey_command_perform`, non-flying destination-facing branch | the helper is inlined into `+0x211..+0x22e`: two point-coordinate subtractions followed by `normalize2d`; emitted wrapper plus inline sequence identifies the reference |
| `_cross_product3d` | 64 bytes, 0 relocs | `action_obey_directmovement_update_facing` | two direct call relocations at `+0x9e` and `+0xd0` |
| `_negate_vector3d` | 48 bytes, 0 relocs | `action_obey_directmovement_update_facing` | backwards/right cases are inlined at `+0x72..+0x88` and `+0x123..+0x138` |

For the 2D facing call, use a genuinely typed `real_point2d`/`real_vector2d` projection or authentic shared view. Do not recover it with a pointer cast between unrelated point/vector types, an inactive-union read, or a prefix facade merely to provoke the wrapper.

## Owner headers and API closure

Most dependencies already have the correct owner declarations and should be consumed by includes, not redeclared in `action_obey.c`:

| Owner | Required declarations/types |
| --- | --- |
| `source/ai/actions.h` | shared obey simple/complex/state types; `actor_action_try_to_enter_vehicle`; public ActionObey declarations |
| `source/ai/actors.h` | `actor_datum`, `direction_specification`, movement/look/path/grenade APIs, `actor_input_sample_position` |
| `source/ai/actor_definitions.h` | actor/variant tags, idle-look range, ranged first-burst delay, grenade properties |
| `source/ai/ai_scenario_definitions.h` | command-list/command/point records and all atom/command-list/modifier domains |
| `source/ai/props.h` | `prop_iterator`, `prop_iterator_new/next`, `prop_get_active_by_unit_index` |
| `source/ai/ai_communication.h` and `source/ai/ai.h` | communication enums and complete `ai_information_packet` |
| `source/game/players.h`, `source/memory/data.h` | `player_data`, `player_datum`, typed `data_iterator_new/next` |
| `source/objects/objects.h`, `source/objects/object_definitions.h` | object iterators/accessors/origin/name/position/reset/node matrices, masks, damage flags, animation graph tag |
| `source/units/units.h`, `source/units/bipeds.h`, `source/units/biped_definitions.h` | unit speech/animation/grenade/facing APIs and complete unit/biped runtime/tag fields |
| `source/scenario/scenario.h`, `source/scenario/scenario_definitions.h` | `global_scenario_get`, command/reference blocks, encounter/squad debug names |
| `source/cutscene/recorded_animation_definitions.h`, `source/cutscene/recorded_animations.h` | scenario animation lookup and recorded-animation control/playback |
| `source/hs/hs.h` | `hs_wake_by_name` |
| `source/math/real_math.h` | all point/vector/random helpers and constants |
| `<stdlib.h>` plus project cseries/error headers | `qsort`, memset/string/format/error/debug support |

Two genuine owner-prototype gaps were found:

1. `actor_move_keep_moving_past_destination(long)` is defined by `source/ai/actor_moving.c` but is absent from `source/ai/actors.h`; `source/ai/action_charge.c` currently carries a foreign C-local declaration. Add the owner prototype to `actors.h` and remove consumer-local copies.
2. `recorded_animation_play(long, short)` is defined by `source/cutscene/recorded_animations.c` but is absent from `source/cutscene/recorded_animations.h`; `source/hs/hs.c` currently declares it locally (with a `word` second parameter). Add the definition-compatible owner prototype to `recorded_animations.h` and remove/repair foreign copies.

No `direction_get_empty` declaration should be added: it is not a January dependency. `vehicle_possibility_qsort`, `action_obey_directmovement_update_facing`, `action_obey_command_begin`, and `action_obey_command_perform` remain genuine statics in this TU. `qsort` comes from the standard header, not a handwritten extern.

## Shared schema/enum prerequisites

Before integration, the owning headers must supply one authentic definition for:

- `obey_individual_simple_control` (0x24), `obey_individual_complex_control` (0x58), and `obey_state_data` (0x84), including direct-movement/jump union arms and every destination/action/shoot/grenade field;
- the complete shared actor/swarm layouts already being audited separately; no obey-only prefix, opaque tail, or parallel incompatible record is acceptable;
- `direction_specification`, `unit_speech_item`, `ai_information_packet`, `vehicle_possibility` (TU-private), and the command/reference records with January strides: command list 0x60, command 0x20, point 0x14;
- the command-list flags, simple-control bits, atom modifier domains (look, move facing, grenade, animate, action, loop), unit animation impulses, biped runtime flags, and existing biped-definition/communication/speech/object domains in their natural owners.

Current Fable-local enums explicitly labeled “belongs in ...” should migrate to those owners with the full packet; they are not a reason to create another private copy. The exact signedness of the shared swarm component flags remains part of the separate schema closure and must not be inferred from this pair of functions.

## Admissible packet boundary

A coherent implementation packet is:

1. land the authentic shared schema and the two owner-prototype fixes;
2. include the actual owners;
3. add both private static bodies together, including the PC-only assertion/debug behavior and ordinary typed accesses;
4. keep `action_obey_individual_perform` calling those statics and remove the two non-static placeholder declarations;
5. reconcile the four math wrappers only after their true calls are present, in the same verified packet;
6. add audit-only aliases from the two authentic names to `_code_000060c0` / `_code_000071a0` for measurement.

This packet is source/API/schema feasible and does not require an external facade. `command_perform` is the smaller, higher-confidence body; `command_begin` is large but semantically closed. Neither should be admitted alone if the goal is to retain public perform without unresolved private ABI.
