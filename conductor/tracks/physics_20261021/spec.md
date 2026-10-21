# Spec: Complete physics

## Overview
The engine has the player's movement over Jolt's character, Halo's mass-point vehicle driving and ragdolls. Other objects do not move: crates, barrels, debris, dropped weapons and grenades do not tumble, and vehicles do not feel like the original. This track gives every object the physics Halo gives it, so that objects, bodies and particles move with the world as they do in the original.

## Functional Requirements
1. **Rigid bodies for objects:** each object with a physics tag (`phys`) is a body moved by Halo's mass-point model: its mass points, their densities and friction, the centre of mass and inertia, buoyancy in water and in the fog planes Halo treats as water, and air and water drag. Objects without a physics tag move as Halo's do: falling items, dropped weapons and equipment slide and come to rest.
2. **Collision:** bodies collide with the BSP and with each other's collision models (`coll`), with the materials' friction and the impact effects and sounds by material.
3. **Rest:** objects go to sleep at rest and wake when touched, as Halo's do.
4. **Forces:** explosions (`jpt!`'s force and radius), projectile impacts, vehicles hitting objects, melee and the players pushing objects. Bipeds are pushed by explosions and thrown when killed, before their ragdolls take over.
5. **Grenades:** the frag's bounce and roll and the plasma's stick to surfaces and units, from the projectile tags.
6. **Vehicles:** the full mass-point model of every stock vehicle checked against the reference traces in more manoeuvres: driving over bumps, flipping and righting, the Warthog's jumps, the Ghost's boost, the Banshee's flight and flip, the Scorpion's treads; entering a vehicle from where the player stands; flipping a vehicle back over by entering it.
7. **Devices:** lifts and doors carry and push what is on them.
8. **Particle physics hook:** particles that ask for collision collide with the BSP using the same collision queries (the particle track uses this).
9. **Determinism:** the simulation's physics stays deterministic across machines for multiplayer: the same inputs give the same results.

## Non-Functional Requirements
- 500 active bodies in a tick cost under 2 ms on the M1 Max.
- The physics station of the test map (a pile of crates, a slope and explosions) is stable: no jitter at rest, no tunnelling.

## Acceptance Criteria
- The traces of objects knocked by grenades in a set of scenes match the reference game's within the tolerances the movement traces use.
- The vehicle traces match the reference game's in every listed manoeuvre.
- Two machines in a multiplayer test agree on every body's state after a minute of play.

## Out of Scope
- Physics Halo does not have (cloth, destruction of the level).
