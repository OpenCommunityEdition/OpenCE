# `biped_limp_noodle.obj` reconstruction checkpoint

This ledger records the reconstruction wave based on canonical commit
`c5ce04c77cfbd0b8f603385f2b5396ef6f918240`. January's split object,
disassembly, relocation records, and CodeView evidence are the byte authority.
The open HCEA reconstructions in
`work/research-cache-halocea/src/blam/units/biped_limp_noodle_*.c` supply the
semantic control flow and names.

The production compiler is the repository XDK 3911 toolchain,
`xbox/bin/vc7/CL.Exe` version `13.00.9254.1` (SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`).
This unit uses `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` plus the repository include
path list recorded in `build.ninja`.

## Validated baseline

Before this wave, two of six functions were strict exact, accounting for 512
of 4,688 padded code bytes. Four functions were unwritten. After the wave,
three of six functions and 1,216 padded code bytes are strict exact; all six
functions are written. Both owned data sections remain exact: 44,808 bytes of
`.bss` and 128 bytes of `.rdata`, or 44,936/44,936 data bytes.

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

The three residual measurements below come from the retained source at
canonical commit `8b02b172d`. Sizes are padded COFF section sizes. The first
divergences are measured facts from relocation-normalized disassembly, not an
assertion that the named compiler mechanism is the only possible cause.

| Function | Meaningful T/B | Padded T/B | Relocs T/B | Target normalized SHA-256 | Base normalized SHA-256 | First meaningful divergence |
| --- | ---: | ---: | ---: | --- | --- | --- |
| `_biped_limp_noodle_valid_joint_rotation` | `1244/1206` | `1248/1216` | `34/34` | `699263a33b8b8131dfa843828894caa194dff415596b92545c5e5f96cd666a14` | `8306eb3e4e81348ff9a5a2f823d70735cfdd1984ebd813dfea954dca5434949b` | `+0x03`: target reserves `0xE4` stack bytes and candidate reserves `0xCC`; the following saved-register and local-slot allocation also differs. |
| `_biped_limp_noodle_move_relax_and_constrain_positions` | `1955/1887` | `1968/1888` | `66/66` | `5964980a1f042d1c452e559fcba37592d608e1798ae0b7b5cef8deb9e40961d8` | `34b872cd2ae1a72136400424c983c399b7a2f8fbc7a3f342d9ed968e90dd19b3` | `+0xE1`: after an otherwise identical prologue and setup, January homes the integer conversion temporary at `[ebp-0x20]`; the candidate selects `[ebp-0x1C]`, followed by a different local-slot schedule. |
| `_biped_limp_noodle_relax_nodes_onto_environment` | `244/240` | `256/240` | `9/9` | `19128c623c5575500437c95cdfd0363c69fcbfdbf57f7972bbdcf1d2dffaf84f` | `d1b5db56b26dff8d5d6bafeeae5e1c0e202e19464ab0bf3dec48bea5e1e7ce18` | `+0x1E`: January calls `tag_get` immediately; the candidate first spills the biped pointer to `[ebp-0x10]`, changing later register and loop scheduling. |

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

## Accepted controls

- The retained solver follows independently recovered HCEA control flow while
  using January's x86 parameter counts, ten-plane scratch array, assertions,
  source lines, and collision-user stack behavior.
- Private functions and `biped_limp_noodle_globals` use semantic names backed
  by the HCEA source family and the recovered symbol map; no address name is
  retained for reconstructed code or data.
- Typed biped, model, animation-graph, tag-block, and collision access uses the
  owning subsystem types and macros.
- Collision declarations live in the associated owner headers, with the
  declaration positions chosen by the verified January C2 schedule.

## Experiment matrix

The original reconstruction worktree preserves the E01 object and therefore
its normalized hashes, but not a complete aligned first-divergence record for
every function. Missing historical fields are stated as unavailable rather
than reconstructed after the fact.

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
| --- | --- | ---: | ---: | --- | --- | --- | --- |
| E01 | Natural HCEA-derived reconstruction with the then-visible two-argument sphere declaration | valid `1248/1200`; move `1968/1872`; wrapper `256/240`; adjust `704/704` | valid `34/34`; move `66/66`; wrapper `9/9`; adjust `23/23` | valid `516bc9681568037b15a4bc607b0136afcba3fc457ded5db9a1f52c556ae42d1e`; move `be3740439796de601b71bd3883963ad2bd25a267548ee1fa6ded81a42ac16e15`; wrapper `d1b5db56b26dff8d5d6bafeeae5e1c0e202e19464ab0bf3dec48bea5e1e7ce18`; adjust `99ecd6da29a4b0ed091ec1c7e14c2f8f7cdf6ce21bbb25449f70b8da934c30ae` | broad local/register schedule; exact adjust has none; detailed historical alignment unavailable | Unit gained adjust; no reported loss | Retained provisionally, then corrected by E02 |
| E02 | Restore January's third `ignore_object_index` sphere argument and pass `biped_index` at all five call sites | valid `1248/1216`; move `1968/1888`; wrapper `256/240`; adjust `704/704` | `34/34`; `66/66`; `9/9`; exact | hashes recorded in the residual table | residual divergences recorded above | All 17 affected units preserved; Units `189/189` | Retained |
| E03 | Put feature-list APIs in `collision_features.h`; remove ad-hoc consumer-local declarations | unchanged | unchanged | unchanged | unchanged | Collision Features `20/20`; Game Engine `169/180`; Collisions `8/20`; no loss | Retained |
| E04 | Move `collision_move_pill` and `collision_move_sphere` declarations away from their original `collisions.h` slots | unchanged in biped | unchanged | unavailable | unrelated Decals load order changed | `_decals_delete_permanent_from_cluster` lost exactness | Reverted; original declaration slots retained |
| E05 | Evaluate the hinge `arccosine(constraint_cosine)` result only for its discarded side-effect shape | valid `1248/1216` | `34/34` | unchanged | unchanged | no loss | Reverted as codegen-neutral and unnecessary |

## Do not repeat

- Do not restore two-argument `collision_test_sphere` declarations: January
  call sites prove three arguments even though this callee does not read the
  third one.
- Do not move the pill/sphere prototype slots: that regresses exact Decals
  scheduling through C2 definition-position sensitivity.
- Do not place collision externs in consuming C files.
- Do not reintroduce the HCEA twelve-plane PPC scratch count; January uses ten.
- Do not force the remaining stack/register schedules with volatile, register
  hints, barriers, raw offsets, representation punning, assembly, pragmas, or
  invented control flow.
- Do not add `point_from_line3d` calls that emit a candidate-only COMDAT.

## Residual classification and reopen criteria

The fail-closed Capstone classifier returns `UNKNOWN` with no confidence for
all three residuals, so all are recorded as `unclassified`. The observations
below are bounded inferences, not proven blocker classes:

- `valid_joint_rotation`: the differing frame size and early allocation are
  measured; a source/topology or ABI/register-lifetime cause is plausible but
  unresolved.
- `move_relax_and_constrain_positions`: the long identical prefix followed by
  a local-slot split suggests a lifetime or scheduling difference. Equal
  relocation counts rule out a simple missing-call explanation but do not
  prove a compiler mechanism.
- `relax_nodes_onto_environment`: January and the candidate measurably disagree
  on when the biped pointer is homed; a register/local-lifetime cause is only
  an inference.

Reopen these residuals only for authoritative January source or local-variable
records, a natural same-compiler donor that explains the observed lifetime,
or a source-authentic correction to recovered semantics. A compiler-control
trick or a coincidental exact spelling is not new evidence.

## Disposition

`_biped_limp_noodle_adjust_orientations` is strict exact. The other three new
functions remain honest fuzzy reconstructions and are not counted as exact.
They are methodologically parked by this ledger pending the reopen evidence
above; no strict board or exact-byte credit is granted for them.

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

Retained source Git blob SHA-1:
`b398d2eee7d533d9447548a2d9128f36cc559347`.
Its canonical LF payload SHA-256 is
`2eed22c071ecc0965840d7f665b1446ff75e5f02f6cf60ba4b9b5b27fb62a667`.
January target object SHA-256:
`113dfab879513774d369627a6bd7a7607baf9bb7bcf82c87ee1bc28b6aa32b69`.

No assembly, volatile scheduling device, forced inline, optimizer pragma,
undefined representation punning, fake source, or nonsensical byte forcing is
present. No object status label, frozen-object path, comparator, or compiler
configuration was changed by this admission.
