/* Research probe: CPU cost per draw call on the legacy Mac GL stack (Apple's
   GL driver on PowerPC G4 + Radeon 9550/9700 mobility) under controlled
   changes of state between draws.  A hidden CGL context with no drawable
   (an FBO, 256x256) is used; each draw is a tiny triangle, so the GPU time
   is negligible and the figure is the driver's CPU time.

   usage: probe [groups] [name-substring]
     groups: letters from  a b c d e f g h  (default all)
       a baseline and redundant state    b fragment program kind and switching
       c texture switching               d vertex parameters (ARB env, GLSL)
       e fragment parameters             f vertex arrays, indices, buffers
       g texture parameters, FBO flips, state toggles
       h many distinct programs / program combinations
   env: DRAWS (default 1000), REPS (default 3)
   Output: us per draw (wall, with glFinish), the submit part before glFinish,
   and the system-time part (kernel entry: Mach messages to the GPU driver). */
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#endif

#define TS 64
#define RT 256

/* ---- timing ---- */
static double now_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1.0e6 + tv.tv_usec;
}
static double tv_us(struct timeval t) { return t.tv_sec * 1.0e6 + t.tv_usec; }

/* ---- objects ---- */
typedef struct { GLenum target; GLuint id; } tex_t;

enum { K_PLAIN, K_CUBE, K_FOUR, K_UNIF, K_ENVF, K_COUNT };
enum { M_GLSL, M_ARB, M_FF };

static GLuint glsl_fp[K_COUNT][2], arb_fp[K_COUNT][2];
static GLuint arb_vp_basic, glsl_many[64], arb_many[64], vp_many[16];
static GLuint glsl_vp[3];              /* GLSL vs+fs, u[1], u[16], u[100] */
static GLuint glsl_vp_loc[3];
static tex_t T_dxt1a, T_dxt1b, T_dxt3a, T_dxt5a, T_dxt5b, T_rgba_a, T_rgba_b, T_rtt, T_cube_a, T_cube_b;
static GLuint fbo_main, fbo_rtt, rb_main;
static GLuint vbo_a, vbo_b, ebo_a, ebo_b, vbo_stream;
static int have_env_params_ext;
static int have_nofinish_note;
static void (*p_env_params_ext)(GLenum, GLuint, GLsizei, const GLfloat *);

static float vtx_a[3 * 16] __attribute__((aligned(16)));
static float vtx_b[3 * 16] __attribute__((aligned(16)));
static unsigned short idx_a[8] __attribute__((aligned(16))) = { 0, 1, 2 };
static unsigned short idx_b[8] __attribute__((aligned(16))) = { 0, 1, 2 };
static float pbuf[256 * 4] __attribute__((aligned(16)));

/* ---- configuration of one test ---- */
static struct {
    int mode, kind, prog_switch, redundant;
    tex_t ul[4][8];
    int un[4];
    int units;
    int vp_glsl;       /* 0 none, else 1..3 index into glsl_vp */
    int vp_n;          /* ARB vertex program reading this many env/local params: 0,1,16,100,195 */
    int vp_local;
    int vp_style;      /* see arb_vs_text */
    int nprog;         /* >2: cycle this many distinct fragment programs (PLAIN kind) */
    int nvp;           /* >0: cycle this many distinct vertex programs */
    int vp_set;        /* 0 none, 1 per-call, 2 batched (EXT), 3 per-call same values, 4 glsl one call, 5 glsl per vec */
    int vp_set_n;
    int fpu_mode;      /* 0 none, 1 glsl all, 2 arb local, 3 arb env, 4 glsl per vec */
    int fpu_n;
    int vbo_mode;      /* 0 static VBO, 1 repoint same, 2 alternate VBOs, 3 static client, 4 repoint client, 5 alternate client, 6 bind only, 7 BufferSubData per draw, 8 BufferData orphan per draw */
    int nattr;
    int idx_mode;      /* 0 client, 1 element VBO, 2 alternate element VBOs, 3 DrawArrays */
    int sampler;       /* 0, 1 filter toggle, 2 wrap toggle, 3 both */
    int fbo_flip;      /* 1 flip with clear, 2 flip with a small draw into the rtt */
    int enable_toggle; /* toggle GL_BLEND per draw */
    int state_toggle;  /* 1 blend, 2 depth func, 3 cull, 4 scissor */
} cfg;

/* ---- shaders ---- */
static GLuint make_glsl(const char *vs, const char *fs, int bind_attribs)
{
    GLuint p = glCreateProgram(), s;
    GLint ok = 0;
    if (vs) {
        s = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(s, 1, &vs, NULL); glCompileShader(s);
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { char l[1024]; GLint n = 0; glGetShaderInfoLog(s, 1023, &n, l); l[n > 0 ? n : 0] = 0; fprintf(stderr, "vs: %s\n", l); exit(2); }
        glAttachShader(p, s);
    }
    s = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(s, 1, &fs, NULL); glCompileShader(s);
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char l[1024]; GLint n = 0; glGetShaderInfoLog(s, 1023, &n, l); l[n > 0 ? n : 0] = 0; fprintf(stderr, "fs: %s\n%s\n", l, fs); exit(2); }
    glAttachShader(p, s);
    if (bind_attribs) { glBindAttribLocation(p, 0, "a0"); glBindAttribLocation(p, 1, "a1"); }
    glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { fprintf(stderr, "link failed\n"); exit(2); }
    glUseProgram(p);
    {
        int i;
        char n[8];
        for (i = 0; i < 4; i++) { GLint l; sprintf(n, "tex%d", i); l = glGetUniformLocation(p, n); if (l >= 0) glUniform1i(l, i); }
    }
    glUseProgram(0);
    return p;
}

