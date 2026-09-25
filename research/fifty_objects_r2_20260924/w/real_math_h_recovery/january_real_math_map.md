# January `real_math.h`: recovered map (round 2, lab only)

Slug: `scratch/w/real_math_h_recovery/`. Tree 42125a52, clean; no tracked file was edited.
`math/real_math` is RESERVED for the external Codex packet. Everything here is evidence plus a measured lab
header (`real_math_jan.h`). None of it is proposed for landing now.

Evidence tiers used throughout:

- **JAN**: January first-party evidence. This covers January bytes and relocations, assert anchors, cachebeta.pdb
  publics (rva == file_offset) and the order of COMDAT batches in the January objects.
- **HCEX**: HCEX_Release.pdb / HCEX.pdb (2011, later revision). Sources are line records (`DIA2Dump -lsrc`), the
  include graph (`-sf`) and type records (`-type`).
- **LATER**: the later /Od build `halo_cache_symbols.exe` (`scratch/tools/odbuild.py`).
- **INF**: inferred.

## 1. Assert-anchor line map

`anchor_scan.py --tree split` scanned all 833 January objects for DIR32 relocations to a literal ending in
`real_math.h`. External `??_C@` names were demangled. The scan found **10 hits on 8 distinct lines, and that is the
complete January set**:

| January line | helper | January copy | ours | /Od (LATER) | HCEX `{` (+1 = first stmt) |
|---:|---|---|---:|---:|---:|
| 848 | projection_sign_from_vector3d | decals COMDAT; inlined twice in collision_bsp | 997 | 871 | 873 |
| 859 / 860 | project_point3d | path_obstacles | 1014 / 1015 | 882 / 883 | 884 |
| 879 / 880 | project_point2d | decals | 1035 / 1036 | 902 / 903 | 901 (locals 902-903, asserts 905/906) |
| 1508 / 1509 | uniform_cubic_spline | following_camera | 1550 / 1551 | 1549 / 1550 | 1551 |
| 1530 | nonuniform_cubic_spline, `t>= t0 && t <= t3` | glow | **absent (junk body)** | 1571 | 1573 |

- HCEX equals /Od + 3 lines at all five /Od anchors, so the two later sources are one header revision.
- Spans: 848→859 is 11 in January, /Od and HCEX (ours 17 = 10 + a 7-line `#ifdef REAL_MATH_EXTERNAL_PROJECT_POINT3D`
  wrapper, **so January had no wrapper there**). 859→879 is 20 in all of them (ours 21 = +1 `#endif`). 1508→1530 is 22
  in all of them.
- 880→1508: January 628, HCEX 646 (+18), ours 514 (+59 lines of campaign `REAL_MATH_EXTERNAL_*` wrappers).
  **January therefore has about 173 lines of real text in this span that we lack.**
  - HCEX-only later additions in the span account for 24 lines: `fast_normalize3d` (14) and `cross_product_magnitude3d`
    (10).
  - Many helpers gained 4 lines of asserts after January. HCEX RELEASE has no code on those lines, and January's copies
    of the same helpers carry no assert. So HCEX line spans do not prove January locals.
- Above 848, HCEX is January +26. The later addition `fast_normalize2d` accounts for 14 of those lines. HCEX has a
  105-line block between `arcsine` and `signed_angular_difference` that nothing emits; its content is unknown.

The full line map of 96 helpers (HCEX `{` lines against ours) is in `align_hcex_ours.txt`, `body_spans.txt` and the
inventory table below.

## 2. Helper inventory (first-party)

`jan_inlines.py` joins cachebeta publics with the `contribs.json` module order. Main facts:

- **93 real_math.h-family helpers are emitted out of line in January, and all 88 inline helpers in our header are
  among them.** No helper of ours lacks January attestation.
- **VC7 batch law** (new, verified on 34 batches): each not-yet-emitted inline callee is emitted just before the first
  function that needs it. Within a batch the order is HEADER DEFINITION ORDER. Every batch is monotone in ours and in
  HCEX (0 inversions; `batches.md`).
- **January helpers absent from our real_math.h.** The January copy of each is EXACT in our build, but as a
  consumer-.c body:

| helper | January evidence | ours today | lab record cost |
|---|---|---|---:|
| vector_intersect_plane2d | public; geometry batch `plane2d_distance_to_point < vector_intersect_plane2d < vector_intersect_plane3d` (JAN position) | plain def in geometry.c | +8 |
| valid_real_vector2d | public; players batch `random_direction3d < valid_real_vector2d` (JAN); slot after valid_real_point2d = HCEX +6-line gap (INF) | plain def in players.c | +4 |
| valid_real_normal2d | public; relocations from actor_looking, actor_moving, actors, units (a shared inline) | `__inline` in actor_looking.c + actor_looking.h prototype | +4 |
| valid_real_plane3d | public; matrix_math #33, callers collisions + render_cameras | prototype l.672 + matrix_math.c def | +1 (proto→def) |
| dequantize_byte_to_real_rectangle3d | public, structure_visibility #9; inlines dequantize ×6 (no call) | commented out in our header; plain def in structure_visibility.c + campaign `static __inline structure_visibility_dequantize_byte_to_real` | open |
| valid_real_rgb_color / valid_real_argb_color | publics shared by bitmap_utilities/decals/effects/hud_draw/particles (rgb) and hud_draw/particles (argb); particles batch `valid_real_point3d < valid_real_argb_color` | bitmap_utilities.c / particles.c + bitmaps.h / particles.h prototypes | +8 (header INF) |

- **Later additions, not January:**
  - `fast_normalize2d/3d`, `cross_product_magnitude3d` and `dequantize_byte_to_real` are header inlines in HCEX
    (770/1020/1107/1726). January emits them in real_math.obj **#24-#27, in .c definition order** between
    `pill_intersects_rectangle3d` and `signed_angle_between_vectors2d`, so they are real_math.c functions (as in ours).
  - `vector_intersect_plane3d`'s 12-line HCEX body has code only on the return line. The extra lines are later asserts,
    and January's copy has no assert.
- **Types (HCEX `-type`):** every real_math.h union/struct has exactly our member names, e.g. point2d `n x y u v`,
  argb `n alpha rgb red green blue`, matrix4x3 `scale n forward left up position`. HCEX also has `real_hsv_color`
  (`n hue saturation value`); ours defines it in bitmap_utilities.h. Its header is unknown (INF).
- **Headers (HCEX `-sf`):** the only 2011 math headers are geometry.h, integer_math.h, periodic_functions.h,
  real_math.h and vector_tree.h. matrix_math.h, random_math.h, real_math_planes.h, real_math_cones.h and
  real_math_declarations.h are campaign inventions. Each also costs +1 file record in every TU that includes it.

The full table follows. Each row gives the helper's `{` line in ours and in HCEX, and the January object and position
of its out-of-line copy:

| # | helper | ours `{` | HCEX `{` | January anchor | January out-of-line copy (object #position) | tier / note |
|---|---|---|---|---|---|---|
| 1 | sine | 739 | 485 |  | actor_combat #11 | JAN |
| 2 | cosine | 745 | 491 |  | actor_combat #12 | JAN |
| 3 | tangent | 751 | 497 |  | actor_combat #13 | JAN |
| 4 | arctangent | 763 | 504 |  | actor_perception #8 | JAN |
| 5 | arccosine | 770 | 510 |  | actor_moving #7 | JAN |
| 6 | arcsine | 776 | 516 |  | following_camera #2 | JAN |
| 7 | signed_angular_difference | 788 | 633 |  | player_control #35 | JAN |
| 8 | square_root | 811 | 644 |  | action_charge #5 | JAN |
| 9 | reciprocal_square_root | 818 | 650 |  | aim_assist #4 | JAN |
| 10 | power | 825 | 657 |  | weapons #42 | JAN |
| 11 | set_real_point2d | 833 | 665 |  | path_obstacles #1 | JAN |
| 12 | set_real_vector2d | 843 | 676 |  | actions #34 | JAN |
| 13 | point_from_line2d | 854 | 688 |  | action_vehicle #5 | JAN |
| 14 | vector_from_points2d | 864 | 699 |  | action_obey #15 | JAN |
| 15 | scale_vector2d | 874 | 709 |  | action_charge #6 | JAN |
| 16 | rotate_vector2d | 885 | 720 |  | path_obstacles #2 | JAN |
| 17 | magnitude_squared2d | 895 | 733 |  | action_charge #7 | JAN |
| 18 | magnitude2d | 901 | 743 |  | action_charge #8 | JAN |
| 19 | normalize2d | 907 | 753 |  | action_charge #9 | JAN |
| 20 | fast_normalize2d | - | 770 |  | - | HCEX; HCEX-only header inline; January = real_math.c #24 |
| 21 | limit2d | 925 | 784 |  | player_control #36 | JAN |
| 22 | distance_squared2d | 937 | 800 |  | action_vehicle #6 | JAN |
| 23 | distance2d | 945 | 809 |  | action_vehicle #7 | JAN |
| 24 | dot_product2d | 952 | 827 |  | action_charge #10 | JAN |
| 25 | cross_product2d | 959 | 838 |  | actor_combat #14 | JAN |
| 26 | negate_vector2d | 966 | 849 |  | action_vehicle #8 | JAN |
| 27 | projection_from_vector3d | 974 | 860 |  | decals #11 | JAN |
| 28 | projection_sign_from_vector3d | 996 | 873 | 848 | decals #12 | JAN |
| 29 | project_point3d | 1013 | 884 | 859/860 | path_obstacles #3 | JAN |
| 30 | project_point2d | 1031 | 901 | 879/880 | decals #13 | JAN |
| 31 | set_real_point3d | 1052 | 920 |  | ai_debug #11 | JAN |
| 32 | set_real_vector3d | 1065 | 933 |  | actions #35 | JAN |
| 33 | point_from_line3d | 1085 | 946 |  | action_charge #11 | JAN |
| 34 | vector_from_points3d | 1097 | 958 |  | action_alert #5 | JAN |
| 35 | scale_vector3d | 1114 | 970 |  | action_charge #12 | JAN |
| 36 | magnitude_squared3d | 1128 | 983 |  | action_alert #6 | JAN |
| 37 | magnitude3d | 1139 | 993 |  | action_charge #13 | JAN |
| 38 | normalize3d | 1150 | 1003 |  | action_charge #14 | JAN |
| 39 | fast_normalize3d | - | 1020 |  | - | HCEX; HCEX-only header inline; January = real_math.c #25 |
| 40 | limit3d | 1169 | 1034 |  | aim_assist #5 | JAN |
| 41 | distance_squared3d | 1183 | 1050 |  | action_alert #7 | JAN |
| 42 | distance3d | 1191 | 1059 |  | action_vehicle #9 | JAN |
| 43 | midpoint3d | 1199 | 1067 |  | actor_moving #8 | JAN |
| 44 | dot_product3d | 1214 | 1078 |  | action_charge #15 | JAN |
| 45 | cross_product3d | 1223 | 1090 |  | action_obey #16 | JAN |
| 46 | cross_product_magnitude3d | - | 1107 |  | - | HCEX; HCEX-only header inline; January = real_math.c #26 |
| 47 | triple_product3d | 1238 | 1117 |  | decals #14 | JAN |
| 48 | add_vectors3d | 1247 | 1127 |  | actor_combat #15 | JAN |
| 49 | subtract_vectors3d | 1259 | 1139 |  | action_charge #16 | JAN |
| 50 | negate_vector3d | 1270 | 1192 |  | action_obey #17 | JAN |
| 51 | dot_product4d | 1280 | 1203 |  | structure_detail_objects #11 | JAN |
| 52 | point_to_line_distance3d | 1314 | 1211 |  | actions #36 | JAN |
| 53 | plane2d_from_points | 1288 | 1253 |  | decals #15 | JAN |
| 54 | plane2d_distance_to_point | 1306 | 1284 |  | geometry #2 | JAN |
| 55 | plane3d_from_point_and_normal | 1323 | 1302 |  | decals #16 | JAN |
| 56 | plane3d_from_points | 1334 | 1316 |  | connected_geometry #5 | JAN |
| 57 | plane3d_negate | 1357 | 1339 |  | decals #17 | JAN |
| 58 | plane3d_distance_to_point | 1374 | 1351 |  | decals #18 | JAN |
| 59 | vector_intersect_plane3d | 1383 | 1361 |  | geometry #4 | JAN |
| 60 | point_in_circle | 1393 | 1380 |  | path_obstacles #4 | JAN |
| 61 | point_in_sphere | 1401 | 1388 |  | path_obstacles #5 | JAN |
| 62 | set_real_quaternion | 1411 | 1398 |  | vehicles #13 | JAN |
| 63 | set_real_euler_angles2d | 1422 | 1413 |  | aim_assist #6 | JAN |
| 64 | set_random_seed | 1430 | 1422 |  | game #23 | JAN |
| 65 | random | 1436 | 1430 |  | weapons #43 | JAN |
| 66 | random_range | 1443 | 1437 |  | action_obey #18 | JAN |
| 67 | random_boolean | 1449 | 1445 |  | actor_combat #16 | JAN |
| 68 | real_random | 1455 | 1451 |  | action_charge #17 | JAN |
| 69 | real_random_range | 1467 | 1458 |  | action_alert #8 | JAN |
| 70 | random_direction3d | 1474 | 1464 |  | players #44 | JAN |
| 71 | random_vector_in_cone3d | 1483 | 1480 |  | projectiles #18 | JAN |
| 72 | local_random | 1489 | 1486 |  | debug_memory #6 | JAN |
| 73 | local_random_range | 1496 | 1495 |  | contrails #9 | JAN |
| 74 | real_local_random | 1506 | 1503 |  | effects #20 | JAN |
| 75 | real_local_random_range | 1519 | 1512 |  | bored_camera #5 | JAN |
| 76 | local_random_direction3d | 1528 | 1526 |  | effects #21 | JAN |
| 77 | local_random_vector_in_cone3d | 1537 | 1542 |  | contrails #10 | JAN |
| 78 | uniform_cubic_spline | 1549 | 1551 | 1508/1509 | following_camera #3 | JAN |
| 79 | nonuniform_cubic_spline | 1575 | 1573 | 1530 | glow #15 | JAN; ours = junk body (v9..v11); January body in glow.c |
| 80 | uniform_cubic_spline_vector3d | 1599 | 1597 |  | following_camera #4 | JAN |
| 81 | nonuniform_cubic_spline_vector3d | 1640 | 1614 |  | glow #16 | JAN; ours = junk body (v10..v20); January body in glow.c |
| 82 | valid_realcmp | 1689 | 1623 |  | actor_combat #26 | JAN |
| 83 | valid_real | 1682 | 1634 |  | actor_combat #17 | JAN |
| 84 | valid_real_point2d | 1696 | 1642 |  | path_obstacle_avoidance #15 | JAN |
| 85 | valid_real_point3d | 1702 | 1654 |  | particles #10 | JAN |
| 86 | valid_real_vector3d | 1708 | 1660 |  | bored_camera #6 | JAN |
| 87 | valid_real_normal3d | 1714 | 1679 |  | actor_combat #27 | JAN |
| 88 | valid_real_vector3d_axes2 | 1721 | 1704 |  | bored_camera #7 | JAN |
| 89 | valid_real_vector3d_axes3 | 1732 | 1712 |  | items #13 | JAN |
| 90 | dequantize_byte_to_real | - | 1726 |  | - | HCEX; HCEX-only header inline; January = real_math.c #27 |
| 91 | dequantize_byte_to_real_rectangle3d | - | 1734 |  | structure_visibility #9 | JAN; ours commented out; January copy = structure_visibility.c def (EXACT) |
| 92 | valid_real_matrix4x3 | 1744 | - |  | items #14 | JAN |
| 93 | interpolate_scalar | 1771 | 1749 |  | player_control #37 | JAN |
| 94 | valid_real_normal2d | - | - |  | actor_looking #10 | JAN; MISSING in ours (actor_looking.c __inline + actor_looking.h proto) |
| 95 | valid_real_plane3d | - | - |  | matrix_math #33 | JAN; ours = prototype + matrix_math.c def |
| 96 | valid_real_vector2d | - | - |  | players #45 | JAN; MISSING in ours (players.c plain def) |
| 97 | vector_intersect_plane2d | - | - |  | geometry #3 | JAN; MISSING in ours (geometry.c plain def) |

