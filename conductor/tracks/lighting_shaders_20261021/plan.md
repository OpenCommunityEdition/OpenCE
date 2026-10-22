# Plan: Lighting, shaders, decals and effects drawn

## Phase 1: Why some maps are flat, and the shader model
- [x] Task: Find why some maps are drawn flat fbf93785
    - [ ] Write tests: every stock BSP's lightmap pages import with non-zero texels, and each lit surface has a lightmap index and coordinates (marked, needs `assets/maps`)
    - [ ] Fix the cause in the importer (lightmap page formats, missing pages, surfaces without lightmap coordinates) or the renderer
- [x] Task: The material model fc24c8c5
    - [ ] Write tests: the material record's layout in the pack; the conversion of each `senv` and `soso` field; linear and display values
    - [ ] A material record in `content` (albedo, normal, roughness, metalness, emission, AO, flags), written by the importer for every shader
- [x] Task: Environment shaders 3347a108
    - [ ] Write tests: detail functions, bump height scale, specular masks and self-illumination functions against values worked by hand from the tag's definitions
    - [ ] `senv` in the importer and the level pass: detail, micro detail, bump, specular, reflection tint, self-illumination
- [x] Task: Model shaders 9c06ee24
    - [ ] Write tests: the Xbox multipurpose channel order; change colours from the object's functions
    - [ ] `soso` in the importer and the object pass; the change colours in the snapshot
- [~] Task: Every shader class
    - [ ] Write tests: a count of shaders with no material in each stock map is zero (marked, needs `assets/maps`); each blend mode's pipeline state
    - [ ] `sotr`, `sgla`, `smet`, `spla`, `swat`, lit `schi`/`scex`; the transparent passes on models and the level
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Why some maps are flat, and the shader model' (Protocol in workflow.md) (deferred to the end)

## Phase 2: The bake
- [ ] Task: The bake's inputs
    - [ ] Write tests: sky lights, ambient colours, shader radiosity properties and the sun read from the tags into the bake's scene
    - [ ] The bake scene: the BSP's triangles in a bounding-volume hierarchy, the materials, the lights
- [ ] Task: The path tracer
    - [ ] Write tests: a closed box with one emitter converges to the known irradiance; a sky dome gives the expected AO on a plane
    - [ ] A Metal compute path tracer over the hierarchy, with next-event estimation and bounced light
- [ ] Task: The outputs
    - [ ] Write tests: the directional lightmap's layout and atlas; the probe grid covers the BSP's volume; reflection probe placement per cluster; the cache key changes with the importer's and baker's versions
    - [ ] Directional lightmaps with AO, irradiance probes, reflection probes; written to the cache; background baking with the stock lightmaps as fallback
- [ ] Task: The mood check
    - [ ] Write tests: each cluster's mean brightness and colour against the stock lightmap, within the set limits (marked, needs `assets/maps`)
    - [ ] Tune the bake's sky and emitter scales until every stock BSP passes
- [ ] Task: Conductor - User Manual Verification 'Phase 2: The bake' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Dynamic lighting
- [ ] Task: Objects lit by the world
    - [ ] Write tests: probe interpolation at a point; the nearest reflection probe; the fake light is gone
    - [ ] Objects lit by probes, the sun and reflection probes
- [ ] Task: Sun shadows
    - [ ] Write tests: cascade splits and their stable snapping
    - [ ] Cascaded shadow maps for objects, matched to the baked shadows
- [ ] Task: Dynamic lights
    - [ ] Write tests: `ligh` tag fields and functions; cluster culling assigns each light to the right cells; the shadowed lights are the nearest
    - [ ] Lights from weapons, projectiles, effects, the flashlight and scenery in the snapshot; clustered shading; spot and cube shadows
- [ ] Task: Screen-space effects
    - [ ] Write tests: each pass can be switched off and shown alone from the shell
    - [ ] GTAO, screen-space reflections, contact shadows
- [ ] Task: The camera
    - [ ] Write tests: exposure adapts within the limits; `lens` tag fields; the occlusion test of a flare behind a wall
    - [ ] Auto exposure, bloom, lens flares, glows and light volumes
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Dynamic lighting' (Protocol in workflow.md) (deferred to the end)

## Phase 4: Decals and effects drawn
- [ ] Task: Level decals
    - [ ] Write tests: the scenario's decal placements import with their shader and layer
    - [ ] Level decals drawn in their layers
- [ ] Task: Dynamic decals
    - [ ] Write tests: lifetime, fade, size range, chained decals and recycling at the limit
    - [ ] Deferred decals on the level and objects, placed by impacts and explosions
- [ ] Task: Effects
    - [ ] Write tests: an effect's events and parts fire at their times and markers; impacts choose the effect by material
    - [ ] The effect system in `sim`: triggers from weapons, projectiles, explosions, scenery and vehicles; the effect parts in the snapshot
- [ ] Task: Particles, contrails, weather and detail objects drawn
    - [ ] Write tests: particle sequences, colours, sizes and blend modes; simple motion by velocity and gravity; weather and detail object placements import
    - [ ] The particle, contrail, weather and detail object passes
- [ ] Task: The golden frames and the benchmark
    - [ ] Golden frames in bloodgulch, a10, a30 and b30 at 3840×2160; the benchmark at most 8.33 ms p99
    - [ ] `docs/engine.md`: the material model, the bake, the passes and their switches
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Decals and effects drawn' (Protocol in workflow.md) (deferred to the end)