static const char *glsl_fs_text(int kind, int variant)
{
    static char buf[K_COUNT][2][1024];
    const char *mul = variant ? "vec4(0.5,0.6,0.7,1.0)" : "vec4(1.0)";
    char *b = buf[kind][variant];
    switch (kind) {
    case K_PLAIN:
        sprintf(b, "#version 120\nuniform sampler2D tex0;\nvoid main(){gl_FragColor=texture2D(tex0,gl_TexCoord[0].xy)*%s;}\n", mul);
        break;
    case K_CUBE:
        sprintf(b, "#version 120\nuniform sampler2D tex0;uniform samplerCube tex1;\nvoid main(){gl_FragColor=texture2D(tex0,gl_TexCoord[0].xy)*textureCube(tex1,gl_TexCoord[0].xyz)*%s;}\n", mul);
        break;
    case K_FOUR:
        sprintf(b, "#version 120\nuniform sampler2D tex0;uniform sampler2D tex1;uniform sampler2D tex2;uniform sampler2D tex3;\nvoid main(){vec2 t=gl_TexCoord[0].xy;gl_FragColor=(texture2D(tex0,t)*texture2D(tex1,t)+texture2D(tex2,t)*texture2D(tex3,t))*%s;}\n", mul);
        break;
    default:
        sprintf(b, "#version 120\nuniform sampler2D tex0;uniform vec4 c[8];\nvoid main(){vec4 t=texture2D(tex0,gl_TexCoord[0].xy)*c[0];t=t*c[1]+c[2];t=t*c[3]+c[4];t=t*c[5]+c[6];gl_FragColor=t*c[7]*%s;}\n", mul);
        break;
    }
    return b;
}

static const char *arb_fs_text(int kind, int variant)
{
    static char buf[K_COUNT][2][1024];
    char *b = buf[kind][variant];
    const char *c = variant ? "{0.5,0.6,0.7,1.0}" : "{1.0,1.0,1.0,1.0}";
    switch (kind) {
    case K_PLAIN:
        sprintf(b, "!!ARBfp1.0\nPARAM m=%s;TEMP t;\nTEX t,fragment.texcoord[0],texture[0],2D;\nMUL result.color,t,m;\nEND\n", c);
        break;
    case K_CUBE:
        sprintf(b, "!!ARBfp1.0\nPARAM m=%s;TEMP t,u;\nTEX t,fragment.texcoord[0],texture[0],2D;\nTEX u,fragment.texcoord[0],texture[1],CUBE;\nMUL t,t,u;MUL result.color,t,m;\nEND\n", c);
        break;
    case K_FOUR:
        sprintf(b, "!!ARBfp1.0\nPARAM m=%s;TEMP t,u,v,w;\nTEX t,fragment.texcoord[0],texture[0],2D;\nTEX u,fragment.texcoord[0],texture[1],2D;\nTEX v,fragment.texcoord[0],texture[2],2D;\nTEX w,fragment.texcoord[0],texture[3],2D;\nMUL t,t,u;MAD t,v,w,t;MUL result.color,t,m;\nEND\n", c);
        break;
    default:
        sprintf(b, "!!ARBfp1.0\nPARAM m=%s;PARAM c[8]={program.%s[0..7]};TEMP t;\nTEX t,fragment.texcoord[0],texture[0],2D;\nMUL t,t,c[0];MAD t,t,c[1],c[2];MAD t,t,c[3],c[4];MAD t,t,c[5],c[6];MUL t,t,c[7];MUL result.color,t,m;\nEND\n",
                c, kind == K_ENVF ? "env" : "local");
        break;
    }
    return b;
}

static const char *glsl_many_text(int v)
{
    static char b[512];
    sprintf(b, "#version 120\nuniform sampler2D tex0;\nvoid main(){gl_FragColor=texture2D(tex0,gl_TexCoord[0].xy)*vec4(%d.0e-2,0.5,0.7,1.0)+vec4(%d.0e-3);}\n", v + 1, v);
    return b;
}
static const char *arb_many_text(int v)
{
    static char b[512];
    sprintf(b, "!!ARBfp1.0\nPARAM m={%d.0e-2,0.5,0.7,1.0};PARAM a={%d.0e-3,%d.0e-3,%d.0e-3,%d.0e-3};TEMP t;\nTEX t,fragment.texcoord[0],texture[0],2D;\nMAD result.color,t,m,a;\nEND\n", v + 1, v, v, v, v);
    return b;
}

static GLuint make_arb(GLenum target, const char *text, const char *tag)
{
    GLuint p;
    GLint pos = -1, ok = -1;
    glGenProgramsARB(1, &p);
    glBindProgramARB(target, p);
    glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)strlen(text), text);
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
    if (pos != -1) { fprintf(stderr, "%s: ARB error at %d: %s\n%s\n", tag, (int)pos, (const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB), text); exit(2); }
    glGetProgramivARB(target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &ok);
    if (!ok) printf("note: %s is NOT within native limits\n", tag);
    return p;
}

/* style 0: n individual PARAMs as the port's translator writes them (PARAM cK = program.env[K]);
   style 1: one PARAM array of n read with constant indices;
   style 2: array of n read through an address register (ARL A0 from an attribute), as the translator's relative form.
   salt makes otherwise equal programs distinct. */
static const char *arb_vs_text(int n, int local, int style, int salt)
{
    static char b[32768];
    char t[160];
    int i;
    const char *w = local ? "local" : "env";
    sprintf(b, "!!ARBvp1.0\nATTRIB pos=vertex.attrib[0];ATTRIB uv=vertex.attrib[1];\nPARAM k={1.0e-7,1.0e-7,1.0e-7,0.0};\nPARAM salt={%d.0,0.0,0.0,0.0};\nTEMP t;\nADDRESS a;\n", salt);
    if (n > 0 && style == 0)
        for (i = 0; i < n; i++) { sprintf(t, "PARAM c%d=program.%s[%d];\n", i, w, i); strcat(b, t); }
    else if (n > 0) {
        sprintf(t, "PARAM p[%d]={program.%s[0..%d]};\n", n, w, n - 1);
        strcat(b, t);
    }
    strcat(b, "MOV t,pos;\n");
    if (style == 2 && n > 0) {
        strcat(b, "ARL a.x,uv.x;\nMAD t,p[a.x],k,t;\nMAD t,p[a.x+1],k,t;\n");
    } else {
        for (i = 0; i < n; i++) { sprintf(t, style == 0 ? "MAD t,c%d,k,t;\n" : "MAD t,p[%d],k,t;\n", i); strcat(b, t); }
    }
    strcat(b, "MAD t,salt,k,t;MOV result.position,t;MOV result.texcoord[0],uv;\nEND\n");
    return b;
}

