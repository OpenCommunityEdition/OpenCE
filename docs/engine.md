# The native engine

The native engine (`engine/`) is a new engine in Rust and Swift. It does not
use a 32-bit VM, and it imports the user's maps into a typed 64-bit format.
The decompilation in `source/` is a reference for how the game behaves; none
of its code is compiled into the engine.

## Building and testing

```sh
nix build .#engine             # OpenCE Engine.app
python3 tools/engine_build.py --out build/engine --test   # the app and its Swift tests
cd engine && cargo test --release --workspace             # the Rust tests
cd engine && cargo clippy --release --workspace --all-targets -- -D warnings
cd engine && cargo deny check licenses
```

Run them in `nix develop`. Tests that need game data read `assets/maps`
and pass without it. `unsafe` code is refused, every public item needs a
document comment, and `unwrap` and `expect` are refused outside tests.

## The architecture

The engine is Rust crates (`engine/crates/`) under a Swift app
(`engine/app/`). Each crate depends only on the ones above it in this table:

| Crate | What it does |
| --- | --- |
| `content` | The typed data a level is made of, and the `.ocpak` pack it is kept in |
| `import` | Reads a stock cache (Xbox or Custom Edition) into `content`, with bounds-checked readers; keeps the cache folder |
| `physics` | Jolt Physics, through its C interface: the level's collision mesh, bodies, rays and shape casts |
| `script` | HaloScript's runtime: threads, special forms, and a registry of natives |
| `sim` | The simulation: a `bevy_ecs` world on a fixed 60 Hz tick (objects, players, weapons, projectiles, damage, vehicles, ragdolls, animation) |
| `net` | The network protocol: packets, delta-compressed snapshots, checks and rate limits |
| `campaign` | A mission: the scenario's scripts over the simulation, the AI's actors, the game's flow and saves |
| `ffi` | The contract with Swift: UniFFI calls and the per-frame snapshot |

The Swift app has these modules:

| Module | What it does |
| --- | --- |
| `OpenCEEngineCore` | The UniFFI bindings (generated) |
| `OpenCEEngineKit` | The snapshot reader, the controls, the map library |
| `OpenCEEngineRender` | The Metal renderer: the level, models, the HUD, temporal scaling, captures, the benchmark |
| `OpenCEEngineWindow` | The window and its view |
| `OpenCEEngineMenu` | The menus and settings |
| `OpenCEEngine` | The app's entry |

The simulation is deterministic: its systems run on one thread in a fixed
order (`SimSet`), and its maths is `libm`'s and glam's, never the
platform's. The same inputs from the same state give the same world, and
the multiplayer and the tests rely on that.

### Ticks

- The simulation steps at 60 Hz (`opence_sim::clock::TICKS_PER_SECOND`).
  Rates the tags give a second (fire rates, reloads, heat) are converted to
  these ticks.
- The scripts and the game's flow run at Halo's 30 Hz
  (`opence_script::TICKS_PER_SECOND`), on every second simulation tick.
  `Mission::game_tick` is that clock. `game_time`, `sleep`, the recordings,
  the custom animations and the sound lengths count in it.

## The contract

`opence_ffi` is the only crate Swift sees:

- **Calls** go through UniFFI: load a map, start a game, input, settings,
  the network, the campaign. The bindings are generated into
  `OpenCEEngineCore` and `opence_ffiFFI`.
- **The per-frame data** is the snapshot (`ffi/src/snapshot.rs`). It is made
  of `#[repr(C)]` types that cbindgen writes into `opence_snapshot.h`. After
  each tick the simulation publishes a snapshot into a triple buffer, and
  Swift reads the newest one in place. Neither side waits and nothing is
  copied.
- Swift never sees the simulation's world. Only the simulation's own types
  cross into `ffi`, which turns them into the snapshot.

## The content format

A level is imported once and kept in the cache folder as
`<map>-<fingerprint>.ocpak`.

- **The pack:** a 40-byte header, then an rkyv archive of
  `opence_content::Level` (its scenario, structure BSPs, object
  definitions, models, animation graphs, shaders, bitmaps, skies, globals,
  effects and HUDs). `content/src/pack.rs` describes the header.
- **Checks:** a pack is checked before any of it is used: its header, its
  length, its fingerprint, then the archive itself by bytecheck. A damaged
  or hostile pack is an error, never a crash.
- **References:** nothing in a level points into a cache. Every reference is
  an index into one of the level's own lists, and the importer has checked
  each one.
