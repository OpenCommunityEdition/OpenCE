/* Research probe: vertex/index submission paths on the legacy Mac GL stack
   (Apple GL on PowerPC G4, Radeon 9550/9600).  Hidden CGL context, 256x256 FBO,
   fixed-function pipeline, 250 draws per frame; every triangle is about a pixel
   wide, so the figures are submission cost, not fill.

   usage: probe [groups] [name-substring]      groups: s (static), d (dynamic), default sd
   env:   FRAMES (default 30 per repetition), REPS (default 2)

   Static workload: 250 meshes of 50..2000 indices (8 sizes mixed), 32-byte vertices
   (position, normal, texcoord), vertex pointers re-set for every draw.
   Dynamic workload: 250 draws of 4..200 vertices (quads, 6 indices per quad) whose
   vertices are rewritten by the CPU every frame (HUD / particle-like).

   Columns: submit = CPU us per draw (frame loop before glFinish, fill included for
   dynamic); sync = ms per frame including glFinish every frame; pipe = ms per frame
   with one glFinish per repetition (fences protect VAR data); sys = kernel (system)
   us per draw from getrusage over the synchronous run; px = lit pixels of one
   check frame (equal within a group unless a path draws something else). */
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#define RT 256
#define NDRAW 250
#define VSTRIDE 32
#define PAGE 4096

static double now_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1.0e6 + tv.tv_usec;
}
static double sys_us(void)
{
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    return r.ru_stime.tv_sec * 1.0e6 + r.ru_stime.tv_usec;
}

static int g_frames = 30, g_reps = 2;
static unsigned rng_state = 12345;
static unsigned rnd(void) { rng_state = rng_state * 1103515245u + 12345u; return (rng_state >> 8) & 0xffffff; }

static int has_var, has_elem, has_vao, has_fence;

/* ---- static workload ---- */
typedef struct { int nidx, nv, voff, ioff; } mesh_t;   /* voff, ioff in bytes */
static mesh_t mesh[NDRAW];
static unsigned char *vblock;      /* page aligned: vertices, then indices (VAR image) */
static unsigned char *master_v;    /* vertex source */
static unsigned short *idx_client; /* client index copy */
static int vtotal, itotal, ibase;  /* bytes; ibase = index offset inside the VAR image */
static size_t var_size;

/* ---- dynamic workload ---- */
static int dyn_nv[NDRAW], dyn_off[NDRAW], dyn_total;
static unsigned short quad_idx[300];
static int dyn_half;               /* bytes of one VAR half: index area + vertices */
#define DYN_IDX_AREA PAGE

static GLuint vbo_s, ebo_s, ebo_m[NDRAW], vbo_stream, vao[2 * NDRAW], fence[2];
static void *dyn_cpu, *scratch;

static void fill_dyn(float *d, int i)
{
    int v, n = dyn_nv[i];
    float cx = (float)(i % 16) / 16.0f * 1.8f - 0.9f, cy = (float)(i / 16) / 16.0f * 1.8f - 0.9f;
    for (v = 0; v < n; v++) {
        int q = v >> 2, c = v & 3;
        d[0] = cx + ((c & 1) ? 0.008f : 0.0f) + q * 0.0003f;
        d[1] = cy + ((c & 2) ? 0.008f : 0.0f);
        d[2] = 0.0f; d[3] = 0.0f; d[4] = 0.0f; d[5] = 1.0f;
        d[6] = (float)(c & 1); d[7] = (float)(c >> 1);
        d += 8;
    }
}

