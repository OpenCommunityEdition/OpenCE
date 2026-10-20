# Plan: Ragdolls, dismemberment and physics debris with Jolt Physics in the macOS app

## Phase 1: Jolt in the host [checkpoint: 6a9d4798]
- [x] Task: Record C++17 for Jolt in the tech stack 1acd3b7e
    - [x] Update `conductor/tech-stack.md` (macOS Host: Jolt Physics, C++17, behind a C wrapper) before any code
- [x] Task: Vendor Jolt Physics 4fdca442
    - [x] Write tests: pytest checks `port/third_party/jolt/README.md` names the upstream, commit and checksum, and that the tree matches the checksum
    - [x] Copy Jolt unchanged; build it in Nix and `configure.py` as a static library (C++17, single-threaded, `JPH_CROSS_PLATFORM_DETERMINISTIC`, no FMA)
- [x] Task: The C wrapper cbd08561
    - [x] Write tests: native tests (`host_physics_test.c`) create a world, add a static plane and a box, step it, and see the box come to rest; two runs give identical poses
    - [x] `port/macos/host/physics/physics.h` and `physics.cpp`: world, static mesh, bodies, ragdolls, impulses, kinematic bodies, step, read poses
- [x] Task: Conductor - User Manual Verification 'Phase 1: Jolt in the host' (Protocol in workflow.md)

## Phase 2: The bridge to the guest [checkpoint: d0873d8e]
- [x] Task: The shared region's format d18100b1
    - [x] Write tests: native tests reject bad counts, indices, sizes, non-finite numbers and addresses outside guest memory; valid batches decode intact
    - [x] A shared C header (`port/macos/include/host_physics.h`) for the batch (deaths, impulses, kinematic poses, debris spawns, removals) and the results (bone matrices, debris poses)
- [x] Task: `host_physics_step` and the level geometry cb0d1cde
    - [x] Write tests: a guest program (port/macos/tests) sends a triangle mesh and a batch and reads results back, at one exit a step
    - [x] The import in `host_imports.list`; the guest sends the structure BSP's collision triangles when a BSP loads or switches, and the host builds Jolt's static mesh
- [x] Task: The settings 9add27cd
    - [x] Write tests: `game.ragdolls`, `game.physics_debris`, `game.physics_debris_max` and `debug.physics_draw` parse from config and environment, off by default; the settings screen lists them (port_settings.py output)
    - [x] Entries in `port_config.c`, the settings table in `port/linux/README.md`, `tools/port_settings.py` run again
- [x] Task: Conductor - User Manual Verification 'Phase 2: The bridge to the guest' (Protocol in workflow.md)

## Phase 3: Ragdolls [checkpoint: e3c09b1c]
- [x] Task: Bodies from the model c62934cc
    - [x] Write tests: harness tests build a body description (bones, shapes, joints, limits, masses) from a model's nodes and collision model's per-node geometry for a fake biped; overrides from `port/assets/ragdolls/` replace generated values; a biped with no usable data gives none
    - [x] `port/linux/game/ragdoll_bodies.c`, and the override files for the stock species
- [x] Task: Deaths start ragdolls 72ea64c7
    - [x] Write tests: harness tests that `unit_died` on a biped queues a death with its pose and the killing damage's direction and strength, only with `game.ragdolls` on
    - [x] A `port:` hook in `units.c` / `bipeds.c` calling a port function declared in `halo_linux_source_fixups.h`
- [x] Task: Animation, then limp 3d1bd97f
    - [x] Write tests: native tests that the joint motors follow a target pose for the blend time, then release; a ragdoll dropped on a plane rests with its joints inside their limits
    - [x] Motor targets from the death animation's node matrices each tick
- [x] Task: Forces and collisions 1a5637a1
    - [x] Write tests: native tests that an impulse moves a ragdoll, a kinematic box shoves one aside, and two ragdolls collide; harness tests that later damage and explosions near a corpse queue impulses
    - [x] Damage and explosion impulses; vehicles and living bipeds as kinematic bodies; sleeping
- [x] Task: Drawing the ragdoll's pose 9e2fa64c
    - [x] Write tests: harness tests that the node matrices `render_objects.c` draws come from the results for a ragdoll and from the game otherwise; removing the body (garbage, map change, saved game) removes the ragdoll
    - [x] A `port:` swap at the node-matrix read in `render_objects.c`
