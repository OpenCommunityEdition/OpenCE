# Plan: Complete physics

## Phase 1: Objects as bodies [checkpoint: e0a7fc61]
- [x] Task: The mass-point model for objects f9b867a9
    - [x] Write tests: mass points, density, inertia, buoyancy and drag from `phys`; a crate falling and resting against a reference trace (marked, needs `assets/maps`)
    - [x] Bodies for objects in `opence_physics` and `sim`
- [x] Task: Collision and rest 33a8b9e3
    - [x] Write tests: friction by material; sleeping and waking; impact effects chosen by material
    - [x] Collision with the BSP and other objects' collision models
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Objects as bodies' (Protocol in workflow.md) (deferred to the end)

## Phase 2: Forces and grenades [checkpoint: 6d8b8a8a]
- [x] Task: Forces 0928583d
    - [x] Write tests: explosion force by distance; impacts; vehicles and players pushing objects; bipeds thrown before ragdolls
    - [x] Forces from damage, impacts and contacts
- [x] Task: Grenades 185f6293
    - [x] Write tests: the frag's bounces and the plasma's sticking against the reference traces
    - [x] Grenade projectile physics
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Forces and grenades' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Vehicles and devices [checkpoint: f7fe247f]
- [x] Task: Vehicles 7affe693
    - [x] Write tests: each manoeuvre of the spec against the reference traces (by property: no reference traces exist)
    - [x] The fixes to the vehicle model; entering from where the player stands; flipping back over
- [x] Task: Devices carrying objects 1e150faf
    - [x] Write tests: a lift carries a crate and a player; a door pushes a body
    - [x] Moving device collision
- [x] Task: Determinism and the station 2fc9330d
    - [x] Write tests: two simulations with the same inputs agree; `station physics` is stable; 500 bodies in under 2 ms
    - [x] The particle collision hook; `docs/engine.md`: physics
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Vehicles and devices' (Protocol in workflow.md) (deferred to the end)

## Phase: Review Fixes
- [x] Task: Apply review suggestions 834b7a92
