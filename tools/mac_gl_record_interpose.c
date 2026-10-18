/* DYLD_INSERT_LIBRARIES interposer: records every OpenGL call the renderer makes
   up to and including one chosen frame, in enough detail to replay the frame
   (tools/mac_gl_replay.c): objects with their data (textures, buffers, ARB and
   GLSL programs), state, and for the chosen frame every call, the client
   vertex arrays and client indices each draw reads.  A frame ends at glFlush
   (the renderer calls it once per frame under HALO_MAC_LAZY_PRESENT=1).

   Environment:
     GLREC_OUT    capture file to write (required)
     GLREC_FRAME  the frame to capture: the Nth glFlush ends it (default 20)
     GLREC_EXIT   1: _exit(0) once the file is written
   Build: tools/mac_gl_replay_checks.sh.  Word layouts of the records are the
   comments next to each wrapper; the replay reads the same. */
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mac_gl_replay_format.h"

#define ID_MAX (1u << 18)
#define OC_NONE 0
#define OC_TEX 1
#define OC_BUF 2
#define OC_ARB 3

typedef struct {
    unsigned op, nw;
    unsigned w[GLR_MAX_WORDS];
    unsigned bloblen;
    void *blob;
    unsigned next;               /* next record of the same object (1-based, 0 ends) */
    unsigned oid;
    unsigned char oclass, dead;
    unsigned short slot;
} Rec;

static Rec *recs;
static unsigned nrecs, caprecs;
static unsigned heads[4][ID_MAX];
static unsigned char fb_tex[ID_MAX];
static unsigned target_frame = 20, flushes, frame_start;
static int done, in_capture, configured;
static unsigned long blob_bytes;
static const char *out_path;

static union { float f; unsigned u; } fu;
static unsigned f2u(float f) { fu.f = f; return fu.u; }
static void d2w(double d, unsigned *w) { memcpy(w, &d, 8); }

static void configure(void)
{
    const char *v;
    configured = 1;
    v = getenv("GLREC_FRAME"); if (v && atoi(v) > 1) target_frame = (unsigned)atoi(v);
    out_path = getenv("GLREC_OUT");
    if (!out_path) done = 1;
}

/* ---- state the recorder follows ---- */
static unsigned active_unit;
static unsigned unit_tex[8][4];
static unsigned array_buf, element_buf;
static unsigned arb_bound[2];
static unsigned unpack_alignment = 4, unpack_row_length;
typedef struct { int enabled; unsigned size, type, norm, stride, buffer; const char *ptr; int dirty; } Attr;
static Attr attr[16];
static unsigned snapshot[8][4];
static int snapshot_tex_ok;
static unsigned pending_tex;     /* texture the next simple record belongs to */

static int tex_index(unsigned target)
{
    if (target == GL_TEXTURE_2D) return 0;
    if (target == GL_TEXTURE_3D) return 1;
    if (target == GL_TEXTURE_CUBE_MAP) return 2;
    return 3;
}
static unsigned tex_slot(unsigned target, unsigned level)
{
    unsigned t = target >= 0x8515 && target <= 0x851A ? 2 + (target - 0x8515) : target == GL_TEXTURE_3D ? 1 : 0;
    return t * 32 + (level & 31);
}
static unsigned bound_tex(unsigned target)
{
    unsigned t = target >= 0x8515 && target <= 0x851A ? 2 : (unsigned)tex_index(target);
    return unit_tex[active_unit & 7][t];
}

/* ---- the record store ---- */
static int storing(int skip_precap)
{
    if (!configured) configure();
    if (done) return 0;
    if (skip_precap && !in_capture) return 0;
    return 1;
}
static unsigned rec_add(unsigned op, const unsigned *w, unsigned nw, const void *blob, unsigned bloblen,
    int oclass, unsigned oid, unsigned slot)
{
    Rec *r;
    if (nrecs == caprecs) {
        caprecs = caprecs ? caprecs * 2 : 1u << 16;
        recs = (Rec *)realloc(recs, caprecs * sizeof(Rec));
        if (!recs) { fprintf(stderr, "glrec: out of memory\n"); _exit(3); }
    }
    r = &recs[nrecs];
    memset(r, 0, sizeof *r);
    r->op = op; r->nw = nw;
    if (nw) memcpy(r->w, w, nw * 4);
    if (bloblen && blob) {
        r->blob = malloc(bloblen);
        if (!r->blob) { fprintf(stderr, "glrec: out of memory\n"); _exit(3); }
        memcpy(r->blob, blob, bloblen);
        r->bloblen = bloblen; blob_bytes += bloblen;
    }
    nrecs++;
    if (oclass && oid < ID_MAX) {
        r->oclass = (unsigned char)oclass; r->oid = oid; r->slot = (unsigned short)slot;
        r->next = heads[oclass][oid]; heads[oclass][oid] = nrecs;   /* newest first */
    }
    return nrecs;
}
/* slot < 0: every record of the object; else those with that slot */
static void kill_obj(int oclass, unsigned oid, int slot)
{
    unsigned i, prev = 0, nxt;
    if (oid >= ID_MAX) return;
    for (i = heads[oclass][oid]; i; i = nxt) {
        Rec *r = &recs[i - 1];
        nxt = r->next;
        if (slot < 0 || r->slot == slot) {
            if (r->blob) { free(r->blob); r->blob = 0; blob_bytes -= r->bloblen; r->bloblen = 0; }
            r->dead = 1;
            if (prev) recs[prev - 1].next = nxt; else heads[oclass][oid] = nxt;
        } else prev = i;
    }
}
static void rec_simple(unsigned op, const unsigned *w, unsigned nw, int skip_precap)
{
    if (!storing(skip_precap)) return;
    if (pending_tex) { rec_add(op, w, nw, 0, 0, OC_TEX, pending_tex, 0xFFFF); pending_tex = 0; }
    else rec_add(op, w, nw, 0, 0, 0, 0, 0);
}

