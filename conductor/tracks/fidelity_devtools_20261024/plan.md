# Plan: Fidelity, performance and developer tools

Scope: the systems on devgulch and Blood Gulch only. Campaign levels are ported and fixed one at a time in later tracks.

## Phase 1: devgulch and the launcher's Developer tab [checkpoint: 9410383]
- [x] Task: devgulch from bloodgulch.map aae24b5
    - [x] Write tests: the import makes devgulch from the user's map; it loads; nothing of it is in the repository or the app bundle
    - [x] The generated copy and its station layout
- [x] Task: The stations 12b603e
    - [x] Write tests: each station spawns its set (vehicles, AI squads, weapons and equipment, effects row, sound emitters, shader test surfaces, animation stands, HUD and UI triggers); a station command teleports there
    - [x] The stations; devgulch becomes the default test level, replacing devmap
- [x] Task: The launcher's Developer tab 5d266e3
    - [x] Write tests (XCTest): the tab lists devgulch, its stations, view modes and toggles; launching passes them to the engine
    - [x] The tab and its launch arguments (the view modes and toggles are wired in Phase 3)
- [x] Task: Conductor - User Manual Verification 'Phase 1: devgulch and the launcher's Developer tab' (Protocol in workflow.md)

## Phase 2: Stop the heat, see the cost [checkpoint: eb37a0e]
- [x] Task: Bake and probes out of play 123545e
    - [x] Write tests: nothing is baked or probed after the level starts; a cached bake loads already lit; a missing cache bakes behind the loading screen
    - [x] Bake at import or on the loading screen; cache by map hash; no lighting swap mid-game
- [x] Task: Thermals and idling 2af3aeb
    - [x] Write tests: the frame cap holds; the app is idle when unfocused or minimised; frames in flight stay bounded
    - [x] Frame cap by default, idle when unfocused, CAMetalDisplayLink pacing
- [x] Task: GPU and CPU timings, and the hitch log de38ff7
    - [x] Write tests: every pass reports a GPU time; the CPU spans nest; a forced compile, upload or allocation is logged with its cause
    - [x] Counter sample buffers, CPU spans, the hitch log, perf.log
- [x] Task: Conductor - User Manual Verification 'Phase 2: Stop the heat, see the cost' (Protocol in workflow.md)

## Phase 3: Developer tools in game [checkpoint: 799a6a9]
- [x] Task: The dev state model 6cd68c2
    - [x] Write tests: each control round-trips through Rust, Swift, the shell and the socket; tuned values save and load per map; when off, the frame is unchanged and costs nothing
    - [x] One DevState shared by the panel, the shell, `opence` and the Developer tab
- [x] Task: View modes and the stock lightmap reference view 8350da6
    - [x] Write tests: each view mode gives its own golden frame on devgulch; the stock view draws the map's lightmaps
    - [x] Debug outputs in the shaders, selected by a uniform
    - [x] Wire the shell's undrawn switches (render.wireframe, render.lightmaps, render.objects, render.hud), each with a golden frame, and add them to RenderSwitches.flags so the Developer tab offers them
- [x] Task: Lighting controls bbeeda1
    - [x] Write tests: each slider moves the frame in the expected direction; reset restores the default
    - [x] Exposure, sun, bounce, probes, AO, emissive, bloom, tone curve
- [x] Task: Object inspector, live timings and the audio view 2ae1595
    - [x] Write tests: a mouse pick returns the object under the cursor; the inspector lists tag, shader, animation, physics, light and sounds; the audio view counts voices by category
    - [x] GPU picking ID buffer; SwiftUI panel overlays
- [x] Task: Conductor - User Manual Verification 'Phase 3: Developer tools in game' (Protocol in workflow.md)

## Phase 4: Lighting and shaders [checkpoint: bd7eb7a]
- [x] Task: Collect the problems 56a8d15
    - [x] On devgulch's shader surfaces and Blood Gulch, with the stock view, view modes and inspector, list each problem with its cause in docs/engine.md
