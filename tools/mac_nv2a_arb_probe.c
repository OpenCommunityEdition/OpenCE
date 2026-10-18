/* Host-side checks of port/mac/mac_nv2a_arb.c without a GPU.

   - retail vertex shader table entries 31 and 50 (the data is in the
     repository's sources, no game data is needed) translate with the bone
     gather, are refused without it, and the program text is well formed;
   - a small ARB interpreter that is also a checker (declarations before use,
     known opcodes, no reads of undefined temporary components, no reads of
     outputs, every result written) runs the generated text beside a direct
     interpreter of the NV2A words that follows port/linux/src/nv2a_vsh.c
     (the GLSL translator), on random programs and on both retail shaders,
     and the outputs are compared;
   - the refusal cases (unsupported operations, malformed gathers).
   Build with -ffp-contract=off so that the reference does not fuse. */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../port/mac/mac_nv2a_arb.h"
#include "../port/mac/mac_bone_gather.h"

static const unsigned long vertex_shader_code[] =
{
#include "../source/rasterizer/xbox/rasterizer_xbox_vertex_shaders_data.inc"
};

static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); } } while (0)

/* ---------- helpers on program words */

static unsigned long field(const unsigned long *p, int word, int low, int count)
{
    return ((p[word] & 0xFFFFFFFFUL) >> low) & ((1UL << count) - 1);
}

static void set(unsigned long *p, int word, int low, int count, unsigned long value)
{
    unsigned long mask = ((1UL << count) - 1) << low;
    p[word] = ((p[word] & ~mask) | ((value << low) & mask)) & 0xFFFFFFFFUL;
}

static unsigned long seed_state = 12345;
static unsigned long rnd(void)
{
    seed_state = (seed_state * 1103515245UL + 12345UL) & 0x7FFFFFFFUL;
    return (seed_state >> 8) & 0x7FFFFF;
}
static unsigned long below(unsigned long n) { return rnd() % n; }

static float random_value(void)
{
    switch (below(8)) {
    case 0: return 0.0f;
    case 1: return 1.0f;
    case 2: return -1.0f;
    case 3: return 0.5f;
    default: return ((float)below(8001) - 4000.0f) / 1000.0f;
    }
}

/* ---------- the retail table */

static int retail_entry(unsigned index, const unsigned long **code, unsigned long *count)
{
    static unsigned long offsets[67];
    static int loaded;
    if (!loaded) {
        FILE *f = fopen("source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.c", "r");
        char line[256];
        unsigned n = 0;
        if (!f) return 0;
        while (fgets(line, sizeof(line), f) && n < 67) {
            unsigned long offset, bytes;
            if (sscanf(line, " VERTEX_SHADER_ENTRY(0x%lX, 0x%lX)", &offset, &bytes) == 2 &&
                strstr(line, "#define") == NULL) offsets[n++] = offset;
        }
        fclose(f);
        if (n != 67) return 0;
        loaded = 1;
    }
    if (index >= 67) return 0;
    *count = (vertex_shader_code[offsets[index] / 4] & 0xFFFFFFFFUL) >> 16;
    *code = vertex_shader_code + offsets[index] / 4 + 1;
    return 1;
}

/* ---------- reference: the GLSL translator's semantics, directly */

struct ref_out {
    float pos[4], tex[8][4], fog, psize;
    int finite;   /* the position before the w guard is finite */
};

static float fclamp(float x, float lo, float hi)
{
    /* a NaN stays a NaN (and is then not compared): clamp(NaN) is unspecified */
    return x < lo ? lo : (x > hi ? hi : x);
}

static void ref_operand(const unsigned long *p, unsigned which, float r[13][4],
    const float attr[16][4], const float c[192][4], int a0, float out[4])
{
    unsigned long negate, sw[4], index, mux, input = field(p, 1, 9, 4), constant = field(p, 1, 13, 8);
    float v[4];
    unsigned k;
    if (which == 0) {
        negate = field(p, 1, 8, 1); sw[0] = field(p, 1, 6, 2); sw[1] = field(p, 1, 4, 2);
        sw[2] = field(p, 1, 2, 2); sw[3] = field(p, 1, 0, 2); index = field(p, 2, 28, 4); mux = field(p, 2, 26, 2);
    } else if (which == 1) {
        negate = field(p, 2, 25, 1); sw[0] = field(p, 2, 23, 2); sw[1] = field(p, 2, 21, 2);
        sw[2] = field(p, 2, 19, 2); sw[3] = field(p, 2, 17, 2); index = field(p, 2, 13, 4); mux = field(p, 2, 11, 2);
    } else {
        negate = field(p, 2, 10, 1); sw[0] = field(p, 2, 8, 2); sw[1] = field(p, 2, 6, 2);
        sw[2] = field(p, 2, 4, 2); sw[3] = field(p, 2, 2, 2);
        index = (field(p, 2, 0, 2) << 2) | field(p, 3, 30, 2); mux = field(p, 3, 28, 2);
    }
    memset(v, 0, sizeof(v));
    if (mux == 1) memcpy(v, r[index], sizeof(v));
    else if (mux == 2) memcpy(v, attr[input], sizeof(v));
    else if (mux == 3) {
        if (field(p, 3, 1, 1)) {
            long row = (long)a0 + (long)constant;
            row = row < 0 ? 0 : (row > 191 ? 191 : row);
            memcpy(v, c[row], sizeof(v));
        } else memcpy(v, c[constant], sizeof(v));
    }
    for (k = 0; k < 4; ++k) out[k] = negate ? -v[sw[k]] : v[sw[k]];
}

