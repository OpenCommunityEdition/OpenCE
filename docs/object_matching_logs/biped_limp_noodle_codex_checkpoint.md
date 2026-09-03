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
| `_biped_limp_noodle_valid_joint_rotation` | 1,244 | 1,248 | 1,216 | fuzzy |
| `_biped_limp_noodle_move_relax_and_constrain_positions` | 1,955 | 1,968 | 1,888 | fuzzy |
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

## Collision-interface ownership reconciliation

January proves that calls to `collision_test_sphere` pass a third
`ignore_object_index` argument, even though this particular x86 callee does
not read it. It also proves the seven-argument
`collision_get_features_in_sphere` and `collision_move_point` interfaces.
Those collision-feature APIs now live in `collision_features.h`, beside the
feature-list and collision-plane types they consume. The ad-hoc declarations
were removed from `biped_limp_noodle.c`, `game_engine.c`, and
`collisions.c`; every biped sphere query now passes `biped_index`.

The pre-existing `collision_move_pill` and `collision_move_sphere` declaration
tails were also corrected from an incompatible collision-result/count pair to
the January-confirmed maximum-plane-count and `collision_plane` array. Their
declarations remain in their original slots in `collisions.h`; moving those
slots changes C2 scheduling in an otherwise unrelated exact Decals function.
The feature-list APIs remain in the associated `collision_features.h` owner
section. A direct C2 compile of `collisions.c` is warning-free with these
interfaces.

This placement is also C2 schedule-safe. All four direct
`collision_features.h` consumers and all fourteen direct `collisions.h`
consumers were compiled and gated. No exact function changed: Units remains
189/189, Decals retains its exact
`_decals_delete_permanent_from_cluster`, and the Collisions, Game Engine,
Collision Features, and Biped exact sets are preserved. The corrected third
argument improves the two large fuzzy candidates by 16 padded bytes each.

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
- `_collision_move_pill`, `_collision_move_sphere`, and
  `_collision_test_sphere` remain exact after the signature reconciliation.
- `biped_limp_noodle.obj`: three exact, three residual, zero unwritten;
  corrected-interface candidates are 1,216, 1,888, and 240 padded bytes.
- All four `collision_features.h` and fourteen `collisions.h` direct
  consumers were compiled and gated during the final blast-radius check.
- Tool suite: 261/261 tests passed.
- `git diff --check`: passed.
- Candidate symbol audit: no `point_from_line3d` definition or COMDAT.

Source SHA-256:
`75dd79b1a99b59bea13bc6da390db6037828f4b4d4ec57ea722e3a1b3ba623db`.
January target object SHA-256:
`113dfab879513774d369627a6bd7a7607baf9bb7bcf82c87ee1bc28b6aa32b69`.

No assembly, volatile scheduling device, forced inline, optimizer pragma,
undefined representation punning, fake source, or nonsensical byte forcing is
present. No global configuration, frozen-object path, semantic ledger, or
parked-function policy was changed.
