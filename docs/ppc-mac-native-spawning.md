# Next integration work for native player spawning

This checklist records the source/data boundary after the native biped storage,
weapon constructor-parts, collision-model and world-location work. Hardware evidence belongs in the
progress report. None of the interfaces below presently establishes a completed
`object_new`, a working biped update loop or a playable session.

## Build and validation cadence

Following the user's batching discussion, group the next work around an actual
biped, its starting weapons and world placement. Use local bounds/schema checks
while implementing dependencies. Run a shared MacBook compile/Intel control gate
for a batch; reserve PowerBook runs for integrated milestones, with both G4s
required for endian/ABI/graphics changes and milestone confirmation. A narrow
probe is appropriate to isolate a new failure, not as the default deliverable
for each added field. Do not equate Intel-only checks with PPC validation.

## Current boundaries

- `port/mac/native_object_tags.c` and generated `native_object_schema.inc`
  convert the complete inherited object/unit/biped storage, including every
  reachable typed block. With item/weapon storage the combined schema covers
  324 fields in 28 structures. This is complete **storage
  conversion**, not complete runtime semantic validation or dependency closure.
  Cached node indices, enum domains and cross-block function/color indices still
  need the existing tag-validator checks or equivalent checks before constructors
  consume them. Model and world-animation roots now convert with the starting weapons.
  Effects, HUD, dialogue and other referenced payloads still require independent
  conversion; the integrated loader does not claim transitive closure.
- `port/mac/native_tags.c` copies metadata, validates canonical inheritance and
  owns published native roots. Build this inventory before publication; raw
  retail parent claims must not be accepted as proof of a reference's family.
  Never publish an audit bundle or an incomplete native-shaped root. A successful
  biped lookup proves only that root is available, not that its dependencies are.
- `port/mac/retail_collision_model.c` decodes semantic `coll` data into portable
  structures. `port/mac/native_collision_tags.c` now adapts those fields into
  actual engine `collision_model` roots, including resistance, regions and
  permutations, materials, spheres, nodes and all eight arrays of each node BSP.
  Its owner frees every native allocation; effects have owned names and salted
  handles but still need separately converted payloads. Nonempty obsolete
  modifiers remain rejected. See the latest progress entry for validation scope;
  do not infer PPC runtime validation from a successful cross-build.
- `mac_native_object_visit_references` walks the same descriptors used for biped
  and weapon conversion, including nested references and predicted resources.
  A loader can use this to inventory direct dependencies before attempting
  constructors. It preserves duplicates, skips NONE and supports early stop;
  group zero denotes an unspecified predicted-resource family. It does not
  establish transitive closure, validate arbitrary caller-supplied native
  pointers or make a root safe for gameplay by itself.
- `port/mac/engine_world.c` supplies scoped actual location queries and an
  opt-in native portal/PVS graph. `engine_world_partition.c` initializes actual
  cluster-partition storage; real reconnect/list/disconnect are exercised. These
  do not provide rendering, AI, attachment services or complete gameplay-world
  initialization. Full `object_reconnect_to_map` is not yet exercised.
- `port/mac/retail_object_spawn.c` is a checked constructor-input audit bundle.
  Its observed ammunition values are expectations derived from the inputs;
  they are not execution of `weapon_new`.

## Integrated actor loader and remaining link boundary

`mac_actor_assets_load` atomically owns and publishes 17 roots for Blood Gulch's
biped, two starting weapons and their projectile/contrail dependencies.
`mac_actor_assets_load_with_bitmaps` adds the resident tracer bitmap for 18 roots. It checks model/collision hierarchy and animation
checksums, translates finalized model groups, deduplicates lights and rolls back
on failure. Shader references and attachment references retain owned names;
that does not imply their referenced bitmap/effect/lens payloads are loaded.
First-person animation graphs have separate conversion coverage and are not
included in these roots. Model resources currently provide CPU storage and
actual engine compression; GPU upload/drawing remain separate work.

`tools/mac_spawn_link_checks.sh` and `tools/mac_spawn_provider_checks.sh` retain
the complete real object-type callback tables. They compile whole engine units
and deliberately fail the link when genuine providers are still missing.
`tools/mac_spawn_provider_report.py` indexes candidate definitions for that
failure; a candidate is not proof that its unit builds or that its global state
is initialized. Broad linking must be followed by actual runtime initialization,
full constructor execution and cleanup before claiming spawning. The current
hardware milestone exercises actual enhanced weapon constructor parts including
projectile-trail texture preload; it still does not call full `object_new`.