static void build_data(void)
{
    static const int tab[8] = { 50, 120, 240, 420, 700, 1000, 1500, 2000 };
    int i, k;
    unsigned short *idx;
    vtotal = itotal = 0;
    for (i = 0; i < NDRAW; i++) {
        int n = tab[(i * 7 + 3) & 7];
        n -= n % 3;
        mesh[i].nidx = n;
        mesh[i].nv = n / 4 + 4;
        mesh[i].voff = vtotal;
        mesh[i].ioff = itotal;
        vtotal += (mesh[i].nv * VSTRIDE + 31) & ~31;
        itotal += (n * 2 + 31) & ~31;
    }
    ibase = (vtotal + PAGE - 1) & ~(PAGE - 1);
    var_size = ((size_t)ibase + itotal + PAGE - 1) & ~(size_t)(PAGE - 1);
    if (var_size < (size_t)2 * 1704000) var_size = (size_t)2 * 1704000;   /* room for two dynamic halves */
    vblock = (unsigned char *)valloc(var_size);   /* page aligned (no posix_memalign on 10.4) */
    memset(vblock, 0, var_size);
    master_v = (unsigned char *)malloc(vtotal);
    idx_client = (unsigned short *)malloc(itotal);
    memset(idx_client, 0, itotal);
    for (i = 0; i < NDRAW; i++) {
        float cx = (float)(i % 16) / 16.0f * 1.8f - 0.9f, cy = (float)(i / 16) / 16.0f * 1.8f - 0.9f;
        float *v = (float *)(vblock + mesh[i].voff);
        for (k = 0; k < mesh[i].nv; k++, v += 8) {
            v[0] = cx + ((int)(rnd() % 100) - 50) * 0.0002f;
            v[1] = cy + ((int)(rnd() % 100) - 50) * 0.0002f;
            v[2] = 0.0f; v[3] = 0.0f; v[4] = 0.0f; v[5] = 1.0f;
            v[6] = (rnd() & 255) / 255.0f; v[7] = (rnd() & 255) / 255.0f;
        }
        idx = (unsigned short *)((unsigned char *)idx_client + mesh[i].ioff);
        for (k = 0; k < mesh[i].nidx; k++)
            idx[k] = (unsigned short)(((k / 3) + rnd() % 8) % mesh[i].nv);
    }
    memcpy(master_v, vblock, vtotal);
    memcpy(vblock + ibase, idx_client, itotal);
    /* dynamic */
    dyn_total = 0;
    for (i = 0; i < NDRAW; i++) {
        dyn_nv[i] = 4 * (1 + (i * 37) % 50);
        dyn_off[i] = dyn_total;
        dyn_total += dyn_nv[i] * VSTRIDE;
    }
    for (i = 0; i < 50; i++) {
        static const unsigned short q[6] = { 0, 1, 2, 2, 1, 3 };
        for (k = 0; k < 6; k++) quad_idx[i * 6 + k] = (unsigned short)(i * 4 + q[k]);
    }
    dyn_half = DYN_IDX_AREA + ((dyn_total + PAGE - 1) & ~(PAGE - 1));
    dyn_cpu = malloc(dyn_total);
    scratch = malloc(200 * VSTRIDE);
}

static void ptrs(const unsigned char *p)
{
    glVertexPointer(3, GL_FLOAT, VSTRIDE, p);
    glNormalPointer(GL_FLOAT, VSTRIDE, p + 12);
    glTexCoordPointer(2, GL_FLOAT, VSTRIDE, p + 24);
}
static void arrays_on(int on)
{
    if (on) {
        glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    } else {
        glDisableClientState(GL_VERTEX_ARRAY); glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
}

static void reset_state(void)
{
    glBindVertexArrayAPPLE(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    if (has_var) glDisableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
    if (has_elem) glDisableClientState(GL_ELEMENT_ARRAY_APPLE);
    arrays_on(1);
}

static void var_on(GLenum storage, void *p, size_t n)
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glVertexArrayParameteriAPPLE(GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, (GLint)storage);
    glVertexArrayRangeAPPLE((GLsizei)n, p);
    glEnableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
    glFlushVertexArrayRangeAPPLE((GLsizei)n, p);
}

/* ================= static variants ================= */
static void st_vbo_client(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        ptrs((const unsigned char *)0 + mesh[i].voff);
        glDrawRangeElements(GL_TRIANGLES, 0, mesh[i].nv - 1, mesh[i].nidx, GL_UNSIGNED_SHORT,
                            (const unsigned char *)idx_client + mesh[i].ioff);
    }
}
static void st_vbo_ebo(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        ptrs((const unsigned char *)0 + mesh[i].voff);
        glDrawRangeElements(GL_TRIANGLES, 0, mesh[i].nv - 1, mesh[i].nidx, GL_UNSIGNED_SHORT,
                            (const unsigned char *)0 + mesh[i].ioff);
    }
}
static void st_vbo_ebo_mesh(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        ptrs((const unsigned char *)0 + mesh[i].voff);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_m[i]);
        glDrawRangeElements(GL_TRIANGLES, 0, mesh[i].nv - 1, mesh[i].nidx, GL_UNSIGNED_SHORT, (const void *)0);
    }
}
static void st_var_client(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        ptrs(vblock + mesh[i].voff);
        glDrawRangeElements(GL_TRIANGLES, 0, mesh[i].nv - 1, mesh[i].nidx, GL_UNSIGNED_SHORT,
                            (const unsigned char *)idx_client + mesh[i].ioff);
    }
}
static void st_var_elem(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        ptrs(vblock + mesh[i].voff);
        glDrawRangeElementArrayAPPLE(GL_TRIANGLES, 0, mesh[i].nv - 1, mesh[i].ioff / 2, mesh[i].nidx);
    }
}
static void st_vao_client(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        glBindVertexArrayAPPLE(vao[i]);
        glDrawRangeElements(GL_TRIANGLES, 0, mesh[i].nv - 1, mesh[i].nidx, GL_UNSIGNED_SHORT,
                            (const unsigned char *)idx_client + mesh[i].ioff);
    }
}
static void st_vao_ebo(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        glBindVertexArrayAPPLE(vao[i]);
        glDrawRangeElements(GL_TRIANGLES, 0, mesh[i].nv - 1, mesh[i].nidx, GL_UNSIGNED_SHORT,
                            (const unsigned char *)0 + mesh[i].ioff);
    }
}
static void st_vao_ebo_mesh(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        glBindVertexArrayAPPLE(vao[i]);
        glDrawRangeElements(GL_TRIANGLES, 0, mesh[i].nv - 1, mesh[i].nidx, GL_UNSIGNED_SHORT, (const void *)0);
    }
}

