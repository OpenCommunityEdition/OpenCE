# Plan: A new native engine in Rust and Swift

## Phase 0: Foundations [checkpoint: f58d2e10]
- [x] Task: Record the new engine in the tech stack and the product guide 876be761
    - [x] Update `conductor/tech-stack.md` (the `engine/` workspace, bevy_ecs, rkyv, MetalFX, SwiftUI menus, Jolt from Rust, `cargo deny`) and the vision in `conductor/product.md`, before any code
- [x] Task: The workspace and its Nix build ac664f6a
    - [x] Write tests: pytest checks that `nix build .#engine` lays out `OpenCE Engine.app` with the Rust library, and that `cargo deny check` passes
    - [x] `engine/Cargo.toml` with the crates `content`, `import`, `sim`, `physics`, `script`, `net` and `ffi`; `engine/app/Package.swift`; the flake's `engine` package and dev-shell tools
- [x] Task: The contract between Rust and Swift ebbafae4
    - [x] Write tests: cargo tests of the snapshot's layout (sizes and offsets fixed, checked against the generated header); an XCTest that loads the library, starts an empty world and reads a snapshot
    - [x] UniFFI calls in `ffi`; the `#[repr(C)]` snapshot types and a cbindgen header; a double-buffered snapshot that Swift reads without copying
- [x] Task: The fixed tick and determinism 987d50bd
    - [x] Write tests: a world stepped at 60 Hz from recorded inputs gives the same hash on every run; the renderer's interpolation factor is correct across uneven frame times
    - [x] The `sim` schedule in a fixed order, the input record, the world hash, and deterministic maths (`libm`, no FMA)
- [x] Task: Conductor - User Manual Verification 'Phase 0: Foundations' (Protocol in workflow.md) (deferred to the end)

## Phase 1: Import a map and fly through it
- [x] Task: Bounds-checked cache readers 48d7cd76
    - [x] Write tests: readers built over fake caches (Xbox and Custom Edition headers, tag index, tag data) read fields and refuse out-of-range offsets, counts and bad tag classes with an error naming the tag and field
    - [x] `import`: the cache headers, tag index and the reflexive and data readers; layouts from the decompilation and `docs/custom_edition_caches.md`
- [x] Task: Typed tags for the level 72f447f8
    - [x] Write tests: builders for a scenario, a structure BSP (with collision and lightmaps), shaders and bitmaps import into `content`'s types; malformed ones are refused
    - [x] `content` types and their importers: scenario (BSP list), structure BSP, the shader classes, bitmaps
- [x] Task: The native format bb3fc566
    - [x] Write tests: an `.ocpak` round-trips; a truncated or flipped file fails validation, never panicking; the import cache is rebuilt when the importer's version changes
    - [x] rkyv archives with validation; bitmap conversion to BC or RGBA8 with mipmaps; the cache folder
- [x] Task: Fuzz the importer d0abab1f
    - [x] Write tests: `cargo fuzz` targets for the cache readers and `.ocpak` loading, run for a fixed time in the suite without crashes
- [x] Task: Import every map 8ccbe7ba
    - [x] Write tests: pytest (marked, needs `assets/maps`) imports every stock map and the Custom Edition maps present, and checks the counts of tags, BSPs and bitmaps
- [x] Task: The app and window 07a57e7b
    - [x] Write tests: XCTest of the window model (full screen, any size, Retina scale), cursor capture on focus and release on Escape or loss of focus, raw mouse deltas without acceleration
    - [x] The Swift app: AppKit window with a `CAMetalLayer`, full screen, cursor capture, the display link at the display's rate
- [x] Task: The level's renderer 6b3048f2
    - [x] Write tests: golden frames (manual approval) of bloodgulch and a campaign BSP from fixed cameras at 1920×1080, 3440×1440 and 3840×2160
    - [x] Metal pipelines built once at load; the BSP's geometry and lightmaps; materials mapped from the environment, transparent and sky shaders; HDR with EDR or tone-mapping; Hor+ field of view
- [x] Task: Temporal anti-aliasing and upscaling c1625de6
    - [x] Write tests: golden frames with MetalFX's temporal scaler (jitter, motion vectors, depth) and in native mode; a still camera converges to a stable image
    - [x] MetalFX temporal scaler, the jitter sequence, motion vectors; the video settings (render scale, anti-aliasing mode)
- [x] Task: The free camera 2175ae72
    - [x] Write tests: the camera's motion from recorded mouse and keyboard input is exact; the map chosen from the command line or a picker loads
    - [x] Fly camera, the map picker, the frame-time overlay
- [x] Task: Conductor - User Manual Verification 'Phase 1: Import a map and fly through it' (Protocol in workflow.md) (approved by the user 2026-10-21, with the golden frames)