static struct { int n, local, style, salt; GLuint id; } vpc[64];
static int vpc_count;
static GLuint get_vp(int n, int local, int style, int salt)
{
    int i;
    char tag[64];
    for (i = 0; i < vpc_count; i++)
        if (vpc[i].n == n && vpc[i].local == local && vpc[i].style == style && vpc[i].salt == salt) return vpc[i].id;
    sprintf(tag, "vp n=%d local=%d style=%d salt=%d", n, local, style, salt);
    vpc[vpc_count].n = n; vpc[vpc_count].local = local; vpc[vpc_count].style = style; vpc[vpc_count].salt = salt;
    vpc[vpc_count].id = make_arb(GL_VERTEX_PROGRAM_ARB, arb_vs_text(n, local, style, salt), tag);
    return vpc[vpc_count++].id;
}

static const char *glsl_vs_text(int n)
{
    static char b[1024];
    sprintf(b, "#version 120\nattribute vec4 a0;attribute vec4 a1;uniform vec4 u[%d];\nvoid main(){vec4 p=a0;for(int k=0;k<%d;k++)p+=u[k]*1.0e-7;gl_Position=p;gl_TexCoord[0]=a1;}\n", n, n);
    return b;
}

/* ---- textures ---- */
static unsigned long rs = 12345;
static unsigned char rbyte(void) { rs = rs * 1103515245UL + 12345UL; return (unsigned char)(rs >> 16); }

static tex_t make_tex(GLenum fmt, int rgba)
{
    tex_t t;
    int lvl, w = TS, h = TS, i;
    static unsigned char data[TS * TS * 4];
    t.target = GL_TEXTURE_2D;
    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    for (lvl = 0; w >= 1 || h >= 1; lvl++) {
        int cw = w ? w : 1, ch = h ? h : 1;
        if (rgba) {
            for (i = 0; i < cw * ch * 4; i++) data[i] = rbyte();
            glTexImage2D(GL_TEXTURE_2D, lvl, GL_RGBA8, cw, ch, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        } else {
            int bs = fmt == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16;
            int sz = ((cw + 3) / 4) * ((ch + 3) / 4) * bs;
            for (i = 0; i < sz; i++) data[i] = rbyte();
            glCompressedTexImage2D(GL_TEXTURE_2D, lvl, fmt, cw, ch, 0, sz, data);
        }
        w >>= 1; h >>= 1;
        if (!w && !h) break;
        if (!w) w = 1;
        if (!h) h = 1;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return t;
}

static tex_t make_cube(void)
{
    tex_t t;
    int f, lvl, w, i;
    static unsigned char data[TS * TS * 4];
    t.target = GL_TEXTURE_CUBE_MAP;
    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, t.id);
    for (f = 0; f < 6; f++)
        for (lvl = 0, w = TS; w >= 1; lvl++, w >>= 1) {
            for (i = 0; i < w * w * 4; i++) data[i] = rbyte();
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, lvl, GL_RGBA8, w, w, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return t;
}

static tex_t make_rtt(void)
{
    tex_t t;
    t.target = GL_TEXTURE_2D;
    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TS, TS, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffersEXT(1, &fbo_rtt);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_rtt);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, t.id, 0);
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) { fprintf(stderr, "rtt incomplete\n"); exit(2); }
    glViewport(0, 0, TS, TS);
    glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_main);
    glViewport(0, 0, RT, RT);
    return t;
}

/* ---- per-draw work ---- */
static void set_attrib_pointers(const float *base_or_null, GLuint vbo)
{
    int a;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (a = 0; a < cfg.nattr; a++)
        glVertexAttribPointer((GLuint)a, 4, GL_FLOAT, GL_FALSE, 64, (const void *)((const char *)(vbo ? (const char *)0 : (const char *)base_or_null) + a * 16));
}

static void set_frag(int v)
{
    if (cfg.mode == M_GLSL) glUseProgram(glsl_fp[cfg.kind][v]);
    else if (cfg.mode == M_ARB) glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, arb_fp[cfg.kind][v]);
    else glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, v ? GL_REPLACE : GL_MODULATE);
}

