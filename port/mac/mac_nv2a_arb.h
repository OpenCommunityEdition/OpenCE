#ifndef HALO_MAC_NV2A_ARB_H
#define HALO_MAC_NV2A_ARB_H

#include <stddef.h>

/* NV2A vertex program -> ARBvp1.0 translator for the legacy Mac renderer.

   The program text is generated from the same Xbox vertex-program words the
   shared GLSL translator (port/linux/src/nv2a_vsh.c) reads, and computes the
   same results, so that a shader can run in the GPU's vertex unit where
   Apple's GLSL compiler would send it to a software path. It is pure text
   generation: no GL calls, no game state.

   Environment parameters (program.env[n], set with glProgramEnvParameter4f):
       0..191  the game's vertex constants c[0..191]
       192     viewport_scale   (the GLSL uniform of the same name)
       193     viewport_offset  (the GLSL uniform of the same name)
       194     x: screen_offset, y: point_size (the GLSL uniforms)
   MAC_NV2A_ARB_ENV_COUNT (195) parameters are used by every program.
   ARB_vertex_program guarantees only 96; Apple's R300 driver reports more.
   Callers compare GL_MAX_PROGRAM_ENV_PARAMETERS_ARB with the count and keep
   the GLSL path when it is smaller.

   Inputs: NV2A v0..v15 are vertex.attrib[0..15]. With bone gather the six
   relative constant reads become vertex.attrib[10..15], as
   mac_bone_gather.c's rewrite does for GLSL; with MAC_NV2A_ARB_RELATIVE
   they read program.env[A0.x + k] and attributes 10..15 are not used.

   Outputs (the varying slots; see docs/ppc-mac-arb-translator.md):
       result.texcoord[0]  oB0 clamped to [0,1]   (GLSL xB0)
       result.texcoord[1]  oB1 clamped to [0,1]   (GLSL xB1)
       result.texcoord[2]  oT0                    (GLSL xT0)
       result.texcoord[3]  oT1                    (GLSL xT1)
       result.texcoord[4]  oT2                    (GLSL xT2)
       result.texcoord[5]  oT3                    (GLSL xT3)
       result.texcoord[6]  oD0 clamped to [0,1]   (GLSL xD0)
       result.texcoord[7]  oD1 clamped to [0,1]   (GLSL xD1)
       result.pointsize.x  oPts.x
       result.position     the screen-space conversion undone, as in GLSL,
                           with the Mac depth remap z = 2z - w applied
   oFog.x (GLSL xFog) is not clamped by any ARB result register that is free
   (result.color is clamped, result.fogcoord is clamped by some drivers), so
   it travels in a component of a texcoord slot that the program never
   writes: that component's value is the constant default (0,0,0,1)[c],
   which the fragment side restores. The layout says which. A program that
   never writes oFog needs no slot: xFog is the constant 1.0. */

#define MAC_NV2A_ARB_ENV_VIEWPORT_SCALE 192
#define MAC_NV2A_ARB_ENV_VIEWPORT_OFFSET 193
#define MAC_NV2A_ARB_ENV_SCREEN_POINT 194 /* x screen_offset, y point_size */
#define MAC_NV2A_ARB_ENV_COUNT 195
#define MAC_NV2A_ARB_MAX_INSTRUCTIONS 136 /* the NV2A's program memory */

struct mac_nv2a_arb_layout {
    unsigned env_count;      /* parameters program.env[0..env_count-1] to set */
    int bone_gather_used;    /* 1: attributes 10..15 carry the gathered rows;
                                the caller applies the bone gather plan */
    int relative_used;       /* 1: the program addresses constants with A0
                                (MAC_NV2A_ARB_RELATIVE); no gather, no extra
                                attributes, env 0..191 must hold all of c[] */
    int fog_texcoord;        /* texcoord slot carrying oFog.x, or -1 when the
                                program never writes oFog (xFog is 1.0) */
    int fog_component;       /* 0..3 (x, y, z, w) within that slot */
    unsigned skin_base;      /* port: a skinning program (the bone gather's pattern) under
                                MAC_NV2A_ARB_RELATIVE: the first and last offset of its
                                relative reads (60 and 62), else 0 */
    unsigned skin_top;
};