- [x] Task: Exposure and tone mapping matched to the original d54fa7a
    - [x] Write tests: mean and peak brightness stay within tolerance of the stock view's; the ring and bright textures are not clipped
    - [x] The fix
- [x] Task: Black artifacts, flicker and blotchy emissives 6ef5857
    - [x] Write tests: no NaN or black pixels on every shader class; the same frame twice gives the same image; emissive golden frames; ExposureTests' start view held to 10% of stock's mean
    - [x] The fixes
- [x] Task: Seams b2f6c38
    - [x] Write tests: no lightmap or texture seams on the seam views' golden frames
    - [x] The fixes
- [x] Task: Shadow edges 0b80026
    - [x] Write tests: neighbouring texels of one surface step hard no more often than on the stock pages (`ShadowEdgeTests`)
    - [x] Each texel's paths start over its footprint
- [x] Task: Conductor - User Manual Verification 'Phase 4: Lighting and shaders' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)

## Phase 5: Picking up, vehicles and responsiveness [checkpoint: 4ef9f36]
- [x] Task: Picking up d2efbc5
    - [x] Write tests: walking over a weapon or holding the action button picks it up or swaps; ammo and grenades are taken as in the original
    - [x] The fix
- [x] Task: Vehicles 675acc9
    - [x] Write tests: entering gives the vehicle camera and hides the first-person weapon; driver, gunner and passenger seats as in the original; exiting restores first person
    - [x] The fix
- [x] Task: Responsiveness a9d3702
    - [x] Write tests: positions between ticks interpolate; the camera takes the latest input; input-to-photon latency under two frames plus one tick; the replay hash is unchanged
    - [x] Tick interpolation and the latency fixes
- [x] Task: Conductor - User Manual Verification 'Phase 5: Picking up, vehicles and responsiveness' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)

## Phase 6: Animation [checkpoint: 14bcc84]
- [x] Task: The animation stands 741a726
    - [x] Write tests: each stand plays every animation of its graph; none fails to resolve
    - [x] The fixes
- [x] Task: Vehicle and weapon animation 9a676e5
    - [x] Write tests: entering, driving, exiting and seat changes animate; first-person reload, melee, throw and switch play
    - [x] The fixes
- [x] Task: Conductor - User Manual Verification 'Phase 6: Animation' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)

## Phase 7: Sound [checkpoint: dd023f8]
- [x] Task: The sound emitters 8e6ba2b
    - [x] Write tests: each sound class's emitter on devgulch produces a voice; none fail; the audio view's counts match
    - [x] The fixes
- [x] Task: Sounds of play e235aed
    - [x] Write tests: firing, impacts, footsteps, vehicles, pickups and the UI make their sounds
    - [x] The fixes
- [x] Task: Conductor - User Manual Verification 'Phase 7: Sound' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)

## Phase 8: Particles and effects [checkpoint: 8f5676c]
- [x] Task: The effects row 0e1dae2
    - [x] Write tests: each effect spawns its full particle set; golden frames of the row
    - [x] The fixes
- [x] Task: Effects of play 0e1dae2
    - [x] Write tests: muzzle flashes, impacts, explosions and vehicle effects spawn as in the original
    - [x] The fixes
- [x] Task: Conductor - User Manual Verification 'Phase 8: Particles and effects' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)

## Phase 9: The HUD [checkpoint: 749261c]
The menus and the rest of the UI are left to the UI rewrite's own track.
- [x] Task: The HUD 25adc28
    - [x] Write tests: no two HUD elements overlap at any supported resolution; the HUD reacts to ammo, shields, grenades and prompts
    - [x] The fixes
