# RULING - admitting source/game/game_engine (base cdc8ebd3)

Nothing in this packet lands without the owner's answers. Measurements are in MEASURED.md and every step is in
LEDGER.md. Patches and apply order are in patches/ (APPLY_ORDER.txt).

## Question 1 (the pick): which motion-sensor copy in `game_engine_player_get_custom_motion_sensor_positions`?

> "game_engine is 180/180 strict exact and passes the whole-object audit either way. Both spellings give the same
> object: not one byte differs. Do you admit (A) the in-loop alias `struct netgame_goal *goal =
> &global_goal[goal_index];`, which the /Od frame does not have, or (B) the /Od-attested view copy
> `positions[count] = *(real_point2d *)&global_goal[goal_index].position;`, which is load-bearing, so it fails the
> byte-inert condition of your rule-24 narrow admission? Or neither, and game_engine stays held?"

Side effects of a YES to either option (the same for both):

- game_engine.c is rewritten per review4's F01R house-clean. The shared headers change: game_engine.h (37 TUs),
  player_control.h (21), hud_messaging.h (8) and sound_classes.h (6).
- F02 adds 4 new headers (see question 2). hud_nav_points.c gets an include.
- config.json flips game_engine to Matching. Halo objects go from 379 to 380.
- The full 447-TU sweep has 0 regressions and 0 park drift.
- No newly emitted symbol. The same 11 surplus header-inline COMDATs as production are emitted (identical to
  January's selected copies, links pass), and every caller is strict exact.
- symbols.json is unchanged.

### The /Od readout for this function (2020 build 0x5a20e0, quoted)

```
005a20e3  sub     esp, 0xc                       ; frame: count [ebp-4] (short), player [ebp-8], goal_index [ebp-0xc]
005a2188  mov     ecx, dword ptr [ebp - 0xc]     ; goal_index
005a218b  shl     ecx, 5                         ; * sizeof(struct netgame_goal), computed ONCE
005a218e  mov     edx, dword ptr [ecx + 0xc569d8] ; global_goal[i].position.x  (integer mov)
005a2194  mov     eax, dword ptr [ecx + 0xc569dc] ; global_goal[i].position.y  (integer mov)
005a219a  movsx   ecx, word ptr [ebp - 4]        ; count
005a219e  mov     esi, dword ptr [ebp + 0xc]     ; positions
005a21a1  mov     dword ptr [esi + ecx*8], edx
005a21a4  mov     dword ptr [esi + ecx*8 + 4], eax
```

In this build, an aggregate copy compiles to integer `mov` pairs. See /Od get_goal_position 0x59d5b9..0x59d5c6,
which is `*position = global_goal[index].position;`. Float scalars move with `movss` instead (for example /Od
0x59ac8e..0x59acb5). So 0x5a20e0 is **one 8-byte aggregate assignment** with the index computed once. There is no
pointer local.

### Option A: in-loop alias (review4's default; patches/optionA/)

What lands, verbatim (no comment):

```c
		for (goal_index = 0; goal_index < 32; goal_index++)
		{
			struct netgame_goal *goal = &global_goal[goal_index];

			if (goal_matches_player(goal_index, player, player_index) && count < maximum_count)
			{
				goal_indices[count] = goal_index;
				positions[count].x = goal->position.x;
				positions[count].y = goal->position.y;
				count++;
			}
		}
```

Evidence **for**:

- It is plain C, with no cast.
- The same alias idiom is first-party in this TU. /Od goal_matches_player (0x5b23c0) keeps a `goal` pointer slot
  `[ebp-8] = &global_goal[i]` (`shl ecx,5; add ecx,0xc569d8; mov [ebp-8],ecx`).

Evidence **against**:

- /Od 0x5a20e0 has no goal pointer slot: its frame is exactly 0xc = count, player and goal_index. It also copies the
  point as one aggregate, not as two float components.
- The alias is load-bearing. Lab strip at cdc8ebd3: without it, the function is RESIDUAL. January's strength-reduced
  induction-variable base is `&global_goal[0].position.x`; the plain spelling picks `.y` (+4, 4 instructions).
- House-rule class: a non-/Od-attested, load-bearing named local (brief s.7, owner-gated). Review3 listed this exact
  construct as "still blocking OBJECT_COMPLETE".

### Option B: F07 view copy (patches/optionB/)

What lands, verbatim:

```c
			if (goal_matches_player(goal_index, player, player_index) && count < maximum_count)
			{
				goal_indices[count] = goal_index;
				/* /Od 0x5a20e0: one 8-byte copy of the goal's x,y (the 3D point viewed as its 2D prefix) */
				positions[count] = *(real_point2d *)&global_goal[goal_index].position;
				count++;
			}
```

Evidence **for**:

- **Per-site /Od attestation** (the quote above): one aggregate copy, index computed once, integer movs.
- **Compatible prefix layout.** real_point2d {x, y} is the prefix of real_point3d {x, y, z}. HCEX's real_point3d has
  no 2D member, so an 8-byte aggregate copy needs a view.
- **Strictly exact caller.** The function is 180/180 EXACT.
- **Bungie idiom.** January's own assert strings stringify view casts:
  - actors `(real_vector2d *) &actor->input.facing_vector`;
  - actor_looking `(real_vector2d *) &actor->output.facing_vector`;
  - actor_moving `(real_vector2d *) movement_direction`;
  - observer `(real_point3d *) &observer->focus_offset`.
- **Ledger disclosure:** this file and LEDGER.md S4.

Evidence **against**:

- Your 2026-09-21 rule-24 narrow admission also requires a **byte-inert** cast. This one is load-bearing: stripping
  it gives the same RESIDUAL as option A's strip. The cast-free aggregate spelling does not compile.
- Ruling 2 of 2026-09-21: "Rule 24 stays strict for NEW representation-punning casts."

## Question 2 (only if Q1 is yes): the focused headers and your 2026-09-21 ruling "do not invent a one-consumer header"

> "Does the no-one-consumer-header ruling apply to F02's four new headers? If yes, land the
> variant_no_single_consumer_headers packet. That puts the three single-consumer layouts back in game_engine.c, and
> the result is byte-identical everywhere. Do you then accept hud_nav_points.h, a new header with one non-owner
> consumer? The only genuine alternative is to move the nav-point trio into hud.h, and that breaks
> `_populate_statistic_buffer` through a declared-name-count tie."

What the ruling affects:

- **hud_globals_definitions.h, item_collection_definitions.h, scenario_netgame_definitions.h:** each has exactly one
  consumer, game_engine.c. None is in the tree or in HCEX's header list. Their own comments say they exist to avoid
  count ties in the broad genuine headers. **Directly affected.**
- **hud_nav_points.h:** two consumers, game_engine.c and its owner hud_nav_points.c (via F04). It is invented too:
  HCEX's hud.h holds the nav-point API.

Measured alternatives (MEASURED.md s.5):

- The genuine broad headers fail at cdc8ebd3:
  - item_definitions.h moves hud_weapon's `_render_weapon_hud`;
  - scenario_definitions.h regresses ai_communication `_ai_communication_update_speech_timers` and makes the ai park
    stale;
  - hud.h flips game_engine's own populate;
  - hudg cannot move without rewriting 7 HUD TUs.
- **TU-local (the variant):** 180/180. Identical to option A in every one of the 447 TUs, with the same audit, pdb,
  surplus and link results.

Comments that land in game_engine.c with the variant, verbatim:

```c
/* scenario_starting_equipment.flags (HCEX names) */
/* HUD globals ('hudg') tag layout, as recorded by the HCEX PDB */
/* item collection ('itmc') tag layout, as recorded by the HCEX PDB */
/* scenario multiplayer blocks, as recorded by the HCEX PDB */
```

- The variant also carries `extern struct hud_globals_definition *hud_globals;` in game_engine.c. This is a
  consumer-local extern of hud.c's global, the same form hud_messaging.c, hud_nav_points.c and hud_unit.c use today.
  hud_unit was admitted with it in batch 5a.
- With F02 as is, that extern lives in hud_globals_definitions.h instead.

## Question 3 (disclosure; YES to Q1 admits it as is): the pre-existing `debug_player_color` aggregate

> "Admitting game_engine also admits the TU's anonymous aggregate
> `struct { short value; word pad; short teleporter_flash_type; word teleporter_flash_pad; real
> teleporter_flash_maximum_intensity; real_argb_color teleporter_flash_color; real teleporter_flash_duration; }
> debug_player_color`, which owns January's 32-byte .data. It also admits the descriptive .bss names
> `_game_engine_teleport_message_ticks` and `_game_engine_teleport_flash_fade_function`. Do you accept them, or is
> this the held invented-aggregate / descriptive-static class (as for rasterizer_xbox_profile and render_debug)?"

- It was landed long ago: GE09 in docs/object_matching_logs/game_engine_obj.md. Neither review3 nor review4
  examined it.
- `word pad` members in a TU-private struct are the law (j) signature of an invented aggregate.
- **HCEX.pdb** (DIA2Dump -sym game_engine_update_teleporter, hcex_update_teleporter.txt): the flash values are
  FUNCTION-STATIC LOCALS of game_engine_update_teleporter:
  - `float red`, `green`, `blue`, `duration`, initialised in .data;
  - `int blocked_message_delay` and `long fade_function` in .bss. These correspond to the two descriptive .bss names.
- The 2020 /Od exe (data only):
  - debug_player_color is at 0xa1dfd0;
  - unrelated globals follow (up to 0xa1dfe8, "game_engine_update");
  - the flash values sit at 0xa1dfec..0xa1dffc, each referenced only by update_teleporter 0x5afba0.
- **No first-party source supports one aggregate spanning both.**
- No alternative form was tried in this wave. Static locals would need January's static-local symbol spelling in
  symbols.json, which is its own owner question.

## House-rule classes touched by the packet (all disclosed; review4 verified)

- **Shared-header edits, count-neutral.** game_engine.h gets renames only, plus the two game_engine_multiplayer_sounds
  prototypes (rule 15). player_control.h, hud_messaging.h and sound_classes.h get genuine owner prototypes (rule 9).
  The sweep has 0 regressions.
- **New headers.** F02 (question 2).
- **TU-local enum values in game_engine.c.** The flag bits 3/4/5 are TU-local, because game_engine.h's name count is
  fragile.
- **Enum names that are not HCEX's.**
  - The `_postgame_statistic_*` constant names are descriptive: HCEX records only the tag name.
  - Weapon set 10 is named `_game_engine_weapons_no_grenades`. That is January's behaviour; HCEX 13 = no_grenades,
    10 = covenant.
- **A dead local, /Od-attested.** `local_player_count` in select_players_to_display is dead in January. Nit, byte-inert
  to rename: its descriptive name shadows the public function `local_player_count()`.
- **Pre-existing, owner-landed BUG comment kept verbatim:** "BUG (preserved for exact matching): January passes team
  index zero for both CTF checks ..." in netgame_verify_spawn_points.
- **Comment kept:** "January visits the first five of the six item collections" in
  handle_custom_starting_equipment.
- **Three own-COMMON `extern` data declarations** remain: game_engine_globals, global_stage and
  timeout_for_endgame_sound.
- **Zero-credit follow-ups, not blockers.** The review4 advisories:
  - scalar bare blocks not audited against /Od;
  - goal_radar 0/1/2 literals;
  - the `parameter0`/`parameter1` names in game_engine_playlist_next;
  - the mix of `if (game_engine)` and `game_engine_running()`.
