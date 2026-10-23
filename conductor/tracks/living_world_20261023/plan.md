# Plan: Weather, water and a world that reacts

## Phase 1: Weather systems
- [ ] Task: Climates and weather states
    - [ ] Write tests: each map's climate; the states change as fronts on the wind; the state is part of the replay; with `world.weather` off nothing changes
    - [ ] The weather model and `world.weather`
- [ ] Task: What the weather drives
    - [ ] Write tests: the cloud cover, fog, particles, wind, lightning (flash, then thunder by distance) and ambient sound follow the state
    - [ ] The connections to the atmosphere, particle and sound tracks
- [ ] Task: Weather over the network
    - [ ] Write tests: the host sends the state and clients agree; the packet decoder rejects bad weather messages; the protocol version is raised
    - [ ] The weather message in `net`
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Weather systems' (Protocol in workflow.md) (deferred to the end)

## Phase 2: Surfaces that react
- [ ] Task: Wetness and puddles
    - [ ] Write tests: the top-down occlusion map keeps interiors dry; hollows found at import; wet surfaces darken and lose roughness, then dry
    - [ ] The wetness, puddle, ripple and streak shading
- [ ] Task: Snow
    - [ ] Write tests: snow builds on upward surfaces under the sky and melts
    - [ ] Snow build-up
- [ ] Task: Ground marks and decals
    - [ ] Write tests: footprints and tyre tracks by material; mud thrown from wheels; blood and scorch decals get wet, run and wash out; pools spread
    - [ ] Persistent tracks and decal weathering
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Surfaces that react' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Bodies and models that react
- [ ] Task: The layer per object
    - [ ] Write tests: wetness, snow, mud, blood and burns are added where they hit, fade over time and wash off in water
    - [ ] The layer for bipeds, vehicles and first-person weapons
- [ ] Task: The first person
    - [ ] Write tests: rain and snow strike the view model; the visor drops setting is off by default
    - [ ] The first-person effects
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Bodies and models that react' (Protocol in workflow.md) (deferred to the end)

## Phase 4: Water
- [ ] Task: Waves and flow
    - [ ] Write tests: FFT waves are periodic and scaled per map; flow maps follow rivers and waterfalls
    - [ ] FFT waves, small waves and flow maps
- [ ] Task: Interactive water
    - [ ] Write tests: the shallow-water simulation is stable; ripples and wakes from bipeds, vehicles, grenades, projectiles and explosions; it is not in the simulation's state
    - [ ] The local shallow-water simulation
- [ ] Task: The water shader
    - [ ] Write tests: absorption and scattering by depth in the map's colours; refraction, reflections, foam, caustics and underwater fog in golden frames
    - [ ] The water shader; buoyancy from the physics track
- [ ] Task: The golden frames and the budget
    - [ ] Write tests: golden frames for clear, rain or snow and storm in bloodgulch, b30 and d40; the Warthog in b30's surf; the benchmark meets each tier's target; every feature is in the settings with its cost
    - [ ] `docs/engine.md`: the living world
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Water' (Protocol in workflow.md) (deferred to the end)
