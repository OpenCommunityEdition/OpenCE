# Plan: The Halo UI, the launcher and the Xbox loading screen

## Phase 1: Importing the UI [checkpoint: d1c810f4]
- [x] Task: The UI pack b30af7b9
    - [x] Write tests: `ui.map` imports; widget definitions, children, handlers, string lists, fonts and the UI scenario are read; bad references are refused; a fuzz target for the UI pack
    - [x] The importer's UI path and the typed UI content
- [x] Task: Fonts d6bb0b83
    - [x] Write tests: each glyph's metrics and kerning; the distance field matches the glyph's coverage
    - [x] Font tags with signed distance fields built at import
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Importing the UI' (Protocol in workflow.md) (deferred to the end)

## Phase 2: The widget system [checkpoint: 952b38d0]
- [x] Task: The widget tree 882708ba
    - [x] Write tests: navigation and focus links; handlers fire their functions; conditional widgets; list items from data; spinners; formatted text
    - [x] `opence_ui` crate: the tree, the handlers, the UI functions wired to the engine, the unwired list
- [x] Task: Drawing the UI 50f43aa3
    - [x] Write tests: the 640×480 layout scaled to the window; the UI pass's frame time
    - [x] The UI pass, the background scene with the new lighting, widget animation
- [x] Task: Keyboard and mouse d72f23da
    - [x] Write tests: keys to the pad's buttons; hover focus, click, wheel and right-click by the widgets' bounds
    - [x] Input to the widget tree; UI sounds
- [ ] Task: Conductor - User Manual Verification 'Phase 2: The widget system' (Protocol in workflow.md) (deferred to the end)

## Phase 3: The screens and the launcher [checkpoint: f570a440]
- [x] Task: Every screen reached 71f38fa5
    - [x] Write tests: a scripted walk through every screen, headless (marked, needs `assets/maps`)
    - [x] Campaign, multiplayer, settings, profiles, pause and post-game screens wired; the engine's own menus removed
- [x] Task: The launcher d2346075
    - [x] Write tests: XCTest of the launcher's three choices and its settings
    - [x] Play, Settings and Exit only
- [ ] Task: Conductor - User Manual Verification 'Phase 3: The screens and the launcher' (Protocol in workflow.md) (deferred to the end)

## Phase 4: The loading screen [checkpoint: b6a1f1f9]
- [x] Task: The chant 85cf1883
    - [x] Write tests: the tone is found in an XBE by its section and decoded; without an XBE nothing plays
    - [x] Reading and decoding the compressed tone from the user's `default.xbe`, into the cache only
- [x] Task: The screen 72ee6af2
    - [x] Write tests: the panning light's timing and the fade against the behaviour of `progress_bar.c`; skipping only once the map is ready
    - [x] The loading screen pass; background loading and import of the next map; the fade into the next mission; `ui.loading_screen`
- [x] Task: Transitions eaee433f
    - [x] Write tests: no frame over 33 ms from the end of a10 to the first frame of a30 (marked, needs `assets/maps`)
    - [x] `docs/engine.md`: the UI, the launcher and the loading screen; golden frames of the screens
- [ ] Task: Conductor - User Manual Verification 'Phase 4: The loading screen' (Protocol in workflow.md) (deferred to the end)
