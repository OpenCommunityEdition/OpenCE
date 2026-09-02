# Opus 44,928-byte wave canonical reconciliation (2026-09-02)

## Purpose

This ledger reconciles the reported `+44,928` padded-byte Claude/Opus session on
`claude/breakable-surfaces-20260830` at `617703d2e` against canonical
`jonas/ai-debug-integration-20260829` at `1cc83929d`. It answers a narrower and
more useful question than comparing branch totals: which January code sections
were strict exact in Opus but were not strict exact in canonical?

The comparison used rename-stable keys of `unit + target code-section ordinal`.
That is important because canonical has replaced many `code_...` names with
evidence-backed semantic names. The frozen inputs are:

- `scratch/opus-617703d2e-stable.json`: 8,246 sections, 4,982 strict exact;
- `scratch/canonical-1cc83929d-stable.json`: 8,245 sections, 5,309 strict exact.

Canonical has 375 strict-exact sections absent from the older Opus snapshot.
Opus has only 48 strict-exact sections, totaling 15,696 padded bytes, which are
not strict exact in that canonical snapshot. Therefore the reported 44,928-byte
session gain is not a 44,928-byte canonical deficit.

## Reconciliation result

| Disposition at the frozen canonical baseline | Functions | Padded bytes |
| --- | ---: | ---: |
| Truthful ordinary-C implementation retained as a measured residual | 25 | 11,456 |
| Credible strict-exact recovery admitted in this wave | 1 | 704 |
| Deliberately not emitted pending a real owner/caller or credible source | 22 | 3,536 |
| **Total Opus-only strict set audited** | **48** | **15,696** |

The 25 residual functions are not lost source progress. Canonical deliberately
keeps their semantic implementation and refuses only the Opus spelling that
made the bytes exact through assembly, forced inlining, `noinline`, volatile or
barrier steering, false BSS/type ownership, duplicate inline-helper bodies, or
other unsupported source shapes. Their detailed evidence remains in the object
ledgers cited below.

## Recovered exception: `ai_communication_started`

`_ai_communication_started` is the credible exception. The Opus body is
ordinary typed C, contains no compiler-steering construct, and compiles to all
704 January bytes with all 48 relocation identities. Its normalized SHA-256 is
`eed6aa664227288ffe15a7f7c3c51791dbe88c42ef947d279e9a66c5422a0cd8`
for both target and candidate.

The genuine call graph also emits the 672-byte speech-timer helper. Independent
HCEA source names that helper `ai_communication_update_speech_timers`, and names
the already-exact 208-byte reset helper
`actor_reset_idle_vocalization_timer`. Those semantic names replace
`code_00032530` and `code_000322f0` in source and `config/symbols.json`; neither
address-based name is retained.

The speech-timer helper itself is an honest residual: target and candidate are
both 672 padded bytes with 43 relocations, with target normalized SHA-256
`91999fda75815c6638478824f3efa53ee6717572f365530362cbec00e78a50ac`
and candidate SHA-256
`7db4861a5adf519e0457a0ffe785ff4d47e878fda47cc0a06206a529776ac91c`.
Objdiff reports 97.76382 percent similarity. The remaining differences are the
previously documented natural register/lifetime schedule; no register hint,
volatile access, barrier, pragma, assembly, false alias, or inert dependency is
introduced. The typed implementation is retained and fuzzy-parked.

The public `ai_communication_started` declaration is placed in its owner header,
`source/ai/ai_communication.h`. The two private helper declarations remain in
their owner translation unit. Typed object access uses `unit_get` and
`actor_get`; tag-block access uses `TAG_BLOCK_GET_ELEMENT`.

## Already retained as credible residuals

These 25 Opus-exact sections, totaling 11,456 padded bytes, already exist in
canonical as ordinary-C residual implementations:

- AI: `action_converse_perform`, `actor_combat_find_nearby_target`,
  `actor_combat_check_collateral_damage`, `infection_swarm_aim_jump`, and
  `find_turning_point`;
- platform/camera/cutscene: `get_mutex_from_pool`, `dead_camera_update`, and
  `recorded_animation_play_internal`;
- game engine: `game_engine_verify_current_map`;
- players: `unit_should_autopick_weapon`, `player_set_action_result`,
  `player_handle_powerup`, `player_handle_powerup_minor`,
  `player_update_powerups`, `player_teleport_on_bsp_switch`,
  `players_reconnect_to_structure_bsp`, `player_examine_nearby_vehicle`,
  `player_examine_nearby_device`, `player_examine_nearby_item`, and
  `player_examine_nearby_objects`;