## Runtime initialization now available

`port/mac/engine_arena.c` compiles the whole real game-state source with a native
Mach reservation. Use it instead of separately compiling `game_state.c`.
`mac_engine_arena_start` must precede actor model buffers so the fixed game-state
range is reserved first. Allocation registration and data/pool creation remain
actual engine functions. The bootstrap is engine-thread-only; it does not
implement saved-game file operations or map lifecycle callbacks.

The separate light runtime test now invokes actual light initialization and two
map lifecycle cycles using that arena and genuine partitions. The actor-assets
test invokes actual weapon map initialization through native tag iteration.
These are prerequisites, not a substitute for calling the complete
`objects_initialize` / `objects_initialize_for_new_map` chain. The original
partition teardown retains side arrays and registry entries, so whole-engine
restart cleanup remains outstanding. Dispose all engine users before stopping
the arena; its live allocation guards cannot establish that arbitrary external
pointers have been released.

Shared settings run through the native file/path boundary in `engine_config.c`;
the generated compilation view preserves the real setting table, parser and
write behavior. Typed common storage does not initialize service pointers.
`engine_clock.c` supplies actual monotonic counter/frequency services and
`engine_crt.c` supplies ASCII label comparisons. UTF-16 calls must still avoid
Darwin's incompatible 32-bit wchar routines.

## Next full-link blockers after the 130-unit gate

The 118 unresolved symbols match on PPC and Intel. Remaining real providers
include AI encounters/perception/stimulus, camera/director, sound management and
Xbox rasterizer draw/model/widget/texture paths. These require genuine native
backend work; unresolved callbacks must not be replaced by success stubs.

`errors.c` also emits an unresolved `__sync_val_compare_and_swap` under GCC 4.0
for lazy debug-file publication. Supply a typed ILP32 pointer CAS with real full
memory-barrier semantics (or a proper lock) at that boundary. Terminal/dialogue
paths need `_snprintf` / `_vsnprintf` compatibility, and error reporting needs
native elapsed milliseconds. These are implementation gaps, not evidence of a
PPC map-layout failure. The broad UTF-16 object audit found only unicode.c using
wide imports; that unit now routes to the tested native UTF-16 implementation.
Audit added units again rather than relying on SDK wchar32 routines.

## Constructor chain that must remain intact

`source/game/players.c` chooses multiplayer biped handle `e2070093` from
`globals\globals`, constructs placement data and calls `object_new`.
`source/objects/objects.c:object_new` then:

1. Checks tag group/type consistency, gets the registered object-type definition
   and allocates an object header/datum through `object_header_new`.
2. Applies placement and allocates node matrices and orientation blocks according
   to the referenced model's node count.
3. Calls `object_type_new` (`source/objects/object_types.c`), which invokes the
   registered part constructors. A biped includes the object, unit and biped
   parts; a weapon includes the object, item and weapon parts.
4. Selects change colors and region permutations, initializes vitality, computes
   node matrices, reconnects to the map, computes exported functions/colors and
   creates attachments. Creation effects and failure cleanup follow.

`source/units/units.c:unit_new` requires an animation graph, selects a unit
seat/weapon animation label, assigns dialogue and calls `unit_add_initial_weapons`.
That function recursively calls `object_new` for assault rifle `e25e00ea` and
pistol `e34d01d9`. Their creation is not optional merely because the outer object
is the first player. `source/items/items.c:item_new` also reads actual game time.
`source/items/weapons.c:weapon_new` initializes magazines/triggers and, under the
network-player build define, precaches projectile trails.

The target biped also has a flashlight attachment. Full object construction must
support its relevant light/attachment path rather than dropping the reference or
returning success from a no-op callback.

## Native weapon conversion implemented in this increment

The extended `tools/mac_native_object_schema.py` uses actual declarations from
`source/items/item_definitions.h`, `source/items/weapon_definitions.h` and
`source/game/aim_assist.h`. It reuses the complete object descriptors and
owned-allocation decoder, with an actual `weapon_definition` entry point and
source-derived layout witnesses:

| Structure or field | Disk/native 32-bit layout |
| --- | --- |
| `weapon_definition` | size `0x508` |
| inherited object | offset `0`, size `0x17c` |
| inherited item | offset `0x17c`, size `0x18c` |
| weapon component | offset `0x308`, size `0x200` |
| first-person interface | whole-root offset `0x45c`, size `0x34` |
| magazine block | whole-root offset `0x4f0`; element `0x70`; max 2 |
| trigger block | whole-root offset `0x4fc`; element `0x114`; max 2 |
| ammunition objects | magazine offset `0x64`; element `0x1c`; max 8 |
| firing effects | trigger offset `0x108`; element `0x84`; max 8 |

The converter includes all item and weapon scalars, aim-assist parameters, all references,
magazine reload/chamber metadata, ammunition objects, triggers and firing-effect
references. The source's `weapon_firing_effect` layout is in
`port/linux/game/tag_schema_objects.c`. That file also identifies effect-or-sound
references, damage effects and allowed projectile **object** families; do not
incorrectly restrict trigger projectiles to `proj` alone.

Two generator details are handled explicitly:

- First-person interface reference group metadata is written with dotted member
  paths in the existing schema; the generator now extracts those entries for
  `weapon_interface_definition`, including the retail first-person model family.
- `_weapon_definition.unused_block` is a real block descriptor that the source
  says is unread. An explicit empty-block descriptor rejects nonempty data and
  clears all three native words, including poisoned source pointers. Other
  intentionally unused scalar/padding bytes remain preserved.

The rifle has one magazine and one trigger: 240 initial rounds become 60 loaded
and 180 spare. The pistol has one of each: 60 become 12 loaded and 48 spare.
These follow the exact `weapon_new` rule, not a total-capacity clamp. The native
original storage test passed 31 positive checks and 28 mutation/empty-block checks on
Intel and both G4s. It tests malformed counts, spans, reference salts, nonfinite
values, negative ammunition and invalid magazine indices. Digests match across
all three Macs: biped 618158e7, rifle a18ee21a, pistol 0ea985ea. The constructor
probe separately exercises initial ammunition below loaded capacity.

## Verified constructor-parts milestone and next work

Fully converted rifle and pistol tags now publish through the registry, with
constructor-sensitive magazine and trigger domains checked. Actual `item_new`
and `weapon_new` execute against genuine `object_header_new`/data/pool storage
and actual game-time functions on MacBook Intel and both G4s: 63 checks pass,
including ammunition, trigger state, lighting flags, compaction, stale handles,
slot reuse and teardown. A low-ammunition case exercises the alternate loading
branch. This is isolated constructor-parts execution, still not `object_new`.

`tools/mac_weapon_constructor_checks.sh` builds an explicit stock-engine
configuration without the enhanced-network-player define. It therefore does
not exercise projectile-trail precaching. Complete that dependency chain before
claiming this milestone for the enhanced configuration; no stub implements it.
The diagnostic includes whole `objects.c` and `game_time.c` in small wrapper
translation units to access their private state and allocation functions.
It does not replace their lookup, allocator or time logic.

The actual native model and animation **graph** roots are now implemented for
the target actors and checked on both G4s; the following layout constraints
explain why the earlier skeleton/clip adapters alone could not satisfy them.
A skeleton or single sampled clip is insufficient: creation uses model regions,
markers and node data, while unit initialization walks animation seats, weapon
classes/types and animation lists. Retail `mod2` geometry parts are `0x84` versus
engine `mode` parts `0x68`; resolve conversion and registry/reference-family
translation deliberately using `port/linux/game/custom_edition_geometry.c` as
the existing layout evidence. Renaming the group without converting parts is
unsafe.

After those pieces pass, establish dependency-closure validation and the real
map cluster/linking services, then attempt full `object_new` with its unchanged
constructor chain. Missing services are implementation work, not candidates for
fake success returns. The acceptance check is a valid allocated biped and its
actual starting weapons that survive engine updates, followed by movement and
collision at a genuine Blood Gulch start; rendering a camera or sampling a pose
does not substitute for that check.

## Reproduce the verified storage and world checks

Generate source-derived metadata on the modern host, preserving the checked-in
native field descriptor output:

```sh
python3 tools/mac_engine_semantics.py
python3 tools/mac_native_object_schema.py
```

Stage the relevant source, clean-room port headers, generated metadata and
`port/mac` files into the MacBook's existing
`$PROBES/20261015/engine` checkout. From there:

