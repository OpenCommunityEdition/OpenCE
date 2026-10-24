# Plan: The complete particle system

## Phase 1: Particles and their physics [checkpoint: 7a7810a8]
- [x] Task: Particle tags in full 9bbfbb5c
    - [x] Write tests: every field of `part` and `pphy` imports; colour and size over life
    - [x] The typed tags
- [x] Task: Particle physics 6ed8e519
    - [x] Write tests: gravity, drag and wind; bounce, slide, stop and die on collision; effects on collision
    - [x] The update on the CPU, with the collision queries (no GPU update: the CPU's is the original's, per particle, and fits the budget)
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Particles and their physics' (Protocol in workflow.md) (deferred to the end)

## Phase 2: Systems, effects and contrails
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

## Phase 3: Weather, lighting and limits
- [ ] Task: Weather and wind
    - [ ] Write tests: density by the weather palette; wind by the wind palette; fade near the camera
    - [ ] `rain` and `wind` in full
- [ ] Task: Lighting and limits
    - [ ] Write tests: lit particles take probes and lights; recycling at the limit; the budget
    - [ ] Particle lighting, emitted lights, limits and the setting
- [ ] Task: The particle station and the documentation
    - [ ] `station particles`; `docs/engine.md`: particles
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Weather, lighting and limits' (Protocol in workflow.md) (deferred to the end)