static void ref_run(const unsigned long *prog, unsigned long count, const float attr[16][4],
    const float c[192][4], const float vs[4], const float vo[4], float so, float ps, struct ref_out *o)
{
    float r[13][4], od0[4] = { 0, 0, 0, 1 }, od1[4] = { 0, 0, 0, 1 }, ob0[4] = { 0, 0, 0, 1 }, ob1[4] = { 0, 0, 0, 1 };
    float ot[4][4], ofog[4], opts[4], unused[4], clip[4], pos_final[4];
    float *out_regs[13];
    int a0 = 0, captured = 0;
    unsigned long i;
    unsigned k;
    memset(r, 0, sizeof(r));
    for (k = 0; k < 4; ++k) { ot[k][0] = ot[k][1] = ot[k][2] = 0; ot[k][3] = 1; }
    r[12][3] = 1.0f;
    for (k = 0; k < 4; ++k) { ofog[k] = 1.0f; opts[k] = ps; unused[k] = 0; clip[k] = 0; }
    for (k = 0; k < 13; ++k) out_regs[k] = unused;
    out_regs[0] = r[12]; out_regs[3] = od0; out_regs[4] = od1; out_regs[5] = ofog; out_regs[6] = opts;
    out_regs[7] = ob0; out_regs[8] = ob1; out_regs[9] = ot[0]; out_regs[10] = ot[1]; out_regs[11] = ot[2]; out_regs[12] = ot[3];
    for (i = 0; i < count; ++i) {
        const unsigned long *p = prog + i * 4;
        unsigned long mac = field(p, 1, 21, 4), ilu = field(p, 1, 25, 3), mac_mask = field(p, 3, 24, 4);
        unsigned long temporary = field(p, 3, 20, 4), ilu_mask = field(p, 3, 16, 4), out_mask = field(p, 3, 12, 4);
        float A[4], B[4], C[4], m[4] = { 0, 0, 0, 0 }, u[4] = { 0, 0, 0, 0 };
        ref_operand(p, 0, r, attr, c, a0, A);
        ref_operand(p, 1, r, attr, c, a0, B);
        ref_operand(p, 2, r, attr, c, a0, C);
        switch (mac) {
        case 1: for (k = 0; k < 4; ++k) m[k] = A[k]; break;
        case 2: for (k = 0; k < 4; ++k) m[k] = A[k] * B[k]; break;
        case 3: for (k = 0; k < 4; ++k) m[k] = A[k] + C[k]; break;
        case 4: for (k = 0; k < 4; ++k) { float t = A[k] * B[k]; m[k] = t + C[k]; } break;
        case 5: { float d = A[0] * B[0]; d = d + A[1] * B[1]; d = d + A[2] * B[2]; for (k = 0; k < 4; ++k) m[k] = d; } break;
        case 6: { float d = A[0] * B[0]; d = d + A[1] * B[1]; d = d + A[2] * B[2]; d = d + B[3]; for (k = 0; k < 4; ++k) m[k] = d; } break;
        case 7: { float d = A[0] * B[0]; d = d + A[1] * B[1]; d = d + A[2] * B[2]; d = d + A[3] * B[3]; for (k = 0; k < 4; ++k) m[k] = d; } break;
        case 8: m[0] = 1.0f; m[1] = A[1] * B[1]; m[2] = A[2]; m[3] = B[3]; break;
        case 9: for (k = 0; k < 4; ++k) m[k] = B[k] < A[k] ? B[k] : A[k]; break;
        case 10: for (k = 0; k < 4; ++k) m[k] = A[k] < B[k] ? B[k] : A[k]; break;
        case 11: for (k = 0; k < 4; ++k) m[k] = A[k] < B[k] ? 1.0f : 0.0f; break;
        case 12: for (k = 0; k < 4; ++k) m[k] = A[k] >= B[k] ? 1.0f : 0.0f; break;
        case 13: for (k = 0; k < 4; ++k) m[k] = A[k]; break;
        default: break;
        }
        switch (ilu) {
        case 1: for (k = 0; k < 4; ++k) u[k] = C[k]; break;
        case 2: for (k = 0; k < 4; ++k) u[k] = 1.0f / C[0]; break;
        case 3: {
            float v = 1.0f / C[0];
            v = v > 0.0f ? fclamp(v, 5.42101e-20f, 1.884467e+19f) : fclamp(v, -1.884467e+19f, -5.42101e-20f);
            for (k = 0; k < 4; ++k) u[k] = v;
        } break;
        case 4: for (k = 0; k < 4; ++k) u[k] = 1.0f / sqrtf(fabsf(C[0])); break;
        case 5: u[0] = exp2f(floorf(C[0])); u[1] = C[0] - floorf(C[0]); u[2] = exp2f(C[0]); u[3] = 1.0f; break;
        case 6: {
            float x = fabsf(C[0]);
            if (x == 0.0f) { u[0] = -1.0e30f; u[1] = 1.0f; u[2] = -1.0e30f; u[3] = 1.0f; }
            else { float e = floorf(log2f(x)); u[0] = e; u[1] = x / exp2f(e); u[2] = log2f(x); u[3] = 1.0f; }
        } break;
        case 7: {
            float s = C[0] > 0.0f ? powf(C[1] > 0.0f ? C[1] : 0.0f, fclamp(C[3], -127.9961f, 127.9961f)) : 0.0f;
            u[0] = 1.0f; u[1] = C[0] > 0.0f ? C[0] : 0.0f; u[2] = s; u[3] = 1.0f;
        } break;
        default: break;
        }
        if (ilu == 3 && field(p, 3, 28, 2) == 1 && ((field(p, 2, 0, 2) << 2) | field(p, 3, 30, 2)) == 12) {
            memcpy(clip, r[12], sizeof(clip)); captured = 1;
        }
        if (mac == 13) {
            float f = floorf(m[0] + 0.001f);
            a0 = f < -1.0e9f ? -1000000000 : (f > 1.0e9f ? 1000000000 : (int)f);
        } else if (mac != 0 && mac_mask) {
            for (k = 0; k < 4; ++k) if (mac_mask & (8u >> k)) r[temporary][k] = m[k];
        }
        if (ilu != 0 && ilu_mask) {
            unsigned long t = mac != 0 ? 1 : temporary;
            for (k = 0; k < 4; ++k) if (ilu_mask & (8u >> k)) r[t][k] = u[k];
        }
        if (out_mask && field(p, 3, 11, 1) && (field(p, 3, 2, 1) ? ilu : mac) != 0) {
            float *dest = out_regs[field(p, 3, 3, 8) < 13 ? field(p, 3, 3, 8) : 1];
            const float *src = field(p, 3, 2, 1) ? u : m;
            if (field(p, 3, 3, 8) >= 13 || field(p, 3, 3, 8) == 1 || field(p, 3, 3, 8) == 2) dest = unused;
            for (k = 0; k < 4; ++k) if (out_mask & (8u >> k)) dest[k] = src[k];
        }
        if (field(p, 3, 0, 1)) break;
    }
    {
        float scale[3], pre[4];
        for (k = 0; k < 3; ++k) scale[k] = vs[k] != 0.0f ? vs[k] : 1.0f;
        if (captured) {
            for (k = 0; k < 3; ++k) {
                float h = k == 0 ? 0.5f + so : (k == 1 ? 0.5f : 0.0f);
                float t = (c[59][k] + h) - vo[k];
                float a = clip[k] * c[58][k];
                pre[k] = (a + t * clip[3]) / scale[k];
            }
            pre[3] = clip[3];
        } else {
            for (k = 0; k < 3; ++k) {
                float v = k == 0 ? r[12][0] + (0.5f + so) : (k == 1 ? r[12][1] + 0.5f : r[12][2]);
                pre[k] = ((v - vo[k]) / scale[k]) * r[12][3];
            }
            pre[3] = r[12][3];
        }
        o->finite = pre[0] - pre[0] == 0.0f && pre[1] - pre[1] == 0.0f && pre[2] - pre[2] == 0.0f && pre[3] - pre[3] == 0.0f;
        memcpy(pos_final, pre, sizeof(pos_final));
        if (!(fabsf(pos_final[3]) > 0.0f)) { pos_final[0] = pos_final[1] = pos_final[2] = 0.0f; pos_final[3] = -1.0f; }
        pos_final[2] = 2.0f * pos_final[2] - pos_final[3];
        memcpy(o->pos, pos_final, sizeof(o->pos));
    }
    for (k = 0; k < 4; ++k) {
        o->tex[0][k] = fclamp(ob0[k], 0.0f, 1.0f); o->tex[1][k] = fclamp(ob1[k], 0.0f, 1.0f);
        o->tex[2][k] = ot[0][k]; o->tex[3][k] = ot[1][k]; o->tex[4][k] = ot[2][k]; o->tex[5][k] = ot[3][k];
        o->tex[6][k] = fclamp(od0[k], 0.0f, 1.0f); o->tex[7][k] = fclamp(od1[k], 0.0f, 1.0f);
    }
    o->fog = ofog[0];
    o->psize = opts[0];
}

/* ---------- the ARB interpreter and checker */

#define MAX_REGS 700
#define MAX_STATEMENTS 4096

struct stmt {
    char op[8];
    int dst, dst_mask;       /* register, component bits */
    int nsrc, src[3], neg[3];
    int swz[3][4], swn[3];   /* swizzle indices; swn is 1 for a scalar swizzle */
    int rel[3], reloff[3];   /* source is program.env[A0.x + reloff] */
};

struct arb {
    char names[MAX_REGS][40];
    int kind[MAX_REGS];      /* 0 temporary, 1 attribute, 2 parameter, 3 result */
    int bind[MAX_REGS];      /* attribute or environment index */
    float init[MAX_REGS][4]; /* parameter literals */
    int env[MAX_REGS];       /* -1, or the program.env index */
    int nregs;
    struct stmt st[MAX_STATEMENTS];
    int nst;
    char error[160];
    int ops_seen[32];
    int have_cr;             /* PARAM cr[192] = { program.env[0..191] }, the array A0 indexes */
};

struct arb_state {
    float v[MAX_REGS][4];
    int a0;
    int defined[MAX_REGS];
};

static int arb_fail(struct arb *a, const char *what, const char *detail)
{
    sprintf(a->error, "%s: %.100s", what, detail);
    return 0;
}

static int find_reg(struct arb *a, const char *name)
{
    int i;
    for (i = 0; i < a->nregs; ++i) if (!strcmp(a->names[i], name)) return i;
    return -1;
}

static int add_reg(struct arb *a, const char *name, int kind)
{
    if (a->nregs >= MAX_REGS || find_reg(a, name) >= 0 || strlen(name) >= sizeof(a->names[0])) return -1;
    strcpy(a->names[a->nregs], name);
    a->kind[a->nregs] = kind;
    a->env[a->nregs] = -1;
    return a->nregs++;
}

static int is_swizzle(const char *s)
{
    size_t n = strlen(s), k;
    if (n < 1 || n > 4) return 0;
    for (k = 0; k < n; ++k) if (!strchr("xyzw", s[k])) return 0;
    return 1;
}