- memory: `lrar_allocate`;
- object lights: `light_unmarked`, `light_mark`,
  `find_point_lights_for_object_in_cluster`, and `light_reconnect_to_map`.

The exact Opus forms are intentionally not restored. In particular:

- `action_converse_perform` used gameplay inline assembly to force a register;
- `actor_combat_find_nearby_target` disturbed the protected
  `point_from_line3d` inline schedule;
- `actor_combat_check_collateral_damage` requires an owner-header shape that
  regresses `units.obj` from 189/189;
- `game_engine_verify_current_map` and `lrar_allocate` depended on unsupported
  `noinline`/manual helper expansion;
- the Players group used combinations of copied helpers, dead emitters,
  volatile/barrier steering, duplicate fetches, and representation tricks;
- `recorded_animation_play_internal` reached exact only with false BSS
  ownership; and
- the Object Lights forms relied on duplicate bodies, false linkage, or helpers
  without a genuine emitting caller.

## Deliberately unwritten boundaries

The remaining 22 Opus-exact sections total 3,536 padded bytes. They stay
unwritten until their authentic source owner naturally emits them:

- `midpoint3d` (64): a fake standalone duplicate of the shared real-math inline;
- Decals helpers (704 total): `projection_from_vector3d`, `project_point2d`,
  `triple_product3d`, `plane2d_from_points`,
  `plane3d_from_point_and_normal`, and
  `bsp3d_get_plane_from_designator`; these were unowned standalone copies of
  shared inline/macro behavior;
- `player_examine_nearby_unit` (16): no truthful source form currently emits
  the target-only leaf;
- Weapons (1,632 total): `animation_update` has an unresolved authentic COMDAT
  grouping; `weapon_trigger_get_charged_fraction` and
  `weapon_export_function_values` atomically introduce a candidate-owned float
  constant that January treats as external; and
  `weapon_handle_potential_inventory_item` depended on a layout-only surrogate
  contradicting authenticated fields;
- Object Lights (1,056 total): the two anonymous 32-byte leaves,
  `render_debug_light`, `light_get_bounding_sphere`, `sample_lightmap`, and
  `sample_diffuse_texture` still lack a credible complete owner/caller graph;
- four Active Camouflage XDK wrappers (64 total): the targets are correctly
  named `IDirect3DDevice8` wrappers, but must be instantiated by the real typed
  cache/draw functions rather than copied, assembled, or emitted by a dummy
  anchor.

## Validation and provenance

The isolated gate moved `ai_communication.obj` from 36 exact / 0 residual / 12
unwritten to 37 exact / 1 residual / 10 unwritten. Every inherited exact section
remained exact. The exact recovery and helper were compiled from the complete
translation unit; no proof-only caller or dormant emission anchor is retained.

Primary local evidence:

- `docs/object_matching_logs/claude_wave_small_exact_batch_20260831.md`;
- `docs/object_matching_logs/players_obj_january_safe_harvest_20260902.md`;
- `docs/object_matching_logs/object_lights_obj_small_family_recovery_20260902.md`;
- `docs/object_matching_logs/game_engine_obj_jonas_no_fake_inline_cleanup_20260831.md`;
- `docs/object_matching_logs/thread_win32_obj_house_rule_correction_20260831.md`;
- `docs/object_matching_logs/action_converse_obj_jonas_ordinary_c_fuzzy_park_20260830.md`;
- `docs/object_matching_logs/actor_combat_obj_collateral_damage_rejected_20260831.md`;
- `docs/object_matching_logs/recorded_animations_obj_natural_recovery_20260830.md`;
- `docs/object_matching_logs/weapons_obj_jonas_safe_trio_and_charged_export_blocker_20260901.md`;
- `docs/object_matching_logs/lrar_cache_obj_jonas_no_fake_inline_cleanup_20260831.md`;
- `docs/object_matching_logs/rasterizer_xbox_active_camouflage_obj_jonas_credibility_recovery_20260830.md`;
- HCEA `src/blam/ai/ai_communication_started.c`,
  `ai_communication_update_speech_timers.c`, and
  `actor_reset_idle_vocalization_timer.c`.

January's Halo CE Xbox COFF remains the sole byte-admission authority. Donor
source supplies names, types, and control-flow evidence only.
