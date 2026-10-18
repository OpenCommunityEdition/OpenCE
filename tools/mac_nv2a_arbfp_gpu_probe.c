/* GPU comparison of the NV2A pixel shader -> ARBfp1.0 translator (port/mac/mac_nv2a_arbfp.c) against
   the GLSL fragment program the renderer uses today (nv2a_pixel_shader_to_glsl, mac_glsl120_pixel,
   mac_nv2a_arb_fragment_rewrite: the fragment-only program paired with an ARB vertex program).

   Both fragment programs read the same interpolated texcoord slots, constants and textures, drawn
   by the same ARB vertex program (a pass-through that writes result.texcoord[0..7]) over a quad,
   into a 64x64 RGBA8 target that is first cleared to a sentinel colour, so a discarded fragment
   shows as the sentinel in both. Per pixel and channel the difference of the 8-bit results is
   compared (a coverage difference is a large one).

   Subjects: the pixel shaders of the real frame (tools/mac_nv2a_arbfp_keys.txt: 46 shaders) in four
   scenarios each (texture_scale 1 with every stage projective, then scales other than 1 with none
   projective; new varyings, constants and textures each), a family of synthetic states (every
   alpha-test function, fog mode, texture mode the frame does not use, output mapping), and random
   combiner words. Also reported: the ARB program's native ALU/TEX/indirection counts and whether it
   is under the native limits, and with `time` the cost of a full-screen quad for both programs.

   usage: mac_nv2a_arbfp_gpu_probe [keys.txt] [frame] [synthetic] [random=N] [time] [tolerance=N]
          [only=shader_0037] [dump=dir]
   Hidden CGL context only (no window). See tools/mac_nv2a_arbfp_gpu_checks.sh. */
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include "../port/mac/mac_nv2a_shader_api.h"
#include "../port/mac/mac_glsl120.h"
#include "../port/mac/mac_nv2a_arb.h"
#include "../port/mac/mac_nv2a_arbfp.h"
#undef memset
#undef memcpy
#undef memcmp
#undef malloc
#undef realloc
#undef free
#undef strcmp
#undef strlen
#undef strcpy
#undef strcat
#undef sprintf
#undef strncmp
#undef strtok
#undef strncpy
#undef strchr
#undef strstr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mach/mach_time.h>
#include "mac_nv2a_arbfp_keys.h"

#define GL_FRAGMENT_PROGRAM_ARB_ 0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB_ 0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB_ 0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB_ 0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB_ 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB_ 0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB_ 0x880A
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB_ 0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB_ 0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB_ 0x8810
#define GL_PROGRAM_ERROR_POSITION_ARB_ 0x864B
#define GL_PROGRAM_ERROR_STRING_ARB_ 0x8874
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB_ 0x88B6
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB_ 0x88B5
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB_ 0x88A7
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB_ 0x88A6
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB_ 0x88AA

static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); } } while (0)

#define SIZE 64
#define TIME_SIZE 512

static int tolerance = 2;
static int random_cpu = 0;    /* the random states use only what the CPU reference covers (2D fetches) */
static int random_notex = 0;  /* the random states have no texture fetches (the CPU reference covers them) */
static int random_mux = 1;   /* 0: the random states have no mux flag (its test of r0.a flips on rounding) */
static const GLubyte sentinel[4] = { 51, 102, 153, 204 };

/* ---------- random */

static unsigned long rng_state = 1;
static unsigned long rng(void)
{
    rng_state = rng_state * 1664525UL + 1013904223UL;
    return (rng_state >> 8) & 0xffffffUL;
}
static unsigned name_hash(const char *name)
{
    unsigned h = 2166136261u;
    while (*name) h = (h ^ (unsigned char)*name++) * 16777619u;
    return h & 0xffffu;
}
static float frand(float lo, float hi) { return lo + (hi - lo) * ((float)(rng() & 0xffff) / 65535.0f); }
static float color8(void) { return (float)(rng() % 256) / 255.0f; }

/* ---------- GL helpers */

static GLuint compile_glsl(GLenum type, const char *source, char *log, size_t log_size)
{
    GLuint shader = glCreateShader(type);
    GLint ok = GL_FALSE, n = 0;
    log[0] = 0;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        glGetShaderInfoLog(shader, (GLsizei)log_size - 1, &n, log);
        if (n < 0) n = 0;
        log[n] = 0;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint load_arb(GLenum target, const char *text, char *error, size_t error_size)
{
    GLuint id = 0;
    GLint position = -1;
    glGenProgramsARB(1, &id);
    glBindProgramARB(target, id);
    while (glGetError() != GL_NO_ERROR) {}
    glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)strlen(text), text);
    if (glGetError() != GL_NO_ERROR) {
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB_, &position);
        snprintf(error, error_size, "ARB program error at %d: %s", (int)position, (const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB_));
        glDeleteProgramsARB(1, &id);
        return 0;
    }
    return id;
}

static const char vertex_passthrough[] =
    "!!ARBvp1.0\n"
    "MOV result.position, vertex.position;\n"
    "MOV result.texcoord[0], vertex.texcoord[0];\nMOV result.texcoord[1], vertex.texcoord[1];\n"
    "MOV result.texcoord[2], vertex.texcoord[2];\nMOV result.texcoord[3], vertex.texcoord[3];\n"
    "MOV result.texcoord[4], vertex.texcoord[4];\nMOV result.texcoord[5], vertex.texcoord[5];\n"
    "MOV result.texcoord[6], vertex.texcoord[6];\nMOV result.texcoord[7], vertex.texcoord[7];\n"
    "END\n";

/* ---------- textures */

static GLuint tex_2d[4], tex_3d[4], tex_cube[4];
static unsigned char tex_2d_data[4][64 * 64 * 4];   /* what the 2D textures hold (for the CPU reference) */

static void fill_pattern(unsigned char *p, unsigned n, unsigned seed) __attribute__((unused));
static void fill_pattern(unsigned char *p, unsigned n, unsigned seed)
{
    unsigned i;
    rng_state = 77 + seed * 7919;
    for (i = 0; i < n; i++) {
        float a = frand(0.0f, 1.0f);
        p[i * 4 + 0] = (unsigned char)(rng() % 256);
        p[i * 4 + 1] = (unsigned char)(rng() % 256);
        p[i * 4 + 2] = (unsigned char)(rng() % 256);
        p[i * 4 + 3] = (unsigned char)(a * 255.0f);
    }
}

static void smooth(unsigned char *p, unsigned w, unsigned h, unsigned seed)
{
    /* low-frequency colours with sparse alpha: neighbours differ little (bilinear filtering and
       dependent reads stay well-conditioned) */
    unsigned x, y;
    float fx = 1.0f + (float)(seed % 3), fy = 2.0f + (float)(seed % 2), ph = (float)seed;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            float u = (float)x / (float)w, v = (float)y / (float)h;
            unsigned char *q = p + (y * w + x) * 4;
            q[0] = (unsigned char)(127.5f + 127.0f * sinf(6.2831853f * (fx * u + ph)));
            q[1] = (unsigned char)(127.5f + 127.0f * sinf(6.2831853f * (fy * v + 0.3f * ph)));
            q[2] = (unsigned char)(127.5f + 127.0f * sinf(6.2831853f * (u + v) + ph));
            q[3] = (unsigned char)(127.5f + 127.0f * cosf(6.2831853f * (u * fy - v * fx)));
        }
}

/* a periodic pattern (the texture repeats, so no seam jumps between neighbours) */
static void smooth3d(unsigned char *p, unsigned n, unsigned seed)
{
    unsigned x, y, z;
    for (z = 0; z < n; z++)
        for (y = 0; y < n; y++)
            for (x = 0; x < n; x++) {
                float u = (float)x / (float)n, v = (float)y / (float)n, w = (float)z / (float)n, ph = (float)seed;
                unsigned char *q = p + ((z * n + y) * n + x) * 4;
                q[0] = (unsigned char)(127.5f + 127.0f * sinf(6.2831853f * (u + 2.0f * v) + ph));
                q[1] = (unsigned char)(127.5f + 127.0f * sinf(6.2831853f * (v + w) + 2.0f * ph));
                q[2] = (unsigned char)(127.5f + 127.0f * sinf(6.2831853f * (w - u) + 3.0f * ph));
                q[3] = (unsigned char)(127.5f + 127.0f * cosf(6.2831853f * (u + v + w) + ph));
            }
}