/* The bone_gather argument of mac_nv2a_arb_translate:
     0                       any relative constant read is refused;
     1                       bone gather: the program must match the gather
                             pattern and the six reads become attributes
                             10..15 (layout.bone_gather_used);
     MAC_NV2A_ARB_RELATIVE   native relative addressing, any program (below). */
#define MAC_NV2A_ARB_RELATIVE 2
/* Native relative addressing as MAC_NV2A_ARB_RELATIVE, with the outputs laid
   out for a fixed-function fragment pipeline (HALO_MAC_FF_COMBINERS): T0..T3
   in result.texcoord[0..3] (unclamped), D0 and D1 in result.color.primary and
   .secondary (clamped to [0,1]); B0, B1 and the fog are not written. */
#define MAC_NV2A_ARB_RELATIVE_FF 3
/* port: the mode with (window << 8): a skinning program (layout.skin_base != 0 from the plain
   translation) whose relative reads span program.env[skin_base .. skin_base + window - 1]
   alone; every address of the draw must lie inside (the caller bounds them), else the results
   differ from the plain program's. Refused for any other program. */

/* Translate one program of instruction_count instructions (four 32-bit words
   each, in the low bits of each unsigned long). Translation stops at the
   instruction with the final bit set, as the GLSL translator does.

   Returns 1 with a malloc'd ARBvp1.0 program in *arb_source_out (and the
   count of environment parameters and the layout filled in), 0 when the
   program uses something this translator does not support (error says what;
   the caller keeps the GLSL path), or -1 on allocation failure or a bad
   argument.

   MAC_NV2A_ARB_RELATIVE: every NV2A ARL becomes
       ADD t, src.x, 0.001; MAX/MIN t (bounds below); ARL A0.x, t;
   (the GLSL's a0 = int(floor(mac.x + 0.001))), A0 starts at 0 as the GLSL's
   a0 does, and a relative constant read c[a0 + k] becomes cr[A0.x + k] with
   PARAM cr[192] = { program.env[0..191] } (ARB allows a relative address
   only into a declared parameter array). The GLSL clamps each read's index to 0..191; the
   float loaded into A0 is instead held in [-kmin, 191 - kmax] over the
   offsets k of the program's relative reads, so the results are the same
   for every address that keeps all reads inside c[0..191] (the bone
   addresses of valid data) and every read stays inside the environment
   parameters for any other address (an address beyond the range selects
   other rows than the GLSL's per-read clamp; NaN is not defined). ARB allows
   an immediate offset of -64..63: when a program's highest k exceeds 63 its
   lowest is added to the ARL source and subtracted from every immediate;
   offsets more than 63 apart are refused. Needs env parameters 0..191
   (all of c[]), no vertex attributes beyond the program's own v0..v15.
   With bone_gather 1 a program that has relative constant reads must match
   the bone gather pattern exactly or it is refused; with it zero any
   relative constant read is refused. A program that lights per
   pixel (the GLSL translator's lighting argument) is outside this
   translator's scope: the caller must not offer it. layout_out and
   env_parameter_count_out may be NULL. */
int mac_nv2a_arb_translate(const unsigned long *instructions,
    unsigned long instruction_count, int bone_gather, char **arb_source_out,
    unsigned *env_parameter_count_out, struct mac_nv2a_arb_layout *layout_out,
    char *error, size_t error_size);

/* GLSL 1.20 lines that map the fragment shader's varying names to the ARB
   program's texcoord slots: #define lines for xD0, xD1, xB0, xB1, xT0..xT3
   and xFog. Returns 1, or 0 when out_size is too small. */
int mac_nv2a_arb_fragment_prelude(const struct mac_nv2a_arb_layout *layout,
    char *out, size_t out_size);

/* Rewrite a generated GLSL 1.20 fragment shader (mac_glsl120_pixel output):
   drop the `varying` declarations of the eight colors/coordinates and the
   fog and put the prelude after the #version line. Returns 1 with a malloc'd
   source, 0 if the shader declares any other varying (the per-pixel lighting
   shaders' xWorldNormal and xWorldPosition cannot be carried: all eight
   texcoord slots are in use), or -1 on allocation failure. */
int mac_nv2a_arb_fragment_rewrite(const char *fragment_source,
    const struct mac_nv2a_arb_layout *layout, char **rewritten_out,
    char *error, size_t error_size);

#endif
