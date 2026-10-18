/* Replays a frame captured by tools/mac_gl_record_interpose.c in a hidden CGL
   context, many times, and times it; with ablations that change the frame to
   find which part of it is expensive.

   usage: gl_replay capture.glr [-n frames] [-A ablations] [-m PFT] [-v] [-o out.ppm] [-q]
     -n   frames per phase (default 100); the first 3 of each are not counted
     -A   letters, after the first frame only:
          a  no glProgramEnvParameter calls            b  no texture binds/parameters
          c  one vertex and one fragment program (the first draw's) for every draw
          d  scissor 1x1 for every draw                e  indices from one static element buffer
          f  client vertex arrays copied to one static VBO (same layout)
          F  client vertex arrays repacked (used attributes only) in one static VBO
          g  draws sorted by (vertex, fragment) program, program binds only on change
          G  the same, fragment program first        h  glDrawArrays of the same vertex count
          x  draws of vertex programs reading 100 or more env parameters left out
          z  no draws at all (all state calls kept)
     -m   phases: P pipelined (no wait between frames), F glFinish after every frame,
          T per-call-type timing (glFinish after every frame)
     -L n drop the top n mip levels of textures at setup (a texture with one level loses all its data)
   -X n leave out the data of every texture image larger than n bytes
   -Z   the 2048x2048 texture allocated without data (the text atlas) is made 4x4, or GLR_ATLAS_EDGE square (-X 4000000 implied)
   -S   every texture with data becomes one 1x1 level
   -v   print a checksum of the final picture      -o  write it as a PPM
   Build: tools/mac_gl_replay_checks.sh */
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <mach/mach_time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mac_gl_replay_format.h"

typedef struct { unsigned op, nw, bloblen; const unsigned *w; const unsigned char *blob; } Op;

static const char *op_names[OP_COUNT + 1] = {
    "none",
#define X(n) #n,
    GLR_SPECIAL_OPS(X)
#undef X
#define GLR_GEN_NAMES
#include "gl_simple.inc"
#undef GLR_GEN_NAMES
    0
};

static union { float f; unsigned u; } cvt;
static float u2f(unsigned u) { cvt.u = u; return cvt.f; }
static unsigned f2u_s(float f) { cvt.f = f; return cvt.u; }
static double w2d(const unsigned *w) { double d; memcpy(&d, w, 8); return d; }

/* ---- clock ---- */
static mach_timebase_info_data_t tb;
static double ns_per_tick;
static unsigned long long now(void) { return mach_absolute_time(); }
static double ms_of(unsigned long long ticks) { return (double)ticks * ns_per_tick / 1e6; }

/* ---- object name maps: recorded name -> this context's name ---- */
enum { C_TEX, C_BUF, C_ARB, C_FBO, C_RBO, C_QRY, C_GLSL, C_COUNT };
#define MAPMAX (1u << 18)
#define FAKE_IBO 0x3FFF0u
#define FAKE_VBO 0x3FFF1u
#define FAKE_CMP 0x3FFF2u
static unsigned *idmap[C_COUNT];
static int *locmap[MAPMAX >> 4];
static unsigned mapid(int c, unsigned id)
{
    if (!id) return 0;
    if (id >= MAPMAX) return 0;
    return idmap[c][id];
}
static void mapset(int c, unsigned id, unsigned real) { if (id < MAPMAX) idmap[c][id] = real; }

/* ---- program facts, from the text ---- */
#define PMAX 65536
static unsigned vp_env[PMAX], prog_len[PMAX], prog_ops[PMAX];
static unsigned char prog_target[PMAX];
static unsigned bound_arb[2];       /* recorded names bound now */
static unsigned cur_vp, cur_fp, cur_glsl;

static void program_facts(unsigned target, unsigned id, const unsigned char *t, unsigned len)
{
    unsigned i, distinct = 0, ops = 0;
    unsigned char seen[1024];
    if (id >= PMAX) return;
    memset(seen, 0, sizeof seen);
    for (i = 0; i < len; i++) {
        if (t[i] == ';') ops++;
        if (i + 12 <= len && !memcmp(t + i, "program.env[", 12)) {
            unsigned a = 0, b, j = i + 12;
            while (j < len && t[j] >= '0' && t[j] <= '9') a = a * 10 + t[j++] - '0';
            b = a;
            if (j + 1 < len && t[j] == '.' && t[j + 1] == '.') {
                j += 2; b = 0;
                while (j < len && t[j] >= '0' && t[j] <= '9') b = b * 10 + t[j++] - '0';
            }
            for (; a <= b && a < 1024; a++) if (!seen[a]) { seen[a] = 1; distinct++; }
        }
    }
    prog_target[id] = target == GL_FRAGMENT_PROGRAM_ARB;
    vp_env[id] = distinct; prog_len[id] = len; prog_ops[id] = ops;
}

/* ---- per-call-type and per-program timing ---- */
static int timing, gpu_check;
static CGLContextObj ctx;
static unsigned long long op_ticks[OP_COUNT + 16];
static unsigned long op_count[OP_COUNT + 16];
typedef struct { unsigned vp, fp, glsl; unsigned long draws; unsigned long long ticks; unsigned long verts; int vgpu_min, fgpu_min; } ProgTime;
static ProgTime ptab[1024];
static unsigned nptab;
static ProgTime *prog_entry(void)
{
    unsigned i;
    for (i = 0; i < nptab; i++) if (ptab[i].vp == cur_vp && ptab[i].fp == cur_fp && ptab[i].glsl == cur_glsl) return &ptab[i];
    if (nptab >= 1024) return &ptab[1023];
    ptab[nptab].vp = cur_vp; ptab[nptab].fp = cur_fp; ptab[nptab].glsl = cur_glsl;
    ptab[nptab].vgpu_min = ptab[nptab].fgpu_min = 1;
    return &ptab[nptab++];
}

/* ---- executing one record ---- */
static const unsigned char *cur_blob[16];
static GLuint scratch_u[64];
static GLfloat scratch_f[64];
static int errors_checked;
static unsigned long client_pointer_calls, draws_done;

static unsigned type_bytes(unsigned type)
{
    switch (type) {
    case 0x1400: case 0x1401: return 1;
    case 0x1402: case 0x1403: case 0x140B: return 2;
    case 0x140A: return 8;
    }
    return 4;
}
static void gen_objects(int c, void (*fn)(GLsizei, GLuint *), const Op *o)
{
    unsigned n = o->w[0], i; GLuint ids[256];
    if (n > 256) n = 256;
    fn((GLsizei)n, ids);
    for (i = 0; i < n; i++) mapset(c, ((const unsigned *)o->blob)[i], ids[i]);
}
static void delete_objects(int c, void (*fn)(GLsizei, const GLuint *), const Op *o)
{
    unsigned n = o->w[0], i; GLuint ids[256];
    if (n > 256) n = 256;
    for (i = 0; i < n; i++) {
        unsigned rec = ((const unsigned *)o->blob)[i];
        ids[i] = mapid(c, rec); mapset(c, rec, 0);
    }
    fn((GLsizei)n, ids);
}
static int uniform_loc(unsigned rec)
{
    if (rec >= 4096 || cur_glsl >= (MAPMAX >> 4) * 16) return -1;
    if (!locmap[cur_glsl >> 4]) return -1;
    return locmap[cur_glsl >> 4][(cur_glsl & 15) * 4096 + rec];
}

