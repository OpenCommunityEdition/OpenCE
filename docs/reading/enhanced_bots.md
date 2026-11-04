# Standalone enhanced bots

Enhanced bots are an optional GulchCE controller, selected with
`bots.enhanced` when a map loads. Stock bots keep their original Slayer
combat, navigation and recovery algorithms. Exactly one controller generates
actions for a bot; enabling enhanced mode does not run the stock decision loop.
`bots.vehicles` and `bots.thread` remain off by default. Network admission,
player creation, engine physics and player action replication remain shared
engine services. Clients replay host actions and never run bot decisions.

## Ownership and reading order

1. [bots.c](../../port/linux/game/bots.c), `bots_initialize_for_new_map`,
   `bots_update_actions` and `bots_dispose_from_old_map`: latch the controller,
   dispatch before the stock loop, and dispose the selected owner. Common
   settings, admission and management interfaces remain here and in
   [bots_network.c](../../port/linux/game/bots_network.c).
2. [enhanced_bots_controller.c](../../port/linux/game/enhanced_bots_controller.c),
   `enhanced_controller_update`, `bots_choose_goal`, `bots_follow_path` and
   `bots_goal_failed`: independent combat state, strategic intent, current
   navigation goal, paths, search budget and recovery.
3. [enhanced_bots.c](../../port/linux/game/enhanced_bots.c),
   `enhanced_bots_goal` and `enhanced_bots_capture_approach`: objective policy
   using existing read-only engine queries. Roles, lanes, vantage points,
   powerups, Infection and link navigation remain enhanced-owned helpers;
   see [the earlier feature guide](bots_and_android_features.md).
4. [bots_vehicles.c](../../port/linux/game/bots_vehicles.c),
   `bots_vehicle_player_goal`, `bots_vehicle_choose_goal` and
   `bots_vehicle_action`: seat claims, entry approach, progress deadlines,
   driver/passenger roles, ground driving and ordinary Use-button entry/exit.
5. [enhanced_bots_flight.c](../../port/linux/game/enhanced_bots_flight.c),
   `flight_search`, `enhanced_flight_action` and `enhanced_flight_can_exit`:
   bounded 3D routing, takeoff, cruise, approach, descent and grounded exits.
6. [bots_worker.inl](../../port/linux/game/bots_worker.inl),
   `bots_worker_submit` and `bots_worker_job`: private graph snapshots and
   rejection of answers from an old goal, unit, parent vehicle or seat.

