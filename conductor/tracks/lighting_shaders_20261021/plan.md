# Plan: Lighting, shaders, decals and effects drawn

## Phase 1: Why some maps are flat, and the shader model [checkpoint: 0a6c7464]
- [x] Task: Find why some maps are drawn flat fbf93785
    - [x] Write tests: every stock BSP's lightmap pages import with non-zero texels, and each lit surface has a lightmap index and coordinates (marked, needs `assets/maps`)
    - [x] Fix the cause in the importer (lightmap page formats, missing pages, surfaces without lightmap coordinates) or the renderer
- [x] Task: The material model fc24c8c5
    - [x] Write tests: the material record's layout in the pack; the conversion of each `senv` and `soso` field; linear and display values
    - [x] A material record in `content` (albedo, normal, roughness, metalness, emission, AO, flags), written by the importer for every shader
- [x] Task: Environment shaders 3347a108
    - [x] Write tests: detail functions, bump height scale, specular masks and self-illumination functions against values worked by hand from the tag's definitions
    - [x] `senv` in the importer and the level pass: detail, micro detail, bump, specular, reflection tint, self-illumination
- [x] Task: Model shaders 9c06ee24
    - [x] Write tests: the Xbox multipurpose channel order; change colours from the object's functions
    - [x] `soso` in the importer and the object pass; the change colours in the snapshot
- [x] Task: Every shader class 296098f6
    - [x] Write tests: a count of shaders with no material in each stock map is zero (marked, needs `assets/maps`); each blend mode's pipeline state
    - [x] `sotr`, `sgla`, `smet`, `spla`, `swat`, lit `schi`/`scex`; the transparent passes on models and the level
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Why some maps are flat, and the shader model' (Protocol in workflow.md) (deferred to the end)

## Phase 2: The bake [checkpoint: 4959ec66]
- [x] Task: The bake's inputs 2756e640
    - [x] Write tests: sky lights, ambient colours, shader radiosity properties and the sun read from the tags into the bake's scene
    - [x] The bake scene: the BSP's triangles in a bounding-volume hierarchy, the materials, the lights
- [x] Task: The path tracer f6c668b6
    - [x] Write tests: a closed box with one emitter converges to the known irradiance; a sky dome gives the expected AO on a plane
    - [x] A Metal compute path tracer over the hierarchy, with next-event estimation and bounced light
- [x] Task: The outputs 58a2519e
    - [x] Write tests: the directional lightmap's layout and atlas; the probe grid covers the BSP's volume; reflection probe placement per cluster; the cache key changes with the importer's and baker's versions
    - [x] Directional lightmaps with AO, irradiance probes, reflection probes; written to the cache; background baking with the stock lightmaps as fallback
- [x] Task: The mood check e5e6c727
    - [x] Write tests: each cluster's mean brightness and colour against the stock lightmap, within the set limits (marked, needs `assets/maps`; by page and by BSP, the clusters are not imported)
    - [x] Tune the bake's sky and emitter scales until every stock BSP passes (two listed as known colour differences: longest, d20 bsp 1)
- [ ] Task: Conductor - User Manual Verification 'Phase 2: The bake' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Dynamic lighting [checkpoint: 1a0e5786]
- [x] Task: Objects lit by the world a9649dfd
    - [x] Write tests: probe interpolation at a point; the nearest reflection probe; the fake light is gone
    - [x] Objects lit by probes, the sun and reflection probes
- [x] Task: Sun shadows 3970a29c
    - [x] Write tests: cascade splits and their stable snapping
    - [x] Cascaded shadow maps for objects, matched to the baked shadows
- [x] Task: Dynamic lights 2c8c92a7
    - [x] Write tests: `ligh` tag fields and functions; cluster culling assigns each light to the right cells; the shadowed lights are the nearest
    - [x] Lights from weapons, projectiles, effects, the flashlight and scenery in the snapshot; clustered shading; spot and cube shadows
- [x] Task: Screen-space effects fa7a6a1c
    - [x] Write tests: each pass can be switched off and shown alone from the shell
    - [x] GTAO, screen-space reflections, contact shadows
- [x] Task: The camera da2a42c5
    - [x] Write tests: exposure adapts within the limits; `lens` tag fields; the occlusion test of a flare behind a wall
    - [x] Auto exposure, bloom, lens flares, glows and light volumes
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Dynamic lighting' (Protocol in workflow.md) (deferred to the end)

## Phase 4: Decals and effects drawn [checkpoint: 2206385c]
- [x] Task: Level decals 0d2368c6
    - [x] Write tests: the scenario's decal placements import with their shader and layer
    - [x] Level decals drawn in their layers
- [x] Task: Dynamic decals 8d9da30a
    - [x] Write tests: lifetime, fade, size range, chained decals and recycling at the limit
    - [x] Deferred decals on the level and objects, placed by impacts and explosions
- [x] Task: Effects 5fd90923
    - [x] Write tests: an effect's events and parts fire at their times and markers; impacts choose the effect by material
    - [x] The effect system in `sim`: triggers from weapons, projectiles, explosions, scenery and vehicles; the effect parts in the snapshot
- [x] Task: Particles, contrails, weather and detail objects drawn 793366fb
    - [x] Write tests: particle sequences, colours, sizes and blend modes; simple motion by velocity and gravity; weather and detail object placements import
    - [x] The particle, contrail, weather and detail object passes
- [x] Task: The golden frames and the benchmark 5c993461 (the benchmark is above its bar: p99 14.19 ms; docs/engine.md)
    - [x] Golden frames in bloodgulch, a10, a30 and b30 at 3840×2160 (awaiting approval)
    - [ ] The benchmark at most 8.33 ms p99: not met, p99 14.19 ms (MetalFX alone takes 6.4 ms at 4K and full scale)
    - [x] `docs/engine.md`: the material model, the bake, the passes and their switches
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Decals and effects drawn' (Protocol in workflow.md) (deferred to the end)
