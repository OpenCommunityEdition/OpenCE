# Spec: Ragdolls, dismemberment and physics debris with Jolt Physics

## Overview
Halo CE's characters die by playing a death animation, and its corpses only settle their bones onto the ground (`biped_limp_noodle_relax_nodes_onto_environment`). Its debris is point physics: pieces fall and bounce but never tumble. This track adds Jolt Physics to the macOS app so that every character becomes a ragdoll when it dies, and the effects' debris tumbles as small rigid bodies.

Both are **for show only**. The game keeps its own physics and its own game state. Jolt's results replace only what is drawn: the bone matrices that `render_objects.c` reads for a body, and the debris pieces' positions and orientations. Nothing new crosses the network, saved games are unchanged, and each machine simulates its own bodies.

Jolt (C++, MIT) runs in the app's host process, outside the 32-bit guest. The guest sends it one batch per game tick through shared memory (deaths, hits, the level's collision geometry when a BSP loads, and the moving objects' positions), and reads the results back from that memory, at one VM exit a tick.

## Functional Requirements

### Jolt in the host
1. Jolt Physics is vendored unchanged in `port/third_party/jolt`, with a README naming its upstream, version or commit, checksum and what uses it. It is built with Nix's clang as C++17, in its cross-platform deterministic mode, so runs repeat exactly. (Amended in Phase 6: it is stepped by a pool of three worker threads beside the calling one, with Jolt's deterministic simulation on, which keeps runs exact; one thread took 6 ms a step with thirty ragdolls tumbling.)
2. A thin C interface of our own (`port/macos/host/physics/`) wraps the parts of Jolt the host needs; the rest of the host stays C (gnu11).
3. `conductor/tech-stack.md` is updated first: C++17 for Jolt and its wrapper in the host.