static void draw_one(int i)
{
    int u, k;
    if (cfg.fbo_flip) {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_rtt);
        glViewport(0, 0, TS, TS);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_main);
        glViewport(0, 0, RT, RT);
    }
    switch (cfg.state_toggle) {
    case 1: if (i & 1) glEnable(GL_BLEND); else glDisable(GL_BLEND); break;
    case 2: glDepthFunc((i & 1) ? GL_LEQUAL : GL_LESS); break;
    case 3: glCullFace((i & 1) ? GL_BACK : GL_FRONT); break;
    case 4: glScissor(0, 0, (i & 1) ? RT : RT - 1, RT); break;
    default: break;
    }
    if (cfg.nvp) glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vp_many[i % cfg.nvp]);
    if (cfg.nprog > 2) {
        if (cfg.mode == M_GLSL) glUseProgram(glsl_many[i % cfg.nprog]);
        else glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, arb_many[i % cfg.nprog]);
    } else if (!cfg.vp_glsl) {
        if (cfg.prog_switch) set_frag(i & 1);
        else if (cfg.redundant) set_frag(0);
    } else if (cfg.redundant) glUseProgram(glsl_vp[cfg.vp_glsl - 1]);
    for (u = 0; u < cfg.units; u++) {
        if (cfg.un[u] > 1 || cfg.redundant) {
            tex_t t = cfg.ul[u][(i + u) % cfg.un[u]];
            glActiveTexture(GL_TEXTURE0 + u);
            glBindTexture(t.target, t.id);
        }
    }
    if (cfg.units > 1 && (cfg.un[0] > 1 || cfg.un[1] > 1 || cfg.redundant)) glActiveTexture(GL_TEXTURE0);
    if (cfg.sampler) {
        if (cfg.sampler & 1) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (i & 1) ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
        if (cfg.sampler & 2) { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (i & 1) ? GL_CLAMP_TO_EDGE : GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (i & 1) ? GL_CLAMP_TO_EDGE : GL_REPEAT); }
    }
    switch (cfg.vp_set) {
    case 1: case 3:
        for (k = 0; k < cfg.vp_set_n; k++) {
            float v = cfg.vp_set == 3 ? 0.0f : (float)((i & 1) + k) * 1.0e-3f;
            if (cfg.vp_local) glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)k, v, 0, 0, 0);
            else glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)k, v, 0, 0, 0);
        }
        break;
    case 2:
        for (k = 0; k < cfg.vp_set_n; k++) { pbuf[k * 4] = (float)((i & 1) + k) * 1.0e-3f; }
        if (p_env_params_ext) p_env_params_ext(cfg.vp_local ? GL_VERTEX_PROGRAM_ARB : GL_VERTEX_PROGRAM_ARB, 0, cfg.vp_set_n, pbuf);
        break;
    case 4: case 5: {
        GLint loc = glGetUniformLocation(glsl_vp[cfg.vp_glsl - 1], "u");
        for (k = 0; k < cfg.vp_set_n; k++) pbuf[k * 4] = (float)((i & 1) + k) * 1.0e-3f;
        if (cfg.vp_set == 4) glUniform4fv(loc, cfg.vp_set_n, pbuf);
        else for (k = 0; k < cfg.vp_set_n; k++) glUniform4fv(glsl_vp_loc[cfg.vp_glsl - 1] + k, 1, pbuf + k * 4);
        break; }
    default: break;
    }
    if (cfg.fpu_mode) {
        for (k = 0; k < cfg.fpu_n; k++) pbuf[k * 4] = (float)((i & 1) + k) * 1.0e-3f + 1.0f;
        switch (cfg.fpu_mode) {
        case 1: glUniform4fv(glGetUniformLocation(glsl_fp[cfg.kind][0], "c"), cfg.fpu_n, pbuf); break;
        case 4: for (k = 0; k < cfg.fpu_n; k++) glUniform4fv(glGetUniformLocation(glsl_fp[cfg.kind][0], k ? "c[1]" : "c[0]"), 1, pbuf + k * 4); break;
        case 2: for (k = 0; k < cfg.fpu_n; k++) glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)k, pbuf + k * 4); break;
        case 3: for (k = 0; k < cfg.fpu_n; k++) glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)k, pbuf + k * 4); break;
        }
    }
    switch (cfg.vbo_mode) {
    case 1: set_attrib_pointers(NULL, vbo_a); break;
    case 2: set_attrib_pointers(NULL, (i & 1) ? vbo_b : vbo_a); break;
    case 4: set_attrib_pointers(vtx_a, 0); break;
    case 5: set_attrib_pointers((i & 1) ? vtx_b : vtx_a, 0); break;
    case 6: glBindBuffer(GL_ARRAY_BUFFER, vbo_a); break;
    case 7: glBindBuffer(GL_ARRAY_BUFFER, vbo_stream); glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vtx_a), vtx_a); break;
    case 8: glBindBuffer(GL_ARRAY_BUFFER, vbo_stream); glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_a), vtx_a, GL_STREAM_DRAW); break;
    default: break;
    }
    if (cfg.idx_mode == 3) glDrawArrays(GL_TRIANGLES, 0, 3);
    else if (cfg.idx_mode == 0) glDrawRangeElements(GL_TRIANGLES, 0, 2, 3, GL_UNSIGNED_SHORT, idx_a);
    else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (cfg.idx_mode == 2 && (i & 1)) ? ebo_b : ebo_a);
        glDrawRangeElements(GL_TRIANGLES, 0, 2, 3, GL_UNSIGNED_SHORT, (const void *)0);
    }
}

/* ---- test control ---- */
static const char *g_groups = "abcdefgh", *g_filter;
static int g_draws = 1000, g_reps = 3;
static double g_base = 0;
static char g_name[128];
static int g_active;

static void apply_static(void)
{
    int u;
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_main);
    glViewport(0, 0, RT, RT);
    if (cfg.vp_glsl) {
        glDisable(GL_VERTEX_PROGRAM_ARB);
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        glUseProgram(glsl_vp[cfg.vp_glsl - 1]);
    } else {
        GLuint vp = cfg.vp_n ? get_vp(cfg.vp_n, cfg.vp_local, cfg.vp_style, 0) : arb_vp_basic;
        glEnable(GL_VERTEX_PROGRAM_ARB);
        glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vp);
        if (cfg.mode == M_GLSL) { glDisable(GL_FRAGMENT_PROGRAM_ARB); glUseProgram(cfg.nprog > 2 ? glsl_many[0] : glsl_fp[cfg.kind][0]); }
        else if (cfg.mode == M_ARB) { glUseProgram(0); glEnable(GL_FRAGMENT_PROGRAM_ARB); glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cfg.nprog > 2 ? arb_many[0] : arb_fp[cfg.kind][0]); }
        else {
            glUseProgram(0); glDisable(GL_FRAGMENT_PROGRAM_ARB);
            glActiveTexture(GL_TEXTURE0); glEnable(GL_TEXTURE_2D);
        }
    }
    for (u = 0; u < 4; u++) {
        if (cfg.un[u] > 0) { glActiveTexture(GL_TEXTURE0 + u); glBindTexture(cfg.ul[u][0].target, cfg.ul[u][0].id); }
    }
    glActiveTexture(GL_TEXTURE0);
    for (u = 0; u < cfg.nattr; u++) glEnableVertexAttribArray((GLuint)u);
    for (; u < 8; u++) glDisableVertexAttribArray((GLuint)u);
    if (cfg.vbo_mode == 3 || cfg.vbo_mode == 4 || cfg.vbo_mode == 5) set_attrib_pointers(vtx_a, 0);
    else if (cfg.vbo_mode == 7 || cfg.vbo_mode == 8) { glBindBuffer(GL_ARRAY_BUFFER, vbo_stream); set_attrib_pointers(NULL, vbo_stream); }
    else set_attrib_pointers(NULL, vbo_a);
    if (cfg.idx_mode == 1 || cfg.idx_mode == 2) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_a);
    else glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    /* constant parameters */
    {
        int k;
        for (k = 0; k < 256; k++) { pbuf[k * 4] = 0; pbuf[k * 4 + 1] = pbuf[k * 4 + 2] = pbuf[k * 4 + 3] = 0.0f; }
        for (k = 0; k < 256; k++) {
            if (!cfg.vp_glsl) {
                glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)k, 0, 0, 0, 0);
                glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)k, 0, 0, 0, 0);
            }
        }
        for (k = 0; k < 8; k++) {
            glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)k, 1, 1, 1, 1);
            glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)k, 1, 1, 1, 1);
        }
        if (cfg.kind >= K_UNIF && cfg.mode == M_GLSL) {
            GLint l = glGetUniformLocation(glsl_fp[cfg.kind][0], "c");
            float one[32];
            for (k = 0; k < 32; k++) one[k] = 1.0f;
            glUniform4fv(l, 8, one);
        }
    }
}