```sh
sh tools/mac_native_tags_checks.sh /Applications/Halo.app/Contents/Resources/GameData/Maps/bloodgulch.map
sh tools/mac_native_object_checks.sh /Applications/Halo.app/Contents/Resources/GameData/Maps/bloodgulch.map
sh tools/mac_collision_model_checks.sh /Applications/Halo.app/Contents/Resources/GameData/Maps/bloodgulch.map
sh tools/mac_world_checks.sh /Applications/Halo.app/Contents/Resources/GameData/Maps/bloodgulch.map
```

These build PPC and i386 with GCC 4.0 and the 10.4u SDK and run the Intel
control. Copy the resulting `native_tags_ppc`, `native_object_ppc`,
`collision_model_ppc` and `engine_world_ppc` executables to the G4 probe directory
and run them against the installed PowerBook map or the iBook's staged
`bloodgulch-v7.map`. They are headless tests; do not launch the user's desktop
game. Compare the numerical digests and failure counts, not pointer values.
The progress log records observed results. Building against the Tiger SDK alone
does not establish Tiger runtime compatibility.

Run the constructor-parts test on the same MacBook staging checkout with:

```sh
sh tools/mac_weapon_constructor_checks.sh /Applications/Halo.app/Contents/Resources/GameData/Maps/bloodgulch.map
```

Copy `build/mac-audit/weapon_constructor_ppc` to each G4 and pass its map path.
Observed logs: `build/mac-audit/weapon-constructor-{build,powerbook,ibook}.txt`.

The next-batch collision adapter control is `tools/mac_native_collision_checks.sh`.
It builds both architectures on the MacBook and runs only Intel. The current
native-object probe also inventories direct dependencies. The native collision adapter has now passed on both G4s in the integrated
actor batch; the separate direct-reference enumeration probe remains Intel-only.
Use `tools/mac_actor_assets_checks.sh`, `tools/mac_native_model_checks.sh`,
`tools/mac_native_animation_checks.sh` and `tools/mac_world_graph_checks.sh` for
the current MacBook controls. The progress log records the corresponding PPC
runs. The 14-root actor batch exercises actual stock weapon constructor parts,
not full `object_new` or biped spawning.

## Latest enhanced weapon milestone

The native registry now owns projectile, contrail and resident bitmap payloads
needed by the actual AR precache path. Enhanced `item_new`/`weapon_new` passes
150 checks on Intel and both G4s, after releasing both source files. Native
texture/model headers obey shared D3D physical-address and tag-owner release
contracts. See the latest progress entry for exact logs and allocation totals.

The 203-unit full-spawning link still has 34 graphics/cache dependencies after
native socket/handle integration. Thread handles and native BSD endpoints are
real services with bounded scope; they do not establish XNet/P2P networking.
Whole external bitmap-file reads remain a prototype constraint incompatible
with the eventual 256 MiB minimum-memory target; add ranged loading before
claiming that target. Full runtime startup and actual biped spawning remain next.


## Full constructor milestone (2026-10-15)

The native Blood Gulch biped and both initial weapons now execute the actual
object_new/type/unit/biped path, including four attachment lights. Intel and
both G4s pass 49 checks through two map cycles and complete object/light cleanup.
See the latest entries in `ppc-mac-progress.md` and
`ppc-mac-renderer-spawn-boundary.md` for evidence and exact limits. No simulation
tick, controlling player or visible rendering has been established yet.

## Local player association milestone (2026-10-15)

`tools/mac_object_runtime_probe.c --player` extends the existing `--spawn`
constructor test without changing its 49-check mode. The same native Blood
Gulch biped is assigned to a real local player using `player_new(0, NONE, 0,
NULL)`, `local_player_set_player_index`, and `players_set_local_player_unit`.
The machine argument is a valid local machine slot, not NONE. Checks observe
the machine table, local-player slot, player/unit ownership in both directions,
the player-control unit, and actively-controlled/controllable flags produced by
the engine. No test writes these flags or substitutes a gameplay callback.

Detaching uses the same real player-unit API, then clears the local slot,
removes the machine association and deletes the player datum before ordinary
constructor-test object cleanup. Both map cycles end without player records,
objects or lights. No game-globals tag is needed by this association-only path;
that does not imply movement or player spawning policy can omit it.

