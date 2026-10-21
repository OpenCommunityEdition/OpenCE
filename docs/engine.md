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

### Not yet drawn

These are deferred to a later track:

- Decals on the level (the scenario's decal palette);
- Particle systems and effects (`effe`, `pctl`, `part`), including the effects
  that scenery plays.

The importer does not read these tags yet. A map that has them loads and
draws without them.

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

A frame's own GPU time is 9.10 ms at p50 and 10.10 ms at p99. That is longer
than the frame time because the GPU overlaps one frame's work with the next.
