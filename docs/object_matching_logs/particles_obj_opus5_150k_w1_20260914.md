# `particles.obj` — opus5 150K house-clean lane, wave w1 (2026-09-14)

## Result

No change to `source/effects/particles.c`. Final gate equals baseline: 18 exact / 2 residual / 0 unwritten,
guard passes, park `_particle_next_frame` drift 0.

## `_particle_update_physics` (928/35 target, 944/36 ours) — not landed

January facts recovered from the aligned disassembly:

1. The final speed test is `fcomp 0.0625; test ah,5; jp <direction copy>`: source
   `magnitude_squared3d(velocity) < 0.0625f` with the settled/dies-at-rest block as the THEN arm and
   `particle->direction = *velocity` as the ELSE arm (ours `>=` gives `test ah,1; jne`).
2. In the attached-object branch January stores the scaled j/k velocity through `[particle+0x4c/0x50]` and
   forwards all three scaled components into the position update: direct
   `particle->translational_velocity.<c>` field spelling, with `velocity = &particle->translational_velocity`
   assigned afterwards.
3. January shares one `particle_die` block (reloads EBX from `[ebp+8]`) between the air/water death test and the
   dies-at-rest test — hence 35 relocations; ours emits two.
4. January keeps `particle_index` in memory (EBX only at entry/else branch), caches `definition` in ESI, and
   lays the attached (else) branch before the common tail; ours pins `particle_index` in ESI and places the else
   branch after the tail with a jump back.

Shapes: `scale_vector3d` inline — compiler-identical; `scale_vector3d` + `point_from_line3d` (scratch only; the
`_point_from_line3d` guard fails, inadmissible) — also compiler-identical, so this residual is not gated on the
point_from_line3d policy; (1) — compare pattern matches, 960 bytes; (1)+(2) — the velocity/position block is
instruction-identical to January, still 960/36 because (3)/(4) remain. Best scratch:
`scratch/workers/player_effects_particles_contrails/p_pu2.c`.

Reopen criterion: a source-level cause for the shared die block/particle_index home (e.g. a recovered structure
of the death tests), starting from the (1)+(2) body.

## `_particle_next_frame` (parked) — not re-spent

The park already records 12 shapes. January pins TRUE in AL and reuses it as the bit-0 mask
(`mov cl,[esi+2]; mov al,1; test al,cl`); none of this lane's measured laws addresses that register choice.
