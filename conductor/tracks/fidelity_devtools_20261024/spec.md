# Spec: Fidelity, performance and developer tools

## Overview
The game loads, but a lot of it is wrong. The lighting looks worse than before, and some textures and the ring are overblown. Object shaders flicker between states, and there are black artifacts and texture seams. Few particles and sounds come through, and only some animations play. The main menu barely responds, and the UI overlaps. Vehicles are driven from first person with the weapon out, and objects can't be picked up. Play feels unresponsive, and the Mac heats up because the light bake and probes run during play.

This track merges the whole performance track (performance_20261023) and a new developer-tools scope. It does three things:
- builds the tools to see what's wrong while the game runs;
- builds a dev Blood Gulch that tests every system on one level;
- fixes the bugs until the game is close to the original in faith: its look, gameplay and feel, not a 1:1 copy.

The scope is the systems, on devgulch and Blood Gulch only. Campaign levels are ported and fixed one at a time in later tracks.

Found in play once the animations worked (2026-10-25), and fixed in Phases 12 to 15:
- weapons' firing effects are in the wrong place, and in first person come from the face, not the gun; thrown grenades can't be seen;
- particles still face the camera; explosions are missing parts; blood decals on metal; bullet impacts and wheels blur between frames;
- vehicles get in and out too quickly, collide only with each other, play their turret and gun animations with no one using them, turn their wheels and tracks wrongly, spray too many sprites the same on every surface, and show the wrong HUDs;
- ragdolls stretch and split, and the death camera doesn't follow the body;
- music and many sounds (hitting surfaces and the rest) are missing.

Lighting, shaders and graphics are rewritten and enhanced. Everything else (AI, particles, animation, sound, script logic, gameplay) is ported from `source/` and matched to it first, then extended; Phase 11 measures where it still differs.

No new systems or extensions that weren't in the original. A clean-up or redesign is allowed only where it fixes a bug.

## Functional Requirements

### A. Developer tools
1. **In-game dev panel** (a key toggles it; off in release by default):
   - **Lighting controls:** exposure, sun intensity and colour, bounce, probe weight, AO, emissive gain, bloom, tone-map curve. Each one live, resettable, and saved per map as tuned values.
   - **View modes:**
     - final image, albedo, normals, roughness and metal;
     - lighting only, lightmap only, probes, lightmap UVs and seams;
     - overdraw, wireframe, shader class, mip level;
     - and a **stock lightmap reference view**, developer-only, that draws the map's own lightmaps for side-by-side comparison.
   - **System toggles:** each render pass, particles, decals, sound categories, AI, physics.
   - **Object inspector:** pick with the mouse to see the tag, shader, animation state, physics state, light inputs and sounds playing.
   - **Live timings:** per pass on the GPU, per system on the CPU, and the hitch log.
   - **Audio view:** voices playing, culled and failed, by category.
2. **Launcher Developer tab:**
   - choose the level (dev Blood Gulch by default), the spawn station and the starting view mode;
   - systems on or off at start, and screenshot or capture on launch;
   - one button to launch straight into a station (lighting, vehicles, AI, particles, sounds, animation, UI).
3. **Shell and control socket:** every panel control is also a shell command and an `opence` socket call, so tests and scripts can drive and capture any state.

### B. Dev Blood Gulch (devgulch)
4. At import, the engine copies the user's bloodgulch.map into a devgulch level and places these stations on it:
   - every vehicle, ready to enter;
   - every AI actor type, in spawnable squads;
   - every weapon and equipment, as pickups;
   - a particles and effects row;
   - sound emitters for every sound class;
   - lighting test surfaces for each shader class (metal, emissive, glass, decals);
   - animation stands;
   - triggers for the HUD and UI.
5. devgulch replaces the gathered devmap as the default test level, and it is never shipped (it's built from the user's data).

### C. Performance (all of performance_20261023)
6. Everything in that track's spec, with its benchmark paths limited to devgulch, Blood Gulch and 16 players on Blood Gulch: measuring, the hitch log, benchmark paths, quality tiers, MetalFX, display-link pacing, tick interpolation, no compiles or allocations during play, culling, tile memory, a parallel simulation, and the full graphics settings suite.
7. **The bake and probe updates never run during play.** They run at import or behind the loading screen, are cached, and the level starts already lit. No lighting swaps mid-game. Thermals: the frame cap is respected, and the game is idle when the window isn't focused.

### D. Fidelity fixes, each with a regression test or golden frame
8. **Lighting and shaders:**
   - exposure and tone mapping matched to the original's brightness, so textures and the ring aren't overblown;
   - no black artifacts on models;
   - shader state stable from frame to frame (no metal, decal or emissive flicker);
   - emissives smooth, not blotchy;
   - lightmap and texture seams closed.
9. **Particles:** every effect plays with its full particle set and looks like the original's.
10. **Sound:** every sound class plays: weapons, vehicles, AI dialogue, footsteps, ambience, music and UI.
11. **Animation:** every biped, weapon, first-person, vehicle and device animation plays, including entering, driving and exiting vehicles.
12. **Gameplay:**
    - picking up weapons and objects;
    - entering a vehicle switches to the original's vehicle camera and hides the first-person weapon;
    - driving and gunner seats work as in the original;
    - input feels responsive, with input-to-photon latency measured.
13. **UI:**
    - the main menu is fully navigable;
    - no overlapping elements;
    - the HUD reacts to the player's actions (ammo, shields, grenades, prompts) as the original does.

## Non-Functional Requirements
- The simulation stays deterministic: the replay hash test passes.
- Dev tools cost nothing when off.
- The performance track's targets and its 8 GB budget still apply.
- Lighting stays enhanced for players. The stock view is a developer tool only.

## Acceptance Criteria
- From the launcher's Developer tab, you can open each devgulch station and see that system working.
- The dev panel can isolate any visual bug down to its pass, shader or object.
- No bake or probe work happens after the level starts. The Mac stays cool at the frame cap.
- Each bug listed in the overview has a test and is fixed, and you confirm it in a manual check on devgulch and Blood Gulch.
- The performance track's acceptance criteria are met.

## Out of Scope
- Campaign levels: each is ported and fixed in a later track, once the systems work.
- New features and systems the original didn't have. The atmosphere, living-world, AI-decision and coverage-audit tracks stay deferred.
- Gamepad support.
- Intel Macs.