static int begin(const char *name)
{
    memset(&cfg, 0, sizeof(cfg));
    cfg.mode = M_GLSL; cfg.kind = K_PLAIN; cfg.units = 1; cfg.nattr = 4;
    cfg.ul[0][0] = T_rgba_a; cfg.un[0] = 1;
    if (!strchr(g_groups, name[0])) return 0;
    if (g_filter && !strstr(name, g_filter)) return 0;
    strncpy(g_name, name, sizeof(g_name) - 1);
    return 1;
}

static void run(void)
{
    int rep, i;
    double best = 1e30, best_sub = 0, best_sys = 0;
    GLenum err;
    apply_static();
    for (i = 0; i < 100; i++) draw_one(i);
    glFinish();
    glGetError();
    for (rep = 0; rep < g_reps; rep++) {
        struct rusage r0, r1;
        double t0, t1, t2;
        getrusage(RUSAGE_SELF, &r0);
        t0 = now_us();
        for (i = 0; i < g_draws; i++) draw_one(i);
        t1 = now_us();
        glFinish();
        t2 = now_us();
        getrusage(RUSAGE_SELF, &r1);
        if (t2 - t0 < best) {
            best = t2 - t0; best_sub = t1 - t0;
            best_sys = tv_us(r1.ru_stime) - tv_us(r0.ru_stime);
        }
    }
    err = glGetError();
    best /= g_draws; best_sub /= g_draws; best_sys /= g_draws;
    if (!strncmp(g_name, "a: baseline", 11)) g_base = best;
    printf("%-62s %7.1f us  submit %7.1f  sys %6.1f  d%+7.1f%s\n", g_name, best, best_sub, best_sys, best - g_base, err ? "  GLERR" : "");
    fflush(stdout);
}

static void list1(int u, tex_t a) { cfg.ul[u][0] = a; cfg.un[u] = 1; }
static void list2(int u, tex_t a, tex_t b) { cfg.ul[u][0] = a; cfg.ul[u][1] = b; cfg.un[u] = 2; }

static void setup_gl(void)
{
    int i, j;
    GLuint rb_dummy;
    (void)rb_dummy;
    glGenRenderbuffersEXT(1, &rb_main);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb_main);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, RT, RT);
    glGenFramebuffersEXT(1, &fbo_main);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_main);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, rb_main);
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) { fprintf(stderr, "main fbo incomplete\n"); exit(2); }
    glViewport(0, 0, RT, RT);
    glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_CULL_FACE); glDisable(GL_DITHER); glDisable(GL_SCISSOR_TEST);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    /* triangle data: 3 vertices x 4 attributes, interleaved 64-byte stride */
    for (i = 0; i < 3; i++)
        for (j = 0; j < 4; j++) {
            float *a = &vtx_a[i * 16 + j * 4], *b = &vtx_b[i * 16 + j * 4];
            a[0] = j == 0 ? (i == 1 ? -0.88f : -0.9f) : (float)(i & 1);
            a[1] = j == 0 ? (i == 2 ? -0.88f : -0.9f) : (float)(i >> 1);
            a[2] = 0.0f; a[3] = 1.0f;
            memcpy(b, a, 16);
            if (j == 0) b[0] += 0.1f;
        }
    glGenBuffers(1, &vbo_a); glBindBuffer(GL_ARRAY_BUFFER, vbo_a); glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_a), vtx_a, GL_STATIC_DRAW);
    glGenBuffers(1, &vbo_b); glBindBuffer(GL_ARRAY_BUFFER, vbo_b); glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_b), vtx_b, GL_STATIC_DRAW);
    glGenBuffers(1, &vbo_stream); glBindBuffer(GL_ARRAY_BUFFER, vbo_stream); glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_a), vtx_a, GL_STREAM_DRAW);
    glGenBuffers(1, &ebo_a); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_a); glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6, idx_a, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo_b); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_b); glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6, idx_b, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    /* textures */
    T_rgba_a = make_tex(0, 1); T_rgba_b = make_tex(0, 1);
    T_dxt1a = make_tex(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0); T_dxt1b = make_tex(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0);
    T_dxt3a = make_tex(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0);
    T_dxt5a = make_tex(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0); T_dxt5b = make_tex(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0);
    T_cube_a = make_cube(); T_cube_b = make_cube();
    T_rtt = make_rtt();
    /* programs */
    for (i = 0; i < K_COUNT; i++)
        for (j = 0; j < 2; j++) {
            char tag[32];
            glsl_fp[i][j] = make_glsl(NULL, glsl_fs_text(i, j), 0);
            sprintf(tag, "arbfp k%d v%d", i, j);
            arb_fp[i][j] = make_arb(GL_FRAGMENT_PROGRAM_ARB, arb_fs_text(i, j), tag);
        }
    arb_vp_basic = get_vp(0, 0, 0, 0);
    for (i = 0; i < 64; i++) {
        char tag[32];
        glsl_many[i] = make_glsl(NULL, glsl_many_text(i), 0);
        sprintf(tag, "arbfp many %d", i);
        arb_many[i] = make_arb(GL_FRAGMENT_PROGRAM_ARB, arb_many_text(i), tag);
    }
    for (i = 0; i < 16; i++) vp_many[i] = get_vp(31, 0, 0, i + 1);
    {
        static const int ns[3] = { 1, 16, 100 };
        char nm[16];
        for (i = 0; i < 3; i++) {
            glsl_vp[i] = make_glsl(glsl_vs_text(ns[i]), glsl_fs_text(K_PLAIN, 0), 1);
            sprintf(nm, "u[%d]", 0);
            glsl_vp_loc[i] = (GLuint)glGetUniformLocation(glsl_vp[i], "u");
        }
    }
    {
        const char *ext = (const char *)glGetString(GL_EXTENSIONS);
        have_env_params_ext = ext && strstr(ext, "GL_EXT_gpu_program_parameters") != NULL;
#ifdef GL_EXT_gpu_program_parameters
        if (have_env_params_ext) p_env_params_ext = glProgramEnvParameters4fvEXT;
#endif
        printf("GL_EXT_gpu_program_parameters: %s\n", p_env_params_ext ? "yes" : "no (batched case skipped)");
    }
    (void)have_nofinish_note;
}

