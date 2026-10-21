# Spec: The complete animation set

## Overview
The engine plays a small set of animations today: idles and movement for some bipeds, and parts of the first-person view. This track plays every animation in Halo's animation graphs (`antr`) as the original does, for players, AI, vehicles, weapons, devices and the first-person view, and checks them against the reference game's poses.

## Functional Requirements
1. **The graph:** the full `antr` structure: modes, weapon classes, weapon types, the animation lists of each, the units' seats, the weapon and vehicle blocks, the device blocks, the first-person blocks and the inheritance between graphs. A missing animation falls back as Halo's does.
2. **Frames:** base, overlay and replacement animations; the compressed and uncompressed frame data; the nodes' default positions; the frame events (sounds, effects, footsteps, the "key" frame); looping and the exit frames.
3. **Bipeds:** idle, turning in place, moving in eight directions with blending by speed, airborne, jumping, landing (soft and hard), crouching, aiming and looking overlays, firing and recoil, reloading, throwing grenades, melee, picking up and switching weapons, the flinch and the ping, death (by direction and damage type) and their transitions.
4. **Vehicles and seats:** entering and leaving each seat from its side, driving and gunning, the boarding animations, the vehicle's own animations (suspension, steering, turret aim, the Warthog's wheels and the Banshee's flaps), and being thrown out of a wreck.
5. **Weapons:** each weapon's own animations (firing, reloading, charging, overheating, the Needler's needles) and their states.
6. **First person:** the arms and the weapon together: ready, idle, moving, firing, reloading (and empty reloads), melee, the grenade throw, switching, the overheated vent and the zoomed state.
7. **AI:** the actors' animations as their behaviour calls them: the alerts, the dives, the evades, the berserk and the surrender, the Grunts' flee, the Jackals' shield, the Elites' cloak, the Hunters' charge, and the Flood's forms; the AI's look and aim overlays.
8. **Devices:** machines, controls and lifts by their position and power functions.
9. **Cutscenes:** the scripted `custom_animation` and `unit_custom_animation` of a cutscene, with their relative positions to the cutscene flags and the camera's animation from `recorded_animation` and camera tracks.
10. The poses blend as Halo's do: the order of overlays, the blend between the old and new animation on a transition, and the aim overlays' limits.

## Non-Functional Requirements
- Animating 64 bipeds costs under 1 ms of the simulation's tick on the M1 Max.
- The `station animation` of the test map plays every animation of a chosen graph.

## Acceptance Criteria
- A test compares the engine's node transforms with traces from the reference game (`tools/engine_animation_reference.py`) for a set of bipeds, vehicles and weapons in every listed state, within 0.1 mm and 0.1°.
- Entering, driving and leaving each stock vehicle from each seat looks as in the original (manual check).
- The first two missions' cutscenes play their animations in place (manual check, with the coverage track's audit to come).

## Out of Scope
- New animations, or blending that Halo does not do.
- Facial animation (Halo has none).
