# MENDICANT AI: port specification (ai_brain hook library to OpenCE source)

This is the specification the port was written from, kept as its design record. The files as
built differ from the plan in places:

| Planned | As built (`port/linux/game/mendicant/`) |
| --- | --- |
| `mendicant.c`, `mendicant_training.c`, `mendicant_logs.c` | `mendicant_main.c`, `mendicant_config.c` |
| `mendicant_runtime.c` | `mendicant_models.c`, `mendicant_infer.c` |
| `mendicant_movement.c`, `mendicant_maneuver.c`, `mendicant_positions.c` | `mendicant_move.c` |
| `mendicant_bridge.h` | gone: the bridge is declared in `mendicant_internal.h` |
| `doctrine_table.h` | `mendicant_doctrine.h` |
| the struct-offset static asserts | left out: the code uses the members; only the model bridge's layout is checked (`mendicant_bridge.c`) |

`mendicant.h` declares only the calls the game's sources make; everything the modules share is
in `mendicant_internal.h`.

## Regenerating the doctrine and the models

The doctrine (the steps, what may follow each, the chains) is `tools/mendicant/doctrine.json`.
After changing it:

```sh
python3 tools/mendicant/doctrine_gen.py --table <ai_brain mod>/data/doctrine/doctrine_table.json
ninja linux
```

`doctrine_gen.py` writes `port/linux/game/mendicant/mendicant_doctrine.h` (and, with `--table`,
the `doctrine_table.json` the trainer's `policy.py` reads), and fails if a step has no case in
`mendicant_units.c`; `--check` only checks that the header is up to date. The strategic models
must then be trained for the new doctrine and exported again: the game refuses a models file
made for another doctrine (`mendicant_doctrine_hash`). The models are exported with the mod's
Python:

```sh
<mod>/.venv/bin/python -I tools/mendicant/export_models.py --mod <mod> \
    --out <data root>/mods/mendicant/tags/mendicant/models.aibw
```

## The settings screen

Video Setup > CAMPAIGN AI (`tools/port_settings.py`'s
`video_settings/campaign_ai` and its AIM AND FIRE page, written by
`python3 tools/ce_menus.py --settings-only`) sets the player's
`ai.mendicant.*` settings: on, the trained models, individuals, drill
variety, discipline, threat targeting, unit radius, aim spread, spread by
difficulty, moving targets and the Marines' rate of fire. It is reached from
the main menu's player profile and from a campaign's pause menu (SETTINGS),
and works with a controller as the other settings screens do. OK writes the
changes to config.toml; `menu_functions.c`'s "port settings save" then calls
`mendicant_settings_changed`, which reads them again at once, keeping on, the
models, the seed, the bridge, training and the logs as the map started with
them until the next map's start. The weights, the seed, the bridge, training,
the logs and the tests stay in config.toml only.

## The specification

Fork: OpenCE build-337, commit 899219c.
Source of the port: the ai_brain hook library (`ai_brain.c`, `command.inc`,
`maneuver.inc`, `vehicles.inc`, `actor_go.inc`, `doctrine_table.h`, `brain.py`, `policy.py`),
bound today to OpenCE build 273 by hard-coded addresses.

The inventory below started from `data/distribution_research/research_touchpoints.md` and
`engine_touchpoints.md`. It was re-checked on 2026-11-05 against the current mod (`ai_brain.c` is
4562 lines; `struct model_bridge` is now 34948 bytes because `stance[32]` was added) and against
build-337's source. Every line number below was read from the fork's working tree. How each kind
of fact was checked:

- **Struct offsets.** All 123 raw offsets the mod uses were compiled as `offsetof` static asserts
  against the source headers, with the game's own compile flags (from `ninja -t commands linux`,
  minus LTO and PGO). All of them pass. The check file is
  `build/mendicant_scratch/offsets_check.c` (under `build/`, which git ignores). Copy it into
  `mendicant_platform.c` as a block of static asserts.
- **Inlining.** No function the mod detours or redirects was inlined anywhere in build 273. This
  was checked with `llvm-dwarfdump`: the only `DW_TAG_inlined_subroutine` hits among the hooked
  functions are the two dispatchers, `actor_type_decide_action` and `actor_action_control`, and
  the mod hooks their tables rather than the dispatchers themselves. So a hook at a function's
  entry or return in source matches the binary detour exactly.
- **Call sites.** Every call site the mod redirects was mapped to its enclosing function with
  `objdump`/`addr2line` on build 273 (29 call sites). The results are in the tables below.

Scope: everything the hook library does for the AI of a single-player game or a co-op host.

**Left out of the fork entirely:** theater mode and the AI Chief. That covers `chief_*`,
`mission_navigate`, `mission_target_point`, `mission_skip_target`, `episode` Chief fields,
`let_ai_drive_player_units` (H16, H17), `players_coop_extras_held` (E1), the update-queue
trampoline (E2), the observer camera (E3), and the `theater_*` files.

**What the training loop loses without the AI Chief** (section 10.3 has the details):
- whole-level episodes, which advance only because the Chief walks the level
- scenario fights with `chief.present`, which become fights without a Chief
- the Chief model's training (`chief_request`, `chief_intent`, `chief_style`, the outcome fields)

The ordinary scenario fights (`scenario_mode` 1 with no Chief) keep working.

---

## 0. Ground rules for the port

- **Code style.** gnu89 (all declarations before statements), the house style of
  `port/linux/game/bots.c` (tabs, plain descriptive comments), and no system maths: use
  `sqrt`, `atan2`, `sin`, `cos`, `fabs` (`halo_math.h` routes them to the vendored musl). Write
  `fminf`/`fmaxf` as local macros. `atan2f`, `cosf` and `sinf` become `(real)atan2(...)` and so
  on. `sqrtf(x)` and `(real)sqrt(x)` give the same result; the angle functions may differ by one
  ulp, which is allowed (section 10.7).
- **No addresses, patches or assembly.**
  - Every engine call is a plain C call to the source function.
  - Every global is the source symbol: an `extern` where the source has no header declaration,
    each listed in section 3.
  - Every `FIELD(type, base, 0x...)` becomes `actor->member` (section 4).
  - `datum_try_and_get` replaces the hand-written datum lookups `actor_get`, `prop_get`,
    `go_datum` and `object_pointer`. They return NULL for a stale index, as the mod's do.
- **Off by default.** With `ai.mendicant.enabled = false`, every hook in `source/` is a call that
  returns at once and does exactly what stock does:
  - the wrappers (`mendicant_select_firing_position`, `mendicant_action_charge_setup`,
    `mendicant_handle_lost_contact`) call the original function with the same arguments
  - `mendicant_target_weight` returns its argument
  - the vetoes return FALSE.
- **When the AI runs.** `mendicant_active` is TRUE only when all of these hold:
  - `ai.mendicant.enabled` is set
  - `!game_engine_running()`: campaign or co-op, not a multiplayer game engine
  - `!network_game_distributed_client()`: the host only (`game.c:362`)

  It is computed in `mendicant_initialize_for_new_map` and checked again in
  `mendicant_game_tick_begin`.
- **Network.** Nothing new goes over the wire, so `HALO_PORT_NETWORK_VERSION` (41) stays
  unchanged. Clients see the host's actors through `network_actors.c` as before.
- **Edits in `source/`.** Each change to `source/` is marked `/* port: MENDICANT AI ... */`. Each
  touched file includes the header with a note naming its file:
  `#include "mendicant/mendicant.h" /* port: port/linux/game/mendicant/mendicant.c */`. It
  resolves through the game units' existing `-iquote port/linux/game`.

---

## 1. Hook inventory: every patch, detour and redirect, and its place in source

Columns: the hook library's mechanism → the exact insertion point in the fork → the call placed
in `source/`. The prototypes are in `mendicant.h` (section 2).

### 1.1 Decisions (H1)

| # | Hook library | Build-273 site | Fork insertion point | Call placed in source/ |
|---|---|---|---|---|
| H1 | `decide_wrappers[12]` written over `actor_type_*.decide_action` (`ai_brain.c:4556-4560`, `DECIDE_WRAPPER` at 4376) | table at 0x15ab4e4. `actor_type_decide_action` is inlined into the decision loop, so the loop calls through the table. | `source/ai/actor_types.c:217-218`, in `actor_type_decide_action` (the only caller is `actors.c:1962`) | Around line 218: `mendicant_decide_before(actor_index, actor_type_definition);` then the original `actor_type_definition->decide_action(actor_index);` then `mendicant_decide_after(actor_index);`. The comment: `/* port: MENDICANT AI samples, disciplines and aims each actor before its decision, and commands its unit after it (port/linux/game/mendicant/mendicant.c) */` |

What runs where:

- **`mendicant_decide_before`** runs, in the wrapper's order:
  1. `sample_actor`: the training tally and the sample log (M5)
  2. `discipline_before_decide` (M2)
  3. `aim_actor` (M4)
  4. `vehicle_try` if `ai.mendicant.vehicle_test` is set (M4)
  5. `player_sample` (M5)
- **`mendicant_decide_after`** runs `move_test` (M3), then `drill_update` (M2).
- **Dropped:** the wrapper's `if (training && !tick_hooked) training_update()`. The tick is
  always hooked in source.
- **The type slot** (0 elite … 11 mounted weapon) is the row the actor's definition has among the
  12 `extern struct actor_type_definition actor_type_*` (declared as in `actor_types.c:94-104`).
  It is used only for the sample log's `type` column.

### 1.2 Firing positions (H2, H3, H4)

| # | Hook library | Build-273 site | Fork insertion point | Call placed in source/ |
|---|---|---|---|---|
| H2 | `redirect_selection` → `select_firing_position_entry` (asm) → `select_firing_position` (`ai_brain.c:488`) | call 0x129edef in `action_fight_perform` | `source/ai/action_fight.c:211` | Rename the call `actor_active_select_firing_position(` to `mendicant_select_firing_position(`, with the same six arguments |
| H3 | same | call 0x118b9ef in `action_uncover_perform` | `source/ai/action_uncover.c:137` | same rename |
| H4 | `guard_change_position_entry` → `guard_change_position` (`ai_brain.c:3880`) | call 0x118861b in `action_guard_perform` | `source/ai/action_guard.c:723`, before the `actor_change_firing_position` call | `mendicant_guard_position(actor_index, &selected_firing_position_index, &previous_owner_actor_index, &position_flags);` (locals declared at `action_guard.c:664-666`) |

H2 and H3 behave as follows:

- **An ordered actor** (`order_actor[slot] == actor_index`) returns its `order_position` with
  `*owner = NONE` and `*path_valid = FALSE`, and never calls the original. The selection's
  `candidate` is left unset in the binary. The port zeroes `*best` instead, because
  `action_uncover.c:146-158` reads `candidate.line_of_sight` (section 10.7).
