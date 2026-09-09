# Claude/Fable/Opus reconciliation checkpoint (2026-09-08)

## Accounting boundary

This ledger records the current uncommitted AI, encounter, biped, geometry,
collision, physics, and saved-game packet outside
`claude_remaining_donor_batch_20260908.md`. A fresh isolated gate was run for
every unit named below. Every gate passed the forbidden-emitted-symbol check
for `_point_from_line3d`.

Only strict COFF-equal functions receive exact credit. Every residual described
as fuzzy in this ledger receives **zero exact functions and zero exact code
bytes**, regardless of matching size or relocation count. No fuzzy percentage
is inferred.

The core AI/math/physics contribution below is 9 strict-exact functions and
3,386 meaningful / 3,440 padded code bytes:

| Unit | Function | Meaningful | Padded |
| --- | --- | ---: | ---: |
| `encounters.obj` | `_encounter_determine_pursuit_availability` | 635 | 640 |
| `geometry.obj` | `_convex_hull2d_intersect` | 527 | 528 |
| `geometry.obj` | `_convex_hull3d_test_vector` | 270 | 272 |
| `geometry.obj` | `_convex_polygon3d_verify` | 387 | 400 |
| `geometry.obj` | `_points_dimension2d` | 152 | 160 |
| `collision_bsp.obj` | `_collision_bsp_test_pill_new_recursive` | 672 | 672 |
| `collision_bsp.obj` | `_collision_leaf_test_vector` | 390 | 400 |
| `physics.obj` | `_friction_evaluate` | 247 | 256 |
| `physics.obj` | `_get_material_type` | 106 | 112 |
| **Total** | **9 functions** | **3,386** | **3,440** |

The four geometry functions move that unit from 14 to 18 exact, the two
collision functions move `collision_bsp.obj` from 21 to 23 exact, and the two
physics functions move `physics.obj` from 6 to 8 exact. The math/physics subset
therefore contributes 2,751 meaningful / 2,800 padded bytes. The corrected
`short q_count` declaration in `geometry.h` was checked across all six source
files that include that header without losing an inherited exact function.

The current complete per-unit gates are:

| Unit | Exact | Residual | Unwritten |
| --- | ---: | ---: | ---: |
| `actor_perception.obj` | 32 | 6 | 6 |
| `ai_communication.obj` | 37 | 2 | 9 |
| `encounters.obj` | 47 | 9 | 5 |
| `bipeds.obj` | 26 | 5 | 20 |
| `geometry.obj` | 18 | 6 | 6 |
| `collision_bsp.obj` | 23 | 7 | 0 |
| `physics.obj` | 8 | 2 | 7 |

## Actor perception: three retained fuzzies

Three formerly unwritten functions now have coherent typed reconstructions
grounded in January control flow and the HCEA cross-build behavior. Their
current production measurements are:

| Function | Target / candidate padded | Target / candidate relocations | Verdict |
| --- | ---: | ---: | --- |
| `_actor_situation_combat_status_update` | 320 / 320 | 7 / 7 | fuzzy; the complete relocation records agree, normalized code does not |
| `_actor_perception_friend_prop_is_attacking` | 432 / 432 | 16 / 16 | fuzzy |
| `_actor_perception_aiming_vector_test_blockage` | 400 / 384 | 10 / 10 | fuzzy |

Their combined January scope is 1,152 padded bytes. All 32 inherited exact
functions remain exact. These bodies use named actor/prop fields and typed
owners; they do not use raw object casts, byte offsets, address-derived names,
or synthetic compiler anchors. They are retained as the best current semantic
reconstructions but contribute zero exact credit.

## AI communication: player-rating fuzzy

`_ai_communication_get_player_rating` is now a complete, credible
reconstruction at 832 / 832 padded bytes and 49 / 49 relocations. The remaining
normalized-code difference is in floating-point evaluation and scheduling, not
missing high-level behavior. It remains a fuzzy function worth zero exact
credit. The unit retains its 37 exact functions.

## Encounters

`_encounter_determine_pursuit_availability` is strict exact at 635 meaningful /
640 padded bytes, with 23 relocations. Target and candidate normalized digests,
relocation records, sizes, and code all agree. Its typed pursuit logic is
supported by both January disassembly and the HCEA behavior; no raw layout or
compiler bait is involved.

The final `wait_after_pursuit` assignment intentionally tests the two output
pointer parameters rather than their pointed-to values. This looks like an
always-true bug because both known callers pass non-null addresses, but it is
not a coincidental match: January's x86 target explicitly tests both pointer
arguments, and the independently reconstructed HCEA PowerPC binary records the
same behavior. It is retained as an authenticated original-binary defect, not
as compiler steering or invented logic.

Three additional complete encounter bodies are retained as fuzzies:

| Function | Target / candidate padded | Target / candidate relocations | Verdict |
| --- | ---: | ---: | --- |
| `_encounter_compute_activation_cluster_bit_vector` | 1,200 / 1,232 | 46 / 46 | fuzzy; ordered relocation targets agree but offsets/code differ |
| `_encounter_control_actors` | 448 / 448 | 16 / 16 | fuzzy; ordered relocation targets agree but offsets/code differ |
| `_encounter_update_respawn` | 704 / 704 | 27 / 27 | fuzzy; ordered relocation targets agree but offsets/code differ |

The 448-byte `_encounter_control_actors` residual was deliberately parked
after one reconciliation pass. It already has complete, source-plausible logic,
the exact padded envelope, and the same ordered set of 16 relocation targets.
The remaining delta is compiler register/stack-load scheduling reflected by a
different normalized digest and shifted relocation offsets. Further source
perturbation would be low-leverage compiler steering rather than recovery of
missing semantics. Under the house rule, it receives zero exact credit and is
left for a later pass with new evidence. This is the 448-byte residual that
must not consume another extended tuning session.

## Bipeds

Three public biped functions now hold the best current typed implementations,
but remain production fuzzies:

| Function | Target / candidate padded | Target / candidate relocations | Verdict |
| --- | ---: | ---: | --- |
| `_biped_get_sight_position` | 416 / 400 | 19 / 19 | fuzzy |
| `_biped_get_autoaim_pill` | 336 / 336 | 9 / 9 | fuzzy |
| `_biped_accelerate` | 480 / 480 | 18 / 18 | fuzzy |

These three cover 1,232 padded target bytes and receive zero exact credit.
They use `biped_get`, `biped_definition_get`, named enum constants, `real`
types, and existing vector/flag helpers.

The retained private update cluster consists of `biped_update_dead`,
`biped_make_footstep`, `biped_update_landing`, `biped_update_slipping`, and
`biped_apply_rotation`. The genuine `_biped_update` caller remains unwritten,
so VC7 correctly eliminates this static, unrooted cluster in the production
object. The source bodies are preserved for future reconstruction, but they
are neither claimed as emitted functions nor given exact credit.

In a diagnostic build where it was temporarily rooted and aliased to its
January target, `biped_apply_rotation` reproduced 298 meaningful / 304 padded
bytes and 9 relocations exactly. The same-build symbol atlas supports the
private name. Nevertheless, the production gate still reports the target
section unwritten, so this diagnostic result receives zero production code
credit until a genuine `biped_update` caller naturally roots it. The other
private helpers remain best-known fuzzy source, also at zero credit.

The biped data recovery is independently exact: the complete section beginning
at `_rider_ejection`, including `rider_ejection = TRUE` and the named
`biped_update_section`, matches January at 1,536 bytes, one relocation, and the
same normalized digest. This is exact data, not exact code.

## Remaining math/physics fuzzies

The exact math/physics gains above coexist with three retained fuzzy bodies:

| Function | Target / candidate padded | Target / candidate relocations | Verdict |
| --- | ---: | ---: | --- |
| `_convex_hull2d` | 784 / 784 | 25 / 25 | fuzzy |
| `_compute_ground_plane` | 336 / 336 | 14 / 14 | fuzzy; ordered relocation targets agree but offsets/code differ |
| `_physics_compute_new` | 2,944 / 3,120 | 49 / 51 | fuzzy |

Their combined January scope is 4,064 padded bytes. They remain useful,
source-plausible reconstruction progress, but contribute zero exact credit.

Across all production fuzzies explicitly measured in this checkpoint, the
combined January target scope is 9,632 padded bytes. That number describes
documented reconstruction coverage only; it is not added to any exact-byte or
exact-function total.

## Saved-game closeout and deliberate exact debit

Six source-plausible saved-game functions are newly strict exact, contributing
3,675 meaningful / 3,728 padded bytes:

| Function | Meaningful | Padded |
| --- | ---: | ---: |
| `_saved_game_perform_file_system_checks` | 206 | 208 |
| `_saved_game_file_find_profile_index_for_directory_path` | 342 | 352 |
| `_synchronize_metadata_display_name_with_profile_name` | 1,032 | 1,040 |
| `_saved_game_files_delete_all_custom_profiles` | 216 | 224 |
| `_enumerate_memory_units` | 982 | 992 |
| `_create_enumerated_saved_game_file` | 897 | 912 |
| **Total** | **3,675** | **3,728** |

The provisional donor also made
`_saved_game_files_enumerate_available_to_local_player_index` strict exact by
moving `number_of_available_profiles = 0` inside successful outer mutex
acquisition. Its unconditional final output store could then read an
uninitialized local when mutex acquisition failed. That undefined,
nonsensical path is rejected under the house rules even though it reproduced
January's bytes. Canonical initializes the count before acquisition and parks
the resulting 368-byte residual at zero exact credit. This is an intentional
359-meaningful / 368-padded debit from the provisional sweep, not a hidden
regression.
