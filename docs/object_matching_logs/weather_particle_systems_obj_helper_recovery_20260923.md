# Weather particle physics helper recovery (2026-09-23)

Base `3c74fa36` on `jonas/exact-pilots`. This packet changes only
`source/effects/weather_particle_systems.c`; it does not alter a header,
compiler option, symbol map, park, semantic override, or object-admission row.

`_weather_particle_update_physics` is strict exact: January and rebuilt COFF
are 688 padded bytes, 29 relocations, and normalized SHA-256
`da0e0a2b59f80ca133dcc058df3e25c4891d6f166a7283b71990112311077733`.
The production meaningful-code gain is **681 bytes**, not 688. The other 19
functions in the translation unit remain 18 exact and one residual
(`_weather_particle_system_render`); no sibling is lost.

The recovered source uses the existing shared-header `point_from_line3d`
helper at two sites, rather than hand-expanding its three coordinate stores.
The first treats `particle->velocity` as a three-real point for an in-place
`velocity += acceleration * time_delta_sec`; the second updates position by
the seeded jitter vector scaled by `0.001f`. `real_vector3d` and
`real_point3d` each have three `real` components at offsets 0, 4, and 8.

## Independent first-party source-shape witness

The supplied later `/Od` `halo_cache_symbols.exe` contains the corresponding
weather update at VA `0x005816e0`: it calls the weather system/type accessors,
normalizes the acceleration, performs the point-physics update, and applies
seeded jitter. This is a later 2020 build, not the January target, so only
the two narrow call-site facts below are imported. The raw PE instructions
were decoded independently of decompiled C:

- At `0x0058191a..0x0058193d`, the program pushes `particle+0x10` as both
  input and output, `particle+0x1c` as the vector, and `system+8` as the
  scalar, then calls `0x00409ab1`. The offsets coincide with the January
  weather particle's velocity, acceleration, and time-delta operands.
- At `0x005819e1..0x00581a0e`, it passes `particle+4` as input/output,
  `0.001f` as the scalar, and the seeded direction as the vector to the
  same callee. This coincides with January's position-jitter sequence.
- `0x00409ab1` is a thunk to `0x0042e0d0`. That function reads three vector
  components, multiplies each by the scalar, adds the corresponding input
  point component, stores all three to the output, and returns the output
  pointer: the `point_from_line3d` contract.

Thus the point/vector view is attested at the exact use site, not invented to
steer the compiler. Removing both C casts in a memory-only gate probe leaves
the function strict exact, proving they are byte-inert typing. The declaration
layouts are compatible. This meets the per-site Lane B representation-view
exception; the casts are not a general precedent for unproven sites.

The genuine shared-header helper causes this TU to emit a 48-byte
`_point_from_line3d` COMDAT. It has zero relocations and normalized SHA
`9b763841f8519177`, byte-identical to January's selected copy in
`action_charge.obj`. The selected-provider link succeeds with the two objects
in either order. This is the previously approved folded-inline COMDAT class,
not a hand-written helper body. It is credited only because its caller is
strict exact and the full sweep is regression-free.

## Verification

- Full build and production progress: Halo exact meaningful code
  1,561,123 to 1,561,804; strict functions +1.
- Stable 8,245-function comparison: +1 exact, 0 regressions.
- Park validation: 100 active, 0 stale, 0 invalid; admission audit:
  0 contradicted and 0 revoked.
- Fake-match scan: 0 leads; `git diff --check` clean; 1,154 pytest tests pass,
  5 skipped, 26 subtests pass. The source file remains CRLF.

`_weather_particle_system_render` remains residual at zero credit. Earlier
evidence in `weather_particle_systems_obj_opus5_150k_w3_20260914.md` places
its remaining differences in five x87 operand/load-order rows; this packet
does not perturb it or claim the object fully matching.