January batches (each is an order constraint; `particles` continues `< valid_real_argb_color`):

| January object | batch (header definition order; emitted just before the first user) |
|---|---|
| vehicles | set_real_quaternion |
| structure_visibility | dequantize_byte_to_real_rectangle3d |
| structure_detail_objects | dot_product4d |
| glow | nonuniform_cubic_spline < nonuniform_cubic_spline_vector3d |
| matrix_math | valid_real_plane3d |
| geometry | plane2d_distance_to_point < vector_intersect_plane2d < vector_intersect_plane3d |
| connected_geometry | plane3d_from_points |
| weapons | power < random |
| projectiles | random_vector_in_cone3d |
| items | valid_real_vector3d_axes3 < valid_real_matrix4x3 |
| players | random_direction3d < valid_real_vector2d |
| player_control | signed_angular_difference < limit2d < interpolate_scalar |
| game | set_random_seed |
| aim_assist | reciprocal_square_root < limit3d < set_real_euler_angles2d |
| particles | valid_real_point3d |
| effects | real_local_random < local_random_direction3d |
| decals | projection_from_vector3d < projection_sign_from_vector3d < project_point2d < triple_product3d < plane2d_from_points < plane3d_from_point_and_normal < plane3d_negate < plane3d_distance_to_point |
| contrails | local_random_range < local_random_vector_in_cone3d |
| debug_memory | local_random |
| following_camera | arcsine < uniform_cubic_spline < uniform_cubic_spline_vector3d |
| bored_camera | real_local_random_range < valid_real_vector3d < valid_real_vector3d_axes2 |
| path_obstacles | set_real_point2d < rotate_vector2d < project_point3d < point_in_circle < point_in_sphere |
| path_obstacle_avoidance | valid_real_point2d |
| ai_debug | set_real_point3d |
| actor_perception | arctangent |
| actor_moving | arccosine < midpoint3d |
| actor_looking | valid_real_normal2d |
| actor_combat | sine < cosine < tangent < cross_product2d < add_vectors3d < random_boolean < valid_real |
| actor_combat | valid_realcmp < valid_real_normal3d |
| actions | set_real_vector2d < set_real_vector3d < point_to_line_distance3d |
| action_vehicle | point_from_line2d < distance_squared2d < distance2d < negate_vector2d < distance3d |
| action_obey | vector_from_points2d < cross_product3d < negate_vector3d < random_range |
| action_charge | square_root < scale_vector2d < magnitude_squared2d < magnitude2d < normalize2d < dot_product2d < point_from_line3d < scale_vector3d < magnitude3d < normalize3d < dot_product3d < subtract_vectors3d < real_random |
| action_alert | vector_from_points3d < magnitude_squared3d < distance_squared3d < real_random_range |

