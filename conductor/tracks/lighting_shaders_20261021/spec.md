# Spec: Lighting, shaders, decals and effects drawn

## Overview
The engine draws the level today as its base and detail maps times the stock lightmap. Objects are lit by a fixed fake light, and there are no bump maps, specular, reflections, self-illumination, dynamic lights, lens flares, decals or particles. The result looks flat.

This track replaces that with a modern lighting model and a shader model that fits it. On 2026-10-21 the project decided the lighting is **enhanced only**, with no "original" mode. The aim is still the original in spirit: the same light colours, the same mood in each area, and the same readability of enemies, pickups and paths. Every shader class Halo has is drawn, and decals and particles appear in the world. Particle *behaviour* (physics, collisions, the full particle system) is a later track; this one draws what the effects tags describe, with their own simple motion.

## Functional Requirements

### The shader model
1. All surfaces use one physically based model: albedo, normal, roughness, metalness, emission and ambient occlusion. Each Halo shader class maps onto it from its own tag fields.
2. **Environment** (`senv`) maps as follows:
   - the base map (decoded from display values to linear) gives the albedo;
   - the primary and secondary detail maps and the micro detail map are combined as the shader's detail function says;
   - the bump map gives the normal, with its height scale;
   - the specular mask (the base map's or the bump map's alpha, as the flags say) gives the specular amount;
   - the perpendicular and parallel colours and brightness give the reflection's strength and tint;
   - the self-illumination map, colours and animation functions give the emission;
   - the alpha-tested flag is kept.
3. **Model** (`soso`) maps as follows:
   - the base map gives the albedo;
   - the multipurpose map gives specular, self-illumination, colour change and the detail mask, with the Xbox's channel order (unlike the PC's);
   - the change colour comes from the object's colour functions (the armour colour, for example);
   - the detail map and its function, the reflection cube map, and the perpendicular and parallel brightness are used;
   - the self-illumination animation is kept.
4. **Transparent shaders** each get their own pass, with the blend mode their tags say:
   - chicago and extended chicago (`schi`, `scex`), extended to the new lighting where they are lit;
   - generic (`sotr`): its stages are evaluated as its tag's map and colour stages say;
   - glass (`sgla`): tinted reflection and refraction, bump-mapped;
   - meter (`smet`), plasma (`spla`) and water (`swat`): water is bump-mapped and animated, with reflection and refraction, the original's colours and fog.
5. Every blend mode of the transparent shaders is supported on models as well as on the level (alpha, add, multiply, double multiply, alpha-multiply-add).

### Baked lighting
6. At import, each structure BSP's lighting is re-baked with a GPU path tracer (Metal compute, the engine's own; no external baker). Its light comes from what Halo's own radiosity used:
   - the sky's lights (direction, colour, power) and its indoor and outdoor ambient;
   - each shader's radiosity properties (power, the colour of emitted light, the tint of light passing through);
   - the sun.
7. The bake writes:
   - **Directional lightmaps:** irradiance with its dominant direction (or L1 spherical harmonics), at a higher resolution than the stock pages, with bounced light;
   - **Ambient occlusion** baked into the lightmaps;
   - **Light probes:** a grid of irradiance probes through the BSP's volume, to light objects and particles;
   - **Reflection probes:** cube maps captured at the BSP's clusters, for specular reflections.
8. The bake is checked against the stock lightmaps: each cluster's mean brightness and colour must be close to the original's, so each area keeps its mood. The test reports the difference by cluster.
9. Baked results are kept in the cache folder beside the `.ocpak`. They are rebuilt when the importer's or the baker's version changes. A bake that is not yet done falls back to the stock lightmaps, so a map is playable at once. Bakes run in the background while the game plays.

### Dynamic lighting
10. **The sun:** cascaded shadow maps for objects, so bipeds and vehicles cast and receive sun shadows that agree with the baked shadows of the level.
11. **Dynamic lights** (`ligh`): point and spot lights from weapons, projectiles, effects, the flashlight and lit scenery. They use their colour, radius, falloff, gel map and animation functions. Shadows come from cube or spot shadow maps for the nearest few, chosen each frame. Lights are culled in clusters, so that hundreds can be on screen.
12. **Objects** are lit by the light probes, the sun with its shadows, the dynamic lights and the reflection probes, replacing the fixed fake light.
13. **Screen-space effects:**
    - ambient occlusion (GTAO) for what the bake cannot know (objects on the level, objects on each other);
    - screen-space reflections over the reflection probes;
    - contact shadows.
14. **The camera:**
    - automatic exposure with limits per area;
    - bloom;
    - lens flares (`lens`) from the BSP's lens flare markers and from lights, with their reflections, rotation and occlusion test;
    - glows (`glw!`) and light volumes (`mgs2`);
    - the HDR output and tone map the renderer already has.

### Decals
15. The scenario's level decals are drawn on the level with their shader, layer and colour.
16. Dynamic decals (`deca`), such as bullet holes, scorch marks and blood, are placed by impacts and explosions. They follow the tag's flags, lifetime, fade, size range and chained decals. They are projected onto the level and onto objects (deferred decals), and the oldest are recycled when the limit is reached.

### Effects drawn
17. Effects (`effe`) are triggered as their tags say: their events, their parts (particles, lights, decals, sounds, other effects) and their locations on markers. The triggers include weapons firing, projectiles hitting surfaces (by material type), explosions, scenery's own effects and vehicles' effects.
18. Particles (`part`), particle systems (`pctl`) and contrails (`cont`) are drawn with their bitmaps, sequences, colours, sizes and blend modes, lit by the light probes where their shader says. In this track they move only by their own velocities, gravity and lifetime; collisions and physics come in the particle-system track.
19. Weather (`rain`) is drawn where the BSP's weather palette places it, and detail objects (`dobc`, such as grass) are drawn from the BSP's detail object data.

## Non-Functional Requirements
- 4K at 120 frames a second on the M1 Max stays the target: the benchmark's p99 must stay at most 8.33 ms on bloodgulch at 3840×2160. Effects that cannot fit have a quality setting, and the default is chosen to fit.
- The bake of a stock multiplayer map takes under a minute on the M1 Max; a campaign BSP under five. The game is playable with the stock lightmaps meanwhile.
- Every new pass can be switched off from the shell (`render.<pass>`), and shown alone (`render.view <buffer>`), for debugging.

## Acceptance Criteria
- Golden frames (manual approval) from fixed cameras in bloodgulch, a10, a30 and b30, at 3840×2160, against the stock look in the reference app. The approval is for "the original in spirit".
- Every shader class of every stock map is drawn (a test counts the shaders with no material, and expects none).
- The flat maps the user saw are drawn lit.
- Bullet holes, scorch marks, muzzle flashes, impacts and explosions appear when weapons fire.
- The benchmark meets the target.

## Out of Scope
- Particle collisions and physics (the particle-system track).
- Sound (the sound track).
- Ray tracing that needs hardware the M1 Max lacks. Everything is compute or raster.
