# The native engine

The native engine (`engine/`) is a new engine in Rust and Swift. It does not
use a 32-bit VM, and it imports the user's maps into a typed 64-bit format.
The decompilation in `source/` is a reference for how the game behaves; none
of its code is compiled into the engine.

## The renderer

### Skies

- A level's first sky is drawn first, at the far plane, centred on the camera.
- Its shaders are drawn unlit, at the model's highest level of detail.
- The sky has no depth test and writes no depth, so the level and the objects
  draw over it.

### Fog

The engine uses the outdoor fog of the level's first sky for the whole level.
It does not follow the fog planes and fog palettes of each cluster. Fog grows
linearly with the distance along the view, from the fog's start to its
opaque distance, up to the fog's maximum density.

Each blend mode of a transparent shader fogs differently, so that the fog is
not counted twice:

| Blend | Fog |
| --- | --- |
| Alpha blend, opaque | Mixed toward the fog's colour |
| Add | Faded toward black |
| Multiply | Faded toward white |
| Double multiply | Faded toward grey (0.5) |

### Transparent shaders

- **Transparent chicago and extended chicago** (`schi`, `scex`):
  - Up to four maps are combined in order, each by the previous map's colour
    function and alpha function.
  - Maps are drawn unlit.
  - An extended chicago is drawn the same way, from the maps the importer reads.
- **Transparent generic** (`sotr`): not imported. Its parts are skipped in the
  sky and drawn as plain surfaces elsewhere.
- **Blending on models:** model parts are blended with alpha blending only.
  The level's transparent surfaces have every blend mode.

### Not yet drawn

These are deferred to a later track:

- Decals on the level (the scenario's decal palette);
- Particle systems and effects (`effe`, `pctl`, `part`), including the effects
  that scenery plays.

The importer does not read these tags yet. A map that has them loads and
draws without them.