The MacBook-built Intel executable and identical PPC executable on PowerBook
and iBook each passed **85 checks, zero failures**. Logs are
`build/mac-audit/player-runtime-{build,powerbook,ibook}.txt`. Reproduce with
`sh tools/mac_object_runtime_checks.sh <bloodgulch.map> <bitmaps.map> --player`
on the staged MacBook checkout. No input sampling, simulation tick or visible
rendering was exercised, so a playable session is still unverified.

## Real player action queue milestone (2026-10-15)

`--player-queue` preserves the association test and invokes the actual
update-server/client queue lifecycle against its real local player. Across two
map cycles, Intel and both G4s passed **91 harness checks plus 20 queue checks
per cycle**, zero failures. Evidence:
`build/mac-audit/player-queue-runtime-{build,powerbook,ibook}.txt`.

The queue checks deliver facing/throttle and peak trigger values, retain a
brief button press across input frames, suppress repeated one-shot presses,
rearm after release, and reject a second dequeue after exhaustion. Clock and
connection state are restored and the queues are deleted before player cleanup.
`engine_menu_state.c` provides the actual shared menu activity setter/getter
with explicit scoped flag ownership; the harness verifies both TRUE and FALSE.
This subset must not link alongside the complete UI widget owner and does not
implement UI widgets or menus. No queued action is yet applied to the unit in
this queue-only mode, and no simulation tick is claimed.

## Native game-globals and player-control initialization (2026-10-15)

`--player-control` extends queue mode by finding the real matg root in Blood
Gulch, decoding its complete owned storage, and publishing it into the same
native registry before either source mapping is closed. The runtime temporarily
binds `global_game_globals` to that root and restores the previous pointer before
registry teardown. This proves the player-control fields survive mapping release;
it does not load matg's transitive sound, HUD, effect or other dependencies.

On each map cycle the actual `player_control_initialize_for_new_map` runs before
player association. All four local control slots report NONE, and actual engine
look-rate globals receive the native tag's 60-degree pitch and 120-degree yaw
rates. The runtime also observes its six-point look curve. The normal real player
association and 20-check input queue lifecycle then execute as before.

MacBook Intel and both G4s each pass **123 harness checks plus 20 queue checks per
map cycle**, zero failures, across two cycles. Evidence is
`build/mac-audit/player-control-runtime-{build,powerbook,ibook}.txt`.
Reproduce with `sh tools/mac_object_runtime_checks.sh <bloodgulch.map>
<bitmaps.map> --player-control` on the staged MacBook checkout.

Queue-to-unit/PVS application remains a separate opt-in research path guarded
by `MAC_PLAYER_QUEUE_APPLY` in the helper. It is not compiled in these verified
runs. Its real `players_update_before_game` retention currently needs genuine
`hud_picked_up_weapon` and `hud_picked_up_powerup` providers; no substitutes were
added. These results establish real control initialization and queued inputs,
not applied unit movement, a simulation tick, or a playable rendered session.

## Next bounded action/PVS gate: HUD closure audit (2026-10-15)

This is source inspection, not a new build or hardware result. The optional
`MAC_PLAYER_QUEUE_APPLY` path retains `players_update_before_game()` and its
conditional pickup handlers. The observed next link errors are
`hud_picked_up_weapon` and `hud_picked_up_powerup`; both genuine providers are
in `source/interface/hud.c`. Each checks the local-player index against NONE
and calls the existing `hud_add_item_message(local_player_index, definition,
0, 0)` in `source/interface/hud_messaging.c`. That routine writes the real
per-player message array, calls the private `find_free_hud_message()` for
reuse/oldest-slot selection, timestamps with `game_time_get()`, and advances
the message sequence byte. It does not load tags or render while enqueueing.
The messaging translation unit is already in the genuine provider batch.

The recommended next increment is to compile the complete actual `hud.c`
with the existing engine flags and dead stripping, retaining its pickup
providers and the resulting real message insertion dependencies. Reconcile
its initialized `hud_globals` owner with the existing tentative declaration
in `engine_render_globals.c`; do not replace a duplicate with opaque storage
or introduce empty pickup handlers. This expected closure has not yet been
built, so further imports or compiler boundaries must be reported as observed.
Keep `MAC_PLAYER_QUEUE_APPLY` separate from the verified queue/control baseline.

