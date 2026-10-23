# Spec: Modern lighting and atmosphere

## Overview
The lighting track gave the engine physically based shading, re-baked light, probes, dynamic shadows, GTAO and SSR. This track brings the lighting up to current games' standards outdoors and indoors. It adds a physically based sky and a day-night cycle blended with each map's own sky, volumetric clouds and fog with light shafts, contact shadows, emissive surfaces that cast light, and reflections on glass. The levels are on Halo, a ring world: the sky keeps the ring's arch, the gas giant and the stars, and the light keeps each map's colours and mood. The techniques follow published work:
- the sky and aerial perspective after Hillaire (2020);
- the clouds after Guerrilla's Nubis (2017 and 2023);
- froxel volumetric fog after Wronski (2014) and Hillaire (2015);
- contact shadows after Bend Studio's screen-space shadows (2023);
- emissive area lights with linearly transformed cosines (Heitz et al. 2016).

## Functional Requirements

### Sky and time of day
1. **The atmosphere** is rendered from transmittance, multiple-scattering, sky-view and aerial-perspective lookup tables for a flat ring surface. Its colours are fitted to each map's sky tag (its colours, fog and lights), so the stock time of day looks like the stock sky.
2. **Blending with the stock sky:** the stock sky model is kept as a layer: the ring's band, Threshold, the stars and the clouds painted into it. Its colour follows the atmosphere's light, and it can be faded per map where the procedural sky replaces it.
3. **The day-night cycle** is on by default (`render.time_of_day`; "stock" keeps the map's fixed time). The sun's direction follows the ring's turn. The cycle length and the start time are settings, and the time can be frozen. Night uses moonlight from Threshold and starlight, kept bright enough for play.
4. **Baked light across the day:**
   - the bake stores the sun's contribution apart from the sky's and the artificial lights' contribution, at keyframes through the day (for example eight), and the renderer blends them;
   - the sun's direct light and shadows are always dynamic;
   - interiors keep their baked artificial lights;
   - light through openings changes with the day.

### Volumetrics
5. **Volumetric fog:** a froxel grid lit by the sun with its shadows, the sky, the dynamic lights and the probes. It gives light shafts through openings and foliage. It uses the maps' fog tags (density, colour, height) and fog planes, with temporal reprojection.
6. **Volumetric clouds:** raymarched from a weather map and noise, at their altitude above the ring.
   - They cast moving shadows on the ground and the volumetric fog.
   - They are lit by the atmosphere, and their colours are matched to the map's sky.
   - The ring's arch shows between them.
   - Their cover is set per map and by the weather track.

### Shadows and occlusion
7. **Contact shadows** come from screen-space shadow rays for the sun and for the nearest dynamic lights, indoors and outdoors. They fill in the detail that shadow maps miss, where objects meet the ground and the walls.
8. **Occlusion under objects:** capsule and ellipsoid occlusion for bipeds and vehicles darkens the ground's baked and probe light beneath and around them, combined with GTAO and with specular occlusion. Objects then sit on the ground instead of floating.

### Emissive light
9. **Self-illuminated surfaces light the world:** the Covenant's lights, the Forerunner panels and the screens.
   - Static ones are baked with their emitted colour.
   - Animated or moving ones are polygonal area lights (LTC) and light their surroundings with soft shadows.
   - Plasma projectiles and other emissive effects keep their `ligh` lights.

### Reflections
10. **Glass and glossy surfaces:**
    - hierarchical screen-space reflections, with fallback to parallax-corrected reflection probes;
    - on Macs with hardware ray tracing (M3 and later), ray-traced reflections at the Ultra tier;
    - glass (`sgla`) with thickness, tinted refraction, a Fresnel term and its bump map.
11. **Indoors and outdoors:** probes and clusters know whether they see the sky, so the time of day, the clouds and the weather light only what is outside. Exposure adapts smoothly between the two.

### The look
12. Each map keeps its palette, fog colour, sky colour and readability. Golden frames at the stock time of day stay close to the lighting track's approved frames, within a stated tolerance.

## Non-Functional Requirements
- Every feature has a cost at each of the performance track's tiers, and the Ultra tier fits 4K/120 on the M1 Max with upscaling.
- Every pass can be switched off (`render.<pass>`) and shown alone in the shell.
- These are rendering-quality features: on by default.

## Acceptance Criteria
- Golden frames at dawn, noon, dusk and night in bloodgulch, a10, b30 and c10, and indoors in a30 and b40, approved manually as "the original in spirit".
- Contact shadows and occlusion under bipeds and vehicles are visible in the golden frames.
- The benchmark meets the performance track's targets with every feature on at Ultra.

## Out of Scope
- Weather systems, wetness, water simulation and the world's reaction (the living-world track).
- A full ray-traced global illumination; the bake stays.
