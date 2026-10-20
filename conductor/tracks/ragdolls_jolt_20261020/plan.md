# Plan: Ragdolls and physics debris with Jolt Physics in the macOS app

## Phase 1: Jolt in the host
- [ ] Task: Record C++17 for Jolt in the tech stack
    - [ ] Update `conductor/tech-stack.md` (macOS Host: Jolt Physics, C++17, behind a C wrapper) before any code
- [ ] Task: Vendor Jolt Physics
    - [ ] Write tests: pytest checks `port/third_party/jolt/README.md` names the upstream, commit and checksum, and that the tree matches the checksum
    - [ ] Copy Jolt unchanged; build it in Nix and `configure.py` as a static library (C++17, single-threaded, `JPH_CROSS_PLATFORM_DETERMINISTIC`, no FMA)
- [ ] Task: The C wrapper
    - [ ] Write tests: native tests (`host_physics_test.c`) create a world, add a static plane and a box, step it, and see the box come to rest; two runs give identical poses
    - [ ] `port/macos/host/physics/physics.h` and `physics.cpp`: world, static mesh, bodies, ragdolls, impulses, kinematic bodies, step, read poses
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Jolt in the host' (Protocol in workflow.md)

## Phase 2: The bridge to the guest
- [ ] Task: The shared region's format
    - [ ] Write tests: native tests reject bad counts, indices, sizes, non-finite numbers and addresses outside guest memory; valid batches decode intact
    - [ ] A shared C header (`port/macos/include/host_physics.h`) for the batch (deaths, impulses, kinematic poses, debris spawns, removals) and the results (bone matrices, debris poses)
- [ ] Task: `host_physics_step` and the level geometry
    - [ ] Write tests: a guest program (port/macos/tests) sends a triangle mesh and a batch and reads results back, at one exit a step
    - [ ] The import in `host_imports.list`; the guest sends the structure BSP's collision triangles when a BSP loads or switches, and the host builds Jolt's static mesh
- [ ] Task: The settings
    - [ ] Write tests: `game.ragdolls`, `game.physics_debris`, `game.physics_debris_max` and `debug.physics_draw` parse from config and environment, off by default; the settings screen lists them (port_settings.py output)
    - [ ] Entries in `port_config.c`, the settings table in `port/linux/README.md`, `tools/port_settings.py` run again
- [ ] Task: Conductor - User Manual Verification 'Phase 2: The bridge to the guest' (Protocol in workflow.md)

## Phase 3: Ragdolls
- [ ] Task: Bodies from the model
    - [ ] Write tests: harness tests build a body description (bones, shapes, joints, limits, masses) from a model's nodes and collision model's per-node geometry for a fake biped; overrides from `port/assets/ragdolls/` replace generated values; a biped with no usable data gives none
    - [ ] `port/linux/game/ragdoll_bodies.c`, and the override files for the stock species
- [ ] Task: Deaths start ragdolls
    - [ ] Write tests: harness tests that `unit_died` on a biped queues a death with its pose and the killing damage's direction and strength, only with `game.ragdolls` on
    - [ ] A `port:` hook in `units.c` / `bipeds.c` calling a port function declared in `halo_linux_source_fixups.h`
- [ ] Task: Animation, then limp
    - [ ] Write tests: native tests that the joint motors follow a target pose for the blend time, then release; a ragdoll dropped on a plane rests with its joints inside their limits
    - [ ] Motor targets from the death animation's node matrices each tick
- [ ] Task: Forces and collisions
    - [ ] Write tests: native tests that an impulse moves a ragdoll, a kinematic box shoves one aside, and two ragdolls collide; harness tests that later damage and explosions near a corpse queue impulses
    - [ ] Damage and explosion impulses; vehicles and living bipeds as kinematic bodies; sleeping
- [ ] Task: Drawing the ragdoll's pose
    - [ ] Write tests: harness tests that the node matrices `render_objects.c` draws come from the results for a ragdoll and from the game otherwise; removing the body (garbage, map change, saved game) removes the ragdoll
    - [ ] A `port:` swap at the node-matrix read in `render_objects.c`
- [ ] Task: Golden frames of ragdolls
    - [ ] Write tests: `tools/test_macos_frames.py` cases with the frame clock: a Grunt killed by a scripted grenade (mid-fall, at rest) and a body shoved by a Warthog; the existing goldens unchanged with the setting off
    - [ ] Tune the overrides until the frames look right; approve the references
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Ragdolls' (Protocol in workflow.md)

## Phase 4: Debris
- [ ] Task: Particles as rigid bodies
    - [ ] Write tests: harness tests that point-physics particles that collide with the level spawn debris bodies with the definition's size and lifetime, only with `game.physics_debris` on; the cap hands the oldest back to point physics
    - [ ] A `port:` hook in the particle systems' point-physics update; drawing from the results
- [ ] Task: Garbage objects
    - [ ] Write tests: harness tests that garbage objects become debris bodies and are drawn from Jolt's pose
    - [ ] A `port:` hook for garbage
- [ ] Task: Golden frames of debris
    - [ ] Write tests: casings and debris at rest after a burst of fire, with the frame clock
    - [ ] Approve the references
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Debris' (Protocol in workflow.md)

## Phase 5: Performance, network and documentation
- [ ] Task: The benchmark with ragdolls and debris
    - [ ] Write tests: the benchmark script spawns 30 ragdolls and 200 debris pieces on bloodgulch at 3840×2160 and reports p99 and the physics tick's time
    - [ ] Meet p99 ≤ 8.33 ms and ≤ 1 ms a tick on the M1 Max; record the results in `docs/macos_host.md`
- [ ] Task: A network game with the settings on
    - [ ] Write tests: `tools/test_macos_host.py` plays two copies with `HALO_NET_ONLINE=false` and both settings on; `HALO_PORT_NETWORK_VERSION` is unchanged
- [ ] Task: `debug.physics_draw` and documentation
    - [ ] Write tests: the wireframe pass draws only with the setting on
    - [ ] Describe the physics bridge in `docs/macos_host.md`
- [ ] Task: Conductor - User Manual Verification 'Phase 5: Performance, network and documentation' (Protocol in workflow.md)
