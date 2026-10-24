# Plan: Placed lights

The phase checks are automated: the light tests, the golden frames and the
reference traces at each phase's end, then a report at the end of the
track.

## Phase 1: One light component
- [ ] Task: LightSource and mobility
    - [ ] Write tests: a light source's fields and its mobility's rules (a Baked light refuses to move or change in play, a Stationary one refuses to move, a Movable one may be attached to a marker)
    - [ ] The component and its systems, in the render set after the sim's
- [ ] Task: The tag lights as Movable light sources
    - [ ] Write tests: the `ligh` lights of objects and effects become Movable light sources with the same colour, radius and function; the light tests and golden frames match
    - [ ] Move the snapshot's lights onto the component
- [ ] Task: Shadowed light choice
    - [ ] Write tests: the nearest few Movable and Stationary lights cast shadows, Stationary preferred when equally near
    - [ ] The choice in `lights.rs` and `LightPass.swift`
- [ ] Task: Conductor - User Manual Verification 'Phase 1: One light component' (Protocol in workflow.md)

## Phase 2: The bake and the probes
- [ ] Task: Baked lights in the bake
    - [ ] Write tests: a Baked point and spot light light the lightmaps and the probes, and only them; a light behind a wall casts no light through it
    - [ ] The bake's scene takes the Baked lights' direct and bounced light
- [ ] Task: Stationary lights' bounce
    - [ ] Write tests: each Stationary light's bounce is kept apart and scaled by its intensity in play; its direct light changes without a bake; the bound per map is enforced
    - [ ] The bounce layers, their blending in the level's and the objects' shaders
- [ ] Task: Probe overrides
    - [ ] Write tests: an override scales, tints or replaces only the probes inside it, blended over its edge; its sky flag is written into the probes it covers
    - [ ] The override volumes in the bake and the probe lookup
- [ ] Task: The bake cache's key
    - [ ] Write tests: changing a Baked or Stationary light invalidates the bake; changing a Movable light or a Stationary light's colour does not; nothing bakes during play
    - [ ] The lights in the cache key
- [ ] Task: Conductor - User Manual Verification 'Phase 2: The bake and the probes' (Protocol in workflow.md)

## Phase 3: The overlay
- [ ] Task: The overlay file
    - [ ] Write tests: a map's lights and overrides round-trip through `<map>.lights.toml` in the tuning folder; an overlay for another map hash is not loaded; bad values, unknown fields and too many lights are refused with a message
    - [ ] The reader and writer, loaded with the map's tuning
- [ ] Task: The overlay's fuzz target
    - [ ] Write tests: the target runs a short corpus in the test step
    - [ ] The target in `engine/fuzz`
- [ ] Task: No overlay, no change
    - [ ] Write tests: a map without an overlay renders the same frames and bakes the same lightmaps as before the track
- [ ] Task: Conductor - User Manual Verification 'Phase 3: The overlay' (Protocol in workflow.md)

## Phase 4: Placing lights
- [ ] Task: The shell and socket commands
    - [ ] Write tests: `light add`, `light set`, `light remove`, `light list`, `probe_override …` and `light bake` work from the shell and the socket, headless, and refuse bad arguments
    - [ ] The commands in the dev state
- [ ] Task: The dev panel's lights page
    - [ ] Write tests (XCTest): the page lists the map's lights and overrides; its controls call the commands; an edited Baked or Stationary light is drawn as Movable and the bake is marked out of date
    - [ ] The page, placing at the crosshair or the camera, the gizmo
- [ ] Task: Debug views
    - [ ] Write tests: each view draws (reach and cone, mobility by colour, cluster counts, override volumes)
    - [ ] The views in the debug drawing
- [ ] Task: devgulch's lighting station
    - [ ] Write tests: the station has one light of each mobility and a probe override, made at import; the station command goes there
    - [ ] The station
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Placing lights' (Protocol in workflow.md)

## Phase 5: Cost and report
- [ ] Task: Measure
    - [ ] Write tests: 64 Movable lights in view on the lighting station stay within the frame cap; a map with no placed lights takes the same frame time as before; 16 Stationary lights add at most half again to the bake
    - [ ] Fix what the measurements find
- [ ] Task: The report
    - [ ] Frame and bake costs, the light and override counts tested, and what the atmosphere track can build on
- [ ] Task: Conductor - User Manual Verification 'Phase 5: Cost and report' (Protocol in workflow.md)