static void make_textures(unsigned seed)
{
    unsigned i, f;
    unsigned char *buffer = (unsigned char *)malloc(32 * 32 * 32 * 4);
    for (i = 0; i < 4; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, tex_2d[i]);
        smooth(buffer, 64, 64, seed * 4 + i);
        memcpy(tex_2d_data[i], buffer, 64 * 64 * 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_3D, tex_3d[i]);
        smooth3d(buffer, 16, seed * 4 + i);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 16, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glBindTexture(GL_TEXTURE_CUBE_MAP, tex_cube[i]);
        for (f = 0; f < 6; f++) {
            smooth(buffer, 32, 32, seed * 24 + i * 6 + f);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGBA8, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    free(buffer);
}

/* the unit's texture of the stage's sampler type bound on its target */
static void bind_stage_textures(const struct nv2a_pixel_shader_key *key)
{
    unsigned i;
    for (i = 0; i < 4; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        switch (key->sampler_type[i]) {
        case 2: glBindTexture(GL_TEXTURE_3D, tex_3d[i]); break;
        case 3: glBindTexture(GL_TEXTURE_CUBE_MAP, tex_cube[i]); break;
        default: glBindTexture(GL_TEXTURE_2D, tex_2d[i]); break;
        }
    }
    glActiveTexture(GL_TEXTURE0);
}

/* ---------- one scenario's inputs */

struct inputs {
    float vertex[4][8][4];         /* the quad's corners: texcoord slots 0..7 */
    float ps_c0[8][4], ps_c1[8][4], final0[4], final1[4], fog_color[4], fog_parameters[4];
    float alpha_reference;
    float texture_scale[4][4], bump_matrix[4][4], bump_luminance[4][4];
};

static void random_inputs(struct inputs *in, const struct nv2a_pixel_shader_key *key, int fog_tex, int fog_comp,
    int unit_scale, unsigned seed)
{
    int v, s, c;
    rng_state = 1000 + seed * 104729UL;
    memset(in, 0, sizeof(*in));
    for (v = 0; v < 4; v++) {
        static const float cx[4] = { 0.0f, 1.0f, 1.0f, 0.0f }, cy[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
        float w[4];
        for (s = 0; s < 8; s++)
            for (c = 0; c < 4; c++) in->vertex[v][s][c] = 0.0f;
        /* B0 B1 D0 D1: the vertex program's clamped colours */
        for (c = 0; c < 4; c++) {
            in->vertex[v][0][c] = color8(); in->vertex[v][1][c] = color8();
            in->vertex[v][6][c] = color8(); in->vertex[v][7][c] = color8();
        }
        for (s = 0; s < 4; s++) {
            float *t = in->vertex[v][2 + s];
            int sampler = key->sampler_type[s];
            int mode = (int)((key->texture_modes >> (5 * s)) & 0x1f);
            w[s] = unit_scale ? frand(0.6f, 1.7f) : 1.0f;
            if (sampler == 3 || sampler == 2 || mode >= 5) {
                /* directions and arbitrary vectors */
                t[0] = frand(-1.0f, 1.0f); t[1] = frand(-1.0f, 1.0f); t[2] = frand(-1.0f, 1.0f);
                t[3] = frand(-1.0f, 1.0f);
            } else {
                /* a 2D lookup: texel-space s/q, t/q over the texture, w = q */
                float q = w[s];
                t[0] = (cx[v] * 1.4f + frand(-0.1f, 0.1f)) * q;
                t[1] = (cy[v] * 1.4f + frand(-0.1f, 0.1f)) * q;
                t[2] = frand(0.0f, 1.0f);
                t[3] = q;
                if (mode == 4) { t[0] = frand(-0.5f, 1.5f); t[1] = frand(-0.5f, 1.5f); t[2] = frand(-0.5f, 1.5f); t[3] = frand(-0.5f, 1.5f); }
            }
        }
        if (fog_tex >= 0) in->vertex[v][fog_tex][fog_comp] = frand(-0.2f, 1.3f);
    }
    for (s = 0; s < 8; s++)
        for (c = 0; c < 4; c++) { in->ps_c0[s][c] = color8(); in->ps_c1[s][c] = color8(); }
    for (c = 0; c < 4; c++) {
        in->final0[c] = color8(); in->final1[c] = color8(); in->fog_color[c] = color8();
    }
    in->fog_parameters[0] = frand(0.0f, 0.5f);
    in->fog_parameters[1] = in->fog_parameters[0] + frand(0.3f, 1.5f);
    in->fog_parameters[2] = frand(0.3f, 2.5f);
    in->alpha_reference = (float)(60 + rng() % 140);
    for (s = 0; s < 4; s++) {
        in->texture_scale[s][0] = unit_scale ? 1.0f : frand(0.25f, 1.0f);
        in->texture_scale[s][1] = unit_scale ? 1.0f : frand(0.25f, 1.0f);
        for (c = 0; c < 4; c++) {
            in->bump_matrix[s][c] = frand(-0.4f, 0.4f);
            in->bump_luminance[s][c] = frand(-0.2f, 1.2f);
        }
    }
}

struct glsl_program {
    GLuint program;
    GLint c0, c1, final0, final1, fog_color, fog_parameters, alpha_reference, texture_scale, bump_matrix, bump_luminance;
    GLint tex[4];
};

static GLint uloc(GLuint program, const char *name) { return glGetUniformLocation(program, name); }

static void set_glsl_uniforms(const struct glsl_program *g, const struct inputs *in)
{
    unsigned i;
    glUseProgram(g->program);
    if (g->c0 >= 0) glUniform4fv(g->c0, 8, &in->ps_c0[0][0]);
    if (g->c1 >= 0) glUniform4fv(g->c1, 8, &in->ps_c1[0][0]);
    if (g->final0 >= 0) glUniform4fv(g->final0, 1, in->final0);
    if (g->final1 >= 0) glUniform4fv(g->final1, 1, in->final1);
    if (g->fog_color >= 0) glUniform4fv(g->fog_color, 1, in->fog_color);
    if (g->fog_parameters >= 0) glUniform4fv(g->fog_parameters, 1, in->fog_parameters);
    if (g->alpha_reference >= 0) glUniform1f(g->alpha_reference, in->alpha_reference);
    if (g->texture_scale >= 0) glUniform4fv(g->texture_scale, 4, &in->texture_scale[0][0]);
    if (g->bump_matrix >= 0) glUniform4fv(g->bump_matrix, 4, &in->bump_matrix[0][0]);
    if (g->bump_luminance >= 0) glUniform4fv(g->bump_luminance, 4, &in->bump_luminance[0][0]);
    for (i = 0; i < 4; i++) if (g->tex[i] >= 0) glUniform1i(g->tex[i], (GLint)i);
}

static void set_arb_env(const struct inputs *in)
{
    unsigned i;
    float v[4];
    for (i = 0; i < 8; i++) {
        glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, MAC_NV2A_ARBFP_ENV_C0 + i, in->ps_c0[i]);
        glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, MAC_NV2A_ARBFP_ENV_C1 + i, in->ps_c1[i]);
    }
    glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, MAC_NV2A_ARBFP_ENV_FINAL_C0, in->final0);
    glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, MAC_NV2A_ARBFP_ENV_FINAL_C1, in->final1);
    glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, MAC_NV2A_ARBFP_ENV_FOG_COLOR, in->fog_color);
    glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, MAC_NV2A_ARBFP_ENV_FOG_PARAMETERS, in->fog_parameters);
    v[0] = in->alpha_reference; v[1] = v[2] = v[3] = 0.0f;
    glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, MAC_NV2A_ARBFP_ENV_ALPHA_REFERENCE, v);
    for (i = 0; i < 4; i++) {
        glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, MAC_NV2A_ARBFP_ENV_TEXTURE_SCALE + i, in->texture_scale[i]);
        glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, MAC_NV2A_ARBFP_ENV_BUMP_MATRIX + i, in->bump_matrix[i]);
        glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, MAC_NV2A_ARBFP_ENV_BUMP_LUMINANCE + i, in->bump_luminance[i]);
    }
}