static void su_vbo(void) { glBindBuffer(GL_ARRAY_BUFFER, vbo_s); }
static void su_vbo_ebo(void) { glBindBuffer(GL_ARRAY_BUFFER, vbo_s); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_s); }
static void su_var_shared(void) { var_on(GL_STORAGE_SHARED_APPLE, vblock, var_size); }
static void su_var_cached(void) { var_on(GL_STORAGE_CACHED_APPLE, vblock, var_size); }
static void su_var_shared_elem(void)
{
    var_on(GL_STORAGE_SHARED_APPLE, vblock, var_size);
    glEnableClientState(GL_ELEMENT_ARRAY_APPLE);
    glElementPointerAPPLE(GL_UNSIGNED_SHORT, vblock + ibase);
}
static void su_var_cached_elem(void)
{
    var_on(GL_STORAGE_CACHED_APPLE, vblock, var_size);
    glEnableClientState(GL_ELEMENT_ARRAY_APPLE);
    glElementPointerAPPLE(GL_UNSIGNED_SHORT, vblock + ibase);
}
/* VAO setup: mode 0 vbo pointers, 1 + shared EBO, 2 + per-mesh EBO */
static void vao_build(int mode)
{
    int i;
    glGenVertexArraysAPPLE(NDRAW, vao);
    for (i = 0; i < NDRAW; i++) {
        glBindVertexArrayAPPLE(vao[i]);
        arrays_on(1);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_s);
        ptrs((const unsigned char *)0 + mesh[i].voff);
        if (mode == 1) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_s);
        if (mode == 2) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_m[i]);
    }
    glBindVertexArrayAPPLE(0);
}
static void su_vao0(void) { vao_build(0); }
static void su_vao1(void) { vao_build(1); }
static void su_vao2(void) { vao_build(2); }
static void td_vao(void) { glBindVertexArrayAPPLE(0); glDeleteVertexArraysAPPLE(NDRAW, vao); }
static void td_vao2(void) { glBindVertexArrayAPPLE(0); glDeleteVertexArraysAPPLE(2 * NDRAW, vao); }

