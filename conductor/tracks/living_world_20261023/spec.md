# Spec: Weather, water and a world that reacts

## Overview
Halo's maps place fixed weather (rain, snow) and draw static water. This track adds weather systems that change over time, and a world that reacts to weather, liquids and fights:
- surfaces get wet, snow builds up, and puddles form;
- bipeds and vehicles carry rain, snow, mud and blood;
- footprints and tyre tracks stay where they were made;
- water moves with waves, ripples and wakes.

Each map keeps its climate and style. The techniques follow published work:
- weather and wetness after Lagarde's notes on wet surfaces (2012–13) and Frostbite's and Decima's weather;
- open water after Tessendorf's FFT ocean (2001);
- interactive water as a local shallow-water height field.

## Functional Requirements

### Weather systems (opt-in)
1. **The weather setting** (`world.weather`) is off by default; when off, each map has only the weather it places. When on, each outdoor map has a climate: snowy maps stay cold, and desert and canyon maps stay dry. Its weather moves between states (clear, overcast, rain, storm, snow, fog) as fronts carried by the map's wind.
2. **What the weather drives:**
   - the atmosphere track's cloud cover and fog;
   - rain and snow particles, through the particle track;
   - the wind (`wind`);
   - lightning, with a flash light and thunder delayed by distance;
   - the ambient sound.
3. **Weather is cosmetic** by default: projectiles, vehicles and players move as in the stock game. The host sends the weather state to clients, a small and rare message, and `net`'s `PROTOCOL_VERSION` is raised.

### Surfaces that react
4. **Wetness:**
   - surfaces under the sky darken and lose roughness in rain, and dry over time;
   - puddles form in hollows found at import, and rain ripples on them;
   - streaks run down walls.

   A top-down occlusion map keeps interiors and covered areas dry.
5. **Snow** builds up on upward-facing surfaces under the sky, then melts or is blown off. Bipeds and vehicles leave footprints and tracks in it.
6. **Ground marks:**
   - footprints and tyre tracks in snow, sand and mud, by the BSP's material, stay for a while as decals and, where cheap, as a shallow displacement;
   - vehicles throw mud and dirt from their wheels.
7. **Decals:** blood, plasma burns and scorch marks get wet, run and wash out in rain. Pools of blood and Flood fluid spread a little and shine.

### Bodies and models that react
8. **Bipeds and vehicles take on the weather and the fight:**
   - they get wet in rain and dry indoors;
   - snow gathers on the Master Chief's shoulders and on the Warthog;
   - mud on the lower parts from driving and walking;
   - blood and burns from hits.

   This goes in a layer per object that fades over time and is cleaned by water. Weapons in first person show it too.
9. **The first person:** rain and snow strike the view model. Drops on the visor are a setting, off by default.

### Water
10. **Water bodies (`swat`):**
    - open water has FFT waves at a scale fitting each map, such as the Silent Cartographer's beach;
    - still water and swamps have small waves and flow maps;
    - rivers and waterfalls follow flow maps along their direction.
11. **Interactive water:** a local shallow-water simulation around the camera takes ripples and wakes from bipeds, vehicles, grenades, projectiles and explosions. It is cosmetic, not part of the simulation's state. Splash effects come from the particle track.
12. **The water shader:**
    - absorption and scattering by depth, with the map's water colours;
    - refraction, and reflections from SSR, probes or ray tracing;
    - foam at shores and in wakes, and caustics on the bottom;
    - fog underwater.

    The physics track's water queries and buoyancy are used for objects.

## Non-Functional Requirements
- The simulation's results are unchanged with the weather off, and the replay hash test passes. With the weather on, the weather state is part of the replay.
- Everything fits the performance track's tier budgets. The wetness, snow and dirt layers are rendering-quality features, on by default; the weather systems are off by default.

## Acceptance Criteria
- With the weather on, bloodgulch, b30 and d40 go through clear, rain or snow, and storm. Golden frames are approved manually for each.
- A Warthog driven through b30's surf throws spray, leaves a wake and comes out wet. Driving through mud leaves tracks and mud on it.
- Interiors stay dry in rain.
- The benchmark meets the targets at each tier.

## Out of Scope
- Weather that affects play (wind on grenades, slippery ground). It may be a later setting, off by default.
- Water that moves objects beyond Halo's own buoyancy.