static void draw_quad(const struct inputs *in)
{
    static const float xs[4] = { -1.0f, 1.0f, 1.0f, -1.0f }, ys[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
    int v, s;
    static const int order[6] = { 0, 1, 2, 0, 2, 3 };   /* two triangles, the diagonal from corner 0 to corner 2 */
    int k;
    glBegin(GL_TRIANGLES);
    for (k = 0; k < 6; k++) {
        v = order[k];
        for (s = 0; s < 8; s++) glMultiTexCoord4fvARB(GL_TEXTURE0_ARB + s, in->vertex[v][s]);
        glVertex4f(xs[v], ys[v], 0.0f, 1.0f);
    }
    glEnd();
}

/* ---------- the subjects */

struct subject {
    char name[40];
    struct nv2a_pixel_shader_key key;
    int fog_tex, fog_comp;
    unsigned long samples;
    int frame;
};

struct subject_result {
    int ran, refused;
    char reason[160];
    int max_diff;
    unsigned long over, pixels, covered, distinct;
    int native_alu, native_tex, native_ind, under_native, limits_exceeded;
    int cpu, cpu_glsl_max, cpu_arb_max, glsl_off;
    unsigned long cpu_glsl_over, cpu_arb_over;
    int alu, tex, temps;
    int alu_p, tex_p;
    double glsl_ns, arb_ns;
};

static const char *dump_directory;
static GLuint fbo, color_texture, depth_buffer, big_fbo, big_texture;
static GLuint vertex_program;
static int radeon;

static int build_glsl(const struct subject *sj, struct glsl_program *g, char *why)
{
    char *modern, *legacy = NULL, *rewritten = NULL, error[160], log[2048];
    struct mac_nv2a_arb_layout layout;
    GLuint shader, program;
    GLint linked = 0;
    modern = nv2a_pixel_shader_to_glsl(&sj->key);
    if (!modern) { strcpy(why, "GLSL generation failed"); return 0; }
    if (!mac_glsl120_pixel(modern, &legacy, error, sizeof(error))) { snprintf(why, 160, "GLSL 1.20 rewrite: %s", error); free(modern); return 0; }
    free(modern);
    /* GLSL 1.20 has no integer '&': the generator's mux on the low bit of r0.a is written as a parity test
       (int() truncates towards zero and mod() is floor-based, as the two's complement bit is) */
    {
        static const char from[] = "(int(r0.a * 255.0 + 0.5) & 1) != 0";
        static const char to[] = "mod(float(int(r0.a * 255.0 + 0.5)), 2.0) != 0.0";
        char *at, *patched;
        while ((at = strstr(legacy, from)) != NULL) {
            patched = (char *)malloc(strlen(legacy) + sizeof(to));
            memcpy(patched, legacy, (size_t)(at - legacy));
            strcpy(patched + (at - legacy), to);
            strcpy(patched + (at - legacy) + strlen(to), at + sizeof(from) - 1);
            free(legacy);
            legacy = patched;
        }
    }
    /* a debugging aid: MAC_ARBFP_GLSL_FROM / MAC_ARBFP_GLSL_TO replace a piece of the GLSL text */
    if (getenv("MAC_ARBFP_GLSL_FROM") && getenv("MAC_ARBFP_GLSL_TO")) {
        const char *from = getenv("MAC_ARBFP_GLSL_FROM"), *to = getenv("MAC_ARBFP_GLSL_TO");
        char *at = strstr(legacy, from), *patched;
        if (at) {
            patched = (char *)malloc(strlen(legacy) + strlen(to) + 1);
            memcpy(patched, legacy, (size_t)(at - legacy));
            strcpy(patched + (at - legacy), to);
            strcpy(patched + (at - legacy) + strlen(to), at + strlen(from));
            free(legacy);
            legacy = patched;
        }
    }
    memset(&layout, 0, sizeof(layout));
    layout.fog_texcoord = sj->fog_tex; layout.fog_component = sj->fog_comp;
    if (mac_nv2a_arb_fragment_rewrite(legacy, &layout, &rewritten, error, sizeof(error)) != 1) {
        snprintf(why, 160, "fragment rewrite: %s", error); free(legacy); return 0;
    }
    free(legacy);
    if (dump_directory) {
        char path[400];
        FILE *f;
        sprintf(path, "%s/%s.glsl.frag", dump_directory, sj->name);
        f = fopen(path, "wb");
        if (f) { fputs(rewritten, f); fclose(f); }
    }
    shader = compile_glsl(GL_FRAGMENT_SHADER, rewritten, log, sizeof(log));
    free(rewritten);
    if (!shader) { snprintf(why, 160, "GLSL compile: %.120s", log); return 0; }
    program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    glDeleteShader(shader);
    if (linked != GL_TRUE) { strcpy(why, "GLSL link failed"); glDeleteProgram(program); return 0; }
    g->program = program;
    g->c0 = uloc(program, "ps_c0"); g->c1 = uloc(program, "ps_c1");
    g->final0 = uloc(program, "ps_final_c0"); g->final1 = uloc(program, "ps_final_c1");
    g->fog_color = uloc(program, "fog_color"); g->fog_parameters = uloc(program, "fog_parameters");
    g->alpha_reference = uloc(program, "alpha_reference"); g->texture_scale = uloc(program, "texture_scale");
    g->bump_matrix = uloc(program, "bump_matrix"); g->bump_luminance = uloc(program, "bump_luminance");
    g->tex[0] = uloc(program, "tex0"); g->tex[1] = uloc(program, "tex1");
    g->tex[2] = uloc(program, "tex2"); g->tex[3] = uloc(program, "tex3");
    return 1;
}


/* ---------- a CPU reference of port/linux/src/nv2a_psh.c's semantics for states without texture fetches
   (texture modes none and passthru): the same C float arithmetic, written from the generator's GLSL, not
   from the translator. It tells which of the two GPU programs is off when they disagree. */

static float c_clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float c_max(float a, float b) { return a > b ? a : b; }

static void cpu_read(const struct nv2a_pixel_shader_key *key, const struct inputs *in, float regs[16][4], unsigned long input,
    int alpha_portion, int stage, float out[3], int final)
{
    unsigned long reg = input & 0x0f, mapping = input & 0xe0;
    int alpha_channel = (input & 0x10) != 0, i, n;
    unsigned long count = key->combiner_state[D3DRS_PSCOMBINERCOUNT];
    float src[4] = { 0, 0, 0, 0 }, v[3];
    switch (reg) {
    case 1: for (i = 0; i < 4; i++) src[i] = stage < 0 ? in->final0[i] : in->ps_c0[(count & 0x1000) ? stage : 0][i]; break;
    case 2: for (i = 0; i < 4; i++) src[i] = stage < 0 ? in->final1[i] : in->ps_c1[(count & 0x10000) ? stage : 0][i]; break;
    case 3: case 4: case 5: case 8: case 9: case 10: case 11: case 12: case 13:
        for (i = 0; i < 4; i++) src[i] = regs[reg][i];
        break;
    case 14: case 15:
        if (stage < 0) for (i = 0; i < 4; i++) src[i] = regs[reg][i];
        break;
    default: break;
    }
    if (alpha_portion) { v[0] = alpha_channel ? src[3] : src[2]; n = 1; }
    else if (alpha_channel) { v[0] = v[1] = v[2] = src[3]; n = 3; }
    else { v[0] = src[0]; v[1] = src[1]; v[2] = src[2]; n = 3; }
    for (i = 0; i < n; i++) {
        float x = v[i];
        if (final) {
            x = (input & 0x20) ? 1.0f - c_clamp(x, 0.0f, 1.0f) : c_clamp(x, 0.0f, 1.0f);
        } else {
            switch (mapping) {
            case 0x00: x = c_max(x, 0.0f); break;
            case 0x20: x = 1.0f - c_clamp(x, 0.0f, 1.0f); break;
            case 0x40: x = 2.0f * c_max(x, 0.0f) - 1.0f; break;
            case 0x60: x = 1.0f - 2.0f * c_max(x, 0.0f); break;
            case 0x80: x = c_max(x, 0.0f) - 0.5f; break;
            case 0xa0: x = 0.5f - c_max(x, 0.0f); break;
            case 0xc0: break;
            default: x = -x; break;
            }
        }
        out[i] = x;
    }
    if (n == 1) out[1] = out[2] = out[0];
}

/* bilinear, repeating, as GL_LINEAR with GL_REPEAT on a 64x64 texture */
static void cpu_sample(int unit, float u, float v, float out[4])
{
    float fx = u * 64.0f - 0.5f, fy = v * 64.0f - 0.5f, wx, wy;
    int x0 = (int)floorf(fx), y0 = (int)floorf(fy), c, dx, dy;
    wx = fx - (float)x0; wy = fy - (float)y0;
    for (c = 0; c < 4; c++) out[c] = 0.0f;
    for (dy = 0; dy < 2; dy++)
        for (dx = 0; dx < 2; dx++) {
            int xx = ((x0 + dx) % 64 + 64) % 64, yy = ((y0 + dy) % 64 + 64) % 64;
            float weight = (dx ? wx : 1.0f - wx) * (dy ? wy : 1.0f - wy);
            for (c = 0; c < 4; c++) out[c] += weight * (float)tex_2d_data[unit][(yy * 64 + xx) * 4 + c] / 255.0f;
        }
}

/* the state is one the reference handles: no fetches, or 2D fetches in the project2d and cube map modes */
static int cpu_supported(const struct nv2a_pixel_shader_key *key)
{
    int s;
    if (key->coverage_alpha) return 0;
    for (s = 0; s < 4; s++) {
        unsigned long mode = (key->texture_modes >> (5 * s)) & 0x1f;
        if (mode == 0 || mode == 4) continue;
        if ((mode == 1 || mode == 3) && (key->sampler_type[s] == 1 || key->sampler_type[s] == 0)) continue;
        return 0;
    }
    return 1;
}

static float output_map(float x, unsigned long flags)
{
    switch (flags & 0x38) {
    case 0x08: return x - 0.5f;
    case 0x10: return x * 2.0f;
    case 0x18: return (x - 0.5f) * 2.0f;
    case 0x20: return x * 4.0f;
    case 0x30: return x * 0.5f;
    default: return x;
    }
}

/* returns 0 when the fragment is discarded */
static int cpu_pixel(const struct subject *sj, const struct inputs *in, int px, int py, float result[4])
{
    const struct nv2a_pixel_shader_key *key = &sj->key;
    const DWORD *state = key->combiner_state;
    float u = ((float)px + 0.5f) / (float)SIZE, w = ((float)py + 0.5f) / (float)SIZE;
    float varying[8][4], regs[16][4];
    int slot, i, stage, portion, c;
    unsigned long count = state[D3DRS_PSCOMBINERCOUNT];
    for (slot = 0; slot < 8; slot++)
        for (c = 0; c < 4; c++) {
            const float a0 = in->vertex[0][slot][c], a1 = in->vertex[1][slot][c], a2 = in->vertex[2][slot][c], a3 = in->vertex[3][slot][c];
            varying[slot][c] = u >= w ? a0 + u * (a1 - a0) + w * (a2 - a1) : a0 + u * (a2 - a3) + w * (a3 - a0);
        }
    memset(regs, 0, sizeof(regs));
    {
        float xfog = sj->fog_tex >= 0 ? varying[sj->fog_tex][sj->fog_comp] : 1.0f, factor = 1.0f;
        float seen[8][4];
        memcpy(seen, varying, sizeof(seen));
        if (sj->fog_tex >= 0) seen[sj->fog_tex][sj->fog_comp] = sj->fog_comp == 3 ? 1.0f : 0.0f;
        for (i = 0; i < 4; i++) { regs[4][i] = seen[6][i]; regs[5][i] = seen[7][i]; }
        for (stage = 0; stage < 4; stage++) {
            unsigned long mode = (key->texture_modes >> (5 * stage)) & 0x1f;
            if (mode == 4) for (i = 0; i < 4; i++) regs[8 + stage][i] = c_clamp(seen[2 + stage][i], 0.0f, 1.0f);
            if ((mode == 1 || mode == 3) && key->sampler_type[stage] == 1) {
                const float *t = seen[2 + stage];
                float coord_u, coord_v;
                if (mode == 1) {
                    float q = t[3] != 0.0f ? t[3] : 1.0f;
                    coord_u = t[0] / q; coord_v = t[1] / q;
                } else { coord_u = t[0]; coord_v = t[1]; }
                cpu_sample(stage, coord_u * in->texture_scale[stage][0], coord_v * in->texture_scale[stage][1], regs[8 + stage]);
            }
        }
        if (key->fog_enable) {
            switch (key->fog_table_mode) {
            case 1: factor = expf(-in->fog_parameters[2] * xfog); break;
            case 2: factor = expf(-(in->fog_parameters[2] * xfog) * (in->fog_parameters[2] * xfog)); break;
            case 3: factor = (in->fog_parameters[1] - xfog) / c_max(in->fog_parameters[1] - in->fog_parameters[0], 1.0e-6f); break;
            default: factor = xfog; break;
            }
        }
        for (i = 0; i < 3; i++) regs[3][i] = in->fog_color[i];
        regs[3][3] = c_clamp(factor, 0.0f, 1.0f);
        regs[12][3] = regs[8][3];
    }
    count &= 0xff;
    if (count > 8) count = 8;
    for (stage = 0; stage < (int)count; stage++) {
        float res[2][3][3];      /* [portion][AB, CD, SUM][component] */
        unsigned long dest[2][3], flagsv[2];
        for (portion = 0; portion < 2; portion++) {
            int alpha = portion == 1;
            unsigned long inputs = alpha ? state[D3DRS_PSALPHAINPUTS0 + stage] : state[D3DRS_PSRGBINPUTS0 + stage];
            unsigned long outputs = alpha ? state[D3DRS_PSALPHAOUTPUTS0 + stage] : state[D3DRS_PSRGBOUTPUTS0 + stage];
            unsigned long flags = outputs >> 12;
            float A[3], B[3], C[3], D[3], AB[3], CD[3], SUM[3];
            int n = alpha ? 1 : 3;
            cpu_read(key, in, regs, (inputs >> 24) & 0xff, alpha, stage, A, 0);
            cpu_read(key, in, regs, (inputs >> 16) & 0xff, alpha, stage, B, 0);
            cpu_read(key, in, regs, (inputs >> 8) & 0xff, alpha, stage, C, 0);
            cpu_read(key, in, regs, inputs & 0xff, alpha, stage, D, 0);
            for (i = 0; i < 3; i++) { AB[i] = A[i] * B[i]; CD[i] = C[i] * D[i]; }
            if (!alpha && (flags & 0x02)) AB[0] = AB[1] = AB[2] = A[0] * B[0] + A[1] * B[1] + A[2] * B[2];
            if (!alpha && (flags & 0x01)) CD[0] = CD[1] = CD[2] = C[0] * D[0] + C[1] * D[1] + C[2] * D[2];
            for (i = 0; i < 3; i++) {
                if (flags & 0x04) {
                    int pick;
                    if (((state[D3DRS_PSCOMBINERCOUNT] & 0x100) != 0)) pick = regs[12][3] >= 0.5f;
                    else pick = ((int)(regs[12][3] * 255.0f + 0.5f) & 1) != 0;
                    SUM[i] = pick ? CD[i] : AB[i];
                } else SUM[i] = AB[i] + CD[i];
            }
            for (i = 0; i < 3; i++) {
                res[portion][0][i] = c_clamp(output_map(AB[i], flags), -1.0f, 1.0f);
                res[portion][1][i] = c_clamp(output_map(CD[i], flags), -1.0f, 1.0f);
                res[portion][2][i] = c_clamp(output_map(SUM[i], flags), -1.0f, 1.0f);
            }
            (void)n;
            dest[portion][0] = (outputs >> 4) & 0x0f; dest[portion][1] = outputs & 0x0f; dest[portion][2] = (outputs >> 8) & 0x0f;
            flagsv[portion] = flags;
        }
        for (portion = 0; portion < 2; portion++) {
            int which;
            for (which = 0; which < 3; which++) {
                unsigned long reg = dest[portion][which];
                if (!(reg == 4 || reg == 5 || (reg >= 8 && reg <= 13))) continue;
                if (portion == 0) {
                    for (i = 0; i < 3; i++) regs[reg][i] = res[0][which][i];
                    if ((which == 0 && (flagsv[0] & 0x80)) || (which == 1 && (flagsv[0] & 0x40))) regs[reg][3] = res[0][which][2];
                } else regs[reg][3] = res[1][which][0];
            }
        }
    }
    {
        float rgb[3], alpha;
        unsigned long abcd = state[D3DRS_PSFINALCOMBINERINPUTSABCD], efg = state[D3DRS_PSFINALCOMBINERINPUTSEFG];
        if (abcd == 0 && efg == 0) {
            rgb[0] = regs[12][0]; rgb[1] = regs[12][1]; rgb[2] = regs[12][2]; alpha = regs[12][3];
        } else {
            float E[3], F[3], A[3], B[3], C[3], D[3], G[3], t1[3], t2[3];
            unsigned long settings = efg & 0xff;
            for (i = 0; i < 4; i++) regs[14][i] = regs[15][i] = 0.0f;
            cpu_read(key, in, regs, (efg >> 24) & 0xff, 0, -1, E, 1);
            cpu_read(key, in, regs, (efg >> 16) & 0xff, 0, -1, F, 1);
            for (i = 0; i < 3; i++) regs[15][i] = E[i] * F[i];
            for (i = 0; i < 3; i++) {
                t1[i] = (settings & 0x40) ? 1.0f - c_clamp(regs[5][i], 0.0f, 1.0f) : c_clamp(regs[5][i], 0.0f, 1.0f);
                t2[i] = (settings & 0x20) ? 1.0f - c_clamp(regs[12][i], 0.0f, 1.0f) : c_clamp(regs[12][i], 0.0f, 1.0f);
                regs[14][i] = t1[i] + t2[i];
                if (settings & 0x80) regs[14][i] = c_clamp(regs[14][i], 0.0f, 1.0f);
            }
            cpu_read(key, in, regs, (abcd >> 24) & 0xff, 0, -1, A, 1);
            cpu_read(key, in, regs, (abcd >> 16) & 0xff, 0, -1, B, 1);
            cpu_read(key, in, regs, (abcd >> 8) & 0xff, 0, -1, C, 1);
            cpu_read(key, in, regs, abcd & 0xff, 0, -1, D, 1);
            cpu_read(key, in, regs, (efg >> 8) & 0xff, 1, -1, G, 1);
            for (i = 0; i < 3; i++) rgb[i] = A[i] * B[i] + (1.0f - A[i]) * C[i] + D[i];
            alpha = G[0];
        }
        if (key->alpha_test_function) {
            float a8 = floorf(c_clamp(alpha, 0.0f, 1.0f) * 255.0f + 0.5f), ref = in->alpha_reference;
            int keep = 1;
            switch (key->alpha_test_function) {
            case 512: keep = 0; break;
            case 513: keep = a8 < ref; break;
            case 514: keep = a8 == ref; break;
            case 515: keep = a8 <= ref; break;
            case 516: keep = a8 > ref; break;
            case 517: keep = a8 != ref; break;
            case 518: keep = a8 >= ref; break;
            default: break;
            }
            if (!keep) return 0;
        }
        result[0] = c_clamp(rgb[0], 0.0f, 1.0f); result[1] = c_clamp(rgb[1], 0.0f, 1.0f);
        result[2] = c_clamp(rgb[2], 0.0f, 1.0f); result[3] = c_clamp(alpha, 0.0f, 1.0f);
    }
    return 1;
}

static void read_target(unsigned char *pixels, int size)
{
    glFinish();
    glReadPixels(0, 0, size, size, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

static void clear_sentinel(void)
{
    glClearColor(sentinel[0] / 255.0f, sentinel[1] / 255.0f, sentinel[2] / 255.0f, sentinel[3] / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

static double now(void)
{
    static mach_timebase_info_data_t base;
    if (!base.denom) mach_timebase_info(&base);
    return (double)mach_absolute_time() * (double)base.numer / (double)base.denom * 1e-9;
}

static void query_native(struct subject_result *r)
{
    GLint v = 0;
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB_, &v); r->native_alu = (int)v;
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB_, &v); r->native_tex = (int)v;
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB_, &v); r->native_ind = (int)v;
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB_, &v); r->under_native = (int)v;
    if (getenv("MAC_ARBFP_VERBOSE") && !r->under_native) {
        GLint temps = 0, params = 0, instructions = 0;
        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEMPORARIES_ARB_, &temps);
        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_PARAMETERS_ARB_, &params);
        glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, 0x88A2, &instructions);
        fprintf(stderr, "  over the native limits: temporaries %d parameters %d instructions %d alu %d tex %d indirections %d\n",
            (int)temps, (int)params, (int)instructions, r->native_alu, r->native_tex, r->native_ind);
    }
}

