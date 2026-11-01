# Optional bots and Android controls

These are GulchCE additions. Their defaults preserve OpenCE's local Slayer
bots and Android's saved touch layout. Xbox, Custom Edition and MCC keep
their existing map loaders, limits, rules and resource ownership.

## What players can enable

- `bots.enhanced = true` adds CTF and King of the Hill goals, team roles,
  precision-weapon vantage choices and powerup selection. It is read at map
  load. Slayer combat, spawning, pathfinding and unstick behavior remain
  owned by the existing bot implementation.
- `bots.routes` selects `direct` (default), `lanes` or `mixed`. A lane is an
  alternate route through two intermediate destinations. Unreachable lanes
  are abandoned for that bot's current life.
- `bots.thread = true` moves enhanced bots' graph searches to a bounded
  background worker. **Path Worker** in Bot settings controls it; it is off
  by default and takes effect on the next map. Collision queries and action
  decisions still run on the game thread.
- `bots.vehicles = true` lets enhanced bots seek ground-vehicle seats and
  turrets. They use ordinary player controls to enter, steer and leave.
  Aircraft navigation is not implemented. Switching this option off asks
  seated bots to leave; disabling enhanced bots takes effect on the next map.
- `bots.network = true` permits bots on a network host. Set it before
  creating the host or apply it with **OK** in the lobby's Bots settings;
  `bots.count`, `bots.skill` and `bots.teams` still apply.
  Up to 63 bots join, subject to the lobby's player limit (63 bots plus one
  human need at least 64 player slots). The default count remains zero.
- In the CE menus, **Settings > Network > Bots** configures count, network
  admission, difficulty, teams, enhanced tactics, vehicles and routes.
  **Create Game** and the host's lobby also have a **Bots** button. During
  a match, open **Settings > Network > Bots > Manage Current Bots** to add
  or remove one bot on either team, or let Any Team balance a team game.
  Live changes do not change the saved lobby count. Clients cannot manage
  the host's bots. Xbox menu artwork and navigation remain unchanged.
- Android's **Settings > Gamepad > Android Controls > Gyro Aim** offers
  **Touch Settings**, **Off**, **On** and **Zoom Only**. Touch Settings is
  the default and preserves the layout's saved gyro switch and sensitivity.
  Explicit modes use independent horizontal/vertical sensitivity and invert
  settings. Zoom Only discards motion while unzoomed.
- Touch Options can be moved in the layout editor; Export, Import and Save
  sit in its center. General settings can disable aiming with held buttons
  and choose a three- or four-finger tap for enabled cheats. Older
  layouts retain their saved controls and gyro behavior when loaded.
  OpenCE version-4 layouts omit GulchCE's `menu-fingers` field; importing
  those files uses the existing three-finger cheat gesture default.
- Android's **Cheat Menu** option adds a **CHEATS** touch button beside
  **Hide/Touch**, as well as **Cheats** in touch **Options**. The button is
  available during offline or hosted gameplay, including when the gameplay controls
  are hidden; the layout editor and cinematics hide it. Tapping opens the
  existing dialog through `openDialog`, which releases held controls.
  It defaults off and is unavailable to joining clients, including while joining
  a lobby. Android hosts use the same cheat APIs as desktop hosts. It exposes
  invincibility, ammunition, bottomless clip, super jump, omnipotence,
  no fall damage, Medusa, bump possession, Easy Rider, weapon/powerup/vehicle
  spawning and camouflage through existing engine cheat APIs. Easy Rider
  disables rider ejection; it does not change the saved game's defaults.

For example, set `bots.count = 6`, `bots.enhanced = true`,
`bots.network = true` and `bots.routes = "mixed"`, then create a CTF host.
Six bots join as ordinary players if the lobby has space. The host decides
their goals and publishes their controls. A client needs the same map and a
compatible OpenCE protocol, but does not run the new bot AI. MCC sessions
still require GulchCE's existing MCC support and negotiation.

The complete settings and environment variables are in
[the port settings table](../../port/linux/README.md) and
[the Android guide](../../port/android/README.md).

## Read the bot flow

1. [bots_network.c](../../port/linux/game/bots_network.c):
   `bots_network_session_begin` initializes admission policy;
   `bots_player_ownership` records synthetic players actually created by
   this host. A high machine number alone never grants bot ownership.
   `bots_network_lobby_enable` changes admission only after the bot management
   module checks the pregame state and unused machine IDs. Disabling it
   removes lobby bots but retains the reservation until session disposal.