/* cr[A0.x + n][.swizzle] with a possible leading minus: 1 and the
   offset, 0 when the text is not a relative reference. The offset must lie
   in ARB_vertex_program's -64..63 and the text must be exactly that form. */
static int parse_relative(char *text, int *neg, int *swz, int *swn, int *offset)
{
    char *close;
    int k, n = 0, value = 0;
    while (*text == ' ') ++text;
    *neg = 0;
    if (*text == '-') { *neg = 1; ++text; }
    if (strncmp(text, "cr[A0.x", 7)) return 0;
    close = strchr(text, ']');
    if (!close) return -1;
    if (text + 7 != close) {
        char sign = 0;
        if (sscanf(text + 7, " %c %d%n", &sign, &value, &n) != 2 || (sign != '+' && sign != '-') ||
            text + 7 + n != close || value < 0) return -1;
        if (sign == '-') value = -value;
    }
    if (value < -64 || value > 63) return -1;
    *offset = value;
    *swn = 0;
    if (close[1] == '.') {
        if (!is_swizzle(close + 2)) return -1;
        *swn = (int)strlen(close + 2);
        for (k = 0; k < *swn; ++k) swz[k] = (int)(strchr("xyzw", close[2 + k]) - "xyzw");
    } else if (close[1]) return -1;
    if (*swn == 1) { swz[1] = swz[2] = swz[3] = swz[0]; *swn = 4; }
    if (*swn == 0) for (k = 0; k < 4; ++k) swz[k] = k;
    else if (*swn != 4) return -1;
    return 1;
}

/* name[.swizzle] with a possible leading minus; returns the register */
static int parse_ref(struct arb *a, char *text, int *neg, int *swz, int *swn, int *mask_bits, int dest)
{
    char *dot;
    int reg, k;
    while (*text == ' ') ++text;
    *neg = 0;
    if (*text == '-') { *neg = 1; ++text; }
    dot = strrchr(text, '.');
    *swn = 0;
    if (dot && is_swizzle(dot + 1)) {
        *swn = (int)strlen(dot + 1);
        for (k = 0; k < *swn; ++k) swz[k] = (int)(strchr("xyzw", dot[1 + k]) - "xyzw");
        if (mask_bits) { *mask_bits = 0; for (k = 0; k < *swn; ++k) *mask_bits |= 1 << swz[k]; }
        *dot = 0;
    } else if (mask_bits) *mask_bits = 15;
    if (*swn == 1 && !dest) { swz[1] = swz[2] = swz[3] = swz[0]; *swn = 4; }
    if (*swn == 0) for (k = 0; k < 4; ++k) swz[k] = k;
    else if (*swn != 4 && !dest) return -2;
    reg = find_reg(a, text);
    return reg;
}

static int arb_parse(struct arb *a, const char *source)
{
    char *copy = (char *)malloc(strlen(source) + 1), *s, *next;
    static const char *ops[] = { "MOV", "MUL", "ADD", "SUB", "MAD", "DP3", "DPH", "DP4", "DST", "MIN", "MAX",
        "SLT", "SGE", "RCP", "RSQ", "ABS", "FLR", "EX2", "LG2", "ARL", NULL };
    int first = 1, ended = 0, k;
    memset(a, 0, sizeof(*a));
    strcpy(copy, source);
    for (k = 0; k < 8; ++k) {
        char name[32];
        sprintf(name, "result.texcoord[%d]", k);
        add_reg(a, name, 3);
    }
    add_reg(a, "result.position", 3);
    add_reg(a, "result.pointsize", 3);
    for (s = copy; s && *s; s = next) {
        char *semi = strchr(s, ';'), *nl = strchr(s, '\n');
        char stmt[512];
        size_t n;
        /* a statement ends at ';'; the header and END end at the line */
        if (first) {
            if (strncmp(s, "!!ARBvp1.0\n", 11)) { free(copy); return arb_fail(a, "header", s); }
            first = 0; next = s + 11; continue;
        }
        if (!strncmp(s, "END\n", 4)) { ended = 1; next = s + 4; if (*next) { free(copy); return arb_fail(a, "text after END", next); } break; }
        if (!semi) { free(copy); return arb_fail(a, "missing semicolon", s); }
        (void)nl;
        n = (size_t)(semi - s);
        if (n >= sizeof(stmt)) { free(copy); return arb_fail(a, "statement too long", s); }
        memcpy(stmt, s, n); stmt[n] = 0;
        next = semi + 1;
        while (*next == '\n' || *next == ' ') ++next;
        {
            char *p = stmt;
            while (*p == ' ' || *p == '\n') ++p;
            if (!strncmp(p, "ATTRIB ", 7)) {
                char name[40]; int idx, r;
                if (sscanf(p + 7, "%39s = vertex.attrib[%d]", name, &idx) != 2 || idx < 0 || idx > 15) { free(copy); return arb_fail(a, "ATTRIB", p); }
                r = add_reg(a, name, 1); if (r < 0) { free(copy); return arb_fail(a, "redeclared", p); }
                a->bind[r] = idx;
            } else if (!strcmp(p, "PARAM cr[192] = { program.env[0..191] }")) {
                if (a->have_cr) { free(copy); return arb_fail(a, "redeclared", p); }
                a->have_cr = 1;
            } else if (!strncmp(p, "PARAM ", 6)) {
                char name[40]; int idx, r; float f[4];
                if (sscanf(p + 6, "%39s = program.env[%d]", name, &idx) == 2 && idx >= 0 && idx < 256) {
                    r = add_reg(a, name, 2); if (r < 0) { free(copy); return arb_fail(a, "redeclared", p); }
                    a->env[r] = idx;
                } else {
                    char *b = strchr(p, '{');
                    char *e = b ? strchr(b, '}') : NULL;
                    char *q;
                    int i;
                    if (!b || !e || sscanf(p + 6, "%39s", name) != 1) { free(copy); return arb_fail(a, "PARAM", p); }
                    q = b + 1;
                    for (i = 0; i < 4; ++i) {
                        char *endp;
                        f[i] = strtof(q, &endp);
                        if (endp == q) { free(copy); return arb_fail(a, "PARAM literal", p); }
                        q = endp; while (*q == ' ' || *q == ',') ++q;
                    }
                    if (q != e) { free(copy); return arb_fail(a, "PARAM literal count", p); }
                    r = add_reg(a, name, 2); if (r < 0) { free(copy); return arb_fail(a, "redeclared", p); }
                    memcpy(a->init[r], f, sizeof(f));
                }
            } else if (!strncmp(p, "ADDRESS ", 8)) {
                if (strcmp(p + 8, "A0") || add_reg(a, "A0", 4) < 0) { free(copy); return arb_fail(a, "ADDRESS", p); }
            } else if (!strncmp(p, "TEMP ", 5)) {
                char *name = p + 5, *comma;
                for (;;) {
                    int r;
                    while (*name == ' ') ++name;
                    comma = strchr(name, ',');
                    if (comma) *comma = 0;
                    r = add_reg(a, name, 0);
                    if (r < 0) { free(copy); return arb_fail(a, "TEMP", name); }
                    if (!comma) break;
                    name = comma + 1;
                }
            } else if (!strncmp(p, "OUTPUT ", 7)) {
                char name[40]; int idx, r;
                if (sscanf(p + 7, "%39s = result.texcoord[%d]", name, &idx) != 2 || idx < 0 || idx > 7) { free(copy); return arb_fail(a, "OUTPUT", p); }
                r = add_reg(a, name, 3); if (r < 0) { free(copy); return arb_fail(a, "redeclared", p); }
                a->bind[r] = 100 + idx;
            } else {
                char op[8], *args, *tok, *save;
                struct stmt *t;
                int oi, nargs = 0, need, reg;
                char *argv[4];
                for (k = 0; p[k] && p[k] != ' ' && k < 7; ++k) op[k] = p[k];
                op[k] = 0;
                for (oi = 0; ops[oi]; ++oi) if (!strcmp(op, ops[oi])) break;
                if (!ops[oi]) { free(copy); return arb_fail(a, "unknown opcode", p); }
                if (a->nst >= MAX_STATEMENTS) { free(copy); return arb_fail(a, "too long", p); }
                t = &a->st[a->nst];
                strcpy(t->op, op);
                args = p + k;
                for (tok = args, save = args; ; ) {
                    char *comma = strchr(save, ',');
                    if (nargs >= 4) { free(copy); return arb_fail(a, "operands", p); }
                    argv[nargs++] = save;
                    if (!comma) break;
                    *comma = 0; save = comma + 1;
                }
                (void)tok;
                need = (!strcmp(op, "MAD")) ? 3 : ((!strcmp(op, "MOV") || !strcmp(op, "RCP") || !strcmp(op, "RSQ") || !strcmp(op, "ABS") ||
                    !strcmp(op, "FLR") || !strcmp(op, "EX2") || !strcmp(op, "LG2") || !strcmp(op, "ARL")) ? 1 : 2);
                if (nargs != need + 1) { free(copy); return arb_fail(a, "operand count", p); }
                {
                    int sw[4], swn, neg, mask;
                    reg = parse_ref(a, argv[0], &neg, sw, &swn, &mask, 1);
                    if (reg < 0 || neg || a->kind[reg] == 1 || a->kind[reg] == 2) { free(copy); return arb_fail(a, "destination", p); }
                    if ((a->kind[reg] == 4) != !strcmp(op, "ARL") || (a->kind[reg] == 4 && (swn != 1 || sw[0] != 0))) { free(copy); return arb_fail(a, "address destination", p); }
                    {
                        /* a destination mask is in x, y, z, w order */
                        int last = -1, i;
                        for (i = 0; i < swn; ++i) { if (sw[i] <= last) { free(copy); return arb_fail(a, "mask order", p); } last = sw[i]; }
                    }
                    t->dst = reg; t->dst_mask = swn ? mask : 15;
                }
                t->nsrc = need;
                for (k = 0; k < need; ++k) {
                    int sw[4], swn, neg, offset = 0, relative;
                    t->rel[k] = 0;
                    relative = parse_relative(argv[k + 1], &neg, sw, &swn, &offset);
                    if (relative < 0) { free(copy); return arb_fail(a, "relative source", p); }
                    if (relative) {
                        if (find_reg(a, "A0") < 0 || !a->have_cr) { free(copy); return arb_fail(a, "no ADDRESS", p); }
                        t->rel[k] = 1; t->reloff[k] = offset; t->src[k] = 0; t->neg[k] = neg;
                        memcpy(t->swz[k], sw, sizeof(sw)); t->swn[k] = swn;
                        continue;
                    }
                    reg = parse_ref(a, argv[k + 1], &neg, sw, &swn, NULL, 0);
                    if (reg < 0 || a->kind[reg] == 4) { free(copy); return arb_fail(a, "source", p); }
                    if (a->kind[reg] == 3) { free(copy); return arb_fail(a, "read of an output", p); }
                    t->src[k] = reg; t->neg[k] = neg;
                    memcpy(t->swz[k], sw, sizeof(sw)); t->swn[k] = swn;
                }
                a->ops_seen[oi] = 1;
                ++a->nst;
            }
        }
    }
    free(copy);
    if (!ended) return arb_fail(a, "no END", "");
    return 1;
}