static void run_subject(const struct subject *sj, int scenarios, int do_time, struct subject_result *r, const char *dump_dir)
{
    struct glsl_program g;
    char why[200], error[160];
    GLuint arb[2] = { 0, 0 };
    char *text[2] = { NULL, NULL };
    struct mac_nv2a_arb_layout layout;
    struct mac_nv2a_arbfp_info info[2];
    unsigned long masks[2];
    int variant, scenario, rc, i;
    unsigned char a_pix[SIZE * SIZE * 4], b_pix[SIZE * SIZE * 4];
    unsigned seen[256];
    memset(r, 0, sizeof(*r));
    memset(&g, 0, sizeof(g));
    memset(info, 0, sizeof(info));
    masks[0] = 0xf; masks[1] = 0;
    memset(&layout, 0, sizeof(layout));
    layout.fog_texcoord = sj->fog_tex; layout.fog_component = sj->fog_comp;
    for (variant = 0; variant < 2; variant++) {
        rc = mac_nv2a_arbfp_translate_info(&sj->key, &layout, masks[variant], &text[variant], &info[variant], error, sizeof(error));
        if (rc == 0) { r->refused = 1; strncpy(r->reason, error, sizeof(r->reason) - 1); goto done; }
        if (rc < 0) { CHECK(rc > 0); goto done; }
        if (dump_dir) {
            char path[400];
            FILE *f;
            sprintf(path, "%s/%s.%s.arb", dump_dir, sj->name, variant ? "noproj" : "proj");
            f = fopen(path, "wb");
            if (f) { fputs(text[variant], f); fclose(f); }
        }
    }
    r->alu = (int)info[1].alu_instructions; r->tex = (int)info[1].tex_instructions; r->temps = (int)info[1].temporaries;
    r->alu_p = (int)info[0].alu_instructions; r->tex_p = (int)info[0].tex_instructions;
    if (!build_glsl(sj, &g, why)) { r->refused = 1; snprintf(r->reason, sizeof(r->reason), "GLSL: %s", why); goto done; }
    for (variant = 0; variant < 2; variant++) {
        arb[variant] = load_arb(GL_FRAGMENT_PROGRAM_ARB, text[variant], error, sizeof(error));
        if (!arb[variant]) {
            fprintf(stderr, "%s: %s\n%s\n", sj->name, error, text[variant]);
            CHECK(arb[variant] != 0);
            goto done;
        }
    }
    r->ran = 1;
    for (scenario = 0; scenario < scenarios; scenario++) {
        struct inputs in;
        int proj = scenario % 2 == 0;           /* texture_scale 1, every stage projective */
        unsigned seed = (unsigned)scenario * 31 + name_hash(sj->name);
        int x, y, c;
        unsigned long over = 0;
        random_inputs(&in, &sj->key, sj->fog_tex, sj->fog_comp, proj, seed);
        make_textures(seed);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
        glViewport(0, 0, SIZE, SIZE);
        /* GLSL */
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        bind_stage_textures(&sj->key);
        set_glsl_uniforms(&g, &in);
        clear_sentinel();
        draw_quad(&in);
        read_target(a_pix, SIZE);
        glUseProgram(0);
        /* ARB */
        bind_stage_textures(&sj->key);
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, arb[proj ? 0 : 1]);
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        set_arb_env(&in);
        clear_sentinel();
        draw_quad(&in);
        read_target(b_pix, SIZE);
        if (scenario == 0) { query_native(r); r->limits_exceeded = !r->under_native; }
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        CHECK(glGetError() == GL_NO_ERROR);
        memset(seen, 0, sizeof(seen));
        for (y = 0; y < SIZE; y++)
            for (x = 0; x < SIZE; x++)
                for (c = 0; c < 4; c++) {
                    int d = (int)a_pix[(y * SIZE + x) * 4 + c] - (int)b_pix[(y * SIZE + x) * 4 + c];
                    if (d < 0) d = -d;
                    if (d > r->max_diff) r->max_diff = d;
                    if (d > tolerance) over++;
                    seen[a_pix[(y * SIZE + x) * 4 + c]] = 1;
                    r->pixels++;
                }
        r->over += over;
        if (cpu_supported(&sj->key)) {
            r->cpu = 1;
            for (y = 0; y < SIZE; y++)
                for (x = 0; x < SIZE; x++) {
                    float expect[4];
                    int kept = cpu_pixel(sj, &in, x, y, expect);
                    for (c = 0; c < 4; c++) {
                        int want = kept ? (int)(expect[c] * 255.0f + 0.5f) : sentinel[c];
                        int dg = (int)a_pix[(y * SIZE + x) * 4 + c] - want, da = (int)b_pix[(y * SIZE + x) * 4 + c] - want;
                        if (dg < 0) dg = -dg;
                        if (da < 0) da = -da;
                        if (dg > r->cpu_glsl_max) r->cpu_glsl_max = dg;
                        if (da > r->cpu_arb_max) r->cpu_arb_max = da;
                        if (dg > tolerance) r->cpu_glsl_over++;
                        if (da > tolerance) r->cpu_arb_over++;
                    }
                }
        }
        for (c = 0; c < 256; c++) if (seen[c]) r->distinct++;
        for (i = 0; i < SIZE * SIZE; i++)
            if (memcmp(a_pix + i * 4, sentinel, 4)) r->covered++;
        if (over && getenv("MAC_ARBFP_VERBOSE")) {
            int printed = 0;
            for (y = 0; y < SIZE; y++)
                for (x = 0; x < SIZE; x++) {
                    int worst = 0;
                    for (c = 0; c < 4; c++) {
                        int d = (int)a_pix[(y * SIZE + x) * 4 + c] - (int)b_pix[(y * SIZE + x) * 4 + c];
                        if (d < 0) d = -d;
                        if (d > worst) worst = d;
                    }
                    if (worst > tolerance && printed++ < 12)
                        fprintf(stderr, "  %s scenario %d (%d,%d): glsl %d %d %d %d arb %d %d %d %d\n", sj->name, scenario, x, y,
                            a_pix[(y * SIZE + x) * 4], a_pix[(y * SIZE + x) * 4 + 1], a_pix[(y * SIZE + x) * 4 + 2], a_pix[(y * SIZE + x) * 4 + 3],
                            b_pix[(y * SIZE + x) * 4], b_pix[(y * SIZE + x) * 4 + 1], b_pix[(y * SIZE + x) * 4 + 2], b_pix[(y * SIZE + x) * 4 + 3]);
                }
        }
    }
    /* a program over the native limits is one the renderer keeps on GLSL (it checks GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB);
       the driver's fallback for it is not what is being compared */
    /* where the CPU reference exists and the ARB program agrees with it (a few boundary pixels aside) while the
       GLSL program does not, the GLSL driver is the one off (found with NVIDIA's compiler on random states) */
    r->glsl_off = r->cpu && r->over && r->cpu_arb_over <= 16 && r->cpu_glsl_over > 16;
    if (!r->limits_exceeded && !r->glsl_off) CHECK(r->over == 0);
    if (do_time) {
        struct inputs in;
        enum { DRAWS = 6 };
        double t0, t1;
        int d;
        random_inputs(&in, &sj->key, sj->fog_tex, sj->fog_comp, 1, 5);
        make_textures(5);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, big_fbo);
        glViewport(0, 0, TIME_SIZE, TIME_SIZE);
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        bind_stage_textures(&sj->key);
        set_glsl_uniforms(&g, &in);
        draw_quad(&in);
        glFinish();
        t0 = now();
        for (d = 0; d < DRAWS; d++) draw_quad(&in);
        glFinish();
        t1 = now();
        r->glsl_ns = (t1 - t0) * 1e9 / ((double)DRAWS * TIME_SIZE * TIME_SIZE);
        glUseProgram(0);
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, arb[0]);
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        set_arb_env(&in);
        draw_quad(&in);
        glFinish();
        t0 = now();
        for (d = 0; d < DRAWS; d++) draw_quad(&in);
        glFinish();
        t1 = now();
        r->arb_ns = (t1 - t0) * 1e9 / ((double)DRAWS * TIME_SIZE * TIME_SIZE);
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    }
done:
    for (variant = 0; variant < 2; variant++) {
        free(text[variant]);
        if (arb[variant]) glDeleteProgramsARB(1, &arb[variant]);
    }
    if (g.program) glDeleteProgram(g.program);
    glUseProgram(0);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

