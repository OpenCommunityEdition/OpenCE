# Spec: The menus, launcher and loading screen redesigned

## Overview
The menus are rewritten as a UI system of our own: its screens, layout,
widgets, renderer and motion are designed for the engine and for modern
displays, not bound to the Xbox's widget system or to `ui.map`'s widget
definitions. It stays faithful to the original: the same screens in the
same flow, each doing what the original's did, in the original's design
language (its palette, panel shapes, type hierarchy and feel) and with its
sounds. It is a modern rendering of the original, not a new identity.
The Halo logo stays on the main menu. The launcher grows into a richer pre-game window in
the same language. The loading screen is the Xbox's, recreated natively
in HD and completed. Every screen is designed first in a Claude design
artifact (static mockups, a clickable prototype and motion studies) and
built only once the user approves it.

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
   The artifact uses only our own drawings and the fonts, and the Halo
   logo the user supplies to it; it stays private, and nothing else from
   `ui.map` or the xbe goes into it. Each round's feedback is recorded.
5. **The approved design is binding.** Its palette, shapes, type scale,
   spacing, motion curves, its drawings and its fonts (OpenCE and
   Overpass) are the assets the rest of the track builds with and ships.
   Nothing is substituted after approval without a new design round.

### The menus, rewritten (no setting brings back the old rendering)
6. A UI system of our own in `engine/crates/ui`: screens declared in Rust
   from the approved design (their items, panes and what each item does),
   a layout engine (anchors, columns, panes and dividers, at 3840×2160 and
   scaled by height), widgets (list, tab, pane, option row, card, capsule,
   keycap), focus and input for keyboard and mouse, and a Metal pass that
   draws them as vector shapes. The Xbox widget tree from `ui.map` no
   longer drives the menus; its screens, functions and flow are the
   reference the new screens are checked against: the same choices run
   the same game functions (start a level, set the difficulty, save a
   profile).
7. Faithful content: the strings (titles, items, descriptions, mission
   text) and the sounds are still read from the user's `ui.map` at run
   time, so the words and clicks are the original's. Content bitmaps from
   it (the logo, mission pictures, difficulty emblems) are drawn filtered
   up to their new size.
8. Fonts: titles are set in OpenCE (Newtown, respaced, the face Halo's
   titles were drawn in) and body text in Overpass, at the design's sizes,
   drawn from signed-distance-field atlases built from the font files.
9. The Halo logo is drawn from the user's `ui.map` logo bitmap at run time,
   on the main menu. It is never shipped or committed. The launcher shows
   no logo.
10. 4K native and widescreen: the screens are designed at 3840×2160 across
    the full 16:9 width. Other windows scale by height; wider and narrower
    ones move the anchored parts. The background scene fills the window.
11. Colour and motion: the design's palette, with focus and hover eased in
    and out, transitions between screens and smooth scrolling, independent
    of the frame rate.
12. Every screen can be navigated with the keyboard and the mouse.

### The launcher
13. A pre-game window in the same design language, with no logo and no
    artwork backdrop: only the palette's colours and shading. It holds:
    - Play, Settings, Exit; the Play screen summarises the profile
      (missions finished, hardest difficulty, time played) and its latest
      save (mission, difficulty, when), and its Launch starts the game at
      the main menu, choosing no campaign or save (the main menu's
      Campaign does that);
    - a map library: the campaign, multiplayer and Custom Edition maps,
      their import status, and Import;
    - profiles;
    - multiplayer settings: player name, colour, port, server list;
    - the lobby: a list of the players, one row each with dividers, giving
      each one's team, whether they have the map (or how much of it has
      arrived) and their ping, so that it stays readable with 16 players;
    - the version, what changed, and whether an update is out;
    - data folders, display and window mode.

### The loading screen (the Xbox's, recreated in HD and completed)
14. Original, the default, is the Xbox's screen recreated natively for
    modern displays: the same sweep, trail, timing and constants as
    `progress_bar.c`, drawn at the display's resolution (its units scaled
    by the frame's height, the blur in screen pixels) instead of the
    640×480 frame scaled up. OpenCE's own loading screen is the other
    choice of the setting.
15. It plays between every pair of campaign maps (at the end of each
    mission, before the next) and when a saved game is loaded, not only
    between a10 and a30.
16. The procedural cloud noise from `progress_bar.c`
    (`progress_bar_create_noise_texture`) is added under the light, made
    at the screen's size.
17. Our own hand-drawn picture, drawn at 4K, is shown where `loading.tga`
    would have been.
18. The chant from the user's xbe and the background load stay as they
    are.

## Non-Functional Requirements
- The UI's frame time stays under 1 ms at 3840×2160. Text is crisp at any
  size.
- Nothing from `ui.map` or the xbe (the logo included) is committed or
  shipped. The only exception in the artifact is the logo the user
  supplied to it, which stays in that private artifact.
- New art is our own and credited in `AUTHORS.md`.

## Acceptance Criteria
- The user approves the design artifact before any code is written.
- Golden frames of each screen match the approved mockups.
- Side by side with the reference captures, each screen reads as the
  original's, and each choice runs the same game function the original's
  widget did.
- Every screen can be reached by keyboard and by mouse.
- The launcher has everything listed in item 13.
- The loading screen shows the clouds and the picture at every campaign
  transition. Its timing test case is a10 to a30: no hitch over 33 ms.

## Out of Scope
- New screens beyond the launcher's.
- A gamepad.
- Xbox Live screens.
- Hosting update downloads (the launcher only checks for them).