static int arb_run(struct arb *a, const float env[256][4], const float attr[16][4],
    struct arb_state *s, char *error)
{
    int i, k, r;
    s->a0 = 1 << 20; /* undefined until the program loads it */
    for (r = 0; r < a->nregs; ++r) {
        s->defined[r] = 0;
        memset(s->v[r], 0, sizeof(s->v[r]));
        if (a->kind[r] == 1) { memcpy(s->v[r], attr[a->bind[r]], 16); s->defined[r] = 15; }
        else if (a->kind[r] == 2) {
            if (a->env[r] >= 0) memcpy(s->v[r], env[a->env[r]], 16); else memcpy(s->v[r], a->init[r], 16);
            s->defined[r] = 15;
        }
    }
    for (i = 0; i < a->nst; ++i) {
        const struct stmt *t = &a->st[i];
        float src[3][4], res[4] = { 0, 0, 0, 0 };
        int n;
        for (n = 0; n < t->nsrc; ++n)
            for (k = 0; k < 4; ++k) {
                int c = t->swz[n][k];
                if (t->rel[n]) {
                    long row = (long)s->a0 + t->reloff[n];
                    if (row < 0 || row > 191) { sprintf(error, "statement %d reads program.env[%ld]", i, row); return 0; }
                    src[n][k] = t->neg[n] ? -env[row][c] : env[row][c];
                    continue;
                }
                if (!(s->defined[t->src[n]] & (1 << c))) { sprintf(error, "statement %d reads an undefined component of %s", i, a->names[t->src[n]]); return 0; }
                src[n][k] = t->neg[n] ? -s->v[t->src[n]][c] : s->v[t->src[n]][c];
            }
        if (!strcmp(t->op, "MOV")) for (k = 0; k < 4; ++k) res[k] = src[0][k];
        else if (!strcmp(t->op, "MUL")) for (k = 0; k < 4; ++k) res[k] = src[0][k] * src[1][k];
        else if (!strcmp(t->op, "ADD")) for (k = 0; k < 4; ++k) res[k] = src[0][k] + src[1][k];
        else if (!strcmp(t->op, "SUB")) for (k = 0; k < 4; ++k) res[k] = src[0][k] - src[1][k];
        else if (!strcmp(t->op, "MAD")) for (k = 0; k < 4; ++k) { float p = src[0][k] * src[1][k]; res[k] = p + src[2][k]; }
        else if (!strcmp(t->op, "DP3") || !strcmp(t->op, "DPH") || !strcmp(t->op, "DP4")) {
            float d = src[0][0] * src[1][0];
            d = d + src[0][1] * src[1][1];
            d = d + src[0][2] * src[1][2];
            if (!strcmp(t->op, "DPH")) d = d + src[1][3];
            else if (!strcmp(t->op, "DP4")) d = d + src[0][3] * src[1][3];
            for (k = 0; k < 4; ++k) res[k] = d;
        } else if (!strcmp(t->op, "DST")) { res[0] = 1.0f; res[1] = src[0][1] * src[1][1]; res[2] = src[0][2]; res[3] = src[1][3]; }
        else if (!strcmp(t->op, "MIN")) for (k = 0; k < 4; ++k) res[k] = src[1][k] < src[0][k] ? src[1][k] : src[0][k];
        else if (!strcmp(t->op, "MAX")) for (k = 0; k < 4; ++k) res[k] = src[0][k] < src[1][k] ? src[1][k] : src[0][k];
        else if (!strcmp(t->op, "SLT")) for (k = 0; k < 4; ++k) res[k] = src[0][k] < src[1][k] ? 1.0f : 0.0f;
        else if (!strcmp(t->op, "SGE")) for (k = 0; k < 4; ++k) res[k] = src[0][k] >= src[1][k] ? 1.0f : 0.0f;
        else if (!strcmp(t->op, "RCP")) for (k = 0; k < 4; ++k) res[k] = 1.0f / src[0][0];
        else if (!strcmp(t->op, "RSQ")) for (k = 0; k < 4; ++k) res[k] = 1.0f / sqrtf(fabsf(src[0][0]));
        else if (!strcmp(t->op, "ABS")) for (k = 0; k < 4; ++k) res[k] = fabsf(src[0][k]);
        else if (!strcmp(t->op, "FLR")) for (k = 0; k < 4; ++k) res[k] = floorf(src[0][k]);
        else if (!strcmp(t->op, "EX2")) for (k = 0; k < 4; ++k) res[k] = exp2f(src[0][0]);
        else if (!strcmp(t->op, "LG2")) for (k = 0; k < 4; ++k) res[k] = log2f(src[0][0]);
        if (!strcmp(t->op, "ARL")) {
            if (!(src[0][0] > -1.0e6f && src[0][0] < 1.0e6f)) { sprintf(error, "statement %d: ARL of %g", i, src[0][0]); return 0; }
            s->a0 = (int)floorf(src[0][0]);
            continue;
        }
        for (k = 0; k < 4; ++k) if (t->dst_mask & (1 << k)) { s->v[t->dst][k] = res[k]; s->defined[t->dst] |= 1 << k; }
    }
    /* OUTPUT aliases write the result they name */
    for (r = 0; r < a->nregs; ++r)
        if (a->kind[r] == 3 && a->bind[r] >= 100) {
            char name[32];
            int target;
            sprintf(name, "result.texcoord[%d]", a->bind[r] - 100);
            target = find_reg(a, name);
            for (k = 0; k < 4; ++k)
                if (s->defined[r] & (1 << k)) { s->v[target][k] = s->v[r][k]; s->defined[target] |= 1 << k; }
        }
    /* every result written whole */
    for (r = 0; r < a->nregs; ++r)
        if (a->kind[r] == 3 && !strncmp(a->names[r], "result.", 7)) {
            int need = !strcmp(a->names[r], "result.pointsize") ? 1 : 15;
            if ((s->defined[r] & need) != need) { sprintf(error, "%s is not fully written", a->names[r]); return 0; }
        }
    return 1;
}

