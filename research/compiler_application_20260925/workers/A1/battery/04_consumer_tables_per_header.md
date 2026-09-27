# Consumer sweep tables per lead-owned header patch (HEAD fe283cc5; all patches applied together = finalA)

Columns: revgate (gate.py logic vs January build/split) exact/residual/unwritten on the HEAD root and on the
candidate root; per-function A/B = identical row sets; keyed diff = tools/coff_compare.section_infos_equal over
every section-defining symbol vs build/base. Consumer lists = CL /showIncludes transitive census (plus plain
`grep -rl '#include "<header>"'` hits that are build units, marked (grep) in the combined table).

## A1_02_LEAD_game_engine_h.patch - game/game_engine.h: 37 consumer TUs

| consumer TU | HEAD exact/res/unw | candidate | A/B | keyed diff vs build/base |
|---|---|---|---|---|
| source/ai/encounters | 61/0/0 | 61/0/0 | identical | SAME |
| source/camera/dead_camera | 3/1/0 | 3/1/0 | identical | SAME |
| source/game/game | 26/1/0 | 26/1/0 | identical | SAME |
| source/game/game_engine | 180/0/0 | 180/0/0 | identical | SAME |
| source/game/game_engine_ctf | 39/0/0 | 39/0/0 | identical | SAME |
| source/game/game_engine_king | 29/0/0 | 29/0/0 | identical | SAME |
| source/game/game_engine_list | 0/0/0 | 0/0/0 | identical | SAME |
| source/game/game_engine_oddball | 38/0/0 | 38/0/0 | identical | SAME |
| source/game/game_engine_race | 36/0/0 | 36/0/0 | identical | SAME |
| source/game/game_engine_slayer | 27/0/0 | 27/0/0 | identical | SAME |
| source/game/game_globals | 4/0/0 | 4/0/0 | identical | SAME |
| source/game/game_statistics | 4/0/0 | 4/0/0 | identical | SAME |
| source/game/players | 68/2/0 | 68/2/0 | identical | SAME |
| source/interface/hud | 22/0/0 | 22/0/0 | identical | SAME |
| source/interface/hud_messaging | 36/0/0 | 36/0/0 | identical | SAME |
| source/interface/hud_unit | 22/0/0 | 22/0/0 | identical | SAME |
| source/interface/interface | 17/1/0 | 17/1/0 | identical | SAME |
| source/interface/motion_sensor | 17/2/0 | 17/2/0 | identical | SAME |
| source/interface/player_ui | 42/0/0 | 42/0/0 | identical | SAME |
| source/interface/ui_widget | 96/6/0 | 96/6/0 | identical | SAME |
| source/items/items | 18/0/0 | 18/0/0 | identical | SAME |
| source/items/projectiles | 29/1/0 | 29/1/0 | identical | SAME |
| source/items/weapons | 78/1/0 | 78/1/0 | identical | SAME |
| source/main/main | 93/1/1 | 93/1/1 | identical | SAME |
| source/math/random_math | 17/0/0 | 17/0/0 | identical | SAME |
| source/networking/network_client_manager | 52/0/0 | 52/0/0 | identical | SAME |
| source/networking/network_client_message_handler | 17/0/0 | 17/0/0 | identical | SAME |
| source/networking/network_game_manager | 19/0/0 | 19/0/0 | identical | SAME |
| source/networking/network_server_manager | 70/0/0 | 70/0/0 | identical | SAME |
| source/networking/network_server_message_handler | 22/0/0 | 22/0/0 | identical | SAME |
| source/objects/damage | 33/1/0 | 33/1/0 | identical | SAME |
| source/objects/object_lights | 43/0/0 | 43/0/0 | identical | SAME |
| source/objects/objects | 121/0/0 | 121/0/0 | identical | SAME |
| source/render/render | 13/0/0 | 13/0/0 | identical | SAME |
| source/saved games/playlist_profile | 14/0/0 | 14/0/0 | identical | SAME |
| source/units/bipeds | 43/8/0 | 43/8/0 | identical | SAME |
| source/units/units | 189/0/0 | 189/0/0 | identical | SAME |

