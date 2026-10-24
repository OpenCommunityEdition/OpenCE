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

## Phase 5: Picking up, vehicles and responsiveness
- [x] Task: Picking up d2efbc5
    - [x] Write tests: walking over a weapon or holding the action button picks it up or swaps; ammo and grenades are taken as in the original
    - [x] The fix
- [~] Task: Vehicles
    - [ ] Write tests: entering gives the vehicle camera and hides the first-person weapon; driver, gunner and passenger seats as in the original; exiting restores first person
    - [ ] The fix
- [ ] Task: Responsiveness
    - [ ] Write tests: positions between ticks interpolate; the camera takes the latest input; input-to-photon latency under two frames plus one tick; the replay hash is unchanged
    - [ ] Tick interpolation and the latency fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 5: Picking up, vehicles and responsiveness' (Protocol in workflow.md)

## Phase 6: Animation
- [ ] Task: The animation stands
    - [ ] Write tests: each stand plays every animation of its graph; none fails to resolve
    - [ ] The fixes
- [ ] Task: Vehicle and weapon animation
    - [ ] Write tests: entering, driving, exiting and seat changes animate; first-person reload, melee, throw and switch play
    - [ ] The fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 6: Animation' (Protocol in workflow.md)

## Phase 7: Sound
- [ ] Task: The sound emitters
    - [ ] Write tests: each sound class's emitter on devgulch produces a voice; none fail; the audio view's counts match
    - [ ] The fixes
- [ ] Task: Sounds of play
    - [ ] Write tests: firing, impacts, footsteps, vehicles, pickups and the UI make their sounds
    - [ ] The fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 7: Sound' (Protocol in workflow.md)

## Phase 8: Particles and effects
- [ ] Task: The effects row
    - [ ] Write tests: each effect spawns its full particle set; golden frames of the row
    - [ ] The fixes
- [ ] Task: Effects of play
    - [ ] Write tests: muzzle flashes, impacts, explosions and vehicle effects spawn as in the original
    - [ ] The fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 8: Particles and effects' (Protocol in workflow.md)

## Phase 9: UI and HUD
- [ ] Task: The main menu
    - [ ] Write tests: every main-menu item can be reached by keyboard and mouse and selected; input reaches it at once
    - [ ] The fixes
- [ ] Task: Overlaps and the HUD
    - [ ] Write tests: no two UI or HUD elements overlap at any supported resolution; the HUD reacts to ammo, shields, grenades and prompts
    - [ ] The fixes
- [ ] Task: Conductor - User Manual Verification 'Phase 9: UI and HUD' (Protocol in workflow.md)

## Phase 10: Performance
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
- [x] Task: Conductor - User Manual Verification 'Phase 10: Performance' (Protocol in workflow.md)

## Phase 11: Tiers and graphics settings
- [ ] Task: Quality tiers
    - [ ] Write tests: each GPU family picks its tier; an 8 GB budget fits at Low
    - [ ] The tier table and its detection
- [ ] Task: The settings model and screens
    - [ ] Write tests: every setting exists with its default; presets; one change gives Custom; each setting changes the frame; screens show descriptions, measured cost and the memory meter
    - [ ] The settings in Rust and Swift, the SwiftUI screens, the benchmark button
- [ ] Task: Meeting the targets
    - [ ] Write tests: each path at Ultra on the M1 Max meets 8.33 ms p99
    - [ ] The last fixes; docs/engine.md updated
- [x] Task: Conductor - User Manual Verification 'Phase 11: Tiers and graphics settings' (Protocol in workflow.md)

## Wrap-up
- [ ] Retire performance_20261023 into this track (marked merged in tracks.md, its folder archived)