/* ---------- synthetic and random states */

static void synth_base(struct subject *s, const char *name)
{
    memset(s, 0, sizeof(*s));
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->fog_tex = -1;
    s->key.combiner_state[D3DRS_PSCOMBINERCOUNT] = 1;
    s->key.combiner_state[D3DRS_PSRGBINPUTS0] = 0x08040000;     /* t0 * v0 */
    s->key.combiner_state[D3DRS_PSRGBOUTPUTS0] = 0x0c00;        /* sum -> r0 */
    s->key.combiner_state[D3DRS_PSALPHAINPUTS0] = 0x18141000;
    s->key.combiner_state[D3DRS_PSALPHAOUTPUTS0] = 0x0c00;
    s->key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x0c;
    s->key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = 0x1c00;
    s->key.texture_modes = 1;
    s->key.sampler_type[0] = 1;
}

static int make_synthetic(struct subject *list, int max)
{
    int n = 0, i, mode;
    char name[40];
    static const unsigned long funcs[7] = { 512, 513, 514, 515, 516, 517, 518 };
    static const char *const func_names[7] = { "never", "less", "equal", "lequal", "greater", "notequal", "gequal" };
    for (i = 0; i < 7 && n < max; i++) {
        sprintf(name, "alpha_%s", func_names[i]);
        synth_base(&list[n], name);
        list[n].key.alpha_test_function = funcs[i];
        n++;
    }
    for (i = 0; i < 4 && n < max; i++) {
        sprintf(name, "fog_mode%d", i);
        synth_base(&list[n], name);
        /* final: fog.rgb * (1 - fog.a)... out = lerp(fog.a, r0, fog.rgb) */
        list[n].key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x13 << 24 | 0x0c << 16 | 0x03 << 8;
        list[n].key.fog_enable = 1; list[n].key.fog_table_mode = (unsigned char)i;
        list[n].fog_tex = i; list[n].fog_comp = i;
        n++;
    }
    if (n < max) {
        synth_base(&list[n], "coverage_alpha");
        list[n].key.coverage_alpha = 1;
        n++;
    }
    for (i = 0; i < 4 && n < max; i++) {
        /* the fog coordinate in a component of a texcoord slot the pixel shader also reads (the unwritten
           component reads as its default, as the GLSL prelude has it) */
        sprintf(name, "fog_in_t0_%c", "xyzw"[i]);
        synth_base(&list[n], name);
        list[n].key.fog_enable = 1; list[n].key.fog_table_mode = 3;
        list[n].key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x13UL << 24 | 0x0cUL << 16 | 0x03UL << 8;
        list[n].fog_tex = 2; list[n].fog_comp = i;
        n++;
    }
    for (mode = 1; mode <= 18 && n < max; mode++) {
        int sampler;
        for (sampler = 1; sampler <= 3 && n < max; sampler++) {
            int stage = mode >= 9 || mode == 15 || mode == 16 ? 2 : 1;
            sprintf(name, "mode%d_s%d", mode, sampler);
            synth_base(&list[n], name);
            list[n].key.texture_modes = 1UL | ((unsigned long)mode << (5 * stage));
            list[n].key.sampler_type[0] = 1;
            list[n].key.sampler_type[stage] = (unsigned char)sampler;
            if (mode == 9 || mode == 11 || mode == 12 || mode == 13 || mode == 14 || mode == 18) {
                /* dot product chains: stage 1 supplies dot1 */
                list[n].key.texture_modes = 1UL | (17UL << 5) | ((unsigned long)mode << (5 * stage));
                if (stage == 1) { list[n].key.texture_modes = 1UL | ((unsigned long)mode << 5); }
            }
            list[n].key.combiner_state[D3DRS_PSDOTMAPPING] = (unsigned long)((mode + sampler) & 3) | (((unsigned long)mode & 3) << 4) | (1UL << 8);
            list[n].key.combiner_state[D3DRS_PSINPUTTEXTURE] = 0;
            list[n].key.combiner_state[D3DRS_PSCOMPAREMODE] = 0x0a50UL;
            list[n].key.combiner_state[D3DRS_PSRGBINPUTS0] = (unsigned long)(0x08 + stage) << 24 | 0x04 << 16;
            list[n].key.combiner_state[D3DRS_PSALPHAINPUTS0] = (unsigned long)(0x18 + stage) << 24 | 0x14 << 16;
            n++;
        }
    }
    return n;
}

