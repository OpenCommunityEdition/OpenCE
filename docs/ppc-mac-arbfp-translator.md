# NV2A pixel combiner to ARBfp1.0 translator

`port/mac/mac_nv2a_arbfp.{h,c}` turns the NV2A pixel state of a draw (texture
stage modes, up to eight general combiner stages, the final combiner, the
alpha test, fog) into an `ARBfp1.0` program, as the counterpart of the
vertex translator (`docs/ppc-mac-arb-translator.md`). It computes what the
shared GLSL generator `port/linux/src/nv2a_psh.c` computes for the same state,
as a fragment program that is cheaper for the renderer to select and to feed
constants (`glProgramEnvParameter4fvARB` costs a sixth of a changed GLSL
uniform on the Radeons, see `build/mac-audit/draw-overhead/FINDINGS.md`) and
does less work per pixel (`build/mac-audit/fragment-cost/FINDINGS.md`: the
generated GLSL evaluates every stage fully). It is pure text generation, C89,
and calls nothing of GL. Nothing in the renderer uses it yet.

## API

```c
int mac_nv2a_arbfp_translate(const struct nv2a_pixel_shader_key *key,
    const struct mac_nv2a_arb_layout *layout, unsigned long projective_mask,
    char **arb_out, char *error, size_t error_size);
int mac_nv2a_arbfp_translate_info(..., struct mac_nv2a_arbfp_info *info, ...);
int mac_nv2a_arbfp_translate_state(const struct mac_nv2a_arbfp_state *state,
    ...same..., info, ...);   /* no engine headers needed */
```

Each returns 1 with a malloc'd program in `*arb_out`, 0 when the state is
not supported (`error` says why; the caller keeps the GLSL program), -1 on
allocation failure or a bad argument. The key is the renderer's own
(`struct nv2a_pixel_shader_key`, `mac_nv2a_draw_api.h`). The color constants
in it are not read: they are environment parameters (so one program serves
every constant), exactly as the GLSL source cache zeroes them in its key.
`mac_nv2a_arbfp_translate_state` takes the same fields already picked out
(`struct mac_nv2a_arbfp_state`) and needs no engine headers; build
`mac_nv2a_arbfp.c` with `-DMAC_NV2A_ARBFP_STANDALONE` to get only that.

* `layout` is the vertex program's (`mac_nv2a_arb_translate`), or NULL for no
  fog. The fragment reads the varyings where the ARB vertex programs write
  them: `fragment.texcoord[2..5]` for T0..T3, `[6]` and `[7]` for D0 and D1,
  and oFog in `layout->fog_texcoord`/`fog_component`.
* `projective_mask`: bit n set means texture stage n's `texture_scale` is
  (1,1) (every texture but borrowed render targets), so a 2D `project2d`
  stage is a single `TXP` from `fragment.texcoord[2+n]`. Without the bit the
  stage divides by w (guarded as the GLSL guards it) and multiplies by
  `program.env[21+n]`. A 3D texture is always `TXP`. The one difference:
  `TXP` with w exactly 0 divides by zero where the GLSL divides by 1.
* `info` (optional): ALU and texture instruction counts, the temporaries,
  which `program.env[n]` the program reads (`env_used[n]`: upload only those)
  and whether it can `KIL`.

## Environment parameters

`glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, n, v)`; all colors are
rgba in [0, 1] as `color_to_vec4` makes them (the range analysis relies on it).

| `program.env[n]` | Contents (the GLSL uniform) |
| --- | --- |
| 0..7 | `ps_c0[stage]` (all stages read `ps_c0[0]` unless the combiner count has bit 0x1000) |
| 8..15 | `ps_c1[stage]` (bit 0x10000 of the count) |
| 16, 17 | `ps_final_c0`, `ps_final_c1` |
| 18 | `fog_color` |
| 19 | `fog_parameters`: x start, y end, z density |
| 20 | x: `alpha_reference` as a number 0..255 |
| 21..24 | `texture_scale[stage]` (xy) |
| 25..28 | `bump_matrix[stage]` |
| 29..32 | `bump_luminance[stage]` |

`MAC_NV2A_ARBFP_ENV_COUNT` is 33; `GL_MAX_PROGRAM_ENV_PARAMETERS_ARB` is 128 on
the three test machines. The Radeons' `GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB` is
32: the driver counts the environment parameters a program reads plus its
distinct inline constants, so a state that reads many of both can be refused
by `GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB` (the renderer keeps GLSL for it).

## Supported states

| Part | Supported | Refused (returns 0) |
| --- | --- | --- |
| Texture modes | none, project2d, project3d (2D and 3D samplers), cubemap (any sampler), passthru, clipplane, bumpenvmap, bumpenvmap_luminance, dot_st, dot_zw, dot_product, dot_reflect_diffuse, dot_reflect_specular, dot_reflect_specular_constant, dot_str_3d, dot_str_cube, dependent_ar, dependent_gb; all dot mappings and input textures | brdf and undefined modes; project on a cube map; bump and dot modes in stage 0 |
| Combiners | all input mappings, output mappings, dot products (`vec3(dot)`), both mux forms (r0.a >= 0.5 and the low bit), the mirrored blue-to-alpha flags, unique constants per stage, the final combiner with v1r0_sum and ef_product | |
| Output | alpha test with all seven functions (and never), fog (none/linear/exp/exp2, vertex-supplied or constant), `coverage_alpha` | `alpha_kill`, `color_sign`, `count_samples`, `point_threshold`, `per_pixel_lighting`, `alpha_test_samples`, `source_blend` (none of them is set by the legacy Mac renderer) |

## How it translates

The program is built as single-assignment values with their ranges, then
allocated:

* **Views and ranges.** An operand is a view of a register, parameter,
  varying or temporary (sign, component) with the range of the values it can
  hold. Inputs: texture samples, `v0`/`v1` (the vertex program clamps B0, B1,
  D0, D1 to [0,1] before they leave) and the colors are in [0,1]; the zero
  register and registers no stage wrote are the constant 0; the combiner
  clamps every output to [-1,1].
* **Folding.** A mapping of a known value is evaluated, a product by 0, 1 or
  -1 vanishes, `A*B + C*D` is a `MUL` and a `MAD`, an unused output is not
  computed. A clamp the range makes a no-op is not written.
* **Pending clamps.** A combiner output that may leave [-1,1] is stored
  unclamped and the clamp is pending. Its consumers mostly want a one-sided
  clamp, which is one saturating instruction on the stored value:
  `max(clamp(x,-1,1), 0) = sat(x)` and `1 - sat(clamp(x,-1,1)) = sat(1 - x)`
  (`ADD_SAT`). The full clamp (`MAX` and `MIN`) is written only for a signed
  consumer, and only for the sides the range can exceed.
* **Dead code.** What nothing reads (a stage output overwritten before use, a
  texture fetch no stage reads) is dropped. The last `MOV_SAT` to
  `result.color` is folded into its producer (`MUL_SAT result.color.xyz, ...`).
* **Temporaries.** A vector (xyz) and a scalar (w) value share one ARB
  temporary; the allocation is by live range.
* **Alpha test.** With a8 = floor(sat(a) * 255 + 0.5) and the integer
  reference R, every comparison is the sign of a difference for `KIL`: the
  strict ones move the half step into the floor (`floor(q - 0.5) - R` for
  greater, `R - floor(q + 1.5)` for less; equal and not-equal take
  `0.5 - |a8 - R|` and `|a8 - R| - 0.5`).
* **Fog slot.** The vertex program carries oFog in a component of a texcoord
  slot that it never writes; the GLSL prelude hides it from the shader by
  reading it as the default of an unwritten component, (0,0,0,1). The same is
  done here: a reader of that component gets the constant, a vector reader of
  that slot a cleaned copy (two instructions, only if the slot is read).

## What the renderer must do

1. Per draw, build the key as today. If the vertex program is an ARB one and
   `mac_nv2a_arbfp_translate_info` returns 1 for the key (cache the result,
   with the program id and `GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB`, by the key
   words without the color constants, the `projective_mask`, and the
   layout's fog slot and component), use it; otherwise keep the GLSL
   program. A program is created once with `glGenProgramsARB`,
   `glBindProgramARB`, `glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, ...)`.
   After the first bind and draw, check
   `glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB)`
   (the Radeons' limits are 64 ALU, 32 TEX, 4 indirections, 32 parameters) and
   keep GLSL for a program over them.
2. For a draw with an ARB fragment program: `glUseProgram(0)`,
   `glEnable(GL_FRAGMENT_PROGRAM_ARB)`, `glBindProgramARB`. For a draw with
   GLSL: `glDisable(GL_FRAGMENT_PROGRAM_ARB)` and `glUseProgram`. The ARB
   vertex program stays enabled for both.
3. Environment parameters: upload with `glProgramEnvParameter4fvARB` only
   those with `info.env_used[n]` set, and only when the value changed since
   the last upload (they are global to the context, not per program, so
   nothing is re-sent on a program switch): c0/c1 as `mac_gpu_upload_lean_uniforms`
   has them (`color_to_vec4`), final c0/c1, fog color, fog parameters (start,
   end, density), env 20 x = `D3DRS_ALPHAREF & 255` as a float, the texture
   scales of the stages the program reads, and the bump matrices and
   luminances when a bump mode is in use.
4. Texture unit n holds stage n's texture, bound on the target of
   `key.sampler_type[n]` (2D, 3D or cube map). No sampler uniforms. The
   target of `TEX`/`TXP` must match the bound texture.
5. The alpha test is in the program (`KIL`): the fixed-function alpha test
   must stay off. Fog is in the program: fixed-function fog stays off.
6. Pass `projective_mask` with bit n set for every stage whose
   `texture_scale[n]` is exactly (1,1) (what `mac_arb_projective_fetches`'
   mask is today); the program then does not read `env[21+n]` for a 2D
   project stage. Upload `texture_scale` for the other stages.

## Checks

* `tools/mac_nv2a_arbfp_checks.sh` (host, no GPU): the 46 pixel shaders of the
  seeded frame are regenerated from the keys recovered by
  `tools/mac_arbfp_keys_from_glsl.py` (`tools/mac_nv2a_arbfp_keys.txt`) and
  compared with the dumped GLSL, translated with all stages projective and
  with none, and every program is run through a validator of the ARBfp1.0
  subset the translator writes (statements, operand counts, declared
  temporaries, write masks, swizzles, and that every component an
  instruction reads has been written). Also every texture mode, fog mode,
  alpha test, output mapping and mux/dot flag, the refusals, and 4000 random
  combiner states.
* `tools/mac_nv2a_arbfp_gpu_checks.sh` (GPU): the probe draws the same
  varyings, constants and textures through the passthrough ARB vertex
  program with the GLSL fragment program (`nv2a_pixel_shader_to_glsl`,
  `mac_glsl120_pixel`, `mac_nv2a_arb_fragment_rewrite`: the renderer's) and
  with the ARB one, into a 64x64 target cleared to a sentinel, four
  scenarios per state (scale 1 with every stage projective; other scales
  with none), and compares the 8-bit pixels. It also reports the native
  instruction counts and whether the program is under the native limits, and
  with `time` the cost of a full-screen quad. A CPU reference of the
  generator's semantics for states with no fetches or with 2D fetches
  says which program is off when they disagree. Results:
  `build/mac-audit/arbfp/PROVENANCE.md`.