/* ---- hooks of the generated wrappers ---- */
static void hook_glActiveTextureARB(GLenum t) { active_unit = (t - GL_TEXTURE0_ARB) & 7; }
static void hook_glEnableVertexAttribArray(GLuint i) { if (i < 16) attr[i].enabled = 1; }
static void hook_glDisableVertexAttribArray(GLuint i) { if (i < 16) attr[i].enabled = 0; }
static void hook_glPixelStorei(GLenum p, GLint v)
{
    if (p == GL_UNPACK_ALIGNMENT) unpack_alignment = (unsigned)v;
    else if (p == GL_UNPACK_ROW_LENGTH) unpack_row_length = (unsigned)v;
}
static void hook_texparam(GLenum target) { pending_tex = bound_tex(target); }

#define GLR_GEN_WRAPPERS
#include "gl_simple.inc"
#undef GLR_GEN_WRAPPERS

/* ---- object names ---- */
/* Gen*: w0 = n, blob = the n names.  Delete*: the same.  Bind*: w0 target, w1 name. */
#define GEN_WRAPPER(fn, op) \
static void w_##fn(GLsizei n, GLuint *ids) \
{ fn(n, ids); if (storing(0) && n > 0) { unsigned w = (unsigned)n; rec_add(op, &w, 1, ids, (unsigned)n * 4, 0, 0, 0); } }
GEN_WRAPPER(glGenTextures, OP_glGenTextures)
GEN_WRAPPER(glGenBuffers, OP_glGenBuffers)
GEN_WRAPPER(glGenProgramsARB, OP_glGenProgramsARB)
GEN_WRAPPER(glGenFramebuffersEXT, OP_glGenFramebuffersEXT)
GEN_WRAPPER(glGenRenderbuffersEXT, OP_glGenRenderbuffersEXT)
GEN_WRAPPER(glGenQueriesARB, OP_glGenQueriesARB)

static void rec_delete(unsigned op, GLsizei n, const GLuint *ids)
{
    unsigned w = (unsigned)n;
    if (storing(0) && n > 0) rec_add(op, &w, 1, ids, (unsigned)n * 4, 0, 0, 0);
}
static void w_glDeleteTextures(GLsizei n, const GLuint *ids)
{
    GLsizei i;
    if (storing(0)) for (i = 0; i < n; i++) {
        unsigned u, t;
        kill_obj(OC_TEX, ids[i], -1);
        for (u = 0; u < 8; u++) for (t = 0; t < 4; t++) if (unit_tex[u][t] == ids[i]) unit_tex[u][t] = 0;
    }
    rec_delete(OP_glDeleteTextures, n, ids); glDeleteTextures(n, ids);
}
static void w_glDeleteBuffers(GLsizei n, const GLuint *ids)
{
    GLsizei i;
    if (storing(0)) for (i = 0; i < n; i++) {
        kill_obj(OC_BUF, ids[i], -1);
        if (array_buf == ids[i]) array_buf = 0;
        if (element_buf == ids[i]) element_buf = 0;
    }
    rec_delete(OP_glDeleteBuffers, n, ids); glDeleteBuffers(n, ids);
}
static void w_glDeleteProgramsARB(GLsizei n, const GLuint *ids)
{
    GLsizei i;
    if (storing(0)) for (i = 0; i < n; i++) kill_obj(OC_ARB, ids[i], -1);
    rec_delete(OP_glDeleteProgramsARB, n, ids); glDeleteProgramsARB(n, ids);
}
static void w_glDeleteFramebuffersEXT(GLsizei n, const GLuint *ids) { rec_delete(OP_glDeleteFramebuffersEXT, n, ids); glDeleteFramebuffersEXT(n, ids); }
static void w_glDeleteRenderbuffersEXT(GLsizei n, const GLuint *ids) { rec_delete(OP_glDeleteRenderbuffersEXT, n, ids); glDeleteRenderbuffersEXT(n, ids); }
static void w_glDeleteQueriesARB(GLsizei n, const GLuint *ids) { rec_delete(OP_glDeleteQueriesARB, n, ids); glDeleteQueriesARB(n, ids); }
static void w_glDeleteShader(GLuint s) { if (storing(0)) rec_add(OP_glDeleteShader, (const unsigned *)&s, 1, 0, 0, 0, 0, 0); glDeleteShader(s); }
static void w_glDeleteProgram(GLuint p) { if (storing(0)) rec_add(OP_glDeleteProgram, (const unsigned *)&p, 1, 0, 0, 0, 0, 0); glDeleteProgram(p); }