2. [network_server_manager.c](../../source/networking/network_server_manager.c):
   `network_game_server_port_bots` retains the existing lobby, team and
   capacity rules. An opted-in host reserves the final 63 machine IDs;
   ordinary hosts retain all human machine slots.
   `network_game_server_update_countdown` also asks `bots_lobby_can_start`
   in the bot module whether one human machine has owned bots. Those bots
   have synthetic machine IDs but no connections, so the ordinary
   **START NOW** request can start play without another computer joining.
   Upstream now starts immediately for the host; a client request only
   shortens the countdown. This hook only extends the inactive-countdown start gate;
   existing player minimums, team requirements and countdown pauses still
   apply. It adds no session state or network messages and applies equally
   to Xbox, Custom Edition and MCC lobbies. High-ID human players never
   acquire this permission merely from their IDs.
   [bots_management.inl](../../port/linux/game/bots_management.inl)'s
   `bots_lobby_settings_apply` is called by the Bots menu's OK event after
   saving choices. It checks connections, including pending joins, and
   unowned high-ID players before enabling bots in an existing network lobby.
   Conflicts produce an actionable error without stealing a player's ID.
   It runs the existing lobby reconciliation immediately; clients and hosts
   already loading or playing only save their choices for a future host.
   `bots_manage` uses that server's existing join/quit dispatch for live
   changes. Admission assigns the authoritative slot before encoding;
   failed encoding rolls back the unannounced record. A departing bot's
   machine identity stays unavailable until after the scheduled quit tick.
3. [bots.c](../../port/linux/game/bots.c): `bots_update_actions` runs only
   on the host. Upstream `bots_combat.c` supplies six skill levels: Recruit,
   Easy, Normal, Heroic, Veteran and Legendary. `bots_level_may_fire`,
   `bots_level_melee` and `bots_level_grenade` apply their combat rules.
   GulchCE retains Infection zombies' no-gun/no-grenade rule and close-range
   melee cadence; enhanced navigation, vehicles and objectives stay optional.
   `bots_choose_goal` asks the enhanced module for urgent
   objectives before the inherited combat/item/roaming decisions.
4. [enhanced_bots.c](../../port/linux/game/enhanced_bots.c):
   Its enhanced aiming, healing, powerup reach and retreat thresholds use
   the same six-level enum as upstream combat. Easy, Normal, Heroic and
   Legendary retain their original enhanced values. Recruit shares Easy's
   enhanced table row while upstream combat supplies its weaker reactions
   and aiming; Veteran interpolates Heroic and Legendary. This prevents an
   inserted skill from silently selecting the next difficulty's tactics.
   When enhanced bots are enabled, their head/SWAT/feet selection owns the
   aim-point override; the base automatic head selection must not bypass
   its probabilities. Other upstream combat rules still run, including
   reaction, shot pacing, grenade decisions and skill-dependent movement.
   `enhanced_bots_update` caches objective state once per tick;
   `enhanced_bots_goal` selects flag, carrier, hill or vantage destinations.
   A flag carrier keeps advancing while fighting. A bot on the hill holds
   its position; an off-hill bot advances toward it. With OpenCE's Multi-Hill
   enabled, these existing bot queries target the first active hill, whose
   state `king_hills_to_globals` exposes through the original accessors.
5. [bots_roles.c](../../port/linux/game/bots_roles.c),
   [bots_routes.c](../../port/linux/game/bots_routes.c) and
   [bots_vantage.c](../../port/linux/game/bots_vantage.c):
   `bots_tactics_update` advances bounded graph preparation;
   `bots_routes_search_goal` supplies lane waypoints to the original path
   search. A vantage point is a position overlooking useful map locations;
   `bots_tactics_goal` chooses one suited to a bot's weapon and role.
6. [bots_vehicles.c](../../port/linux/game/bots_vehicles.c):
   `bots_vehicle_goal` reserves reachable seats and
   `bots_vehicle_action` supplies ordinary action/steering inputs. The
   engine checks reach, occupancy, hostility and attachment itself.
7. [player_queues_new.c](../../source/game/player_queues_new.c):
   `update_server_note_bot_action` puts the host's chosen input in the
   existing update for the tick it executed. Clients replay the same
   player actions through the existing simulation.