For the first no-fire, no-action, no-equipment action, these pickup branches
should remain unexecuted. Check the real player's unit association,
controllability, enabled input, absent parent, and valid native cluster before
invocation. Then call the real player update, inspect the unit's stored
throttle and trigger, and compare every actual cluster bit of both combined
PVS arrays against the union of the player's BSP PVS and any activating
cluster PVS. Do not initialize an invented HUD root to satisfy unused pickup
branches. A passing result would prove action application and visibility
composition, not biped movement or a complete simulation tick.

Actual pickup-message execution needs genuine messaging lifecycle first:
`hud_messaging_initialize()` allocates its 0x11A8-byte private record through
`game_state_malloc()`, and `hud_messaging_initialize_for_new_map()` binds
`hud_msg_def` to `hud_globals->messaging` and clears that record. The latter
cannot safely run with a NULL HUD tag. For complete HUD initialization,
`hud_initialize_for_new_map()` resolves a real `hudg` through the first
`matg.interface_tag_references` record's `_interface_hud_globals` reference.
That reference block, the referenced `hudg` scalar/block fields, and any
subsequently used UTF-16 string lists, HUD message text, fonts, bitmaps, and
sound references require validated native conversion and registry ownership.
Full `hud_initialize()` additionally initializes unit/weapon interfaces,
nav-points, and the motion sensor; it is a broader lifecycle than message
insertion. The current bounded movement gate must not claim that lifecycle,
HUD rendering, or pickup audio is operational.

## Actual unit-control and native PVS application (2026-10-15)

`--player-apply` now compiles the complete genuine `source/interface/hud.c`
provider and explicitly enables `MAC_PLAYER_QUEUE_APPLY`. It linked on PPC and
Intel without additional substitutes or changes to the HUD owner; its real
initialized `hud_globals` definition coexists with the existing tentative
renderer declaration. Other runtime modes leave this optional path disabled.

The test starts with the real player/control/native-matg lifecycle, verifies
controllability and enabled input, queues a no-fire/no-action input, and calls
actual `players_update_before_game`. Observed unit throttle is `(0.375,-0.125,0)`;
trigger and action flags remain zero. The player cluster becomes the biped's
native cluster 3. Every bit in both real combined PVS arrays matches the union
of the native BSP cluster PVS and any activating-cluster PVS: Blood Gulch has
30 clusters, with four hidden for this placement. No visibility bits or unit
flags are fabricated by the test.

The MacBook Intel executable and identical PPC executable on PowerBook and
iBook each passed **123 harness checks plus 87 action/PVS checks per map cycle**,
zero failures, through two cycles. Evidence:
`build/mac-audit/player-apply-runtime-{build,powerbook,ibook}.txt`.
Reproduce with `sh tools/mac_object_runtime_checks.sh <bloodgulch.map>
<bitmaps.map> --player-apply` on the staged MacBook checkout.

This supersedes the earlier unresolved HUD link observation for this bounded
path. Pickup messaging itself remains uninitialized and unexercised: the action
contains no pickup/fire buttons, and the real branch is not entered. No physics
or object update tick was run, so applied control and visibility are established,
not movement, rendered gameplay, or a complete playable session.

## Full simulation startup boundary (2026-10-15)

Source audit of `source/game/game.c` distinguishes the bounded callback tests
from the eventual complete session. `game_initialize()` allocates private game
state and initializes the game engine, interface, scenario, camera, renderer,
objects, effects, sound, AI, scripting, input and other services. The existing
constructor harness initializes only its explicit subset; it must not also call
this entry point without first consolidating lifecycle ownership.

`game_load()` sets the private map-loaded state only after `scenario_load()`
succeeds. That loader obtains the complete scenario and globals tags, then
switches the actual BSP. `game_initialize_for_new_map()` runs the corresponding
service map lifecycles before setting active state and placing scenario objects.
The partial scenario in `engine_world.c` currently provides type, player starts
and BSP references; its zero-filled unconverted blocks cannot stand in for a
complete scenario during this startup path.

`game_tick()` requires that genuine active lifecycle and updates units, players,
AI, effects, first-person weapons, game rules, scripts, all objects and HUD.
`objects_update()` also reads private game state, updates cluster activation and
decals, then dispatches the object callbacks. A successful isolated biped callback
therefore cannot establish either complete update entry point. Do not bypass
these boundaries by assigning active/map-loaded flags or skipping callbacks.
The recommended progression is action application, bounded real biped physics,
complete required native HUD/scenario dependencies, then consolidation under the
actual initialization/load/map lifecycle before full-tick validation.