## Phase 2: Objects, models and animation
- [x] Task: Typed tags for objects 0489d862
    - [x] Write tests: builders for object, scenery, weapon, vehicle, biped, device, model (gbxmodel and the Xbox model), animation graph and collision model tags import; malformed ones are refused
    - [x] `content` types and importers for the object tags
- [x] Task: Objects in the world 0e056b18
    - [x] Write tests: the scenario's placements spawn entities with their tag, position, permutation and team; a handle cannot be used after its entity is removed
    - [x] Entities, components and typed handles in `sim`; spawning from the scenario; the registry for object kinds
- [x] Task: Animation 9e57d296
    - [x] Write tests: node matrices from the animation graph for fixed frames match the reference app's (recorded); blending and overlays; compressed and uncompressed animations
    - [x] Animation playback and blending in Rust; node matrices into the snapshot
- [x] Task: Drawing models 61fe8c9e
    - [x] Write tests: golden frames of bloodgulch's objects and a campaign map's standing bipeds; levels of detail chosen by screen size
    - [x] Skinned models on the GPU, the model shaders as materials, permutations and regions
- [x] Task: Skies, fog, decals and effects ebd01c83
    - [x] Write tests: golden frames of the sky and fog on two maps; particle systems and effects from scenery, with the frame clock
    - [x] Sky, fog, level decals, particles and the effects that scenery plays
- [x] Task: The benchmark f49099b6
    - [x] Write tests: `tools/test_engine_benchmark.py` flies a recorded path over bloodgulch at 3840×2160 for 60 s and reports p50 and p99
    - [x] Meet p99 ≤ 8.33 ms on the M1 Max; record the result in `docs/engine.md`
- [x] Task: Conductor - User Manual Verification 'Phase 2: Objects, models and animation' (Protocol in workflow.md) (approved by the user 2026-10-21, with the golden frames)

## Phase 3: Player movement and collision
- [x] Task: Reference traces 3732076e
    - [x] Write tests: a recorder in the reference app writes the player's position, velocity and state each tick for scripted inputs (run, jump, crouch, fall, climb a ramp); the files are checked into `engine/tests/traces/`
    - [x] The trace recorder (a debug setting of the reference app) and the scripted inputs
- [x] Task: Jolt from Rust 30743d17
    - [x] Write tests: cargo tests that a world built from an imported collision BSP stops a falling box; rays and shape casts hit the expected surfaces; two runs are identical
    - [x] `physics`: a safe Rust interface over the C wrapper, the static mesh from the collision BSP, rays and shape casts
- [x] Task: The player's movement 9de1460d
    - [x] Write tests: replaying the scripted inputs gives traces within tolerance of the reference app's
    - [x] `CharacterVirtual` tuned from the biped tag; walk, run, crouch, jump, step up, slide, fall damage; collision with objects
- [x] Task: Playing as the player 3abed295
    - [x] Write tests: XCTest that key bindings and mouse look reach the simulation as typed input; the first-person camera follows the biped
    - [x] Input bindings (a setting), mouse sensitivity, first- and third-person cameras, spawning at the scenario's start points
- [x] Task: Conductor - User Manual Verification 'Phase 3: Player movement and collision' (Protocol in workflow.md) (approved by the user 2026-10-21, with the golden frames)

## Phase 4: Weapons, damage, grenades and ragdolls
- [x] Task: Typed tags for weapons and damage ece79a01
    - [x] Write tests: builders for weapon, projectile, damage effect, equipment and grenade tags import; malformed ones are refused
    - [x] `content` types and importers
- [x] Task: Weapons 8cf477a2
    - [x] Write tests: rate of fire, spread, heat, ammunition and reload times match the tags in simulated ticks; picking up, swapping and dropping
    - [x] Weapon systems, triggers, projectiles and hitscan, melee; the registry for weapon behaviours (melee strikes with the damage task, which it needs)
- [x] Task: Damage, shields and death 858205a2
    - [x] Write tests: damage by hit region, shields recharging after their delay, death, and the damage effects' impulses, against tag values and reference traces
    - [x] Damage, shields, health, death events
- [x] Task: Grenades 6d96214b
    - [x] Write tests: a thrown frag bounces and detonates on its timer; a plasma grenade sticks to a biped and moves with it
    - [x] Throwing, bouncing, sticking, detonation
- [x] Task: The first-person view and the HUD 5ceb92dc
    - [x] Write tests: golden frames of the first-person weapon and the HUD at 16:9, 16:10 and 21:9, anchored to the edges; the motion tracker shows bipeds at their positions
    - [x] First-person models and animations; the HUD drawn by Metal at native resolution
