# Plan: The complete animation set

## Phase 1: The graph and the frames
- [ ] Task: The full graph
    - [ ] Write tests: every block of `antr` imports; inheritance; the fallbacks for a missing animation; frame events
    - [ ] The typed graph in `content` and its import
- [ ] Task: The blender
    - [ ] Write tests: base, overlay and replacement; the transition blend; aim limits; against the reference traces (marked, needs `assets/maps`)
    - [ ] The pose blender in `sim`, with the overlays in Halo's order
- [ ] Task: Conductor - User Manual Verification 'Phase 1: The graph and the frames' (Protocol in workflow.md) (deferred to the end)

## Phase 2: Bipeds, weapons and the first person
- [ ] Task: Biped states
    - [ ] Write tests: each state of the spec's list chosen from the unit's state, against the reference traces
    - [ ] The biped animation state machine
- [ ] Task: Weapons and the first person
    - [ ] Write tests: each weapon state and the first-person states against the reference traces
    - [ ] Weapon animations and the first-person arms
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Bipeds, weapons and the first person' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Vehicles, AI, devices and cutscenes
- [ ] Task: Vehicles and seats
    - [ ] Write tests: entering, leaving, boarding and ejection by seat; the vehicles' own animations
    - [ ] Seat animations and vehicle animations
- [ ] Task: AI
    - [ ] Write tests: each behaviour's animation request, by actor type
    - [ ] AI animation requests and overlays
- [ ] Task: Devices and cutscenes
    - [ ] Write tests: devices by their functions; custom animations relative to the cutscene flags; the camera's tracks
    - [ ] Device animation; cutscene animation and camera
- [ ] Task: The animation station and the documentation
    - [ ] Write tests: `station animation` plays every animation of a graph
    - [ ] `docs/engine.md`: the graph and the blender
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Vehicles, AI, devices and cutscenes' (Protocol in workflow.md) (deferred to the end)