/* ================= dynamic variants ================= */
static void dyn_draw_client(int i, const unsigned char *base)
{
    ptrs(base + dyn_off[i]);
    glDrawRangeElements(GL_TRIANGLES, 0, dyn_nv[i] - 1, dyn_nv[i] / 4 * 6, GL_UNSIGNED_SHORT, quad_idx);
}
static void dy_client(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) fill_dyn((float *)((unsigned char *)dyn_cpu + dyn_off[i]), i);
    for (i = 0; i < NDRAW; i++) dyn_draw_client(i, (unsigned char *)dyn_cpu);
}
static void dy_fill_only(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) fill_dyn((float *)((unsigned char *)dyn_cpu + dyn_off[i]), i);
}
static void dy_orphan_sub(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        int bytes = dyn_nv[i] * VSTRIDE;
        fill_dyn((float *)scratch, i);
        glBufferData(GL_ARRAY_BUFFER, bytes, NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, scratch);
        ptrs((const unsigned char *)0);
        glDrawRangeElements(GL_TRIANGLES, 0, dyn_nv[i] - 1, dyn_nv[i] / 4 * 6, GL_UNSIGNED_SHORT, quad_idx);
    }
}
static void dy_bufdata(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        fill_dyn((float *)scratch, i);
        glBufferData(GL_ARRAY_BUFFER, dyn_nv[i] * VSTRIDE, scratch, GL_STREAM_DRAW);
        ptrs((const unsigned char *)0);
        glDrawRangeElements(GL_TRIANGLES, 0, dyn_nv[i] - 1, dyn_nv[i] / 4 * 6, GL_UNSIGNED_SHORT, quad_idx);
    }
}
static void dy_orphan_map(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) {
        int bytes = dyn_nv[i] * VSTRIDE;
        void *p;
        glBufferData(GL_ARRAY_BUFFER, bytes, NULL, GL_STREAM_DRAW);
        p = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        if (!p) { fprintf(stderr, "map failed\n"); exit(2); }
        fill_dyn((float *)p, i);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        ptrs((const unsigned char *)0);
        glDrawRangeElements(GL_TRIANGLES, 0, dyn_nv[i] - 1, dyn_nv[i] / 4 * 6, GL_UNSIGNED_SHORT, quad_idx);
    }
}
static void dy_frame_vbo(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) fill_dyn((float *)((unsigned char *)dyn_cpu + dyn_off[i]), i);
    glBufferData(GL_ARRAY_BUFFER, dyn_total, NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dyn_total, dyn_cpu);
    for (i = 0; i < NDRAW; i++) dyn_draw_client(i, (const unsigned char *)0);
}
static void dy_frame_map(int f)
{
    int i;
    unsigned char *p;
    glBufferData(GL_ARRAY_BUFFER, dyn_total, NULL, GL_STREAM_DRAW);
    p = (unsigned char *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if (!p) { fprintf(stderr, "map failed\n"); exit(2); }
    for (i = 0; i < NDRAW; i++) fill_dyn((float *)(p + dyn_off[i]), i);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    for (i = 0; i < NDRAW; i++) dyn_draw_client(i, (const unsigned char *)0);
}
static void dy_frame_vbo_vao(int f)
{
    int i;
    for (i = 0; i < NDRAW; i++) fill_dyn((float *)((unsigned char *)dyn_cpu + dyn_off[i]), i);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_stream);
    glBufferData(GL_ARRAY_BUFFER, dyn_total, NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dyn_total, dyn_cpu);
    for (i = 0; i < NDRAW; i++) {
        glBindVertexArrayAPPLE(vao[i]);
        glDrawRangeElements(GL_TRIANGLES, 0, dyn_nv[i] - 1, dyn_nv[i] / 4 * 6, GL_UNSIGNED_SHORT, quad_idx);
    }
}
/* VAR: two halves, fence per half.  mode: 0 client idx, 1 element array, 2 flush per draw, 3 VAO */
static void dy_var_common(int f, int mode)
{
    int i, h = f & 1;
    unsigned char *half = vblock + (size_t)h * dyn_half, *vd = half + DYN_IDX_AREA;
    glFinishFenceAPPLE(fence[h]);
    if (mode == 2) {
        for (i = 0; i < NDRAW; i++) {
            fill_dyn((float *)(vd + dyn_off[i]), i);
            glFlushVertexArrayRangeAPPLE(dyn_nv[i] * VSTRIDE, vd + dyn_off[i]);
            dyn_draw_client(i, vd);
        }
    } else {
        for (i = 0; i < NDRAW; i++) fill_dyn((float *)(vd + dyn_off[i]), i);
        glFlushVertexArrayRangeAPPLE(dyn_total, vd);
        if (mode == 1) {
            glElementPointerAPPLE(GL_UNSIGNED_SHORT, half);
            for (i = 0; i < NDRAW; i++) {
                ptrs(vd + dyn_off[i]);
                glDrawRangeElementArrayAPPLE(GL_TRIANGLES, 0, dyn_nv[i] - 1, 0, dyn_nv[i] / 4 * 6);
            }
        } else if (mode == 3) {
            for (i = 0; i < NDRAW; i++) {
                glBindVertexArrayAPPLE(vao[h * NDRAW + i]);
                glDrawRangeElements(GL_TRIANGLES, 0, dyn_nv[i] - 1, dyn_nv[i] / 4 * 6, GL_UNSIGNED_SHORT, quad_idx);
            }
        } else {
            for (i = 0; i < NDRAW; i++) dyn_draw_client(i, vd);
        }
    }
    glSetFenceAPPLE(fence[h]);
}
static void dy_var_client(int f) { dy_var_common(f, 0); }
static void dy_var_elem(int f) { dy_var_common(f, 1); }
static void dy_var_flushdraw(int f) { dy_var_common(f, 2); }
static void dy_var_vao(int f) { dy_var_common(f, 3); }