The ~7,000-line fork bot core is not imported. The focused objective module
extends the existing core; it does not duplicate combat, respawning,
path search or collision handling. Route/vantage preparation retains
bounded working memory (32,768 walkable nodes and a 262,144-entry surface
lookup at most) and releases it when finished. The optional worker only
traverses private graph snapshots; bot decisions stay on the host's game
thread. No network packet, deterministic simulation rule or protocol version
is added. This bot extension does not add to the current upstream Xbox/CE protocol. See
[NETCODE.md](../../port/linux/NETCODE.md) for authority and transport rules.

Map initialization resets objective, precision-cache and vehicle state.
Map disposal frees graph/grid allocations, including partial preparation.
Respawn releases seat reservations and vantage claims. Session disposal
clears bot ownership and the network reservation. Map-loading and MCC
cleanup paths remain unchanged.

`tools/harness/tests/test_lobby_countdown.py` exercises the real countdown
and bot ownership functions with 1, 31 and 63 bots, split-screen humans,
existing countdowns, team/minimum/pause checks and session ownership reset.
It also checks that removing the bot start hook reproduces the failure.
These asset-free checks do not establish map-loading or gameplay compatibility.

[bots_menu.inl](../../port/linux/game/bots_menu.inl)'s `bots_menu_event`
connects generated CE widgets to those host operations and displays their
result. Settings retain the existing save, cancel and defaults behavior.
The console offers `add_bot`, `add_bot_red`, `add_bot_blue`, `drop_bot`,
`drop_bot_red` and `drop_bot_blue` through the same permission checks.
The console hook follows the dedicated server's role check. Neither path
executes arbitrary console strings from a widget or another machine.

## Read the Android flow

[TouchControls.java](../../port/android/app/src/main/java/com/halo/decomp/TouchControls.java)
retains its existing sensor lifecycle and
[GyroscopeAim.java](../../port/android/app/src/main/java/com/halo/decomp/GyroscopeAim.java)
integration. Explicit gyro modes bypass swipe sensitivity, reset accumulated
motion on mode changes and leave legacy mode behavior intact.
[host_touch.c](../../port/android/host/host_touch.c) transfers scalar mode
and motion values under its existing mutex.
[touch_gyro.c](../../port/linux/src/touch_gyro.c) applies per-axis settings;
`halo_linux_gyro_filter` discards unzoomed motion in Zoom Only mode.

[AndroidCheatMenu.java](../../port/android/app/src/main/java/com/halo/decomp/AndroidCheatMenu.java)
uses the touch overlay's existing dialog/input-reset path. It sends typed
IDs through [host_android_cheats.c](../../port/android/host/host_android_cheats.c),
an eight-entry mutex-protected queue. No console text or guest pointers cross
this boundary. [android_cheats.c](../../port/linux/game/android_cheats.c)
rechecks offline/host permission on the game thread and consumes at most four
requests per poll. It follows the desktop console's
`hs_playing_in_anothers_game` rule: both distributed clients and clients
joining a lobby without a local server are denied. Changing from host to
client clears queued requests and releases menu-owned switches. Existing
host cheat semantics and object replication are reused; this menu adds no
network messages or client-side cheat settings.

Both the desktop console and Android menu reach `cheat_all_weapons` in
[cheats.c](../../source/game/cheats.c). Its map-supplied weapon list, or its fallback
enumeration of loaded weapon tags, can include CTF flags and Oddball balls. These
objective items need ownership assigned by the game mode. The generic cheat spawn
has no such owner, so `distributed_client_change_valid` in
[network_objects.c](../../port/linux/game/network_objects.c) rejects them. The host
could still pick them up, leaving clients unable to reproduce its inventory.

`cheat_objects` now skips weapons with the objective flag (the same bit used by
`weapon_is_flag`) when the game is a network host. This applies to both weapon-list
paths and both cheat interfaces. For example, spawning all weapons in Blood Gulch
creates ordinary weapons without adding unowned flags or balls. Offline spawning,
powerups, vehicles and the game modes' own objective creation retain their paths.
This is a shared cheat-spawn correction, not an MCC adapter or a change to client
validation. It adds no state, messages or peer agreement requirement; the
current upstream and separate MCC protocols are described in NETCODE.md.

Host access does not establish that every cheat synchronizes correctly; physical
Android touch and Android-to-desktop network testing remain separate checks.