int main(int argc, char **argv)
{
    CGLPixelFormatAttribute attrs[] = { kCGLPFAAccelerated, kCGLPFAColorSize, (CGLPixelFormatAttribute)24, (CGLPixelFormatAttribute)0 };
    CGLPixelFormatObj fmt = NULL;
    CGLContextObj ctx = NULL;
    GLint nf = 0;
    int n;
    if (getenv("DRAWS")) g_draws = atoi(getenv("DRAWS"));
    if (getenv("REPS")) g_reps = atoi(getenv("REPS"));
    if (argc > 1) g_groups = argv[1];
    if (argc > 2) g_filter = argv[2];
    if (CGLChoosePixelFormat(attrs, &fmt, &nf) || !fmt || CGLCreateContext(fmt, NULL, &ctx) || CGLSetCurrentContext(ctx)) { fprintf(stderr, "no CGL context\n"); return 2; }
    printf("GL_RENDERER=%s VERSION=%s\ndraws=%d reps=%d groups=%s\n", (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION), g_draws, g_reps, g_groups);
    setup_gl();

    /* a: baseline */
    if (begin("a: baseline (GLSL fp + ARB vp, 1 tex, VBO, client idx)")) run();
    strcpy(g_name, "a: baseline"); /* keep the matcher in run() */
    if (begin("a: baseline repeat")) run();
    if (begin("a: redundant rebind of program + texture each draw")) { cfg.redundant = 1; run(); }
    if (begin("a: redundant rebind, ARB fp")) { cfg.mode = M_ARB; cfg.redundant = 1; run(); }
    if (begin("a: ARB fp baseline")) { cfg.mode = M_ARB; run(); }
    if (begin("a: fixed function baseline")) { cfg.mode = M_FF; run(); }
    if (begin("a: GLSL vs+fs baseline (no ARB vp)")) { cfg.vp_glsl = 1; run(); }
    if (begin("a: ARB vp reading 100 env, unchanged")) { cfg.vp_n = 100; run(); }
    if (begin("a: ARB vp reading 195 env, unchanged")) { cfg.vp_n = 195; run(); }
    if (begin("a: DrawArrays instead of indexed")) { cfg.idx_mode = 3; run(); }

    /* b: fragment program kind and switching */
    if (begin("b: GLSL fp, switch between 2 programs")) { cfg.prog_switch = 1; run(); }
    if (begin("b: ARB fp, switch between 2 programs")) { cfg.mode = M_ARB; cfg.prog_switch = 1; run(); }
    if (begin("b: fixed function, switch texenv MODULATE/REPLACE")) { cfg.mode = M_FF; cfg.prog_switch = 1; run(); }
    if (begin("b: GLSL fp 4 textures, switch 2 programs")) { cfg.kind = K_FOUR; cfg.units = 4; for (n = 0; n < 4; n++) list1(n, T_rgba_a); cfg.prog_switch = 1; run(); }
    if (begin("b: ARB fp 4 textures, switch 2 programs")) { cfg.mode = M_ARB; cfg.kind = K_FOUR; cfg.units = 4; for (n = 0; n < 4; n++) list1(n, T_rgba_a); cfg.prog_switch = 1; run(); }
    if (begin("b: GLSL fp 4 textures, no switch")) { cfg.kind = K_FOUR; cfg.units = 4; for (n = 0; n < 4; n++) list1(n, T_rgba_a); run(); }
    if (begin("b: ARB fp 4 textures, no switch")) { cfg.mode = M_ARB; cfg.kind = K_FOUR; cfg.units = 4; for (n = 0; n < 4; n++) list1(n, T_rgba_a); run(); }
    if (begin("b: GLSL switch to glUseProgram(0) and back (ARB vp stays)")) { cfg.prog_switch = 1; run(); }

    /* c: texture switching */
    {
        struct { const char *n; tex_t *a, *b; } pairs[] = {
            { "rgba8 a/b (same)", &T_rgba_a, &T_rgba_b }, { "dxt1 a/b (same)", &T_dxt1a, &T_dxt1b },
            { "dxt5 a/b (same)", &T_dxt5a, &T_dxt5b },
            { "dxt1/dxt5", &T_dxt1a, &T_dxt5a }, { "dxt1/dxt3", &T_dxt1a, &T_dxt3a }, { "dxt3/dxt5", &T_dxt3a, &T_dxt5a },
            { "dxt1/rgba8", &T_dxt1a, &T_rgba_a }, { "dxt5/rgba8", &T_dxt5a, &T_rgba_a },
            { "rgba8/rtt", &T_rgba_a, &T_rtt }, { "dxt1/rtt", &T_dxt1a, &T_rtt }, { "dxt5/rtt", &T_dxt5a, &T_rtt },
            { "rtt/rtt-self", &T_rtt, &T_rtt }
        };
        int p, m;
        char nm[128];
        for (m = 0; m < 2; m++)
            for (p = 0; p < (int)(sizeof(pairs) / sizeof(pairs[0])); p++) {
                sprintf(nm, "c: %s unit0 alternate %s", m ? "ARB fp " : "GLSL fp", pairs[p].n);
                if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; list2(0, *pairs[p].a, *pairs[p].b); run(); }
            }
        for (m = 0; m < 2; m++) {
            sprintf(nm, "c: %s cycle dxt1,dxt3,dxt5,rgba8,rtt", m ? "ARB fp " : "GLSL fp");
            if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; cfg.ul[0][0] = T_dxt1a; cfg.ul[0][1] = T_dxt3a; cfg.ul[0][2] = T_dxt5a; cfg.ul[0][3] = T_rgba_a; cfg.ul[0][4] = T_rtt; cfg.un[0] = 5; run(); }
            sprintf(nm, "c: %s 2D + cube, both constant", m ? "ARB fp " : "GLSL fp");
            if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; cfg.kind = K_CUBE; cfg.units = 2; list1(0, T_dxt1a); list1(1, T_cube_a); run(); }
            sprintf(nm, "c: %s cube a/b alternate (2D const)", m ? "ARB fp " : "GLSL fp");
            if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; cfg.kind = K_CUBE; cfg.units = 2; list1(0, T_dxt1a); list2(1, T_cube_a, T_cube_b); run(); }
            sprintf(nm, "c: %s 2D dxt1/dxt5 alternate with cube const", m ? "ARB fp " : "GLSL fp");
            if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; cfg.kind = K_CUBE; cfg.units = 2; list2(0, T_dxt1a, T_dxt5a); list1(1, T_cube_a); run(); }
            sprintf(nm, "c: %s 4 units constant dxt1/dxt3/dxt5/rgba8", m ? "ARB fp " : "GLSL fp");
            if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; cfg.kind = K_FOUR; cfg.units = 4; list1(0, T_dxt1a); list1(1, T_dxt3a); list1(2, T_dxt5a); list1(3, T_rgba_a); run(); }
            sprintf(nm, "c: %s 4 units, all alternate rgba8 a/b", m ? "ARB fp " : "GLSL fp");
            if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; cfg.kind = K_FOUR; cfg.units = 4; for (n = 0; n < 4; n++) list2(n, T_rgba_a, T_rgba_b); run(); }
            sprintf(nm, "c: %s 4 units, all alternate dxt1 a/b", m ? "ARB fp " : "GLSL fp");
            if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; cfg.kind = K_FOUR; cfg.units = 4; for (n = 0; n < 4; n++) list2(n, T_dxt1a, T_dxt1b); run(); }
            sprintf(nm, "c: %s 4 units, rotate dxt1,dxt3,dxt5,rgba8", m ? "ARB fp " : "GLSL fp");
            if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; cfg.kind = K_FOUR; cfg.units = 4; for (n = 0; n < 4; n++) { cfg.ul[n][0] = T_dxt1a; cfg.ul[n][1] = T_dxt3a; cfg.ul[n][2] = T_dxt5a; cfg.ul[n][3] = T_rgba_a; cfg.un[n] = 4; } run(); }
        }
        if (begin("c: fixed function unit0 alternate dxt1/dxt5")) { cfg.mode = M_FF; list2(0, T_dxt1a, T_dxt5a); run(); }
        if (begin("c: fixed function unit0 alternate rgba8 a/b")) { cfg.mode = M_FF; list2(0, T_rgba_a, T_rgba_b); run(); }
    }

    /* d: vertex parameters */
    {
        static const int ns[4] = { 1, 16, 100, 192 };
        int k, st;
        char nm[128];
        static const char *sn[3] = { "individual PARAMs", "PARAM array, const index", "PARAM array, address reg" };
        for (st = 0; st < 3; st++)
            for (k = 0; k < 4; k++) {
                sprintf(nm, "d: ARB vp %s n=%d, unchanged", sn[st], ns[k]);
                if (begin(nm)) { cfg.vp_n = ns[k]; cfg.vp_style = st; run(); }
            }
        for (k = 0; k < 4; k++) {
            sprintf(nm, "d: ARB vp indiv n=%d, all %d changed per draw (4f)", ns[k], ns[k]);
            if (begin(nm)) { cfg.vp_n = ns[k]; cfg.vp_set = 1; cfg.vp_set_n = ns[k]; run(); }
        }
        for (k = 0; k < 3; k++) {
            sprintf(nm, "d: ARB vp indiv n=%d, same values re-set per draw", ns[k]);
            if (begin(nm)) { cfg.vp_n = ns[k]; cfg.vp_set = 3; cfg.vp_set_n = ns[k]; run(); }
        }
        if (p_env_params_ext)
            for (k = 0; k < 4; k++) {
                sprintf(nm, "d: ARB vp indiv n=%d, one glProgramEnvParameters4fvEXT", ns[k]);
                if (begin(nm)) { cfg.vp_n = ns[k]; cfg.vp_set = 2; cfg.vp_set_n = ns[k]; run(); }
            }
        for (k = 0; k < 3; k++) {
            sprintf(nm, "d: ARB vp LOCAL indiv n=%d, all changed per draw", ns[k]);
            if (begin(nm)) { cfg.vp_n = ns[k]; cfg.vp_local = 1; cfg.vp_set = 1; cfg.vp_set_n = ns[k]; run(); }
        }
        for (k = 0; k < 3; k++) {
            static const int gn[3] = { 1, 16, 100 };
            sprintf(nm, "d: GLSL vs, uniform vec4 u[%d] one glUniform4fv per draw", gn[k]);
            if (begin(nm)) { cfg.vp_glsl = k + 1; cfg.vp_set = 4; cfg.vp_set_n = gn[k]; run(); }
        }
        if (begin("d: GLSL vs, u[16] as 16 glUniform4fv per draw")) { cfg.vp_glsl = 2; cfg.vp_set = 5; cfg.vp_set_n = 16; run(); }
        if (begin("d: GLSL vs, u[100] unchanged")) { cfg.vp_glsl = 3; run(); }
        if (begin("d: ARB vp indiv n=100, 16 of them changed per draw")) { cfg.vp_n = 100; cfg.vp_set = 1; cfg.vp_set_n = 16; run(); }
        if (begin("d: ARB vp indiv n=100, 1 of them changed per draw")) { cfg.vp_n = 100; cfg.vp_set = 1; cfg.vp_set_n = 1; run(); }
        if (begin("d: ARB vp indiv n=31 (typical), 31 changed per draw")) { cfg.vp_n = 31; cfg.vp_set = 1; cfg.vp_set_n = 31; run(); }
        if (begin("d: ARB vp indiv n=31 (typical), 31 changed, one EXT call")) { cfg.vp_n = 31; cfg.vp_set = 2; cfg.vp_set_n = 31; run(); }
        if (begin("d: ARB vp array sweep n=128 unchanged")) { cfg.vp_n = 128; cfg.vp_style = 1; run(); }
        if (begin("d: ARB vp array sweep n=140 unchanged")) { cfg.vp_n = 140; cfg.vp_style = 1; run(); }
        if (begin("d: ARB vp array sweep n=160 unchanged")) { cfg.vp_n = 160; cfg.vp_style = 1; run(); }
        if (begin("d: ARB vp array sweep n=180 unchanged")) { cfg.vp_n = 180; cfg.vp_style = 1; run(); }
        if (begin("d: ARB vp array n=192 changed one param per draw")) { cfg.vp_n = 192; cfg.vp_style = 1; cfg.vp_set = 1; cfg.vp_set_n = 1; run(); }
        if (begin("d: ARB vp address-reg array n=192, 1 param changed per draw")) { cfg.vp_n = 192; cfg.vp_style = 2; cfg.vp_set = 1; cfg.vp_set_n = 1; run(); }
    }

    /* e: fragment parameters */
    {
        static const int ns[3] = { 1, 8, 8 };
        int k, m;
        char nm[128];
        for (k = 0; k < 2; k++) {
            sprintf(nm, "e: GLSL fp uniform c[8], %d vec4 changed per draw (one call)", ns[k]);
            if (begin(nm)) { cfg.kind = K_UNIF; cfg.fpu_mode = 1; cfg.fpu_n = ns[k]; run(); }
            sprintf(nm, "e: ARB fp local, %d params changed per draw", ns[k]);
            if (begin(nm)) { cfg.mode = M_ARB; cfg.kind = K_UNIF; cfg.fpu_mode = 2; cfg.fpu_n = ns[k]; run(); }
            sprintf(nm, "e: ARB fp env, %d params changed per draw", ns[k]);
            if (begin(nm)) { cfg.mode = M_ARB; cfg.kind = K_ENVF; cfg.fpu_mode = 3; cfg.fpu_n = ns[k]; run(); }
        }
        if (begin("e: GLSL fp uniform c[0],c[1] two glUniform4fv per draw")) { cfg.kind = K_UNIF; cfg.fpu_mode = 4; cfg.fpu_n = 2; run(); }
        if (begin("e: GLSL fp uniform program, nothing changed")) { cfg.kind = K_UNIF; run(); }
        if (begin("e: ARB fp local program, nothing changed")) { cfg.mode = M_ARB; cfg.kind = K_UNIF; run(); }
        for (m = 0; m < 1; m++) {}
    }

    /* f: arrays and indices */
    if (begin("f: VBO attribs re-pointed (same VBO) each draw, 4 attribs")) { cfg.vbo_mode = 1; run(); }
    if (begin("f: VBO attribs re-pointed (same VBO) each draw, 8 attribs? (4 shown)")) { cfg.vbo_mode = 1; cfg.nattr = 2; run(); }
    if (begin("f: VBO alternate VBO a/b, re-point 4 attribs")) { cfg.vbo_mode = 2; run(); }
    if (begin("f: glBindBuffer(ARRAY) only each draw")) { cfg.vbo_mode = 6; run(); }
    if (begin("f: client arrays, pointers set once")) { cfg.vbo_mode = 3; run(); }
    if (begin("f: client arrays, pointers re-set each draw")) { cfg.vbo_mode = 4; run(); }
    if (begin("f: client arrays alternate a/b each draw")) { cfg.vbo_mode = 5; run(); }
    if (begin("f: VBO glBufferSubData 192 B per draw")) { cfg.vbo_mode = 7; run(); }
    if (begin("f: VBO glBufferData (orphan) 192 B per draw")) { cfg.vbo_mode = 8; run(); }
    if (begin("f: element VBO (bound once)")) { cfg.idx_mode = 1; run(); }
    if (begin("f: element VBO alternate a/b")) { cfg.idx_mode = 2; run(); }
    if (begin("f: client arrays + client indices, ARB fp")) { cfg.mode = M_ARB; cfg.vbo_mode = 3; run(); }
    if (begin("f: VBO alternate + switch GLSL programs + alternate textures")) { cfg.vbo_mode = 2; cfg.prog_switch = 1; list2(0, T_dxt1a, T_dxt5a); run(); }
    if (begin("f: 2 attributes only, VBO static")) { cfg.nattr = 2; run(); }

    /* g: texture parameters, FBO flips, state toggles */
    if (begin("g: glTexParameteri min filter toggle per draw")) { cfg.sampler = 1; run(); }
    if (begin("g: glTexParameteri wrap toggle per draw")) { cfg.sampler = 2; run(); }
    if (begin("g: glTexParameteri min filter + wrap toggle, ARB fp")) { cfg.mode = M_ARB; cfg.sampler = 3; run(); }
    if (begin("g: FBO flip (rtt clear) each draw, tex unrelated")) { cfg.fbo_flip = 1; run(); }
    if (begin("g: FBO flip each draw, then sample rtt, GLSL")) { cfg.fbo_flip = 1; list1(0, T_rtt); run(); }
    if (begin("g: FBO flip each draw, then sample rtt, ARB")) { cfg.mode = M_ARB; cfg.fbo_flip = 1; list1(0, T_rtt); run(); }
    if (begin("g: glBlend toggle per draw")) { cfg.state_toggle = 1; run(); }
    if (begin("g: glDepthFunc toggle per draw (depth off)")) { cfg.state_toggle = 2; run(); }
    if (begin("g: glCullFace toggle per draw (cull off)")) { cfg.state_toggle = 3; run(); }
    if (begin("g: glScissor change per draw (scissor off)")) { cfg.state_toggle = 4; run(); }
    /* h: many distinct programs (new combinations) */
    {
        static const int np[4] = { 2, 8, 32, 64 };
        int k, m;
        char nm[128];
        for (m = 0; m < 2; m++)
            for (k = 0; k < 4; k++) {
                sprintf(nm, "h: %s fp cycle %d distinct programs", m ? "ARB " : "GLSL", np[k]);
                if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; cfg.nprog = np[k] < 3 ? 0 : np[k]; cfg.prog_switch = np[k] == 2; if (np[k] > 2) cfg.kind = K_PLAIN; run(); }
            }
        for (m = 0; m < 2; m++) {
            sprintf(nm, "h: %s fp cycle 8 programs x dxt1/dxt5 textures", m ? "ARB " : "GLSL");
            if (begin(nm)) { cfg.mode = m ? M_ARB : M_GLSL; cfg.nprog = 8; list2(0, T_dxt1a, T_dxt5a); run(); }
        }
        if (begin("h: ARB vp cycle 2 distinct vertex programs (n=31)")) { cfg.nvp = 2; run(); }
        if (begin("h: ARB vp cycle 8 distinct vertex programs (n=31)")) { cfg.nvp = 8; run(); }
        if (begin("h: ARB vp cycle 16 distinct vertex programs (n=31)")) { cfg.nvp = 16; run(); }
        if (begin("h: 8 vp x 8 GLSL fp cycled together")) { cfg.nvp = 8; cfg.nprog = 8; run(); }
        if (begin("h: 8 vp x 8 ARB fp cycled together")) { cfg.nvp = 8; cfg.nprog = 8; cfg.mode = M_ARB; run(); }
        if (begin("h: 16 vp x 7 GLSL fp (113 combos) cycled")) { cfg.nvp = 16; cfg.nprog = 7; run(); }
        if (begin("h: 16 vp x 7 ARB fp (113 combos) cycled")) { cfg.nvp = 16; cfg.nprog = 7; cfg.mode = M_ARB; run(); }
    }
    ctx = ctx; fmt = fmt;
    return 0;
}
