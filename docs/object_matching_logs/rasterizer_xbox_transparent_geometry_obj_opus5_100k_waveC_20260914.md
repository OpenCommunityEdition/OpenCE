# rasterizer_xbox_transparent_geometry.obj - opus5 100K consolidated lane, wave C (2026-09-14)

## 1. Scope and provenance

- Translation unit: `source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c`
- Target: `build/split/source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.obj`
- Compiler: XDK 3911 VC7 13.00.9254, repository flags (`/O2 /Oy-`, no `/QIfist`).
- Worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`, wave C worker. Scratch:
  `scratch/workersC/rasterizer_xbox_transparent_geometry/`, log `scratch/workersC/rasterizer_xbox_transparent_geometry.md`.
- Evidence: January target COFF (call relocations, listings); wave-B best candidate
  (`scratch/workers/rasterizer_xbox_transparent_geometry/best.c`); VC7 probe compiles (this TU's flags);
  scratch copy of `source/units/bipeds.c` (probe only); Stian build-2276 `rasterizer.c` (topology only);
  HCEA `rasterizer_transparent_geometry_group_draw.c` (decompiler shapes, names only);
  `rasterizer_xbox_models.c` `fog_plane` local (same-subsystem precedent).
- Previous ledgers: `rasterizer_xbox_transparent_geometry_obj_opus5_100k_20260914.md` (wave B),
  `rasterizer_xbox_transparent_geometry_group_draw_owner_blocker_20260908.md`.

## 2. Validated baseline (lane head)

`gate.py ... --all --forbid-emitted-symbol _point_from_line3d`: **exact 21, residual 1, unwritten 0**. No parks.

| Function | January | Baseline candidate |
| --- | --- | --- |
| `_rasterizer_transparent_geometry_group_draw` | 15,120 / 872 relocs / `e9d4cea8f7e21b93` | 15,072 / 888 / `939338a30c36a6ac` |

First meaningful divergence (baseline): the inlined `dot_product3d` in the zsprite block (January calls it at
0xfae) - the baseline expands all three real_math helpers that January calls out of line.

## 3. Result

**Nothing landed; the production file is unchanged** (`git diff --stat` empty; final gate 21/1/0, guard passed).
The residual is still not strict EXACT. Candidates kept for the orchestrator:

| Candidate | Header | Size | Relocs | Reloc targets | Frame | Masked blocks |
| --- | --- | --- | --- | --- | --- | --- |
| `scratch/workersC/rasterizer_xbox_transparent_geometry/best.c` (w1) | current | 15,104 | 872 | equal except 2 assert literal names | 0x550, 0 slot mismatches | 60 |
| `.../best_with_header.c` (w1h) + `proposed_shader_definitions.h` + `shaders_with_header.c` | proposed | 15,104 | 872 | all equal | 0x550 | 58 |

w1 is byte-identical to the wave-B best (normalized sha16 `aa7f9c64588b425c`) but **no longer needs the
`REAL_MATH_EXTERNAL_DOT_PRODUCT3D` / `REAL_MATH_EXTERNAL_PLANE3D_DISTANCE_TO_POINT` defines** (section 4.1).
Both candidates keep all 21 siblings at January hashes; owner census equals `build/base`; fake scan 0 leads.

## 4. Accepted shapes in the candidate (not in production)

1. **Out-of-line real_math calls from natural source.** `struct render_camera const *camera =
   &global_window_parameters.camera;` bound at the top of the zsprite block (all camera fields there read
   through it, including `-dot_product3d(&camera->forward, (real_vector3d *)&camera->position)`), and
   `struct render_fog const *fog = &global_window_parameters.fog;` + the same `camera` local at the top of
   both planar-fog blocks (`plane3d_distance_to_point(&fog->plane, &camera->position)`, `fog->planar_*`).
   VC7 constant-propagates the pointers (identical absolute operands and pushes), but the inliner sees
   pointer-valued arguments in a cold block and emits January's 1 + 2 call relocations (section 5).
2. Everything else is the wave-B shape list (wave-B ledger section 4), unchanged.

## 5. VC7 inline law measured this wave (argument kind x block frequency)

Probe compiles (`q1`-`q7`, standalone functions with this TU's flags), all measured:

| Call arguments | Threshold for an out-of-line call |
| --- | --- |
| pointer parameters / pointer locals / `&param->field` / `&global_array[i]` | >= 5 enclosing conditions (`&&` operands and nested `if`s count alike; a `?:` arm counts as one - measured under 4 ifs); inside a 2-trip `for` loop >= 8; one `if` inside a case of a 7-target switch is out of line, inside a 5-target switch still inline |
| any address of a named object (`&global`, `&global.member`, `&local_struct`, a cast of one) for `dot_product3d` | never (inlined at 20 levels) |
| `plane3d_distance_to_point` with a pointer plane and `&global` point | threshold applies (only the plane argument counts) |

- The unqualified "inline frequency law" does not move group_draw's sites: `?:` arm (p2), MIN/MAX duplication (p3),
  5/16 extra nested ifs around the dot (p4, z16) or plane (f16), loop removed (p5), function cut to 8.9K (r1-r3):
  all still inline, because January-spelled `&global_window_parameters.x` arguments are named-object addresses.
- biped_update_jumping (scratch bipeds copy): its law sites take `&biped->...` / a `velocity` pointer local, which
  is why the law showed there; removing `!impulse &&` makes the scale call inline (4 levels). Open detail:
  deleting the three `point_from_line3d` calls also inlines its `MAX(0, dot)` arm copy.
- Consequence for other "per-caller out-of-line real_math" residuals: look for a pointer-valued argument
  spelling (bound pointer local, `&pointer->field`) in a block >= 5 conditions deep (>= 8 inside a loop),
  before any REAL_MATH_EXTERNAL define. Existing example: `rasterizer_xbox_models.c` `fog_plane`.

## 6. Experiment matrix (wave C; siblings stayed 21/21 exact in every compiled variant)

| ID | Shape | Size / relocs | Masked blocks | Result |
| --- | --- | --- | --- | --- |
| b0 | wave-B best.c reproduced | 15,104 / 872 | 60 | reference |
| b1 | b0 without REAL_MATH_EXTERNAL defines | 15,200 / 883 | 90 | all helpers inline |
| p1-p5, z6-z16, f4-f16, r1-r3 | frequency probes (extra guards, ?: arm, 5-16 nested ifs, loop removal, 8.9K cut) | - | - | still inline (learning only) |
| y1 / y2 | camera pointer (dot) / + fog & camera pointers (plane) | 15,168/880 ; 15,104/872 | 63 | calls appear |
| **w1** | camera/fog pointer locals bound at block tops | **15,104 / 872** | **60** | byte-identical to b0; best.c |
| w2 | `fog_plane` + `camera_position` pointer locals | 15,104 / 872 | 59 | glass operand order lottery; not adopted |
| m1 / m2 | mixed pointer + global address args | 15,168 / 879 | - | dot inlines, one plane call |
| w1h | w1 + nested-shader_base naming (shadow header) | 15,104 / 872 | 58 | all reloc targets equal; best_with_header.c |
| t1 / t2 | HCEA `node_matrices` local in cortana / both skinning blocks | 15,120 / 872 | 55 / 52 | slot map broken (64 mismatches), merge block still wrong; reverted |
| g1-g3 | glass diffuse term operand spellings | 15,104 / 872 | 60 | byte-identical (canonicalised) |
| n1 / n3 | short `function_index` after frame_count / after counter_limit | 15,120 ; 15,104 | 117 ; 81 | frame 0x554 / slot swap; reverted |

## 7. Do-not-repeat

- Inline-frequency spellings with named-object address arguments (they never go out of line).
- `?:` arm / extra guard / nested-if / loop-removal / function-shrinking probes for the zsprite and fog calls.
- Glass diffuse/detail operand spellings; HCEA node_matrices local; named short function index placements
  (plus wave-B's list: variable names, declaration order, lerp operand order, `(short)` casts, skinning scope).
- Choosing among natural spellings by masked-block count alone (w2) - symbol-numbering lottery.

## 8. Header proposal re-verification (ORCHESTRATOR-PROPOSAL, unchanged text from wave B)

`source/shaders/shader_definitions.h`: nest `struct shader_radiosity_properties radiosity` (flags, detail_level,
power, color_of_emitted_light, tint_color) and `struct shader_physics_properties physics` (flags, material_type)
inside `struct shader_base` (offsets unchanged); `source/shaders/shaders.c` lines 298 and 332
`shader->base.flags` -> `shader->base.radiosity.flags`; this TU embeds `struct shader_base shader` plus
`generic`/`chicago` sub-structs (done in best_with_header.c). Evidence: January `.rdata` literals
`!TEST_FLAG(shader_transparent_generic->shader.radiosity.flags, ...) || shader_transparent_generic->generic.type==...`
(194 bytes) and the chicago twin; HCEA `_shader.h`.

Shadow compile (`scratch/workersC/rasterizer_xbox_transparent_geometry/shadow.py`): all **41** transitive `.c`
consumers (closure over shader_definitions.h, render_sprite.h, particles.h, particle_systems.h,
particle_system_definitions.h, contrail_definitions.h), control vs shadow per-function normalized hashes:
40 TUs unchanged; `rasterizer_xbox_models.c` only `__rasterizer_model_transparent_geometry_submit` moves, and it is
non-exact in both (1,344/93 vs January 1,296/83) - zero exact losses. Direct includers: contrail_definitions.h,
decals.c, particle_system_definitions.h, particles.h, weather_particle_systems.c, game_engine_king.c, models.c,
object_lights.c, antenna.c, flags.c, lightning.c, rasterizer_transparent_geometry.c,
rasterizer_xbox_active_camouflage.c, rasterizer_xbox_dynavobgeom.c, rasterizer_xbox_environment.c,
rasterizer_xbox_environment_fog.c, rasterizer_xbox_models.c, rasterizer_xbox_plasma_energy.c,
rasterizer_xbox_shadows.c, rasterizer_xbox_transparent_geometry.c, rasterizer_xbox_water.c,
shader_transparent_chicago_preprocessor.c, shader_transparent_generic_preprocessor.c, render_objects.c,
render_sprite.h, shader_definitions.c, shaders.c, structure_lens_flares.c, structure_render.c,
structure_visibility.c. The shadow test used production files; the orchestrator must re-gate against any
in-flight worker edits.

## 9. Residual classification (best.c / best_with_header.c)

- Measured: equal size class (15,104 vs 15,120), equal relocation multiset, equal frame and slot map; remaining
  tie sites: cortana-vs-no-queue skinning store tail-merge (0x81c / 0x96a), numeric function index width and
  register roles (0x13d4 / 0x1fa5 / 0x203b), stage colour lerp x87 keep-vs-consume (0x1c2b), glass diffuse
  commutative load order (0x3178-0x3209), meter pixel-shader store order (January stores in source order,
  0x37ad-0x38bc), plus register renames and address-following alignment pads/jump tables.
- Measured: adding locals in earlier switch arms (y2, w2) flips the glass operand order with no other change.
- Inference: these are C2 tie-breaks keyed on symbol/temp numbering (see memory declaration-count oracle),
  so they are not honestly steerable while the TU carries TU-local struct/extern copies instead of
  January's header declarations. Scheduling/register class, not missing logic.

## 10. Reopen criteria

1. Header proposal applied (start from `best_with_header.c`), and
2. January-shaped shared declarations replacing this TU's local struct/extern copies (changes symbol numbering
   globally), or new evidence for the skinning merge block / meter store order / numeric index width.

## 11. Disposition

Still residual, zero exact credit. Production source untouched. No park entry.
