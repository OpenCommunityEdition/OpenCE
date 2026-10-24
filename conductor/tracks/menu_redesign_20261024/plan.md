# Plan: The menus, launcher and loading screen redesigned

## Phase 1: The design language and the design artifact
- [ ] Task: Reference captures of the original
    - [ ] Write tests: `opence capture-ui` walks every screen headless and writes PNGs to `assets/ui_reference/` (gitignored); a test that the folder is ignored
    - [ ] The capture command; the palette sampled from the captures into a local swatch file
- [ ] Task: The design reference
    - [ ] `docs/ui_design.md`: the palette, panel and frame shapes, bevels, spacing, the type scale (OpenCE and Overpass sizes matched to the original's), focus and selection, motion curves, sounds
- [ ] Task: Static mockups
    - [ ] A private Claude design artifact with every screen in the spec, drawn from the reference with our own art and a marked logo placeholder
    - [ ] Rounds of the user's feedback, recorded in the artifact and in `docs/ui_design.md`
- [ ] Task: The clickable prototype and motion studies
    - [ ] The screens wired into the real flow (focus, hover, back)
    - [ ] Motion studies of focus, transitions, scrolling and the loading sweep
- [ ] Task: Conductor - User Manual Verification 'Phase 1: The design language and the design artifact' (Protocol in workflow.md): the user approves the design, and everything after builds from it

## Phase 2: Drawing the menus in the new language
- [ ] Task: Fonts from the font files
    - [ ] Write tests: SDF atlases built from OpenCE and Overpass; metrics and kerning; layout widths for the same strings match the old layout within the design's tolerance
    - [ ] The atlas build (at app build time); text drawn from the atlases instead of the font tags
- [ ] Task: Vector panels and frames
    - [ ] Write tests: each widget type's shapes from the design (backgrounds, frames, highlights, arrows); content bitmaps kept
    - [ ] A vector UI pass in Metal: SDF rounded rectangles, bevels and strokes
- [ ] Task: The logo
    - [ ] Write tests: the logo bitmap is found in the UI pack and placed where the original placed it; the wordmark shows without it
- [ ] Task: Widescreen layout
    - [ ] Write tests: anchors, columns and stretching per screen at 4:3, 16:9, 16:10 and 21:9; hit boxes follow the layout
- [ ] Task: Colour and motion
    - [ ] Write tests: the palette is applied; focus, transitions and scrolling ease over time, independent of the frame rate
- [ ] Task: Golden frames of every screen against the mockups; frame time under 1 ms at 4K
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Drawing the menus in the new language' (Protocol in workflow.md)

## Phase 3: The launcher
- [ ] Task: The launcher's layout and backdrop
    - [ ] Write tests: XCTest of each section and its navigation
    - [ ] SwiftUI in the design language; the backdrop (our own art, credited in AUTHORS.md); the logo from the UI pack, or the wordmark
- [ ] Task: The map library and profiles
    - [ ] Write tests: maps listed with their import status; Import runs the importer; profiles are created and picked
- [ ] Task: Multiplayer settings, the version and updates
    - [ ] Write tests: the player name, colour, port and servers are saved to the engine's settings; the update check (without downloading)
- [ ] Task: Conductor - User Manual Verification 'Phase 3: The launcher' (Protocol in workflow.md)

## Phase 4: The loading screen completed
- [ ] Task: The cloud noise
    - [ ] Write tests: the noise texture built as progress_bar.c builds it; its coordinates and blend in the pass
- [ ] Task: The loading picture
    - [ ] Our own hand-drawn 320×240 picture, credited in AUTHORS.md, drawn where loading.tga was
- [ ] Task: Every campaign transition
    - [ ] Write tests: the screen plays at each mission's end before the next, and on loading a saved game
    - [ ] The timing case: a10 to a30 with no hitch over 33 ms; a golden frame of the screen
- [ ] Task: Conductor - User Manual Verification 'Phase 4: The loading screen completed' (Protocol in workflow.md)
