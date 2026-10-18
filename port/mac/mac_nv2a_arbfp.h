#ifndef HALO_MAC_NV2A_ARBFP_H
#define HALO_MAC_NV2A_ARBFP_H

#include <stddef.h>
#include "mac_nv2a_arb.h"

/* NV2A pixel combiner state -> ARB_fragment_program (ARBfp1.0) translator
   for the legacy Mac renderer.

   It computes what the shared GLSL generator (port/linux/src/nv2a_psh.c)
   computes for the same state, as a program for the GPU's fragment unit that
   is cheaper to switch to and to feed constants (glProgramEnvParameter4f is
   about a sixth of a changed GLSL uniform's cost on the Radeons) and that
   does less work per pixel: everything the state fixes is folded when the
   program is written. It is pure text generation (no GL calls). It reads the
   fragment varyings from the texcoord slots that the ARB vertex programs
   write (struct mac_nv2a_arb_layout): B0 B1 texcoord 0 1, T0..T3 texcoord
   2..5, D0 D1 texcoord 6 7, and oFog in the layout's slot and component.
   See docs/ppc-mac-arbfp-translator.md.

   Constants: program.env[n] (glProgramEnvParameter4fvARB with
   GL_FRAGMENT_PROGRAM_ARB), a fixed layout so that the renderer sets each
   only when it changes. All colors are the GLSL uniforms' values: rgba in
   [0,1] (the range analysis relies on that, see the document). */
#define MAC_NV2A_ARBFP_ENV_C0 0             /* 0..7   ps_c0[stage]            */
#define MAC_NV2A_ARBFP_ENV_C1 8             /* 8..15  ps_c1[stage]            */
#define MAC_NV2A_ARBFP_ENV_FINAL_C0 16      /* ps_final_c0                    */
#define MAC_NV2A_ARBFP_ENV_FINAL_C1 17      /* ps_final_c1                    */
#define MAC_NV2A_ARBFP_ENV_FOG_COLOR 18     /* fog_color                      */
#define MAC_NV2A_ARBFP_ENV_FOG_PARAMETERS 19 /* x start, y end, z density     */
#define MAC_NV2A_ARBFP_ENV_ALPHA_REFERENCE 20 /* x: the reference, 0..255     */
#define MAC_NV2A_ARBFP_ENV_TEXTURE_SCALE 21 /* 21..24 texture_scale[stage]    */
#define MAC_NV2A_ARBFP_ENV_BUMP_MATRIX 25   /* 25..28 bump_matrix[stage]      */
#define MAC_NV2A_ARBFP_ENV_BUMP_LUMINANCE 29 /* 29..32 bump_luminance[stage]  */
#define MAC_NV2A_ARBFP_ENV_COUNT 33

/* The state a pixel shader depends on: the fields of struct
   nv2a_pixel_shader_key (port/linux/src/xgpu.h) that the translation reads,
   with the combiner words already picked out. The color constants are not
   part of it (they are env parameters). */
struct mac_nv2a_arbfp_state {
    unsigned long rgb_inputs[8], alpha_inputs[8];   /* D3DRS_PSRGBINPUTS0+n, PSALPHAINPUTS0+n   */
    unsigned long rgb_outputs[8], alpha_outputs[8]; /* D3DRS_PSRGBOUTPUTS0+n, PSALPHAOUTPUTS0+n */
    unsigned long combiner_count;       /* D3DRS_PSCOMBINERCOUNT (count and flags)     */
    unsigned long final_abcd, final_efg; /* D3DRS_PSFINALCOMBINERINPUTSABCD, ...EFG    */
    unsigned long compare_mode, dot_mapping, input_texture; /* D3DRS_PSCOMPAREMODE, PSDOTMAPPING, PSINPUTTEXTURE */
    unsigned long texture_modes;        /* D3DRS_PSTEXTUREMODES                        */
    unsigned char sampler_type[4];      /* _xgpu_sampler_none/2d/3d/cube = 0..3        */
    unsigned char alpha_kill[4], color_sign[4];
    unsigned long alpha_test_function;  /* D3DCMP_*, or 0 when disabled                */
    unsigned char fog_enable, fog_table_mode;
    unsigned char count_samples, coverage_alpha, point_threshold, per_pixel_lighting;
    unsigned char alpha_test_samples, source_blend;
};

/* What the program uses, so that the renderer can upload only that. */
struct mac_nv2a_arbfp_info {
    unsigned alu_instructions;   /* ARB instructions other than TEX/TXP/KIL      */
    unsigned tex_instructions;   /* TEX and TXP                                  */
    unsigned temporaries;        /* TEMP registers declared                      */
    unsigned char env_used[MAC_NV2A_ARBFP_ENV_COUNT]; /* 1: program.env[n] is read */
    unsigned char uses_kil;      /* alpha test, clip plane or kill: needs the alpha-test state */
};


