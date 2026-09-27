# A1 owner questions - source/game/game_engine admission (measured at HEAD fe283cc5)

Nothing here lands without the owner's answers. game_engine.obj is already 180/180 strict exact, its 3,792 data
bytes are credited, and tools/audit_object_admission.py lists it as a candidate with a 0/0 gap. The only thing at
stake is the object status: Halo objects 388 -> 389 (+1). No function, code-byte or data-byte credit changes.
Measurements: battery/ (00-10), CARDS.md (C01-C14), LOG.md. Patches: patches/.

## Q1. The motion-sensor copy in game_engine_player_get_custom_motion_sensor_positions (fifty-objects owner queue
item 6 Q1, re-measured at fe283cc5 under the 2026-09-26 rulebook)

Pick one: (A) the in-loop alias `struct netgame_goal *goal = &global_goal[goal_index];` then
`positions[count].x = goal->position.x; positions[count].y = goal->position.y;` (patch 01A, default), or (B) the
/Od-attested one-statement view copy `positions[count] = *(real_point2d *)&global_goal[goal_index].position;`
(patch 01B), or (C) neither, and game_engine stays held.

- Both are 180/180. The A and B objects are identical (keyed diff 0/302). Both sweep 447/447 SAME.
- Strip test at fe283cc5, with neither form (direct component copies): 179/180. The motion-sensor function is
  RESIDUAL by 4 instructions: the strength-reduced induction variable starts at `&_global_goal+4`, not +0
  (battery/08_q1_strip_alndiff.txt). So both forms are load-bearing.
- For B: /Od 0x5a20e0 has frame = count, player, goal_index (no pointer slot), computes the index once, and copies 8
  bytes with integer movs. That is an aggregate copy; this build copies aggregates with movs and moves float scalars
  with movss. real_point2d is the layout prefix of real_point3d, and HCEX's real_point3d has no 2-D member. January's
  assert strings stringify Bungie 3D->2D view casts (actors, actor_looking, actor_moving, observer).
  - Against B: rule 28 says a load-bearing struct view copy falls outside the admitted 3D-to-2D class unless it is
    separately approved.
- For A: plain C, and a goal-pointer local is first-party in this TU (/Od goal_matches_player 0x5b23c0 keeps
  `[ebp-8] = &global_goal[i]`).
  - Against A: the /Od frame of 0x5a20e0 has no such slot. It is a load-bearing named local without /Od attestation
    (rules 20 and 23).

## Q2. TU-local complete HCEX tag layouts and one consumer-local extern

Two parts:
- Q2a: do you accept the hudg, itmc and netgame layouts in game_engine.c (their only consumer in the tree) as
  admissible for now?
- Q2b: do you accept the consumer-local `extern struct hud_globals_definition *hud_globals;`?

Or should game_engine stay held until a board-wide HUD / scenario / item header consolidation?

What game_engine.c defines, all verbatim HCEX member names and complete (no opaque views):
- `struct hud_globals_definition` and its six nested types (0x450, 'hudg');
- `struct item_permutation_definition` and `struct item_collection_definition` ('itmc');
- `struct scenario_netgame_flag`, `scenario_netgame_equipment` and `scenario_starting_equipment`, with the HCEX
  starting-equipment flag enum.

Genuine-home placements measured at fe283cc5 (rule 16; rule 62 zero exact losses):
- **itmc into items/item_definitions.h** (which already owns ITEM_COLLECTION_DEFINITION_TAG): FAILS.
  - Result: units `_unit_preprocess_node_orientations` goes EXACT -> residual, and the residual
    `_render_weapon_hud` bytes move.
  - At 09f5208f the same move lost nothing (C10). The lead's structures.h +1 prototype in fe283cc5 changed the
    count, so the move is now a loss. Withdrawn (patches/WITHDRAWN_*).
- **Equipment layouts and their HCEX flag enums into scenario/scenario_definitions.h**: FAILS (C11). Measured at
  09f5208f, not re-run at fe283cc5.
  - `_ai_communication_update_speech_timers` goes EXACT -> residual.
  - `_unit_preprocess_node_orientations` goes EXACT -> residual.
  - The parked `_ai_test_ballistic_line_of_fire` bytes move.