- **Versions:** a pack records two versions. `FORMAT_VERSION`
  (`content/src/pack.rs`) is the layout. `IMPORTER_VERSION`
  (`import/src/level_cache.rs`) is what the importer reads. Raise the first
  when a `content` type changes, and the second when the importer reads
  something new. Either change makes every map be imported again.

## Adding to the engine

### An object kind

The kinds are `ObjectKind` (`content/src/object.rs`), in the game's order
(`ObjectKind::ALL`, the cache's type field).

1. **Name it.** Add the kind to `ObjectKind` and to `ALL`, with its tag
   group in `ObjectKind::group`.
2. **Read its definition.** In `import/src/object.rs`, read the fields it
   adds to an object's, through the tag's checked readers. If the scenario
   places it, give its placement block in `PLACEMENT_BLOCKS`
   (`import/src/level.rs`).
3. **Make its objects.** Give it a hook in `ObjectKinds::default`
   (`sim/src/objects.rs`): the components its objects are made with. Its
   systems are added to the schedule in `sim/src/lib.rs` in the right
   `SimSet`. State it keeps goes into `Simulation::world_hash`, which the
   tests use to check that two runs give the same world.
4. **Raise the versions** and test the importer with a fake cache
   (`import/src/fake.rs`), and the simulation with a placed object.

### A weapon behaviour

- **Its fields.** A weapon is `content::Weapon`, with its triggers, its
  magazines and their flags (`content/src/weapon.rs`), read in
  `import/src/weapon.rs`. A behaviour the tags turn on is a flag constant
  there or a new field. Raise the versions when you add one.
- **What a tick does.** `WeaponState::step` (`sim/src/weapon.rs`) steps the
  triggers, the magazines and the heat, as `weapon_update`
  (`source/items/weapons.c`) does, and returns the tick's `Shot`s. Rates are
  per 60 Hz tick.
- **What a shot makes.** `shoot` (`sim/src/player.rs`) turns each `Shot`
  into projectiles and effects. The projectiles and their impacts are in
  `sim/src/projectile.rs`.
- **Its tests.** Test the behaviour with a weapon built in the test, as the
  tests at the end of `sim/src/weapon.rs` are.

### A script function

1. **Its signature.** Give its name, its type and its parameters' types in
   `campaign/src/signatures.rs`, exactly as Halo declares them
   (`source/hs/hs.c`). A scenario compiled against those declarations then
   calls it.
2. **Its native.** Write the native in the module of the system it
   belongs to (`game.rs`, `objects.rs`, `units.rs`, `devices.rs`,
   `cinematic.rs`, `ai.rs`), as a
   `fn(&mut Mission, &[Value]) -> Value`, and list it in that module's
   `NATIVES`. A function that only the player would see or hear is
   listed in `presents!` in `presented.rs` instead, and becomes an `Event`
   for the app to show.
3. **What `registry()` does with it.** `registry()` (`campaign/src/lib.rs`)
   registers every native that has a signature. A function the scenario
   names but nothing registered is reported by `Runtime::missing` and
   returns its type's default. `campaign/tests/levels.rs` requires none to
   be missing on the first two missions.

## The first two missions

`campaign/tests/levels.rs` plays *The Pillar of Autumn* (`a10`) and *Halo*
(`a30`) from start to end. The player is a pilot that does what the
waiting scripts wait for:

- it walks into each trigger volume they test;
- it presses each button and looks around;
- it kills the enemies they wait to see dead.

The scripts must win the mission. Every function they call must be given,
and no thread may end with a fault.

### What it relies on

- **Volume tests:** an object is in a trigger volume when its
  bounding-sphere centre is (Halo's test). For a player, that centre is its
  unit's.
- **Sounds:** a scripted sound lasts as long as its longest permutation,
  which the importer reads from the `snd!` tag. `sound_impulse_time` counts
  down from that.
- **Structures:** the structure switches when a player stands in a
  switching volume whose source is the loaded structure
  (`Mission::switch_structure`).

## The renderer

### Skies

- A level's first sky is drawn first, at the far plane, centred on the camera.
- Its shaders are drawn unlit, at the model's highest level of detail.
- The sky has no depth test and writes no depth, so the level and the objects
  draw over it.

### Fog

The engine uses the outdoor fog of the level's first sky for the whole level.
It does not follow the fog planes and fog palettes of each cluster. Fog grows
linearly with the distance along the view, from the fog's start to its
opaque distance, up to the fog's maximum density.

Each blend mode of a transparent shader fogs differently, so that the fog is
not counted twice:

| Blend | Fog |
| --- | --- |
| Alpha blend, opaque | Mixed toward the fog's colour |
| Add | Faded toward black |
| Multiply | Faded toward white |
| Double multiply | Faded toward grey (0.5) |

### Transparent shaders

- **Transparent chicago and extended chicago** (`schi`, `scex`):
  - Up to four maps are combined in order, each by the previous map's colour
    function and alpha function.
  - Maps are drawn unlit.
  - An extended chicago is drawn the same way, from the maps the importer reads.
- **Transparent generic** (`sotr`): not imported. Its parts are skipped in the
  sky and drawn as plain surfaces elsewhere.
- **Blending on models:** model parts are blended with alpha blending only.
  The level's transparent surfaces have every blend mode.

### The material model

The importer writes one material record (`content/src/material.rs`) for every
shader of every class. It is the only record the lighting reads, and its
colours are linear.

| Field | From |
| --- | --- |
| Albedo map and colour | The shader's base or diffuse map, and its tint |
| Normal map and scale | The bump map's heights, with the bump map's scale |
| Roughness | The kind of reflection: bumped cube map 0.3, flat cube map 0.15, the radiosity reflection 0.6, none 1 |
| Specular at normal and at grazing incidence | The reflection's perpendicular and parallel brightness and tint. A tinted specular stands in for metalness |
| Reflection map | The cube map the game reflected |
| Emission map and colour | The self-illumination map and colour (black: none) |
| Flags | Alpha tested, blended, two sided, decal, unlit |

The game's shaders are not physically based. The record keeps what each one
shows: environment (`senv`) and model (`soso`) shaders are lit and reflect;
transparent ones (`schi`, `scex`, `sotr`, `sgla`, `smet`, `spla`, `swat`) keep
their blend modes. A model shader's multipurpose map is read in the Xbox's
channel order, and its change colours come from the object's functions in
the snapshot.

### The bake

The engine bakes each structure BSP's light itself; the stock lightmaps are
only a fallback.

1. **The scene** (`crates/bake`): the BSP's triangles in a bounding-volume
   hierarchy, their materials, the sky's lights, the sun and the shaders'
   radiosity emitters.
2. **The tracer** (`PathTracer.swift`): a Metal compute path tracer over the
   hierarchy, with next-event estimation and bounced light. It traces 64 paths
   for each lightmap texel, 256 for each probe axis and 16 for each reflection
   probe texel. A CPU tracer checks it in the tests.
3. **The outputs:**
   - directional lightmaps with ambient occlusion, as half floats, with a
     second texel for the light's direction;
   - an irradiance probe grid over the BSP's volume, for what moves;
   - a reflection probe for each cluster.

   The probes leave out the sun's direct light and keep how much of the sun
   each sees, because the shadow maps draw the sun on objects.
4. **The cache:** the result is written to the cache folder. Its name is a key
   of the importer's and the baker's versions (`FORMAT_VERSION`,
   `BAKE_VERSION`), the settings and the scene, so a changed level or baker
   bakes again.
5. **In the background** (`LightBaker.swift`): the app bakes a level that is
   not cached while it draws the stock lightmaps, and switches over when the
   bake is done.

**The mood check** (`BakeTests.swift`, which needs `assets/maps`): each page's and
BSP's mean brightness and colour must stay within set limits of the stock
lightmaps. Two are listed as known colour differences: longest, and d20's
second BSP.

### The passes

A frame is drawn in this order. Each switch is a shell variable
(`set render.bloom 0`). `render.view` shows one buffer alone: `frame`, `depth`,
`normals`, `ao`, `contact`, `ssr`, `luminance` or `bloom`.

| Pass | Does | Switch |
| --- | --- | --- |
| Sun shadows | Cascaded shadow maps for objects, matched to the baked shadows; spot and cube shadows for the nearest dynamic lights | `render.sun_shadows` |
| Sky | The first sky, unlit, at the far plane | |
| Level and objects, opaque | Baked light, or probes and the sun for objects; clustered dynamic lights from weapons, projectiles, effects, the flashlight and scenery | `render.lightmaps`, `render.lights`, `render.objects`, `render.wireframe` |
| Decals | The scenario's decals and those that shots and explosions lay, deferred onto the level and objects | `render.decals` |
| Screen-space effects | Ambient occlusion (GTAO) and contact shadows, at half resolution by default, brought up with depth weights; reflections over the reflection probes | `render.gtao`, `render.ssr`, `render.contact_shadows`, `render.ao_quality` (`half`, `full`) |
| Blended | The level's transparent surfaces, the models' blended parts, glows and light volumes, the first-person weapon | `render.glows` |
| Particles | Particles, contrails, weather and detail objects, blended in the shader by each one's blend function | `render.particles` |
| Temporal | MetalFX's temporal scaler, from the motion vectors | |
| Camera | Auto exposure, bloom and lens flares | `render.exposure`, `render.bloom`, `render.lens_flares` |
| Tone map and HUD | To the display, then the HUD over it | `render.hud` |

### Effects and particles

- **Effects** (`effe`) run in `sim` (`crates/sim/src/effects.rs`). Weapons,
  projectiles, explosions, scenery and vehicles start them; an impact picks its
  effect by the material it strikes. Each event fires its parts (lights,
  decals, particles, contrails) at its time.
- **Particles** (`part`) follow their sequence, colour and size over their
  lives, and move by their velocity and gravity. They are drawn as quads
  facing the screen.
- **Contrails** (`cont`) are strips of points. Each segment is a quad along it
  that turns to face the camera.
- **Weather** (`rain`) falls in a box around the camera, in the clusters that have it.
- **Detail objects** (`dobc`) are placed by the importer and drawn with the
  frame's particles.

The snapshot carries up to 2048 particles a frame.

These are not done yet:

- particle systems (`pctl`);
- particles start at the effect's origin, not at its markers;
- contrails do not take on the velocity of what leaves them;
- particles do not collide;
- weather ignores its polyhedra and its height fades;
- detail objects are blended and not sorted;
- particles are drawn after the first-person weapon, so they can draw over it.

## Animation

### The graph

An animation graph (`antr`) is imported whole into `content`: its nodes,
its animations with their frame data, and the lists that name them by
seat, weapon class, weapon type, weapon, vehicle and device. Halo's graphs
inherit nothing from one another (the inheritance block came with Halo 2),
so a missing animation falls back only within its own graph, as
`sim/src/animation_graph.rs` finds it: by the unit's seat, then its weapon
class, then its weapon type, each list's entry checked against the graph.
With `game.enhanced_animations` a reload a weapon type lacks is borrowed
from the standing seat's same class and type. An animation is refused, and
the pose left as it was, when it has no frames, is of another kind than
asked, or moves other nodes than the model has.

An animation's frames are read as the game reads them
(`model_animations.c`): a base frame into each node's orientation (a
rotation, a translation and a scale), then replacement and overlay
animations over it. Every offset and count comes from the map and is
checked, so a broken animation draws the rest pose and never reads past
its data.

### The blender

`sim/src/animation_blend.rs` builds an object's pose in the order
`object_get_node_orientations` takes it:

1. the base animation's frame;
2. the graph's object overlays, by the object's functions;
3. a unit's layers, in `unit_preprocess_node_orientations`'s order: the
   aiming and looking screens, the action and ping animations, a
   vehicle's steering, throttle, wheels and suspension
   (`vehicle_animation.rs`), a device's position and power
   (`device_animation.rs`);
4. the blend from the last pose, over the frames a change asked for;
5. the nodes' matrices, from the root's transform down.

What a unit plays is chosen by `unit_states.rs` (the biped's state, by its
seat and weapon class), `first_person_states.rs` (the first-person arms
and weapon), `seats.rs` (entering, riding and leaving) and
`ai_impulse.rs` (an actor's alerts, dives, evades and berserk). A
cutscene's `custom_animation` starts at a frame (or the first) and may
blend in from the pose before it. The scripts' camera follows the root
node of a one-node graph's animation, at the frame its timer has reached
(`camera_set_animation`).

The test map's `animation` station plays every animation of the player
biped's graph in turn, named; `ffi/tests/stations.rs` checks that each
one plays.

## Sound

The `audio` crate plays a level's sounds; nothing of Bungie's audio is
shipped, and every sound is read from the player's maps. A sound's
permutations are decoded on their first play and kept: Xbox ADPCM (the
stock maps'), 16-bit PCM and Ogg Vorbis (Custom Edition's). What plays each
time is chosen as `sound_definitions.c` chooses it (`audio/src/choice.rs`):
the pitch range nearest the sound's pitch, a permutation of it not repeated
until every other has played, and the gain and pitch its scale gives. The
51 sound classes (`classes.rs`) limit how many of a sound play at once and
set its priority, its distances and how much reverb it takes.

The mixer (`mixer.rs`) resamples each voice to 48 kHz at its pitch,
attenuates it by its distance, pans it with equal power as the port's
DirectSound voices are panned, bends it by its motion (Doppler), delays it
by the speed of sound and muffles it by what stands between it and the
listener. The voices in the world are sent on to the room's reverb
(`reverb.rs`), a Schroeder and Moorer reverb standing in for the Xbox's
I3DL2 one. The app's Core Audio node pulls the mix on its own thread,
behind a lock of its own (`ffi/src/audio.rs`), so that a tick never waits
for it.

### The world's sounds

A tick queues the sounds it makes (`ffi/src/sound_events.rs`): its
effects' sound parts, the sounds its animations reach, and its footsteps,
by the material underfoot. They are played after the tick. Around the
camera (`ffi/src/sound_world.rs`), the objects' looping sounds and the
rooms' backgrounds are refreshed each frame, each heard through the walls
between it and the camera, and the room's sound environment is glided to.
A looping sound (`audio/src/player/looping.rs`) plays its tracks' starts,
loops and stops, switches to their alternates, and plays its details now
and again around its source; it stops when a frame does not refresh it.

### Music and dialogue

The scripts' sounds are kept by the mission (`campaign/src/sounds.rs`) and
played by the app after the ticks (`ffi/src/campaign.rs`):

- `sound_impulse_start` starts an impulse from an object, or heard
  everywhere alike, and starting one again stops the first. It ends, for
  the scripts, after its longest permutation, counted in game ticks as
  `scripted_sound_time` counts it, so `sound_impulse_time` and the waits on
  it last as the original's do. `sound_impulse_stop` stops it.
- `sound_looping_start` starts the music, which the app refreshes each
  frame until `sound_looping_stop`; `sound_looping_set_alternate` switches
  its tracks to their alternates.
- `sound_class_set_gain` fades a class over the ticks given.

A revert to the last save silences the impulses started before it.

While a line of the scripted dialogue plays (classes 44, 46 and 47), every
other sound glides down to 70% of its gain, and back after it.
Conversations (`campaign/src/conversations.rs`, `ai_conversation`) speak
their lines in turn as `ai_conversation.c` does: each line waits for the
dialogue before it and a pause, or for `ai_conversation_advance` where it
is marked to, and plays from its participant's object, or heard everywhere
for the participants the original plays unspatialized. A participant's
speaker is found by its object's name, and a line's variant by the
original's rules.

The settings' master, effects and music volumes are applied by the app
(`setVolumes`). `campaign/tests/sounds.rs` checks, on *The Pillar of
Autumn* and *Halo*, that every sound the scripts start or wait on has its
length, that a wait lasts its longest permutation to the tick, and that
the music loops until it is stopped.

## Physics

The `physics` crate is Jolt Physics (`port/third_party/jolt`) through the
C interface in `port/macos/host/physics`. Jolt is built in its
cross-platform deterministic mode, without fused multiply-add, so the same
calls give the same results on every run and every machine. A `World`
holds a level's collision mesh, made from its collision BSP, and bodies in
layers: the level, the platforms (kinematic bodies the simulation moves)
and the bodies (dynamic ones Jolt moves). Rays and shape casts name the
layers they look for; `Layers::GROUND`, the level and the platforms, is
what stands under a player. Everything is in world units, converted to
metres at the boundary, and the level's materials are found by a point on
them (`ground.rs`), since Jolt's mesh does not carry them.

What moves in it, and by whose rules:

- **The player** walks, runs, crouches and jumps by Halo's biped rules
  (`movement.rs`), converted from its 30 Hz to the engine's 60, and its
  character collides with the level. Standing on a platform, it is carried
  at the platform's velocity.
- **Vehicles** move by Halo's mass points (`sim/src/vehicle.rs`), not
  Jolt's vehicle constraints, so that they handle as they did; they step
  at 30 Hz, on every other tick. A vehicle on its back is turned over when
  the player acts by it.
- **Items** lying about move as `items.c` moves them
  (`sim/src/item_physics.rs`): a point that falls, bounces and rests.
- **Ragdolls** are bodies of parts joined by swing-twist joints
  (`ragdoll.rs`), following the death animation for a moment before they
  go limp. **Debris** (spent casings) and the test map's **crates** are
  plain dynamic bodies.
- **Platforms** are the machines (lifts, doors, gates): a box for each
  node of their collision models, moved each tick to where the machine's
  animation put it (`sim/src/platforms.rs`). What stands on one is carried,
  and what one meets is pushed ahead.
- **Forces**: damage pushes what it strikes by its effect's acceleration
  (`sim/src/forces.rs`), and grenades bounce and detonate by their
  projectile's rules.

The particles ask `Simulation::collide_point` where a point's path meets
the level or a platform, and `particle_collision::rebound` puts the point
just off the surface with its velocity bounced by the particle's
elasticity and friction.

The physics station's tests (`physics/tests/station.rs`) check that a pile
of crates on the slope rests without jitter, settles again after
explosions and never sinks through the ground; that two runs with a lift
and explosions agree to the bit; and, in a release build, that 500 bodies
tumbling together step in under 2 ms. `sim/tests/crates.rs` checks that
two simulations given the same inputs agree, a player throwing a frag into
the pile.

## The Halo UI

The menus are the game's own: the widget definitions, bitmaps, fonts and
strings of the player's `ui.map` (and the pause menu every map carries),
imported into the cache as the levels are. Nothing from `ui.map` is
shipped. Fonts get a signed distance field each at import, so text stays
sharp at any window size.

`crates/ui` (`opence_ui`) is the widget code (`source/interface/ui_widget.c`):

- one tree of widgets for each local player, in an arena named by `Id`,
  with the history of screens each came from;
- focus moved through children and list items, and the handlers' functions
  run; the ones that reach the game (start a level, set the difficulty,
  host or join) are the `Host`'s, and the ones the engine has no use for
  return `Outcome::Unwired`;
- `draw.rs` lays each screen out at the game's 640×480 and scales it to the
  window; `pointer.rs` maps keys to the pad's buttons and gives the mouse
  hover, click, the wheel and right-click (back). There is no gamepad.

`OpenCEEngineMenu` holds the app's side: `HaloMenu` (the actions the
widgets run, and the next mission after each), the settings, the fonts and
the server list.

### The launcher

The window before the game has three choices: **Play**, **Settings** and
**Exit**. Its settings are only what must be set before the game starts:
the data folders, the display and the window mode. Every other setting is
in the game's own menus.

### The loading screen

`crates/ui/src/loading.rs` is `source/interface/progress_bar.c`: a band of
light pans across the screen as the map loads, its last frames left in a
fading trail, while the chant's four voices swell with the progress. The
game stepped it at 30 frames a second; here a frame of any length moves it
as many thirtieths of a second would (a frame over 0.25 s counts as 0.25 s).

It goes through four stages:

1. **Loading:** the map loads on another thread (16 MB of stack), and the
   loader reports its progress.
2. **Settling:** the map is ready, and the light finishes its sweep, for 3 s
   at most. Any key skips it.
3. **Crossfade:** the screen fades over the mission's frames for 0.5 s.
4. **Done:** the mission shows alone.

The world is held until the screen is done. With `ui.loading_screen` off,
the screen is plain: no light and no chant, and it fades as soon as the map
is ready. `ui.map` has no `loading.tga`, so the light pans over a white
field.

The chant is a tone of 511 blocks of Xbox ADPCM that the game carries in
its executable. The engine ships none of it. `ChantPlayer` looks for the
player's `default.xbe` beside each data folder. `opence_ui::chant` finds
the tone among the executable's sections by the shape of its blocks, and
knows it by its SHA-256. It decodes the tone into the cache's `chant.wav`
once. Each voice plays it at its own pitch. Without the executable the
screen is silent.

So that the first frame of a mission is no longer than the rest,
`LoadedLevel` draws one frame from the start camera at the window's size on
the loader's thread. That makes the size's targets and the temporal scaler
before the mission shows. `TransitionTests` draws a10, loads a30 behind the
screen, fades into it and draws a30, pacing each frame to 1/60 s. It fails
if any frame takes over 33 ms; the worst so far is 14.2 ms. It needs
`assets/maps`.

### Golden frames

`UiTests` writes the main menu at 1920×1080
(`build/engine/golden/ui_main_menu_1920x1080.png`), and `LoadingTests`
writes the loading screen half-way through its load at 1280×720
(`loading_screen_1280x720.png`). Each is compared with an approved frame in
`assets/golden/engine` when there is one. They are made from the player's
maps, so none is committed.

These are not done yet:

- fonts have no kerning, and text does not wrap;
- UI sounds are queued but not played;
- generated list items draw no background;
- profile editing and the controller settings are unwired;
- co-op, split-screen and the demos are disabled.

## The benchmark

`tools/test_engine_benchmark.py` runs `OpenCEEngine --benchmark 60`. The
engine flies a fixed path through bloodgulch for 60 seconds: a loop around the
first player start, rising and falling, looking ahead and down and up. It
draws offscreen at 3840×2160 with temporal anti-aliasing at full scale, with
the scenario's objects and the sky. No window opens.

- **Frames in flight:** up to three, as the window has.
- **Frame time:** the time between the ends of two frames' GPU work. It counts
  whichever of the CPU and the GPU is slower.
- **The bar:** p99 must be at most 8.33 ms (120 frames a second).

| Machine | Date | Frames | p50 | p99 |
| --- | --- | --- | --- | --- |
| M1 Max | 2026-10-21 | 8235 | 7.28 ms | 7.94 ms |
| M1 Max | 2026-10-23 | 5416 | 11.78 ms | 14.19 ms |

The first row was taken before the screen-space effects, the lights, the
exposure, bloom, decals and particles were drawn. The second has them all on,
as they start, and is **above the bar**. Its frame's own GPU time is 13.97 ms
at p50 and 17.06 ms at p99; the GPU overlaps one frame's work with the next.

Where a frame's GPU time goes, from a Metal System Trace of the second row:

| Work | ms per frame |
| --- | --- |
| MetalFX temporal anti-aliasing (its three passes) | 6.4 |
| The screen pass (occlusion, contact shadows, reflections, the resolve) | 2.9 |
| The level's opaque draws | 1.7 |
| The level's blended draws | 0.95 |
| Decals | 0.5 |
| Luminance and bloom | 0.5 |
| Tone map, motion | 0.75 |

MetalFX alone at 3840×2160 and full scale is most of the budget. With every
switch off (`--off render.gtao,render.ssr,render.contact_shadows,render.sun_shadows,render.lights,render.exposure,render.bloom,render.lens_flares,render.glows,render.decals,render.particles`)
p99 is still 9.1 ms. With every effect on, a lower render scale (which
MetalFX brings up to the window's size) comes near the bar: p99 is 10.54 ms
at `--render-scale 0.75` and 8.34 ms at 0.5 (12-second runs).

## The developer tools

The engine's tools reach into a running world: a shell in the game, the
same commands over a local socket, debug drawing, a test driver and a
generated test map. They are made in `opence_dev` (the registry, the
socket's limits, the scripts, the profiler, the debug shapes) and
`opence_ffi` (the engine's own commands, in `devtools.rs`), and drawn by
the app.

### The shell

`~` (the key left of 1) slides the shell down over the game. The world
keeps running unless it is paused, and keys go to the shell while it is
open. The prompt has line editing (word jumps, kill and yank), history
kept between runs in `shell_history.txt` in the app's support folder
(Up, Down, Control-R), Tab completion of commands, variables, tag paths,
objects and HaloScript's functions and globals, and scrollback with the
wheel. The engine's log, Rust's and Swift's, streams into it. Text is
drawn in Overpass Mono from a glyph atlas at the display's scale.

A line that is not a command is HaloScript, run against the mission:
`(ai_place x30_marines)` works as `spawn` does. `watch <expression>` puts
a variable or an expression on a line that stays on screen when the shell
is closed.

### Commands and variables

Every crate adds to the registry; `help [name]` lists them and their
arguments, and `vars [filter]` the variables.

| Command | Does |
| --- | --- |
| `map <name> [bsp]`, `bsp <index>` | Loads a map, or moves to a structure BSP with its scenery |
| `spawn <tag> [count]`, `kill [object\|all]` | Makes or kills objects |
| `teleport <x y z>\|<object>`, `noclip`, `god` | Moves and protects the player |
| `give <weapon>`, `enter <vehicle> [seat]` | Arms the player, seats them |
| `time <scale>`, `pause`, `step [ticks]` | The world's clock |
| `objects [filter]`, `inspect <id>`, `tags [filter]` | The world's objects and the level's tags |
| `frame <path> [width] [height]` | Saves the next frame to a PNG (1280 by 720 unless told; a relative path is the working folder's) |
| `log [filter]` | `render=debug,warn` sets the levels kept; alone, the last lines |
| `watch`, `unwatch` | The watch line |
| `debug_line`, `debug_box`, `debug_sphere`, `debug_text`, `debug_clear` | Shapes, each for some ticks |
| `stat frame [off]`, `trace <frames>\|stop` | The profiler's breakdown, and a Chrome trace for Perfetto |
| `script <file>\|stop` | Runs a test script in the window |
| `station [name]\|stop` | Runs a test map station |
| `set`, `vars`, `bind`, `unbind`, `exec`, `help`, `clear`, `quit` | The registry's own |

Variables are typed (bool, int, float, text, a choice) and some are saved
to the settings. The drawing switches are listed in "The
passes" above; the debug layers are `debug.draw.<layer>`.

### The socket and `opence`

With `--control` (beside the window) or `--headless` (with no window), the
engine listens on a Unix socket, `opence/control.sock` in the user's
runtime folder (`XDG_RUNTIME_DIR`, else `TMPDIR`), or at
`OPENCE_CONTROL_SOCKET`. The folder is the user's alone, and the engine
never listens on a network address. A request is one JSON line, a command
line or a structured request (`state`, `objects`, `frame`, `subscribe
log`), and each reply is one JSON line carrying the text the shell shows
and the same result as data. A request is at most 64 KiB and 16 levels
deep, a connection may send 200 a second (400 at once), and eight
connections are served.

```sh
opence run 'spawn warthog 2'          # one line (--json for the data)
opence state                          # the world's state
opence frame out.png --size 1920x1080 # the view, drawn offscreen
opence watch                          # the log as it is written
opence script check.ocs               # a script, stopping at a failure
opence shell                          # an interactive shell, completed by the engine
```

It exits 0 when everything worked, 1 when the engine refused something
and 2 when it could not reach the engine. `--headless` takes `--map`,
`--bsp`, `--maps`, `--cache` and `--tick-rate`, and draws a frame only
when one is asked for.

### Debug drawing

Any system calls the immediate-mode shapes (a line, a box, a sphere, text
in the world, each for some ticks); the renderer draws them after the
world. The layers are switched with `debug.draw.<layer>`: `general` (the
commands' and systems' shapes), `collision` (the level's near the camera),
`bodies`, `ai` (firing positions and paths), `triggers`, `markers`,
`lights`, `sounds` and `labels` (the test map's headings). When every
layer is off, nothing is kept or drawn.

A build run from the checkout watches `engine/app/Shaders`: an edited
shader is compiled again while the game runs, and one that fails keeps the
old pipeline and prints the error to the shell.

### The test driver

A command script (`.ocs`) is a list of shell lines with four of its own:

| Line | Does |
| --- | --- |
| `wait <ticks>` | Lets the world run |
| `wait_until <expression> [timeout]` | Lets it run until the expression holds (ten seconds without a timeout) |
| `expect <expression>` | Fails unless the expression holds |
| `frame <name>` | Takes a frame |

An expression is a value (a variable, the world's state, HaloScript),
compared or not with `==`, `!=`, `<`, `<=`, `>` or `>=`. A failure stops
the script and reports its line, its expression and the state. A script
runs headless as fast as the world ticks (`opence script`), or in the
window at its pace (`script`).

### The test map

`devmap` (`--map devmap`, `map devmap`) is made by the engine: a flat
plane with collision, a grid and a sky, and galleries of every object
definition in the user's maps, each placed once from the map that holds
it, with a label: weapons on racks, vehicles in bays, bipeds on pedestals,
devices, scenery, equipment and projectiles. The menus do not offer it.

Its stations run a test on the spot, from a pad or `station <name>`:
`weapons` fires each weapon at targets and shows its shots, damage and
heat; `vehicles` drives a course with corners and ramps; `animation` plays
each of a biped's animations with its name; `doors` cycles every device;
`physics` drops a pile of crates on a slope. Their results are data, so
`ffi/tests/stations.rs` checks them.

### Placement checks

`tools/engine_placement_traces.py` runs the reference app headless on each
stock map and records its objects after spawning (tag, position, rotation,
permutation, BSP and parent) in `build/placement_traces`, never committed,
with the structure BSP each run ended in. `ffi/tests/placements.rs` starts
each map in the engine, switches to that BSP as the mission's scripts did,
and requires each object's place within 1 mm and its facing within 0.01°.

The engine places objects as the reference's `object_types.c` does:

- the kinds other than scenery and light fixtures first, in type order;
- then the scenery and lights whose origin, or bounding offset, is in a
  leaf of the structure BSP;
- a name already placed is skipped, and placements that are not automatic
  are left out; the difficulty bits are never read;
- a vehicle with physics is raised by half its bounding radius, and an
  item not at rest or levitating by 0.05;
- missions start in BSP 0, and a switch removes the unnamed scenery and
  lights and places the new BSP's, but not a named one that is there.

Multiplayer vehicles are left out of the comparison, because the
gametype, not the scenario, decides which are made.