static unsigned long rnd_reg_byte(int final)
{
    static const unsigned char regs[] = { 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 12, 13, 4, 8, 1, 14, 15 };
    /* (combiner inputs read 14 and 15 as zero; final ones read them, E and F -final 2- must not: the
       generator's own declarations of v1r0_sum and ef_product are in terms of E and F) */
    unsigned long reg = regs[rng() % (sizeof(regs) - (final == 2 ? 2 : 0))];
    unsigned long byte = reg | ((rng() & 1) ? 0x10 : 0);
    if (final) { if (rng() % 3 == 0) byte |= 0x20; }
    else byte |= (unsigned long)(rng() % 8) << 5;
    return byte;
}

static void make_random(struct subject *s, unsigned seed)
{
    int st, j;
    static const unsigned char dests[] = { 0, 0, 4, 5, 8, 9, 10, 11, 12, 12, 12, 13 };
    memset(s, 0, sizeof(*s));
    sprintf(s->name, "random_%u", seed);
    rng_state = 9000 + seed * 2654435761UL;
    s->fog_tex = -1;
    {
        unsigned long count = 1 + rng() % 8;
        if (rng() & 1) count |= 0x100;
        if (rng() & 1) count |= 0x1000;
        if (rng() & 1) count |= 0x10000;
        s->key.combiner_state[D3DRS_PSCOMBINERCOUNT] = count;
    }
    for (st = 0; st < 8; st++) {
        unsigned long words[2], outs[2];
        for (j = 0; j < 2; j++) {
            words[j] = (rnd_reg_byte(0) << 24) | (rnd_reg_byte(0) << 16) | (rnd_reg_byte(0) << 8) | rnd_reg_byte(0);
            outs[j] = dests[rng() % sizeof(dests)] | ((unsigned long)dests[rng() % sizeof(dests)] << 4) |
                ((unsigned long)dests[rng() % sizeof(dests)] << 8);
            outs[j] |= (((rng() % 3 == 0 && random_mux) ? 0x04UL : 0) | (rng() % 4 == 0 ? 0x02UL : 0) | (rng() % 4 == 0 ? 0x01UL : 0) |
                ((rng() % 3 ? (rng() % 6) : 0) << 3) | (rng() % 5 == 0 ? 0x80UL : 0) | (rng() % 5 == 0 ? 0x40UL : 0)) << 12;
        }
        s->key.combiner_state[D3DRS_PSRGBINPUTS0 + st] = words[0];
        s->key.combiner_state[D3DRS_PSALPHAINPUTS0 + st] = words[1];
        s->key.combiner_state[D3DRS_PSRGBOUTPUTS0 + st] = outs[0];
        s->key.combiner_state[D3DRS_PSALPHAOUTPUTS0 + st] = outs[1];
    }
    if (rng() % 5) {
        s->key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = (rnd_reg_byte(1) << 24) | (rnd_reg_byte(1) << 16) | (rnd_reg_byte(1) << 8) | rnd_reg_byte(1);
        s->key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = (rnd_reg_byte(2) << 24) | (rnd_reg_byte(2) << 16) | (rnd_reg_byte(1) << 8) | (rng() & 0xe0);
    }
    for (j = 0; j < 4; j++) {
        unsigned long mode = rng() % 6;
        if (mode == 2 || mode == 5) mode = 1;
        if (mode == 3) { s->key.sampler_type[j] = (rng() % 3) ? 3 : 1; }
        else s->key.sampler_type[j] = (rng() % 8) ? 1 : 0;
        if (random_notex) mode = (rng() & 1) ? 4 : 0;
        if (random_cpu && mode == 3) s->key.sampler_type[j] = 1;
        s->key.texture_modes |= mode << (5 * j);
    }
    if (getenv("MAC_ARBFP_FORCE_COUNT")) {
        /* a debugging aid: bisect a failing random state by the number of combiner stages */
        unsigned long c = s->key.combiner_state[D3DRS_PSCOMBINERCOUNT];
        s->key.combiner_state[D3DRS_PSCOMBINERCOUNT] = (c & ~0xffUL) | (unsigned long)atoi(getenv("MAC_ARBFP_FORCE_COUNT"));
    }
    if (getenv("MAC_ARBFP_FORCE_FINAL")) {
        /* a debugging aid: the final combiner passes one register (rgb and alpha) through */
        unsigned long reg = (unsigned long)atoi(getenv("MAC_ARBFP_FORCE_FINAL"));
        s->key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = reg;
        s->key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = (reg | 0x10) << 8;
        if (getenv("MAC_ARBFP_FORCE_CHANNEL")) {   /* r, g, b or a only */
            char ch = getenv("MAC_ARBFP_FORCE_CHANNEL")[0];
            if (ch == 'a') s->key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x0;
            else s->key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = 0;
            if (ch == 'a') s->key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0, s->key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] |= 0x01;
        }
    }
    if (rng() % 3 == 0) s->key.alpha_test_function = 513 + rng() % 6;
    if (rng() % 3 == 0) { s->key.fog_enable = 1; s->key.fog_table_mode = (unsigned char)(rng() % 4); s->fog_tex = (int)(rng() % 8); s->fog_comp = (int)(rng() % 4); }
}

