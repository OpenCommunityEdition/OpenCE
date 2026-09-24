# game_engine_finish LEDGER (tree 16542e46; task: finish whole-object admission of source/game/game_engine)

## F0 setup / rebase
- Tree clean at 16542e46. review3 R01/R03/R05c patches no longer apply (f7cd2e72 renamed game_variant members,
  game_engine.h struct rewritten). R02/R04/R05h/R06 still apply.
- Rebase method: `git merge-file -p HEAD 954eebd2 review3-final` (LF-normalised copies in base954/ head/ theirs/ merged/).
  2 conflicts only:
  (a) game_engine.c render_nav_points condition: head `universal_variant.goal_radar == 1 && (short)local_player_index != NONE`
      vs R01 `unknown24 == 1 && local_player_index != NONE` (R01 made the parameter `short`) -> resolved to
      `global_variant.universal_variant.goal_radar == 1 && local_player_index != NONE`.
  (b) game_engine.h struct universal_variant/game_variant: R03 did not touch it (base struct) -> took HEAD's recovered layout.
      R03's other hunks (float->real, short local_player_index, 2 multiplayer-sound prototypes) merged cleanly.
- rebased/ = LF files; shadow/ = full copy of working-tree source/ with rebased files installed (CRLF).
- rcompile.py (copy of reviewer harness, production cflags, cwd = shadow):
  control real tree: 180/180 EXACT, 0 changed vs base. REBASED shadow: 180/180 EXACT, 0 of 302 changed vs build/base.

## F1 open item (1) game_engine_man_out -> CLOSED (EXACT, /Od-attested)
- Hypothesis: January's man_out has NO int materialisation on the false path (`test al,al; jne T; pop edi; pop ebp; ret`,
  T: `mov al,1`), i.e. a BOOLEAN-typed value reaches the return; reviewer's mo_v1/v2 (`return a||b||c;`) give
  `xor eax,eax / mov eax,1` (int-typed || result), mo_v3 (`result=FALSE; if(chain) result=TRUE;`) adds a callee-saved bl.
  /Od 0x5a0b90: player slot [ebp-4], a NAMED byte local [ebp-5] assigned from the ||-temp [ebp-6], then `mov al,[ebp-5]`
  => `boolean man_out = player->quit_out_of_game || is_out_of_lives(player_index) || is_odd_man_out(player_index); return man_out;`
  (the one spelling the reviewer did not try: a declaration-initialised named boolean).
- mo_a1 (exactly that; is_out_of_lives stays the ordinary public call, VC7 inlines it as January did):
  180/180 EXACT, 0 of 302 changed vs build/base.