### The bridge to the guest
4. A shared region in guest memory, set up once (in the way the memory watch's table is shared), holds:
   - the guest's batch for the tick: new deaths, impulses, the moving bodies' poses, and debris spawns;
   - the host's results: each ragdoll's bone matrices and each debris piece's pose.
5. One import, `host_physics_step`, carries out a tick. The host checks every entry in the batch (counts, indices, sizes, finite numbers, addresses inside guest memory) before using it, as untrusted input.
6. When a structure BSP loads (including a campaign BSP switch), the guest sends its collision geometry once, and the host builds Jolt's static mesh from it.

### Ragdolls
7. Every biped becomes a ragdoll when it dies, in the campaign and in multiplayer: Master Chief and the players, Marines, Grunts, Jackals, Elites, Hunters, the Flood, Sentinels' bipeds and any Custom Edition biped.
8. **Animation, then limp:** at death the game's death animation starts. Jolt's joint motors follow its pose for a short, tunable time, then let go, so the body falls with intent, then collapses.
9. Each species' body (a shape per bone, the joints and their limits, and the masses) is generated from its model's nodes and its collision model's per-node geometry. A data file of per-species overrides (`port/assets/ragdolls/`) tunes what generation gets wrong. A biped with no usable data keeps the stock death.
10. **Forces:**
    - The hit that killed a body, and later bullets, grenades and other explosions, push it, in the direction and with the strength of the game's damage.
    - Vehicles and living bipeds are kinematic bodies in Jolt, moved to the game's positions each tick, so they shove ragdolls aside.
    - Ragdolls collide with each other, and with the level's static geometry.
11. Bodies that come to rest sleep (Jolt's sleeping) and cost nothing until something touches them.
12. The drawn pose comes from Jolt, swapped in where `render_objects.c` reads the body's node matrices. The game's own node matrices, markers, collision and limp-noodle step are unchanged.
13. When a body is removed (garbage collection, a map change, a saved game loaded), its ragdoll is removed. After a saved game loads, corpses are drawn as the game places them.

### Dismemberment
14. A strong enough explosion near a dying or dead biped (grenades, rockets, plasma, fuel rods, a vehicle blowing up) can tear off limbs: the head, the arms at the shoulder or elbow, the legs at the hip or knee. The chance and the threshold come from the explosion's damage and distance, and from per-species values in `port/assets/ragdolls/` (which joints can break, and how much damage each needs). Bullets and melee never dismember.
15. A torn-off limb is cut from the ragdoll by breaking its Jolt joint; the limb becomes a body of its own, flung by the explosion's impulse, and collides, tumbles and sleeps like any other.
16. Drawing: the model is drawn once for the body with the detached bones' vertices collapsed, and once for each limb with every other bone's vertices collapsed, so no new mesh is made. The cut is covered by the species' existing blood effect and decal (red for humans, blue or purple for the Covenant, the Flood's own), spawned at the joint.
17. Living characters are never dismembered, so gameplay (hit regions, damage, the AI) is unchanged; the Flood's own arm-loss damage regions stay as the game has them.
18. A setting, `game.dismemberment` (off by default), which needs `game.ragdolls`.

### Debris
19. The effects' particles that use point physics and collide with the level (casings, chunks, sparks, glass) can be simulated as small rigid bodies in Jolt. They tumble, spin and come to rest on the level's surfaces. Their size and lifetime come from the particle's own definition.
20. Garbage objects (the pieces left by destroyed objects) tumble in Jolt in the same way, drawn from Jolt's pose.
21. A cap on the number of debris bodies (a setting); past it the oldest pieces fall back to the game's point physics.

### Settings
22. `game.ragdolls` (off by default) and `game.physics_debris` (off by default), each with an environment variable and a comment for `config.toml`, listed in the settings table and shown on the port's settings screen (generated by `tools/port_settings.py`). They apply from the next death or the next debris spawned.
23. `game.dismemberment` (off by default, needs `game.ragdolls`), documented in the same way.
24. `debug.physics_draw` (off by default) draws Jolt's bodies as wireframes over the game, for tuning.

## Non-Functional Requirements
- **Performance:** with 30 ragdolls and 200 debris pieces active on bloodgulch at 3840×2160, p99 stays ≤ 8.33 ms (the benchmark), and a physics tick takes ≤ 1 ms on the M1 Max. The bridge adds at most one VM exit a tick.
- **Gameplay and netcode untouched:** the simulation, the game state and everything sent between machines are unchanged; `HALO_PORT_NETWORK_VERSION` is not raised. With both settings off, frames are identical to today's (the golden frames).
- **Repeatable:** with the frame clock (`debug.frame_clock`), a run's ragdolls and debris are the same every time, so golden frames can test them.
- **Safety:** the batch and the level geometry from the guest are bounds-checked; bad entries are dropped and logged, never trusted.
- **Source changes:** `source/` changes are minimal, each marked `port:`; the hooks call port functions declared in `halo_linux_source_fixups.h`.
- **Coverage:** >80% for the new host and guest code.

## Acceptance Criteria
1. Jolt builds from the vendored copy under Nix; `nix build .#app` succeeds.
2. Unit tests: the bridge's encoding and validation (bad counts, indices, sizes, non-finite numbers and addresses rejected); body generation from a model's nodes and collision geometry; a ragdoll dropped onto a plane comes to rest with its joints inside their limits.
3. Golden frames, from a fixed camera with the frame clock:
   - a Grunt killed by a scripted grenade on a campaign map, mid-fall and at rest;
   - a body shoved by a Warthog;
   - an Elite dismembered by a scripted grenade, mid-flight and at rest, and a bullet kill that does not dismember;
   - casings and debris at rest after a burst of fire;
   - with both settings off, the existing golden frames unchanged.
4. The benchmark with 30 ragdolls and 200 debris pieces meets the performance target above, recorded in `docs/macos_host.md`.
5. A network game between two copies plays as before with the settings on (`tools/test_macos_host.py`), with `HALO_NET_ONLINE=false`.
6. Settings documented in the settings table; `docs/macos_host.md` describes the physics bridge.

## Out of Scope
- Changing gameplay physics: vehicles, grenades, players' movement and the game's collision stay Halo's.
- Ragdolls or debris that affect gameplay, or that are synchronised between machines.
- Cloth, destructible level geometry and fluid.
- Ray tracing (its own track) and any non-macOS build.
- Code or data from Havok or from any newer Halo engine.