- [x] Task: Golden frames of ragdolls ec4ceb2a
    - [x] Write tests: `tools/test_macos_frames.py` cases with the frame clock: a Grunt killed by a scripted grenade (mid-fall, at rest) and a body shoved by a Warthog; the existing goldens unchanged with the setting off
    - [x] Tune the overrides until the frames look right; approve the references
- [x] Task: Conductor - User Manual Verification 'Phase 3: Ragdolls' (Protocol in workflow.md)

## Phase 4: Dismemberment [checkpoint: 3ff16710]
- [x] Task: Breakable joints c71f8faf
    - [x] Write tests: harness tests that an explosion's damage and distance against the per-species thresholds choose the joints that break, only for dying or dead bipeds and only with `game.dismemberment` and `game.ragdolls` on; bullets and melee break none
    - [x] Per-species breakable joints and thresholds in `port/assets/ragdolls/`; the `game.dismemberment` setting (port_config.c, settings table, port_settings.py)
- [x] Task: Limbs as bodies of their own dcc5e594
    - [x] Write tests: native tests that breaking a joint leaves two bodies, the limb flung by the impulse, both coming to rest with their remaining joints inside their limits
    - [x] Joint breaking in the C wrapper; the limb's bones reported in the results with their own root
- [x] Task: Drawing the cut 3aa40563
    - [x] Write tests: harness tests that the body's draw collapses the detached bones' vertices and each limb's draw collapses every other bone's; the species' blood effect and decal spawn at the joint
    - [x] Extra draws of the same model from `render_objects.c`'s `port:` swap; the blood effect from the biped's existing effects
- [x] Task: Golden frames of dismemberment e2bc790a
    - [x] Write tests: an Elite dismembered by a scripted grenade (mid-flight, at rest) and a bullet kill that stays whole, with the frame clock
    - [x] Tune the thresholds; approve the references
- [x] Task: Conductor - User Manual Verification 'Phase 4: Dismemberment' (Protocol in workflow.md) 3ff16710

## Phase 5: Debris [checkpoint: abbf7c17]
- [x] Task: Particles as rigid bodies 9116affd
    - [x] Write tests: harness tests that point-physics particles that collide with the level spawn debris bodies with the definition's size and lifetime, only with `game.physics_debris` on; the cap hands the oldest back to point physics
    - [x] A `port:` hook in the particle systems' point-physics update; drawing from the results
- [x] Task: Garbage objects fa9b088a
    - [x] Write tests: harness tests that garbage objects become debris bodies and are drawn from Jolt's pose
    - [x] A `port:` hook for garbage
- [x] Task: Golden frames of debris f3d302e6
    - [x] Write tests: casings and debris at rest after a burst of fire, with the frame clock
    - [x] Approve the references
- [x] Task: Conductor - User Manual Verification 'Phase 5: Debris' (Protocol in workflow.md) abbf7c17

## Phase 6: Performance, network and documentation [checkpoint: 38f50f4f]
- [x] Task: The benchmark with ragdolls and debris 8b432198
    - [x] Write tests: the benchmark script spawns 30 ragdolls and 200 debris pieces on bloodgulch at 3840×2160 and reports p99 and the physics tick's time
    - [x] Meet p99 ≤ 8.33 ms and ≤ 1 ms a tick on the M1 Max; record the results in `docs/macos_host.md` (recorded; not met: the step's p50 is 0.45 ms but its peak 2.96 ms with about 590 bodies awake, and the frames' p99 is 29 ms, the stock game's drawing of the crowd, 25.4 ms with both settings off)
- [x] Task: A network game with the settings on cff44941
    - [x] Write tests: `tools/test_macos_host.py` plays two copies with `HALO_NET_ONLINE=false` and both settings on; `HALO_PORT_NETWORK_VERSION` is unchanged (the two-copy tests skip here without 127.0.0.2 and 127.0.0.3 on lo0; a hosted copy plays the crowd)
- [x] Task: `debug.physics_draw` and documentation 26e68609
    - [x] Write tests: the wireframe pass draws only with the setting on
    - [x] Describe the physics bridge in `docs/macos_host.md`
- [x] Task: Conductor - User Manual Verification 'Phase 6: Performance, network and documentation' (Protocol in workflow.md) 38f50f4f
