# Plan: The menus, launcher and loading screen redesigned

## Phase 1: The design language and the design artifact
- [x] Task: Reference captures of the original ba7d643e
    - [x] Write tests: `OpenCEEngine --capture-ui` walks every screen headless and writes PNGs to `assets/ui_reference/` (gitignored); a test that the folder is ignored
    - [x] The capture command; the palette sampled from the captures into a local swatch file
- [x] Task: The design reference c82db598
    - [x] `docs/ui_design.md`: the palette, panel and frame shapes, bevels, spacing, the type scale (OpenCE and Overpass sizes matched to the original's), focus and selection, motion curves, sounds
- [~] Task: Static mockups 0df3b584
    - [x] A private Claude design artifact with every screen in the spec, drawn from the reference with our own art and a marked logo placeholder
    - [ ] Rounds of the user's feedback, recorded in the artifact and in `docs/ui_design.md`
- [x] Task: The clickable prototype and motion studies 54be3505
    - [x] The screens wired into the real flow (focus, hover, back)
    - [x] Motion studies of focus, transitions, scrolling and the loading sweep
- [ ] Task: Conductor - User Manual Verification 'Phase 1: The design language and the design artifact' (Protocol in workflow.md): the user approves the design, and everything after builds from it

## Phase 2: A UI system of our own
- [ ] Task: Screens, layout and widgets
    - [ ] Write tests: each screen declared from the design; the layout engine places anchors, columns, panes and dividers at 3840×2160 and at 4:3, 16:10 and 21:9, in 1080p, 1440p and 4K windows; hit boxes follow the layout
    - [ ] `engine/crates/ui`: screen declarations, the layout engine, the widgets (list, tab, pane, option row, card, capsule, keycap)
- [ ] Task: Faithful to the original
    - [ ] Write tests: for every screen, each choice runs the same game function as the original widget (checked against the widget tree from `ui.map`); strings and sounds read from `ui.map`
    - [ ] The flow: main menu, profiles, campaign, difficulty, settings, player setup, the pause menus, post-game
- [ ] Task: Focus, input and motion
    - [ ] Write tests: keyboard and mouse focus, hover, back; focus, transitions and scrolling ease over time, independent of the frame rate
- [ ] Task: Fonts from the font files
    - [ ] Write tests: SDF atlases built from OpenCE and Overpass; metrics and kerning at the design's 4K sizes, the same widths at every scale (height / 2160)
    - [ ] The atlas build (at app build time)
- [ ] Task: The Metal UI pass
    - [ ] Write tests: SDF rounded rectangles, tabs with a flat end, dividers, strokes; content bitmaps (the logo, mission pictures, emblems) filtered up to their size
    - [ ] The pass, and the old widget rendering removed from the menus
- [ ] Task: The logo
    - [ ] Write tests: the logo bitmap is found in the UI pack and drawn on the main menu; the wordmark shows without it
- [ ] Task: Golden frames of every screen against the mockups; frame time under 1 ms at 4K
- [ ] Task: Conductor - User Manual Verification 'Phase 2: A UI system of our own' (Protocol in workflow.md)

## Phase 3: The launcher
- [ ] Task: The launcher's layout and backdrop
    - [ ] Write tests: XCTest of each section and its navigation; the window resizes, the rail keeps its width and the right side grows
    - [ ] SwiftUI in the design language; the backdrop (our own art, credited in AUTHORS.md); the logo from the UI pack, or the wordmark
- [ ] Task: The map library and profiles
    - [ ] Write tests: maps listed by kind (campaign, multiplayer, Custom Edition, misc) with their import status; a new game starts from a map; Import runs the importer; profiles are created and picked
- [ ] Task: Multiplayer: the server browser and the lobby
    - [ ] Write tests: servers listed and joined; a game hosted from the lobby
- [ ] Task: Settings, the version and updates
    - [ ] Write tests: every setting, grouped by section, saved to the engine's settings; the version and hash; the update check (without downloading)
- [ ] Task: Conductor - User Manual Verification 'Phase 3: The launcher' (Protocol in workflow.md)

## Phase 4: The loading screen in HD, completed
- [ ] Task: Original in HD
    - [ ] Write tests: the sweep, trail and timing match `progress_bar.c`'s constants at 640×480 and scale with the frame's height at 1080p, 1440p and 4K; the blur is in screen pixels
    - [ ] The setting: Original (the default) or OpenCE's
- [ ] Task: The cloud noise
    - [ ] Write tests: the noise built as progress_bar.c builds it, at the screen's size; its coordinates and blend in the pass
- [ ] Task: The loading picture
    - [ ] Our own hand-drawn picture at 4K, credited in AUTHORS.md, drawn where loading.tga was
- [ ] Task: Every campaign transition
    - [ ] Write tests: the screen plays at each mission's end before the next, and on loading a saved game
    - [ ] The timing case: a10 to a30 with no hitch over 33 ms; a golden frame of the screen
- [ ] Task: Conductor - User Manual Verification 'Phase 4: The loading screen in HD, completed' (Protocol in workflow.md)