## 3. Prototype inventory

Every one of our 119 real_math.h prototypes names a January public. The following **January publics have no
declaration in our real_math.h** (`math_publics.txt`):

- real_math.c: vector_intersects_triangle2d, vector_intersects_triangle3d, pill_intersects_rectangle2d,
  line_from_planes3d (real_math_planes.h), pin_normal_to_cone3d (real_math_cones.h).
- matrix_math.c: matrix3x3_determinant, matrix3x3_inverse, matrix3x3_from_axis_and_angle, matrix3x3_multiply
  (matrix_math.h), matrix3x3_rotation_to_quaternion. vehicles.c calls this last one with no declaration, which is C4013.
- random_math.c: random_math_initialize, random_math_dispose (real_math.c calls both undeclared), get_random_seed and
  random_seed_debug_log (random_math.h), get_number_suitable_for_initializing_random_seed, seed_random_orientation.

That these functions exist and are external is JAN. That they sit in real_math.h is INF: Bungie's
`prototypes/X.C` sections, plus HCEX having no focused headers. January definition order differs from our prototype
order in 25/10/4 places (`proto_order.txt`). Prototypes emit no bytes, so their order has no first-party evidence.

## 4. Junk repairs

| repair | evidence | record delta |
|---|---|---:|
| `nonuniform_cubic_spline`: v9..v11 junk (v9 used uninitialised) → glow.c body with assert anchor 1530 | glow `_nonuniform_cubic_spline` 192 B strict EXACT; January anchor 1530 | 0 |
| `nonuniform_cubic_spline_vector3d`: v10..v20 junk → per-component calls | glow `_nonuniform_cubic_spline_vector3d` 176 B strict EXACT | −11 |
| `real_local_random` named local | January COMDAT a8ad3cb3 (effects) + /Od | +1 |
| `valid_real_plane3d` prototype → `__inline` | January public + Lane B ruling | +1 |