/* the value of a result after the run (OUTPUT aliases write to their result) */
static void arb_results(struct arb *a, struct arb_state *s, float tex[8][4], float pos[4], float *psize)
{
    int k;
    for (k = 0; k < 8; ++k) {
        char name[32];
        sprintf(name, "result.texcoord[%d]", k);
        memcpy(tex[k], s->v[find_reg(a, name)], 16);
    }
    memcpy(pos, s->v[find_reg(a, "result.position")], 16);
    *psize = s->v[find_reg(a, "result.pointsize")][0];
}

/* ---------- comparison */

static int close_enough(float ref, float got)
{
    float scale;
    if (!(ref - ref == 0.0f)) return 1;    /* the reference is infinite or NaN: not compared */
    if (got - got != 0.0f) return 0;
    scale = fabsf(ref) > fabsf(got) ? fabsf(ref) : fabsf(got);
    if (scale < 1.0f) scale = 1.0f;
    return fabsf(ref - got) <= 2.0e-4f * scale;
}

/* the fragment-side view of the translator's varyings */
static void unpack(const struct mac_nv2a_arb_layout *l, float tex[8][4], float tex_out[8][4], float *fog)
{
    static const float defaults[4] = { 0, 0, 0, 1 };
    memcpy(tex_out, tex, sizeof(float) * 32);
    *fog = 1.0f;
    if (l->fog_texcoord >= 0) {
        *fog = tex[l->fog_texcoord][l->fog_component];
        tex_out[l->fog_texcoord][l->fog_component] = defaults[l->fog_component];
    }
}

struct compare_totals { unsigned runs, position_skipped, programs, fog_carried, fog_constant, captured, refused; };

static int compare_run(const char *label, const unsigned long *prog, unsigned long count, struct arb *arb,
    const struct mac_nv2a_arb_layout *layout, float env[256][4], float attr[16][4],
    struct compare_totals *totals)
{
    struct arb_state *state = (struct arb_state *)malloc(sizeof(*state));
    struct ref_out ref;
    float tex[8][4], texu[8][4], pos[4], psize, fog;
    char error[200];
    int ok = 1, s, k;
    float c[192][4];
    memcpy(c, env, sizeof(c));
    ref_run(prog, count, attr, c, env[192], env[193], env[194][0], env[194][1], &ref);
    if (!arb_run(arb, env, attr, state, error)) {
        fprintf(stderr, "%s: ARB run: %s\n", label, error);
        free(state);
        return 0;
    }
    arb_results(arb, state, tex, pos, &psize);
    unpack(layout, tex, texu, &fog);
    for (s = 0; s < 8; ++s) for (k = 0; k < 4; ++k)
        if (!close_enough(ref.tex[s][k], texu[s][k])) {
            fprintf(stderr, "%s: slot %d comp %d: reference %g ARB %g\n", label, s, k, ref.tex[s][k], texu[s][k]);
            ok = 0;
        }
    if (!close_enough(ref.fog, fog)) { fprintf(stderr, "%s: fog: reference %g ARB %g\n", label, ref.fog, fog); ok = 0; }
    if (!close_enough(ref.psize, psize)) { fprintf(stderr, "%s: point size: reference %g ARB %g\n", label, ref.psize, psize); ok = 0; }
    if (ref.finite) {
        for (k = 0; k < 4; ++k)
            if (!close_enough(ref.pos[k], pos[k])) {
                fprintf(stderr, "%s: position %d: reference %g ARB %g\n", label, k, ref.pos[k], pos[k]);
                ok = 0;
            }
    } else ++totals->position_skipped;
    ++totals->runs;
    free(state);
    return ok;
}

/* ---------- random programs */

static void random_environment(float env[256][4], float attr[16][4])
{
    int i, k;
    for (i = 0; i < 256; ++i) for (k = 0; k < 4; ++k) env[i][k] = random_value();
    for (i = 0; i < 16; ++i) for (k = 0; k < 4; ++k) attr[i][k] = random_value();
    /* a plausible viewport, sometimes with a zero scale component */
    env[192][0] = 320.0f; env[192][1] = -240.0f; env[192][2] = below(5) ? 16777215.0f : 0.0f; env[192][3] = 0.0f;
    if (!below(6)) env[192][0] = 0.0f;
    env[193][0] = 320.0f; env[193][1] = 240.0f; env[193][2] = 0.0f; env[193][3] = 0.0f;
    env[194][0] = below(3) ? 0.0f : 64.0f; env[194][1] = 1.0f + (float)below(8);
    /* the screen-space conversion's constants */
    env[58][0] = 320.0f; env[58][1] = -240.0f; env[58][2] = 16777215.0f; env[58][3] = 0.0f;
    env[59][0] = 320.0f; env[59][1] = 240.0f; env[59][2] = 0.0f; env[59][3] = 0.0f;
}

static void random_operand(unsigned long *p, unsigned which)
{
    unsigned long mux = 1 + below(3), index = mux == 1 ? (below(5) ? below(8) : 12) : below(12);
    unsigned long neg = below(4) == 0, sw[4];
    unsigned k;
    for (k = 0; k < 4; ++k) sw[k] = below(4);
    if (below(3) == 0) for (k = 1; k < 4; ++k) sw[k] = sw[0];
    if (which == 0) {
        set(p, 1, 8, 1, neg); set(p, 1, 6, 2, sw[0]); set(p, 1, 4, 2, sw[1]); set(p, 1, 2, 2, sw[2]); set(p, 1, 0, 2, sw[3]);
        set(p, 2, 28, 4, index); set(p, 2, 26, 2, mux);
    } else if (which == 1) {
        set(p, 2, 25, 1, neg); set(p, 2, 23, 2, sw[0]); set(p, 2, 21, 2, sw[1]); set(p, 2, 19, 2, sw[2]); set(p, 2, 17, 2, sw[3]);
        set(p, 2, 13, 4, index); set(p, 2, 11, 2, mux);
    } else {
        set(p, 2, 10, 1, neg); set(p, 2, 8, 2, sw[0]); set(p, 2, 6, 2, sw[1]); set(p, 2, 4, 2, sw[2]); set(p, 2, 2, 2, sw[3]);
        set(p, 2, 0, 2, index >> 2); set(p, 3, 30, 2, index & 3); set(p, 3, 28, 2, mux);
    }
}