/* ---------- main */

int main(int argc, char **argv)
{
    CGLPixelFormatAttribute attrs[] = { kCGLPFAAccelerated, kCGLPFAColorSize, (CGLPixelFormatAttribute)24, (CGLPixelFormatAttribute)0 };
    CGLPixelFormatObj format = NULL;
    CGLContextObj context = NULL;
    GLint formats = 0, limit = 0;
    const char *keys_path = "tools/mac_nv2a_arbfp_keys.txt", *only = NULL, *dump_dir = NULL;
    int do_frame = 1, do_synth = 1, do_time = 0, randoms = 150, selected = 0, arg, i, n;
    const GLubyte *renderer, *ext;
    struct record *records = NULL;
    int record_count = 0;
    struct subject *subjects;
    int subject_count = 0;
    char error[200];
    struct subject_result *results;
    unsigned long total_over = 0;
    int worst = 0, refused = 0, ran = 0, over_limits = 0, glsl_off_count = 0;
    GLuint vp;

    for (arg = 1; arg < argc; arg++) {
        /* frame, synthetic and random=N select what runs (default: the first two and 150 random states) */
        if (!strcmp(argv[arg], "frame")) { if (!selected) { do_frame = do_synth = 0; randoms = 0; selected = 1; } do_frame = 1; }
        else if (!strcmp(argv[arg], "synthetic")) { if (!selected) { do_frame = do_synth = 0; randoms = 0; selected = 1; } do_synth = 1; }
        else if (!strncmp(argv[arg], "random=", 7)) { if (!selected) { do_frame = do_synth = 0; randoms = 0; selected = 1; } randoms = atoi(argv[arg] + 7); }
        else if (!strcmp(argv[arg], "time")) do_time = 1;
        else if (!strcmp(argv[arg], "nomux")) random_mux = 0;
        else if (!strcmp(argv[arg], "notex")) random_notex = 1;
        else if (!strcmp(argv[arg], "cpu")) random_cpu = 1;
        else if (!strncmp(argv[arg], "tolerance=", 10)) tolerance = atoi(argv[arg] + 10);
        else if (!strncmp(argv[arg], "only=", 5)) only = argv[arg] + 5;
        else if (!strncmp(argv[arg], "dump=", 5)) dump_dir = dump_directory = argv[arg] + 5;
        else keys_path = argv[arg];
    }
    CHECK(CGLChoosePixelFormat(attrs, &format, &formats) == kCGLNoError && format);
    if (!format) return 2;
    CHECK(CGLCreateContext(format, NULL, &context) == kCGLNoError && context);
    if (!context) return 2;
    CHECK(CGLSetCurrentContext(context) == kCGLNoError);
    renderer = glGetString(GL_RENDERER);
    ext = glGetString(GL_EXTENSIONS);
    printf("renderer: %s / %s\n", (const char *)renderer, (const char *)glGetString(GL_VERSION));
    radeon = strstr((const char *)renderer, "Radeon") != NULL || strstr((const char *)renderer, "ATI") != NULL;
    if (!ext || !strstr((const char *)ext, "GL_ARB_fragment_program") || !strstr((const char *)ext, "GL_ARB_vertex_program")) {
        printf("no ARB programs\n"); return 2;
    }
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB_, &limit);
    printf("fragment program limits: env parameters %d (need %d)", (int)limit, MAC_NV2A_ARBFP_ENV_COUNT);
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB_, &limit); printf(", native ALU %d", (int)limit);
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB_, &limit); printf(", TEX %d", (int)limit);
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB_, &limit); printf(", indirections %d", (int)limit);
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB_, &limit); printf(", temporaries %d", (int)limit);
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, 0x88AB, &limit); printf(", native parameters %d", (int)limit);
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, 0x88B1, &limit); printf(", native attribs %d\n", (int)limit);

    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glGenTextures(1, &color_texture);
    glBindTexture(GL_TEXTURE_2D, color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SIZE, SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, color_texture, 0);
    CHECK(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT);
    glGenFramebuffersEXT(1, &big_fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, big_fbo);
    glGenTextures(1, &big_texture);
    glBindTexture(GL_TEXTURE_2D, big_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TIME_SIZE, TIME_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, big_texture, 0);
    CHECK(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    (void)depth_buffer;
    glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_DITHER); glDisable(GL_CULL_FACE);
    glGenTextures(4, tex_2d); glGenTextures(4, tex_3d); glGenTextures(4, tex_cube);
    vp = load_arb(GL_VERTEX_PROGRAM_ARB, vertex_passthrough, error, sizeof(error));
    CHECK(vp != 0);
    if (!vp) { fprintf(stderr, "%s\n", error); return 2; }
    vertex_program = vp;
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vp);
    glEnable(GL_VERTEX_PROGRAM_ARB);

    subjects = (struct subject *)calloc(1024, sizeof(*subjects));
    results = (struct subject_result *)calloc(1024, sizeof(*results));
    if (do_frame) {
        if (!read_keys(keys_path, &records, &record_count)) return 2;
        for (i = 0; i < record_count && subject_count < 1024; i++) {
            struct subject *s = &subjects[subject_count++];
            memset(s, 0, sizeof(*s));
            strncpy(s->name, records[i].name, sizeof(s->name) - 1);
            s->key = records[i].key; s->fog_tex = records[i].fog_tex; s->fog_comp = records[i].fog_comp;
            s->samples = records[i].samples; s->frame = 1;
        }
    }
    if (do_synth) subject_count += make_synthetic(subjects + subject_count, 1024 - subject_count);
    for (i = 0; i < randoms && subject_count < 1024; i++) make_random(&subjects[subject_count++], (unsigned)i + 1);

    printf("%-14s %9s | %4s %4s %5s | %6s %6s    | %s | %s\n", "subject", "samples", "alu", "tex", "temps", "nat/arb alu", "nat/arb tex ind under", "ok", "max diff / pixels over / covered");
    for (n = 0; n < subject_count; n++) {
        struct subject_result *r = &results[n];
        if (only && strcmp(only, subjects[n].name)) continue;
        run_subject(&subjects[n], 4, do_time, r, dump_dir);
        if (r->refused) {
            ++refused;
            printf("%-14s %9lu | refused: %.100s\n", subjects[n].name, subjects[n].samples, r->reason);
            continue;
        }
        if (!r->ran) continue;
        ++ran;
        if (r->glsl_off) ++glsl_off_count;
        if (r->limits_exceeded) ++over_limits;
        else if (r->glsl_off) { if (r->cpu_arb_max > worst) worst = r->cpu_arb_max; }
        else { total_over += r->over; if (r->max_diff > worst) worst = r->max_diff; }
        printf("%-14s %9lu | %4d %4d %5d | %2d/%-3d %2d/%-3d i%d %d | %s | max %3d over %5lu covered %5lu distinct %3lu",
            subjects[n].name, subjects[n].samples, r->alu, r->tex, r->temps, r->native_alu, r->alu_p, r->native_tex, r->tex_p,
            r->native_ind, r->under_native, r->over ? (r->limits_exceeded ? "LIMIT" : (r->glsl_off ? "GLSL?" : "DIFF")) : "ok", r->max_diff, r->over, r->covered, r->distinct);
        if (r->cpu) printf(" | cpu: glsl %d/%lu arb %d/%lu", r->cpu_glsl_max, r->cpu_glsl_over, r->cpu_arb_max, r->cpu_arb_over);
        if (do_time) printf(" | glsl %7.2f arb %7.2f ns/px", r->glsl_ns, r->arb_ns);
        printf("\n");
    }
    printf("subjects run %d (over the native limits, not compared: %d; GLSL off the CPU reference while ARB agrees: %d), refused %d, worst difference %d/255 (tolerance %d), pixels over tolerance %lu\n", ran, over_limits, glsl_off_count, refused, worst, tolerance, total_over);
    if (do_time) {
        double sg = 0, sa = 0, w = 0, ga = 0, aa = 0;
        for (n = 0; n < subject_count; n++) {
            if (!results[n].ran || !subjects[n].frame) continue;
            sg += results[n].glsl_ns * (double)subjects[n].samples;
            sa += results[n].arb_ns * (double)subjects[n].samples;
            w += (double)subjects[n].samples;
            ga += results[n].glsl_ns; aa += results[n].arb_ns;
        }
        if (w > 0) printf("sample-weighted ns/pixel over the frame's shaders: glsl %.2f arb %.2f (unweighted sums: %.1f / %.1f)\n", sg / w, sa / w, ga, aa);
    }
    (void)radeon; (void)vertex_program;
    printf("%u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
