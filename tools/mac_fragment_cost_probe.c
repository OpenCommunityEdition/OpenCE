/* Research probe: fragment cost of the generated NV2A pixel shaders on the
   legacy Mac GL stack.  For each shader serial it renders N full-screen
   quads (hidden CGL context, FBO, an ARB vertex passthrough) with

     orig     the shader as port/mac generates it (GLSL 1.20)
     opt      same shader rebuilt by tools/mac_fragment_ir.py (GLSL, folded)
     optproj  opt with texture2DProj on the varying
     arb      ARB_fragment_program of the same dataflow (TXP straight from
              the interpolant)
     arbmul   arb with the texture_scale multiply in front of each TXP

   and reports ns per pixel, the native ARB counts and how far each result
   differs from orig.  Also --calib: synthetic ARB programs of k MADs and
   t texture fetches, to turn ns/pixel into instruction equivalents.
   usage: probe <dir with dump/ and opt/> [--calib] [serial ...] */
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define W 640
#define H 480
#ifndef GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x880A
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x880C
#endif

static double now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *b;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    b = (char *)malloc((size_t)n + 1);
    if (fread(b, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(b); return NULL; }
    b[n] = 0;
    fclose(f);
    return b;
}

static GLuint arb_vertex;

static GLuint make_arb(GLenum target, const char *text, int report, const char *tag)
{
    GLuint p;
    GLint pos = -1;
    glGenProgramsARB(1, &p);
    glBindProgramARB(target, p);
    glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)strlen(text), text);
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
    if (pos != -1) {
        fprintf(stderr, "%s: ARB error at %d: %s\n", tag, (int)pos, (const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB));
        return 0;
    }
    if (report) {
        GLint alu = -1, tex = -1, ind = -1, ok = -1, tmp = -1;
        glGetProgramivARB(target, GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &alu);
        glGetProgramivARB(target, GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB, &tex);
        glGetProgramivARB(target, GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &ind);
        glGetProgramivARB(target, GL_PROGRAM_NATIVE_TEMPORARIES_ARB, &tmp);
        glGetProgramivARB(target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &ok);
        printf("    native %s: alu=%d tex=%d indirections=%d temps=%d under_limits=%d\n", tag, (int)alu, (int)tex, (int)ind, (int)tmp, (int)ok);
    }
    return p;
}

static GLuint make_glsl(const char *frag, const char *tag)
{
    GLuint s = glCreateShader(GL_FRAGMENT_SHADER), p;
    GLint ok = 0;
    glShaderSource(s, 1, &frag, NULL);
    glCompileShader(s);
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLint n = 0;
        glGetShaderInfoLog(s, sizeof(log) - 1, &n, log);
        log[n > 0 ? n : 0] = 0;
        fprintf(stderr, "%s: GLSL compile: %s\n", tag, log);
        return 0;
    }
    p = glCreateProgram();
    glAttachShader(p, s);
    glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { fprintf(stderr, "%s: GLSL link failed\n", tag); return 0; }
    return p;
}

/* shared constants */
static float c0[8][4], c1[8][4], fc0[4], fc1[4], fogc[4] = { 0.5f, 0.6f, 0.7f, 1.0f }, scale[4][4];
static float alpha_ref = 128.0f, alpha_thr;
static char alpha_op[8];

static void set_glsl_uniforms(GLuint p)
{
    GLint l;
    int i;
    glUseProgram(p);
    if ((l = glGetUniformLocation(p, "ps_c0")) >= 0) glUniform4fv(l, 8, &c0[0][0]);
    if ((l = glGetUniformLocation(p, "ps_c1")) >= 0) glUniform4fv(l, 8, &c1[0][0]);
    if ((l = glGetUniformLocation(p, "ps_final_c0")) >= 0) glUniform4fv(l, 1, fc0);
    if ((l = glGetUniformLocation(p, "ps_final_c1")) >= 0) glUniform4fv(l, 1, fc1);
    if ((l = glGetUniformLocation(p, "fog_color")) >= 0) glUniform4fv(l, 1, fogc);
    if ((l = glGetUniformLocation(p, "alpha_reference")) >= 0) glUniform1f(l, alpha_ref);
    if ((l = glGetUniformLocation(p, "texture_scale")) >= 0) glUniform4fv(l, 4, &scale[0][0]);
    for (i = 0; i < 4; i++) {
        char n[8];
        sprintf(n, "tex%d", i);
        if ((l = glGetUniformLocation(p, n)) >= 0) glUniform1i(l, i);
    }
}