static void w_glBindTexture(GLenum target, GLuint id)
{
    if (storing(0)) {
        unsigned w[2]; w[0] = target; w[1] = id;
        rec_add(OP_glBindTexture, w, 2, 0, 0, 0, 0, 0);
    }
    unit_tex[active_unit & 7][target >= 0x8515 && target <= 0x851A ? 2 : tex_index(target)] = id;
    glBindTexture(target, id);
}
static void w_glBindBuffer(GLenum target, GLuint id)
{
    if (storing(0)) {
        unsigned w[2]; w[0] = target; w[1] = id;
        rec_add(OP_glBindBuffer, w, 2, 0, 0, 0, 0, 0);
    }
    if (target == GL_ARRAY_BUFFER) array_buf = id;
    else if (target == GL_ELEMENT_ARRAY_BUFFER) element_buf = id;
    glBindBuffer(target, id);
}
static void w_glBindProgramARB(GLenum target, GLuint id)
{
    if (storing(0)) {
        unsigned w[2]; w[0] = target; w[1] = id;
        rec_add(OP_glBindProgramARB, w, 2, 0, 0, 0, 0, 0);
    }
    arb_bound[target == GL_FRAGMENT_PROGRAM_ARB] = id;
    glBindProgramARB(target, id);
}
static void w_glBindFramebufferEXT(GLenum target, GLuint id)
{
    unsigned w[2]; w[0] = target; w[1] = id;
    if (storing(0)) rec_add(OP_glBindFramebufferEXT, w, 2, 0, 0, 0, 0, 0);
    glBindFramebufferEXT(target, id);
}
static void w_glBindRenderbufferEXT(GLenum target, GLuint id)
{
    unsigned w[2]; w[0] = target; w[1] = id;
    if (storing(0)) rec_add(OP_glBindRenderbufferEXT, w, 2, 0, 0, 0, 0, 0);
    glBindRenderbufferEXT(target, id);
}
static void w_glUseProgram(GLuint id) { if (storing(0)) rec_add(OP_glUseProgram, (const unsigned *)&id, 1, 0, 0, 0, 0, 0); glUseProgram(id); }
static void w_glFramebufferTexture2DEXT(GLenum t, GLenum a, GLenum tt, GLuint tex, GLint level)
{
    unsigned w[5]; w[0] = t; w[1] = a; w[2] = tt; w[3] = tex; w[4] = (unsigned)level;
    if (storing(0)) rec_add(OP_glFramebufferTexture2DEXT, w, 5, 0, 0, 0, 0, 0);
    if (tex < ID_MAX) fb_tex[tex] = 1;
    glFramebufferTexture2DEXT(t, a, tt, tex, level);
}
static void w_glFramebufferRenderbufferEXT(GLenum t, GLenum a, GLenum rt, GLuint rb)
{
    unsigned w[4]; w[0] = t; w[1] = a; w[2] = rt; w[3] = rb;
    if (storing(0)) rec_add(OP_glFramebufferRenderbufferEXT, w, 4, 0, 0, 0, 0, 0);
    glFramebufferRenderbufferEXT(t, a, rt, rb);
}

/* ---- GLSL ---- */
static GLuint w_glCreateShader(GLenum type)
{
    GLuint id = glCreateShader(type);
    unsigned w[2]; w[0] = type; w[1] = id;
    if (storing(0)) rec_add(OP_glCreateShader, w, 2, 0, 0, 0, 0, 0);
    return id;
}
static GLuint w_glCreateProgram(void)
{
    GLuint id = glCreateProgram();
    if (storing(0)) rec_add(OP_glCreateProgram, (const unsigned *)&id, 1, 0, 0, 0, 0, 0);
    return id;
}
/* ShaderSource: w0 shader, blob = the strings joined */
static void w_glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length)
{
    if (storing(0)) {
        GLsizei i; unsigned total = 0, at = 0; char *joined;
        for (i = 0; i < count; i++) total += length && length[i] >= 0 ? (unsigned)length[i] : (unsigned)strlen(string[i]);
        joined = (char *)malloc(total + 1);
        for (i = 0; i < count; i++) {
            unsigned n = length && length[i] >= 0 ? (unsigned)length[i] : (unsigned)strlen(string[i]);
            memcpy(joined + at, string[i], n); at += n;
        }
        rec_add(OP_glShaderSource, (const unsigned *)&shader, 1, joined, total, 0, 0, 0);
        free(joined);
    }
    glShaderSource(shader, count, string, length);
}
static void w_glCompileShader(GLuint s) { if (storing(0)) rec_add(OP_glCompileShader, (const unsigned *)&s, 1, 0, 0, 0, 0, 0); glCompileShader(s); }
static void w_glAttachShader(GLuint p, GLuint s)
{ unsigned w[2]; w[0] = p; w[1] = s; if (storing(0)) rec_add(OP_glAttachShader, w, 2, 0, 0, 0, 0, 0); glAttachShader(p, s); }
static void w_glLinkProgram(GLuint p) { if (storing(0)) rec_add(OP_glLinkProgram, (const unsigned *)&p, 1, 0, 0, 0, 0, 0); glLinkProgram(p); }
static void w_glBindAttribLocation(GLuint p, GLuint i, const GLchar *name)
{
    unsigned w[2]; w[0] = p; w[1] = i;
    if (storing(0)) rec_add(OP_glBindAttribLocation, w, 2, name, (unsigned)strlen(name) + 1, 0, 0, 0);
    glBindAttribLocation(p, i, name);
}
static GLint w_glGetUniformLocation(GLuint p, const GLchar *name)
{
    GLint loc = glGetUniformLocation(p, name);
    unsigned w[2]; w[0] = p; w[1] = (unsigned)loc;
    if (storing(0)) rec_add(OP_glGetUniformLocation, w, 2, name, (unsigned)strlen(name) + 1, 0, 0, 0);
    return loc;
}
static GLint w_glGetAttribLocation(GLuint p, const GLchar *name)
{
    GLint loc = glGetAttribLocation(p, name);
    unsigned w[2]; w[0] = p; w[1] = (unsigned)loc;
    if (storing(0)) rec_add(OP_glGetAttribLocation, w, 2, name, (unsigned)strlen(name) + 1, 0, 0, 0);
    return loc;
}
static void w_glUniform1f(GLint loc, GLfloat v)
{ unsigned w[2]; w[0] = (unsigned)loc; w[1] = f2u(v); if (storing(0)) rec_add(OP_glUniform1f, w, 2, 0, 0, 0, 0, 0); glUniform1f(loc, v); }
static void w_glUniform1i(GLint loc, GLint v)
{ unsigned w[2]; w[0] = (unsigned)loc; w[1] = (unsigned)v; if (storing(0)) rec_add(OP_glUniform1i, w, 2, 0, 0, 0, 0, 0); glUniform1i(loc, v); }
static void w_glUniform4fv(GLint loc, GLsizei count, const GLfloat *v)
{
    unsigned w[2]; w[0] = (unsigned)loc; w[1] = (unsigned)count;
    if (storing(0) && count > 0) rec_add(OP_glUniform4fv, w, 2, v, (unsigned)count * 16, 0, 0, 0);
    glUniform4fv(loc, count, v);
}

