# Spec: The Halo UI, the launcher and the Xbox loading screen

## Overview
The engine has its own simple menus today. This track replaces them with the Xbox game's own UI, read from the user's `ui.map`: its widgets, layouts, fonts, strings, sounds and the 3D background scene. It is drawn at native resolution, with modern sharpness and mouse support, so it is 1:1 with the original in spirit, look and feel. The launcher (the window before the game) is cut to Play, Settings and Exit; everything else is reached from the game's UI. Between campaign maps, the Xbox loading screen plays as an easter egg, with its panning light and the monk chant, and the next map loads behind it.

## Functional Requirements

### Reading `ui.map`
1. The importer no longer filters out `ui.map`. It imports it into a UI pack: widget definitions (`DeLa`), their children, event handlers, bitmaps, fonts (`font`), string lists (`str#`, `ustr`), colour tables, sounds, the virtual keyboard, and the UI scenario with its background objects, camera and lights.
2. All of it goes through the bounds-checked readers, and every reference is checked.

### The widget system
3. A widget tree in Rust, run on the simulation's tick, with Halo's behaviour as the tags describe it:
   - widget types: container, text box, spinner list, column list, game model, movie and custom;
   - navigation between children, with focus and the up/down/left/right links;
   - event handlers and their functions (go to screen, go back, load game, start campaign, change setting and so on), wired to the engine;
   - conditional widgets, list items from data (saved games, profiles, maps, network games, players), spinners, and text from string lists with formatted values;
   - animation of widgets: their own bitmap sequences and the transitions;
   - sounds on focus, select and back.
4. Each UI function the screens call is listed. One the engine cannot do yet is listed in `docs/engine.md` and does nothing, as `UNWIRED.md` does for the VM port.

### Drawing it
5. The UI is drawn in its 640×480 layout, scaled to the window's height and centred, with the background scene filling the width. Its bitmaps are drawn with high-quality filtering. Its fonts are drawn from the font tags, sharpened by a signed distance field built at import, so text is crisp at 4K.
6. The background scene (the Xbox's rotating Halo ring and its lighting) is drawn by the engine's renderer, with the new lighting.

### Input
7. Keyboard: the arrow keys, Enter, Escape and Tab, as the Xbox pad's directions, A, B and the shoulder buttons.
8. Mouse: hovering focuses a widget, clicking selects it, the wheel scrolls lists, and right-click goes back. Hit boxes come from the widgets' bounds.

### What the screens reach
9. Campaign (new game, difficulty, load a saved game), multiplayer (system link: host, join, game types, maps), settings (controls, look sensitivity, inversion, audio, video) and profiles. Their settings are the engine's settings, saved as they are now.
10. The pause menu and the post-game carnival screens come from the same widgets.

### The launcher
11. The launcher window shows only **Play**, **Settings** and **Exit**. Play opens the game on its main menu. Settings holds what must be set before the game starts: the data folders, the display and the window mode.

### The Xbox loading screen
12. Between campaign maps (the end of one mission and the start of the next, and loading a saved game), the Xbox loading screen is drawn, reimplemented from the behaviour of `source/interface/progress_bar.c`: its background, the panning light across it, its timing and its fade. It shows the progress of the load.
13. The monk chant is read from the user's own Xbox `default.xbe` when the data folder has it (the compressed tone the executable embeds), decoded and played under the screen. Without it, the screen is silent. Bungie's audio is never shipped or written outside the cache.
14. The next map is loaded and imported in the background while the screen plays. The screen can be skipped with any key once the map is ready. It fades into the first frame of the next mission, which then fades in, with no black frames or hitches between.
15. The setting `ui.loading_screen` (on by default) turns it off, for a plain fade instead.

## Non-Functional Requirements
- The UI's frame time is under 1 ms at 3840×2160.
- No UI code reads past a tag's data: fuzzing covers the UI pack.
- Nothing from `ui.map` or the executable is committed or shipped.

## Acceptance Criteria
- Golden frames (manual approval) of the main menu, campaign difficulty, the multiplayer lobby, the settings and the pause menu, next to the reference app's.
- Every screen of the Xbox UI can be reached with the keyboard and with the mouse.
- A campaign started from the main menu reaches a10, and the end of a10 plays the loading screen and starts a30 with no hitch over 33 ms.
- The launcher shows only Play, Settings and Exit.

## Out of Scope
- The PC version's menus (the VM port keeps them).
- A gamepad.
- Xbox Live screens: they are hidden.