static void dsu_none(void) { }
static void dsu_stream(void) { glBindBuffer(GL_ARRAY_BUFFER, vbo_stream); }
static void dsu_fences(void)
{
    glGenFencesAPPLE(2, fence);
    glSetFenceAPPLE(fence[0]); glSetFenceAPPLE(fence[1]);
}
static void dsu_var(GLenum st, int elem)
{
    dsu_fences();
    var_on(st, vblock, (size_t)2 * dyn_half);
    if (elem) {
        int h;
        glEnableClientState(GL_ELEMENT_ARRAY_APPLE);
        for (h = 0; h < 2; h++) memcpy(vblock + (size_t)h * dyn_half, quad_idx, sizeof quad_idx);
        glFlushVertexArrayRangeAPPLE((GLsizei)(2 * dyn_half), vblock);
    }
}
static void dsu_var_shared(void) { dsu_var(GL_STORAGE_SHARED_APPLE, 0); }
static void dsu_var_cached(void) { dsu_var(GL_STORAGE_CACHED_APPLE, 0); }
static void dsu_var_shared_elem(void) { dsu_var(GL_STORAGE_SHARED_APPLE, 1); }
static void dsu_var_cached_elem(void) { dsu_var(GL_STORAGE_CACHED_APPLE, 1); }
static void dsu_var_shared_vao(void)
{
    int i, h;
    dsu_var(GL_STORAGE_SHARED_APPLE, 0);
    glGenVertexArraysAPPLE(2 * NDRAW, vao);
    for (h = 0; h < 2; h++)
        for (i = 0; i < NDRAW; i++) {
            glBindVertexArrayAPPLE(vao[h * NDRAW + i]);
            arrays_on(1);
            glEnableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
            ptrs(vblock + (size_t)h * dyn_half + DYN_IDX_AREA + dyn_off[i]);
        }
    glBindVertexArrayAPPLE(0);
}
static void dsu_frame_vbo_vao(void)
{
    int i;
    glBindBuffer(GL_ARRAY_BUFFER, vbo_stream);
    glBufferData(GL_ARRAY_BUFFER, dyn_total, NULL, GL_STREAM_DRAW);
    glGenVertexArraysAPPLE(NDRAW, vao);
    for (i = 0; i < NDRAW; i++) {
        glBindVertexArrayAPPLE(vao[i]);
        arrays_on(1);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_stream);
        ptrs((const unsigned char *)0 + dyn_off[i]);
    }
    glBindVertexArrayAPPLE(0);
}
static void dtd_fences(void) { glFinishFenceAPPLE(fence[0]); glFinishFenceAPPLE(fence[1]); glDeleteFencesAPPLE(2, fence); }
static void dtd_fences_vao(void) { dtd_fences(); td_vao2(); }

typedef struct {
    const char *name;
    int dyn, need;                 /* need: 1 var, 2 elem, 4 vao, 8 fence (bit mask) */
    void (*setup)(void);
    void (*frame)(int);
    void (*teardown)(void);
} variant_t;