## Native HUD runtime and movement attempt (2026-10-15)

`mac_native_hud_globals_decode` owns the complete 1,104-byte HUD globals root,
its button-icon and waypoint-arrow blocks and reference names. Blood Gulch's
`e7d50661` owns 2,810 bytes, converts 339 values, has 18 icons and 10 arrows,
and reports ten dependency references. Digest `eb225dce` matches Intel and
both G4s; all pass 23 checks including malformed block/address/reference,
anchor and nonfinite duration rejection. References are not a claim of loaded
fonts, sound or other HUD resources. Evidence: `native-hud-{build,powerbook,ibook}.txt`
and the corresponding `native-hud-regression-*` logs in `build/mac-audit`.

The action runtime resolves HUD globals from matg's actual interface reference,
publishes owned native storage before source unmapping, and scopes the real
`hud_globals` pointer. It calls real messaging allocation, map reset and dispose.
`tools/mac_hud_runtime_probe.c` compiles the entire unchanged messaging source
instead of its standalone object, allowing checks against its actual private
records without a copied layout. Actual weapon and repeated ammunition pickup
calls produce two messages, a quantity of 12, the current timestamp and sequence
count 3; each new map starts clear. This does not draw text or play pickup audio.

MacBook and both G4s pass 125 harness checks plus 87 queued-action/PVS checks
and 13 HUD checks per map, for two map cycles, zero failures. Evidence:
`build/mac-audit/player-hud-runtime-{build,powerbook,ibook}.txt`. Reproduce with
`sh tools/mac_object_runtime_checks.sh <bloodgulch.map> <bitmaps.map> --player-apply`.

The separately attempted `--player-movement` invokes the full real biped callback.
Intel first failed in `cinematic_in_progress`; adding genuine cinematic allocation
and per-map lifecycle resolved that dependency. The next observed fault is
`game_players_are_double_speed` accessing game.c's uninitialized private game
state, called from `biped_update_moving`. Backtraces are in
`player-movement-debug.txt` and `player-movement-cinematic-debug.txt`. The callback
has not completed, and displacement is unverified. Do not present this diagnostic
mode as a working movement path or rerun it unchanged on every automation.
Continue with genuine game initialization and its dependencies, without forcing
active/map-loaded flags, replacing getters with constants or skipping animation.

## Core-state initialization and actual movement verified (2026-10-15)

The previous movement faults are resolved in the isolated runtime. The complete
native game.c compilation view factors the exact existing allocation/zeroing
prefix into a shared initializer, leaving the rest of full `game_initialize`
unchanged. The generator rejects any change to the expected prefix. The scoped
native owner provides the real declared variant, refuses duplicate initialization,
and refuses teardown of an active/loaded game. It restores the saved variant and
clears the private pointer before freeing the arena. No replacement getter or
forced active/map-loaded state is used. Generate the required compilation view
with `python3 tools/mac_game_state_boundary.py build/mac-audit/mac_game_state_body.h`
on the modern host, and stage it to the MacBook before building.

Following the actual callback failures added real cinematic, breakable-surface
and collision-log lifecycles. The latter establishes the engine's base collision
user context; callback push/pop balance is now verified. No physics, animation,
collision or footstep callbacks are removed. The stock callback executes against
the owned native model/animation/matg/collision data and previously applied input.

MacBook Intel and both G4s pass eight consecutive biped callbacks in each of two
map lifecycles: 66 movement checks per map, 129 harness checks, 87 action/PVS
checks and 13 HUD checks per map, all with zero failures. All eight updates retain
support surface 1412 and cluster 3. Final logged position is
`(98.679604, -157.54628, 1.7034857)` and velocity is
`(0.02625907, 0.013073682, 0)` on each machine. This tests finite and bounded
movement, ground support, cluster validity and collision-context preservation.
It is not a full object/game tick, animation visual validation, arbitrary-world
collision proof or a playable session. Evidence:
`build/mac-audit/player-movement-eight-{build,powerbook,ibook}.txt`.

Reproduce using the staged `tools/mac_object_runtime_checks.sh` with the two
retail maps and `--player-movement`. The script still supports the prior action
and spawning gates. Lower-cost subagents reviewed the core initialization seam
and collision dependencies; the separate initial full-startup link census is
recorded in `ppc-mac-game-startup.md`. Next work is integration into genuine
object-update/service lifecycles and then complete map/game startup, without
interpreting this bounded callback sequence as full simulation.

