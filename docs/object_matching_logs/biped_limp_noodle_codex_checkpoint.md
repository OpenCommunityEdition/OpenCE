# `biped_limp_noodle.obj` reconstruction checkpoint

This ledger records the reconstruction wave based on canonical commit
`c5ce04c77cfbd0b8f603385f2b5396ef6f918240`. January's split object,
disassembly, relocation records, and CodeView evidence are the byte authority.
The open HCEA reconstructions in
`work/research-cache-halocea/src/blam/units/biped_limp_noodle_*.c` supply the
semantic control flow and names.

## Result

All six January functions now emit honest C. The four functions that were
previously absent account for 4,138 meaningful target bytes and 4,176 padded
target bytes. `_biped_limp_noodle_adjust_orientations` is strictly exact,
adding 695 meaningful / 704 padded exact bytes. The other three new functions
are retained as reviewable fuzzy reconstructions after one natural exact pass.

| Function | Target meaningful | Target padded | Candidate padded | Verdict |
| --- | ---: | ---: | ---: | --- |
| `_biped_limp_noodle_get_max_relaxation_iterations` | 6 | 16 | 16 | exact |
| `_biped_limp_noodle_valid_joint_rotation` | 1,244 | 1,248 | 1,200 | fuzzy |
| `_biped_limp_noodle_move_relax_and_constrain_positions` | 1,955 | 1,968 | 1,872 | fuzzy |
| `_validate_real_vector3d_axes3` | 492 | 496 | 496 | exact |
| `_biped_limp_noodle_adjust_orientations` | 695 | 704 | 704 | exact |
| `_biped_limp_noodle_relax_nodes_onto_environment` | 244 | 256 | 240 | fuzzy |

The reconstructed private identities are evidence-backed rather than address
names. The translation unit owns an authentic 44,808-byte
`biped_limp_noodle_globals` BSS layout: one collision feature list and 64 saved
node positions. Definitions and object access use subsystem typed macros and
project `real`/`boolean` types. The January assertion paths and source lines
are preserved.

The solver now performs the complete semantic sequence: validate joint motion,
collect collision features, relax node positions under gravity and collision
constraints, rebuild node orientations, and iterate until stable. It uses the
January-confirmed ten-plane collision scratch array rather than the twelve
planes present in the PPC-oriented HCEA reconstruction.

## Deliberate collision-interface disposition

January proves that calls to `collision_test_sphere` pass a third
`ignore_object_index` argument, and also proves that this particular x86
callee never reads it. Correcting that declaration in `collisions.h` caused a
real C2 definition-position regression in the otherwise exact
`decals::_decals_delete_permanent_from_cluster` (400 padded bytes). Adding the
two missing movement declarations to that header caused the same regression.

The public collision header and `collisions.c` were therefore restored byte
for byte to the canonical baseline. This TU keeps local declarations for the
two previously undeclared collision owners and uses the established two-arg
sphere declaration. That is a deliberate, semantics-preserving fuzzy
compromise; no exact function was sacrificed to improve a residual. The three
residual functions should be revisited when the collision header can be
reconstructed as a complete January scheduling unit.

Direct component arithmetic is used at the three line/point constructions for
which HCEA and January agree. This prevents emission of a
`point_from_line3d` COMDAT and preserves the required January inline schedule.

## Verification

- Full `halobetacache_build`: passed for both the base commit and this wave.
- Stable all-function diff: 8,245 functions inspected; one 704-byte gain and
  zero regressions (5,216 -> 5,217 exact).
- `units.obj`: 189 exact, zero residual, zero unwritten.
- `bipeds.obj`: unchanged at 25 exact, one inherited residual, 25 unwritten.
- `collisions.obj`: unchanged at eight exact, zero residual, 12 unwritten.
- `biped_limp_noodle.obj`: three exact, three residual, zero unwritten.
- All 17 `collisions.h` consumers were rebuilt during the blast-radius check.
- Tool suite: 261/261 tests passed.
- `git diff --check`: passed.
- Candidate symbol audit: no `point_from_line3d` definition or COMDAT.

Source SHA-256:
`b4540bb45a95408315ab24391dad0dbf12f4c1b00d5f2f7fd5375bd2f592792a`.
January target object SHA-256:
`113dfab879513774d369627a6bd7a7607baf9bb7bcf82c87ee1bc28b6aa32b69`.

No assembly, volatile scheduling device, forced inline, optimizer pragma,
undefined representation punning, fake source, or nonsensical byte forcing is
present. No global configuration, frozen-object path, semantic ledger, or
parked-function policy was changed.
