# Plan: The complete animation set

## Phase 1: The graph and the frames [checkpoint: fba329d3]
- [x] Task: The full graph f8a0b81d
    - [x] Write tests: every block of `antr` imports; inheritance; the fallbacks for a missing animation; frame events
    - [x] The typed graph in `content` and its import
- [x] Task: The blender 0d718fe9
    - [x] Write tests: base, overlay and replacement; the transition blend; aim limits; against the reference traces (marked, needs `assets/maps`)
    - [x] The pose blender in `sim`, with the overlays in Halo's order
- [ ] Task: Conductor - User Manual Verification 'Phase 1: The graph and the frames' (Protocol in workflow.md) (deferred to the end)

## Phase 2: Bipeds, weapons and the first person [checkpoint: 89c991f7]
- [x] Task: Biped states e327e6b2
    - [x] Write tests: each state of the spec's list chosen from the unit's state, against the reference traces
    - [x] The biped animation state machine
- [x] Task: Weapons and the first person b46112c9
    - [x] Write tests: each weapon state and the first-person states against the reference traces
    - [x] Weapon animations and the first-person arms
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Bipeds, weapons and the first person' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Vehicles, AI, devices and cutscenes [checkpoint: 929ad889]
- [x] Task: Vehicles and seats 9f50ea51
    - [x] Write tests: entering, leaving, boarding and ejection by seat; the vehicles' own animations
    - [x] Seat animations and vehicle animations
- [x] Task: AI 6edd6b85
    - [x] Write tests: each behaviour's animation request, by actor type
    - [x] AI animation requests and overlays
- [x] Task: Devices and cutscenes 6edd6b85
    - [x] Write tests: devices by their functions; custom animations relative to the cutscene flags; the camera's tracks
    - [x] Device animation; cutscene animation and camera
- [x] Task: The animation station and the documentation a39b3a10
    - [x] Write tests: `station animation` plays every animation of a graph
    - [x] `docs/engine.md`: the graph and the blender
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Vehicles, AI, devices and cutscenes' (Protocol in workflow.md) (deferred to the end)

## Phase: Review Fixes
- [x] Task: Apply review suggestions 700c2086