## Collision-material completeness and object-update guard (2026-10-15)

A review before broadening to `objects_update` found that the scoped world lacked
BSP collision materials. Movement was real, but collision queries could fall back
to an unknown material. The world now decodes the entire material record array
using the clean native 20-byte structure (tag reference, pad, short type at +18),
with layout witnesses. All eleven Blood Gulch types agree with the referenced
shader physics type at +34. Shader reference strings are copied and freed with
the world; no map pointers remain. Surface material indices and shader metadata
are checked before publishing. The root independently confirmed surface 1412
references material slot 2, type 7, in the raw installed version-7 map.

The updated movement gate performs an actual structure collision ray after eight
callbacks and checks surface 1412 / thick-metal type 7, plus the owned shader name
after source unmapping. Intel, PowerBook and iBook pass 130 harness checks plus
72 movement, 87 action/PVS and 13 HUD checks per map, across two map cycles.
Evidence: `build/mac-audit/movement-material-{build,powerbook,ibook}.txt`.

The optional object-update path links genuine `hud_unit.c` and
`structure_runtime_decals.c` and their initialization functions. However, actual
map inspection shows eight runtime decals and four scenario palette entries,
with four decal references each in clusters 3 and 19. The zero-filled unconverted
fields must not silently suppress this content. The runtime now checks the world
capability report and refuses `--object-update` before callbacks for this map.
All three machines return expected status 3 and log that dependency. These are
refusal tests, not successful object-update runtime validation. Fog-related
blocks and flagged fog references are absent here. The capability predicate is
only a necessary data gate, not proof that every broader service is initialized.

Next convert and validate real runtime decals, per-cluster ranges, scenario palette
and referenced decal definitions/resources, initialize their genuine services,
then remove this specific blocker through data readiness and run object updates.
Do not clear their counts, suppress the callback, or reinterpret the successful
bounded biped test as full game simulation.

## Scenario multiplayer metadata subset (2026-10-16)

`native_scenario_gameplay.c` now converts the three populated Blood Gulch
scenario blocks consumed by the stock game-engine multiplayer setup: 191
netgame flags, 37 netgame equipment records and two starting-equipment records.
It uses the source `scenario` offsets and structure sizes as compile-time
witnesses, checks block counts and bounds, finite poses, flag/game-type/team
domains, and every nonempty `itmc` reference. Tag names are owned copies; an
empty typed item-collection reference may retain its `itmc` group with a NONE
handle. The runtime-only spawned-item index starts at NONE. The world attaches
these owned arrays to its scoped `scenario` and releases them with that world.
The existing 72 player starts remain supplied by `retail_spawns`; they are not
duplicated here.

The focused check reports 445 passing checks on Intel, PowerBook G4 and iBook
G4. It also rejects a count above the source limit, an out-of-range flag enum,
and a reference whose group disagrees with its actual tag. Evidence is in
`build/mac-audit/scenario-gameplay/{build-and-i386.log,powerbook.txt,ibook.txt}`.
This proves bounded conversion and world attachment only. It does not call
`scenario_tags_load`, `scenario_load` or `scenario_switch_structure_bsp`, spawn
netgame items, or make the scenario a fully loaded playable map. The later
world-graph batch converts owned sky-reference records and attaches the empty
Blood Gulch script arena under a scoped cache-membership lease; it does not load
the sky definition or make compiled scripts executable. Triggers and other
scenario blocks remain unconverted.

## Owned sky references and scenario-script cache lease (2026-10-16)

The scoped world now owns the scenario's sky-reference array and copied tag
names. `mac_engine_world_sky_handle` returns a validated salted handle for an
external loader, without claiming the referenced `sky ` payload or its render
dependencies are available. The graph probe compares native handles against
the raw cache and rejects malformed count, group and handle mutations.

The same world scope binds `engine_script_cache` only while the callback runs.
The provider accepts converted syntax/string allocations and rejects raw
source-map pointers, nested leases and mismatched releases. The combined graph
probe passes 12,018 checks on Intel and both G4s against the same version-7 map.
Evidence is in `build/mac-audit/scenario-scripts-world/`. This is not a complete
scenario cache load or playable map startup.