- [x] Task: Conductor - User Manual Verification 'Phase 9: The HUD' (Protocol in workflow.md) (superseded: automated verification only, at the user's instruction)

## Phase 10: Lightmap resolution and interiors
Found in play: shadows on the far hills break into large blocky triangles across the cliffs and ground, and the bunker interiors are lit coarsely, with leaks and blotches on the walls.
- [ ] Task: Measure the lightmaps' density
    - [ ] Write tests: a sweep of each surface's world size per lightmap texel on Blood Gulch and devgulch, the far hills and the bunker interiors, against the stock lightmaps'
    - [ ] The sweep, and the causes listed in docs/engine.md
- [ ] Task: Shadows on the far hills
    - [ ] Write tests: on the far hills' views, a shadow edge crosses the triangles' edges without stepping more often than on the stock pages; no texel larger than the stock's
    - [ ] The fixes
- [ ] Task: The bunker interiors
    - [ ] Write tests: no light leaks (texels on the inside lit by the sun through a wall); a flat wall's texels vary no more than the stock's; golden frames of each bunker's interior
    - [ ] The fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 10: Lightmap resolution and interiors' (Protocol in workflow.md)

## Phase 11: Matching the source
Lighting, shaders and graphics are rewritten and enhanced. AI, particles, animation, sound, script logic and gameplay are ported from `source/` first, matched to it, and only then extended; this phase finds where they still differ.
- [ ] Task: A reference map of each system
    - [ ] Write tests: each function of `source/` that AI, particles, animation, sound, scripts and gameplay port has a reference trace, or is listed with a reason; `--check` fails on a stale trace
    - [ ] The list (in docs/engine.md), and `tools/engine_*_reference.py` scripts for the systems that have none
- [ ] Task: The divergences
    - [ ] Write tests: the engine against each new trace
    - [ ] A sweep report: each function that differs, and by how much; Phases 12 to 15 take their order from it
- [ ] Task: Conductor - User Manual Verification 'Phase 11: Matching the source' (Protocol in workflow.md)

## Phase 12: Weapons, particles and effects
Found in play (2026-10-25): each weapon's firing effects are in the wrong place, and in first person they come from the face, not the gun; thrown grenades can't be seen; particles still face the camera; explosions are missing parts; blood decals on metal; impacts blur where bullets hit the ground.
- [ ] Task: Where the effects start
    - [ ] Write tests: each weapon's firing effects start at the marker the original uses (in first person, the first-person model's; in third person, the weapon's), within a tolerance; the old engine's (`port/macos`) positions recorded as the reference
    - [ ] The fixes
- [ ] Task: Thrown grenades
    - [ ] Write tests: a thrown grenade is drawn every frame from the throw to its detonation, in first and third person
    - [ ] The fixes
- [ ] Task: How particles face
    - [ ] Write tests: each particle's orientation follows its tag (facing the screen, parallel to its direction, vertical, local), against the original's particle functions' trace
    - [ ] The fixes
- [ ] Task: Explosions
    - [ ] Write tests: each explosion spawns every event and part its effect tag lists (particles, systems, lights, sounds, decals, damage), counted against the tag
    - [ ] The fixes
- [ ] Task: Impacts by material
    - [ ] Write tests: an impact on each material type uses the effect and decal the material table names (no blood on metal)
    - [ ] The fixes
- [ ] Task: No blur on impacts
    - [ ] Write tests: frame interpolation never blends a spawned, teleported or attached object from a place it wasn't; golden frames of a bullet hitting the ground
    - [ ] The fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 12: Weapons, particles and effects' (Protocol in workflow.md)

## Phase 13: Vehicles
Found in play (2026-10-25): getting in and out is too quick; vehicles collide only with other vehicles; turret and gun animations play over and over with no one using them (the warthog's chaingun barrel spins on its own); wheels and tracks don't turn properly and blur; driving sprays too many sprites, the same on every surface; the vehicle HUDs have the wrong icons and meters.
- [ ] Task: Getting in and out
    - [ ] Write tests: the enter and exit animations last as many ticks as the original's, against the animation graph at 30 Hz
    - [ ] The fixes
- [ ] Task: Collision with the world
    - [ ] Write tests: vehicles collide with the structure, scenery, crates, bipeds and devices; none passes through on the vehicles station's paths
    - [ ] The fixes
- [ ] Task: Animations driven by their functions
    - [ ] Write tests: a turret, barrel or other overlay moves only when its function (aim, firing, speed) changes, as the original's object functions say; the warthog's chaingun barrel stays still with no gunner
    - [ ] The fixes
- [ ] Task: Wheels and tracks
    - [ ] Write tests: wheels turn at the ground speed over their radius and tracks scroll with speed, against the original's functions; golden frames of a moving wheel show no smear between ticks
    - [ ] The fixes
- [ ] Task: Effects on the move
    - [ ] Write tests: tyre and dust effects spawn at the rate their tags give and change with the ground's material
    - [ ] The fixes
- [ ] Task: The vehicle HUDs
    - [ ] Write tests: each vehicle and seat shows the HUD interface, icons and meters its tags name
    - [ ] The fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 13: Vehicles' (Protocol in workflow.md)

## Phase 14: Death and ragdolls
Found in play (2026-10-25): when a body dies the ragdoll stretches and splits, the model glitches, and the camera doesn't follow the body.
- [ ] Task: The ragdoll
    - [ ] Write tests: a dying biped's ragdoll starts from the death animation's pose, keeps every bone within its length (no stretching or splitting), and the model follows it every frame
    - [ ] The fixes
- [ ] Task: The death camera
    - [ ] Write tests: the dead player's camera follows the body as the original's does
    - [ ] The fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 14: Death and ragdolls' (Protocol in workflow.md)

## Phase 15: Sound
Found in play (2026-10-25): music and many sounds (hitting surfaces and the rest) are still missing.
- [ ] Task: Music
    - [ ] Write tests: the scripted looping sounds (the music) start, change and stop as the scripts say
    - [ ] The fixes
- [ ] Task: Every sound
    - [ ] Write tests: every sound Blood Gulch's tags refer to (impacts by material, footsteps, vehicles, weapons, ambience, dialogue) plays when its event happens; a list of any that don't
    - [ ] The fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 15: Sound' (Protocol in workflow.md)

## Phase 16: Performance
- [ ] Task: The benchmark paths
    - [ ] Write tests: a path replays the same camera and inputs; the report has mean, p99, p99.9, hitches and pacing; a regression over 5% fails
    - [ ] devgulch, Blood Gulch, and 16 players on Blood Gulch
- [ ] Task: No compiles, uploads or allocations in play
    - [ ] Write tests: no pipeline built after load; a counting allocator sees nothing in a frame
    - [ ] Metal binary archives, background texture streaming
- [ ] Task: Culling, indirect draws, tile memory and half precision
    - [ ] Write tests: the drawn set matches brute-force visibility; golden frames within tolerance
    - [ ] Cluster/PVS and Hi-Z culling, argument buffers, memoryless attachments
- [ ] Task: Upscaling and the parallel simulation
    - [ ] Write tests: MetalFX golden frames at 0.67; dynamic resolution holds the target; replay hash unchanged in parallel
    - [ ] MetalFX, dynamic resolution, the ECS in parallel
- [x] Task: Conductor - User Manual Verification 'Phase 16: Performance' (Protocol in workflow.md)

## Phase 17: Tiers and graphics settings
- [ ] Task: Quality tiers
    - [ ] Write tests: each GPU family picks its tier; an 8 GB budget fits at Low
    - [ ] The tier table and its detection
- [ ] Task: The settings model and screens
    - [ ] Write tests: every setting exists with its default; presets; one change gives Custom; each setting changes the frame; screens show descriptions, measured cost and the memory meter
    - [ ] The settings in Rust and Swift, the SwiftUI screens, the benchmark button
- [ ] Task: Meeting the targets
    - [ ] Write tests: each path at Ultra on the M1 Max meets 8.33 ms p99
    - [ ] The last fixes; docs/engine.md updated
- [x] Task: Conductor - User Manual Verification 'Phase 17: Tiers and graphics settings' (Protocol in workflow.md)

## Wrap-up
- [ ] Retire performance_20261023 into this track (marked merged in tracks.md, its folder archived)