/* ---- queries, ARB programs and small state with pointers ---- */
static void w_glBeginQueryARB(GLenum t, GLuint id)
{ unsigned w[2]; w[0] = t; w[1] = id; if (storing(1)) rec_add(OP_glBeginQueryARB, w, 2, 0, 0, 0, 0, 0); glBeginQueryARB(t, id); }
static void w_glGetQueryObjectuivARB(GLuint id, GLenum pname, GLuint *p)
{ unsigned w[2]; w[0] = id; w[1] = pname; if (storing(1)) rec_add(OP_glGetQueryObjectuivARB, w, 2, 0, 0, 0, 0, 0); glGetQueryObjectuivARB(id, pname, p); }
/* ProgramString: target, format, len; blob = the text */
static void w_glProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid *string)
{
    if (storing(0)) {
        unsigned w[3]; unsigned id = arb_bound[target == GL_FRAGMENT_PROGRAM_ARB];
        w[0] = target; w[1] = format; w[2] = (unsigned)len;
        kill_obj(OC_ARB, id, -1);
        rec_add(OP_glProgramStringARB, w, 3, string, (unsigned)len, OC_ARB, id, 0);
    }
    glProgramStringARB(target, format, len, string);
}
/* ProgramEnvParameter4fv: target, index, 4 floats */
static void w_glProgramEnvParameter4fvARB(GLenum target, GLuint index, const GLfloat *v)
{
    if (storing(0)) {
        unsigned w[6]; w[0] = target; w[1] = index; memcpy(w + 2, v, 16);
        rec_add(OP_glProgramEnvParameter4fvARB, w, 6, 0, 0, 0, 0, 0);
    }
    glProgramEnvParameter4fvARB(target, index, v);
}
static void w_glVertexAttrib4fv(GLuint index, const GLfloat *v)
{
    if (storing(0)) {
        unsigned w[5]; w[0] = index; memcpy(w + 1, v, 16);
        rec_add(OP_glVertexAttrib4fv, w, 5, 0, 0, 0, 0, 0);
    }
    glVertexAttrib4fv(index, v);
}
/* TexParameterfv: target, pname, count, floats */
static void w_glTexParameterfv(GLenum target, GLenum pname, const GLfloat *v)
{
    if (storing(0)) {
        unsigned w[7], n = pname == GL_TEXTURE_BORDER_COLOR ? 4 : 1; unsigned id = bound_tex(target);
        w[0] = target; w[1] = pname; w[2] = n; memcpy(w + 3, v, n * 4);
        rec_add(OP_glTexParameterfv, w, 3 + n, 0, 0, OC_TEX, id, 0xFFFF);
    }
    glTexParameterfv(target, pname, v);
}