**game/game_engine.h: 37 consumer TUs, 0 with any difference.**

## A1_03_LEAD_owner_prototypes.patch - game/player_control.h: 21 consumer TUs

| consumer TU | HEAD exact/res/unw | candidate | A/B | keyed diff vs build/base |
|---|---|---|---|---|
| source/ai/ai_debug | 59/1/0 | 59/1/0 | identical | SAME |
| source/camera/bored_camera | 9/0/0 | 9/0/0 | identical | SAME |
| source/camera/director | 28/0/0 | 28/0/0 | identical | SAME |
| source/camera/editor_flying_camera | 21/0/0 | 21/0/0 | identical | SAME |
| source/camera/following_camera | 8/0/0 | 8/0/0 | identical | SAME |
| source/effects/player_effects | 26/3/0 | 26/3/0 | identical | SAME |
| source/game/aim_assist | 16/0/0 | 16/0/0 | identical | SAME |
| source/game/game | 26/1/0 | 26/1/0 | identical | SAME |
| source/game/game_engine | 180/0/0 | 180/0/0 | identical | SAME |
| source/game/player_control | 49/0/0 | 49/0/0 | identical | SAME |
| source/game/players | 68/2/0 | 68/2/0 | identical | SAME |
| source/hs/hs_library_external | 36/0/0 | 36/0/0 | identical | SAME |
| source/input/input_abstraction | 10/0/0 | 10/0/0 | identical | SAME |
| source/interface/first_person_weapons | 34/0/0 | 34/0/0 | identical | SAME |
| source/interface/hud | 22/0/0 | 22/0/0 | identical | SAME |
| source/interface/hud_draw | 22/0/1 | 22/0/1 | identical | SAME |
| source/interface/hud_weapon | 14/2/0 | 14/2/0 | identical | SAME |
| source/interface/interface | 17/1/0 | 17/1/0 | identical | SAME |
| source/interface/motion_sensor | 17/2/0 | 17/2/0 | identical | SAME |
| source/interface/ui_widget | 96/6/0 | 96/6/0 | identical | SAME |
| source/main/main | 93/1/1 | 93/1/1 | identical | SAME |

**game/player_control.h: 21 consumer TUs, 0 with any difference.**

## A1_03_LEAD_owner_prototypes.patch - interface/hud_messaging.h: 8 consumer TUs

| consumer TU | HEAD exact/res/unw | candidate | A/B | keyed diff vs build/base |
|---|---|---|---|---|
| source/game/game_engine | 180/0/0 | 180/0/0 | identical | SAME |
| source/game/players | 68/2/0 | 68/2/0 | identical | SAME |
| source/hs/hs | 447/1/0 | 447/1/0 | identical | SAME |
| source/interface/hud | 22/0/0 | 22/0/0 | identical | SAME |
| source/interface/hud_messaging | 36/0/0 | 36/0/0 | identical | SAME |
| source/interface/interface | 17/1/0 | 17/1/0 | identical | SAME |
| source/interface/player_ui | 42/0/0 | 42/0/0 | identical | SAME |
| source/saved games/game_state | 24/0/0 | 24/0/0 | identical | SAME |

**interface/hud_messaging.h: 8 consumer TUs, 0 with any difference.**

## A1_03_LEAD_owner_prototypes.patch - sound/sound_classes.h: 6 consumer TUs

| consumer TU | HEAD exact/res/unw | candidate | A/B | keyed diff vs build/base |
|---|---|---|---|---|
| source/game/game | 26/1/0 | 26/1/0 | identical | SAME |
| source/game/game_engine | 180/0/0 | 180/0/0 | identical | SAME |
| source/sound/game_sound | 31/0/0 | 31/0/0 | identical | SAME |
| source/sound/sound_classes | 12/0/0 | 12/0/0 | identical | SAME |
| source/sound/sound_definitions | 7/0/0 | 7/0/0 | identical | SAME |
| source/sound/sound_manager | 65/0/0 | 65/0/0 | identical | SAME |

