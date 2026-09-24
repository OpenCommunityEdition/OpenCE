# actor_combat P1 (cross_product2d): measurements at cdc8ebd3

Every number below was measured in this slug on the tree at cdc8ebd3, which was clean. Raw logs are next to this
file: `gate_*.txt`, `audit_*.txt`, `pdb_storage.txt`, `surplus_identity.txt`, `link_prod_*.txt`, `link_p1_*.txt`,
`census_cross_product2d.txt`, `warn_*.txt`, `admission_audit_*.json` and `alndiff_aim_grenade_p1.txt`. Objects are
in `obj/` and sources in `src/`.

## 1. The edit (patches/01) and its byte effect

| Measure | Production | P1 | P1 with the cast removed (strip test) |
|---|---|---|---|
| gate.py --all actor_combat | 32 EXACT, 2 residual | same rows | same rows |
| `_actor_aim_grenade` | 576/22 sha e9bba5b0 (January 544) | unchanged | unchanged |
| `_actor_combat_plan_grenade_trajectory` (parked) | 288/5 sha ff9a7477 = park base | unchanged | unchanged |
| `_cross_product2d` section | EXACT, 32 B, COMDAT **NODUPLICATES** | EXACT, 32 B, COMDAT **ANY** | EXACT, ANY |
| keyed raw-COFF diff against production (88 sections, `rawcoff.py --keyed`) | n/a | 1 difference: the `_cross_product2d` COMDAT selection | 0 differences against P1 |
| wave-2 objeq.py (section_infos_equal) | n/a | 81 sections, 0 changed, same symbol set | n/a |
| object_audit | FAIL(2): the 2 residual functions only; 60 January symbols, 0 differ | identical output | n/a |
| CL /Zs /W3 warnings | 17 | 17 (same multiset) | 18 (+C4133 `real_vector3d *` to `const real_vector2d *`) |
| tools/fake_match_scan.py | 0 leads | 0 leads | n/a |

- The gate object for production is keyed-identical to `build/base/source/ai/actor_combat.obj`, so the base
  build is current.
- January's split lists every code COMDAT as NODUPLICATES, including `_add_vectors3d`, which our build emits as
  ANY. That is a csplit artifact. After P1, `_cross_product2d` has the same selection as every other header
  inline.
- Only `source/ai/actor_combat.c` changes. No header changes, so no other translation unit compiles differently.
- actor_combat itself stays NonMatching. Its blockers are two residual functions and four pre-existing
  candidate-only `_actor_combat_*_inline` externals that have no January provider. P1 changes neither.

## 2. The five objects the ruling unlocks

All five were measured with tree source; none needs a source change. Each gate object is keyed-identical to
build/base. "Surplus" counts are the object's candidate-only code COMDATs that are not identical to January's
copy.

| Object | gate --all | object_audit | PDB storage | Surplus not identical | Link, production actor_combat | Link, P1 actor_combat | objdiff 3.3.1 report |
|---|---|---|---|---|---|---|---|
| ai/path_obstacle_avoidance | 24/24 EXACT | PASS (52 symbols, 0 differ) | 0 disagreements | 0 of 11 | FAIL(1) `_cross_product2d` | **PASS** (17 rows) | code 7301/7301, data 9570/9570 |
| ai/path_smoothing | 6/6 EXACT | PASS (15, 0) | 0 | 0 of 10 | FAIL(1) `_cross_product2d` | **PASS** (14 rows) | code 3021/3021, data 348/348 |
| math/real_math | 84/84 EXACT | PASS (134, 0) | 0 | 0 of 35 | FAIL(1) `_cross_product2d` | **PASS** (58 rows) | code 20172/20172, data 976/976 |
| physics/collision_features | 20/20 EXACT | PASS (30, 0) | 0 | 0 of 29 | FAIL(1) `_cross_product2d` | **PASS** (37 rows) | code 6929/6929, data 896/896 |
| ai/path_structure_bsp | 6/6 EXACT | PASS (13, 0) | 0 | 0 of 10 | FAIL(1) `_cross_product2d` | **PASS** (14 rows) | code 2512/2512, data 200/200 |

- The PASS links used `provlink2.py --subst=source/ai/actor_combat:obj/actor_combat_p1.obj`, both orders, VC7
  Link.Exe.
- In every one of the five, the only production failure is the LNK2005 against the hand-written NODUP copy.
- **collision_features:** its `_projection_from_vector3d` rejection is stale. The surplus copy is IDENTICAL to
  January's selected decals copy and pair-links PASS in both states.
- **path_structure_bsp:** the HCEX tables `_quantized_pathfinding_surface_widths`/`heights` landed in b9a8d587,
  and the split is current. The audit sees the 64-byte .rdata owned by `_quantized_pathfinding_surface_widths`
  as ok.
- **Object audit:** the split was not emulated because none is needed. There is no symbols.json change, and
  build/split already reflects cdc8ebd3's symbols.json.
- **Data:** all five are at 100% in build/report.json, so no semantic_data_matches entry is needed.
- **Admission audit, production report:**
  - path_obstacle_avoidance, path_smoothing, path_structure_bsp and real_math are already listed as
    `audit-coff-ownership-before-admission` candidates.
  - collision_features is `object-admission-rejected` on the stale entry.
- **Admission audit, emulated after patches 02+03.** The report copy marks the five complete, and the rejections
  copy drops the collision_features entry. Result: candidates 7, contradicted 0, rejected 1 (models, unrelated),
  revoked 0.

**If the owner says yes, the objects that become COMPLETE are these 5:** path_obstacle_avoidance, path_smoothing,
real_math, collision_features and path_structure_bsp. Halo objects would go from 379 to 384. Function and data
credit does not change, because all of it was already credited.

## 3. Every other `_cross_product2d` definer (board census, build/base)

There are 11 definers. All 10 non-actor_combat copies are SELECT_ANY and `section_infos_equal` to January's
actor_combat copy. Every one FAILs the pair link against production actor_combat, and every one PASSes against
P1. The same census on the P1 side leaves 0 NODUP definers.

| Object | Loses the `_cross_product2d` link failure | Remaining provider_link after P1 | Why it still does not become complete |
|---|---|---|---|
| ai/actor_looking | yes | PASS | `_actor_look_update` residual (4704 vs 4720); 15/16 functions |
| effects/decals | yes | PASS | `_decal_clip_to_surface` and `_decal_new_from_collision` residuals; 31/33 functions |
| physics/collision_bsp | yes | PASS | `_bsp3d_test_pill_recursive`, `_bsp3d_test_sphere_recursive` and `_collision_surface_test_sphere` residuals; 27/30 functions |
| math/geometry | yes | FAIL(1) `_plane2d_from_points` | 3 residuals (`_convex_hull3d_expand` and the two `_convex_polygon*_clip_to_plane`), plus a `_plane2d_from_points` provider conflict |
| units/bipeds | yes | FAIL(1) `_object_get_type` | 9 residuals (42/51 functions), plus an `_object_get_type` provider conflict |

All five were re-gated at cdc8ebd3, and each gate object is keyed-identical to build/base.

For decals, P1 removes the premise of the `_cross_product2d` rejection entry ("January's selected Actor Combat
copy is no-duplicates"). Retiring that entry is optional (patch 03b); it does not change decals' status.

## 4. What is NOT measured here (integrator)

These were not run because they are forbidden to workers: the full ninja build, the stable_verdicts sweep, the
parks audit, and pytest. Because only actor_combat.c changes, and its 88 sections are byte-identical apart from
the COMDAT selection, a zero-regression sweep is expected. The batch gate must still confirm it.