/* ---- data ---- */
/* BufferData: target, size, usage, hasdata; BufferSubData: target, offset, size */
static unsigned bound_buffer(GLenum target)
{ return target == GL_ARRAY_BUFFER ? array_buf : target == GL_ELEMENT_ARRAY_BUFFER ? element_buf : 0; }
static void w_glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)
{
    if (storing(0)) {
        unsigned w[4], id = bound_buffer(target);
        w[0] = target; w[1] = (unsigned)size; w[2] = usage; w[3] = data != 0;
        if (id) kill_obj(OC_BUF, id, -1);
        rec_add(OP_glBufferData, w, 4, data, data ? (unsigned)size : 0, id ? OC_BUF : 0, id, 0);
    }
    glBufferData(target, size, data, usage);
}
static void w_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data)
{
    if (storing(0)) {
        unsigned w[3], id = bound_buffer(target);
        w[0] = target; w[1] = (unsigned)offset; w[2] = (unsigned)size;
        rec_add(OP_glBufferSubData, w, 3, data, (unsigned)size, id ? OC_BUF : 0, id, 1);
    }
    glBufferSubData(target, offset, size, data);
}
static unsigned type_bytes(unsigned type)
{
    switch (type) {
    case 0x1400: case 0x1401: case 0x8032: case 0x8362: return 1;
    case 0x1402: case 0x1403: case 0x8033: case 0x8034: case 0x8363: case 0x8364: case 0x8365: case 0x8366:
    case 0x85BA: case 0x85BB: case 0x140B: return 2;
    case 0x1404: case 0x1405: case 0x1406: case 0x8035: case 0x8036: case 0x8367: case 0x8368: case 0x84FA: return 4;
    case 0x140A: return 8;
    }
    return 4;
}
static unsigned format_comps(unsigned format, unsigned type)
{
    switch (type) {   /* packed types carry all components in one element */
    case 0x8032: case 0x8362: case 0x8033: case 0x8034: case 0x8363: case 0x8364: case 0x8365: case 0x8366:
    case 0x85BA: case 0x85BB: case 0x8035: case 0x8036: case 0x8367: case 0x8368: case 0x84FA: return 1;
    }
    switch (format) {
    case 0x1908: case 0x80E1: return 4;
    case 0x1907: case 0x80E0: return 3;
    case 0x190A: return 2;
    }
    return 1;
}
static unsigned image_bytes(unsigned w, unsigned h, unsigned d, unsigned format, unsigned type)
{
    unsigned bpp = format_comps(format, type) * type_bytes(type);
    unsigned rowlen = unpack_row_length ? unpack_row_length : w;
    unsigned row = rowlen * bpp, al = unpack_alignment ? unpack_alignment : 1;
    row = (row + al - 1) / al * al;
    return row * h * (d ? d : 1);
}
/* TexImage2D: target level internal w h border format type hasdata */
static void tex_record(unsigned op, unsigned target, unsigned level, unsigned *w, unsigned nw,
    const void *pixels, unsigned bytes, int image)
{
    unsigned id = bound_tex(target), slot = tex_slot(target, level);
    if (image) kill_obj(OC_TEX, id, (int)slot);
    rec_add(op, w, nw, pixels, pixels ? bytes : 0, OC_TEX, id, slot);
}
static void w_glTexImage2D(GLenum target, GLint level, GLint internal, GLsizei width, GLsizei height,
    GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    if (storing(0)) {
        unsigned w[9];
        w[0] = target; w[1] = (unsigned)level; w[2] = (unsigned)internal; w[3] = (unsigned)width;
        w[4] = (unsigned)height; w[5] = (unsigned)border; w[6] = format; w[7] = type; w[8] = pixels != 0;
        tex_record(OP_glTexImage2D, target, (unsigned)level, w, 9, pixels,
            image_bytes((unsigned)width, (unsigned)height, 1, format, type), 1);
    }
    glTexImage2D(target, level, internal, width, height, border, format, type, pixels);
}
static void w_glTexSubImage2D(GLenum target, GLint level, GLint xo, GLint yo, GLsizei width, GLsizei height,
    GLenum format, GLenum type, const GLvoid *pixels)
{
    if (storing(0)) {
        unsigned w[8];
        w[0] = target; w[1] = (unsigned)level; w[2] = (unsigned)xo; w[3] = (unsigned)yo; w[4] = (unsigned)width;
        w[5] = (unsigned)height; w[6] = format; w[7] = type;
        tex_record(OP_glTexSubImage2D, target, (unsigned)level, w, 8, pixels,
            image_bytes((unsigned)width, (unsigned)height, 1, format, type), 0);
    }
    glTexSubImage2D(target, level, xo, yo, width, height, format, type, pixels);
}
static void w_glTexImage3D(GLenum target, GLint level, GLint internal, GLsizei width, GLsizei height,
    GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    if (storing(0)) {
        unsigned w[10];
        w[0] = target; w[1] = (unsigned)level; w[2] = (unsigned)internal; w[3] = (unsigned)width;
        w[4] = (unsigned)height; w[5] = (unsigned)depth; w[6] = (unsigned)border; w[7] = format; w[8] = type;
        w[9] = pixels != 0;
        tex_record(OP_glTexImage3D, target, (unsigned)level, w, 10, pixels,
            image_bytes((unsigned)width, (unsigned)height, (unsigned)depth, format, type), 1);
    }
    glTexImage3D(target, level, internal, width, height, depth, border, format, type, pixels);
}
/* CompressedTexImage2D: target level internal w h border size hasdata */
static void w_glCompressedTexImage2D(GLenum target, GLint level, GLenum internal, GLsizei width, GLsizei height,
    GLint border, GLsizei size, const GLvoid *data)
{
    if (storing(0)) {
        unsigned w[8];
        w[0] = target; w[1] = (unsigned)level; w[2] = internal; w[3] = (unsigned)width; w[4] = (unsigned)height;
        w[5] = (unsigned)border; w[6] = (unsigned)size; w[7] = data != 0;
        tex_record(OP_glCompressedTexImage2D, target, (unsigned)level, w, 8, data, (unsigned)size, 1);
    }
    glCompressedTexImage2D(target, level, internal, width, height, border, size, data);
}