- **scenario_netgame_flag into scenario_definitions.h**: needs king, oddball and race to drop their own local copies.
  Those are other units, and king is held.
- **hudg into hud_definitions.h** (HCEX game_engine.c includes it): needs the local `struct hud_globals_definition`
  copies consolidated in hs.c, hs_compile.c, hud.c (the definer of hud_globals), hud_messaging.c, hud_nav_points.c,
  hud_unit.c and ui_widget.c.
- **The extern**: it has the same form hud_messaging.c, hud_nav_points.c and hud_unit.c use. hud_unit was admitted
  with it (batch 5a precedent; per rule 2 a precedent alone is not a ruling).

## Q3b. The two descriptive .bss statics

Do you accept, labelled, the file statics `game_engine_teleport_message_ticks` (long) and
`game_engine_teleport_flash_fade_function` (short)?
- They are January's two .bss slots at +0x46C/+0x470. The names are symbols.json's descriptive names.
- The patch adds an honest comment. Rule 15 permits descriptive names that are labelled as such.

The alternative is HCEX's form: static locals `blocked_message_delay` and `fade_function` of
game_engine_update_teleporter.
- LAB C07: as static locals they land AFTER `_global_autogenerate_list/count` (+0x474/+0x478).
  - `.bss` becomes 1146 bytes instead of 1148.
  - update_teleporter goes residual.
  - object_audit FAILs.
- January's layout could only come from defining global_autogenerate_list/count after update_teleporter, for which
  there is no evidence, plus a symbols.json rename.

Q3's aggregate half is resolved in the packet (C06), with no symbols.json change:
- The invented `debug_player_color` aggregate became `short debug_player_color = NONE;` (the hs global,
  `_hs_type_short_integer`).
- It gained seven HCEX-named static locals of update_teleporter: screen_flash_type (short, because of January's word
  load; HCEX later widened it to long), max_intensity, alpha, red, green, blue and duration.
- The .data bytes are identical. 180/180. object_audit PASS, with text identical to production's.
- objdiff scores .data at 100%.
- The seven new static-local COFF symbols (`?name@?1??game_engine_update_teleporter@@9@9`) are disclosed. They
  resolve to January's `_debug_player_color+4..+0x1C` destinations.

## Q4. game_engine_playlist_next(long parameter0, long parameter1, long playlist_type)

January's body ignores all three parameters, HCEX's version has none, and the callers pass (0, 0, 4) and (0, 0, 2).
The names cannot be recovered. game_engine_playlist.h (shared) spells them the same way, and
ui_widget_event_handler_functions.c's local prototype leaves them unnamed.

Keep the names as disclosed placeholders? Or rename them (the header and the definition together; count-neutral)?

## Q5. Acknowledge the count couplings (rule 21 disclosure; all are genuine corrections)

- **R4+R5+R6** (HCEX constants for goal_radar, the game-matching options and multiplayer sounds) were adopted as a
  complete set.
  - I pre-registered that stance (C04) before measuring.
  - R6 alone flips `_populate_statistic_buffer`. Every set containing R6 plus R4 or R5 is exact.
  - Alternative: keep all three sets of literals. That is also 180/180.
- **The nav-point trio in the genuine hud.h** (C02) is EXACT at HEAD. It was residual at cdc8ebd3.
- **populate_statistic_buffer's band in the final candidate** is K=53..57 (mod 64):
  - removing 7-11 C1 numbers before it flips the function;
  - adding up to 52 is safe (battery/09).
- **The units canary `_unit_preprocess_node_orientations`** keeps less margin:
  - its nearest band moves from +12 names (HEAD) to +8 names (packet), because game_engine.h gains two prototypes;
  - units.c is exact in both.
- **Integration**: every packet that edits a header in game_engine.c's include closure (or units.c's,
  rasterizer_frame_statistics.c's or ai_communication.c's) must be re-swept together with this one.
