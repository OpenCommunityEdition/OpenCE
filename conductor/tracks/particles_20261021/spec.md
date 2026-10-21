# Spec: The complete particle system

## Overview
The lighting track draws particles, contrails and weather with simple motion. This track completes their behaviour as Halo's particle code has it: particle systems with their emitters and physics, collisions with the world, particles attached to objects, and the effects that depend on them. Fire, smoke, sparks, blood, plasma, the Flood's spores, waterfalls and snow then move as in the original.

## Functional Requirements
1. **Particles (`part`):** every field: the bitmap and its sequences and animation rate, the shader flags, the size and colour over life, the rotation, the fade by distance and angle, the "collision" and "dies on contact" flags, the effects on collision and death, and the physics (`pphy`) they use.
2. **Particle physics (`pphy`):** gravity, drag, wind, density, the collision with the BSP and with water (bounce, slide, stop and die), using the physics track's collision queries.
3. **Particle systems (`pctl`):** their types, the emitters (point, sphere, cylinder, plane), the emission rates and their functions, the states and their transitions, the physics constants, the complex sprites, the lit and unlit forms, the attachment to objects and markers.
4. **Effects (`effe`)' particle parts:** the counts, the velocity and its cone, the angular velocity, the radius, the relative directions, the "inherit velocity" flags, the scaled-by functions from the effect's object.
5. **Contrails (`cont`):** points and their lifetimes, the shapes (tail, cross, ribbon), the textures' scroll, the colours over life, the "point emission" rates; attached to projectiles and vehicles.
6. **Weather (`rain`):** the particle types, their density by the BSP's weather palette, the wind (`wind`) from the BSP's wind palette, the camera's motion through them and their fade near the camera.
7. **Lighting:** lit particles take the light probes and dynamic lights; emissive particles feed bloom; particles that light the world emit their `ligh`.
8. **Limits:** Halo's limits on particle counts are kept as defaults; above them, the oldest are recycled, and a setting raises the limit.

## Non-Functional Requirements
- 20,000 particles updated and drawn in under 1.5 ms on the M1 Max (updated on the GPU where the physics allows it, on the CPU where the simulation must know them).
- Particles that the simulation never reads are not part of the network state; those it does (collisions that cause effects) are deterministic.

## Acceptance Criteria
- Each stock particle system can be shown alone in the test map (`station particles`), and looks as in the reference app (manual check of a recorded set).
- The fire in a10, the snow in d40, the spores in the Flood levels and the plasma impacts look and move as in the original (manual check).
- The particle budget is met in the busiest scenes of the campaign.

## Out of Scope
- Particles Halo does not have.