The controller derives from OpenCE's BirchWoodGod bot implementation
(`7ea7b0bf7`) and the existing GulchCE adaptations, rather than importing an
unreviewed fork wholesale. Adapted navigation/vehicle code retains its
GPL-3.0-only notices and [recorded provenance](bots_and_android_features.md#credits-and-source-provenance).
The new aircraft planner is independently implemented against engine APIs.

## A seat approach without goal churn

An attacker heading toward the enemy flag sees a friendly human in a nearby
transport. The controller keeps its strategic destination separately from the
seat entrance. It reserves a legal seat, follows that entrance across ticks,
and presses Use only when the engine's nearby-seat check returns that exact
seat. Refreshing the same entrance no longer alternates between objective and
vehicle goals and invalidates the path twice each tick. Close approaches can
steer around the hull. A moving entrance can update the route; lack of progress
releases the claim instead of pursuing it forever.

Friendly human transports can interrupt optional weapon detours, without a
minimum journey length or a Warthog-only restriction. Candidate entrances must
be near enough and slow enough to approach; a large hull does not hide a
nearby door merely because its origin is farther away. A carrier keeps returning the flag;
a bot holding the hill keeps its scoring position. Bot drivers briefly wait for
teammates with active boarding claims. Passengers stay with a friendly human
driver instead of dismounting immediately beside their previous goal. Gunners
retain their seats during quiet journeys. The engine still checks animation,
occupancy, hostility and reach. There is no forced attachment or teleport.

Boarding, changing seats and dismounting invalidate the old navigation mode.
Passengers do not request pedestrian routes; drivers use vehicle-width ground
paths or the flight controller. Vehicle searches do not use pedestrian ladder
or jump recovery, and ground drivers wait for a usable route instead of using
the pedestrian straight-line fallback. Learned obstacles invalidate current
and pending routes. Search admission rotates among waiting bots rather than
always favoring the first three player slots.

## Objective behavior

CTF and King goals no longer lose priority to Slayer's last-seen-enemy chase.
Attackers retain flag destinations instead of choosing precision-weapon vantage
points, and keep advancing while fighting. Defenders can still cover their stand.
A bot on the hill does not leave for a powerup. Failed objectives release
claims and get a retry delay. A flag carrier's return remains urgent; near the
home stand, it walks onto the actual capture point and can jump onto a raised
stand instead of stopping at the normal path tolerance. The final approach
runs after ordinary combat/recovery movement arbitration, and requires a clear
collision ray so it cannot bypass the route through a base entrance.

Path steps require horizontal and vertical proximity, avoiding false arrival
on a different floor. Teleporter/ladder traversal still uses the existing
bounded enhanced link planner and ordinary player controls. Objective state,
vehicle reservations and flight paths reset on respawn/map changes; live
removal also releases reservations. Worker shutdown joins before freeing map
snapshots, including partial preparation.

## Custom vehicles and map families

Vehicle discovery uses loaded unit/vehicle tags, seat definitions and physics
type, not names such as Warthog, Pelican or Longsword. Human-plane and
alien-fighter physics select flight navigation; tank, jeep and scout physics
select ground navigation. Stationary gunner seats keep turret behavior.
Custom aircraft therefore qualify when their authored physics, animations and
seats support ordinary player controls. Flight clearance uses the loaded
physics mass points, testing segments from each point at the current attitude,
relative to the physics center of mass. This follows the engine's vehicle-motion
queries for map and object geometry. Physical thickness determines cruise
height; a flat aircraft does not require its wingspan in headroom. Missing or
invalid shape data falls back to seven rays around its bounding sphere. The
engine uses unscaled mass points for vehicle physics, so visual object scale
does not rescale this hull. Nine terrain probes choose a cruise altitude over
the journey; a craft keeps its altitude until approach instead of descending
early toward a distant destination floor.
Each aircraft has at most 1024 search nodes and 32 route steps; it expands four nodes
with up to 26 neighbors each every eight ticks, with at most 256 expansions
per search. Human-plane tags can use vertical
lift; alien-fighter routes use sloped segments and reject vertical shortcuts. A search that exhausts its budget can return a
validated partial route that makes progress; it backs off when no useful route
is found. Steering uses
the vehicle's heading independently of combat aim, including while waiting
for a route. Hover craft stabilize vertical drift while a search is pending;
otherwise neutral lift slowly drops them back onto the ground.
`debug.bot_trace` also reports flight phase, search budget, route and recovery
state. Takeoff uses ordinary lift
input and turns toward the departure route before accelerating. Hover craft
keep level so their lift does not push them backwards at low thrust; bounded
search steps can align a large hull with an exit. Route lookahead lets heavy
transports skip small intermediate cells when the shortcut is clear. Progress
toward a climb waypoint counts even when it increases distance to the final
objective. Approaches brake according to speed and remain committed as the
craft slows. Landing selection checks up to 25 nearby descent columns so a
roofed objective can be reached on foot from an open landing site. Hover
aircraft align above that column before descending. Pilot fire is retained
only while steering remains aligned with the shot.
Forward-flight craft descend in a circuit around the landing site; hovering
craft use crouch lift. Forward-flight steering retains positive thrust while
airborne because its velocity correction depends on that input. The glide
permits contact with upward-facing ground while rejecting walls.
Aircraft use settled ground contact or a controlled low approach before
attempting to dismount. The latter checks height, horizontal speed and descent
rate; it avoids forcing a forward-flight craft to stop completely in the air.
These checks also apply when vehicle use is disabled. After 45 seconds
without progress along the route, a pilot with detected ground below
recovers by descending and leaving the vehicle when settled. The bot can then
resume its objective on foot. Dismount clears flight state, and a failed trip
avoids that craft for two minutes. Grounded passengers also leave a driverless craft
without requiring their old objective to be nearby.

This is not a guarantee for every modded tag. Script-driven transports,
missing player entry animations, inaccessible entrances, water-only boat
routes and aircraft with unusual control/landing requirements need targeted
support and map tests. Mass-point segments and fallback rays are not a swept animated
mesh proof, and large aircraft require sufficient navigable space.

Xbox and CE use their existing loaders and engine data. MCC adaptation stays
in [mcc_seats.c](../../port/linux/game/mcc_seats.c): `mcc_seat_controller`
reports the controlling player for an existing MCC seat-bearing biped.
Discovery can then offer its passenger seats without interpreting its biped
tag as a vehicle definition. No Xbox/CE loading path is repurposed. The
separation adds no shared-engine hook, packet field, physics or scoring rule;
host-generated actions use the existing protocol, so no version bump is needed.

## Checks and evidence

`tools/harness/tests/test_bots_stock_preservation.py` compares normalized
stock function bodies with hashes from the original implementation; admission
and dispatch are checked separately. This guards source preservation, not
runtime equivalence on every map. Its lifecycle check executes the real
initialization/disposal functions across stock/enhanced/stock map ownership,
including setting changes before disposal.

`test_enhanced_controller.py` exercises repeated seat pursuit, objective
cooldowns, capture approach and search fairness using actual controller
functions. `test_bot_decisions.py` covers real seat selection, slowly moving
human transports, excessive speed, short journeys, reservations and recovery.
`test_enhanced_flight.py` exercises the real flight planner against an open
route, obstructing wall, sealed space and hull clearance, plus descent, exit
conditions, takeoff and retargeting. Worker tests reject stale answers after
respawn, goal changes, boarding and seat changes. These fake-world tests do not
establish flight quality for a particular map's physics.

Validation on Windows, 3-4 November 2026:

- Windows release, Android guest/host and web builds completed. Android/device
  play and a native Linux runtime were not available in this environment.
- The focused bot, worker, lifecycle, MCC-seat, web-build and network-comparator
  suite passed 198 checks, with six environment-dependent skips. All 36 web
  JavaScript tests passed. Negative controls verify that removing seat commitment
  or the final capture collision check makes its regression fail.
- All eight local game scenarios passed on the integrated testing baseline:
  stock Blood Gulch (Normal and Veteran) and Chill Out (Heroic), enhanced
  Slayer, CTF and King, network bots disabled, and an unopposed CTF return
  with a completed capture. King recorded hill score. These are bounded
  scenarios, not a claim of competitive play
  quality on every objective layout.
- Exploratory local runs completed on CE Death Island and custom Hugeass.
  The integrated candidate also completed MCC Digsite MP 1 with 15 bots. Hugeass bots naturally entered modified ground vehicles and
  four custom jet seats. The controlled human-driver scenario observed a bot
  approaching and boarding a Warthog through ordinary Use input.
- Host/client comparisons passed on the integrated candidate with 15 bots
  and two humans for stock Xbox Slayer, enhanced Xbox CTF, CE Death Island
  CTF and MCC Digsite MP Slayer. Both machines used the same candidate build.
  Comparisons covered 67-75 seconds after warmup with no sustained divergence
  at the comparator's tolerances and no log parse errors. A separate live
  CTF run completed three removals and three additions, including slot reuse,
  without sustained divergence. Quit-player statistics records remain until
  match end; they are not active bot ownership.
- A dedicated server and client completed four worker lifecycles across Xbox
  and CE rotations. Its existing rotation parser omitted the MCC package
  entry before loading it (`maps/1.map` lookup), so this does **not** validate
  MCC-to-legacy map transitions. MCC standalone network play and adapter
  activation/isolation are checked separately.
- Aircraft fixtures use real loaded tags and engine physics. Initial pilot
  placement and destinations are controlled; these fixtures do not prove
  natural discovery of every aircraft. They exposed lift, braking, ground-exit
  and flat-hull clearance problems, which have regression coverage. On the
  integrated build, the Pelican travelled 496 units and dismounted 3.0 units
  from its destination; the Longsword travelled 1,206 units and dismounted
  2.6 units from the authored destination. The custom jet also completed a
  map-authored takeoff and delivery, dismounting on a controlled low approach
  and reaching the flag on foot. A separate short approach checks early
  deceleration. All three original pilots survived. This establishes delivery,
  not automatic parking of every aircraft.
- Native/browser WebRTC validation was attempted but the existing test runner
  fails on a Windows file-URL path (`C:\C:\...`) before compiling its helper.
  Browser/native internet crossplay remains unverified here.

Detailed run logs and isolated validation scripts are retained under the
worktree's ignored `build/enhanced-validation/` directory. Original maps and
saves are not changed; copied maps and caches are removed after each run.