/* ---- vertex arrays and draws ---- */
/* VertexAttribPointer (a buffer is bound): idx size type norm stride offset buffer
   VertexAttribPointerClient: idx size type norm stride */
static void w_glVertexAttribPointer(GLuint idx, GLint size, GLenum type, GLboolean norm, GLsizei stride, const GLvoid *ptr)
{
    if (idx < 16) {
        Attr *a = &attr[idx];
        a->size = (unsigned)size; a->type = type; a->norm = norm; a->stride = (unsigned)stride;
        a->buffer = array_buf; a->ptr = (const char *)ptr; a->dirty = 1;
    }
    if (storing(0)) {
        unsigned w[7];
        w[0] = idx; w[1] = (unsigned)size; w[2] = type; w[3] = norm; w[4] = (unsigned)stride;
        if (array_buf) { w[5] = (unsigned)(unsigned long)ptr; w[6] = array_buf; rec_add(OP_glVertexAttribPointer, w, 7, 0, 0, 0, 0, 0); }
        else rec_add(OP_glVertexAttribPointerClient, w, 5, 0, 0, 0, 0, 0);
    }
    glVertexAttribPointer(idx, size, type, norm, stride, ptr);
}

static unsigned long client_bytes;
/* ClientBlob: slot; blob = bytes.  ClientArray: idx size type norm stride slot
   offset_of_vertex_min_in_blob min dirty stride_effective.  They precede a draw. */
static void emit_client_arrays(unsigned minv, unsigned maxv)
{
    unsigned i, n = 0, idx[16];
    unsigned long lo[16], hi[16], sum = 0, ulo = ~0ul, uhi = 0;
    unsigned seff[16];
    for (i = 0; i < 16; i++) {
        Attr *a = &attr[i];
        if (a->enabled && a->buffer == 0 && a->ptr) {
            unsigned elem = a->size * type_bytes(a->type), st = a->stride ? a->stride : elem;
            idx[n] = i; seff[n] = st;
            lo[n] = (unsigned long)a->ptr + (unsigned long)minv * st;
            hi[n] = (unsigned long)a->ptr + (unsigned long)maxv * st + elem;
            sum += hi[n] - lo[n];
            if (lo[n] < ulo) ulo = lo[n];
            if (hi[n] > uhi) uhi = hi[n];
            n++;
        }
    }
    if (!n) return;
    if (uhi - ulo <= sum + sum / 4 + 65536) {
        unsigned w = 0, k;
        rec_add(OP_glClientBlob, (const unsigned *)&w, 1, (const void *)ulo, (unsigned)(uhi - ulo), 0, 0, 0);
        client_bytes += uhi - ulo;
        for (k = 0; k < n; k++) {
            Attr *a = &attr[idx[k]]; unsigned x[10];
            x[0] = idx[k]; x[1] = a->size; x[2] = a->type; x[3] = a->norm; x[4] = a->stride; x[5] = 0;
            x[6] = (unsigned)(lo[k] - ulo); x[7] = minv; x[8] = (unsigned)a->dirty; x[9] = seff[k];
            rec_add(OP_glClientArray, x, 10, 0, 0, 0, 0, 0);
            a->dirty = 0;
        }
    } else {
        unsigned k;
        for (k = 0; k < n; k++) {
            Attr *a = &attr[idx[k]]; unsigned x[10], slot = k;
            rec_add(OP_glClientBlob, (const unsigned *)&slot, 1, (const void *)lo[k], (unsigned)(hi[k] - lo[k]), 0, 0, 0);
            client_bytes += hi[k] - lo[k];
            x[0] = idx[k]; x[1] = a->size; x[2] = a->type; x[3] = a->norm; x[4] = a->stride; x[5] = slot;
            x[6] = 0; x[7] = minv; x[8] = (unsigned)a->dirty; x[9] = seff[k];
            rec_add(OP_glClientArray, x, 10, 0, 0, 0, 0, 0);
            a->dirty = 0;
        }
    }
}
static void index_range(const void *indices, unsigned count, unsigned type, unsigned *minv, unsigned *maxv)
{
    unsigned i, lo = 0xFFFFFFFFu, hi = 0, v;
    for (i = 0; i < count; i++) {
        v = type == GL_UNSIGNED_BYTE ? ((const unsigned char *)indices)[i]
            : type == GL_UNSIGNED_SHORT ? ((const unsigned short *)indices)[i]
            : ((const unsigned *)indices)[i];
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    if (!count) lo = 0;
    *minv = lo; *maxv = hi;
}
/* DrawElements: mode count type ptr_or_offset client min max
   DrawRangeElements: mode start end count type ptr_or_offset client min max (blob = indices when client)
   DrawArrays: mode first count */
static void w_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    if (storing(1) && count > 0) {
        unsigned w[7], minv = 0, maxv = 0, client = element_buf == 0, bytes = (unsigned)count * type_bytes(type);
        if (client) index_range(indices, (unsigned)count, type, &minv, &maxv);
        else { minv = 0; maxv = 0xFFFFFFFFu; }
        if (client) emit_client_arrays(minv, maxv);
        w[0] = mode; w[1] = (unsigned)count; w[2] = type; w[3] = (unsigned)(unsigned long)indices;
        w[4] = client; w[5] = minv; w[6] = maxv;
        rec_add(OP_glDrawElements, w, 7, client ? indices : 0, client ? bytes : 0, 0, 0, 0);
    }
    glDrawElements(mode, count, type, indices);
}
static void w_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
    if (storing(1) && count > 0) {
        unsigned w[9], client = element_buf == 0, bytes = (unsigned)count * type_bytes(type);
        emit_client_arrays(start, end);
        w[0] = mode; w[1] = start; w[2] = end; w[3] = (unsigned)count; w[4] = type;
        w[5] = (unsigned)(unsigned long)indices; w[6] = client; w[7] = start; w[8] = end;
        rec_add(OP_glDrawRangeElements, w, 9, client ? indices : 0, client ? bytes : 0, 0, 0, 0);
    }
    glDrawRangeElements(mode, start, end, count, type, indices);
}
static void w_glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    if (storing(1) && count > 0) {
        unsigned w[3];
        emit_client_arrays((unsigned)first, (unsigned)first + (unsigned)count - 1);
        w[0] = mode; w[1] = (unsigned)first; w[2] = (unsigned)count;
        rec_add(OP_glDrawArrays, w, 3, 0, 0, 0, 0, 0);
    }
    glDrawArrays(mode, first, count);
}