- /Od cross-check: our VC7 at /Od /RTC1 on mo_a1 gives the same topology (player slot, named byte local, ||-temp,
  3 real calls, one return); only the temp width differs (VC7 dword temp vs the 2020 compiler's byte temp) = compiler version.
- Storage: _game_engine_player_is_out_of_lives and _game_engine_man_out are cachebeta PUBLICS (external, used by race/slayer);
  _game_engine_player_is_odd_man_out absent = static (already static). No storage change.

## F2 open item (2) motion-sensor copy (evidence; no new EXACT natural form yet)
- HCEX: `struct netgame_goal { real_point3d position; uchar in_use; long player_index@0x10; short team_index@0x14;
  long ignore_player_index@0x18; short nav_index@0x1C }` (global `global_goal[0x20]`, external), real_point3d has NO 2d member
  (x,y,z / u,v,w / n[3]). HCEX params: (long player_index, real_point2d *points, char *goal_indices, short max_points).
- /Od 0x5a20e0: frame 0xc = count(short,[ebp-4]) + player([ebp-8]) + goal_index(long,[ebp-0xc]); NO goal pointer slot;
  ONE statement copies 8 bytes with integer movs (index recomputed once: `shl ecx,5; mov edx,[ecx+x]; mov eax,[ecx+y];
  mov [esi+ecx*8],edx; mov [esi+ecx*8+4],eax`) => `points[count] = *(real_point2d *)&global_goal[goal_index].position;`
  (a /Od component copy would be two statements, each re-indexing). The 3D->2D view is a pervasive Bungie idiom (January
  assert strings in actors.c/actor_looking.c stringify `(real_vector2d *) &...`).
- January O2 differs between spellings only in the strength-reduced IV base: ms_n1 (direct component copies) picks
  `&global_goal[i].position.y` (+4) as IV (`mov esi,4`, `cmp esi,0x404`); January/ms_n2(alias)/ms_n3(view copy) use +0.
- Status: ms_n2 (in-loop `struct game_engine_goal *goal = &global_goal[goal_index];`, current rebased packet) EXACT but the
  alias is an extra named local absent from the /Od frame; ms_n3 (/Od view copy) EXACT but a LOAD-BEARING view copy (strip
  -> ms_n1 residual) = owner-gated class (brief s.2/s.7). Both are compliant with every other rule.

## F3 name-count sensitivity oracle (lab only, never lands)
- nc_K: K dummy enum constants inserted after the TU MULTIPLAYER_MAXIMUM_PLAYERS enum (top of TU), K = 1,2,4,8,13,16,24,32,40:
  all 180/180 EXACT, 0 changed vs base => game_engine.c has no name-count-fragile function for +1..+40 TU-top names.
  (So TU-local enum definitions for rule 16 are low-risk; still re-measured on every candidate.)

## F4 open item (4) populate_statistic_buffer walks -> CLOSED (EXACT, /Od-attested)
- /Od 0x5b5e60 (RTC: player_iterator 16B): locals in order player_count(=0 init), invert byte = `statistic==4 ? !inverse : inverse`
  (int ternary temps), player_iterator, player (loop-1 `while (player = data_iterator_next(&it))`), loop index,
  player_index, player (loop 2), loop index 3; loop 1 body = `match_assert(player_count < MAX); if (player_count < MAX)
  { buf[player_count].player_index = it.datum_index; player_count++; }`; loops 2/3 INDEXED (imul 0x1c every access), no
  entry pointer; last loop calls sort_statistic_buffer_ranking(&buf[i-1], &buf[i]) == 0.
- Assert-anchor drift: January asserts 0x2C8 -> 0x2FD (53 lines), /Od 0x48a -> 0x4bf (53 lines): same span length =>
  the /Od source for this span is January's.
- HCEX: populate_statistic_buffer(struct statistic_buffer *statistic_buffer, enum postgame_statistic statistic,
  unsigned char inverse) local player_iterator.
- psb_a1 = exactly that shape (params statistic_buffer/statistic/boolean inverse; indexed loops with one index local
  `statistic_index` reused for loops 2 and 3; the tied bit stays `|= 0x80000000`): 180/180 EXACT, 0 changed vs base.
  Replaces: `long invert_parameter` + `(boolean)` casts, if/else assert shape, do/while `entry++` walks, `previous` alias.

## F5 more statistic-walk items (all EXACT, /Od-attested)
- Assert-anchor drift proves the /Od span populate..get_player_place..generate_title_string is January's text length:
  January 712/765 -> 866 -> 878; /Od 1162/1215 -> 1316 -> 1328 (constant +450 offset).
- gpp_a1 game_engine_get_player_place = /Od 0x59d9f0: `struct statistic_buffer statistic_buffer[MULTIPLAYER_MAXIMUM_PLAYERS];
  long place = 0; populate_statistic_buffer(statistic_buffer, 0, FALSE); while (TRUE) { if (buf[place].player_index ==
  player_index) break; place++; match_assert(place<MULTIPLAYER_MAXIMUM_PLAYERS); } return buf[place];` (/Od `mov edx,1;
  test edx,edx` = constant-true loop) -> EXACT (gpp_a2 plain while-condition also EXACT; a1 chosen = /Od form).
  Replaces the hand-rotated `if (x) do {...} while (x)`.
- sp_a1 select_players_to_display = /Od 0x5b6920 names (RTC statistic_buffer 448B, outside_range 112B) + the /Od
  local-player COUNT in loop 1 (`if (player->local_player_index != NONE) local_player_count++;`, dead at /O2: January's
  loop body is exactly the surviving player_get call) -> EXACT, byte-inert (replaces the bare discarded
  `player_get(...)` statement). Disclosure: local_player_count is dead in January (/Od-attested, not a filler).
  sp_a2 (the 2020 memmove size `(max - k - 1) * sizeof`) RESIDUAL -> January keeps `max*sizeof - (k+1)*sizeof`.
- gr_a1 postgame_statistic_get_rating with HCEX `boolean inverse` RESIDUAL: January forwards the 3rd param with
  `mov eax,dword ptr [ebp+0x10]` (32-bit param) while populate reads `mov al,byte ptr [ebp+0x10]` (byte param).
  gr_a3 get_rating(long player_index, long statistic, long inverse) + populate(..., boolean inverse) + /Od locals
  (statistic_buffer[MAX], `player_count = 0` init then call, rating = 0 after the call) -> EXACT.
- c1 = rebased + mo_a1 + psb_a1 + gpp_a1 + sp_a1 + gr_a3 (+TU prototypes renamed): 180/180 EXACT, 0 changed vs base.

## F6 NEW blocker found + fixed: game_engine_predict_resources hand-inlined list_index_to_weapon_definition_index (rule 13)
- Source had 14 hand copies of `game_globals = scenario_get_game_globals(); weapon = TAG_BLOCK_GET_ELEMENT(&game_globals->
  weapon_list, N, struct tag_reference); ... weapon->index` (grenades 12/13, ball 10, flag 11, weapon_indices[0..9]).
- /Od 0x5a5fc0 (2020 revision, more vehicle sets + 16-entry initialiser) calls list_index_to_weapon_definition_index
  (0x5b4150, same callee as /Od remap_equipment's final call) for every one; January emits list_index_to_... (#65) before
  predict_resources (#118), so VC7 inlines the calls (NONE guard folds).
- pr_a1 (calls restored, `struct tag_reference *weapon` local dropped) EXACT; pr_a2 (+ `for (weapon_index = 0;
  weapon_index < 10; weapon_index++)` replacing the hand while-loop) EXACT; pr_a3 (NUMBEROF bound, unsigned) RESIDUAL.
- Side finding (zero-credit, optional): COMMON pool records game_engine_globals/timeout_for_endgame_sound/global_stage
  (0x592540..) sit between the players(288) cluster and game.c(309)'s game_variant_global; game_engine.c (module 307) is
  the only TU using game_engine_globals/global_stage, but ours only has `extern` declarations and no TU defines them.
  Not an audit blocker (split shows them UNDEF); not changed here.

## F7 open item (3) rule-16 switch tables -> CLOSED (byte-inert, all EXACT)
- lab/r16.py -> lab/r16.json (48 count-checked edits) on top of c2; c3 = c2 + r16: 180/180 EXACT, 0 changed vs base.
- TU-local enums added in the constants block (after MULTIPLAYER_MAXIMUM_PLAYERS; F3 showed no name-count fragility):
  * `enum game_engine_mode` {game_engine_mode_active, _postgame_delay, _postgame_rasterize_delay, _postgame_rasterize}
    = HCEX names verbatim; REPLACES the invented mid-file `enum game_engine_postgame_state` (removed); used in the 4
    postgame_state switches and every postgame_state compare/store.
  * `enum postgame_statistic` (HCEX tag name; HCEX PDB has no constants) {_postgame_statistic_ranking (0: fills the
    sort_statistic_buffer_ranking fields; January public name), _score, _kills, _assists, _deaths,
    NUMBER_OF_POSTGAME_STATISTICS}: DESCRIPTIVE constant names (disclosed).
  * `enum game_engine_weapons` (HCEX names) 0..9 = HCEX 0..9 (behaviour-checked against remap_weapon: pistols ->
    pistol/plasma pistol, rifles -> AR/plasma rifle, plasma, sniping, no sniping, rockets, shotguns, short range, human).
    VALUE 10 = `_game_engine_weapons_no_grenades`, NOT HCEX's `_covenant`: January's set 10 (a) makes remap_equipment
    return NONE for grenades, (b) zeroes starting grenades unless infinite, (c) has NO remap_weapon case; /Od 2020 gives
    exactly (a)+(b) to set 13 `_no_grenades` (jump tables decoded: remap_equipment 3,10->plasma 9->frag 13->NONE;
    starting grenades 3,10->plasma 9->frag 13->zero) and adds a covenant remap case => the enum was reordered after
    January. NUMBER_OF_GAME_ENGINE_WEAPON_SETS = 11 matches January's `PIN(weapon_set, 0, 10)`.
  * `enum game_engine_vehicles` (HCEX names) 0..4 {default, none, warthog, ghost, tank} (remap_vehicle: 1 -> NONE,
    2/3/4 -> vehicles block 0/1/2), NUMBER = 5 matches January's `PIN(vehicle_set, 0, 4)`.
  * unnamed `_weapon_list_*` enum (HCEX names) 0..13 (12/13 grenades attested by remap_equipment, 10/11 ball/flag by
    remap_weapon + predict_resources' oddball/ctf branches); HCEX's 14/15 + count omitted (no January evidence).
- Converted: statistic switch + callers (select_players_to_display, get_player_place, 4x get_rating), weapon_set switches
  (remap_equipment, remap_weapon, starting-grenade switch), weapon_list switches/compares, vehicle_set switches
  (remap_vehicle, predict_resources, `!= none`), variant_cleanup PINs + game_engine_index switch (existing
  game_engine_ctf/game_engine_slayer), predict_resources oddball/ctf compares, both internal_rasterize_score switches
  (existing TU `_game_engine_message_*`), and the 52 builder `weapon_set/vehicle_set = N` stores.
- Left as literals (not enums): `switch (remaining_lives)` 0/1 (a count), did_player_win NONE/FALSE/TRUE (January's
  tied value is NONE, not HCEX game_win_type's 0xFF).

## F8 goal-struct naming (rule 21) + flag/constant pass (rule 18) -> all EXACT
- HCEX `struct netgame_goal {position; in_use; long player_index@0x10; short team_index@0x14; long ignore_player_index@0x18;
  short nav_index@0x1C}`; our `game_engine_goal` had 0x10 `target_object_index` (compared with player_index in
  goal_matches_player = semantically a player index) and 0x18 `player_index` (the `player_index != x` exclusion = HCEX
  ignore_player_index). lab/goal.py: tag -> netgame_goal, fields renamed per HCEX, explicit pad members dropped
  (natural alignment = same 0x20 layout, verify typedef kept), set_goal_position params (player_index, team_index,
  ignore_player_index) in definition + game_engine.h prototype. c4 = c3 + goal: 180/180, 0 changed.
- NEW rule-13 find: game_engine_infinite_grenades hand-expanded game_engine_infinite_grenades_internal (the
  `!TEST_FLAG(globals.flags,2) && TEST_FLAG(variant.flags, infinite)` body). /Od 0x5a08d0: `result = FALSE; if
  (game_engine_running() && player_index != NONE) result = internal();` (calls 0x5a0920 = internal). ig_a1 (keep
  `game_engine &&`) and ig_a2 (/Od form) both EXACT; ig_a2 adopted.
- Builders: `flags = (flags & ~M) | V` is hand bit twiddling (rule 18). /Od 2020 builders spell one SET_FLAG per bit
  (constant-condition ternaries). bf_asc / bf_desc (one SET_FLAG per bit in M|V, ascending/descending): both EXACT
  (VC7 folds them into January's single `and ecx,~M; or ecx,V`). Adopted ascending order with named bits.
- Flag names corrected per HCEX semantics, all consistent with January code:
  game_engine_globals.flags: bit0 was `_game_engine_allow_dynamic_lighting_bit` but SET when many players/vehicles and
  allow_dynamic_lighting returns !TEST_FLAG -> HCEX `disable` semantics => `_game_engine_disable_dynamic_lighting_bit`;
  bit1 likewise `_disable_integrated_lights_bit`; bit2 was `_game_engine_disable_infinite_grenades_bit` but set iff
  player_count >= 5 (build_lighting) => HCEX `_game_engine_5_or_more_players_bit`; + bit3 `_9_or_more_players_bit`
  (set iff >= 9; remap_equipment 0.3 vs 0.55 spawn thinning), bit4 `_game_over_sound_disabled_bit` (set with the
  postgame sound-class fade).
  universal_variant.flags: bit1 placeholder `_game_variant_unknown1_bit` -> `_game_variant_allow_friendly_navpoints_bit`
  (display_team_indicators; HCEX _allow_friendly_navpoints_bit); + bit5 `_game_variant_generic_starting_equipment_bit`
  (gates handle_custom_starting_equipment; HCEX _generic_starting_equipment_bit).
  These live in game_engine.h (header patch; renames + 3 added enum constants; only game_engine.c references them).
- Also: remap_equipment powerup_type 2/3 -> existing `_equipment_powerup_overshield/_active_camouflage`; get_place_name
  `(place.flags & FLAG(2))` + bare bits -> TEST_FLAG with the existing HCEX `_place_*` constants (game_engine_place.h);
  game_engine_index 1/2/5 compares and the 26 builder stores -> existing game_engine_type constants.
- c5 = c4 + fl (17 edits) with the header in the shadow: 180/180 EXACT, 0 changed vs base.
- fl2: starting-equipment flags bits 0/1 -> HCEX `_netgame_starting_equipment_flag_no_grenades_bit` /
  `_netgame_starting_equipment_flag_plasma_greandes_bit` (HCEX spelling, marked sic) added to the NEW focused header
  scenario_netgame_definitions.h (R02; game_engine.c is its only includer); postspawn TEST_FLAG(globals.flags, 3/2) ->
  9_or_more/5_or_more names. c6 = c5 + fl2: 180/180 EXACT, 0 changed vs base.

## F9 did_player_win_default gotos -> single exit (EXACT, /Od-attested); team_has_players else
- /Od 0x59bb90: `result` [ebp-8] = 0 at entry; tied/winner/loser are DIRECT stores in if / else-if / else arms
  (`mov [ebp-8],-1 | 1 | 0`, no int temp) => `if (w == NONE) result = NONE; else if (player->team_index == w)
  result = TRUE; else result = FALSE;` and in the place arm `if (tied && !pos) result = NONE; else if (!pos) result =
  TRUE; else result = FALSE;`, one return. The reviewer's single-exit dpw_v2 used `result = a == b` expressions
  (int setcc temps) -> residual, hence its kept `goto tied`.
- dpw_s1 (/Od form, `long result = 0;`) EXACT; dpw_s2 (no `= 0`) EXACT (strip-inert init); dpw_s3/s4 (`result = a == b`)
  RESIDUAL. Adopted s1: both `goto tied` + label gone.
- team_has_players: /Od 0x5b6f70 has `mov [ebp-1],1; jmp END; jmp END` = a jump statement in the then-block FOLLOWED by
  the if/else skip jump => `if (...) { result = TRUE; goto done; } else { ... }`. th_a1 (bare block -> else) EXACT.
  goto kept (reviewer: if/else without the goto is residual).
- c7 = c6 + dpw_s1 + th_a1: 180/180 EXACT, 0 changed vs base.

## F10 internal_rasterize_score local macro + HCEX message parameter names (EXACT)
- The function-local `#define GET_GAME_ENGINE_HUD_FORMAT(index) ((string_list_index = tag_loaded(..)) != NONE ?
  get_string(..) : L"")` (27 uses) is an invented macro. /Od 2020 (0x5b29xx..) spells each case as statements:
  `string_list_index = tag_loaded('ustr', ..); if (!= NONE) format = unicode_string_list_get_string(.., 0x4a); else
  format = L"";` into a named local ([ebp-0x14] string_list_index, [ebp-0xc] format), then usnprintf/ustrncpy.
  irs_a1 (macro deleted; `wchar_t const *format;` local; 17 usnprintf + 10 ustrncpy sites rewritten) EXACT.
- HCEX multiplayer_message_internal(player_index, message, message_data, buffer, buffer_size) (same January ABI as
  internal_rasterize_score); multiplayer_message(..., message, message_data) (HCEX adds later params; January has 3).
  msg: parameter1/parameter2/message/message_character_count -> message/message_data/buffer/buffer_size in
  internal_rasterize_score (+ prototype), multiplayer_message_internal, multiplayer_message (local buffer renamed
  buffer), game_show_score_one_player. c9 = c8 + msg: 180/180, 0 changed. (First attempt without renaming
  multiplayer_message's local `message` array collided -> 7 residual; fixed.)
- upd: HCEX `struct game_engine` = January's with `player_removed` inserted at 0x18 (January members from 0x18 on sit
  at HCEX-4: post_rasterize_objects=post_rasterize, player_update_each_tick=player_update, objective_weapon_update=
  weapon_update, picking_up=weapon_pickup, weapon_dropped=weapon_drop, get_player_score=get_score) => placeholder
  members `unknown2C` -> `pregame_post_rasterize` (HCEX 0x30), `unknown44` -> `update` (HCEX 0x48; called from
  game_engine_update). Header member renames (count-neutral; only game_engine.c names them) + format_message
  callback params -> HCEX message/message_data/buffer/buffer_size. c10: 180/180, 0 changed.
- st_a1: statistic params typed HCEX `enum postgame_statistic` (populate, get_rating, select_players_to_display + prototypes). c11: 180/180, 0 changed.

## F11 shared-header count fragility (declaration-count oracle) -> count-neutral header
- First full sweep of the packet (game_engine.h with +3 enum constants: 9_or_more, game_over_sound, generic_starting
  bits): units `_unit_preprocess_node_orientations` and network_client_message_handler
  `_network_game_client_handle_message` CHANGED (the known units canary). V2 (globals-flag enum moved into the TU, +1
  variant bit in header = net -2 names) still breaks units. V1 (header renames ONLY, count-neutral vs the reviewer's R03)
  = clean. Adopted V1: game_engine.h keeps its 3+5 flag constants (renamed, same count); the three extra bits live in a
  TU enum in game_engine.c with explicit values 3/4 and 5 (only game_engine.c uses them). No count compensation.
- Control sweep (pristine HEAD archive): 446/446 SAME vs build/base. Rebased R01..R04 alone: 445 SAME + game_engine raw
  reloc spelling.

## F12 more rule-13 hand-inlines found by /Od call sets (all EXACT)
- list_index_to_weapon_definition_index: /Od 0x5b4150 calls game_globals_get_weapon (0x5b1170, 2 args) after
  scenario_get_game_globals(); ours hand-expanded its TAG_BLOCK_GET_ELEMENT + ->index. li_a1 (call) EXACT; li_a2 (+ /Od
  0x5b1170's named result local in game_globals_get_weapon) EXACT -> adopted.
- weapon_definition_index_to_list_index: /Od 0x5b7640 = `weapons = count ? element(0) : NULL` ternary, INDEXED
  `weapons[i].index` into an in-loop local, no pointer walk (ours: `weapon++` walk). wd_a1 (/Od form) EXACT; wd_a2
  (without the in-loop local) EXACT; adopted wd_a1.
- linear_to_non_linear_alpha (/Od 0x5b4100 = pow(x, 1.9f)) is CALLED by /Od post_rasterize_in_game (0x5a4360) and
  internal_rasterize_target_name (0x5b3d10); ours hand-expanded `(real)pow((double)x, 1.9f)` at both sites. ltn_a1b
  (`real alpha = linear_to_non_linear_alpha(fade);`) and ltn_a2 (`alpha = linear_to_non_linear_alpha(hold_time * 0.1f)
  * 0.5f;`) EXACT. (ltn_a1 without the alpha local also EXACT.)
- c13 = final + li_a2 + wd_a1 + ltn_a1b + ltn_a2: 180/180, 0 changed.

## F13 game_engine_update: invented iterator union + hand-inlined per-player helpers (EXACT after fix)
- /Od functions are laid out ALPHABETICALLY per TU (anchors: find_closest 0x59aae0 ... weapon_definition_index_to_list_
  index 0x5b7640) -> odmap.py maps equal-count alphabetical gaps 1:1 (61 of 233) and diffs intra-TU call edges.
- /Od game_engine_update (0x5aea50; RTC player_iterator 16B) loop body = CALLS update_player_no_shield, update_player_
  always_invis (0x5af860), update_teleporter, then the player_update_each_tick callback. Ours: `union
  game_engine_update_iterator {data_iterator data; object_iterator object;}` (invented aggregate to share the frame
  slot), a hand-inlined + restructured copy of update_player_always_invis (extra `always_invisible` local, if/else-if),
  a `{ long player_index = ...; }` block, a `void (*player_update)(long)` staging temp, and an early `return`.
  January frame 0x14 = one 16-byte iterator + the inlined no_shield byte local.
- /Od update_player_always_invis calls game_engine_test_trait (0x5ae9c0) - ours hand-expanded `game_engine->test_trait
  && game_engine->test_trait(player_index, 1)`.
- gu_a1 (natural update: `if (game_engine) {...}` single exit, three DISJOINT block-scoped iterators (player loop /
  postgame player loop / object_iterator) so VC7 shares the slot, helper calls) alone RESIDUAL; ai_a1 (always_invis
  -> game_engine_test_trait(player_index, 1)) EXACT alone; gu_a2 = both: EXACT. The union type is removed.
- c14 = c13 + gu_a2 - union: 180/180, 0 changed.

## F14 more helper hand-inlines via /Od call edges (all EXACT)
- game_engine_has_teams (/Od 0x5a06a0): /Od generate_title_string (0x59c350) calls it twice (`boolean has_teams =
  game_engine_has_teams();` right after did_player_win, and `else if (game_engine_has_teams())`), /Od rasterize_in_game_
  score (0x5abbb0) initialises `has_teams = game_engine_has_teams()`. Ours hand-expanded `has_teams = FALSE; if
  (game_engine) has_teams = global_variant...teams;` (x2) and `game_engine && ...teams`. tt_a1/a2/a3 EXACT -> tt_a2.
  (get_distance_rating_for_spawn's `game_engine ? teams : FALSE` is /Od-consistent: /Od 0x59cff0 writes the ternary with
  running(), no has_teams call -> kept.)
- get_place_string (/Od 0x5b2000; `PIN(place & 0x7F, 0, 15)` nested ternary, +0x24) is CALLED by /Od post_rasterize_post_
  game (0x5a4870) and rasterize_in_game_score; ours hand-expanded it at both sites (PIN + tag_loaded + ?:/if-else).
  gps2_a1/a2/a3 EXACT; adopted a3 + the two now-dead locals (`place_index`, `place`, `string_list_index`) removed (c17).
- Corrected /Od identification: 0x5b20e0 = get_postgame_hilite_colors (calls 0x5b21a0 = get_selected_color) - the
  alphabetical 1:1 mapping in that gap was off by one (2020 extra function).
- c17: 180/180, 0 changed.
- internal_rasterize_score: /Od 0x5b2940 begins `result = TRUE; if (game_engine_test_flag(1))` (calls 0x5ae940); ours
  hand-expanded `game_engine && game_engine->test_flag && game_engine->test_flag(1)`. ts_a1 EXACT (c18).
- Edge census (odmap.py with 92 identified /Od functions): every remaining /Od helper edge into a January-shaped
  function is now a real call in ours, except (a) `game_engine_running()` vs `if (game_engine)` (the 2020 build mixes
  both spellings per site; not changed except where adopted above) and (b) 2020-only logic (goal_matches_player /
  render_nav_points / netgame_verify_equipment get_type calls: different 2020 code, January bytes differ).

## F15 FINAL verification (final/ = c18; patches F01..F04 regenerated)
- `git apply --check` F01, F02, F03a, F03b, F04 individually and together vs the tree (16542e46): clean; tree status clean.
  F01 needs F02+F03a+F03b to compile (new headers, renamed constants/members, prototypes) - land together.
- Independent shadow_F = `git archive HEAD source` + F01..F04 (git apply, GIT_CEILING_DIRECTORIES) == final/ byte-for-byte
  (CRLF-normalised).
- rcompile (production cflags, cwd shadow_F): 180/180 EXACT vs build/split; 0 of 302 section-defining symbols changed vs
  build/base; symbol-table multiset identical to production (853/853, $L collapsed).
- object_audit PASS (text identical to production's audit); provider_link SELECTED-PROVIDER LINK PASS; surplus 11 COMDATs
  identical (unchanged set); pdb_storage 0 disagreements (no storage changes); objdiff-cli 3.3.1 mini-report: production
  and final both 27922/32397 code, 173/180 fns (7 pre-existing undercredited), 3792/3792 data, 0 per-function deltas.
- Full sweep (rsweep, 446 TUs, vs build/base): 445 SAME; game_engine differs only in (a) section ORDER -
  _game_engine_update_player_always_invis is now emitted later (C2 defers it: it now calls game_engine_test_trait, which
  our file defines later; production's order already differs from January's split order in many places) and (b) the
  known raw reloc spelling `_global_goal+0x400` for `_global_variant+0` in motion-sensor/render_nav_points (resolved
  target identical; coff_compare EXACT). Control sweep of pristine HEAD: 446 SAME.
- Optional F05h+F05c+F06+F07 on top: 180/180; sweep 445 SAME + game_engine (same order/reloc notes).
- /W3: game_engine.c only: C4013 console_printf gone; +1 C4244 'function' long->boolean (get_rating's 32-bit `inverse`
  forwarded to populate's byte `inverse` = January's ABI, proven by the dword vs byte reads). No other TU changes.
- Consumer census: CONSUMERS.txt (game_engine.h 37 TUs incl. `saved games/playlist_profile`; hud_messaging.h 8 incl.
  `saved games/game_state`; sound_classes.h 6; player_control.h 21; new headers only game_engine (+hud_nav_points)).

## F16 VERDICT / owner items / reopen criteria
- Open list (review3 s.4): (1) man_out CLOSED (mo_a1, /Od form); (3) rule-16 switches CLOSED (r16 + fl/fl2); (4)
  statistic walks CLOSED (psb_a1, gpp_a1, sp_a1, gr_a3); (2) motion-sensor copy: both forms EXACT - default keeps the
  reviewer's in-loop `goal` alias (not in the /Od frame); F07 (optional) = the /Od-attested one-statement 8-byte view
  copy `positions[count] = *(real_point2d *)&global_goal[goal_index].position;` (load-bearing: component copies pick the
  +4 IV) -> OWNER picks.
- Additional blockers found and fixed (not in the review): invented `union game_engine_update_iterator` + hand-inlined
  update_player_always_invis/test_trait/test_flag/has_teams/get_place_string/linear_to_non_linear_alpha/
  list_index_to_weapon_definition_index(x14)/game_globals_get_weapon/infinite_grenades_internal, GET_GAME_ENGINE_HUD_FORMAT
  local macro, hand bit-twiddled builder flags, did_player_win_default gotos, weapon list pointer walk, semantically
  wrong netgame_goal field names and flag-bit names, placeholder `unknown2C/unknown44/parameter1/parameter2` names.
- Section-8(g) scan of final: no caller-local views with opaque spans (TU structs are HCEX netgame_goal/statistic_buffer
  + TU-private globals), no consumer-local prototypes (all 47 TU prototypes are TU functions), no hand-expanded header
  helpers found (teleporter/find_closest fixed by R01; /Od call-edge census for 92 mapped functions clean), no `float`.
- Disclosed (owner/policy): descriptive `_postgame_statistic_*` constants (HCEX has only the tag); weapon set 10 named
  `no_grenades` (January behaviour) not HCEX `covenant`; TU-local continuation bits (3/4/5) because game_engine.h's name
  count is fragile (units/network canaries); /Od-attested dead `local_player_count` (select_players_to_display) and
  strip-inert `= 0` inits (get_rating player_count, did_player_win_default result); authentic C4244 long->boolean.
- NOT done (zero-credit follow-ups): game_engine_globals/global_stage/timeout_for_endgame_sound stay `extern` although
  the COMMON pool places them in game_engine.c's cluster; engine test_flag/test_trait/multiplayer-sound/string-index
  literals (would need new game_engine.h names = count-fragile); `if (game_engine)` vs /Od `game_engine_running()` mix.
- Reopen: a reviewer finds another /Od call edge in the 141 unmapped /Od functions (odmap.py + odmap_extra.json), or the
  owner rules on F07; any game_engine.h edit must keep the declared-name count (sweep units + network_client_message_
  handler first).