## 5. Record-currency calibration (probe.py; draw counter; new)

- Prototype `f(void)` costs 2. With n **named** params it costs **1 + 2n** (3, 5, 7, 9, 11, 13 for n = 1..6). With n
  unnamed params it costs 1 + n.
- Definition = prototype + 1: `void` is 3, n params is 2 + 2n. A redeclaration costs only its params (2n); this
  explains the canary's stack_walk −2.
- A local costs +1, `if` +1 and a distinct literal +1. `return a && b` costs 0.
- **Correction to canary F9:** the 6 verify typedefs in rasterizer_frame_statistics.c cost **17** records, not 6
  (`vt` = −17).
  - Known surplus in the draw TU is therefore +17 (typedefs) + 11 (spline junk) + 2 (stack_walk dup) − 2 (rlr, vp)
    = **+28**. January has +28 (mod 64) of content we lack there, not +17.
  - The three January-attested missing bodies (vip2 + vrv2 + vrn2 = +16) leave a **residual of +12 (mod 64)**. The
    measured net of JAN bodies + vt + swcw is −12.

## 6. Named locals from the later build

The random wrappers compared in /Od:

- No local: `random` 0x58c100, `real_random` 0x42e1c0 and `real_random_range` 0x42f360 return the call result directly.
- Named local: `real_local_random` 0x573ae0, `real_local_random_range` 0x521410, `local_random` 0x551170 (word),
  `local_random_range` 0x566650 (word) and `random_range` 0x43e850 (word) spill the call result into an RTC-initialised
  slot and reload it.

HCEX body spans agree: 5 lines for these, against 3 for the global twins. Adding the local to `random_range`,
`local_random` or `local_random_range` costs +1 each. Whole-board sweeps show the January copies stay EXACT, so the
change is **byte-inert: LATER/HCEX only, not double-attested**.

Caveat for all /Od work: modern /Od also spills SSE↔x87 conversions through RTC-initialised temps
(`dot_product2d` 0x42dd30, `reciprocal_square_root` 0x58a4c0). Only integer call-result spills, and float call results
returned through a slot, are evidence of a named local.

## 7. The lab header `real_math_jan.h` and its measured deltas

The file is ours plus every JAN-tier change. The LATER and INF candidates are gated by
`#define REAL_MATH_JAN_LATER` / `#define REAL_MATH_JAN_INFERRED` (macros cost 0). It compiles in all four modes.
Unwrapped values assume the calibrated model.

Net C1 record delta against ours, measured in each TU's own header context (`tu_net.py`). The counter prelude is the
unit's own `#include` and `REAL_MATH_EXTERNAL_*` lines placed in front of rasterizer_frame_statistics.c:

| TU (header context) | JAN (default) | +LATER | +INF | +LATER+INF |
|---|---:|---:|---:|---:|
| rasterizer_frame_statistics | +7 | +10 | −11 (+117) | −8 (+120) |
| math/geometry | +7 | +10 | −11 | −8 |
| game/players | +7 | +10 | −11 | −8 |
| ai/actor_looking (actor_looking.h redeclares valid_real_normal2d) | +6 | +9 | −12 | −9 |
| physics/collisions (EXTERNAL point_from_line3d → proto; real_math_planes.h) | +8* | +11 | −11 | −8 |
| game/aim_assist (real_math_cones.h) | +7 | +10 | −12 | −9 |
| physics/physics (matrix_math.h) | +7 | +10 | −12 | −9 |
| game/game (random_math.h) | +7 | +10 | −14 | −11 |
| units/bipeds (2 EXTERNAL definers) | +9* | +12 | −9 | −6 |
| effects/particles (particles.h redeclares argb) | +7 | +10 | −13 | −10 |
| ai/actor_combat (real_math_declarations.h) | +7 | +10 | −11 | −8 |
| units/units | +6 | +9 | −13 | −10 |

\* measured with `noext`, which removes the wrappers. The shipped `real_math_jan.h` keeps them, so those two TUs read
+7 by default.