/* ---- queries of state (synchronising calls) ---- */
static GLenum w_glGetError(void) { if (storing(1)) rec_add(OP_glGetError, 0, 0, 0, 0, 0, 0, 0); return glGetError(); }
static void w_glGetIntegerv(GLenum p, GLint *v) { if (storing(1)) rec_add(OP_glGetIntegerv, (const unsigned *)&p, 1, 0, 0, 0, 0, 0); glGetIntegerv(p, v); }
static void w_glGetFloatv(GLenum p, GLfloat *v) { if (storing(1)) rec_add(OP_glGetFloatv, (const unsigned *)&p, 1, 0, 0, 0, 0, 0); glGetFloatv(p, v); }
static void w_glGetBooleanv(GLenum p, GLboolean *v) { if (storing(1)) rec_add(OP_glGetBooleanv, (const unsigned *)&p, 1, 0, 0, 0, 0, 0); glGetBooleanv(p, v); }
static GLboolean w_glIsEnabled(GLenum p) { if (storing(1)) rec_add(OP_glIsEnabled, (const unsigned *)&p, 1, 0, 0, 0, 0, 0); return glIsEnabled(p); }

/* ---- the frame boundary and the file ---- */
static void put(FILE *f, const void *p, unsigned n)
{
    static const char zero[16];
    if (n) fwrite(p, 1, n, f);
    if (n & 15) fwrite(zero, 1, 16 - (n & 15), f);
}
static void dump(void)
{
    FILE *f = fopen(out_path, "wb");
    glr_header h;
    unsigned i, pre = 0, frame = 0, skipped = 0;
    unsigned char *used = (unsigned char *)calloc(ID_MAX, 1);
    unsigned u, t;
    for (i = 0; i < ID_MAX; i++) used[i] = fb_tex[i];
    for (i = frame_start; i < nrecs; i++)
        if (recs[i].op == OP_glBindTexture && recs[i].w[1] < ID_MAX) used[recs[i].w[1]] = 1;
    for (u = 0; u < 8; u++) for (t = 0; t < 4; t++) if (snapshot_tex_ok && snapshot[u][t] < ID_MAX) used[snapshot[u][t]] = 1;
    if (!f) { fprintf(stderr, "glrec: cannot write %s\n", out_path); return; }
    memset(&h, 0, sizeof h);
    h.magic = GLR_MAGIC; h.version = GLR_VERSION;
    put(f, &h, 32);
    for (i = 0; i < nrecs; i++) {
        Rec *r = &recs[i]; glr_rec g;
        if (r->dead) continue;
        if (r->oclass == OC_TEX && r->oid < ID_MAX && !used[r->oid]) { skipped++; continue; }
        g.op = r->op; g.nw = r->nw; g.bloblen = r->bloblen; g.pad = 0;
        fwrite(&g, 1, 16, f);
        put(f, r->w, r->nw * 4);
        put(f, r->blob, r->bloblen);
        if (i < frame_start) pre++; else frame++;
    }
    h.nops_pre = pre; h.nops_frame = frame;
    fseek(f, 0, SEEK_SET);
    fwrite(&h, 1, sizeof h, f);
    fclose(f);
    free(used);
    fprintf(stderr, "glrec: wrote %s: %u setup records, %u frame records, %u texture records of unused textures left out, client array bytes %lu\n",
        out_path, pre, frame, skipped, client_bytes);
}
static void begin_capture(void)
{
    unsigned u, t;
    in_capture = 1; frame_start = nrecs;
    for (u = 0; u < 8; u++) for (t = 0; t < 4; t++) snapshot[u][t] = unit_tex[u][t];
    snapshot_tex_ok = 1;
    fprintf(stderr, "glrec: capturing the frame after flush %u (%u records held, %lu bytes)\n", flushes, nrecs, blob_bytes);
}
static void w_glFlush(void)
{
    if (storing(0)) {
        flushes++;
        if (flushes == target_frame - 1) begin_capture();
        else if (flushes == target_frame) {
            rec_add(OP_glFlush, 0, 0, 0, 0, 0, 0, 0);
            dump();
            done = 1;
            if (getenv("GLREC_EXIT")) { glFlush(); _exit(0); }
        }
    }
    glFlush();
}