- **Any other actor** gets `actor_active_select_firing_position(...)`, then `leashed_position`.

H4 replaces the position only when the actor is ordered, `order_position >= 0` and it differs.
It then sets `*previous_owner = NONE` and `*cached_path_available = FALSE`.

### 1.3 Action control (H5)

| # | Hook library | Build-273 site | Fork insertion point | Call placed in source/ |
|---|---|---|---|---|
| H5 | `hook_action_control`: `global_action_functions[3/5/6].control` (rodata, +32, stride 56) swapped for `action_control_N` = the original, then `apply_role` | table 0x4aba28. `actor_action_control` is inlined; its only caller is `actors.c:4226`. | `source/ai/actions.c:737-748` | Add the declaration `short controlled_action; /* port: MENDICANT AI */` after line 740. Before line 747: `controlled_action = actor->state.action;`. After line 748: `mendicant_action_control_after(actor_index, controlled_action);` |

The callee does something only for `_actor_action_fight` (3), `_actor_action_uncover` (5) and
`_actor_action_guard` (6). These are exactly the slots the mod swapped, and all three have a
`control` function.

### 1.4 Discipline (H6 to H9)

| # | Hook library | Build-273 site | Fork insertion point | Call placed in source/ |
|---|---|---|---|---|
| H6 | `discipline_panic_entry` at all 3 callers of `actor_action_try_to_panic` | 0x1118599 `handle_panic_transition`, 0x1114cc3 `handle_active_cover_seeking`, 0x111b22f `handle_evasion` | Entry of `actor_action_try_to_panic`, `source/ai/actions.c:3688`. Insert after line 3695. Its callers are lines 1764, 1841 and 4289, and nothing else calls it. | `if (mendicant_veto_panic(actor_index)) return FALSE;` |
| H7 | `discipline_berserk` at 2 of the 4 callers of `actor_berserk` | 0x10923a4 `handle_combat_selection`, 0x11169e6 `handle_berserk_transition`. Not hooked: `ai_scripting_berserk` (0x12d7027) and the `actor_berserk(FALSE)` inlined into `actors_update` (0x12b35b6). | `source/ai/actions.c:2263-2264` and `:3168` | Line 2263 becomes `if (!action_changed && !mendicant_veto_berserk(actor_index))`. Line 3168 becomes `if (!mendicant_veto_berserk(actor_index)) actor_berserk(actor_index, TRUE);` |
| H8 | `discipline_charge_entry` at 2 of the 3 `action_charge_setup` calls | 0x1091d94 and 0x1091e89 in `handle_combat_selection`. Left alone: 0x10922f2 (vehicle strafing). | `source/ai/actions.c:2346` (`_charge_goal_melee`) and `:2501` (`_charge_goal_close_range`). Not line 2397 (`_charge_goal_vehicle_strafing`). | Rename `action_charge_setup(` to `mendicant_action_charge_setup(`, same arguments |
| H9 | `discipline_lost_contact_entry` at 3 of the 4 binary sites of `actor_action_handle_lost_contact` | 0x1115b62 and 0x1115e6f in `handle_combat_status`; 0x1114a60 in `handle_exit_pursuit`. Not hooked: 0x111d75b (`actor_action_set_default_state`). | `source/ai/actions.c:3071, 3079, 3089, 3106, 3122` (every call in `actor_action_handle_combat_status`; the compiler merged them into the 2 binary sites) and `:3253` (`handle_exit_pursuit`). Not line 972. | Rename `actor_action_handle_lost_contact(` to `mendicant_handle_lost_contact(` |

### 1.5 Targets and grenades (H10, H11)

| # | Hook library | Build-273 site | Fork insertion point | Call placed in source/ |
|---|---|---|---|---|
| H10 | `target_weight_entry` at all 13 callers of `actor_compute_prop_target_weight` | 0x1093a6a … 0x12bdb70 (13 sites) | The return statement of `actor_compute_prop_target_weight`, `source/ai/actor_perception.c:2221-2224`. The `return 0.0f` at 2047 is left alone, because the mod passes a weight of 0 or less through unchanged. | `return mendicant_target_weight(actor_index, prop_index, <the original expression>);` |
| H11 | `try_throw_grenade_checked` at both callers of `actor_action_try_to_throw_grenade` | 0x111a801 `consider_grenade` (call), 0x1118d80 `handle_grenade_throwing` (tail jump) | Inside `actor_action_try_to_throw_grenade`, `source/ai/actions.c:3302-3349`. It is called only from 3292 and 3389. | After line 3308, add `boolean throw_ordered_before = actor->orders.combat.throw_grenade; /* port: MENDICANT AI */`. Before `return result;` at 3349, add `mendicant_grenade_throw_checked(actor_index, throw_ordered_before);`. The return value is unchanged. |

H11 in detail:

- The check refuses a throw only when it switched `throw_grenade` on during this call
  (`!before && after`) and the throw would land under 3.5 world units from the thrower or a living
  friend. Refusing clears `orders.combat.throw_grenade` and `control.grenade_trying_to_throw`.
- The roles' own grenades call `actor_action_try_to_throw_grenade(actor_index, TRUE)` directly,
  which now carries the check.

### 1.6 The game tick (H12, H13, H14)

| # | Hook library | Build-273 site | Fork insertion point | Call placed in source/ |
|---|---|---|---|---|
| H12 | `hook_game_tick`: game_tick's call of `ai_update` sent to `game_tick_ai_update` | call 0x12fc1f8, host branch only | `source/game/game.c:362-363` | Brace the host branch: `{ profile_scope(game_tick_ai_section, ai_update();) mendicant_ai_update_end(); }` |
| H13 | `game_tick_wrapper` around game_time_update's call of `game_tick` | call 0x131f272 | `source/game/game_time.c:510`, the only call of `game_tick()` in the tree | `mendicant_game_tick_begin(); game_tick(); mendicant_game_tick_end();` |
| H14 | `go_stub_tick` detour at `game_tick`'s entry → `go_tick` → `go_update` | 0x12fba50 prologue | Folded into H13: `mendicant_game_tick_begin` runs `go_update` last | (same call) |

What each tick call runs:

- **`mendicant_ai_update_end`** runs `vehicles_update` (M4), then `training_update` (M5, when
  training). The AI Chief's `chief_update` is dropped.
- **`mendicant_game_tick_begin`** runs `maneuver_check_map`, then `maneuver_swap(1)`, then
  `go_update`. That is the binary's order: the wrapper swaps, then calls `game_tick`, whose
  detoured entry runs `go_tick` before `real_math_reset_precision`.
- **`mendicant_game_tick_end`** runs `maneuver_swap(0)`.

### 1.7 Movement (H15)

| # | Hook library | Build-273 site | Fork insertion point | Call placed in source/ |
|---|---|---|---|---|
| H15 | `go_stub_*` detours of 5 entries → `go_intercept(kind, actor, point)` | 0x118d860, 0x118eec0, 0x118cff0, 0x118da40, 0x118ed80. None is inlined anywhere. | `source/ai/actor_moving.c`: `actor_move_to_point` (2243, insert after 2250), `actor_move_to_move_position` (2281, after 2286), `actor_move_to_firing_position` (2310, after 2316), `actor_move_to_prop` (2340, after 2346), `actor_move_halt` (3546, after 3549; add the declaration `boolean result; /* port */`) | `if (mendicant_move_redirect(_mendicant_move_point, actor_index, destination, &result)) return result;`, and the same with `_mendicant_move_move_position`, `_mendicant_move_firing_position`, `_mendicant_move_prop` (point NULL) and `_mendicant_move_halt` (point NULL) |