**sound/sound_classes.h: 6 consumer TUs, 0 with any difference.**

## A1_04_LEAD_hud_h_nav_point_trio.patch (+A1_05) - interface/hud.h: 10 consumer TUs

| consumer TU | HEAD exact/res/unw | candidate | A/B | keyed diff vs build/base |
|---|---|---|---|---|
| source/game/game | 26/1/0 | 26/1/0 | identical | SAME |
| source/game/game_engine | 180/0/0 | 180/0/0 | identical | SAME |
| source/game/players | 68/2/0 | 68/2/0 | identical | SAME |
| source/interface/hud | 22/0/0 | 22/0/0 | identical | SAME |
| source/interface/hud_messaging | 36/0/0 | 36/0/0 | identical | SAME |
| source/interface/hud_nav_points | 31/1/0 | 31/1/0 | identical | SAME |
| source/interface/hud_sounds | 1/0/0 | 1/0/0 | identical | SAME |
| source/interface/hud_unit | 22/0/0 | 22/0/0 | identical | SAME |
| source/interface/interface | 17/1/0 | 17/1/0 | identical | SAME |
| source/main/main | 93/1/1 | 93/1/1 | identical | SAME |

**interface/hud.h: 10 consumer TUs, 0 with any difference.**

## A1_06_LEAD_players_h_hcex_member_names.patch - game/players.h: 71 consumer TUs