The owning .c files lose their plain definitions (geometry.c −8, players.c −4, actor_looking.c −4, matrix_math.c
−4). Those removals come after the header, so they are not in the table.

## 8. Whole-board sweeps (447 units; base = 7126 EXACT)

Files: `grid_g1.txt`, `grid_g2.txt`, `sweep_*.txt`.

- **P0 / R4**: JAN bodies + companions, with or without swcw + vt. Gains `_collision_move_point` 4752 B. Loses 5:
  draw, alpha_bleed, decals_delete_permanent, network_game_client_handle_message, units preprocess.
- **P2 / R1**: + LATER locals. Loses 5 to 7 (draw, alpha_bleed, decals, get_edge_vertex, error_geometry …).
- **P3**: + LATER + colour validators: drawnet −1, loses 7. **P4**: + 16 prototypes: drawnet +26, loses 8.
- **noext**, removing the `REAL_MATH_EXTERNAL_*` wrappers, loses `_biped_update_airborne` and `_biped_update_jumping`
  and drifts data in 3 units. The wrappers are codegen-relevant in their definers. Only the project_point3d wrapper is
  anchor-proven absent in January.
- None of the tested combinations is zero-loss. The +12 (mod 64) residual in the draw TU has many candidate sums, for
  example 3 LATER locals + 9 (any 4-param prototype), or vcol + 2 void prototypes, or 76 (…). The record count alone
  cannot pick one.

## 9. Open items and reopen criteria

1. **Unknown January content.** The 105-line HCEX block after `arcsine`, and the HCEX gaps after
   `subtract_vectors3d` (+42 lines), `point_to_line_distance3d`/`dot_product4d` (+33) and `plane2d_from_points` (+13),
   are never emitted in January, HCEX_Release or /Od. They are unused helpers, macros or comments, and cannot be
   recovered from bytes. Reopen only with a donor header or a later build that uses them.
2. **Structure_visibility's inlinable `dequantize_byte_to_real`.** January inlines it 6 times while real_math.c keeps
   the out-of-line copy (#27). January's name for the inlinable copy is unknown.
3. **Placement of the colour validators and `real_hsv_color`** (real_math.h vs bitmaps headers). This is INF; a
   January batch that mixes them with real_math.h helpers only fixes their order *after* valid_real_point3d.
4. **Which of the 16 prototypes January's header carried.** This is INF. Existence and external storage are JAN.

## 10. Consumer sweep list (post-Codex)

The header change reaches every TU that includes real_math.h (259 TUs; list in the canary slug's
`consumer_sweep_list.txt`). Mandatory extra checks:

- Owners losing a body:
  - geometry.c (vector_intersect_plane2d)
  - players.c (valid_real_vector2d)
  - actor_looking.c + actor_looking.h (valid_real_normal2d)
  - matrix_math.c (valid_real_plane3d)
  - glow.c (spline hand copies behind `#define` renames)
  - particles.c / bitmap_utilities.c + particles.h / bitmaps.h (colour validators, INF)
- `REAL_MATH_EXTERNAL_*` definers: actor_moving, ai, decals, first_person_weapons, collisions,
  rasterizer_xbox_environment, rasterizer_xbox_models, bipeds.
- Focused-header includers: physics (matrix_math.h), player_effects / game / network_server_manager (random_math.h),
  collisions (real_math_planes.h), aim_assist (real_math_cones.h), actor_combat (real_math_declarations.h).
- Count canaries:
  - draw 4176
  - bitmap_copy 2784
  - units preprocess 1920
  - alpha_bleed 560
  - uncompress_from_mipmap 96
  - decals_delete_permanent 400
  - models transparent_submit 1296
  - error_geometry_polygon 496
  - race ×2 (160, 336)
  - get_edge_vertex 416
  - network_game_client_handle_message (new here)
  - biped_update_airborne / jumping (noext only)
  - collision_move_point 4752 (gained)

Rerun: `python measure.py <spec>`, `python tu_net.py <unit> <spec>`, `python grid.py <tag> name=spec ...` and
`python sweep.py <name> <spec>`. The shadow tree is `sh/`. Refresh it with a copy of `source/` after Codex lands.