static const variant_t variants[] = {
    { "s1 VBO verts + client idx (current)",        0, 0, su_vbo,            st_vbo_client,   NULL },
    { "s2 VBO verts + shared static EBO",           0, 0, su_vbo_ebo,        st_vbo_ebo,      NULL },
    { "s3 VBO verts + per-mesh EBO",                0, 0, su_vbo,            st_vbo_ebo_mesh, NULL },
    { "s4 VAR shared + client idx",                 0, 1, su_var_shared,     st_var_client,   NULL },
    { "s5 VAR shared + APPLE_element_array",        0, 3, su_var_shared_elem, st_var_elem,    NULL },
    { "s6 VAR cached + client idx",                 0, 1, su_var_cached,     st_var_client,   NULL },
    { "s7 VAR cached + APPLE_element_array",        0, 3, su_var_cached_elem, st_var_elem,    NULL },
    { "s8 VAO(VBO ptrs) + client idx",              0, 4, su_vao0,           st_vao_client,   td_vao },
    { "s9 VAO(VBO ptrs + shared EBO)",              0, 4, su_vao1,           st_vao_ebo,      td_vao },
    { "s10 VAO(VBO ptrs + per-mesh EBO)",           0, 4, su_vao2,           st_vao_ebo_mesh, td_vao },
    { "d0 CPU fill only, no GL (reference)",        1, 0, dsu_none,          dy_fill_only,    NULL },
    { "d1 client arrays + client idx",              1, 0, dsu_none,          dy_client,       NULL },
    { "d2 VBO orphan + BufferSubData per draw",     1, 0, dsu_stream,        dy_orphan_sub,   NULL },
    { "d3 VBO BufferData(data) per draw",           1, 0, dsu_stream,        dy_bufdata,      NULL },
    { "d4 VBO orphan + glMapBuffer per draw",       1, 0, dsu_stream,        dy_orphan_map,   NULL },
    { "d5 VBO one upload per frame (orphan+sub)",   1, 0, dsu_stream,        dy_frame_vbo,    NULL },
    { "d6 VBO one glMapBuffer per frame",           1, 0, dsu_stream,        dy_frame_map,    NULL },
    { "d7 VAR shared x2 fenced + client idx",       1, 9, dsu_var_shared,    dy_var_client,   dtd_fences },
    { "d8 VAR shared x2 fenced + element array",    1, 11, dsu_var_shared_elem, dy_var_elem,  dtd_fences },
    { "d9 VAR cached x2 fenced + client idx",       1, 9, dsu_var_cached,    dy_var_client,   dtd_fences },
    { "d10 VAR cached x2 fenced + element array",   1, 11, dsu_var_cached_elem, dy_var_elem,  dtd_fences },
    { "d11 VAR shared, flush range per draw",       1, 9, dsu_var_shared,    dy_var_flushdraw, dtd_fences },
    { "d12 frame VBO upload + VAO per slot",        1, 4, dsu_frame_vbo_vao, dy_frame_vbo_vao, td_vao },
    { "d13 VAR shared x2 fenced + VAO per slot",    1, 13, dsu_var_shared_vao, dy_var_vao,    dtd_fences_vao },
};

static GLuint count_lit(void)
{
    static unsigned char px[RT * RT * 4];
    int i;
    GLuint n = 0;
    glReadPixels(0, 0, RT, RT, GL_RGBA, GL_UNSIGNED_BYTE, px);
    for (i = 0; i < RT * RT; i++) if (px[i * 4] | px[i * 4 + 1] | px[i * 4 + 2]) n++;
    return n;
}