static void set_arb_locals(void)
{
    int i;
    for (i = 0; i < 8; i++) {
        glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)i, c0[i]);
        glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)(8 + i), c1[i]);
    }
    glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 16, fc0);
    glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 17, fc1);
    for (i = 0; i < 4; i++) glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)(18 + i), scale[i]);
    glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 22, fogc);
    glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 24, alpha_thr, alpha_thr, alpha_thr, alpha_thr);
}

static void quad(void)
{
    /* corner colours differ; 2-D coordinates sweep two repeats; the cube
       direction sweeps a face-crossing range */
    static const float px[4][2] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
    static const float uv[4][2] = { { 0, 0 }, { 2, 0 }, { 2, 2 }, { 0, 2 } };
    static const float dir[4][3] = { { -1, -1, 0.7f }, { 1, -1, 0.5f }, { 1, 1, 0.9f }, { -1, 1, 0.3f } };
    static const float col[4][4] = { { 0.9f, 0.2f, 0.4f, 0.8f }, { 0.3f, 0.8f, 0.5f, 0.6f }, { 0.6f, 0.5f, 0.9f, 0.9f }, { 0.1f, 0.7f, 0.2f, 0.7f } };
    int i;
    glBegin(GL_QUADS);
    for (i = 0; i < 4; i++) {
        glMultiTexCoord4fARB(GL_TEXTURE0_ARB, col[i][2], col[i][0], col[i][1], 1.0f);
        glMultiTexCoord4fARB(GL_TEXTURE1_ARB, col[i][1], col[i][3], col[i][0], 0.5f);
        glMultiTexCoord4fARB(GL_TEXTURE2_ARB, uv[i][0], uv[i][1], 0.0f, 1.0f);
        glMultiTexCoord4fARB(GL_TEXTURE3_ARB, uv[i][1], uv[i][0], 0.0f, 1.0f);
        glMultiTexCoord4fARB(GL_TEXTURE4_ARB, uv[i][0] * 0.5f, uv[i][1] * 1.5f, 0.0f, 1.0f);
        glMultiTexCoord4fARB(GL_TEXTURE5_ARB, dir[i][0], dir[i][1], dir[i][2], 1.0f);
        glMultiTexCoord4fARB(GL_TEXTURE6_ARB, col[i][0], col[i][1], col[i][2], col[i][3]);
        glMultiTexCoord4fARB(GL_TEXTURE7_ARB, col[i][3], col[i][2], col[i][1], col[i][0]);
        glVertex3f(px[i][0], px[i][1], 0.0f);
    }
    glEnd();
}

static unsigned char *shot(void)
{
    unsigned char *b = (unsigned char *)malloc(W * H * 4);
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, b);
    return b;
}

static int draws = 12;

/* min over 5 repeats of ms per draw */
static double timeit(void)
{
    int r, i;
    double best = 1e30;
    quad();
    glFinish();
    for (r = 0; r < 5; r++) {
        double t0 = now_ms(), t;
        for (i = 0; i < draws; i++) quad();
        glFinish();
        t = (now_ms() - t0) / draws;
        if (t < best) best = t;
    }
    return best;
}

static double ns_per_pixel(double ms) { return ms * 1e6 / (W * H); }

static void compare(const unsigned char *a, const unsigned char *b, double *mean, int *maxd, int *over2)
{
    long i, n = (long)W * H * 4;
    double sum = 0;
    *maxd = 0; *over2 = 0;
    for (i = 0; i < n; i++) {
        int d = abs((int)a[i] - (int)b[i]);
        sum += d;
        if (d > *maxd) *maxd = d;
        if (d > 2) (*over2)++;
    }
    *mean = sum / n;
}

static void use_glsl(GLuint p)
{
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    set_glsl_uniforms(p);
}
static void use_arb(GLuint p)
{
    glUseProgram(0);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, p);
    set_arb_locals();
}

static GLuint tex2d[3], texcube;

static void make_textures(void)
{
    int i, s, f;
    for (i = 0; i < 3; i++) {
        unsigned char *d = (unsigned char *)malloc(256 * 256 * 4);
        unsigned seed = 12345u + (unsigned)i * 7777u;
        for (s = 0; s < 256 * 256 * 4; s++) {
            seed = seed * 1664525u + 1013904223u;
            d[s] = (unsigned char)(seed >> 24);
        }
        /* smooth with a ramp so the image has structure */
        for (s = 0; s < 256 * 256; s++) {
            int x = s & 255, y = s >> 8;
            d[s * 4 + 0] = (unsigned char)((d[s * 4 + 0] / 3) + (x ^ y) / 2 + 20 * i);
            d[s * 4 + 3] = (unsigned char)(d[s * 4 + 3] / 2 + (y) / 2);
        }
        glGenTextures(1, &tex2d[i]);
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, tex2d[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, d);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        free(d);
    }
    glGenTextures(1, &texcube);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texcube);
    for (f = 0; f < 6; f++) {
        unsigned char d[64 * 64 * 4];
        unsigned seed = 999u + (unsigned)f * 31u;
        for (s = 0; s < 64 * 64 * 4; s++) {
            seed = seed * 1664525u + 1013904223u;
            d[s] = (unsigned char)(seed >> 24);
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, d);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* every unit also carries the other targets so any sampler layout works */
    for (i = 0; i < 4; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        if (i == 3) { glBindTexture(GL_TEXTURE_2D, tex2d[0]); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); }
        else { glBindTexture(GL_TEXTURE_CUBE_MAP, texcube); }
    }
}

