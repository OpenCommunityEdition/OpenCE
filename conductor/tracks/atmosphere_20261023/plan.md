# Plan: Modern lighting and atmosphere

## Phase 1: Sky and time of day
- [ ] Task: The atmosphere
    - [ ] Write tests: the lookup tables match reference values; the fit to each stock sky tag gives its colours at the stock time
    - [ ] Transmittance, multiple-scattering, sky-view and aerial-perspective tables for a flat ring
- [ ] Task: Blending with the stock sky
    - [ ] Write tests: the stock sky layer is tinted by the atmosphere's light; per-map fades; golden frames at the stock time within tolerance
    - [ ] The stock sky as a layer
- [ ] Task: The day-night cycle
    - [ ] Write tests: the sun's direction over the ring's turn; the settings (cycle length, start, freeze, stock); night's light stays above the readability floor
    - [ ] `render.time_of_day` and the sun, moonlight from Threshold and starlight
- [ ] Task: Baked light across the day
    - [ ] Write tests: the bake separates the sun from the sky and artificial lights at keyframes; the blend is continuous; interiors keep their light
    - [ ] Keyframed sun and sky bakes and their blend
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Sky and time of day' (Protocol in workflow.md) (deferred to the end)

## Phase 2: Volumetrics
- [ ] Task: Volumetric fog
    - [ ] Write tests: the froxel grid follows the fog tags and fog planes; light shafts behind an occluder; the temporal reprojection is stable
    - [ ] The froxel fog lit by the sun, sky, dynamic lights and probes
- [ ] Task: Volumetric clouds
    - [ ] Write tests: the cloud cover follows the weather map; the cloud shadows move on the ground and the fog; the ring shows through gaps
    - [ ] The raymarched clouds and their shadow map
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Volumetrics' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Shadows, occlusion and emissive light
- [ ] Task: Contact shadows
    - [ ] Write tests: a small object on the ground casts a contact shadow from the sun and from a dynamic light, indoors and out
    - [ ] Screen-space shadow rays
- [ ] Task: Occlusion under objects
    - [ ] Write tests: the capsules and ellipsoids follow bipeds' and vehicles' nodes; the ground darkens beneath them; specular occlusion
    - [ ] The capsule and ellipsoid occlusion pass
- [ ] Task: Emissive light
    - [ ] Write tests: static self-illumination is baked; animated self-illumination becomes area lights with soft shadows
    - [ ] LTC area lights for self-illuminated surfaces
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Shadows, occlusion and emissive light' (Protocol in workflow.md) (deferred to the end)

## Phase 4: Reflections and the look
- [ ] Task: Reflections and glass
    - [ ] Write tests: hierarchical SSR falls back to parallax-corrected probes; ray-traced reflections on hardware RT; glass thickness, refraction and Fresnel
    - [ ] The reflection passes and the glass shader
- [ ] Task: Indoors and outdoors
    - [ ] Write tests: probes and clusters know whether they see the sky; the exposure adapts smoothly between them
    - [ ] Sky visibility and exposure zones
- [ ] Task: The golden frames and the budget
    - [ ] Write tests: golden frames at dawn, noon, dusk and night in bloodgulch, a10, b30 and c10, and indoors in a30 and b40; every pass in the settings with its cost; the benchmark meets the targets
    - [ ] The settings entries; `docs/engine.md`: atmosphere
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Reflections and the look' (Protocol in workflow.md) (deferred to the end)