Cinematics suspend requests without losing owned toggle
values. Disabling the menu or leaving the map restores values it changed,
unless a later console/script edit changed them again. One-shot spawned
items remain ordinary game objects; they are not undone by closing the UI.

## Why the shared hooks are needed

- Optional background traversal needs the path module's private graph
  routines without its live profiling/debug writes. `bots_path_find_snapshot`
  is an additive entry point in `source/ai/path.c`; the original
  `path_state_find` is unchanged. Only `bots_worker.inl` calls it, with private
  walkability and a snapshot of broken surfaces. This one hook cannot live
  solely in the feature module because traversal is private to `path.c`.
  Xbox, CE and MCC keep their existing graph loading and ownership paths.
- CTF flag/carrier and King hill geometry are private to their engine
  modules. The `game_engine_ctf_flag_state`, `game_engine_king_hill` and
  `game_engine_king_point_on_hill` accessors expose read-only state to
  feature code; they do not change scoring or game rules.
- The lobby owns player admission and machine IDs, and the tick queue owns
  authoritative input publication. Feature code cannot safely replace
  either ownership boundary. Their small hooks call the feature modules.
- Live bot management needs the server's private record and message types;
  its feature-owned include runs there without exposing those internals as
  a new public engine API. The script-console hook only recognizes the six
  typed bot commands; menus call the same feature function directly.
- `player_control` knows the actual zoom state when it consumes input;
  the gyro hook needs that state to discard unzoomed motion.
- `cheats_dispose_from_old_map` is the map boundary at which menu-owned
  switches and pending Android requests must be cleared.

The stress tests also exposed a glow marker assertion. In
[glow.c](../../source/objects/widgets/glow.c), `glow_particle_update` and
`glow_time_wrap_down` accept `t == total_time`, but the original first lookup
in `get_particle_world_position` excluded that endpoint. Its narrow fix
includes the last endpoint while retaining half-open interior intervals and
out-of-range assertions. This corrects an existing engine contract for all
map families; it adds no MCC conversion semantics or gameplay rules. The
failing run did not capture the particle value, so the endpoint defect was
established independently by an extracted-source regression and negative
control rather than inferred solely from that assertion.

All additions in `source/` carry `port:` markers. General-feature logic
stays in feature modules. No MCC compatibility logic was moved into the
Xbox or Custom Edition implementations.

## Credits and source provenance

