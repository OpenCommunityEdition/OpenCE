# rasterizer_xbox_transparent_geometry.obj - opus5 100K consolidated lane (2026-09-14)

## 1. Scope and provenance

- Translation unit: `source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c`
- Target: `build/split/source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.obj`
- Compiler: XDK 3911 VC7 13.00.9254, repository flags `/O2 /Oy- /DDEBUG /Dxbox` (no `/QIfist`).
- Lane head: `b8cd50113` (worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`).
- Evidence used: January target COFF (code, relocations, literals); HCEA
  `src/blam/rasterizer/rasterizer_transparent_geometry_group_draw.c` and `src/headers/_shader.h`,
  `shader_transparent_generic.h` (names/semantics/DB layout only); Stian build-2276 lift
  `src/halo/rasterizer/rasterizer.c` (topology corroboration); `rasterizer_xbox_widgets.c` (render_proc
  union field). No Ghidra/atlas files were prepared for this object.

## 2. Validated baseline (lane head)

`gate.py ... --all --forbid-emitted-symbol _point_from_line3d`: **exact 21, residual 1, unwritten 0**.

| Function | January | Baseline candidate |
| --- | --- | --- |
| `_rasterizer_transparent_geometry_group_draw` | 15,120 / 872 relocs / `e9d4cea8f7e21b93` | 15,072 / 888 / `939338a30c36a6ac` |

Baseline also emitted target-absent `_dot_product3d` and `_plane3d_distance_to_point` COMDATs.

## 3. Result of this lane

**Nothing landed; the production file is unchanged** (`git diff --stat` empty; final gate 21/1/0).
The residual is not strict EXACT, and an already-written residual may only land when exact. The best
candidate is kept for the orchestrator:

| Candidate | Header | Size | Relocs | Reloc targets | Frame | Offset-masked diff blocks |
| --- | --- | --- | --- | --- | --- | --- |
| `scratch/workers/rasterizer_xbox_transparent_geometry/best.c` | current | 15,104 | 872 | equal except 2 assert literals | 0x550 == January | 60 |
| `.../best_with_header.c` + `.../proposed_shader_definitions.h` | proposed | 15,104 | 872 | **all equal** | 0x550 == January | 58 |

Every one of the 21 sibling functions keeps its January normalized hash in both candidates; the candidate
no longer emits the two target-absent real_math COMDATs; fake-match scan: 0 leads.

## 4. Accepted source shapes in the candidate (each decoded from January bytes)

1. `REAL_MATH_EXTERNAL_DOT_PRODUCT3D` / `REAL_MATH_EXTERNAL_PLANE3D_DISTANCE_TO_POINT` around the
   `real_math.h` include. January calls both helpers out of line at every use in this TU (1 + 2 call
   relocations), so the split is per-TU consistent here; the old blocker's "mixed within the function"
   concern is refuted. The two planar-fog sites must evaluate the call once:
   `real plane_distance = plane3d_distance_to_point(...); PIN(-(plane_distance/depth), 0, 1)` (MIN(MAX())
   re-evaluates the call four times).
2. Debug path: camera_transform initializer in the block right before the viewspace test;
   `MIN(red, MIN(green, blue))`/`MAX(red, MAX(green, blue))`; `intensity = PIN(option,0,1)` then
   `== 0.0f -> 1/32` (January's `<0 / >1 / ==0` tree); `color.red= color.green= color.blue= intensity`.
3. Cortana pre-fill: `long source_object_index` bound after the effect-type test (HCEA shape), source_group
   initializer + `do {...} while (source_group = next_group(...))` (January enters the loop untested).
4. NULL-shader widget groups: January calls `[group+0x48](group+0x4c, group+0x50)`; the TU struct gets an
   anonymous `triangle_buffer`/`render_proc` union (the field `rasterizer_xbox_widgets.c` writes) and
   `if (!group->shader) render_proc(...); else {...}` with the frustum resets inside the else.
5. Model case is `switch (group->effect_type)` with a `match_vassert(..., FALSE, msg)` default
   (January `movsx/dec/je`); multipass cache is one `multipass ? (...) : (...)` condition; skinning is
   declared per use block (debug arm, cortana block, no-queue block).
6. Effect arm: `short combiner_index`; `z_near`/`z_far` named locals; texture modes through XDK
   `PS_TEXTUREMODES(...)`; default blend case `match_vassert(FALSE)`.
7. Generic/chicago arms: short layer/map/stage indices and short counter_limit/counter_value; one
   `for (map_index < NUMBER_OF_SHADER_TRANSPARENT_MAPS)` loop with an inner `if (map_index < maps.count)`
   texture block (January's loop bottom compares with 4); map/type declared before the lookup tables;
   ADDRESSW inline; negate-then-scale `if` pair (January `jg`); `[3]` texanim stores last; one
   `real_vector2d map_scale`; generic loop inside `if (maps.count > 0)` (January pretest `jle` straight to
   the create test); `success = success && shader_*_create(...)` after the standard SVSC check; generic
   numeric function index inline in the PIN expression; chicago extra-layer loop never advances (January:
   element index constant 0, loop test `count > 0`; commented in source as original behaviour).
8. Fog: `short stage_count = FLOOR(stages.count, 1)` (January `jge`); `texscale[2][2] *= PIN(1-intensity,0,1)`
   (January keeps the PIN on x87 and stores once); fade/blend default cases `match_vassert(FALSE)` with no
   combiner_constant assignment (January reloads its home); stage colour lerp through a
   `real_argb_color` delta computed first; external colour guard `< NUMBER_OF_SHADER_ANIMATION_SOURCES` (5,
   same spelling as `rasterizer_xbox_models.c`).
9. Glass: three `vsh_constants__texscale[3][4]` (tint/reflection/diffuse; January has three 48-byte arrays);
   `> 0.0f` reflection alpha tests; bump-map usage 3; reflection default `FALSE`; efg `flag ? 0x08 : 0x14`.
10. Meter: `1.0f/MAX(gradient*8, 1)`; SRCBLEND/DESTBLEND both keyed on tint_mode_2 (CONSTANTCOLOR :
    CONSTANTALPHA, SRCALPHA : CONSTANTCOLOR); `rgb_inputs[2]` `flag ? 0xE0 : 0`; animation sources
    `>= 1 && <= 4`; tint-mode-2 block reads meter/background transparency before the pixel calls and each
    branch ends with its own tint_pixel call (January tail-merges it).
11. groups2 loop indexes `groups2[group_index]` directly (January non-destructive `lea`).

## 5. Experiment matrix (summary; per-step log in `scratch/workers/rasterizer_xbox_transparent_geometry.md`)

| Step | Size / relocs | Masked blocks | Note |
| --- | --- | --- | --- |
| baseline v0 | 15,072 / 888 | 343 | |
| v1 macros only | 15,168 / 903 | - | 8 plane calls (MIN/MAX re-evaluation) |
| v2 single-evaluation fog | 14,960 / 875 | 343 | all call relocs equal |
| v3-v9 debug/cortana/widget/effect | 15,024 / 874 | 300 | |
| v10-v13 generic/chicago loops | **15,120** / 874 | 222 | size equal |
| v14-v18 fog/glass/meter decodes | 15,104 / **872** | 138 | reloc multiset equal (literals aside) |
| p19-p21 glass arrays, meter locals | 15,120 / 872 | 105 | frame 0x558 |
| e11b-e14 skinning per block + vector2d scales + cortana/model/multipass/groups2 | 15,104 / 872 | 81 | **frame 0x550, 0 slot mismatches** |
| n47 inline function index | 15,104 / 872 | **60** | best.c |

Measured inert (do not repeat): variable names; declaration order within a block; moving the map scales
between inner block and arm top; skinning at function scope or debug-arm + else-arm top; 1-6 extra function
declarations before the function; lerp operand order, ternary `function_value`, inner-block delta, add order;
glass detail-scale operand order; `(short)` cast or decl/init split of the numeric function index;
`if (success) success = create()` (worse); `maps.count` without `> 0` (emits `je`, wrong).

## 6. Residual classification (candidate best.c)

- Measured facts: equal relocation multiset, equal frame and slot map; 4,105 vs 4,111 instructions. The
  16-byte padded-size gap comes from the tie blocks plus loop-head alignment pads (`jmp`/`lea` fillers at
  0x12cb, 0x14aa, 0x1e79, 0x3a1d) that follow code addresses; jump-table entries differ only by address.
- Remaining 60 masked blocks are register / x87 / emission-order ties: skinning store tail-merge chosen in
  the cortana block in January but in the no-queue block in ours; numeric function-index register roles and
  16-bit width; stage-lerp x87 keep-vs-consume; chicago numeric `setne`/`sub esp` order; glass diffuse detail
  commutative load order; meter pixel-shader constant-register store order; zero-register pretest in the
  generic map loop (see memory `zero-register-pinning-residual`). Classification: scheduling/register ties
  (inference: dag.c emission order and definition-position-driven allocation), not missing logic.
- The two assert literals are an ownership/naming boundary requiring the header proposal below.

## 7. ORCHESTRATOR-PROPOSAL (header; not applied by this worker)

`source/shaders/shader_definitions.h`: split `struct shader_base` into the HCEA/DB-named sub-structs
(offsets unchanged, 0x28 total):

```c
struct shader_radiosity_properties
{
	unsigned short flags;
	short detail_level;
	real power;
	real_rgb_color color_of_emitted_light;
	real_rgb_color tint_color;
};

struct shader_physics_properties
{
	unsigned short flags;
	short material_type;
};

struct shader_base
{
	struct shader_radiosity_properties radiosity;
	struct shader_physics_properties physics;
	short type;
	short pad;
};
```

Evidence: January `.rdata` literals `!TEST_FLAG(shader_transparent_generic->shader.radiosity.flags, ...) ||
shader_transparent_generic->generic.type==_shader_transparent_generic_type_2d_map` (194 bytes) and the chicago
twin; HCEA `_shader.h` (`radiosity`/`physics`/`type`); January `shader->base.type` literals in
shader_definitions.obj and rasterizer_xbox_models.obj stay valid. Required source follow-ups:
`source/shaders/shaders.c` lines 298 and 332 `shader->base.flags` -> `shader->base.radiosity.flags`, and
this TU's generic/chicago definitions become `{ struct shader_base shader; struct shader_transparent_generic
generic; }` / `{ ...; struct shader_transparent_chicago chicago; }` (already done in `best_with_header.c`).
Measured with a shadow copy (`scratch/workers/rasterizer_xbox_transparent_geometry/fash/`): code bytes
identical, both literal relocations become equal. Direct consumers (all must be re-gated, declaration-count
sensitivity is real): contrail_definitions.h, decals.c, particle_system_definitions.h, particles.h,
weather_particle_systems.c, game_engine_king.c, models.c, object_lights.c, antenna.c, flags.c, lightning.c,
rasterizer_transparent_geometry.c, rasterizer_xbox_active_camouflage.c, rasterizer_xbox_dynavobgeom.c,
rasterizer_xbox_environment.c, rasterizer_xbox_environment_fog.c, rasterizer_xbox_models.c,
rasterizer_xbox_plasma_energy.c, rasterizer_xbox_shadows.c, rasterizer_xbox_transparent_geometry.c,
rasterizer_xbox_water.c, shader_transparent_chicago_preprocessor.c, shader_transparent_generic_preprocessor.c,
render_objects.c, render_sprite.h, shader_definitions.c, shaders.c, structure_lens_flares.c,
structure_render.c, structure_visibility.c (plus transitive includers of render_sprite.h / particles.h /
contrail_definitions.h).

## 8. Reopen criteria

Reopen when (a) the header proposal is applied (or rejected), and (b) a new lever for C2 register /
emission-order ties is authenticated (e.g. a same-compiler donor showing the cortana-vs-main skinning
tail-merge choice, the function-index 16-bit register pattern, or the stage-lerp x87 keep pattern), or an
automated natural-knob lottery starting from `best.c` is budgeted. Start from `best.c`/`best_with_header.c`,
not from the production file.

## 9. Disposition

Still residual, zero exact credit. Production source untouched. No park entry created.
