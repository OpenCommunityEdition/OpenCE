# CollisionBSP and Damage owner reconciliation

This batch reconciles two coherent smaller-family packets from the completed
Fable continuation into canonical. The comparison base is published commit
`25c45ad20d49587d98640ca78c0c08014819bbf0`, at 938,227 meaningful exact
code bytes and 6,321 credited functions. Donor headline totals are not added:
all credit below is a fresh current-canonical owner comparison.

## Honest exact accounting

The accepted batch adds six strict functions and 2,305 meaningful / 2,352
padded code bytes, reaching **940,532 meaningful exact code bytes / 6,327
credited functions**:

| Unit | Function | Meaningful | Padded |
| --- | --- | ---: | ---: |
| `source/physics/collision_bsp` | `collision_surface_test_point` | 293 | 304 |
| `source/physics/collision_bsp` | `sphere_test_vector` | 204 | 208 |
| `source/physics/collision_bsp` | `pill_test_vector` | 528 | 528 |
| `source/physics/collision_bsp` | `collision_surface_test_pill` | 465 | 480 |
| `source/objects/damage` | `damage_effect_new_at_location` | 509 | 512 |
| `source/objects/damage` | `object_destroy_region` | 306 | 320 |

`object_destroy_notify_children` also replaces its authenticated
`code_00126090` address name. Its existing 80-byte body was already exact,
so the rename earns zero byte and function credit.

The rename-stable all-unit census advances from 6,367 to **6,373 exact owners
out of 8,245**, with exactly the six rows above and zero regressions. Matching
objects remain **390/833** and matched data remains **2,062,692 bytes**.

## CollisionBSP source boundary

All nine previously missing private CollisionBSP bodies are now written with
semantic private names and translation-unit-local prototypes. The four exact
owners above are credited. Five complete natural bodies remain fuzzy and are
recorded in `config/parked.json` with zero exact credit:

| Function | Target / candidate padded | Relocations | Objdiff |
| --- | ---: | ---: | ---: |
| `collision_bsp_test_pill_new_recursive` | 672 / 672 | 8 / 8 | 99.210526% |
| `collision_leaf_test_vector` | 400 / 400 | 7 / 7 | 89.64331% |
| `collision_bsp_test_vector_recursive` | 800 / 784 | 13 / 13 | 98.178215% |
| `bsp2d_test_pill_recursive` | 272 / 272 | 6 / 6 | 98.95652% |
| `bsp3d_test_pill_recursive` | 1,504 / 1,472 | 31 / 28 | 85.46875% |

The packet uses genuine collision-test enums, project `real` and vector
types, tag-block access, a semantic leaf flag and the real projection mapping.
It emits none of `point_from_line3d`, `projection_sign_from_vector3d`,
`scale_vector3d`, `normalize3d` or `negate_vector3d` as a new code owner.
All 17 inherited exact CollisionBSP functions remain exact.

`pill_test_vector` retains an apparently reversed capsule quadratic because
the independently named HCEA 2011 source at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains the same delta,
coefficient, root and cap-selection behavior. This is documented as an
authenticated original quirk, not accepted merely because it matches.

## Damage source boundary

The Damage packet restores the two exact private helpers, names the existing
child-notification owner, and writes a complete typed `object_damage_body`.
The material owner header now exposes the authenticated shield-leak,
shield/body multiplier fields while preserving its 0x48 layout and checked
+0x28/+0x3C offsets. The header reaches Damage, Objects, CollisionModels and
Collisions; focused gates retain 121/121, 7/7 and 8/8 inherited exact owners in
the three other units.

Two complete Damage functions remain honest fuzzy parks:

| Function | Target / candidate padded | Relocations | Objdiff |
| --- | ---: | ---: | ---: |
| `object_damage_body` | 1,360 / 1,392 | 43 / 44 | 88.00236% |
| `object_cause_damage` | 1,984 / 1,952 | 75 / 75 | 88.5429% |

The nearer donor probe for `object_damage_body` repeatedly cast a raw
`object_get` result to a unit datum. It is rejected despite its higher
superficial similarity. The retained source performs a generic object-type
check and crosses the established `unit_get` typed boundary. Both fuzzy
bodies receive zero exact credit. `object_damage_shield` and
`object_damage_aftermath` remain unwritten.

## Rejected threshold probe

A 296-meaningful-byte `render_debug_string_at_point` candidate was strict in
its focused gate, but its required prototype in the genuine
`render_cameras.h` owner changed the preprocessed definition stream of many
consumers. A full rebuild gained that 304-byte owner while regressing four
previously exact owners:

- `decals_delete_permanent_from_cluster`;
- `rasterizer_dynamic_geometry_initialize`;
- `render_sky`; and
- `unit_preprocess_node_orientations`.

The entire RenderDebug probe was removed and receives no credit. This records
why that probe receives no credit. A later source-only GameState allocation
owner passed with its semantic global rename and brings the combined batch over
the scheduled 940,828 treemap-refresh threshold; see
`game_state_xbox_obj_allocate_buffer_reconciliation_20260908.md`.

## Verification

- full `ninja all_source progress semantic_progress`: pass;
- hardened complete-section comparison: all six credited functions equal,
  including ordered relocations;
- rename-stable global sweep: six gains / 2,352 padded bytes, zero regressions;
- CollisionBSP gate: 21 exact / 9 residual / 0 unwritten;
- Damage mapped gate: 29 exact / 3 residual / 2 unwritten;
- forbidden `point_from_line3d` guards: pass in both changed units;
- fake-match scan of both C files and the material header: zero review leads;
- parked ledger: 323 active / 0 stale / 0 invalid;
- object admission: 0 candidates / 0 contradictions / 0 revocations, with five
  inherited explicit rejections;
- scoped tooling tests: 1,120 passed, 2 skipped, 26 subtests passed.

Scratch evidence is frozen under
`scratch/collision-bsp-owner-packet-20260907/`,
`scratch/damage-owner-packet-20260907/`, and
`scratch/collision-damage-after-stable-20260907.json`.