static void calibration(void)
{
    static const int mads[] = { 0, 4, 8, 16, 32, 48 };
    static const int texs[] = { 0, 1, 2, 4 };
    int i, j;
    printf("calibration (ARB, dependent MAD chain on a varying, ns/pixel):\n");
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 6; i++) {
            char text[8192];
            int k, n = 0;
            GLuint p;
            double ms;
            n += sprintf(text + n, "!!ARBfp1.0\nTEMP R0, R1, R2, R3;\nMOV R0, fragment.texcoord[6];\n");
            for (k = 0; k < texs[j]; k++)
                n += sprintf(text + n, "TXP R%d, fragment.texcoord[%d], texture[%d], 2D;\nMUL R0, R0, R%d;\n", 1 + (k & 1), 2 + k, k < 3 ? k : 0, 1 + (k & 1));
            for (k = 0; k < mads[i]; k++)
                n += sprintf(text + n, "MAD R0, R0, fragment.texcoord[7], fragment.texcoord[6];\n");
            n += sprintf(text + n, "MOV_SAT result.color, R0;\nEND\n");
            p = make_arb(GL_FRAGMENT_PROGRAM_ARB, text, 0, "calib");
            if (!p) continue;
            use_arb(p);
            ms = timeit();
            {
                GLint alu = -1, tex = -1, ind = -1;
                glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &alu);
                glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB, &tex);
                glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &ind);
                printf("  tex=%d mad=%2d  %.3f ms/frame  %.2f ns/pixel  (native alu=%d tex=%d ind=%d)\n", texs[j], mads[i], ms, ns_per_pixel(ms), (int)alu, (int)tex, (int)ind);
            }
        }
    }
}