#define INTERPOSE(r, o) { (const void *)(unsigned long)&r, (const void *)(unsigned long)&o }
typedef struct { const void *replacement; const void *original; } interpose_t;
__attribute__((used)) static const interpose_t interposers[] __attribute__((section("__DATA,__interpose"))) = {
#define GLR_GEN_INTERPOSE
#include "gl_simple.inc"
#undef GLR_GEN_INTERPOSE
    INTERPOSE(w_glGenTextures, glGenTextures), INTERPOSE(w_glGenBuffers, glGenBuffers),
    INTERPOSE(w_glGenProgramsARB, glGenProgramsARB), INTERPOSE(w_glGenFramebuffersEXT, glGenFramebuffersEXT),
    INTERPOSE(w_glGenRenderbuffersEXT, glGenRenderbuffersEXT), INTERPOSE(w_glGenQueriesARB, glGenQueriesARB),
    INTERPOSE(w_glDeleteTextures, glDeleteTextures), INTERPOSE(w_glDeleteBuffers, glDeleteBuffers),
    INTERPOSE(w_glDeleteProgramsARB, glDeleteProgramsARB), INTERPOSE(w_glDeleteFramebuffersEXT, glDeleteFramebuffersEXT),
    INTERPOSE(w_glDeleteRenderbuffersEXT, glDeleteRenderbuffersEXT), INTERPOSE(w_glDeleteQueriesARB, glDeleteQueriesARB),
    INTERPOSE(w_glDeleteShader, glDeleteShader), INTERPOSE(w_glDeleteProgram, glDeleteProgram),
    INTERPOSE(w_glBindTexture, glBindTexture), INTERPOSE(w_glBindBuffer, glBindBuffer),
    INTERPOSE(w_glBindProgramARB, glBindProgramARB), INTERPOSE(w_glBindFramebufferEXT, glBindFramebufferEXT),
    INTERPOSE(w_glBindRenderbufferEXT, glBindRenderbufferEXT), INTERPOSE(w_glUseProgram, glUseProgram),
    INTERPOSE(w_glFramebufferTexture2DEXT, glFramebufferTexture2DEXT),
    INTERPOSE(w_glFramebufferRenderbufferEXT, glFramebufferRenderbufferEXT),
    INTERPOSE(w_glCreateShader, glCreateShader), INTERPOSE(w_glCreateProgram, glCreateProgram),
    INTERPOSE(w_glShaderSource, glShaderSource), INTERPOSE(w_glCompileShader, glCompileShader),
    INTERPOSE(w_glAttachShader, glAttachShader), INTERPOSE(w_glLinkProgram, glLinkProgram),
    INTERPOSE(w_glBindAttribLocation, glBindAttribLocation), INTERPOSE(w_glGetUniformLocation, glGetUniformLocation),
    INTERPOSE(w_glGetAttribLocation, glGetAttribLocation), INTERPOSE(w_glUniform1f, glUniform1f),
    INTERPOSE(w_glUniform1i, glUniform1i), INTERPOSE(w_glUniform4fv, glUniform4fv),
    INTERPOSE(w_glBeginQueryARB, glBeginQueryARB), INTERPOSE(w_glGetQueryObjectuivARB, glGetQueryObjectuivARB),
    INTERPOSE(w_glProgramStringARB, glProgramStringARB), INTERPOSE(w_glProgramEnvParameter4fvARB, glProgramEnvParameter4fvARB),
    INTERPOSE(w_glVertexAttrib4fv, glVertexAttrib4fv), INTERPOSE(w_glTexParameterfv, glTexParameterfv),
    INTERPOSE(w_glBufferData, glBufferData), INTERPOSE(w_glBufferSubData, glBufferSubData),
    INTERPOSE(w_glTexImage2D, glTexImage2D), INTERPOSE(w_glTexSubImage2D, glTexSubImage2D),
    INTERPOSE(w_glTexImage3D, glTexImage3D), INTERPOSE(w_glCompressedTexImage2D, glCompressedTexImage2D),
    INTERPOSE(w_glVertexAttribPointer, glVertexAttribPointer),
    INTERPOSE(w_glDrawElements, glDrawElements), INTERPOSE(w_glDrawRangeElements, glDrawRangeElements),
    INTERPOSE(w_glDrawArrays, glDrawArrays), INTERPOSE(w_glFlush, glFlush),
    INTERPOSE(w_glGetError, glGetError), INTERPOSE(w_glGetIntegerv, glGetIntegerv),
    INTERPOSE(w_glGetFloatv, glGetFloatv), INTERPOSE(w_glGetBooleanv, glGetBooleanv),
    INTERPOSE(w_glIsEnabled, glIsEnabled),
};