- [x] Task: Ragdolls, limbs and debris ec1e9e97
    - [x] Write tests: a biped killed by a scripted grenade becomes a ragdoll that follows the death animation, then goes loose (joint damping set), and is still there after 60 s; the corpse cap removes the oldest; strong explosions tear off limbs; casings are rigid bodies
    - [x] Ragdolls from model nodes and collision geometry (as the ragdoll track's generator does, ported to Rust), motors then damping, the corpse cap (a setting), limbs and debris
- [x] Task: Conductor - User Manual Verification 'Phase 4: Weapons, damage, grenades and ragdolls' (Protocol in workflow.md) (approved by the user 2026-10-21, with the golden frames)

## Phase 5: Vehicles
- [x] Task: Typed tags for vehicles 1ab6b170
    - [x] Write tests: vehicle tags (physics, seats, turrets, suspension) import; malformed ones are refused
    - [x] `content` types and importers
- [x] Task: Reference traces for vehicles db0692ab
    - [x] Write tests: the reference app's recorder writes Warthog and Ghost traces for scripted inputs (accelerate, turn, brake, jump a ramp)
- [x] Task: Driving and flying b6b47cb7
    - [x] Write tests: Warthog and Ghost traces within tolerance; the Banshee and the Scorpion respond to their tags' values
    - [x] Halo's mass points ported for every vehicle (deviation: not Jolt's vehicle constraints, which cannot match the traces); hover and flight from the vehicle tags
- [x] Task: Seats, turrets and destruction 5c17d30d
    - [x] Write tests: entering and leaving each seat, the Warthog's turret, passengers firing, a vehicle destroyed by damage
    - [x] Seats, entering and leaving animations, turrets, vehicle damage
- [x] Task: Conductor - User Manual Verification 'Phase 5: Vehicles' (Protocol in workflow.md) (approved by the user 2026-10-21, with the golden frames)

## Phase 6: Multiplayer
- [x] Task: Game types 8eb85a2f
    - [x] Write tests: Slayer and CTF rules (scores, flag pickup, return and capture, respawn timers, the end of the game) in a simulated world
    - [x] Game-type systems, the multiplayer scenario data (spawns, flags, netgame equipment)
- [x] Task: The packet format f30b98fd
    - [x] Write tests: snapshots and inputs encode and decode; delta compression against an acknowledged snapshot; bad sizes, indices and rates are refused; a fuzz target for the decoder
    - [x] `net`: the protocol and its version, packets, delta compression
- [x] Task: The host and its clients d832734a
    - [x] Write tests: two copies on loopback aliases play Slayer and CTF for 60 s; a client's predicted player stays within tolerance of the host's after reconciling; a client with 100 ms of added delay and 2% loss still plays
    - [x] UDP sockets, joining and leaving, prediction and reconciliation, interpolation of other players, the host's checks
- [x] Task: The menus 5abf24ce
    - [x] Write tests: XCTest of the menu models (main menu, server list, join by address, host setup, settings for game, controls, video and audio); the settings round-trip through TOML
    - [x] SwiftUI menus over the Metal view, mouse first, Overpass and the OpenCE font; LAN discovery for the server list
- [x] Task: Conductor - User Manual Verification 'Phase 6: Multiplayer' (Protocol in workflow.md) (approved by the user 2026-10-21, with the golden frames)

## Phase 7: AI and the campaign
- [x] Task: HaloScript 93369312
    - [x] Write tests: the scenario's compiled script nodes import into a typed tree; the interpreter runs expressions, globals, `sleep`, `sleep_until` and continuous and dormant scripts; an unknown function is reported on import
    - [x] `script`: the tree, the interpreter, typed function declarations registered by each system (`source/hs/` as reference)
- [x] Task: Every scenario's scripts 93a84ce8
    - [x] Write tests: pytest (marked, needs `assets/maps`) imports every stock scenario's scripts and lists the functions still missing; the list shrinks to none by the end of the phase
    - [x] The script functions the campaign uses, added system by system
- [x] Task: The AI 00724226
    - [x] Write tests: actors in a fake encounter perceive, take firing positions, fire, take cover, flee and use vehicles; chosen encounters on *The Pillar of Autumn* behave like the reference app's (recorded)
    - [x] Encounters, squads, actor types, perception, firing positions, command lists, combat and vehicle use
- [x] Task: The campaign's flow f39344c3
    - [x] Write tests: BSP switches, checkpoints saved and loaded in our own format, cutscenes, difficulties
    - [x] Campaign flow, saved games, cutscene cameras, the campaign menu
- [x] Task: The first two levels e106d2e0
    - [x] Write tests: a recorded run plays *The Pillar of Autumn* and *Halo* from start to end with no script errors
- [x] Task: The documentation a790b635
    - [x] `docs/engine.md`: the architecture, the contract, the content format, and how to add an object kind, a weapon behaviour and a script function
- [x] Task: Conductor - User Manual Verification 'Phase 7: AI and the campaign' (Protocol in workflow.md) (approved by the user 2026-10-21, with the golden frames)