The reference fork is
[thelinkin3000/halo-ce-universal at d92cc933](https://github.com/thelinkin3000/halo-ce-universal/tree/d92cc93385edf03d0a4a21cfe4c58dd578a3e786).
Its current project licence is AGPL-3.0-or-later; inherited code retains its
original notices and licences. Selected adaptations are distinguished from
design references below; this is not a wholesale cherry-pick.

| Feature | Authors and reference |
| --- | --- |
| Existing local bots | **BirchWoodGod**, OpenCE commit `4a275b60`; retained as the base implementation. |
| Routes, roles, vantage points and vehicle reference | **BirchWoodGod**'s [halo-ce-vita](https://github.com/BirchWoodGod/halo-ce-vita), adapted by **thelinkin3000** in [88df94fd](https://github.com/thelinkin3000/halo-ce-universal/commit/88df94fd9fb7dc76b3762ef28bfd5d624fe4bba5). The retained tactical code and derived portions keep GPL-3.0-only; its full text ships in [LICENSES/GPL-3.0-only.txt](../../LICENSES/GPL-3.0-only.txt). Vehicle controls are a focused adaptation using this engine's public seat/action interfaces. |
| CTF and hill decisions | **thelinkin3000**, [ef77e2b4](https://github.com/thelinkin3000/halo-ce-universal/commit/ef77e2b4d73fbd81622279ae7cd1f5e5d40771c2) and [91871681](https://github.com/thelinkin3000/halo-ce-universal/commit/918716812b155eb889f73c125b11bd53bb475564). Those commits credit **id Software** and **ioquake3 contributors**: Quake III CTF/team rules and key-area defense inspired the decisions. Quake III did not supply a Halo hill implementation; no additional Quake source was imported here. |
| Network bots | **thelinkin3000**, [ed6cdaed](https://github.com/thelinkin3000/halo-ce-universal/commit/ed6cdaeda8871b7bee781c76bff3a4da2836c1bf), for publishing bot actions through the existing tick queue. GulchCE adds explicit host ownership and opt-in machine reservation. |
| 63-bot capacity and live controls | **thelinkin3000**'s `bots.h`, `overlay_menu.c` and `network_server_manager.c` at the pinned reference above, built on **BirchWoodGod**'s OpenCE/Vita bots. GulchCE adapts capacity and join/quit behavior into feature-owned controls with generated CE menus, explicit ownership and delayed identity reuse. |
| Gyro settings | **thelinkin3000**, [29638d60](https://github.com/thelinkin3000/halo-ce-universal/commit/29638d603d90283e16f95b2ac78d1ab99639912a), as the settings/zoom-mode reference. Built on **MrBruh**'s existing OpenCE Android controls (`fc610044`), preserving their legacy behavior. |
| Android cheat menu | **thelinkin3000**, [406f793f](https://github.com/thelinkin3000/halo-ce-universal/commit/406f793f4c08751f854a3615ed244ea3b1f90b88), whose overlay credits **hardran3**'s **OpenCE-Ultrahand** and **HaloDrones** packages. GulchCE uses a new small typed Android dialog/queue and existing cheat APIs; the fork's general console and renderer overlay are not imported. |
| Navigation, richer vehicles, aiming, powerups and Infection | Focused adaptations from **thelinkin3000**'s `bots.c` at `18a94774`, retaining its credit to **BirchWoodGod**'s Vita navigation, vehicles and powerups (GPL-3.0-only). SWAT/Infection and aiming rules are the fork's additions on that base. The snapshot worker is a separate implementation of the optional worker capability. |
| Touch editor and edge handling | **thelinkin3000**, [d119125f](https://github.com/thelinkin3000/halo-ce-universal/commit/d119125f) (edge controls, button aim, version-4 layout and toolbar), [62824171](https://github.com/thelinkin3000/halo-ce-universal/commit/62824171) (scrolling), and the earlier three/four-finger gesture, built on **MrBruh**'s OpenCE Android controls. The gesture opens GulchCE's existing cheat dialog. |

Code credits belong here and in player-facing commit notes, not in the
art-only `AUTHORS.md`. Release packaging includes the retained GPL text and
this provenance; web distributions include both in `licenses.txt`.

## Reference audit

This source audit compares the original `d92cc933` reference and the fork's
[current main, 18a94774](https://github.com/thelinkin3000/halo-ce-universal/tree/18a94774a73ca275e9c0ab4f281e492ad8bb656e)
(28 October 2026), with GulchCE's combined implementation. The 90 commits
between those references leave the bot core and cheat-entry list unchanged,
but add touch-layout/editor changes and replay features. It does not claim full
behavioral parity with the fork. The bot/touch follow-up adapts the remaining
capabilities below while retaining GulchCE's defaults and engine interfaces.

| Reference capability | GulchCE disposition |
| --- | --- |
| 63 bots, names and host-only network actions | Capacity restored; ordinary protocol-39 player records/actions retained. Lobby limits still apply. |
| Live add/drop by team; difficulty, routes and vehicles in a menu | Implemented through CE settings and Manage Current Bots. The three/four-finger gesture opens the Android cheat dialog when enabled offline or as host; the general renderer overlay remains separate. |
| Nine cheat switches and weapon/powerup/vehicle spawns | All exposed by the Android dialog, plus the earlier camouflage action. Available offline or to the session host; joined clients cannot use them. |
| Explicit gyro modes, per-axis sensitivity and inversion | Adapted; the default Touch Settings mode additionally preserves the user's earlier saved gyro settings. |
| Roles, lanes, vantage choices, CTF/King goals and ground vehicles | Focused adaptation already present, not an import of the fork's complete bot core. Vehicles stay off by default, unlike the fork's enabled default. |
| Teleporter routing/shortcuts, ladder routing and learned navigation obstacles | `bots_navigation.inl` adds bounded link discovery, measured shortcuts, disconnected-floor links, reservations, cooldowns and ordinary climbing controls around the existing path core. |
| Detailed vehicle weighting, passenger pickup/waits, flipping, vehicle-width obstacle learning and turret arc decisions | `bots_vehicles.c` adds these choices, charged turret fire, injury escape and combat driving. Entry/exit/flipping use ordinary player actions; the engine still decides whether the action is legal. |
| Skill-dependent head/feet aim, SWAT aim, objective-target combat priority, Infection-specific goals | `enhanced_bots.c` and `bots_infection.inl` add stable aim choices, carrier/hill target weighting, zombie pursuit/melee and human regrouping/retreat goals. Infection does not select sniper vantage points. |
| Detailed powerup claims, skill thresholds, avoidance and shared scan cache | `bots_powerups.inl` shares a bounded once-per-second scan, resolves claims by distance and bot order, and uses the existing unreachable-item memory and pickup lifecycle. |
| Optional helper-thread pathfinding | `bots_worker.inl` provides three snapshot jobs, stale-answer rejection and joined cleanup. `bots.thread` is off by default and exposed in the bot menu. No performance equivalence or measured speedup is claimed. |
| New touch edge-zone handling and optional held-button aim suppression | Adapted. Existing held-button aiming remains the default. Empty edge space retains Android gestures. |
| Movable Options button and centered layout-editor Export/Import/Save | Adapted with layout version 4; versions 1–3 still load, and new coordinates are validated and rescaled. |
| Scrollable Android General settings | Adapted with the expanded settings inside a scroll view. |
| General overlay console, custom TOML commands, HUD/debug and game-speed controls | Outside the requested bot/Android-cheat UI scope; not imported. |
| Replay recording, Theater and playback controls added after d92cc933 | A separate feature, not part of this bot/Android-cheat completion. Not imported. |

The retained legacy gyro mode and disabled vehicle/worker defaults are
deliberate. Source comparison cannot establish touch feel or device-specific
sensor behavior; runtime evidence is recorded separately below.

## Following an enhanced route

With Enhanced Bots enabled, `bots_choose_goal` selects the host's objective
or an item, then `bots_search` uses the existing path graph. Read
[`bots_navigation.inl`](../../port/linux/game/bots_navigation.inl)'s
`bots_navigation_answer`, `bots_navigation_query` and `bots_navigation_follow`
next. For example, a Blood Gulch bot measures a teleporter's entrance and
exit against its ordinary walk, accepts only a route at most 80% as long
and at least 15 units shorter, walks onto the entrance, then detects the
ordinary engine teleport and searches from the exit. Ladder links connect
otherwise disconnected floors and finish with facing, forward and jump
controls. There are at most 64 links and two reservations per link.
Once climbing starts, the bot keeps that destination through the landing,
even during combat. Its climb timeout replaces ordinary horizontal stuck
recovery until it lands or abandons the link.

[`bots_worker.inl`](../../port/linux/game/bots_worker.inl) optionally runs
only graph traversal on one worker. Submission snapshots walkability and
breakage; event signals transfer job ownership. Completion rejects changed
units, goals, link phases, teleports or answers older than one second.
The copied graphs provide a closed sentinel for open edges. Malformed
collision rings keep searches synchronous so shared engine diagnostics
cannot race. Obstacle queries and path smoothing remain on the game thread. Disposal
joins the worker before releasing feature graphs; scenario geometry remains
loaded until the later scenario unload. Allocation/start failures fall back
to synchronous searches. Job arrays and graph buffers use the engine's paired
tracked allocator; growth allocates before releasing an old buffer so failure
preserves ownership. Fixed-tick benchmarks also remain synchronous.

[`bots_vehicles.c`](../../port/linux/game/bots_vehicles.c)'s
`bots_vehicle_goal` weighs seats and `bots_vehicle_action` drives, waits,
fires or requests exit. Obstacle learning closes a separate vehicle graph;
it never closes biped routes merely because a vehicle is too wide.
[`bots_infection.inl`](../../port/linux/game/bots_infection.inl) shares a
once-per-second role snapshot; [`bots_powerups.inl`](../../port/linux/game/bots_powerups.inl)
shares item discovery but keeps claims and timeouts per bot. Map disposal,
death and respawn clear their respective ownership. Only the host chooses
actions; clients consume the existing records. Protocol versions and
Xbox/CE/MCC map namespaces are unchanged.

## Checks and limits

The navigation/combat/touch follow-up's final self-review covered the combined
modules, generated settings, worker ownership, disabled behavior, map and
respawn cleanup, source provenance and regression fixtures. It corrected
closed-edge snapshot sentinels, allocation cleanup, a clock read before game
initialization, duplicate powerup pursuit and climbing interrupted by horizontal
stuck recovery. A live map transition exposed a worker allocation mismatch;
the corrected allocator pair now survives repeated map disposal. Real
Boarding Action movement also showed that tall ladders
need more than the reference's fixed eight seconds: the allowance now scales
with height, bounded to 8–60 seconds, followed by at most two seconds to land.

Follow-up validation (final game code `4ec67c3d`):

- [Build CI](https://github.com/chestahh/GulchCE/actions/runs/40796118360)
  passed Linux, Windows, Android, web and changelog jobs;
  [MCC validation](https://github.com/chestahh/GulchCE/actions/runs/40796118536)
  also passed. These CI runs validate the final code before testing promotion.
- Windows, Android guest/host and web release builds passed. The original
  `--portable --release` configure options were restored. The unchanged Java
  changes passed SDK 35 compilation and layout/gyro checks; all 36 web page
  tests passed. Bot settings were rendered in a hidden window and inspected.
- The [final web CI job](https://github.com/chestahh/GulchCE/actions/runs/40796118360/job/123192982935)
  passed browser smoke/room checks and native WebRTC exchanges with Chromium
  and Firefox (320 of 320 messages returned in each browser).
- The focused bot/worker/navigation/cheat/gyro/glow suite passed 83 checks. The six
  real-game stock/enhanced/disabled regressions passed on `3899ad7e`;
  enhanced Slayer/CTF/King passed again on `94c60acc` after the climb-control
  correction. These are earlier checks of those unchanged synchronous paths.
  Stock Blood Gulch bots and the default-disabled network-bot path both
  passed again on the final build (two real-game checks).
- A 15-bot Infection match on `3899ad7e` with the worker enabled had active
  movement/combat, completed teleports and a clean exit. Boarding Action matches discovered
  ladder links and approached/climbed them, but random goals alone did not
  establish completed landings during those bounded runs.
- A disposable engine fixture of the `090dbd05` bot code placed one bot at
  the foot of a discovered five-unit Boarding Action ladder with matching
  initial facing and velocity.
  Production traversal code then climbed, landed and resumed ordinary goals,
  without an assertion. Only placement/initial route choice was instrumented;
  this verifies traversal, not natural route selection or every ladder.
- On `090dbd05`, a dedicated host with six bots and a joined client played
  Xbox Blood Gulch, CE Death Island and Xbox Blood Gulch again. The worker started on all three
  maps, cleanup raised no assertions, and the server quit cleanly. The test
  stopped its own client after server shutdown. This establishes those live
  transitions; a live transition involving MCC remains unverified.
- The final Xbox Blood Gulch CTF run with vehicles and the worker admitted
  63 bots plus two humans with unmodified OpenCE build 317. Both peers exited
  cleanly and logged all 65 names. Intact players 0..31 compared for 67 paired
  seconds without sustained divergence under unchanged tolerances. The
  existing 4096-byte log truncation prevents comparing every player's state.
- CE Death Island CTF with the same 63-bot/worker setup also admitted all 65
  participants with OpenCE 317 and exited cleanly. Its intact records compared
  for 67 paired seconds without sustained divergence, with the same log limit.
- MCC DigsiteMP/1 with 63 bots and the worker admitted all 65 participants
  using normal startup timeouts. After the glow endpoint fix, both peers
  exited cleanly without assertions; intact records compared for 73 paired
  seconds without sustained divergence. This uses two GulchCE peers and the
  existing MCC protocol, not an upstream OpenCE client.

The 63-bot/menu/cheat completion was reviewed as a combined implementation,
including generated screens, shared hooks, failure rollback, ownership and
cleanup. Runtime rendering exposed and fixed a description-ownership bug:
management feedback must have an owned text buffer with no string list,
otherwise rendering replaces it every frame. The host's assigned player
slot is encoded before publishing a join, and existing lobby focus is kept.

Earlier 63-bot/menu/cheat completion validation on Windows:

- Windows, Android guest/host and web release builds passed with the original
  PGO options restored. Android Java compiled against SDK 35; Java layout and
  gyro checks and all 36 web page tests passed.
- The focused suite passed 123 checks with five platform skips; 25 additional
  server-teardown checks (including negative controls) passed after updating
  their extracted-source fixture for bot cooldown cleanup. All six
  real-game bot regressions passed, including stock Slayer, enhanced
  Slayer/CTF/King, movement/combat and network bots disabled.
- Unmodified OpenCE build 317 (protocol 39, release archive SHA-256
  `9950628b540cd5245440ae5db7ddad97cc76c9aed1ec20ca1419156acc4fb931`)
  admitted 63 bots plus two humans on Xbox Blood Gulch and CE Death Island.
  Both peers logged all 65 names and exited cleanly. The existing 4096-byte
  player-state line truncates large matches: intact players 0..31 compared
  for 67 and 66 paired seconds, respectively, with no sustained divergence
  under unchanged comparator tolerances. This does not compare all 65 positions.
- Three live additions and three removals agreed with OpenCE for 67 paired
  seconds. Both peers retained the same dead, unreused quit-player datum;
  that ninth logged record is not a ninth active participant. Bot identity
  reuse after the quit deadline and failed encoding are also harness-tested.
- Two GulchCE instances on MCC DigsiteMP/1 admitted all 65 participants and
  exited cleanly without a timeout override. The same intact-record comparison
  covered 76 paired seconds with no sustained divergence; the large-log
  limitation applies here too.
- OpenCE also joined an already-running Xbox match with 63 bots: both peers
  logged all 65 names and exited cleanly. Their once-per-second samples did
  not align, so this establishes admission/lifecycle, not a position comparison.
- Hidden rendered checks covered Bot settings, Manage Bots, Server Setup
  and Network Setup. Scripted controller presses verified that the real
  management screen displays host-only rejection feedback.

The Windows web-build test module has six POSIX path/header failures despite
the web build succeeding; Linux CI supplies the native fixture environment.
Android physical-device interaction, an Internet game through a broker, and
live cross-family map changes were not established by these earlier checks.
The follow-up above adds Xbox/CE transition coverage. Saves
from completed isolated runs are archived losslessly with original paths
and verified hashes; copied maps and disposable binaries are removed.

The focused [harness tests](../../tools/harness/tests/) exercise real source
for lobby defaults/opt-in/ownership, partial and repeated cleanup, gyro
mode/sensitivity behavior, and the Android cheat queue's permissions,
capacity and lifecycle. [test_touch_layout.py](../../tools/test_touch_layout.py)
covers Java layout, gyro integration and sensor-state transitions.
[test_bots.py](../../tools/test_bots.py) exercises real local games.

Earlier implementation validation on Windows (six bots):

- All six real-game bot regressions passed: stock Blood Gulch and Chill
  Out Slayer, enhanced Blood Gulch Slayer/CTF/King, and bots absent from a
  network game when the option is off. These checks establish movement,
  combat and clean exits, not flag captures or complete objective strategy.
- Unmodified OpenCE build 309 (`22947203`) joined a GulchCE host with six
  bots. Xbox Blood Gulch had 68 paired seconds and CE Death Island CTF had
  68, with eight players and no sustained divergences under unchanged
  comparator tolerances. With vehicles enabled, bots entered and drove
  vehicles and the same OpenCE client agreed for 67 paired seconds.
  A CE late-join run admitted both players and all bots, but its samples
  did not align: that run establishes joining/lifecycle behavior only.
- Windows, Android host/guest and web compile/link checks passed. Android
  and web used temporary Windows command adapters for Unix shell steps
  in the existing build graph. Android Java sources compiled against SDK
  35; the Java layout/gyro regressions and 36 web page tests passed.
- The focused regression set passed 89 checks with five platform skips.
  The larger harness/net-comparison run had 1,292 passing checks and six
  skips after correcting a Windows test-compiler macro. All 312 remaining
  failures reproduced on an unchanged export of the branch's `origin/main`
  base (`e94810f1`); they are not a green full-suite result.
- MCC DigsiteMP/1 originally needed the debug timeout override to reach
  gameplay. The subsequent [MCC startup fix](maps_and_resources.md) prepares
  the host identity before admission and avoids hashing its unchanged
  package twice. With normal timeouts and no override, two GulchCE instances
  then agreed for 76 paired seconds with six bots and two human players,
  90 logged samples each and clean exits. A repeated run with the client's
  package in a differently named folder also passed 76 paired seconds.
  Local play produced 61 logged samples. Comparator tolerances were unchanged;
  this is not evidence for every MCC package or arbitrary storage stalls.

Compilation and automated checks do not establish Android touch/gyro feel,
all vehicle/map combinations or full campaign compatibility. The
native/browser WebRTC harness could not run on this Windows host (POSIX
build/path assumptions); Linux CI runs it with Chromium and Firefox. An
Internet game through a broker, Android device/visual checks and live map
transitions involving MCC remain outstanding. Record subsequent checks with
the implementation commit; do not treat suggested checks as results.