int main(int argc, char **argv)
{
    CGLPixelFormatAttribute attrs[] = { kCGLPFAAccelerated, kCGLPFAColorSize, (CGLPixelFormatAttribute)24, (CGLPixelFormatAttribute)0 };
    CGLPixelFormatObj fmt = NULL;
    CGLContextObj ctx = NULL;
    GLint nf = 0;
    GLuint fbo, color, vp;
    const char *dir = argc > 1 ? argv[1] : ".";
    int i, calib = 0, a;
    static const char vp_text[] =
        "!!ARBvp1.0\nMOV result.position, vertex.position;\n"
        "MOV result.texcoord[0], vertex.texcoord[0];\nMOV result.texcoord[1], vertex.texcoord[1];\n"
        "MOV result.texcoord[2], vertex.texcoord[2];\nMOV result.texcoord[3], vertex.texcoord[3];\n"
        "MOV result.texcoord[4], vertex.texcoord[4];\nMOV result.texcoord[5], vertex.texcoord[5];\n"
        "MOV result.texcoord[6], vertex.texcoord[6];\nMOV result.texcoord[7], vertex.texcoord[7];\nEND\n";
    unsigned char *ref_img = NULL, *img;
    if (getenv("DRAWS")) draws = atoi(getenv("DRAWS"));
    if (CGLChoosePixelFormat(attrs, &fmt, &nf) || !fmt || CGLCreateContext(fmt, NULL, &ctx) || CGLSetCurrentContext(ctx)) {
        fprintf(stderr, "no CGL context\n");
        return 2;
    }
    printf("GL_RENDERER=%s VERSION=%s\n", (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION));
    glGenRenderbuffersEXT(1, &color);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, color);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, W, H);
    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, color);
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) { fprintf(stderr, "fbo incomplete %x\n", (unsigned)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)); return 2; }
    glViewport(0, 0, W, H);
    glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_CULL_FACE); glDisable(GL_DITHER);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    make_textures();
    vp = make_arb(GL_VERTEX_PROGRAM_ARB, vp_text, 0, "vp");
    arb_vertex = vp;
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vp);
    for (i = 0; i < 8; i++) {
        int k;
        for (k = 0; k < 4; k++) {
            c0[i][k] = 0.15f + 0.1f * (float)((i * 3 + k * 5) % 7);
            c1[i][k] = 0.9f - 0.08f * (float)((i * 5 + k * 3) % 9);
        }
    }
    for (i = 0; i < 4; i++) { fc0[i] = 0.25f + 0.2f * (float)i; fc1[i] = 0.8f - 0.15f * (float)i; scale[i][0] = scale[i][1] = scale[i][2] = scale[i][3] = 1.0f; }
    glClearColor(0, 0, 0, 0);
    if (getenv("BLEND")) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); printf("alpha blending on\n"); }

    {
        /* trivial shader: the cost of the quad itself */
        static const char t[] = "!!ARBfp1.0\nMOV result.color, fragment.texcoord[6];\nEND\n";
        GLuint p = make_arb(GL_FRAGMENT_PROGRAM_ARB, t, 0, "trivial");
        double ms;
        use_arb(p);
        ms = timeit();
        printf("trivial ARB (1 MOV): %.3f ms/frame  %.2f ns/pixel\n", ms, ns_per_pixel(ms));
    }
    for (a = 2; a < argc; a++) if (!strcmp(argv[a], "--calib")) calib = 1;
    if (calib) calibration();
    for (a = 2; a < argc; a++) {
        unsigned serial;
        char path[512], *src, *st;
        GLuint p[5];
        double ms[5];
        const char *name[5] = { "orig", "opt", "optproj", "arb", "arbmul" };
        int v;
        if (argv[a][0] == '-') continue;
        serial = (unsigned)atoi(argv[a]);
        sprintf(path, "%s/opt/shader_%04u.stats", dir, serial);
        st = read_file(path);
        alpha_op[0] = 0;
        if (st) {
            char *q = strstr(st, "alpha_test ");
            if (q) sscanf(q + 11, "%7s", alpha_op);
            if (!strcmp(alpha_op, "None")) alpha_op[0] = 0;
            printf("shader %u: %s", serial, strstr(st, "tex_stages") ? strstr(st, "tex_stages") : "");
            { char *d = strstr(st, "arb_vp"); if (d) printf("  %s", d); }
        } else printf("shader %u: (no stats)\n", serial);
        if (!strcmp(alpha_op, ">=")) alpha_thr = (alpha_ref - 0.5f) / 255.0f;
        else if (!strcmp(alpha_op, "<=")) alpha_thr = (alpha_ref + 0.5f) / 255.0f;
        else if (!strcmp(alpha_op, "<")) alpha_thr = (alpha_ref - 0.5f) / 255.0f;
        else alpha_thr = (alpha_ref + 0.5f) / 255.0f;
        memset(p, 0, sizeof(p));
        sprintf(path, "%s/dump/shader_%04u.frag", dir, serial); src = read_file(path); if (src) p[0] = make_glsl(src, "orig"); free(src);
        sprintf(path, "%s/opt/shader_%04u.opt.frag", dir, serial); src = read_file(path); if (src) p[1] = make_glsl(src, "opt"); free(src);
        sprintf(path, "%s/opt/shader_%04u.optproj.frag", dir, serial); src = read_file(path); if (src) p[2] = make_glsl(src, "optproj"); free(src);
        sprintf(path, "%s/opt/shader_%04u.arb", dir, serial); src = read_file(path); if (src) p[3] = make_arb(GL_FRAGMENT_PROGRAM_ARB, src, 1, "arb"); free(src);
        sprintf(path, "%s/opt/shader_%04u.mul.arb", dir, serial); src = read_file(path); if (src) p[4] = make_arb(GL_FRAGMENT_PROGRAM_ARB, src, 1, "arbmul"); free(src);
        ref_img = NULL;
        for (v = 0; v < 5; v++) {
            double mean = 0; int maxd = 0, over2 = 0;
            if (!p[v]) { printf("    %-8s unavailable\n", name[v]); continue; }
            if (v < 3) use_glsl(p[v]); else use_arb(p[v]);
            glClear(GL_COLOR_BUFFER_BIT);
            quad();
            glFinish();
            img = shot();
            if (v == 0) ref_img = img;
            else if (ref_img) compare(ref_img, img, &mean, &maxd, &over2);
            ms[v] = timeit();
            printf("    %-8s %7.3f ms/frame %7.2f ns/pixel   vs orig: mean|d|=%.3f max=%d n(d>2)=%d\n", name[v], ms[v], ns_per_pixel(ms[v]), mean, maxd, over2);
            if (v != 0) free(img);
        }
        free(ref_img);
        free(st);
        glUseProgram(0);
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        for (v = 0; v < 3; v++) if (p[v]) glDeleteProgram(p[v]);
    }
    CGLSetCurrentContext(NULL);
    CGLDestroyContext(ctx);
    return 0;
}