static void do_op(const Op *o)
{
    const unsigned *w = o->w;
    switch (o->op) {
    case OP_NONE: break;
#define GLR_GEN_PLAY
#include "gl_simple.inc"
#undef GLR_GEN_PLAY
    case OP_glGenTextures: gen_objects(C_TEX, glGenTextures, o); break;
    case OP_glGenBuffers: gen_objects(C_BUF, glGenBuffers, o); break;
    case OP_glGenProgramsARB: gen_objects(C_ARB, glGenProgramsARB, o); break;
    case OP_glGenFramebuffersEXT: gen_objects(C_FBO, glGenFramebuffersEXT, o); break;
    case OP_glGenRenderbuffersEXT: gen_objects(C_RBO, glGenRenderbuffersEXT, o); break;
    case OP_glGenQueriesARB: gen_objects(C_QRY, glGenQueriesARB, o); break;
    case OP_glDeleteTextures: delete_objects(C_TEX, glDeleteTextures, o); break;
    case OP_glDeleteBuffers: delete_objects(C_BUF, glDeleteBuffers, o); break;
    case OP_glDeleteProgramsARB: delete_objects(C_ARB, glDeleteProgramsARB, o); break;
    case OP_glDeleteFramebuffersEXT: delete_objects(C_FBO, glDeleteFramebuffersEXT, o); break;
    case OP_glDeleteRenderbuffersEXT: delete_objects(C_RBO, glDeleteRenderbuffersEXT, o); break;
    case OP_glDeleteQueriesARB: delete_objects(C_QRY, glDeleteQueriesARB, o); break;
    case OP_glDeleteShader: glDeleteShader(mapid(C_GLSL, w[0])); break;
    case OP_glDeleteProgram: glDeleteProgram(mapid(C_GLSL, w[0])); break;
    case OP_glBindTexture: glBindTexture(w[0], mapid(C_TEX, w[1])); break;
    case OP_glBindBuffer: glBindBuffer(w[0], mapid(C_BUF, w[1])); break;
    case OP_glBindProgramARB:
        glBindProgramARB(w[0], mapid(C_ARB, w[1]));
        bound_arb[w[0] == GL_FRAGMENT_PROGRAM_ARB] = w[1];
        if (w[0] == GL_FRAGMENT_PROGRAM_ARB) cur_fp = w[1]; else cur_vp = w[1];
        break;
    case OP_glBindFramebufferEXT: glBindFramebufferEXT(w[0], mapid(C_FBO, w[1])); break;
    case OP_glBindRenderbufferEXT: glBindRenderbufferEXT(w[0], mapid(C_RBO, w[1])); break;
    case OP_glUseProgram: glUseProgram(mapid(C_GLSL, w[0])); cur_glsl = w[0]; break;
    case OP_glFramebufferTexture2DEXT: glFramebufferTexture2DEXT(w[0], w[1], w[2], mapid(C_TEX, w[3]), (GLint)w[4]); break;
    case OP_glFramebufferRenderbufferEXT: glFramebufferRenderbufferEXT(w[0], w[1], w[2], mapid(C_RBO, w[3])); break;
    case OP_glCreateShader: mapset(C_GLSL, w[1], glCreateShader(w[0])); break;
    case OP_glCreateProgram: mapset(C_GLSL, w[0], glCreateProgram()); break;
    case OP_glShaderSource: {
        const GLchar *s = (const GLchar *)o->blob; GLint len = (GLint)o->bloblen;
        glShaderSource(mapid(C_GLSL, w[0]), 1, &s, &len); break; }
    case OP_glCompileShader: glCompileShader(mapid(C_GLSL, w[0])); break;
    case OP_glAttachShader: glAttachShader(mapid(C_GLSL, w[0]), mapid(C_GLSL, w[1])); break;
    case OP_glLinkProgram: glLinkProgram(mapid(C_GLSL, w[0])); break;
    case OP_glBindAttribLocation: glBindAttribLocation(mapid(C_GLSL, w[0]), w[1], (const GLchar *)o->blob); break;
    case OP_glGetUniformLocation: {
        GLint real = glGetUniformLocation(mapid(C_GLSL, w[0]), (const GLchar *)o->blob);
        if (w[0] < (MAPMAX >> 4) * 16 && (int)w[1] >= 0 && w[1] < 4096) {
            if (!locmap[w[0] >> 4]) {
                locmap[w[0] >> 4] = (int *)malloc(16 * 4096 * sizeof(int));
                memset(locmap[w[0] >> 4], 0xFF, 16 * 4096 * sizeof(int));
            }
            locmap[w[0] >> 4][(w[0] & 15) * 4096 + w[1]] = real;
        }
        break; }
    case OP_glGetAttribLocation: (void)glGetAttribLocation(mapid(C_GLSL, w[0]), (const GLchar *)o->blob); break;
    case OP_glUniform1f: { int l = uniform_loc(w[0]); if (l >= 0) glUniform1f(l, u2f(w[1])); break; }
    case OP_glUniform1i: { int l = uniform_loc(w[0]); if (l >= 0) glUniform1i(l, (GLint)w[1]); break; }
    case OP_glUniform4fv: { int l = uniform_loc(w[0]); if (l >= 0) glUniform4fv(l, (GLsizei)w[1], (const GLfloat *)o->blob); break; }
    case OP_glBeginQueryARB: glBeginQueryARB(w[0], mapid(C_QRY, w[1])); break;
    case OP_glGetQueryObjectuivARB: glGetQueryObjectuivARB(mapid(C_QRY, w[0]), w[1], scratch_u); break;
    case OP_glProgramStringARB:
        glProgramStringARB(w[0], w[1], (GLsizei)w[2], o->blob);
        program_facts(w[0], bound_arb[w[0] == GL_FRAGMENT_PROGRAM_ARB], o->blob, w[2]);
        if (!errors_checked) {
            GLint pos = -1; glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
            if (pos != -1) fprintf(stderr, "program %u failed at %d: %s\n", bound_arb[w[0] == GL_FRAGMENT_PROGRAM_ARB], (int)pos,
                glGetString(GL_PROGRAM_ERROR_STRING_ARB));
        }
        break;
    case OP_glProgramEnvParameter4fvARB: glProgramEnvParameter4fvARB(w[0], w[1], (const GLfloat *)(w + 2)); break;
    case OP_glVertexAttrib4fv: glVertexAttrib4fv(w[0], (const GLfloat *)(w + 1)); break;
    case OP_glTexParameterfv: glTexParameterfv(w[0], w[1], (const GLfloat *)(w + 3)); break;
    case OP_glBufferData: glBufferData(w[0], (GLsizeiptr)w[1], w[3] ? o->blob : 0, w[2]); break;
    case OP_glBufferSubData: glBufferSubData(w[0], (GLintptr)w[1], (GLsizeiptr)w[2], o->blob); break;
    case OP_glTexImage2D: glTexImage2D(w[0], (GLint)w[1], (GLint)w[2], (GLsizei)w[3], (GLsizei)w[4], (GLint)w[5], w[6], w[7], w[8] ? o->blob : 0); break;
    case OP_glTexSubImage2D: glTexSubImage2D(w[0], (GLint)w[1], (GLint)w[2], (GLint)w[3], (GLsizei)w[4], (GLsizei)w[5], w[6], w[7], o->blob); break;
    case OP_glTexImage3D: glTexImage3D(w[0], (GLint)w[1], (GLint)w[2], (GLsizei)w[3], (GLsizei)w[4], (GLsizei)w[5], (GLint)w[6], w[7], w[8], w[9] ? o->blob : 0); break;
    case OP_glCompressedTexImage2D: glCompressedTexImage2D(w[0], (GLint)w[1], w[2], (GLsizei)w[3], (GLsizei)w[4], (GLint)w[5], (GLsizei)w[6], w[7] ? o->blob : 0); break;
    case OP_glVertexAttribPointer: glVertexAttribPointer(w[0], (GLint)w[1], w[2], (GLboolean)w[3], (GLsizei)w[4], (const GLvoid *)(unsigned long)w[5]); break;
    case OP_glVertexAttribPointerClient: break;   /* done at the draw, from the copied data */
    case OP_glClientBlob: cur_blob[w[0] & 15] = o->blob; break;
    case OP_glClientArray: {
        const unsigned char *base = cur_blob[w[5] & 15] + w[6];
        unsigned long p = (unsigned long)base - (unsigned long)w[7] * w[9];
        glVertexAttribPointer(w[0], (GLint)w[1], w[2], (GLboolean)w[3], (GLsizei)w[4], (const GLvoid *)p);
        client_pointer_calls++;
        break; }
    case OP_glDrawElements:
        glDrawElements(w[0], (GLsizei)w[1], w[2], w[4] ? (const GLvoid *)o->blob : (const GLvoid *)(unsigned long)w[3]); draws_done++; break;
    case OP_glDrawRangeElements:
        glDrawRangeElements(w[0], w[1], w[2], (GLsizei)w[3], w[4], w[6] ? (const GLvoid *)o->blob : (const GLvoid *)(unsigned long)w[5]); draws_done++; break;
    case OP_glDrawArrays: glDrawArrays(w[0], (GLint)w[1], (GLsizei)w[2]); draws_done++; break;
    case OP_glFlush: glFlush(); break;
    case OP_glGetError: (void)glGetError(); break;
    case OP_glGetIntegerv: glGetIntegerv(w[0], (GLint *)scratch_u); break;
    case OP_glGetFloatv: glGetFloatv(w[0], scratch_f); break;
    case OP_glGetBooleanv: glGetBooleanv(w[0], (GLboolean *)scratch_u); break;
    case OP_glIsEnabled: (void)glIsEnabled(w[0]); break;
    default: fprintf(stderr, "unknown op %u\n", o->op); exit(2);
    }
}
static int is_draw(unsigned op) { return op == OP_glDrawElements || op == OP_glDrawRangeElements || op == OP_glDrawArrays; }
static void run_ops(const Op *ops, unsigned n)
{
    unsigned i;
    if (gpu_check) {
        for (i = 0; i < n; i++) {
            do_op(&ops[i]);
            if (is_draw(ops[i].op)) {
                ProgTime *p = prog_entry(); GLint v = -1, fr = -1;
                CGLGetParameter(ctx, kCGLCPGPUVertexProcessing, &v);
                CGLGetParameter(ctx, kCGLCPGPUFragmentProcessing, &fr);
                p->draws++; p->verts += ops[i].op == OP_glDrawArrays ? ops[i].w[2] : ops[i].op == OP_glDrawElements ? ops[i].w[1] : ops[i].w[3];
                if (v < p->vgpu_min) p->vgpu_min = v;
                if (fr < p->fgpu_min) p->fgpu_min = fr;
            }
        }
        return;
    }
    if (!timing) { for (i = 0; i < n; i++) do_op(&ops[i]); return; }
    for (i = 0; i < n; i++) {
        unsigned long long t0 = now(), t1;
        do_op(&ops[i]);
        t1 = now();
        op_ticks[ops[i].op] += t1 - t0; op_count[ops[i].op]++;
        if (is_draw(ops[i].op)) {
            ProgTime *p = prog_entry();
            p->draws++; p->ticks += t1 - t0;
            p->verts += ops[i].op == OP_glDrawArrays ? ops[i].w[2] : ops[i].op == OP_glDrawElements ? ops[i].w[1] : ops[i].w[3];
        }
    }
}

