# Plan: The complete particle system

## Phase 1: Particles and their physics [checkpoint: 7a7810a8]
- [x] Task: Particle tags in full 9bbfbb5c
    - [x] Write tests: every field of `part` and `pphy` imports; colour and size over life
    - [x] The typed tags
- [x] Task: Particle physics 6ed8e519
    - [x] Write tests: gravity, drag and wind; bounce, slide, stop and die on collision; effects on collision
    - [x] The update on the CPU, with the collision queries (no GPU update: the CPU's is the original's, per particle, and fits the budget)
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Particles and their physics' (Protocol in workflow.md) (deferred to the end)

## Phase 2: Systems, effects and contrails [checkpoint: c7e2de33]
- [x] Task: Particle systems 36abd8c0
    - [x] Write tests: emitters, rates and their functions, states and transitions, attachment to markers
    - [x] `pctl` in full
- [x] Task: Effects' particle parts 36abd8c0
    - [x] Write tests: counts, cones, inherited velocity, scaled-by functions
    - [x] The effect parts in full
- [x] Task: Contrails a3649cde
    - [x] Write tests: point lifetimes, shapes, scroll, colours over life
    - [x] `cont` in full
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Systems, effects and contrails' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Weather, lighting and limits [checkpoint: a40b225e]
- [x] Task: Weather and wind 61fe864e
    - [x] Write tests: density by the weather palette; wind by the wind palette; fade near the camera
    - [x] `rain` and `wind` in full
- [x] Task: Lighting and limits d6cdc29e
    - [x] Write tests: lit particles take probes and lights; recycling at the limit; the budget
    - [x] Particle lighting, emitted lights, limits and the setting
- [x] Task: The particle station and the documentation a1b009d2
    - [x] `station particles`; `docs/engine.md`: particles
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Weather, lighting and limits' (Protocol in workflow.md) (deferred to the end)

## Phase: Review Fixes
- [x] Task: Apply review suggestions 3d3d7073