/* ---------- fixed-function combiners (HALO_MAC_FF_COMBINERS)

   A pixel shader whose instructions GL_ARB_texture_env_combine (with the
   crossbar, and ATI_texture_env_combine3 for a * b + c) can express exactly
   is also described as texture units' combiner state, so that a draw needs no
   fragment program (the vertex program, MAC_NV2A_ARB_RELATIVE_FF, supplies the
   coordinates in texcoord[0..3] and the diffuse colour as the primary colour).
   Stage n runs on texture unit n; the red, green and blue channels (rgb) and
   alpha are separate pipelines. The result of every stage is clamped to [0,1]
   as the combine unit does; the lowering accepts a shader only if no value it
   uses outside the saturating instructions leaves [0,1]. */
#define MAC_FF_MAX_STAGES 4
enum { MAC_FF_REPLACE, MAC_FF_MODULATE, MAC_FF_ADD, MAC_FF_SUBTRACT, MAC_FF_INTERPOLATE, MAC_FF_MODULATE_ADD };
/* sources: 0..3 texture unit, then: */
enum { MAC_FF_SRC_PRIMARY = 4, MAC_FF_SRC_CONSTANT = 5, MAC_FF_SRC_PREVIOUS = 6 };
/* operands */
enum { MAC_FF_OP_COLOR, MAC_FF_OP_ONE_MINUS_COLOR, MAC_FF_OP_ALPHA, MAC_FF_OP_ONE_MINUS_ALPHA };
/* a stage constant's part: none, a literal, or program.env[env] (rgb: .xyz; alpha: .w) */
enum { MAC_FF_CONST_NONE, MAC_FF_CONST_LITERAL, MAC_FF_CONST_ENV };
struct mac_nv2a_ff_stage {
    unsigned char rgb_function, a_function;
    unsigned char rgb_source[3], rgb_operand[3], a_source[3], a_operand[3];
    unsigned char rgb_const_kind, a_const_kind, rgb_env, a_env;
    unsigned char rgb_scale, a_scale;   /* 1, 2 or 4, applied before the clamp */
    float rgb_literal, a_literal;
};
struct mac_nv2a_ff_program {
    int ok;                         /* 1: expressible */
    unsigned stage_count;           /* texture units 0..stage_count-1 run */
    struct mac_nv2a_ff_stage stage[MAC_FF_MAX_STAGES];
    int alpha_test;                 /* 1: kept where alpha >= (ref - alpha_offset) / 255 */
    float alpha_offset;
    int needs_combine3;             /* uses MODULATE_ADD (ATI_texture_env_combine3) */
    unsigned char unit_2d;          /* bit n: the shader samples unit n's 2D texture */
};
/* 1 (default 0): a stage may read the texture of a later unit. */
void mac_nv2a_arbfp_set_ff_forward(int on);
/* Describe the state's pixel shader as combiner stages. Returns 1 with out->ok
   set, 0 when it is not expressible (error says why), -1 on a bad argument
   or allocation failure. */
int mac_nv2a_arbfp_lower_fixed(const struct mac_nv2a_arbfp_state *state,
    const struct mac_nv2a_arb_layout *layout, unsigned long projective_mask,
    struct mac_nv2a_ff_program *out, char *error, size_t error_size);

/* Translate. Returns 1 with a malloc'd ARBfp1.0 program in *arb_out, 0 when
   the state uses something this translator does not support (error says
   what; the caller keeps the GLSL program), -1 on allocation failure or a
   bad argument.

   projective_mask: bit n set = stage n's texture_scale is (1,1), so a 2D
   project2d/project3d stage reads TXP straight from fragment.texcoord (no
   scale uniform, no divide-by-zero guard: a w of exactly 0 samples where the
   GLSL's samples with w taken as 1). Without the bit, the stage divides by w
   and multiplies by program.env[TEXTURE_SCALE + n].

   Texture unit n samples stage n's texture with the target of
   sampler_type[n] (2D, 3D or CUBE). The layout is the vertex program's
   (NULL: no fog, xFog is 1). The alpha-test reference is
   program.env[ALPHA_REFERENCE].x. info may be NULL. */
int mac_nv2a_arbfp_translate_state(const struct mac_nv2a_arbfp_state *state,
    const struct mac_nv2a_arb_layout *layout, unsigned long projective_mask,
    char **arb_out, struct mac_nv2a_arbfp_info *info, char *error, size_t error_size);

/* The same from the renderer's key (struct nv2a_pixel_shader_key, which
   needs the engine's headers; compile mac_nv2a_arbfp.c without
   MAC_NV2A_ARBFP_STANDALONE). */
struct nv2a_pixel_shader_key;
int mac_nv2a_arbfp_translate(const struct nv2a_pixel_shader_key *key,
    const struct mac_nv2a_arb_layout *layout, unsigned long projective_mask,
    char **arb_out, char *error, size_t error_size);
int mac_nv2a_arbfp_translate_info(const struct nv2a_pixel_shader_key *key,
    const struct mac_nv2a_arb_layout *layout, unsigned long projective_mask,
    char **arb_out, struct mac_nv2a_arbfp_info *info, char *error, size_t error_size);

/* The state from a key (no GL, no translation). */
void mac_nv2a_arbfp_state_from_key(const struct nv2a_pixel_shader_key *key,
    struct mac_nv2a_arbfp_state *state);

#endif