/* ---- the file ---- */
static Op *all_ops;
static unsigned nops_pre, nops_frame;
static void load(const char *path)
{
    struct stat st; int fd = open(path, O_RDONLY); unsigned char *m, *p, *end; glr_header *h; unsigned i, n;
    if (fd < 0 || fstat(fd, &st)) { perror(path); exit(1); }
    m = (unsigned char *)mmap(0, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (m == (unsigned char *)MAP_FAILED) { perror("mmap"); exit(1); }
    h = (glr_header *)m;
    if (h->magic != GLR_MAGIC || h->version != GLR_VERSION) { fprintf(stderr, "not a capture file\n"); exit(1); }
    nops_pre = h->nops_pre; nops_frame = h->nops_frame; n = nops_pre + nops_frame;
    all_ops = (Op *)calloc(n, sizeof(Op));
    p = m + 32; end = m + st.st_size;
    for (i = 0; i < n; i++) {
        glr_rec *g;
        if (p + 16 > end) { fprintf(stderr, "truncated\n"); exit(1); }
        g = (glr_rec *)p; p += 16;
        all_ops[i].op = g->op; all_ops[i].nw = g->nw; all_ops[i].bloblen = g->bloblen;
        all_ops[i].w = (const unsigned *)p; p += (g->nw * 4 + 15) & ~15u;
        all_ops[i].blob = g->bloblen ? p : 0; p += (g->bloblen + 15) & ~15u;
        if (p > end || g->op >= OP_COUNT) { fprintf(stderr, "bad record %u\n", i); exit(1); }
    }
}

/* ---- statistics about the capture ---- */
static void stats(void)
{
    unsigned long cnt_pre[OP_COUNT], cnt_frame[OP_COUNT], bytes_pre = 0, bytes_frame = 0, tex_bytes = 0, idx_total = 0, client_idx = 0, client_bytes = 0;
    unsigned i, nd = 0, vp_changes = 0, fp_changes = 0, glsl_changes = 0, lv = 0, lf = 0, lg = 0, nhv = 0;
    unsigned long type_cnt[16];
    memset(cnt_pre, 0, sizeof cnt_pre); memset(cnt_frame, 0, sizeof cnt_frame); memset(type_cnt, 0, sizeof type_cnt);
    for (i = 0; i < nops_pre + nops_frame; i++) {
        const Op *o = &all_ops[i];
        if (i < nops_pre) { cnt_pre[o->op]++; bytes_pre += o->bloblen; }
        else { cnt_frame[o->op]++; bytes_frame += o->bloblen; }
        if (i < nops_pre && (o->op == OP_glTexImage2D || o->op == OP_glCompressedTexImage2D || o->op == OP_glTexImage3D || o->op == OP_glTexSubImage2D)) tex_bytes += o->bloblen;
        if (i >= nops_pre) {
            if (o->op == OP_glClientBlob) client_bytes += o->bloblen;
            if (is_draw(o->op)) {
                nd++;
                if (o->op == OP_glDrawElements) { idx_total += o->w[1]; if (o->w[4]) client_idx++; }
                else if (o->op == OP_glDrawRangeElements) { idx_total += o->w[3]; if (o->w[6]) client_idx++; }
                else idx_total += o->w[2];
                type_cnt[o->w[0] & 15]++;
            }
        }
    }
    printf("capture: %u setup records (%.1f MB of data, %.1f MB of it texture images), %u frame records (%.1f MB of data)\n",
        nops_pre, bytes_pre / 1048576.0, tex_bytes / 1048576.0, nops_frame, bytes_frame / 1048576.0);
    printf("frame: %u draws, %lu indices/vertices submitted, %lu draws with client indices, %.1f MB copied client vertex data\n",
        nd, idx_total, client_idx, client_bytes / 1048576.0);
    printf("frame calls by type:\n");
    for (i = 1; i < OP_COUNT; i++) if (cnt_frame[i]) printf("  %-34s %6lu\n", op_names[i], cnt_frame[i]);
    printf("setup calls (largest):\n");
    for (i = 1; i < OP_COUNT; i++) if (cnt_pre[i] > 100) printf("  %-34s %6lu\n", op_names[i], cnt_pre[i]);
}

/* ---- textures: -L drops the top mip levels at setup; the frame's texture working set ---- */
static unsigned skip_levels, drop_bytes, shrink_all, shrink_big;
static unsigned long tex_size[PMAX], tex_alloc[PMAX];
static unsigned *palloc(unsigned n);
static void analyze_textures(void)
{
    unsigned i, unit = 0, bound[8], k, used_n = 0, live_n = 0; unsigned char used[PMAX];
    unsigned long used_bytes = 0, live_bytes = 0;
    Op *ops = all_ops;
    memset(bound, 0, sizeof bound); memset(used, 0, sizeof used);
    for (i = 0; i < nops_pre + nops_frame; i++) {
        Op *o = &ops[i]; const unsigned *w = o->w;
        if (i == nops_pre) { /* frame starts: forget sizes of nothing, keep binding state */ }
        switch (o->op) {
        case OP_glActiveTextureARB: unit = (w[0] - GL_TEXTURE0_ARB) & 7; break;
        case OP_glBindTexture: bound[unit] = w[1]; break;
        case OP_glDeleteTextures: break;
        case OP_glTexImage2D: case OP_glCompressedTexImage2D: case OP_glTexImage3D: case OP_glTexSubImage2D:
            if (shrink_big && i < nops_pre && o->op == OP_glTexSubImage2D && w[4] >= 1024) { o->op = OP_NONE; o->nw = 0; o->bloblen = 0; o->blob = 0; break; }
            if (shrink_big && i < nops_pre && o->op == OP_glTexImage2D && !w[8] && (unsigned long)w[3] * w[4] * 4 >= (8ul << 20)) {
                /* -Z: a texture of 8 MB or more that has no data when it is made (the text atlas) is made 4x4 or GLR_ATLAS_EDGE */
                unsigned *nw = palloc(o->nw); memcpy(nw, w, o->nw * 4); { const char *e = getenv("GLR_ATLAS_EDGE"); nw[3] = nw[4] = e ? (unsigned)atoi(e) : 4; } o->w = nw; w = nw;
            }
            if (shrink_all && i < nops_pre) {
                /* -S: every texture with data becomes one 1x1 level (same objects, binds and parameters) */
                static const unsigned char white[16] = { 255, 255, 255, 255 };
                int has = o->op == OP_glTexImage2D ? w[8] : o->op == OP_glCompressedTexImage2D ? w[7] : o->op == OP_glTexImage3D ? w[9] : 1;
                if (o->op == OP_glTexSubImage2D || !has || w[1] != 0) {
                    if (o->op == OP_glTexSubImage2D || has) { o->op = OP_NONE; o->nw = 0; o->bloblen = 0; o->blob = 0; }
                } else {
                    unsigned *nw = palloc(10);
                    memset(nw, 0, 40);
                    nw[0] = w[0]; nw[1] = 0; nw[2] = 0x8058; nw[3] = 1; nw[4] = 1;
                    if (o->op == OP_glTexImage3D) { nw[5] = 1; nw[6] = 0; nw[7] = 0x1908; nw[8] = 0x1401; nw[9] = 1; o->nw = 10; }
                    else { nw[5] = 0; nw[6] = 0x1908; nw[7] = 0x1401; nw[8] = 1; o->op = OP_glTexImage2D; o->nw = 9; }
                    o->w = nw; o->blob = white; o->bloblen = 4;
                }
                break;
            }
            if (i < nops_pre && drop_bytes && o->bloblen > drop_bytes) {
                /* -X: the data of the big images is left out (the texture stays, empty) */
                if (o->op == OP_glTexSubImage2D) { o->op = OP_NONE; o->nw = 0; o->bloblen = 0; o->blob = 0; }
                else { unsigned *nw = palloc(o->nw); memcpy(nw, w, o->nw * 4);
                    nw[o->op == OP_glTexImage2D ? 8 : o->op == OP_glTexImage3D ? 9 : 7] = 0; o->w = nw; o->bloblen = 0; o->blob = 0;
                    if (o->op == OP_glCompressedTexImage2D) { o->op = OP_NONE; o->nw = 0; } }
                break;
            }
            if (i < nops_pre) {
                int has = o->op == OP_glTexImage2D ? w[8] : o->op == OP_glCompressedTexImage2D ? w[7] : o->op == OP_glTexImage3D ? w[9] : 1;
                if (skip_levels && has && w[1] < skip_levels) {
                    o->op = OP_NONE; o->nw = 0; o->bloblen = 0; o->blob = 0;
                    break;
                }
                if (skip_levels && has) { unsigned *nw = palloc(o->nw); memcpy(nw, w, o->nw * 4); nw[1] -= skip_levels; o->w = nw; w = nw; }
                if (bound[unit] < PMAX) { if (has) tex_size[bound[unit]] += o->bloblen; else if (o->op == OP_glTexImage2D) tex_alloc[bound[unit]] += (unsigned long)w[3] * w[4] * 4; }
            }
            break;
        case OP_glTexParameteri:
            if (shrink_all && (w[1] == 0x813D || (w[1] == 0x2801 && w[2] >= 0x2700 && w[2] <= 0x2703))) {
                unsigned *nw = palloc(o->nw); memcpy(nw, w, o->nw * 4); nw[2] = w[1] == 0x813D ? 0 : 0x2601; o->w = nw; break;
            }
            if (skip_levels && i < nops_pre && w[1] == 0x813D) { unsigned *nw = palloc(o->nw); memcpy(nw, w, o->nw * 4); nw[2] = w[2] > skip_levels ? w[2] - skip_levels : 0; o->w = nw; }
            break;
        case OP_glTexParameterf:
            if (shrink_all && w[1] == 0x813A) { unsigned *nw = palloc(o->nw); memcpy(nw, w, o->nw * 4); nw[2] = f2u_s(0.0f); o->w = nw; break; }
            if (skip_levels && i < nops_pre && w[1] == 0x813A) { unsigned *nw = palloc(o->nw); memcpy(nw, w, o->nw * 4); nw[2] = f2u_s(u2f(w[2]) - (float)skip_levels); o->w = nw; }
            break;
        default:
            if (i >= nops_pre && is_draw(o->op)) for (k = 0; k < 4; k++) if (bound[k] && bound[k] < PMAX && !used[bound[k]]) { used[bound[k]] = 1; used_n++; }
        }
    }
    for (i = 0; i < PMAX; i++) { if (tex_alloc[i] > tex_size[i]) tex_size[i] = tex_alloc[i]; if (tex_size[i]) { live_n++; live_bytes += tex_size[i]; if (used[i]) used_bytes += tex_size[i]; } }
    printf("textures%s: %u uploaded (%.1f MB); %u bound at a draw of the frame (%.1f MB, units 0-3)\n",
        skip_levels ? " after dropping top mip levels" : "", live_n, live_bytes / 1048576.0, used_n, used_bytes / 1048576.0);
}

/* ---- ablation plan ---- */
/* the vertex array state a draw depends on: tracked so that draws moved by the
   sort still find their buffers and pointers (and do not read from nowhere) */
typedef struct { unsigned en[16], kind[16], pw[16][7], arr, el, unit, tex[8][2]; } VState;
typedef struct { unsigned first, last; int tail; unsigned vp, fp, glsl; unsigned order; VState need; unsigned dtex[8][2]; } Group;
static void track(VState *v, const Op *o)
{
    const unsigned *w = o->w;
    switch (o->op) {
    case OP_glEnableVertexAttribArray: if (w[0] < 16) v->en[w[0]] = 1; break;
    case OP_glDisableVertexAttribArray: if (w[0] < 16) v->en[w[0]] = 0; break;
    case OP_glVertexAttribPointer: if (w[0] < 16) { unsigned k; v->kind[w[0]] = 1; for (k = 0; k < 7; k++) v->pw[w[0]][k] = w[k]; } break;
    case OP_glVertexAttribPointerClient: case OP_glClientArray: if (w[0] < 16) v->kind[w[0]] = 2; break;
    case OP_glActiveTextureARB: v->unit = (w[0] - GL_TEXTURE0_ARB) & 7; break;
    case OP_glBindTexture: v->tex[v->unit][0] = w[0]; v->tex[v->unit][1] = w[1]; break;
    case OP_glBindBuffer: if (w[0] == GL_ARRAY_BUFFER) v->arr = w[1]; else if (w[0] == GL_ELEMENT_ARRAY_BUFFER) v->el = w[1]; break;
    }
}
static unsigned abl_mask;
static const char abl_letters[] = "abcdefFgGhxzsStB";
#define AB(c) (abl_mask & (1u << (strchr(abl_letters, (c)) - abl_letters)))
static Op *plan, *base_plan;
static unsigned nplan, nbase;
typedef struct { unsigned char *data; unsigned size, cap; } Arena;
static Arena ibo_arena, vbo_arena, cmp_arena;
static unsigned *pool; static unsigned pool_used, pool_cap;
static unsigned *palloc(unsigned n)
{
    unsigned *r;
    if (pool_used + n > pool_cap) { pool_cap = pool_cap ? pool_cap * 2 : 1u << 20; pool = (unsigned *)malloc(pool_cap * 4); pool_used = 0; }
    r = pool + pool_used; pool_used += n; return r;
}
static unsigned arena_add(Arena *a, const void *d, unsigned n)
{
    unsigned at = (a->size + 15) & ~15u;
    if (at + n > a->cap) { a->cap = (at + n) * 2 + 4096; a->data = (unsigned char *)realloc(a->data, a->cap); }
    memset(a->data + a->size, 0, at - a->size);
    if (d) memcpy(a->data + at, d, n);
    a->size = at + n;
    return at;
}
static VState cur_state;
/* data placed so that at least `before` bytes of the arena lie ahead of it */
static unsigned arena_add_after(Arena *a, const void *d, unsigned n, unsigned before)
{
    unsigned at = a->size + before;
    unsigned r;
    if (before) arena_add(a, 0, before);
    r = arena_add(a, d, n);
    (void)at;
    return r;
}
static void emit(const Op *o) { plan[nplan++] = *o; track(&cur_state, o); }
static void emit_synth(unsigned op, unsigned nw, const unsigned *w)
{
    Op o; unsigned *p = palloc(nw ? nw : 1); unsigned i;
    for (i = 0; i < nw; i++) p[i] = w[i];
    o.op = op; o.nw = nw; o.bloblen = 0; o.w = p; o.blob = 0;
    emit(&o);
}
static void emit_bind_buffer(unsigned target, unsigned fake) { unsigned w[2]; w[0] = target; w[1] = fake; emit_synth(OP_glBindBuffer, 2, w); }

static void draw_range(const Op *d, unsigned *first, unsigned *last)
{
    if (d->op == OP_glDrawArrays) { *first = d->w[1]; *last = d->w[1] + d->w[2] - 1; }
    else if (d->op == OP_glDrawElements) { *first = d->w[5]; *last = d->w[6]; }
    else { *first = d->w[7]; *last = d->w[8]; }
}
static unsigned draw_count(const Op *d) { return d->op == OP_glDrawArrays ? d->w[2] : d->op == OP_glDrawElements ? d->w[1] : d->w[3]; }

static void complete_state(const Group *grp)
{
    const VState *need = &grp->need;
    unsigned i, w[8];
    for (i = 0; i < 16; i++) {
        if (need->en[i] != cur_state.en[i]) { w[0] = i; emit_synth(need->en[i] ? OP_glEnableVertexAttribArray : OP_glDisableVertexAttribArray, 1, w); }
        if (need->en[i] && need->kind[i] == 1 && (cur_state.kind[i] != 1 || memcmp(need->pw[i], cur_state.pw[i], 28))) {
            emit_bind_buffer(GL_ARRAY_BUFFER, need->pw[i][6]);
            emit_synth(OP_glVertexAttribPointer, 7, need->pw[i]);
        }
    }
    if (need->arr != cur_state.arr) emit_bind_buffer(GL_ARRAY_BUFFER, need->arr);
    if (need->el != cur_state.el) emit_bind_buffer(GL_ELEMENT_ARRAY_BUFFER, need->el);
    if (AB('t')) {
        for (i = 0; i < 4; i++) if (grp->dtex[i][1] != cur_state.tex[i][1] || grp->dtex[i][0] != cur_state.tex[i][0]) {
            if (cur_state.unit != i) { w[0] = GL_TEXTURE0_ARB + i; emit_synth(OP_glActiveTextureARB, 1, w); }
            w[0] = grp->dtex[i][0]; w[1] = grp->dtex[i][1]; emit_synth(OP_glBindTexture, 2, w);
        }
    }
}

/* one group: the calls leading to one draw (or the tail after the last) */
static void emit_group(const Op *fops, const Group *g, int explicit_binds)
{
    unsigned i, npend = 0; const Op *pend[16]; unsigned long slot_off[16]; int vbo_bound = 0;
    memset(slot_off, 0, sizeof slot_off);
    for (i = g->first; i <= g->last; i++) {
        const Op *o = &fops[i]; unsigned op = o->op;
        if (AB('a') && op == OP_glProgramEnvParameter4fvARB) continue;
        if (AB('B') && op == OP_glBindTexture) continue;
        if (AB('t') && !g->tail && (op == OP_glBindTexture || op == OP_glActiveTextureARB)) continue;
        if (AB('b') && (op == OP_glBindTexture || op == OP_glTexParameteri || op == OP_glTexParameterf || op == OP_glTexParameterfv
            || op == OP_glTexEnvi || op == OP_glTexEnvf)) continue;
        if (explicit_binds && !g->tail && (op == OP_glBindProgramARB || op == OP_glUseProgram)) continue;
        if ((AB('s') || AB('S')) && !AB('d') && (op == OP_glViewport || op == OP_glScissor)) {
            /* the render scale experiment: the same frame in 0.6 (s) or 0.3 (S) of the width and height */
            unsigned w[4]; float f = AB('s') ? 0.6f : 0.3f; unsigned k;
            for (k = 0; k < 4; k++) w[k] = (unsigned)((float)o->w[k] * f);
            emit_synth(op, 4, w); continue;
        }
        if (AB('d') && op == OP_glScissor) { unsigned w[4]; w[0] = 0; w[1] = 0; w[2] = 1; w[3] = 1; emit_synth(OP_glScissor, 4, w); continue; }
        if (AB('d') && op == OP_glDisable && o->w[0] == GL_SCISSOR_TEST) continue;
        if (op == OP_glClientBlob) {
            if (AB('z') || (AB('x') && g->vp < PMAX && vp_env[g->vp] >= 100)) continue;
            if (AB('f')) {
                unsigned j, need = 0, slot = o->w[0] & 15;
                for (j = i + 1; j <= g->last; j++) if (fops[j].op == OP_glClientArray && (fops[j].w[5] & 15) == slot) {
                    unsigned long m = (unsigned long)fops[j].w[7] * fops[j].w[9];
                    if (m > fops[j].w[6] && m - fops[j].w[6] > need) need = (unsigned)(m - fops[j].w[6]);
                }
                if (need > (8u << 20)) need = 0;   /* (a far-off first vertex: the offset wraps) */
                slot_off[slot] = arena_add_after(&vbo_arena, o->blob, o->bloblen, need);
                continue;
            }
            if (AB('F')) { cur_blob[o->w[0] & 15] = o->blob; continue; }   /* kept for the draw */
            emit(o); continue;
        }
        if (op == OP_glClientArray) {
            if (AB('z') || (AB('x') && g->vp < PMAX && vp_env[g->vp] >= 100)) continue;
            if (AB('f')) {
                unsigned w[7]; unsigned long p = slot_off[o->w[5] & 15] + o->w[6] - (unsigned long)o->w[7] * o->w[9];
                if (!vbo_bound) { emit_bind_buffer(GL_ARRAY_BUFFER, FAKE_VBO); vbo_bound = 1; }
                w[0] = o->w[0]; w[1] = o->w[1]; w[2] = o->w[2]; w[3] = o->w[3]; w[4] = o->w[4]; w[5] = (unsigned)p; w[6] = 0;
                emit_synth(OP_glVertexAttribPointer, 7, w);
                continue;
            }
            if (AB('F')) { if (npend < 16) { pend[npend] = o; npend++; } continue; }
            emit(o); continue;
        }
        if (is_draw(op)) {
            unsigned first, last, count = draw_count(o);
            if (AB('z') || (AB('x') && g->vp < PMAX && vp_env[g->vp] >= 100)) { npend = 0; continue; }
            draw_range(o, &first, &last);
            if (AB('F') && npend) {
                unsigned k, nv, v, sum = 0, offs[16], at, base_at;
                if (last == 0xFFFFFFFFu) last = first + count - 1;
                nv = last - first + 1;
                for (k = 0; k < npend; k++) { offs[k] = sum; sum += pend[k]->w[1] * type_bytes(pend[k]->w[2]); }
                {   /* (room for the vertices before the first, so that the base of the array is not below the buffer) */
                    unsigned before = first * sum;
                    if (before > (8u << 20)) before = 0;
                    at = arena_add_after(&cmp_arena, 0, nv * sum, before);
                    base_at = before ? at - first * sum : at - first * sum;   /* (wraps when before == 0) */
                }
                for (v = 0; v < nv; v++) for (k = 0; k < npend; k++) {
                    const Op *a = pend[k]; unsigned eb = a->w[1] * type_bytes(a->w[2]);
                    memcpy(cmp_arena.data + at + v * sum + offs[k], cur_blob[a->w[5] & 15] + a->w[6] + (unsigned long)v * a->w[9], eb);
                }
                emit_bind_buffer(GL_ARRAY_BUFFER, FAKE_CMP);
                for (k = 0; k < npend; k++) {
                    const Op *a = pend[k]; unsigned w[7];
                    w[0] = a->w[0]; w[1] = a->w[1]; w[2] = a->w[2]; w[3] = a->w[3]; w[4] = sum;
                    w[5] = base_at + offs[k]; w[6] = 0;   /* (wraps: the base the draw's indices count from) */
                    emit_synth(OP_glVertexAttribPointer, 7, w);
                }
            }
            npend = 0;
            if (AB('h') && op != OP_glDrawArrays) {
                unsigned w[3], nv;
                if (last == 0xFFFFFFFFu) { first = 0; nv = count; } else nv = last - first + 1;
                w[0] = o->w[0]; w[1] = first; w[2] = count < nv ? count : nv;
                emit_synth(OP_glDrawArrays, 3, w);
                continue;
            }
            if (AB('e') && ((op == OP_glDrawElements && o->w[4]) || (op == OP_glDrawRangeElements && o->w[6]))) {
                unsigned w[9], k, at = arena_add(&ibo_arena, o->blob, o->bloblen);
                for (k = 0; k < o->nw; k++) w[k] = o->w[k];
                if (op == OP_glDrawElements) { w[3] = at; w[4] = 0; } else { w[5] = at; w[6] = 0; }
                emit_bind_buffer(GL_ELEMENT_ARRAY_BUFFER, FAKE_IBO);
                emit_synth(op, o->nw, w);
                continue;
            }
        }
        emit(o);
    }
}
static int group_cmp_vp(const void *a, const void *b)
{
    const Group *x = (const Group *)a, *y = (const Group *)b;
    if (x->vp != y->vp) return x->vp < y->vp ? -1 : 1;
    if (x->fp != y->fp) return x->fp < y->fp ? -1 : 1;
    if (x->glsl != y->glsl) return x->glsl < y->glsl ? -1 : 1;
    return x->order < y->order ? -1 : x->order > y->order;
}
static int group_cmp_fp(const void *a, const void *b)
{
    const Group *x = (const Group *)a, *y = (const Group *)b;
    if (x->fp != y->fp) return x->fp < y->fp ? -1 : 1;
    if (x->vp != y->vp) return x->vp < y->vp ? -1 : 1;
    if (x->glsl != y->glsl) return x->glsl < y->glsl ? -1 : 1;
    return x->order < y->order ? -1 : x->order > y->order;
}
static int group_cmp_tex(const void *a, const void *b)
{
    const Group *x = (const Group *)a, *y = (const Group *)b; unsigned i;
    for (i = 0; i < 4; i++) {
        if (x->dtex[i][1] != y->dtex[i][1]) return x->dtex[i][1] < y->dtex[i][1] ? -1 : 1;
    }
    return group_cmp_vp(a, b);
}
static void build_plans(void)
{
    const Op *fops = all_ops + nops_pre;
    Group *groups = (Group *)calloc(nops_frame + 2, sizeof(Group));
    unsigned ng = 0, i, gs = 0, prefix_end = 0, first_draw = nops_frame, last_clear = 0, have_clear = 0;
    unsigned vp, fp, gl;
    VState sim, gstart;
    int explicit_binds = AB('c') || AB('g') || AB('G') || AB('t');
    nbase = nops_frame;
    base_plan = (Op *)malloc((nops_frame + 1) * sizeof(Op));
    memcpy(base_plan, fops, nops_frame * sizeof(Op));
    if (!abl_mask) return;
    /* the programs bound when the frame starts */
    vp = fp = gl = 0;
    memset(&sim, 0, sizeof sim);
    for (i = 0; i < nops_pre; i++) {
        const Op *o = &all_ops[i];
        track(&sim, o);
        if (o->op == OP_glBindProgramARB) { if (o->w[0] == GL_FRAGMENT_PROGRAM_ARB) fp = o->w[1]; else vp = o->w[1]; }
        else if (o->op == OP_glUseProgram) gl = o->w[0];
    }
    for (i = 0; i < nops_frame; i++) if (is_draw(fops[i].op)) { first_draw = i; break; }
    for (i = 0; i < first_draw; i++) if (fops[i].op == OP_glClear) { last_clear = i; have_clear = 1; }
    prefix_end = have_clear ? last_clear + 1 : 0;
    cur_state = sim; gstart = sim;
    for (i = 0; i < nops_frame; i++) {
        const Op *o = &fops[i];
        track(&sim, o);
        if (o->op == OP_glBindProgramARB) { if (o->w[0] == GL_FRAGMENT_PROGRAM_ARB) fp = o->w[1]; else vp = o->w[1]; }
        else if (o->op == OP_glUseProgram) gl = o->w[0];
        if (i < prefix_end) { gs = prefix_end; gstart = sim; continue; }
        if (is_draw(o->op)) {
            groups[ng].first = gs; groups[ng].last = i; groups[ng].vp = vp; groups[ng].fp = fp; groups[ng].glsl = gl;
            groups[ng].need = gstart; memcpy(groups[ng].dtex, sim.tex, sizeof sim.tex); groups[ng].order = ng; ng++; gs = i + 1; gstart = sim;
        }
    }
    if (gs < nops_frame) { groups[ng].need = gstart; groups[ng].first = gs; groups[ng].last = nops_frame - 1; groups[ng].tail = 1; groups[ng].order = ng; groups[ng].vp = vp; groups[ng].fp = fp; groups[ng].glsl = gl; ng++; }
    {
        unsigned ndraw = ng && groups[ng - 1].tail ? ng - 1 : ng;
        if (AB('g')) qsort(groups, ndraw, sizeof(Group), group_cmp_vp);
        if (AB('G')) qsort(groups, ndraw, sizeof(Group), group_cmp_fp);
        if (AB('t')) qsort(groups, ndraw, sizeof(Group), group_cmp_tex);
        plan = (Op *)malloc((nops_frame * 3 + ng * 16 + 64) * sizeof(Op));
        nplan = 0;
        if (AB('d')) { unsigned w = GL_SCISSOR_TEST; emit_synth(OP_glEnable, 1, &w); }
        if (prefix_end) { Group pg; memset(&pg, 0, sizeof pg); pg.first = 0; pg.last = prefix_end - 1; pg.tail = 1; emit_group(fops, &pg, 0); }
        {
            unsigned lv = 0xFFFFFFFFu, lf = 0xFFFFFFFFu, lg = 0xFFFFFFFFu, k;
            for (k = 0; k < ng; k++) {
                Group g = groups[k];
                if (AB('c') && ndraw) { g.vp = groups[0].vp; g.fp = groups[0].fp; g.glsl = groups[0].glsl; }
                if (AB('g') || AB('G') || AB('t')) complete_state(&g);
                if (explicit_binds && !g.tail) {
                    unsigned w[2];
                    if (g.vp != lv) { w[0] = GL_VERTEX_PROGRAM_ARB; w[1] = g.vp; emit_synth(OP_glBindProgramARB, 2, w); lv = g.vp; }
                    if (g.fp != lf) { w[0] = GL_FRAGMENT_PROGRAM_ARB; w[1] = g.fp; emit_synth(OP_glBindProgramARB, 2, w); lf = g.fp; }
                    if (g.glsl != lg) { w[0] = g.glsl; emit_synth(OP_glUseProgram, 1, w); lg = g.glsl; }
                }
                emit_group(fops, &g, explicit_binds);
            }
        }
    }
    free(groups);
}
static void upload_arena(Arena *a, GLenum target, unsigned fake)
{
    GLuint b = 0;
    if (!a->size) return;
    glGenBuffers(1, &b);
    glBindBuffer(target, b);
    glBufferData(target, (GLsizeiptr)a->size, a->data, GL_STATIC_DRAW);
    glBindBuffer(target, 0);
    mapset(C_BUF, fake, b);
    printf("static buffer %s: %.1f MB\n", fake == FAKE_IBO ? "indices" : fake == FAKE_VBO ? "vertices" : "repacked vertices", a->size / 1048576.0);
}

/* ---- running ---- */
static void make_context(void)
{
    CGLPixelFormatObj pf; GLint n; CGLError e;
    CGLPixelFormatAttribute attr[] = { kCGLPFAAccelerated, kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
        kCGLPFADepthSize, (CGLPixelFormatAttribute)24, (CGLPixelFormatAttribute)0 };
    e = CGLChoosePixelFormat(attr, &pf, &n);
    if (e || !pf) { fprintf(stderr, "pixel format: %s\n", CGLErrorString(e)); exit(1); }
    e = CGLCreateContext(pf, NULL, &ctx);
    if (e || !ctx) { fprintf(stderr, "context: %s\n", CGLErrorString(e)); exit(1); }
    e = CGLSetCurrentContext(ctx);
    if (e) { fprintf(stderr, "make current: %s\n", CGLErrorString(e)); exit(1); }
    printf("GL_RENDERER %s | %s\n", (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION));
}
static double cpu_seconds(void)
{
    struct rusage r; getrusage(RUSAGE_SELF, &r);
    return r.ru_utime.tv_sec + r.ru_utime.tv_usec * 1e-6 + r.ru_stime.tv_sec + r.ru_stime.tv_usec * 1e-6;
}
#define WARM 3
static void phase(char kind, unsigned frames, const char *label)
{
    static double per_frame[4096]; unsigned f, counted = 0; unsigned long long t_start = 0, sub = 0, wait = 0, t0, t1, t2; double cpu0 = 0;
    timing = kind == 'T';
    if (kind == 'V') {
        unsigned i, sw_v = 0, sw_f = 0; unsigned long dv = 0, df = 0, dall = 0;
        gpu_check = 1; nptab = 0;
        run_ops(base_plan, nbase);
        glFinish();
        gpu_check = 0;
        for (i = 0; i < nptab; i++) {
            dall += ptab[i].draws;
            if (ptab[i].vgpu_min == 0) { sw_v++; dv += ptab[i].draws; }
            if (ptab[i].fgpu_min == 0) { sw_f++; df += ptab[i].draws; }
            if (ptab[i].vgpu_min != 1 || ptab[i].fgpu_min != 1)
                printf("GPUCHECK vp %u fp %u: %lu draws, %.0f verts/draw, vertex on GPU=%d fragment on GPU=%d\n", ptab[i].vp, ptab[i].fp,
                    ptab[i].draws, (double)ptab[i].verts / ptab[i].draws, ptab[i].vgpu_min, ptab[i].fgpu_min);
        }
        printf("GPUCHECK %lu draws, %lu with vertex processing on the CPU (%u program pairs), %lu with fragment processing on the CPU (%u pairs)\n",
            dall, dv, sw_v, df, sw_f);
        return;
    }
    memset(op_ticks, 0, sizeof op_ticks); memset(op_count, 0, sizeof op_count); nptab = 0;
    client_pointer_calls = 0; draws_done = 0;
    for (f = 0; f < frames; f++) {
        int use_abl = abl_mask && f > 0;
        if (f == WARM) { glFinish(); t_start = now(); cpu0 = cpu_seconds(); client_pointer_calls = 0; draws_done = 0;
            memset(op_ticks, 0, sizeof op_ticks); memset(op_count, 0, sizeof op_count); nptab = 0; }
        t0 = now();
        run_ops(use_abl ? plan : base_plan, use_abl ? nplan : nbase);
        t1 = now();
        if (kind != 'P') glFinish();
        t2 = now();
        if (f >= WARM) { sub += t1 - t0; wait += t2 - t1; if (counted < 4096) per_frame[counted] = ms_of(t2 - t0); counted++; }
    }
    glFinish();
    {
        unsigned long long t_end = now(); double cpu = cpu_seconds() - cpu0;
        double wall = ms_of(t_end - t_start) / counted;
        if (kind != 'P' && counted > 2 && counted <= 4096) {
            unsigned a, b; double mn, md, mx;
            for (a = 0; a < counted; a++) for (b = a + 1; b < counted; b++) if (per_frame[b] < per_frame[a]) { double t = per_frame[a]; per_frame[a] = per_frame[b]; per_frame[b] = t; }
            mn = per_frame[0]; md = per_frame[counted / 2]; mx = per_frame[counted - 1];
            printf("FRAMES phase=%c wall_ms min %.2f median %.2f max %.2f\n", kind, mn, md, mx);
        }
        printf("RESULT %s abl=%s phase=%c frames=%u submit_ms=%.2f wait_ms=%.2f wall_ms=%.2f cpu_ms=%.2f draws=%lu clientptr=%lu\n",
            label, abl_mask ? "on" : "none", kind, counted, ms_of(sub) / counted, ms_of(wait) / counted, wall,
            cpu * 1000.0 / counted, draws_done / counted, client_pointer_calls / counted);
    }
    if (kind == 'T') {
        unsigned i, j; unsigned order[OP_COUNT]; unsigned m = 0; double total = 0;
        for (i = 1; i < OP_COUNT; i++) if (op_count[i]) order[m++] = i;
        for (i = 0; i < m; i++) for (j = i + 1; j < m; j++) if (op_ticks[order[j]] > op_ticks[order[i]]) { unsigned t = order[i]; order[i] = order[j]; order[j] = t; }
        for (i = 0; i < m; i++) total += ms_of(op_ticks[order[i]]);
        printf("PEROP per frame (timed calls, %.2f ms total)\n", total / counted);
        for (i = 0; i < m; i++) {
            unsigned o = order[i]; double ms = ms_of(op_ticks[o]) / counted;
            if (ms < 0.01 && i > 15) break;
            printf("PEROP %-32s calls/frame %7.1f  ms/frame %8.3f  us/call %8.2f\n", op_names[o], (double)op_count[o] / counted, ms,
                1000.0 * ms_of(op_ticks[o]) / op_count[o]);
        }
        /* draws by program */
        for (i = 0; i < nptab; i++) for (j = i + 1; j < nptab; j++) if (ptab[j].ticks > ptab[i].ticks) { ProgTime t = ptab[i]; ptab[i] = ptab[j]; ptab[j] = t; }
        {
            double hv_ms = 0, lo_ms = 0; unsigned long hv_n = 0, lo_n = 0;
            for (i = 0; i < nptab; i++) {
                if (ptab[i].vp < PMAX && vp_env[ptab[i].vp] >= 100) { hv_ms += ms_of(ptab[i].ticks); hv_n += ptab[i].draws; }
                else { lo_ms += ms_of(ptab[i].ticks); lo_n += ptab[i].draws; }
            }
            printf("PERPROG draw calls of vertex programs reading >=100 env params: %.1f draws/frame, %.2f ms/frame (%.1f us/draw); others: %.1f draws/frame, %.2f ms/frame (%.1f us/draw)\n",
                (double)hv_n / counted, hv_ms / counted, hv_n ? 1000.0 * hv_ms / hv_n : 0.0, (double)lo_n / counted, lo_ms / counted, lo_n ? 1000.0 * lo_ms / lo_n : 0.0);
        }
        for (i = 0; i < nptab && i < 25; i++)
            printf("PERPROG vp %4u (env refs %3u, %3u ops) fp %4u (%3u ops) glsl %u: draws/frame %6.1f  draw-call ms/frame %7.3f  us/draw %7.1f  verts/draw %7.1f\n",
                ptab[i].vp, ptab[i].vp < PMAX ? vp_env[ptab[i].vp] : 0, ptab[i].vp < PMAX ? prog_ops[ptab[i].vp] : 0,
                ptab[i].fp, ptab[i].fp < PMAX ? prog_ops[ptab[i].fp] : 0, ptab[i].glsl,
                (double)ptab[i].draws / counted, ms_of(ptab[i].ticks) / counted, 1000.0 * ms_of(ptab[i].ticks) / ptab[i].draws,
                (double)ptab[i].verts / ptab[i].draws);
    }
    timing = 0;
}

int main(int argc, char **argv)
{
    const char *path = 0, *modes = "PFT", *out = 0; unsigned frames = 100, i; int verify = 0, quiet = 0; const char *abl = "";
    unsigned long long t0;
    setvbuf(stdout, NULL, _IOLBF, 0);
    for (i = 1; i < (unsigned)argc; i++) {
        if (!strcmp(argv[i], "-n") && i + 1 < (unsigned)argc) frames = (unsigned)atoi(argv[++i]);
        else if (!strcmp(argv[i], "-A") && i + 1 < (unsigned)argc) abl = argv[++i];
        else if (!strcmp(argv[i], "-m") && i + 1 < (unsigned)argc) modes = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < (unsigned)argc) { out = argv[++i]; verify = 1; }
        else if (!strcmp(argv[i], "-L") && i + 1 < (unsigned)argc) skip_levels = (unsigned)atoi(argv[++i]);
        else if (!strcmp(argv[i], "-S")) shrink_all = 1;
        else if (!strcmp(argv[i], "-Z")) { shrink_big = 1; if (!drop_bytes) drop_bytes = 4000000; }
        else if (!strcmp(argv[i], "-X") && i + 1 < (unsigned)argc) drop_bytes = (unsigned)atoi(argv[++i]);
        else if (!strcmp(argv[i], "-v")) verify = 1;
        else if (!strcmp(argv[i], "-q")) quiet = 1;
        else if (argv[i][0] != '-') path = argv[i];
        else { fprintf(stderr, "bad option %s\n", argv[i]); return 2; }
    }
    if (!path) { fprintf(stderr, "usage: gl_replay capture [-n frames] [-A ablations] [-m PFT] [-v] [-o ppm] [-q]\n"); return 2; }
    for (; *abl; abl++) {
        const char *at = strchr(abl_letters, *abl);
        if (!at) { fprintf(stderr, "unknown ablation %c\n", *abl); return 2; }
        abl_mask |= 1u << (at - abl_letters);
    }
    mach_timebase_info(&tb); ns_per_tick = (double)tb.numer / tb.denom;
    for (i = 0; i < C_COUNT; i++) idmap[i] = (unsigned *)calloc(MAPMAX, 4);
    make_context();
    load(path);
    if (!quiet) stats();
    {
        unsigned long long t_an = now();
        analyze_textures();
        (void)t_an;
    }
    t0 = now();
    run_ops(all_ops, nops_pre);
    glFinish();
    errors_checked = 1;
    printf("setup replayed in %.0f ms; GL error 0x%x\n", ms_of(now() - t0), (unsigned)glGetError());
    build_plans();
    if (abl_mask) {
        upload_arena(&ibo_arena, GL_ELEMENT_ARRAY_BUFFER, FAKE_IBO);
        upload_arena(&vbo_arena, GL_ARRAY_BUFFER, FAKE_VBO);
        upload_arena(&cmp_arena, GL_ARRAY_BUFFER, FAKE_CMP);
        printf("ablated frame: %u calls (recorded frame %u)\n", nplan, nbase);
    }
    for (; *modes; modes++) phase(*modes, frames, "replay");
    if (verify) {
        unsigned char *px = (unsigned char *)malloc(640 * 480 * 4); unsigned long h = 5381; unsigned k;
        glReadPixels(0, 0, 640, 480, GL_RGBA, GL_UNSIGNED_BYTE, px);
        for (k = 0; k < 640 * 480 * 4; k++) h = h * 33 + px[k];
        printf("CHECKSUM %08lx GL error 0x%x\n", h & 0xFFFFFFFFul, (unsigned)glGetError());
        if (out) {
            FILE *f = fopen(out, "wb"); unsigned y;
            if (f) { fprintf(f, "P6\n640 480\n255\n"); for (y = 0; y < 480; y++) for (k = 0; k < 640; k++) fwrite(px + ((479 - y) * 640 + k) * 4, 1, 3, f); fclose(f); }
        }
    }
    return 0;
}