static void measure(const variant_t *v)
{
    int rep, f, i;
    double best_sync = 1e30, best_sub = 0, best_sys = 0, best_pipe = 1e30;
    GLuint px;
    GLenum err;
    reset_state();
    v->setup();
    for (f = 0; f < 5; f++) { v->frame(f); glFinish(); }
    for (rep = 0; rep < g_reps; rep++) {
        double sub = 0, t0, t1, t2, s0 = sys_us();
        t0 = now_us();
        for (f = 0; f < g_frames; f++) {
            t1 = now_us();
            v->frame(f);
            t2 = now_us();
            sub += t2 - t1;
            glFinish();
        }
        t0 = (now_us() - t0) / g_frames;
        if (t0 < best_sync) { best_sync = t0; best_sub = sub / g_frames; best_sys = (sys_us() - s0) / g_frames; }
    }
    for (rep = 0; rep < g_reps; rep++) {
        double t0 = now_us();
        for (f = 0; f < g_frames; f++) v->frame(f);
        glFinish();
        t0 = (now_us() - t0) / g_frames;
        if (t0 < best_pipe) best_pipe = t0;
    }
    glClearColor(0, 0, 0, 0); glClear(GL_COLOR_BUFFER_BIT);
    v->frame(0);
    glFinish();
    px = count_lit();
    err = glGetError();
    if (v->teardown) v->teardown();
    reset_state();
    (void)i;
    printf("%-44s submit %7.2f us/draw  sync %7.2f ms  pipe %7.2f ms  sys %6.2f us/draw  px %u%s\n",
           v->name, best_sub / NDRAW, best_sync / 1000.0, best_pipe / 1000.0, best_sys / NDRAW, px,
           err ? "  GLERR" : "");
    if (err) printf("   glGetError=0x%x\n", (unsigned)err);
    fflush(stdout);
}

static int has_ext(const char *name)
{
    const char *e = (const char *)glGetString(GL_EXTENSIONS);
    return e && strstr(e, name) != NULL;
}

int main(int argc, char **argv)
{
    CGLPixelFormatAttribute attrs[] = { kCGLPFAAccelerated, kCGLPFAColorSize, (CGLPixelFormatAttribute)24, (CGLPixelFormatAttribute)0 };
    CGLPixelFormatObj fmt = NULL;
    CGLContextObj ctx = NULL;
    GLint nf = 0;
    GLuint rb, fbo;
    const char *groups = argc > 1 ? argv[1] : "sd", *filter = argc > 2 ? argv[2] : "";
    unsigned i;
    if (getenv("FRAMES")) g_frames = atoi(getenv("FRAMES"));
    if (getenv("REPS")) g_reps = atoi(getenv("REPS"));
    if (CGLChoosePixelFormat(attrs, &fmt, &nf) || !fmt || CGLCreateContext(fmt, NULL, &ctx) || CGLSetCurrentContext(ctx)) {
        fprintf(stderr, "no CGL context\n"); return 2;
    }
    printf("GL_RENDERER=%s VERSION=%s\nframes=%d reps=%d groups=%s draws=%d\n", (const char *)glGetString(GL_RENDERER),
           (const char *)glGetString(GL_VERSION), g_frames, g_reps, groups, NDRAW);
    has_var = has_ext("GL_APPLE_vertex_array_range");
    has_elem = has_ext("GL_APPLE_element_array");
    has_vao = has_ext("GL_APPLE_vertex_array_object");
    has_fence = has_ext("GL_APPLE_fence");
    printf("VAR %d element_array %d VAO %d fence %d\n", has_var, has_elem, has_vao, has_fence);
    glGenRenderbuffersEXT(1, &rb);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, RT, RT);
    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, rb);
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) { fprintf(stderr, "fbo incomplete\n"); return 2; }
    glViewport(0, 0, RT, RT);
    glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_CULL_FACE); glDisable(GL_LIGHTING); glDisable(GL_DITHER);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glColor4f(1, 1, 1, 1);
    build_data();
    glGenBuffers(1, &vbo_s);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_s);
    glBufferData(GL_ARRAY_BUFFER, vtotal, master_v, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo_s);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_s);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, itotal, idx_client, GL_STATIC_DRAW);
    glGenBuffers(NDRAW, ebo_m);
    for (i = 0; i < NDRAW; i++) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_m[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh[i].nidx * 2, (unsigned char *)idx_client + mesh[i].ioff, GL_STATIC_DRAW);
    }
    glGenBuffers(1, &vbo_stream);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    for (i = 0; i < sizeof variants / sizeof variants[0]; i++) {
        const variant_t *v = &variants[i];
        int need = v->need;
        if ((need & 1) && !has_var) continue;
        if ((need & 2) && !has_elem) continue;
        if ((need & 4) && !has_vao) continue;
        if ((need & 8) && !has_fence) continue;
        if (v->dyn ? !strchr(groups, 'd') : !strchr(groups, 's')) continue;
        if (filter[0] && !strstr(v->name, filter)) continue;
        measure(v);
    }
    return 0;
}