static const unsigned long output_addresses[] = { 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

static void random_instruction(unsigned long *p, int force_mac, int force_ilu)
{
    unsigned which;
    memset(p, 0, 4 * sizeof(unsigned long));
    for (which = 0; which < 3; ++which) random_operand(p, which);
    set(p, 1, 9, 4, below(16));
    set(p, 1, 13, 8, below(192));
    set(p, 1, 21, 4, force_mac >= 0 ? (unsigned long)force_mac : (below(3) ? 1 + below(12) : below(14)));
    set(p, 1, 25, 3, force_ilu >= 0 ? (unsigned long)force_ilu : (below(2) ? 0 : below(8)));
    set(p, 3, 24, 4, below(3) ? below(16) : 0);
    set(p, 3, 20, 4, below(4) ? below(12) : 12);
    set(p, 3, 16, 4, below(3) ? below(16) : 0);
    if (below(3)) {
        set(p, 3, 12, 4, 1 + below(15));
        set(p, 3, 11, 1, 1);
        set(p, 3, 3, 8, output_addresses[below(11)]);
        set(p, 3, 2, 1, below(2));
    }
}

static unsigned long random_program(unsigned long *words, int force_mac, int force_ilu)
{
    unsigned long n = 1 + below(12), i;
    for (i = 0; i < n; ++i) random_instruction(words + i * 4, i == 0 ? force_mac : -1, i == 0 ? force_ilu : -1);
    /* write the screen-space conversion like the game: a clip capture sometimes */
    if (below(3) == 0 && n >= 2) {
        set(words + 4, 1, 25, 3, 3);
        set(words + 4, 3, 28, 2, 1);
        set(words + 4, 2, 0, 2, 3);
        set(words + 4, 3, 30, 2, 0);
    }
    set(words + (n - 1) * 4, 3, 0, 1, 1);
    /* the position is written by something in nearly every program */
    if (below(4)) {
        set(words + (n - 1) * 4, 3, 12, 4, 15);
        set(words + (n - 1) * 4, 3, 11, 1, 1);
        set(words + (n - 1) * 4, 3, 3, 8, 0);
    }
    return n;
}

/* ---------- tests */

static struct arb *new_arb(void) { return (struct arb *)calloc(1, sizeof(struct arb)); }

static void test_random(int force_mac, int force_ilu, unsigned programs, struct compare_totals *totals)
{
    unsigned p;
    for (p = 0; p < programs; ++p) {
        unsigned long words[12 * 4 + 4], n = random_program(words, force_mac, force_ilu);
        struct mac_nv2a_arb_layout layout;
        char *source = NULL, error[200];
        unsigned env_count = 0;
        int result = mac_nv2a_arb_translate(words, n, 0, &source, &env_count, &layout, error, sizeof(error));
        struct arb *arb = new_arb();
        int v;
        if (result == 0) {
            /* refused: acceptable only for what the translator names */
            CHECK(strstr(error, "no free varying") || strstr(error, "relative") || strstr(error, "unsupported") || strstr(error, "range"));
            if (!(strstr(error, "no free varying") || strstr(error, "relative") || strstr(error, "unsupported") || strstr(error, "range")))
                fprintf(stderr, "refusal: %s\n", error);
            ++totals->refused;
            free(arb);
            continue;
        }
        CHECK(result == 1);
        if (result != 1) { free(arb); continue; }
        ++totals->programs;
        if (layout.fog_texcoord >= 0) ++totals->fog_carried; else ++totals->fog_constant;
        if (strstr(source, "MOV clip, pos;")) ++totals->captured;
        CHECK(env_count == MAC_NV2A_ARB_ENV_COUNT && layout.env_count == env_count);
        if (!arb_parse(arb, source)) {
            fprintf(stderr, "random program %u (mac %d ilu %d): %s\n--- source\n%s", p, force_mac, force_ilu, arb->error, source);
            CHECK(0);
        } else {
            for (v = 0; v < 6; ++v) {
                float env[256][4], attr[16][4];
                char label[64];
                random_environment(env, attr);
                sprintf(label, "random %u mac %d ilu %d vertex %d", p, force_mac, force_ilu, v);
                if (!compare_run(label, words, n, arb, &layout, env, attr, totals)) {
                    unsigned long i;
                    for (i = 0; i < n; ++i)
                        fprintf(stderr, "  %08lX %08lX %08lX %08lX\n", words[i * 4] & 0xFFFFFFFFUL, words[i * 4 + 1] & 0xFFFFFFFFUL, words[i * 4 + 2] & 0xFFFFFFFFUL, words[i * 4 + 3] & 0xFFFFFFFFUL);
                    fprintf(stderr, "--- source\n%s", source);
                    CHECK(0);
                    break;
                }
                CHECK(1);
            }
        }
        free(arb); free(source);
    }
}

static void test_retail(unsigned index, struct compare_totals *totals)
{
    const unsigned long *code;
    unsigned long count;
    struct mac_nv2a_arb_layout layout;
    char *source = NULL, error[200], *refused = NULL;
    unsigned env_count = 0, native = 0;
    struct arb *arb = new_arb();
    struct mac_bone_gather_plan plan;
    unsigned v, lines = 0;
    const char *c;
    if (!retail_entry(index, &code, &count)) { printf("retail shader %u: table not found, skipped\n", index); free(arb); return; }
    CHECK(mac_nv2a_arb_translate(code, count, 0, &refused, &env_count, &layout, error, sizeof(error)) == 0 && refused == NULL);
    CHECK(strstr(error, "relative") != NULL);
    CHECK(mac_nv2a_arb_translate(code, count, 1, &source, &env_count, &layout, error, sizeof(error)) == 1);
    if (!source) { free(arb); return; }
    CHECK(env_count == MAC_NV2A_ARB_ENV_COUNT);
    CHECK(layout.bone_gather_used == 1);
    for (c = source; *c; ++c) if (*c == '\n') ++lines;
    printf("retail shader %u: %lu NV2A instructions -> %u ARB lines, fog texcoord %d.%c, %u env parameters\n",
        index, count, lines, layout.fog_texcoord, layout.fog_texcoord < 0 ? '-' : "xyzw"[layout.fog_component], env_count);
    CHECK(strstr(source, "!!ARBvp1.0\n") == source);
    CHECK(strstr(source, "program.local") == NULL);
    CHECK(strstr(source, "result.color") == NULL);
    CHECK(strstr(source, "vertex.attrib[10]") && strstr(source, "vertex.attrib[15]"));
    CHECK(strstr(source, "result.position") && strstr(source, "result.pointsize"));
    CHECK(strstr(source, "ADDRESS") == NULL);
    if (!arb_parse(arb, source)) { fprintf(stderr, "retail %u: %s\n", index, arb->error); CHECK(0); free(arb); free(source); return; }
    native = (unsigned)arb->nst;
    printf("retail shader %u: %u ARB instructions\n", index, native);
    plan.version = 1; plan.address_attribute = 5; plan.address_constant = 7;
    plan.first_gather_attribute = 10; plan.gather_attribute_count = 6; plan.row_base = 60; plan.row_limit = 191;
    for (v = 0; v < 40; ++v) {
        float env[256][4], attr[16][4], records[16][4], constants[192][4];
        char label[64];
        unsigned k, j;
        random_environment(env, attr);
        /* the address: bone indices as the game packs them, the scale c[7].w */
        env[7][3] = 3.0f;
        attr[5][0] = (float)below(20); attr[5][1] = (float)below(20);
        if (v % 7 == 0) { attr[5][0] = -2.0f; attr[5][1] = 70.0f; }
        memcpy(constants, env, sizeof(constants));
        memcpy(records, attr, sizeof(records));
        CHECK(mac_bone_gather_apply(&plan, &records[0][0], 1, 16, (const float (*)[4])constants, error, sizeof(error)) == 1);
        for (k = 10; k < 16; ++k) for (j = 0; j < 4; ++j) attr[k][j] = records[k][j];
        sprintf(label, "retail %u vertex %u", index, v);
        CHECK(compare_run(label, code, count, arb, &layout, env, attr, totals));
    }
    free(arb); free(source);
}

/* the program words of a retail shader, copied for mutation */
static unsigned long *copy_words(const unsigned long *code, unsigned long count)
{
    unsigned long *w = (unsigned long *)malloc((count * 4 + 4) * sizeof(unsigned long)), i;
    for (i = 0; i < count * 4; ++i) w[i] = code[i] & 0xFFFFFFFFUL;
    return w;
}

/* ---------- native relative addressing (MAC_NV2A_ARB_RELATIVE) */

static unsigned relative_reads(const unsigned long *code, unsigned long count, unsigned *arls)
{
    unsigned long i;
    unsigned reads = 0, k;
    *arls = 0;
    for (i = 0; i < count; ++i) {
        const unsigned long *p = code + i * 4;
        if (field(p, 1, 21, 4) == 13) ++*arls;
        if (field(p, 3, 1, 1))
            for (k = 0; k < 3; ++k) {
                unsigned long mux = k == 0 ? field(p, 2, 26, 2) : (k == 1 ? field(p, 2, 11, 2) : field(p, 3, 28, 2));
                if (mux == 3) ++reads;
            }
        if (field(p, 3, 0, 1)) break;
    }
    return reads;
}

/* c[7].w and attribute 5 as the bone-skinned programs expect them: bone
   indices times 3 pick rows 60 + 3 * index; with 20 bones the largest row
   read is 60 + 3 * 19 + 2 = 119 */
static void bone_environment(float env[256][4], float attr[16][4], unsigned v)
{
    random_environment(env, attr);
    env[7][3] = 3.0f;
    attr[5][0] = (float)below(20); attr[5][1] = (float)below(20);
    if (v % 5 == 0) { attr[5][0] = 19.0f; attr[5][1] = 19.0f; }
    if (v % 7 == 0) { attr[5][0] = 0.0f; attr[5][1] = 19.0f; }
}

static void test_relative_all(struct compare_totals *totals)
{
    unsigned index, translated = 0, with_reads = 0, compared = 0;
    for (index = 0; index < 67; ++index) {
        const unsigned long *code;
        unsigned long count;
        struct mac_nv2a_arb_layout layout;
        char *source = NULL, error[200];
        unsigned env_count = 0, reads, arls, v, native;
        struct arb *arb;
        int result;
        if (!retail_entry(index, &code, &count)) { printf("relative: table not found, skipped\n"); return; }
        reads = relative_reads(code, count, &arls);
        result = mac_nv2a_arb_translate(code, count, MAC_NV2A_ARB_RELATIVE, &source, &env_count, &layout, error, sizeof(error));
        if (result != 1) {
            printf("relative: shader %2u (%u relative reads, %u ARL): REFUSED: %s\n", index, reads, arls, error);
            CHECK(result == 0 && source == NULL);
            continue;
        }
        ++translated;
        CHECK(env_count == MAC_NV2A_ARB_ENV_COUNT && layout.bone_gather_used == 0);
        CHECK(layout.relative_used == (reads != 0));
        CHECK(strstr(source, "vertex.attrib[10]") == NULL || !reads);
        arb = new_arb();
        if (!arb_parse(arb, source)) {
            fprintf(stderr, "relative shader %u: %s\n--- source\n%s", index, arb->error, source);
            CHECK(0);
            free(arb); free(source);
            continue;
        }
        native = (unsigned)arb->nst;
        printf("relative: shader %2u: translated, %u relative reads, %u ARL, %u ARB instructions, relative_used %d\n",
            index, reads, arls, native, layout.relative_used);
        if (reads) {
            ++with_reads;
            CHECK(strstr(source, "ADDRESS A0;") && strstr(source, "ARL A0.x, t0.x;") && strstr(source, "cr[A0.x + "));
            CHECK(strstr(source, "ARL A0.x, kc.x;") != NULL);
            for (v = 0; v < 40; ++v) {
                float env[256][4], attr[16][4];
                char label[64];
                bone_environment(env, attr, v);
                sprintf(label, "relative %u vertex %u", index, v);
                if (compare_run(label, code, count, arb, &layout, env, attr, totals)) CHECK(1);
                else { CHECK(0); break; }
                ++compared;
            }
        } else {
            CHECK(strstr(source, "A0") == NULL);
        }
        free(arb); free(source);
    }
    printf("relative: %u of 67 shaders translate, %u have relative reads, %u vertex runs compared\n", translated, with_reads, compared);
}

/* shaders 31 and 50 in mode 2 against mode 1 and the reference, and the
   address clamp: a wild address never reads outside c[0..191] */
static void test_relative_edges(unsigned index)
{
    const unsigned long *code;
    unsigned long count, *w, i;
    struct mac_nv2a_arb_layout layout;
    char *source = NULL, error[200];
    struct arb *arb = new_arb();
    unsigned v;
    int result;
    if (!retail_entry(index, &code, &count)) { free(arb); return; }
    result = mac_nv2a_arb_translate(code, count, MAC_NV2A_ARB_RELATIVE, &source, NULL, &layout, error, sizeof(error));
    CHECK(result == 1);
    if (result != 1) { free(arb); return; }
    CHECK(strstr(source, "PARAM ka = { 0.001, 0.0, -60.0, 129.0 };") != NULL);
    CHECK(arb_parse(arb, source));
    for (v = 0; v < 24; ++v) {
        static const float wild[8] = { -1.0e6f, -1000.0f, -61.0f, -60.0f, 129.0f, 130.0f, 5000.0f, 3.0e5f };
        float env[256][4], attr[16][4];
        char out[200];
        struct arb_state *state = (struct arb_state *)malloc(sizeof(*state));
        random_environment(env, attr);
        env[7][3] = 1.0f;
        attr[5][0] = wild[v % 8]; attr[5][1] = wild[(v / 3) % 8];
        CHECK(arb_run(arb, env, attr, state, out));
        free(state);
    }
    free(arb); free(source);
    /* offsets above 63: the lowest is folded into the address */
    w = copy_words(code, count);
    for (i = 0; i < count; ++i) {
        unsigned long *p = w + i * 4;
        if (field(p, 3, 1, 1)) set(p, 1, 13, 8, field(p, 1, 13, 8) + 60); /* 120..122 */
    }
    arb = new_arb();
    result = mac_nv2a_arb_translate(w, count, MAC_NV2A_ARB_RELATIVE, &source, NULL, &layout, error, sizeof(error));
    CHECK(result == 1);
    if (result == 1) {
        CHECK(strstr(source, "PARAM ka = { 0.001, 120.0, 0.0, 189.0 };") != NULL);
        CHECK(strstr(source, "cr[A0.x + 0]") && strstr(source, "cr[A0.x + 2]"));
        CHECK(arb_parse(arb, source));
        for (v = 0; v < 24; ++v) {
            float env[256][4], attr[16][4], ref_env[256][4];
            char label[64];
            struct compare_totals t;
            memset(&t, 0, sizeof(t));
            bone_environment(env, attr, v);
            /* the rows are 120 + 3 * index: up to 179, in range */
            memcpy(ref_env, env, sizeof(ref_env));
            sprintf(label, "folded %u vertex %u", index, v);
            CHECK(compare_run(label, w, count, arb, &layout, ref_env, attr, &t));
        }
        free(source);
    }
    free(arb);
    /* offsets 64 or more apart are refused; so is an offset beyond c[191] */
    for (i = 0; i < count; ++i) {
        unsigned long *p = w + i * 4;
        if (field(p, 3, 1, 1)) { set(p, 1, 13, 8, 0); break; }
    }
    result = mac_nv2a_arb_translate(w, count, MAC_NV2A_ARB_RELATIVE, &source, NULL, NULL, error, sizeof(error));
    CHECK(result == 0 && source == NULL && strstr(error, "relative") != NULL);
    free(w);
}

static void test_gather_refusals(unsigned index)
{
    const unsigned long *code;
    unsigned long count, i, relative_read = 0, arl_a = 0, arl_b = 0, product = 0;
    unsigned reads = 0, arls = 0;
    unsigned long *w;
    char *source = NULL, error[200];
    if (!retail_entry(index, &code, &count)) return;
    w = copy_words(code, count);
    for (i = 0; i < count; ++i) {
        unsigned long *p = w + i * 4;
        if (field(p, 3, 1, 1)) { if (reads == 1) relative_read = i; ++reads; }
        if (field(p, 1, 21, 4) == 13) { if (arls == 0) arl_a = i; else arl_b = i; ++arls; }
        if (field(p, 1, 21, 4) == 2 && field(p, 1, 9, 4) == 5 && field(p, 1, 13, 8) == 7) product = i;
    }
    CHECK(arls == 2 && reads >= 6 && product < arl_a);
    /* the pristine program is accepted */
    CHECK(mac_nv2a_arb_translate(w, count, 1, &source, NULL, NULL, error, sizeof(error)) == 1);
    free(source); source = NULL;
    /* a relative read at another offset */
    set(w + relative_read * 4, 1, 13, 8, field(w + relative_read * 4, 1, 13, 8) + 1);
    CHECK(mac_nv2a_arb_translate(w, count, 1, &source, NULL, NULL, error, sizeof(error)) == 0 && !source);
    set(w + relative_read * 4, 1, 13, 8, field(w + relative_read * 4, 1, 13, 8) - 1);
    /* the first address load is no longer an address load */
    set(w + arl_a * 4, 1, 21, 4, 1);
    CHECK(mac_nv2a_arb_translate(w, count, 1, &source, NULL, NULL, error, sizeof(error)) == 0 && !source);
    set(w + arl_a * 4, 1, 21, 4, 13);
    /* the second loads from the wrong component */
    {
        unsigned long saved = w[arl_b * 4 + 1];
        set(w + arl_b * 4, 1, 6, 2, 0); set(w + arl_b * 4, 1, 4, 2, 0); set(w + arl_b * 4, 1, 2, 2, 0); set(w + arl_b * 4, 1, 0, 2, 0);
        CHECK(mac_nv2a_arb_translate(w, count, 1, &source, NULL, NULL, error, sizeof(error)) == 0 && !source);
        w[arl_b * 4 + 1] = saved;
    }
    /* something between the product and the second load overwrites r.x */
    for (i = product + 1; i < arl_b; ++i) {
        unsigned long *p = w + i * 4, saved1 = p[1], saved3 = p[3];
        if (i == arl_a) continue;
        set(p, 1, 21, 4, 1);
        set(p, 3, 24, 4, 8);
        set(p, 3, 20, 4, field(w + product * 4, 3, 20, 4));
        CHECK(mac_nv2a_arb_translate(w, count, 1, &source, NULL, NULL, error, sizeof(error)) == 0 && !source);
        p[1] = saved1; p[3] = saved3;
        break;
    }
    /* an ordinary read of attribute 12 */
    for (i = 0; i < count; ++i) {
        unsigned long *p = w + i * 4;
        if (field(p, 2, 26, 2) == 2 && i > arl_b) {
            unsigned long saved = p[1];
            set(p, 1, 9, 4, 12);
            CHECK(mac_nv2a_arb_translate(w, count, 1, &source, NULL, NULL, error, sizeof(error)) == 0 && !source);
            p[1] = saved;
            break;
        }
    }
    /* a program with relative reads but without the gather pattern */
    set(w + product * 4, 1, 21, 4, 1);
    CHECK(mac_nv2a_arb_translate(w, count, 1, &source, NULL, NULL, error, sizeof(error)) == 0 && !source);
    free(w);
}

static void test_refusals(void)
{
    unsigned long w[8];
    char *source = NULL, error[200];
    int result;
    /* a temporary register past r12 read by a used operand */
    memset(w, 0, sizeof(w));
    set(w, 1, 21, 4, 1);               /* MOV */
    set(w, 2, 26, 2, 1);               /* A is a temporary */
    set(w, 2, 28, 4, 14);              /* r14 */
    set(w, 3, 24, 4, 15); set(w, 3, 20, 4, 0); set(w, 3, 0, 1, 1);
    result = mac_nv2a_arb_translate(w, 1, 0, &source, NULL, NULL, error, sizeof(error));
    CHECK(result == 0 && source == NULL && strstr(error, "range"));
    /* a constant register past c[191] */
    memset(w, 0, sizeof(w));
    set(w, 1, 21, 4, 1); set(w, 2, 26, 2, 3); set(w, 1, 13, 8, 200);
    set(w, 3, 24, 4, 15); set(w, 3, 0, 1, 1);
    result = mac_nv2a_arb_translate(w, 1, 0, &source, NULL, NULL, error, sizeof(error));
    CHECK(result == 0 && source == NULL && strstr(error, "range"));
    /* an unknown MAC operation */
    memset(w, 0, sizeof(w));
    set(w, 1, 21, 4, 14); set(w, 3, 0, 1, 1);
    result = mac_nv2a_arb_translate(w, 1, 0, &source, NULL, NULL, error, sizeof(error));
    CHECK(result == 0 && source == NULL && strstr(error, "unsupported"));
    /* a relative read without the gather */
    memset(w, 0, sizeof(w));
    set(w, 1, 21, 4, 1); set(w, 2, 26, 2, 3); set(w, 1, 13, 8, 60); set(w, 3, 1, 1, 1);
    set(w, 3, 24, 4, 15); set(w, 3, 0, 1, 1);
    result = mac_nv2a_arb_translate(w, 1, 0, &source, NULL, NULL, error, sizeof(error));
    CHECK(result == 0 && source == NULL && strstr(error, "relative"));
    result = mac_nv2a_arb_translate(w, 1, 1, &source, NULL, NULL, error, sizeof(error));
    CHECK(result == 0 && source == NULL);
    /* bad arguments */
    CHECK(mac_nv2a_arb_translate(NULL, 1, 0, &source, NULL, NULL, error, sizeof(error)) == -1);
    CHECK(mac_nv2a_arb_translate(w, 0, 0, &source, NULL, NULL, error, sizeof(error)) == -1);
    /* more instructions than the NV2A holds */
    {
        unsigned long *big = (unsigned long *)calloc(200 * 4, sizeof(unsigned long));
        result = mac_nv2a_arb_translate(big, 200, 0, &source, NULL, NULL, error, sizeof(error));
        if (result != 0 || source) fprintf(stderr, "big program: %d %s\n", result, error);
        CHECK(result == 0 && source == NULL);
        free(big);
    }
}

static void test_fragment_side(void)
{
    struct mac_nv2a_arb_layout none, steal;
    char prelude[1024], *out = NULL, error[160];
    static const char fragment[] =
        "#version 120\nvarying vec4 xD0;\nvarying vec4 xD1;\nvarying vec4 xB0;\nvarying vec4 xB1;\n"
        "varying vec4 xT0;\nvarying vec4 xT1;\nvarying vec4 xT2;\nvarying vec4 xT3;\nvarying float xFog;\n"
        "void main() { gl_FragColor = xD0 + xT0 + vec4(xFog) + xB1; }\n";
    static const char lighting[] =
        "#version 120\nvarying vec4 xD0;\nvarying vec4 xWorldNormal;\nvoid main() { gl_FragColor = xD0 + xWorldNormal; }\n";
    memset(&none, 0, sizeof(none)); none.fog_texcoord = -1;
    memset(&steal, 0, sizeof(steal)); steal.fog_texcoord = 1; steal.fog_component = 3;
    CHECK(mac_nv2a_arb_fragment_prelude(&none, prelude, sizeof(prelude)));
    CHECK(strstr(prelude, "#define xD0 gl_TexCoord[6]") && strstr(prelude, "#define xB0 gl_TexCoord[0]") &&
        strstr(prelude, "#define xT3 gl_TexCoord[5]") && strstr(prelude, "#define xFog 1.0"));
    CHECK(mac_nv2a_arb_fragment_prelude(&steal, prelude, sizeof(prelude)));
    CHECK(strstr(prelude, "#define xB1 (vec4(gl_TexCoord[1].x, gl_TexCoord[1].y, gl_TexCoord[1].z, 1.0))") != NULL);
    CHECK(strstr(prelude, "#define xFog (gl_TexCoord[1].w)") != NULL);
    CHECK(!mac_nv2a_arb_fragment_prelude(&steal, prelude, 20));
    CHECK(mac_nv2a_arb_fragment_rewrite(fragment, &steal, &out, error, sizeof(error)) == 1);
    CHECK(out && strstr(out, "varying") == NULL && strstr(out, "#version 120\n#define") == out &&
        strstr(out, "void main() { gl_FragColor = xD0") != NULL);
    free(out); out = NULL;
    CHECK(mac_nv2a_arb_fragment_rewrite(lighting, &none, &out, error, sizeof(error)) == 0 && out == NULL);
    CHECK(strstr(error, "varying") != NULL);
    if (steal.fog_texcoord == 1) printf("prelude with fog in texcoord 1.w:\n%s", prelude);
}

int main(int argc, char **argv)
{
    struct compare_totals totals;
    unsigned long m;
    int i;
    memset(&totals, 0, sizeof(totals));
    if (argc == 2) seed_state = (unsigned long)atol(argv[1]);
    if (argc >= 3 && !strcmp(argv[1], "--dump")) {
        const unsigned long *code;
        unsigned long count;
        char *source = NULL, error[200];
        struct mac_nv2a_arb_layout layout;
        if (!retail_entry((unsigned)atoi(argv[2]), &code, &count)) return 2;
        if (mac_nv2a_arb_translate(code, count, argc > 3 ? atoi(argv[3]) : 1, &source, NULL, &layout, error, sizeof(error)) != 1) { fprintf(stderr, "%s\n", error); return 1; }
        fputs(source, stdout);
        free(source);
        return 0;
    }
    test_retail(31, &totals);
    test_retail(50, &totals);
    test_gather_refusals(31);
    test_relative_all(&totals);
    test_relative_edges(31);
    test_relative_edges(50);
    test_gather_refusals(50);
    test_refusals();
    test_fragment_side();
    for (m = 0; m <= 13; ++m) for (i = 0; i <= 7; ++i) test_random((int)m, i, 6, &totals);
    test_random(-1, -1, 400, &totals);
    printf("random programs: %u translated (%u with oFog carried in a texcoord component, %u with constant fog, %u with a clip capture), %u refused\n",
        totals.programs, totals.fog_carried, totals.fog_constant, totals.captured, totals.refused);
    printf("compared %u vertex runs (%u with a non-finite position not compared)\n", totals.runs, totals.position_skipped);
    printf("NV2A to ARB probe: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
