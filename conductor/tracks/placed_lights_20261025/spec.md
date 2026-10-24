# Spec: Placed lights

## Overview

The engine lights a level the way Halo does. The **static** light is the
bake: the sky's sun and sky lights and the structure's self-illuminated
surfaces (`bake/src/scene.rs`). The **dynamic** lights are the `ligh` tags
that objects and effects carry at their markers, sorted into the view's
cells and added by the level's and the objects' shaders, with the nearest
four casting cube shadows (`LightPass.swift`, `content/src/light.rs`).
There is no **ambient** term other than the baked light and the probes.

What the engine lacks is a way to *place* a light. A light exists only
because a tag puts it on an object or a surface glows, so an interior
that looks flat under the enhanced lighting can only be fixed by changing
the shading. This track adds placed lights, in the manner of current
engines, without a second lighting path:

- one ECS component, `LightSource`, with a **mobility**: Baked, Stationary
  or Movable;
- the `ligh` tags' lights become Movable light sources, so every light
  goes through the same component and the same clustered pass;
- **ambient** is not a constant fill: it is probe overrides (volumes that
  scale, tint or replace the probes' light) and a sky-visibility flag per
  volume;
- lights are placed and tuned from the dev panel, each control also a
  shell command and a control socket call;
- they are kept per map in an overlay file on the user's machine, beside
  the map's lighting tuning, never in the map or the repository.

It comes before the atmosphere track (`atmosphere_20261023`), whose
emissive area lights, baked artificial light across the day and
indoor/outdoor probes use this component and its mobility.

## Functional requirements

### The component
1. `LightSource` holds a shape (point, spot, and later area for the
   atmosphere track), position and orientation, colour (linear), intensity,
   radius, the cone's falloff and cutoff angles, an optional gel, whether
   it casts shadows, whether it makes highlights, and its mobility.
2. **Mobility:**
   - **Baked:** only the bake uses it, for direct and bounced light into
     the lightmaps and the probes. It costs nothing in play. It cannot move
     or change in play.
   - **Stationary:** the bake stores its bounced light only; its direct
     light and shadow are drawn in play, so its colour and intensity may
     change in play (flicker, switched off), and its bounce is scaled
     with it. It cannot move.
   - **Movable:** drawn in play only, through the clustered pass. It may be
     attached to an object or a marker.
3. The `ligh` lights of objects and effects are made Movable light sources
   from their tags, with the tag's colour, radius and function behaviour as
   now. Their frames do not change (the light tests and golden frames
   match).
4. The shadowed lights are chosen as now (the nearest few), from Movable
   and Stationary lights together, with Stationary ones preferred when
   they are equally near.

### Ambient
5. A **probe override** is a box or sphere volume with a blend distance,
   which scales, tints or replaces the probes' light inside it, and a
   flag saying whether it sees the sky. The bake writes the flag into the
   probes it covers; the atmosphere track reads it.
6. There is no constant ambient light.

### The bake
7. The bake takes the Baked and Stationary lights with the sun and the
   glowing surfaces: Baked lights' direct and bounced light, Stationary
   lights' bounced light kept per light so that it can be scaled in play.
8. Lights change the bake cache's key: changing a Baked or Stationary
   light bakes again, behind the loading screen or at the panel's
   "bake" command, never during play. A Stationary light's colour or
   intensity changing in play needs no bake.
9. A Stationary light's bounce is kept for a bounded number of lights per
   map (for example 16); above that, the panel refuses and says why.

### Placing and editing
10. The dev panel has a lights page: the map's placed lights and probe
    overrides, each selectable, with its controls; add, duplicate and
    delete; place at the crosshair or at the camera; move and turn with
    a gizmo drawn by the debug drawing.
11. While a Baked or Stationary light is being edited, it is drawn as
    Movable so that the change is seen at once; the panel marks the bake
    as out of date until it is baked again.
12. Every control is a shell command and a control socket call
    (`light add`, `light set <id> <field> <value>`, `light remove`,
    `light list`, `probe_override …`, `light bake`), so a light set can be
    scripted and tested headless.
13. Debug views: each light's reach and cone, its mobility by colour, the
    clusters' light counts, and the probe overrides' volumes.

### The overlay
14. A map's placed lights and probe overrides are saved in the tuning
    folder beside the map's lighting tuning (`<map>.lights.toml`), keyed
    by the map's hash; an overlay for another version of the map is not
    loaded, and the panel says so.
15. A map without an overlay is lit exactly as now. Stock comparisons and
    the fidelity checks run without overlays.
16. The overlay is untrusted input: a file with out-of-range values,
    unknown fields or too many lights is refused with a message, never a
    crash, and its reader is fuzzed.
17. devgulch gets a lighting station with one light of each mobility and a
    probe override, made at import as the rest of devgulch is.

## Non-functional requirements
- A map with no placed lights renders in the same time as before.
- With 64 Movable lights in view on devgulch's lighting station, the frame
  stays within the frame cap on the user's Mac.
- Baking a map with 16 Stationary lights adds no more than half again to
  its bake time.
- Nothing in the simulation depends on lights: they are render state, and
  the sim's reference traces are unchanged.

## Acceptance criteria
- The existing light tests and golden frames match with the `ligh` lights
  moved onto `LightSource`.
- Each mobility behaves as stated: Baked lights appear only in the
  lightmaps and the probes; Stationary lights' direct light changes in play
  without a bake and their bounce follows; Movable lights move.
- A probe override changes only the probes inside it, blended at its edge.
- Every panel control works from the shell and the socket, headless.
- The overlay round-trips, is refused for another map version, and its
  fuzz target runs in the test step.
- A report measures the frame and bake costs above.

## Out of scope
- Area lights and the time of day (the atmosphere track uses this
  component for them).
- Writing placed lights into map files or tags, or shipping light sets for
  stock maps.
- Lights placed by scripts or by players in a game.
- A constant ambient light.
