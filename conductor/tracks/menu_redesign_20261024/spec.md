# Spec: The menus, launcher and loading screen redesigned

## Overview
The Xbox UI from `ui.map` stays the menus' structure: its screens,
navigation, event handlers, strings and lists. Its look is rebuilt in the
original's own design language: the same palette, panel shapes, type
hierarchy and feel, drawn sharp at native resolution and laid out for
widescreen. It is a modern rendering of the original, not a new identity.
The Halo logo stays. The launcher grows into a richer pre-game window in
the same language. The loading screen is completed as the Xbox had it.
Every screen is designed first in a Claude design artifact (static
mockups, a clickable prototype and motion studies) and built only once
the user approves it.

## Functional Requirements

### The design language
1. A design reference is drawn up from the original: the palette (the
   Xbox menus' blues, teals and highlight colours, sampled from the user's
   own copy), panel and frame shapes, corner and bevel treatment, spacing,
   the type hierarchy (titles, items, captions), focus and selection
   styles, and its sounds.
2. Every redesigned screen follows it. The test is that a player who
   knows the Xbox menus recognises each screen at a glance.
3. Reference captures of the original (taken from the user's copy with the
   developer shell) stay local, in the gitignored `assets/` folder, and are
   compared side by side with each mockup there.

### Design first
4. A private Claude design artifact holds every screen as a static
   mockup:
   - the launcher and its settings;
   - the main menu over its scene;
   - campaign and difficulty, saved games;
   - multiplayer (host, join, game types, maps);
   - settings, profiles, the pause menu, the post-game screens;
   - the loading screen.

   A clickable prototype wires them into the real flow. Motion studies
   cover focus and hover, transitions, scrolling and the loading sweep.
   The artifact uses only our own drawings and the fonts. The Halo logo
   appears there as a marked placeholder, since nothing from `ui.map` or
   the xbe goes into it. Each round's feedback is recorded.
5. **The approved design is binding.** Its palette, shapes, type scale,
   spacing, motion curves, its drawings and its fonts (OpenCE and
   Overpass) are the assets the rest of the track builds with and ships.
   Nothing is substituted after approval without a new design round.

### The menus in that language (no setting brings back the old rendering)
6. Fonts: titles are set in OpenCE (Newtown, respaced, the face Halo's
   titles were drawn in) and body text in Overpass, each matched to the
   original's sizes and weights. They are drawn as signed-distance-field
   atlases built from the font files. The strings, formatting and
   replacements still come from `ui.map`.
7. The Halo logo is drawn from the user's `ui.map` logo bitmap at run time,
   on the main menu where the original placed it, and on the launcher
   once the UI is imported. It is never shipped or committed. Before the
   import, the launcher shows the OpenCE wordmark.
8. Panels, frames, highlights and arrows are redrawn as vector shapes in
   the original's forms, replacing the low-resolution bitmaps. Content
   bitmaps (map previews, the 3D ring scene) stay.
9. Colour and motion: the original's palette, with focus and hover eased
   in and out, transitions between screens and smooth scrolling. Sounds
   still come from the widgets' tags.
10. 4K native and widescreen: the screens are designed at 3840×2160 and
    laid out across the full 16:9 width by layout rules (anchors, columns
    and stretching from the design), not on the original's 640×480 grid.
    Other windows scale by height; wider and narrower ones move the
    anchored parts. The background scene fills the window. Content
    bitmaps from `ui.map` (the logo, mission pictures) are filtered up to
    their new size.
11. Every screen can be navigated with the keyboard and the mouse.

### The launcher
12. A pre-game window in the same design language, with the Halo logo (as
    in item 7) over an artwork backdrop (our own art). It holds:
    - Play, Settings, Exit;
    - a map library: the campaign, multiplayer and Custom Edition maps,
      their import status, and Import;
    - profiles;
    - multiplayer settings: player name, colour, port, server list;
    - the version, what changed, and whether an update is out;
    - data folders, display and window mode.

### The loading screen (the Xbox's, completed)
13. The loading screen plays between every pair of campaign maps (at the
    end of each mission, before the next) and when a saved game is loaded,
    not only between a10 and a30.
14. The procedural cloud noise from `progress_bar.c`
    (`progress_bar_create_noise_texture`) is added under the light.
15. Our own hand-drawn 320×240 picture is drawn where `loading.tga` would
    have been.
16. The sweep, its trail, the chant from the user's xbe and the background
    load stay as they are.

## Non-Functional Requirements
- The UI's frame time stays under 1 ms at 3840×2160. Text is crisp at any
  size.
- Nothing from `ui.map` or the xbe (the logo included) is committed,
  shipped or put in the artifact.
- New art is our own and credited in `AUTHORS.md`.

## Acceptance Criteria
- The user approves the design artifact before any code is written.
- Golden frames of each screen match the approved mockups.
- Side by side with the reference captures, each screen reads as the
  original's.
- Every screen can be reached by keyboard and by mouse.
- The launcher has everything listed in item 12.
- The loading screen shows the clouds and the picture at every campaign
  transition. Its timing test case is a10 to a30: no hitch over 33 ms.

## Out of Scope
- New screens beyond the launcher's.
- A gamepad.
- Xbox Live screens.
- Hosting update downloads (the launcher only checks for them).