`mendicant_move_redirect` returns FALSE (the game's function runs) in any of these cases:

- `go_inside` is set
- the actor has no order, or its order is not MOVING or ARRIVED
- the order has no waypoint surface yet
- the actor yields (it is fighting, fleeing and so on)
- the kind is "point" and the game's point is within 0.1 world units of the waypoint

Otherwise it sets `go_inside`, calls `actor_move_to_point(actor_index, &waypoint, waypoint_surface, NONE)`
(which re-enters, and the guard lets it through), stores that result in `*result` and returns
TRUE.

### 1.8 Training-only output skipping (T1, T2, T3)

| # | Hook library | Build-273 site | Fork insertion point | Call placed in source/ |
|---|---|---|---|---|
| T1 | call to `render_frame` sent to `training_frame` (training only; bit 1 of `AI_BRAIN_TRAINING_SKIP`) | 0x11c2891, in `main_game_render`, which is inlined into `main_loop` | `source/main/main.c:3394`, inside `if (global_screenshot_count.count <= 0)` (3392) | `if (!mendicant_skip_render_frame()) render_frame(...);`. The callee sleeps 1 ms when the episode is over (`posix_mendicant_sleep_microseconds(1000)`). |
| T2 | `sound_render` call NOPed (bit 2) | 0x11c0054 | `source/main/main.c:3328` (`main_game_render`). Not line 1953. | `if (!mendicant_skip_sound_render()) sound_render();` |
| T3 | the call to `mix` in `silent_clock_thread` NOPed (bit 4) | 0x14df6ad | `port/linux/src/dsound_sdl.c:1589`. This is the platform layer, not `source/`, so it needs no `port:` comment. | A static `skip_mix` read once on that thread: `config_boolean("ai.mendicant.enabled") && config_boolean("ai.mendicant.training") && (config_integer("ai.mendicant.training_skip") & 4)`. Then `if (!skip_mix) mix(buffer, 480);`. dsound_sdl.c already includes `port_config.h` (line 54). |

### 1.9 Lifecycle

The hook library had no lifecycle hooks; it relied on a scenario pointer changing.

| # | Fork insertion point | Call placed in source/ |
|---|---|---|
| L1 | `source/ai/ai.c:1934`, the end of `ai_initialize_for_new_map` (its only `return`) | `mendicant_initialize_for_new_map();` |
| L2 | `source/ai/ai.c:533`, the start of `ai_dispose_from_old_map` | `mendicant_dispose_from_old_map();` |

### 1.10 Left out (AI Chief and theater)

| # | What | Status |
|---|---|---|
| H16 | jz NOPed at 0x12b9e77 in `actors_update` (`actors.c:4170`): an actor may drive a player's unit | **Not ported** (AI Chief) |
| H17 | `unit_control` calls NOPed at 0x1326f0c and 0x1327377 (`players.c:5064/5082`) | **Not ported** (AI Chief) |
| E1 | `players_coop_extras_held` entry made to return 1 | **Not ported** (theater) |
| E2 | update-queue trampoline (`actors.c:4178` / `players.c:4879`) | **Not ported** (theater) |
| E3 | `director_update` redirect (`main.c:3696`) | **Not ported** (theater camera) |

Edit count: 11 files in `source/`, about 30 call sites. The files are:
- `source/ai/actor_types.c`, `actions.c`, `action_fight.c`, `action_uncover.c`, `action_guard.c`,
  `actor_perception.c`, `actor_moving.c`, `ai.c`
- `source/game/game.c`, `game_time.c`
- `source/main/main.c`

One file in `port/linux/src` changes: `dsound_sdl.c`.

---

## 2. `port/linux/game/mendicant/mendicant.h`: the calls from source/

```c
/* mendicant.h: MENDICANT AI's calls from the game's sources (port/linux/game/mendicant)

Every function does nothing, or exactly what the stock call would, unless
ai.mendicant.enabled is set and this machine runs the AI (the host of a
campaign or co-op game). */
#ifndef HALO_MENDICANT_H
#define HALO_MENDICANT_H

struct actor_type_definition;
struct firing_position_evaluation_context;
struct firing_position;
struct path_state;
struct charge_state_data;

/* mendicant_move_redirect's kinds: the five actor_move_* entries */
enum
{
	_mendicant_move_firing_position = 0,
	_mendicant_move_move_position,
	_mendicant_move_prop,
	_mendicant_move_halt,
	_mendicant_move_point,
};

/* a map's start and end (ai.c) */
void mendicant_initialize_for_new_map(void);
void mendicant_dispose_from_old_map(void);

/* the game tick (game_time.c around game_tick, game.c after the host's ai_update) */
void mendicant_game_tick_begin(void);
void mendicant_game_tick_end(void);
void mendicant_ai_update_end(void);

/* every decision of every actor (actor_types.c) */
void mendicant_decide_before(long actor_index, struct actor_type_definition const *definition);
void mendicant_decide_after(long actor_index);

/* actions (actions.c) */
void mendicant_action_control_after(long actor_index, short action);
boolean mendicant_veto_panic(long actor_index);
boolean mendicant_veto_berserk(long actor_index);
boolean mendicant_action_charge_setup(long actor_index, short goal, struct charge_state_data *state_data);
boolean mendicant_handle_lost_contact(long actor_index);
void mendicant_grenade_throw_checked(long actor_index, boolean throw_ordered_before);

/* firing positions (action_fight.c, action_uncover.c: in place of
actor_active_select_firing_position; action_guard.c: before its change) */
short mendicant_select_firing_position(long actor_index,
	struct firing_position_evaluation_context *evaluation_context,
	struct firing_position *best_firing_position, long *current_owner,
	struct path_state *area_path_state, boolean *area_path_state_valid);
void mendicant_guard_position(long actor_index, short *firing_position_index,
	long *previous_owner, boolean *cached_path_available);

/* movement (actor_moving.c): TRUE when an ordered actor went to its
waypoint instead; *result is then what the entry returns */
boolean mendicant_move_redirect(short kind, long actor_index, real_point3d const *point, boolean *result);

/* target weights (actor_perception.c) */
real mendicant_target_weight(long actor_index, long prop_index, real weight);

/* training without a picture or sound (main.c) */
boolean mendicant_skip_render_frame(void);
boolean mendicant_skip_sound_render(void);

#endif
```

`real`, `real_point3d` and `boolean` come from `cseries.h`, which every including file already
has.

---

## 3. Calls through an address (class b) and globals: what replaces them

### 3.1 Engine functions

Every inline-asm call becomes a plain call:

| Mod wrapper (address) | Source call | Notes |
|---|---|---|
| `game_time_get` 0x11cbc10 (~20 sites, `go_time`) | `game_time_get()` (`game_time.c:188`, `game.h:181`) | Asserts initialization; called only inside the tick. |
| `actor_change_firing_position` 0x118c740 (`ai_brain.c:374`) | `actor_change_firing_position(actor_index, position, NULL, NONE, NULL, FALSE)` (`actor_firing_position.c:2600`) | The firing_position argument is unused (2602-2660). |
| `call_active_select_firing_position` 0x118bbf0 | `actor_active_select_firing_position(...)` (`:2477`) | |
| `collision_test_vector` 0x1045ea0 (`ai_brain.c:1127`, `go_collision_test_vector`) | `collision_test_vector(flags, &point, &vector, NONE, &collision)` (`collisions.h:141`), with a `struct collision_result` (0x50 bytes) | Flags 0x2b = `_collision_test_front_facing_surfaces_bit`, `back_facing`, `ignore_invisible`, `structure` (bits 0, 1, 3, 5). Flags 0x21 = front-facing and structure. |
| `actor_change_encounter` 0x12afe60 (`ai_brain.c:1889`) | `actor_change_encounter(actor, encounter, (short)squad)` (`actors.c:1329`, `actors.h:1101`) | Used by `drill_update` when a lone Marine joins another squad. |
| `actor_move_to_point` 0x118ed80 (`ai_brain.c:2571`, `go_move_to_point`) | `actor_move_to_point(actor, &point, surface, NONE)` (`actor_moving.c:2243`) | |
| `actor_action_test_grenade` 0x1183500 (fastcall) | `actor_action_test_grenade(actor)` (`actions.c:2066`) | Not in a header: `extern boolean actor_action_test_grenade(long actor_index);` exactly as `actions.c:400` declares it. |
| `actor_action_try_to_throw_grenade` 0x11833a0 | `actor_action_try_to_throw_grenade(actor, TRUE)` (`actions.c:3302`) | Extern, as `actions.c:414` declares it. |
| `actor_combat_plan_grenade_trajectory` 0x1183a40 (4 arguments in the binary) | `actor_combat_plan_grenade_trajectory(actor, variant->grenade_combat.trajectory_type, &point, prop, ignore)` (`actor_combat.c:999`, `actors.h:1245`) | `trajectory_type` is unused by the body. gdb on build 273 shows it optimized out, which is why the binary takes 4 arguments. |
| `actor_action_try_to_panic` 0x1187a30 | (the veto sits in its entry; it is not called) | |
| `actor_berserk` 0x11f9d80 | `actor_berserk(actor, FALSE)` in `discipline_before_decide` (`actors.h:1567`) | |
| `action_charge_setup` 0x11911a0 | `action_charge_setup(actor, goal, state)` (`action_charge.c:899`, `actions.h:439`) | |
| `actor_action_handle_lost_contact` 0x111b780 | `actor_action_handle_lost_contact(actor)` (`actions.c:2581`) | Extern, as `actions.c:393` declares it. |
| `actor_action_change` 0x111dde0 (`actor_action_change_to`) | `actor_action_change(actor, _actor_action_fight, &data)`, with a zeroed `struct action_state_data` (`actions.c:2005`, `actions.h:413`) | |
| `actor_combat_get_firing_variant_definition` 0x10a3200 | `actor_combat_get_firing_variant_definition(actor)` (`actor_combat.c:441`, `actors.h:1232`) | |
| `actor_compute_prop_target_weight` 0x109c920 | (the hook sits in its return) | |
| `actor_action_try_to_enter_vehicle` 0x11c7cd0 (`ai_brain.c:4259`, `vehicles.inc` `try_to_enter`) | `actor_action_try_to_enter_vehicle(actor, vehicle, "", desire, 0, NULL)` (`actions.c:3713`, `actions.h:513`) | |
| `unit_scripting_enter_vehicle` 0x11ffee0 (`magic_enter`) | `unit_scripting_enter_vehicle(unit, vehicle, seat)` (`units.c:4837`, `units.h:976`) | |
| `vehicle_scripting_unload` 0x11f4330 (`unload_seats`) | `vehicle_scripting_unload(vehicle, seats)` (`units.c:4950`, `units.h:811`) | |
| `unit_exit_seat_end` 0x1408470 | `unit_exit_seat_end(unit)` (`units.c:3187`, `units.h:598`) | |
| `actor_path_input_new` 0x113b5e0 | `actor_path_input_new(actor, &state.input)` (`actors.h:1281`) | |
| `path_state_find` 0x11854f0, `path_state_build_path` 0x113c3e0 | `path_state_find(&state)`, `path_state_build_path(&state, &result)` (`path.h:332/334`) | |
| `ai_update`, `game_tick`, the 12 `*_decide_action`, the 3 `action_*_control` | (no longer re-called: the hooks sit around the originals) | |
| `unit_scripting_exit_vehicle`, `ai_scripting_attach_free`, `encounter_force_activate`, `object_set_position`, `main_switch_structure_bsp` | **not ported**: used only by the AI Chief (`ai_brain.c:2919, 2936, 2956, 2180, 2248`, 3169) | |

### 3.2 Globals

| Mod address | Source symbol | Declared |
|---|---|---|
| `actor_data` 0x1cc1ab8 | `actor_data` | `ai/actors.h:1581` |
| `encounter_data` 0x1cc1ab4 | `encounter_data` | `ai/encounters.h:253` |
| `prop_data` 0x2dd6310 | `prop_data` | `ai/props.h:196` |
| `object_header_data` 0x25dc7bc | `object_header_data` | `objects/objects.h:502` |
| `player_data` 0x27dfe40 | `player_data` | `game/players.h:459` |
| `global_scenario` 0x245bb68 | `global_scenario` | `scenario/scenario.h:161` |
| `global_structure_bsp_index` 0x15d0554 | `global_structure_bsp_index` | `scenario/scenario.h:158` |
| `global_structure_bsp` 0x1c0f6a8 | `global_structure_bsp` | `scenario.c:275`, no header: `extern struct structure_bsp *global_structure_bsp;` |
| `global_current_collision_user_depth` 0x1cbc6dc | the same name | `physics/collision_usage.h:91` |
| `game_time_globals` 0x19d8ff0, `+0x18 speed` | `game_time_set_speed(real)` | `game.h:195`. The port clamps the speed to 0..100; the trainer only ever writes 0. |
| `game_globals` 0x19c9738, `+0xe difficulty` | `game_difficulty_level_get()` / `game_difficulty_level_set()` | `game.h:149-150` |
| next-map difficulty 0x15c36c8 | `global_difficulty_level` | `main.c:712`, no header: `extern short global_difficulty_level;` |
| random seeds 0x1a95b34/38 | `get_global_random_seed_address()` / `get_global_local_random_seed_address()` | `real_math.h:843-844`. Training's `episode_start` only. |
| tag table 0x2dd5f20 / count 0x18967f8 | `tag_get_name(tag_index)` | `tag_files/tag_files.h:29` |
| `global_action_functions` 0x4aba28 | (no longer touched: H5 is a call) | |
| actor type table 0x15ab4e4 | (no longer touched: H1 is a call) | |
| build id 0x4001f8 | (gone) | |

`player_unit()` reads absolute player 0 exactly as the mod does: the player is used when
`player_data->valid`, `player_data->count >= 1` and slot 0's identifier is non-zero. It is not
"the first valid player".

---

## 4. Struct offsets: the member that replaces each

Verified by a static-assert compile with the game's flags.

### actor_datum (`ai/actors.h:924`)

| Offset | Member |
|---|---|
| 0x4 | `meta.type` |
| 0x6 | `meta.swarm` |
| 0x8 | `meta.active` |
| 0x18 | `meta.unit_index` |
| 0x34 | `meta.encounter_index` |
| 0x3a | `meta.squad_index` |
| 0x3c | `meta.platoon_index` |
| 0x3e | `meta.team_index` |
| 0x50 | `meta.first_prop_index` |
| 0x5c | `meta.variant_definition_index` |
| 0x6c | `state.action` |
| 0x6e | `state.combat_status` |
| 0x9c | `state.action_data.fight.firing_position_timer` |
| 0x9d | `state.action_data.uncover.uncover_done` |
| 0xa4 | `state.action_data.uncover.pursuit_location.type` |
| 0x158 | `input.vehicle_index` |
| 0x160 | `input.vehicle_passenger` |
| 0x164 | `input.pathfinding_surface_index` |
| 0x168/0x16c/0x170 | `input.pathfinding_point.x/.y/.z` (the mod's "position") |
| 0x1b8 | `input.body_vitality` |
| 0x1bc | `input.shield_vitality` |
| 0x1c0 | `input.recent_body_damage` |
| 0x1c4 | `input.recent_shield_damage` |
| 0x1ca | `external_orders.playfighting` |
| 0x1cb | `external_orders.disable_charging` |
| 0x1ec | `situation.known_enemies` |
| 0x1ed | `situation.visible_reachable_enemies` |
| 0x200 | `situation.area_friends` |
| 0x245 | `situation.close_friends` |
| 0x268 | `target.target_type` |
| 0x270 | `target.target_prop_index` |
| 0x375 | `emotions.forced_to_charge` |
| 0x378 | `emotions.berserk` |
| 0x3b8 | `firing_positions.current_position_index` |
| 0x426 | `orders.move.stationary_crouch` |
| 0x427 | `orders.move.moving_crouch` |
| 0x454 | `orders.combat.shoot_at_target` |
| 0x457 | `orders.combat.override_firing_restrictions` |
| 0x458 | `orders.combat.override_burst_duration` |
| 0x45c | `orders.combat.throw_grenade` |
| 0x45d | `orders.combat.use_manual_target_point` |
| 0x460 | `orders.combat.target_point` |
| 0x46c | `control.path.destination_orders.destination_type` |
| 0x4a8 | `control.path.path.valid` |
| 0x504 | `control.moving` |
| 0x6a0 | `control.grenade_trying_to_throw` |
| 0x6a8 | `control.grenade_current_target` |
| 0x6bc | `control.grenade_current_aim_vector` |
| 0x6c8 | `control.grenade_current_aim_speed` |

### prop_datum (`ai/props.h`)

| Offset | Member |
|---|---|
| 0x8 | `next_prop_index` |
| 0x18 | `unit_index` |
| 0x1c | `actor_index` |
| 0x60 | `enemy` |
| 0x74 | `currently_damaging_me` |
| 0x80 | `last_perceived_body_position` |
| 0x110 | `vehicle_index` |
| 0x11c | `distance` |
| 0x12e | `player` |

### unit_datum (`units/units.h:555`)

Fields of vehicles are read through the same struct.

| Offset | Member |
|---|---|
| 0 | `definition_index` |
| 12 | `object.position` |
| 24 | `object.translational_velocity` |
| 56 | `object.up.k` |
| 60 | `object.angular_velocity` |
| 0x88 | `object.maximum_body_vitality` |
| 0x8c | `object.maximum_shield_vitality` |
| 144 | `object.body_vitality` |
| 148 | `object.shield_vitality` |
| 204 | `object.parent_object_index` |
| 468 | `unit.desired_facing_vector` |
| 492 | `unit.aiming_vector` |
| 552 | `unit.throttle` |
| 674 | `unit.current_weapon_index` |
| 680 | `unit.weapon_object_indices[4]` |
| 718/719 | `unit.grenade_counts[0/1]` |
| 720 | `unit.current_zoom_level` |
| 0x2d4 | `unit.driver_object_index` |
| 0x2d8 | `unit.gunner_object_index` |

### Other structs

| Struct (header) | Offset | Member |
|---|---|---|
| `object_header_datum` (`objects.h:218`) | 3 | `type` (vehicle is 1) |
| | 8 | `datum` |
| `player_datum` (`players.h`) | 52 | `unit_index` |
| `data_array` (`memory/data.h:21`) | 0x20 | `maximum_count` |
| | 0x22 | `size` |
| | 0x24 | `valid` |
| | 0x2e | `count` |
| | 0x34 | `data` |
| `scenario` (`scenario_definitions.h:244`) | 0x42c/0x430 | `ai_encounters.count/.address` |
| `encounter_definition` (`ai_scenario_definitions.h:323`, 0xb0) | 0x7e | `runtime_structure_bsp_reference_index` |
| | 0x80 | `squads` |
| | 0x98/0x9c | `firing_positions.count/.address` |
| `squad_definition` (0xe8) | 0x54 | `firing_position_groups[8]` |
| `firing_position_definition` (0x18) | 0 | `position` |
| | 0xc | `group_index` |
| | 0xe | `cluster_index` |
| | 0x14 | `surface_index` |
| `encounter_datum` (`encounters.h:35`) | 0xc | `force_active` |
| `actor_variant_definition` (`actor_definitions.h:403`) | 124 | `ranged_combat.projectile_error_angle` |
| | 204 | `ranged_combat.burst_geometry.burst_origin_radius` |
| | 224 | `…burst_duration_lower/upper_bound` |
| | 232 | `…burst_separation_lower/upper_bound` |
| | 340 | `ranged_combat.special_fire_mode` |
| | 342 | `…special_fire_situation` |
| | 344 | `…special_fire_chance` |
| | 348 | `…special_fire_delay` |
| `collision_result` (`collisions.h:86`) | 0x18 | `point` |
| | 0x10 | `location.cluster_index` |
| | 0x44 | `surface_index` |
| `path_state` (`path.h:139`, 82060 bytes) | 16 | `input.start_valid` |
| | 20 | `input.start_point` |
| | 32 | `input.start_surface_index` |
| | 72 | `debug` |
| | 76 | `destination_valid` |
| | 80 | `destination.point` |
| | 92 | `destination.surface_index` |
| | 96 | `destination.target_radius` |
| | 100 | `structure` |
| `path_result` | 0 | `valid` |
| | 4 | `endpoint.point` |
| | 16 | `endpoint.surface_index` |

Notes:

- `go_planner_begin` reads the path radius from offset 0 of the path input. That is
  `path_input.pathfinding_radius`.
- `ACTOR_IS_ELITE`, `ACTOR_IS_MARINE`, `ACTOR_IS_GRUNT` and `ACTOR_IS_JACKAL` keep their numeric
  `meta.type` sets (0/5, 6/7, 2/14, 1). The type table at `actor_types.c:106` confirms them.
- The two actions the mod writes as the bare numbers 6 and 9 become `_actor_action_guard` and
  `_actor_action_vehicle` (`actors.h:121-137`).

### Data the mod writes (tag and runtime mutations kept as they are)

- **The `maneuver.inc` swap.** Each encounter definition's `firing_positions.count/.address` is
  pointed at a longer copy during the tick only (M3). The port keeps this behaviour for identity,
  but uses the members (`definition->firing_positions.count`, `.address`), `calloc`/`free`, and
  frees at L2.
- **`aim_actor`.** Writes the variant's error, burst and special-fire members each decision, and
  keeps the originals (M4).
- **Scenario director (training).** Writes `object.maximum_body_vitality/maximum_shield_vitality`
  and the external-orders and emotions flags (M5).
- **`actor_go`.** Writes `encounter_datum.force_active` (M3).
- **Autopilot.** Writes the driver unit's `unit.throttle` and `unit.desired_facing_vector` after
  the AI update (M4). In co-op these writes are not relayed through `unit_control`, so a client's
  copy of an autopilot-driven Warthog follows the host's object sync only. This is the same as the
  hook library.

---

## 5. The five modules

Files, all under `port/linux/game/mendicant/`, all gnu89:

| Module | Files |
|---|---|
| shared | `mendicant.h` (section 2), `mendicant_internal.h` (the interface below), `doctrine_table.h` (copied from the mod byte for byte, still generated by `tools/doctrine_gen.py`; included **only** by `mendicant_command.c`) |
| M1 runtime | `mendicant_runtime.c`, `mendicant_infer.c`, `mendicant_infer.h` |
| M2 command | `mendicant_command.c` |
| M3 movement | `mendicant_movement.c` (actor_go), `mendicant_maneuver.c`, `mendicant_positions.c` |
| M4 combat | `mendicant_combat.c`, `mendicant_vehicles.c` |
| M5 platform | `mendicant.c` (the source-facing hooks, dispatch, lifecycle, config), `mendicant_bridge.c`, `mendicant_bridge.h`, `mendicant_training.c`, `mendicant_logs.c` |
| Platform helper | `port/linux/src/posix_mendicant.c` (Linux desktop only) |

Constants live in `mendicant_internal.h` and keep the mod's names:
- `MENDICANT_MAX_ACTORS` 1024, `MENDICANT_MAX_ENCOUNTERS` 256, `MENDICANT_MAX_SQUAD` 32,
  `MENDICANT_UNIT_KEYS` (1024+256), `MENDICANT_MAX_UNITS` 64, `MENDICANT_MAX_POSITIONS` 512
- the teams: `TEAM_PLAYER` 1, `TEAM_HUMAN` 2, `TEAM_COVENANT` 3, `TEAMS` 4
- `MODE_GAME`, `MODE_DRILL`, `MODE_MODEL`
- the `ROLE_*` enum (12), `STANCES` 5, `BRIDGE_*` sizes, `ENTITIES`, `ENTITY_FEATURES`

### 5.1 Who owns which state

The module named owns the storage and is the only one that resets it. The others read it through
externs in `mendicant_internal.h`. A module may write another's state only where this table says
so.

| State | Owner | Written also by |
|---|---|---|
| `mendicant_config` (struct of every knob, filled each new map), `mendicant_active`, `mendicant_training` | M5 | none |
| `mendicant_bridge` (`struct model_bridge *`: the shared memory, or M5's static local struct), `episode_over`, `actor_team[]`, `team_damage[]`, `last_sample_time[]`, `actor_sampled[]`, `first_sight[]`, log `FILE *`s (`sample`, `drill`, `player`, `move_test`) | M5 | M2 writes request fields of the bridge; M1 writes answer fields |
| `order_actor[]`, `order_position[]`, `actor_role[]`, `actor_grenade_due[]`, `actor_leash[][4]`, `leash_corrections`, `enrolled_actor[]`, `enrolled_key[]`, `cmd[]`, `cmd_leader[]`, `actor_team_of[]`, `actor_disciplined[]`, `actor_mover[]`, `actor_face[][3]`, `unit_assault_range[]`, `commanded_time[]`, `next_bound_time[]`, plan memory (`plan_*`, `memory_*`, `personality*`, `unit_*`, `pending_outcome/own/enemy`), `pending_members[]`, `pending_positions[][]`, `bridge_pending`, `bridge_request_time`, `bridge_lost_last`, `drill_random`, `drill_update_time`, `position_reach_centre/position_reach`, counters (`chains_*`, `solo_samples`, `member_samples`, `discipline_declined[4]`) | M2 | M4: `actor_role[]` (stances, through `mendicant_set_role`) and `actor_grenade_due[]` (cleared by `grenade_ask`). M3: `order_actor/order_position` only through `mendicant_order()`. |
| go orders, waypoint graph, `go_path_state` (82060 bytes, static), `go_forced_by_us[]`, maneuver blocks, pool and slots, `cover_wanted`/`cover_threat`/`cover_rays`, `go_by_drill[]`, `follow_going[]`, `follow_*[]`, `go_fallbacks`, `marines_hops/stuck`, move test state | M3 | M2 sets `cover_wanted`/`cover_threat` before a position search, through `mendicant_cover_want()`. M4 cancels a follow through `mendicant_follow_cancel()`. |
| `aim_original[96]`, `aimed_count`, `aim_target[]`, `aim_target_since[]`, `grenade_wait_since[]`, `grenades_refused`, `targeting_changes`, `role_applications[]`, `autopilot[16]`, vehicle timers, `gunner_left_vehicle`, `individual_next[]`, `individual_cursor`, `individual_requests` | M4 | none |
| weights (`aib_weights`), the static workspace (about 66 KB), the sampling RNG (splitmix64), the weights file stamp | M1 | none |

All of it is static storage (BSS), never on the Android guest stack. The only heap use:
- maneuver blocks (`calloc`), freed at map dispose
- weights storage (`malloc`, 6.6 MB), kept across maps

### 5.2 `mendicant_internal.h`: the declarations between modules

```c
/* ---- M5 platform (mendicant.c, mendicant_bridge.c, mendicant_training.c, mendicant_logs.c) */
struct mendicant_config {
	boolean enabled, training, discipline, targeting, individuals, aim_difficulty, aim_moving,
		aim_debug, vehicle_test, models;
	long training_skip, seed;
	real aim, marine_fire, command_radius, variety;
	char bridge[256], weights[128], sample_log[256], drill_log[256], player_log[256], move_test_log[256];
};
extern struct mendicant_config mendicant_config;
extern boolean mendicant_active;          /* enabled, campaign or co-op, and this machine runs the AI */
extern struct model_bridge *mendicant_bridge; /* NULL: the drill alone (no models, no shared memory) */
extern boolean mendicant_episode_over;
extern FILE *mendicant_drill_file;        /* NULL unless ai.mendicant.drill_log */
long mendicant_time(void);                /* game_time_get() */
long mendicant_player_unit(void);         /* absolute player 0's unit, or NONE */
/* a request is complete: in-process (M1 answers now) or shared memory
(published; in training, waits up to 200 ms for the trainer) */
void mendicant_bridge_submit(void);
boolean mendicant_bridge_answered(void);  /* response_sequence == request_sequence */
void mendicant_training_tally(long actor_index, struct actor_datum *actor); /* sample_actor's tally */

/* ---- M1 runtime (mendicant_runtime.c) */
boolean mendicant_runtime_load(void);     /* ai.mendicant.weights through port_mods; FALSE: drill alone */
void mendicant_runtime_dispose(void);
boolean mendicant_runtime_ready(void);
void mendicant_runtime_answer(struct model_bridge *bridge); /* fills the answer, then response_sequence */

/* ---- M2 command (mendicant_command.c) */
extern long order_actor[MENDICANT_MAX_ACTORS];
extern short order_position[MENDICANT_MAX_ACTORS];
extern char actor_role[MENDICANT_MAX_ACTORS];
extern unsigned char actor_grenade_due[MENDICANT_MAX_ACTORS];
extern real actor_leash[MENDICANT_MAX_ACTORS][4];
extern real actor_face[MENDICANT_MAX_ACTORS][3];
extern real unit_assault_range[MENDICANT_MAX_ACTORS];
extern long leash_corrections;
void mendicant_command_reset(void);       /* the constructor's initial state, and each new map */
void mendicant_command_episode_reset(unsigned long seed); /* episode_start's command part */
void mendicant_command_episode_report(struct model_bridge *bridge); /* episode_end's counters */
void mendicant_orders_clear_all(void);    /* every order_actor[] to NONE */
void mendicant_discipline_before_decide(long actor_index);
void mendicant_drill_update(void);        /* bridge_poll, enrolment, units, then M4 individuals, then M3 follow */
int mendicant_team_mode(short team);      /* MODE_GAME / MODE_DRILL / MODE_MODEL */
int mendicant_side_of(short team);
int mendicant_unit_side(struct actor_datum *actor);   /* SIDE_COVENANT / SIDE_MARINES */
boolean mendicant_actor_can_drill(struct actor_datum *actor);
void mendicant_order(long actor_index, short position); /* cancels a go order by drill or follow first */
void mendicant_set_role(long actor_index, int role);
unsigned long mendicant_drill_rand(void);
/* bridge_request: candidates, team features, then mendicant_bridge_submit (individuals use it too) */
void mendicant_bridge_request(long key, struct mendicant_member *members, int count,
	real_point3d const *target, real axis_x, real axis_y, boolean commanded, long time);
/* the doctrine as M1 needs it (doctrine_table.h stays private to M2) */
int mendicant_doctrine_chains(void);      /* CHAINS (24) */
int mendicant_doctrine_step_choices(void);/* STEPS + 1 (33) */
void mendicant_doctrine_chain_covenant(unsigned char *chain_covenant); /* 1 for a Covenant chain */
unsigned long mendicant_doctrine_hash(void); /* FNV-1a, export_weights.py's doctrine_hash text */
struct mendicant_member { long actor_index; struct actor_datum *actor; };

/* ---- M3 movement (mendicant_movement.c, mendicant_maneuver.c, mendicant_positions.c) */
enum { GO_STATUS_NONE = 0, GO_STATUS_MOVING, GO_STATUS_ARRIVED, GO_STATUS_YIELDING,
	GO_STATUS_NO_GROUND = -1, GO_STATUS_NO_PATH = -2, GO_STATUS_GONE = -3 };
int actor_go_to(long actor_index, real_point3d const *point, real tolerance, int urgency);
int actor_go_status(long actor_index);
void actor_go_cancel(long actor_index);
void mendicant_movement_reset(void);      /* go orders, graph, follow, maneuver: each new map */
void mendicant_movement_dispose(void);    /* puts the level's own firing position lists back, frees blocks */
void mendicant_movement_tick_begin(void); /* maneuver_check_map, swap in, go_update */
void mendicant_movement_tick_end(void);   /* swap out */
void mendicant_maneuver_check_map(void);
short mendicant_select_position(...);     /* body of mendicant_select_firing_position */
void mendicant_guard_change(...);          /* body of mendicant_guard_position */
boolean mendicant_go_redirect(short kind, long actor_index, real_point3d const *point, boolean *result);
void mendicant_follow_chief(long time);   /* marines_follow_chief */
void mendicant_follow_cancel(long actor_index);
void mendicant_go_release_drill(long actor_index);
void mendicant_cover_want(int cover, real_point3d const *threat);
/* the position services the drills use (ai_brain.c 1096-1340 today) */
struct firing_position_definition *mendicant_encounter_firing_positions(long encounter_index, int *count);
boolean mendicant_firing_position_owned(long encounter_index, short position);
boolean mendicant_position_usable(long encounter_index, short position, struct firing_position_definition const *entry);
int mendicant_drill_position(struct actor_datum *actor, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, long *used);
boolean mendicant_drill_move(struct mendicant_member *member, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, long *used);
void mendicant_drill_hold(struct mendicant_member *member, long *used);
boolean mendicant_assign_position(struct mendicant_member *member, short position);
boolean mendicant_go_fallback(struct mendicant_member *member, real_point3d const *point, real tolerance);
int mendicant_maneuver_slot(long encounter_index, real_point3d const *point);
int mendicant_maneuver_near(real_point3d const *point, real radius);
int mendicant_line_blocked(real_point3d const *from, real_point3d const *to); /* -1 no BSP, 0 clear, 1 blocked */
void mendicant_move_test(long actor_index);
void mendicant_movement_report(struct model_bridge *bridge); /* go_fallbacks, follow moves, stuck */
struct mendicant_geometry { real_point3d const *target; real axis_x, axis_y; };

/* ---- M4 combat (mendicant_combat.c, mendicant_vehicles.c) */
void mendicant_combat_reset(void);
void mendicant_aim_actor(long actor_index);
void mendicant_apply_role(long actor_index);       /* after fight/uncover/guard control */
real mendicant_targeting_scale(long actor_index, long prop_index, real weight);
boolean mendicant_grenade_lands_on_friends(struct actor_datum *actor);
void mendicant_individuals_update(long time);      /* the per-enemy models' requests */
void mendicant_apply_stance(long actor_index, int stance); /* bridge_apply's stance branch */
void mendicant_vehicles_update(void);              /* autopilot every tick, boarding every second */
void mendicant_vehicle_try(void);                  /* ai.mendicant.vehicle_test */
boolean mendicant_carries_plasma_pistol(struct unit_datum *unit);
boolean mendicant_carries_assault_rifle(struct unit_datum *unit);
int mendicant_weapon_class(struct unit_datum *unit);
```

### 5.3 How the source hooks dispatch (in `mendicant.c`)

| Hook | Runs |
|---|---|
| `mendicant_decide_before` | M5 `mendicant_training_tally` and the sample log (`sample_actor`), then M2 `mendicant_discipline_before_decide`, M4 `mendicant_aim_actor`, M4 `mendicant_vehicle_try` (when set), and M5 `player_sample` |
| `mendicant_decide_after` | M3 `mendicant_move_test` (when set), then M2 `mendicant_drill_update` |
| `mendicant_action_control_after` | M4 `mendicant_apply_role`, for actions 3, 5 and 6 |
| `mendicant_veto_panic`, `mendicant_veto_berserk`, `mendicant_action_charge_setup`, `mendicant_handle_lost_contact` | M2 (discipline), each only when `mendicant_config.discipline` is set |
| `mendicant_grenade_throw_checked` | M4 |
| `mendicant_target_weight` | M4 `mendicant_targeting_scale`, only when `mendicant_config.targeting` is set |
| `mendicant_select_firing_position`, `mendicant_guard_position`, `mendicant_move_redirect` | M3 |
| `mendicant_game_tick_begin` / `_end` | M3 |
| `mendicant_ai_update_end` | M4 `mendicant_vehicles_update`, then M5 `training_update` (when training) |
| `mendicant_skip_render_frame` / `mendicant_skip_sound_render` | M5 |
| `mendicant_initialize_for_new_map` | Reads the config. Then, when this is the first map or the config changed: M2 reset, M3 reset, M4 reset, M1 load (if `models` is set and not yet loaded, or if the file's stamp changed), and M5 opens the bridge or binds the local struct and opens the logs. |
| `mendicant_dispose_from_old_map` | M3 dispose, which must run first so the level's own firing position lists are back before the tags go |

---

## 6. M1 runtime: C inference in place of brain.py

**Code.** Adapt `data/distribution_research/inference/aib_infer.{c,h}` into `mendicant_infer.{c,h}`:
- rename the prefix `aib_` to `mendicant_infer_`
- keep the ops, `aib_expf`/`aib_tanhf` (Cephes, only + − × ÷), and the fixed-order
  8-accumulator dot product
- remove the chief model binding (the AI Chief is left out; the export's `chief.*` tensors are
  ignored)

Two additions:

1. **The stance model.** `policy.StanceModel` is `Linear(18, 32) → tanh → Linear(32, 5)`, with
   tensors `squad.<side>.stance.layers.0/.2`. The prototype predates it. Add `mendicant_stance`
   to the squad struct, bind it optionally (`has_stance[side]`), and add
   `mendicant_infer_stance_logits(model, own10, team8, logits5)`. `export_weights.py` already
   writes these tensors, since it copies every key of `covenant.pt`/`marines.pt`.
2. **Loading through `port_mods`.**
   - Find the file with `port_mods_file_find(mendicant_config.weights, NULL)`. The default key is
     `"mendicant\\models.aibw"`: file `mods/<mod>/tags/mendicant/models.aibw`. A NULL map name
     means the mod must not use `maps =` in its `mod.toml`.
   - Read it with `port_mods_file_stamp` (size) and `port_mods_file_read` into a `malloc`'d buffer.
     These are declared for game units in `port/linux/game/mods.h:42-47`.
   - Call `mendicant_infer_load(..., mendicant_doctrine_hash())`. A doctrine mismatch, a bad CRC,
     a newer format or a missing file are logged with `platform_log` and leave the runtime off:
     the drill decides, exactly as without brain.py.
   - Reload when `port_mods_file_stamp` changes between maps.

**What `mendicant_runtime_answer(bridge)` does.** It reproduces `brain.py Bridge.serve` and
`policy.py` operation for operation:

1. **Read the request.**
   - `count = member_count`
   - `side_team = team_index`
   - covenant = `side_team == 3`
   - side 0 is Covenant, side 1 is Marines
2. **Strategy.** When `plan_request` is set and a strategy model exists for the side:
   - The input is `plan_features[16] | plan_extra[32] | plan_entities[32][12]` (432 floats), into
     `mendicant_infer_strategy_logits`.
   - The mask is `mendicant_infer_decision_mask(kind = plan_kind, step_mask, covenant)` (Python's
     `decision_mask`: a kind-0 mask with nothing allowed retries with mask 0; kind 1 falls back
     to "done").
   - Draw `u = splitmix64`, then `d = mendicant_infer_sample(logits, mask, 129, mendicant_config.variety, u)`.
     A non-finite distribution falls back to uniform over the allowed decisions (trainer.py:697).
   - If `d >= 96`, then `plan_step = d - 96` and the plan stays `(-1, 1, 0)`. Otherwise
     `plan_drill = d / 4`, `plan_side = (d & 1) ? 1 : -1`, `plan_tempo = (d >> 1) & 1`, and
     `plan_step = -1`.
   - Otherwise (no request or no model): `plan = (-1, 1, 0)` and `plan_step = -1`.
3. **Per-enemy model.** For the side's squad model:
   - When absent, `choice[0..count) = 0` and `stance[0..count) = 0`. brain.py leaves stale
     stances here; the port writes 0 (section 10.7).
   - When present, run `mendicant_infer_squad_scores(team, features, count, scores, &side)`
     (`Brain.decide`: side = +1 if the side logit is 0 or more, or `team[7] < 0.5`; the plan is
     `tanh(out[1:3]) * team[7]`; feature 0 is multiplied by the side). Then
     `mendicant_infer_assign(scores, candidate_count, positions, count, choice)`, which is
     `brain.assign`: best first, no position shared.
4. **Stance.** When `team[7] < 0.5` and a stance model exists: for each member,
   `own = features[m][0][0..10)` and logits come from the stance model. Draw
   `u = splitmix64` and pick `stance[m] = sample(logits, all allowed, 5, T = 1.0, u)`. brain.py
   uses `torch.multinomial` of the softmax, with no variety applied. Otherwise `stance[m] = 0`.
5. **Answer.** Memory barrier, then `response_sequence = request_sequence`.

**When it runs.** `mendicant_bridge_submit` calls it synchronously when there is no shared memory.
`bridge_poll` (M2) sees the answer at its next call (the next `drill_update`, the same or the next
tick) and applies it. `BRIDGE_TIMEOUT_TICKS` (30) and the drill fallback stay, for the
shared-memory path.

**RNG.** One splitmix64 state, seeded at each map by `ai.mendicant.seed`. When the seed is 0, it
mixes the map's name hash with the monotonic clock. It never touches the game's random seeds, so
netcode and `net_compare` replays are unchanged.

**Cost** (research_runtime.md):
- 0.19–0.31 ms per strategy decision on desktop
- an estimated 1–2 ms on an A53 phone
- at most about one decision per tick, so synchronous is affordable.

The arm64_32 checksum (01891103) still has to be confirmed on a device.

**Tests.** `make_golden.py` output goes into `tools/harness` (or a stagekit test). The
`mendicant_infer` test checks the checksum and the decisions on i686, Windows, wasm32 and the
arm64_32 emulator.

---

## 7. M2 command: command.inc, the doctrine, units, enrolment, discipline

These move from the hook library to `mendicant_command.c`. The parts of `ai_brain.c` that come
with `command.inc`:

| What | `ai_brain.c` lines |
|---|---|
| `team_mode`, `side_of`, `actor_can_drill`, `squad_target`, `signed_angle`, `clear_member_orders`, `set_role`, `member_positions`, `order` | 688-775 |
| `bridge_candidates`, `bridge_request`, `bridge_apply` (except the stance branch, which calls M4 `mendicant_apply_stance`), `bridge_poll` | 777-1058 |
| `drill_rand`, `unit_geometry`, `range_to` | 1072-1100 |
| `member_is_elite` | 1257 |
| plan memory: `plan_begin`, `plan_close`, `plan_outcome`, `weapon_class` (moves to M4 and is shared), `plan_extra`, `plan_features` | 1357-1540 |
| `actor_is_elite_leader`, `actor_distance_squared` | 1543-1555 |
| `drill_update` | 1624-1871 |
| discipline: `actor_is_disciplined`, `actor_may_charge`, `discipline_panic`, `discipline_berserk`, `discipline_charge`, `actor_action_change_to`, `discipline_lost_contact`, `discipline_before_decide` | 3670-3820 |

All of `command.inc` (1716 lines) and `doctrine_table.h` (unchanged) come too.

How they translate:

- **`FIELD` reads** become members (section 4). `struct squad_member` becomes
  `struct mendicant_member` (`struct actor_datum *actor`), and `char *actor` becomes
  `struct actor_datum *`.
- **The discipline bodies** become the source-hook bodies:
  - `mendicant_veto_panic` returns `actor_is_disciplined`, and counts the decline.
  - `mendicant_veto_berserk` returns `disciplined && !may_charge`, and counts the decline. The
    hook's `on` argument is always TRUE at the two sites.
  - `mendicant_action_charge_setup` declines goal 0, or goal 2 when the target is more than
    2.5 world units away or unknown. Otherwise it calls `action_charge_setup`.
  - `mendicant_handle_lost_contact` keeps the mod's branches by `state.action`, then calls
    `actor_action_handle_lost_contact`.
- **The bridge.** `bridge_request` writes the request into `mendicant_bridge` and calls
  `mendicant_bridge_submit()`. The training wait loop moves into M5 (shared memory only).
  `bridge->plan_request` is still set to 1 by `unit_update` (`command.inc:1361`, `:1575`) and to 0
  by `individuals_update` (M4).
- **The doctrine accessors for M1.** `mendicant_doctrine_hash` computes FNV-1a over
  `"<chain_name>:<covenant|marines>|..."` + `"#"` + `"step|..."`, with the chain names, sides and
  step names from `doctrine_table.h`. This is the same text as `export_weights.py doctrine_hash`.
- **The mod's constructor state** moves to `mendicant_command_reset`: `enrolled_actor` −1,
  `order_actor` −1, and `cmd[].chain` −1 per episode.
- **`ai.mendicant.command_radius`** replaces `command_radius`.

---

## 8. M3 movement: actor_go, maneuver slots, follow-the-Chief, cover scoring

**`mendicant_movement.c` (`actor_go.inc`, 941 lines):**
- `go_order`, the graph and the planner unchanged
- `go_search_from` builds a `struct path_state` (static, 82060 bytes) with
  `actor_path_input_new(actor, &state.input)`, sets `input.start_valid/start_point/start_surface_index`,
  `debug = NULL`, `destination_valid = TRUE`, `destination` (point, surface,
  `target_radius = 100000`) and `structure = global_structure_bsp`. It then calls
  `path_state_find` and `path_state_build_path(&state, &result)`, and uses `result.endpoint`.
- `go_ground` uses `collision_test_vector(0x21, …)` into a `struct collision_result`.
- `go_hold_encounter` / `go_release_encounters` write `encounter_datum.force_active` through
  `datum_try_and_get(encounter_data, …)`.
- `go_update` runs from `mendicant_movement_tick_begin`.
- the public API (`actor_go_to`, `actor_go_status`, `actor_go_cancel`) is unchanged.
- **Removed:** the `GO_STUB` asm, `go_detour`, `actor_go_install` (prologue checks),
  `__attribute__((visibility))`, and `GO_TEST_HOOK`.

**`mendicant_maneuver.c` (`maneuver.inc`, 275 lines):**
- the slot copies, `maneuver_swap` (bracketing the tick: L1 runs outside it), `maneuver_check_map`,
  the pool, `maneuver_extend` (group bit from `squad->firing_position_groups[8]`),
  `maneuver_slot`, `maneuver_position` and `maneuver_near`
- `mendicant_movement_dispose` restores `firing_positions.address/count` from `maneuver_own_block`
  and frees the copies

**`mendicant_positions.c`** takes these from `ai_brain.c`:

| What | Lines |
|---|---|
| `encounter_firing_positions`, `firing_position_owned` | 392-416 |
| `leashed_position`, `select_firing_position` (now the body of `mendicant_select_firing_position`) | 444-501 |
| `guard_change_position` (now the body of `mendicant_guard_position`) | 3880 |
| `within_reach`, cover (`collision_test_vector` 0x2b, `line_blocked`, `cover_score`, `cover_bonus`) | 1105-1173 |
| `drill_position`, `drill_hold`, `assign_position`, `go_fallback`, `drill_move` | 1178-1340 |
| the move test (`move_test*`, `AI_BRAIN_MOVE_TEST`) | 567-660 |

**Follow the Chief** comes from `ai_brain.c:2667-2772` (`marines_follow_chief`, `follow_hop`):
- "the Chief" is `mendicant_player_unit()`, absolute player 0's unit
- `mission_step_ground` (2642) moves here as `follow_step_ground`, because follow needs it. The
  rest of `mission_*` is left out.
- `follow_hop`'s ordered slot path uses `mendicant_order` (M2) and
  `actor_change_firing_position`/`actor_move_to_point`

**The `go_intercept` body** becomes `mendicant_go_redirect` (H15 above).

---

## 9. M4 combat: grenades, aim, targets, vehicles, individuals, roles

**From `ai_brain.c`:**

| What | Lines | Notes |
|---|---|---|
| `grenade_clear` | 3382-3420 | |
| `grenade_lands_on_friends` | 3437-3460 | The body of `mendicant_grenade_throw_checked` |
| `plan_grenade_trajectory` | 3478 | Section 3 call |
| `grenade_ask`, `grenade_wait` | 3493-3524 | `actor_action_test_grenade` extern, then `actor_action_try_to_throw_grenade(actor, TRUE)` |
| `apply_role` | 3526-3633 | |
| aim: `aim_factor`, `aim_actor`, `carries_*`, `actor_slot_of` | 3953-4125 | `actor_combat_get_firing_variant_definition`, the variant members in section 4 |
| targeting: `target_weight`, `prop_actor_type` | 4128-4222 | |
| `actor_try_to_enter_vehicle`, `empty_vehicle_near` | 4249-4310 | |
| `vehicle_try` | 4327-4404 | |
| `individuals_update`, with `stance_roles` | 1565-1622 | |

**From `vehicles.inc`** (372 lines): all of it.
- `game_call4` becomes the four direct calls in section 3.
- The `follow_going`/`actor_go_cancel` uses (`vehicles.inc:287, 300`) become
  `mendicant_follow_cancel` (M3).

**Knobs:**
- `aim_scale`, `aim_by_difficulty`, `aim_by_movement`, `aim_debug` (logs through `platform_log`)
- `marine_fire` (at least 0.1)
- `targeting`
- `individuals_on`

These come from `mendicant_config`.

---

## 10. M5 platform: config, bridge, training, logs, lifecycle, build

### 10.1 Config keys

These go in `port/linux/src/port_config.c`'s `config_settings[]`, documented in
`port/linux/README.md`'s settings table.

**How a three-part name fits.** `ai.mendicant.x` is written into `config.toml` as `[ai]` /
`mendicant.x = …`, which is a valid TOML dotted key (`config_append_setting` writes `dot + 1`).
`toml_seek(table, "ai.mendicant.x")` finds it. But `config_report_unknown_keys`
(`port_config.c:1857-1879`) only looks two levels deep, and would report `ai.mendicant` as
unknown. Teach it to descend one more level when a section's value is itself a table, and check
`config_setting_index("<section>.<table>.<key>")`.

| AI_BRAIN_* (hook library) | Config key (type, default, platforms) | Environment variable |
|---|---|---|
| `AI_BRAIN_DRILL` (and `TRAINING` implies it) | `ai.mendicant.enabled` (boolean, `false`, all) | `HALO_AI_MENDICANT` |
| (brain.py present or not) | `ai.mendicant.models` (boolean, `true`, all): use `models.aibw` when found | `HALO_AI_MENDICANT_MODELS` |
| `play.sh` `COVENANT_WEIGHTS`, `MARINE_WEIGHTS`, `COVENANT_STRATEGY`, `MARINE_STRATEGY`, `FROZEN` | `ai.mendicant.weights` (string, `"mendicant\\models.aibw"`, all): the mod file. The weights themselves are bundled by `export_weights.py`. | `HALO_AI_MENDICANT_WEIGHTS` |
| `AI_BRAIN_VARIETY` (brain.py) | `ai.mendicant.variety` (real, `1.0`, all) | `HALO_AI_MENDICANT_VARIETY` |
| (torch RNG) | `ai.mendicant.seed` (integer, `0` = from the clock, all) | `HALO_AI_MENDICANT_SEED` |
| `AI_BRAIN_INDIVIDUALS` | `ai.mendicant.individuals` (boolean, `true`, all). The hook defaulted to off, play.sh to on (section 10.7). | `HALO_AI_MENDICANT_INDIVIDUALS` |
| `AI_BRAIN_DISCIPLINE` | `ai.mendicant.discipline` (boolean, `true`, all) | `HALO_AI_MENDICANT_DISCIPLINE` |
| `AI_BRAIN_TARGETING` | `ai.mendicant.targeting` (boolean, `true`, all) | `HALO_AI_MENDICANT_TARGETING` |
| `AI_BRAIN_AIM` | `ai.mendicant.aim` (real, `1.0`, all) | `HALO_AI_MENDICANT_AIM` |
| `AI_BRAIN_AIM_DIFFICULTY` | `ai.mendicant.aim_difficulty` (boolean, `true`, all) | `HALO_AI_MENDICANT_AIM_DIFFICULTY` |
| `AI_BRAIN_AIM_MOVING` | `ai.mendicant.aim_moving` (boolean, `true`, all) | `HALO_AI_MENDICANT_AIM_MOVING` |
| `AI_BRAIN_MARINE_FIRE` | `ai.mendicant.marine_fire` (real, `1.8`, all) | `HALO_AI_MENDICANT_MARINE_FIRE` |
| `AI_BRAIN_COMMAND_RADIUS` | `ai.mendicant.command_radius` (real, `35.0`, all) | `HALO_AI_MENDICANT_COMMAND_RADIUS` |
| `AI_BRAIN_BRIDGE` | `ai.mendicant.bridge` (string, `""` = in-process models, desktop) | `HALO_AI_MENDICANT_BRIDGE` |
| `AI_BRAIN_TRAINING` | `ai.mendicant.training` (boolean, `false`, desktop) | `HALO_AI_MENDICANT_TRAINING` |
| `AI_BRAIN_TRAINING_SKIP` | `ai.mendicant.training_skip` (integer, `7`: bit 1 frame, 2 sound, 4 mixer; desktop) | `HALO_AI_MENDICANT_TRAINING_SKIP` |
| `AI_BRAIN_LOG` | `ai.mendicant.sample_log` (string, `""` = none, desktop) | `HALO_AI_MENDICANT_SAMPLE_LOG` |
| `AI_BRAIN_DRILL_LOG` | `ai.mendicant.drill_log` (string, `""`, desktop) | `HALO_AI_MENDICANT_DRILL_LOG` |
| `AI_BRAIN_PLAYER_LOG` | `ai.mendicant.player_log` (string, `""`, desktop) | `HALO_AI_MENDICANT_PLAYER_LOG` |
| `AI_BRAIN_MOVE_TEST` | `ai.mendicant.move_test_log` (string, `""`, desktop) | `HALO_AI_MENDICANT_MOVE_TEST_LOG` |
| `AI_BRAIN_VEHICLE_TEST` | `ai.mendicant.vehicle_test` (boolean, `false`, desktop) | `HALO_AI_MENDICANT_VEHICLE_TEST` |
| `AI_BRAIN_AIM_DEBUG` | `ai.mendicant.aim_debug` (boolean, `false`, desktop) | `HALO_AI_MENDICANT_AIM_DEBUG` |
| `AI_BRAIN_CHIEF`, `AI_BRAIN_CHIEF_MODEL`, `AI_BRAIN_CHIEF_DEBUG`, `OBSERVER_CAMERA`, `OBSERVER_SCRIPTED`, `AI_BRAIN_LIBRARY`, `AI_BRAIN_RUNS`, `AI_BRAIN_UNIT_LOGS` | **none.** The AI Chief and theater are left out. `LIBRARY`, `RUNS` and `UNIT_LOGS` are trainer-side. | |
| play.sh `DEATHLESS`, `AMMO`, `CHEATS`, `DRILLS`, `HALO_DIR` | **none.** These stay script-side: console cheats, `watch_drills.py`, the install folder. | |

**Training continuity (desktop, optional, M5).** When a `HALO_AI_MENDICANT_*` variable is unset,
`mendicant.c` also reads its legacy `AI_BRAIN_*` name. With that, `trainer.py` and `play.sh` run
the fork with no change other than the binary's path. Their `LD_PRELOAD` of the old `.so` is
harmless: its build-id check refuses the fork's binary and it prints "unknown game build, not
hooking".

**Menus.** Every key is a boolean or a small set of reals. That leaves it ready for a MENDICANT
AI screen in `tools/port_settings.py` (`setting=`, `values=`), regenerated with
`tools/ce_menus.py`, never edited by hand.

### 10.2 Training bridge

Desktop Linux only. It lives in `mendicant_bridge.c` and `mendicant_bridge.h`.

**The layout.** `struct model_bridge` is copied field for field from `ai_brain.c:157-262`: magic
0x41494231, 34948 bytes. It must be wrapped in `#pragma pack(push, 4)` / `#pragma pack(pop)`.
The game units compile with `-malign-double`, which moves `step_mask` (`uint64_t`) from offset
33316 to 33320 and shifts every field after it. This was checked by compiling the struct with
the game's flags: 5 asserts fail without the pack and none fail with it. Static-assert in the
header:

| Field | Offset |
|---|---|
| `team` | 24 |
| `candidate_count` | 56 |
| `choice` | 120 |
| `features` | 184 |
| `positions` | 30904 |
| `team_index` | 32440 |
| `team_mode` | 32444 |
| `seed` | 32452 |
| `plan_features` | 32572 |
| `plan_request` | 32636 |
| `plan_extra` | 32664 |
| `chief_request_sequence` | 32792 |
| `scenario_mode` | 32960 |
| `chains_started` | 33288 |
| `plan_kind` | 33312 |
| `step_mask` | 33316 |
| `plan_step` | 33324 |
| `discipline_declined` | 33328 |
| `plan_entities` | 33344 |
| `enemy_step_label` | 34880 |
| `stance` | 34884 |
| `sizeof` | 34948 |

These are the offsets `trainer.py:43-66` and `brain.py:15-31` use.

**Opening it.** `ai.mendicant.bridge` is a path. `port/linux/src/posix_mendicant.c` (POSIX flags)
provides `void *posix_mendicant_bridge_map(char const *path, unsigned long size)`: open with
`O_RDWR|O_CREAT, 0600`, `ftruncate`, then `mmap MAP_SHARED`. On success: `magic` is set, and
`episode_ticks` defaults to 6000. `chief_wanted` is left as the trainer wrote it and never acted
on.

**The training wait.** It moves from `bridge_request` into `mendicant_bridge_submit`: in training,
spin for up to 200 ms with `posix_mendicant_yield()` and `posix_mendicant_monotonic_ns()` until
`response_sequence == request_sequence`.

**With no path.** `mendicant_bridge` points at a static `struct model_bridge` owned by M5, and
`mendicant_bridge_submit` calls `mendicant_runtime_answer` (M1). With no path and no weights,
`mendicant_bridge = NULL`. In that case `mendicant_team_mode` gives Covenant `MODE_DRILL` and
Marines `MODE_GAME`, which is the hook library's no-brain.py behaviour.

**Excluding it from other builds.** `posix_mendicant.c` goes in `WEB_EXCLUDE`
(`tools/web_build.py:127`) and in `replaced_platform_sources` (`port/windows/port.json:3`).
Android skips `posix_*` already (`tools/android_build.py:485`). In game code, every call is under
`#if !defined(HALO_WINDOWS) && !defined(HALO_ANDROID) && !defined(HALO_WEB)`.

### 10.3 Training, scenario director, deaths, difficulty

`mendicant_training.c`, from `ai_brain.c`:

| What | Lines | Changes |
|---|---|---|
| `episode_start` | 2980-3024 | Its command part goes to `mendicant_command_episode_reset(seed)`. The Chief fields are dropped. The seed XORs use `get_global_random_seed_address()` / `get_global_local_random_seed_address()`. |
| `episode_end`, with deaths by vitality (`actors_alive`: the body's `object.body_vitality > 0`, else sampled within 300 ticks) | 3026-3062 | Calls the M2 and M3 report functions, then `mendicant_orders_clear_all()` |
| `game_speed_set` | 3079 | Becomes `game_time_set_speed` |
| `scale_vitality` | 3086 | |
| `scenario_actors`: knobs on first sight, living counts, strays, wipe end | 3099-3160 | The `chief_unit` test is dropped; `player_unit()` stays |
| `apply_difficulty` | 3183 | Becomes `game_difficulty_level_set` + `global_difficulty_level` |
| `scenario_update` | 3193-3235 | `chief_update` and `scenario_chief` dropped |
| `training_update` | 3237-3257 | `chief_update` dropped |

`training_frame` becomes `mendicant_skip_render_frame`. It returns TRUE when training and
`training_skip & 1`, and sleeps 1 ms when the episode is over.

`mendicant_skip_sound_render` returns TRUE when training and `training_skip & 2`.

**What training loses without the AI Chief** (the trainer is unchanged):

1. **Whole-level episodes** (`scenario_mode` 0, `levels played with the AI Master Chief`). The
   level only advances as the Chief walks it and its triggers fire. Without him, an episode is
   the fight at the level's start until `episode_ticks`.
2. **Scenario jobs with `chief.present`.** `chief_start`, `chief_hold`, `chief_vitality_scale`,
   `chief_style`, and the Chief's reports (`chief_actor`, `chief_position`, `chief_vitality`,
   `chief_state`) all stay at 0 / −1. The fight runs as a no-Chief scenario, and the player's
   biped stays where the level spawned it. The trainer still sends `cheat_deathless_player` for
   these, so it cannot die.
3. **The Chief model's training data**: `chief_request_sequence`, `chief_features`,
   `chief_intent`, the outcome fields.
4. **Marines following a moving Chief.** With no AI Chief, `marines_follow_chief` follows the idle
   player biped, as the hook library does today in no-Chief scenarios.

Scenario fights without a Chief (`chief.present = false`) are unaffected.

### 10.4 Logs

Desktop only: `fopen` of the configured path, a header line, and `fflush` after every line, as
today.

| Log | Source | Notes |
|---|---|---|
| sample log | `sample_actor`, `ai_brain.c:327-372` | Off unless `ai.mendicant.sample_log` is set. The hook library wrote `ai_brain_samples.csv` in the working directory, and refused to hook at all if it could not (section 10.7). |
| player log | `player_sample`, 3310-3362 | |
| drill log | `drill_file` | |
| move test log | | |
| `stderr` messages | | Become `platform_log("mendicant: …")` |

### 10.5 Lifecycle

| When | What happens |
|---|---|
| L1, `ai_initialize_for_new_map` end | Config read; `mendicant_active` set. Each module's reset runs: the constructor's `memset`s of `enrolled_actor`, `grenade_wait_since` and `order_actor`, the go orders, the graph and the maneuver state. M1 loads (or keeps) its weights. The bridge and logs open on the first map only. |
| `mendicant_game_tick_begin` | Rechecks `mendicant_active` (a co-op join can make this machine a distributed client) |
| L2, `ai_dispose_from_old_map` start | M3 dispose (lists restored, blocks freed). go orders are released and any `force_active` the mod set is cleared. |
| Episode resets in training | Unchanged: the clock going backwards (`training_update`) |
| Process exit | Nothing |

### 10.6 Build integration

`port/linux/game` is globbed **without recursion** for the port's game units, so
`port/linux/game/mendicant/*.c` would not be compiled. Each build script's loop must be extended
to `glob("*.c")` plus `glob("mendicant/*.c")`, with the same `game_cflags`:

| Script | Line |
|---|---|
| `tools/linux_build.py` | 587, `for source in sorted(Path(config["game_sources"]).glob("*.c"))` |
| `tools/windows_build.py` | 424 |
| `tools/android_build.py` | 470 |
| `tools/web_build.py` | 250 |

Only the game sources under `source/` use `rglob` (`linux_build.py:36`).

**Includes.**
- `-iquote port/linux/game` (already passed to every game unit) makes `"mendicant/mendicant.h"`
  work from `source/`. Inside the folder, quotes find siblings.
- Port headers: `#include "../mods.h"` for `port_mods_file_*`. Declare
  `long config_integer(char const *name);` and the other config getters, and
  `void platform_log(char const *format, ...);`, locally as `bots.c:111-112` does, or include
  `"../../src/port_config.h"`.

**New platform file.** `port/linux/src/posix_mendicant.c` is picked up by `linux_build.py:614`'s
`posix_` branch (POSIX flags).

**Flags.** The new units are not in `pgo/halo_linux.profdata`. That is harmless
(`-Wno-profile-instr-unprofiled`) until the profile is regenerated.

**Checks to run after any change:** `ninja linux`, `ninja android`, `ninja web`.
- Web: externs must match their callees exactly, since the wasm link fails otherwise. The
  `actions.c` externs (`actor_action_test_grenade`, `actor_action_try_to_throw_grenade`,
  `actor_action_handle_lost_contact`), `global_structure_bsp`, `global_difficulty_level` and the 12
  `actor_type_*` must be declared exactly as in their source files.
- Android: build the guest.
- Windows: cross-build with `configure.py --windows-cross`, when available.

**Docs.**
- `port/linux/README.md`: the settings rows, and a row in "Game source changes" (line 1802)
  listing the 11 `source/` files and why.
- The web README's matching list (line 1890 points to it).
- `docs/mods.md`: a short note that a mod folder may carry `mendicant/models.aibw`.

### 10.7 Deliberate differences from the hook library

1. **No default sample log.** Nothing is written unless `ai.mendicant.sample_log` is set; the
   hook library wrote `ai_brain_samples.csv` outside training by default. Play is unaffected
   (`play.sh` sets `AI_BRAIN_LOG`, which the port reads as `ai.mendicant.sample_log`).
2. **In-process answers are synchronous.** The models answer inside `mendicant_bridge_submit`,
   and the answer is applied at the next `drill_update`; brain.py answered one or more ticks
   later. So the drill log's latency figures of a game with the models in it are 0 and are not
   comparable with the hook build's; reports should say which was measured. The shared-memory
   path keeps the asynchronous behaviour and the 30-tick timeout.
3. **Angle maths.** `atan2f`, `cosf` and `sinf` become the musl double versions cast to `real`,
   which may differ by one ulp. `sqrt` and `fabs` are exact.
4. **State resets at map dispose.** The hook library noticed a new map only when the scenario
   pointer changed, so a reload of the same map at the same address kept slot state.
5. **Selection candidate zeroed.** For an ordered actor, the selection's `candidate` is zeroed;
   the binary left it uninitialized.
6. **Where the AI runs.** It is active only on the campaign or co-op host, not under a
   multiplayer game engine and not on a distributed client. Multiplayer maps carry no actors.
7. **`ai.mendicant.individuals` defaults to on** (play.sh's default), against the hook library's
   off.
8. **Stances when a side's per-enemy model is missing** are written as 0; brain.py left them
   stale. Sampling uses the port's seeded splitmix64 in place of torch's RNG.
9. **No unit starts with a last drill.** At each map's start (and a checkpoint revert) every
   unit's last drill and plan memory are -1 and its chain -1, as a training episode starts them;
   the hook library's 0 told the strategic model every unit's last plans were drill 0.
10. **An actor of an unlisted type is not sampled.** The hook library never wrapped one; the
    port would have logged it as type 12.
11. **Training as the hook trained.** With training on, `ai.mendicant.individuals` is off unless
    `HALO_AI_MENDICANT_INDIVIDUALS` or `AI_BRAIN_INDIVIDUALS` is set (`trainer.py --mode
    individual`).
12. **Saved games and checkpoints.** Before a save the encounters the go orders hold
    `force_active` are let go (they are held again at the next tick); after a load no actor keeps
    a firing position index beyond the level's own lists, AI on or off, and with the AI on its
    orders, units, timers, go orders and following start over (the maneuver copies stay). The hook
    library kept all of it across a revert.
13. **The editor's map restart** (`ai_handle_editing`, which starts a map without ending it)
    puts the level's own firing position lists back before the copies are freed.
14. **The mixer's skip** (`training_skip` 4) follows the frame's and the sound's: training on with
    the trainer's bridge open (`mendicant_skip_mixer`), not the settings alone.

---

## 11. Order of work and checks

1. **Skeleton.** M5 skeleton with `mendicant.h` and every hook returning stock, plus the build
   globs. Check: build linux, android and web, and run a stagekit/stock-parity run with the AI
   off. Nothing should change.
2. **Command and movement.** M2 and M3 with the drill alone (`models = false`).
3. **Combat.** M4.
4. **Runtime.** M1, checked against `make_golden.py`.
5. **Bridge.** Point `trainer.py` at `build/linux/halo` through the legacy environment names, and
   run headless only:
   - `HALO_NULL_RENDERER=1 HALO_HIDDEN_WINDOW=1 HALO_NO_AUDIO=1`
   - console ports 26090-26099

   Compare every drill and strategy measure against the hook build's previous best, noting
   whether higher or lower is better for each.

Keep the offsets static-assert block (section 4) and the bridge-layout asserts (10.2) in the
code, so a struct change upstream fails the build instead of the game.
