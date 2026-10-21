# Plan: The complete particle system

## Phase 1: Particles and their physics
- [ ] Task: Particle tags in full
    - [ ] Write tests: every field of `part` and `pphy` imports; colour and size over life
    - [ ] The typed tags
- [ ] Task: Particle physics
    - [ ] Write tests: gravity, drag and wind; bounce, slide, stop and die on collision; effects on collision
    - [ ] The update on the CPU and on the GPU, with the collision queries
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Particles and their physics' (Protocol in workflow.md) (deferred to the end)

## Phase 2: Systems, effects and contrails
- [ ] Task: Particle systems
    - [ ] Write tests: emitters, rates and their functions, states and transitions, attachment to markers
    - [ ] `pctl` in full
- [ ] Task: Effects' particle parts
    - [ ] Write tests: counts, cones, inherited velocity, scaled-by functions
    - [ ] The effect parts in full
- [ ] Task: Contrails
    - [ ] Write tests: point lifetimes, shapes, scroll, colours over life
    - [ ] `cont` in full
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