| consumer TU | HEAD exact/res/unw | candidate | A/B | keyed diff vs build/base |
|---|---|---|---|---|
| source/ai/action_obey | 27/0/0 | 27/0/0 | identical | SAME |
| source/ai/actor_combat | 33/1/0 | 33/1/0 | identical | SAME |
| source/ai/actor_stimulus | 22/0/0 | 22/0/0 | identical | SAME |
| source/ai/actors | 76/0/0 | 76/0/0 | identical | SAME |
| source/ai/ai | 44/2/0 | 44/2/0 | identical | SAME |
| source/ai/ai_communication | 46/2/0 | 46/2/0 | identical | SAME |
| source/ai/ai_debug | 59/1/0 | 59/1/0 | identical | SAME |
| source/ai/ai_script | 116/0/0 | 116/0/0 | identical | SAME |
| source/ai/encounters | 61/0/0 | 61/0/0 | identical | SAME |
| source/camera/dead_camera | 3/1/0 | 3/1/0 | identical | SAME |
| source/camera/director | 28/0/0 | 28/0/0 | identical | SAME |
| source/camera/observer | 25/1/0 | 25/1/0 | identical | SAME |
| source/cseries/profile | 43/1/0 | 43/1/0 | identical | SAME |
| source/cutscene/cinematics | 16/1/0 | 16/1/0 | identical | SAME |
| source/cutscene/recorded_animations | 16/0/0 | 16/0/0 | identical | SAME |
| source/effects/effects | 38/3/0 | 38/3/0 | identical | SAME |
| source/effects/material_effects | 3/0/0 | 3/0/0 | identical | SAME |
| source/effects/particles | 20/0/0 | 20/0/0 | identical | SAME |
| source/effects/player_effects | 26/3/0 | 26/3/0 | identical | SAME |
| source/game/aim_assist | 16/0/0 | 16/0/0 | identical | SAME |
| source/game/cheats | 14/0/0 | 14/0/0 | identical | SAME |
| source/game/game | 26/1/0 | 26/1/0 | identical | SAME |
| source/game/game_engine | 180/0/0 | 180/0/0 | identical | SAME |
| source/game/game_engine_ctf | 39/0/0 | 39/0/0 | identical | SAME |
| source/game/game_engine_king | 29/0/0 | 29/0/0 | identical | SAME |
| source/game/game_engine_oddball | 38/0/0 | 38/0/0 | identical | SAME |
| source/game/game_engine_race | 36/0/0 | 36/0/0 | identical | SAME |
| source/game/game_engine_slayer | 27/0/0 | 27/0/0 | identical | SAME |
| source/game/game_statistics | 4/0/0 | 4/0/0 | identical | SAME |
| source/game/player_control | 49/0/0 | 49/0/0 | identical | SAME |
| source/game/player_queues_new | 23/0/0 | 23/0/0 | identical | SAME |
| source/game/player_rumble | 12/0/0 | 12/0/0 | identical | SAME |
| source/game/players | 68/2/0 | 68/2/0 | identical | SAME |
| source/hs/hs_library_external | 36/0/0 | 36/0/0 | identical | SAME |
| source/input/input_abstraction | 10/0/0 | 10/0/0 | identical | SAME |
| source/interface/first_person_weapons | 34/0/0 | 34/0/0 | identical | SAME |
| source/interface/hud | 22/0/0 | 22/0/0 | identical | SAME |
| source/interface/hud_draw | 22/0/1 | 22/0/1 | identical | SAME |
| source/interface/hud_messaging | 36/0/0 | 36/0/0 | identical | SAME |
| source/interface/hud_nav_points | 31/1/0 | 31/1/0 | identical | SAME |
| source/interface/hud_unit | 22/0/0 | 22/0/0 | identical | SAME |
| source/interface/hud_weapon | 14/2/0 | 14/2/0 | identical | SAME |
| source/interface/interface | 17/1/0 | 17/1/0 | identical | SAME |
| source/interface/motion_sensor | 17/2/0 | 17/2/0 | identical | SAME |
| source/interface/player_ui | 42/0/0 | 42/0/0 | identical | SAME |
| source/interface/ui_widget | 96/6/0 | 96/6/0 | identical | SAME |
| source/items/items | 18/0/0 | 18/0/0 | identical | SAME |
| source/items/projectiles | 29/1/0 | 29/1/0 | identical | SAME |
| source/items/weapons | 78/1/0 | 78/1/0 | identical | SAME |
| source/main/main | 93/1/1 | 93/1/1 | identical | SAME |
| source/networking/network_client_manager | 52/0/0 | 52/0/0 | identical | SAME |
| source/networking/network_client_message_handler | 17/0/0 | 17/0/0 | identical | SAME |
| source/networking/network_game_globals | 26/0/0 | 26/0/0 | identical | SAME |
| source/networking/network_game_manager | 19/0/0 | 19/0/0 | identical | SAME |
| source/networking/network_server_manager | 70/0/0 | 70/0/0 | identical | SAME |
| source/networking/network_server_message_handler | 22/0/0 | 22/0/0 | identical | SAME |
| source/objects/damage | 33/1/0 | 33/1/0 | identical | SAME |
| source/objects/objects | 121/0/0 | 121/0/0 | identical | SAME |
| source/physics/collision_debug | 1/0/0 | 1/0/0 | identical | SAME |
| source/rasterizer/rasterizer_frame_statistics | 10/0/0 | 10/0/0 | identical | SAME |
| source/render/render_debug | 36/0/0 | 36/0/0 | identical | SAME |
| source/render/render_objects | 22/0/0 | 22/0/0 | identical | SAME |
| source/render/render_particles | 3/0/0 | 3/0/0 | identical | SAME |
| source/saved games/game_state | 24/0/0 | 24/0/0 | identical | SAME |
| source/scenario/scenario | 46/0/0 | 46/0/0 | identical | SAME |
| source/sound/game_sound | 31/0/0 | 31/0/0 | identical | SAME |
| source/sound/sound_manager | 65/0/0 | 65/0/0 | identical | SAME |
| source/structures/structure_detail_objects | 15/0/0 | 15/0/0 | identical | SAME |
| source/units/bipeds | 43/8/0 | 43/8/0 | identical | SAME |
| source/units/units | 189/0/0 | 189/0/0 | identical | SAME |
| source/units/vehicles | 38/1/0 | 38/1/0 | identical | SAME |

**game/players.h: 71 consumer TUs, 0 with any difference.**

