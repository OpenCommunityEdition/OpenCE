/* NV2A pixel combiner state -> ARB_fragment_program: see mac_nv2a_arbfp.h and
   docs/ppc-mac-arbfp-translator.md.

   The reference is port/linux/src/nv2a_psh.c (texture_stage, combiner_stage,
   combiner_input, output_mapping, the final combiner, the alpha test, fog).
   This file does not call it; it computes the same values, folding what the
   state fixes:

   - operands are tracked as "views" of a stored value (a register, a
     program.env parameter, a varying, a temporary) with a sign, a component
     and a RANGE (lo, hi) of the values it can hold. The ranges start from
     what the inputs are: colors and texture samples lie in [0,1], and the
     combiner clamps every output to [-1,1];
   - a clamp is not emitted when the range makes it a no-op, and a combiner
     output that may leave [-1,1] is stored unclamped with the clamp PENDING
     (the view's real range is the clamped one, its stored range, rlo..rhi,
     the unclamped). Most consumers wanted a one-sided clamp of it anyway:
     max(clamp(x,-1,1),0) is sat(x), 1-sat(clamp(x,-1,1)) is sat(1-x), which
     are single saturating instructions, and the full clamp is emitted only
     for a signed consumer;
   - constants (the zero register, mappings of it, a product by 0 or 1) are
     folded, and outputs that no destination receives are not computed;
   - all values are single-assignment virtual registers; temporaries are
     assigned from their live ranges, a vector (xyz) and a scalar (w) value
     sharing one ARB temporary. */
/* (the engine prefix maps these to the game's debug versions) */
#undef memset
#undef memcpy
#undef memcmp
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef strlen
#undef strncpy
#undef strcat
#undef strcpy
#include "mac_nv2a_arbfp.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAC_NV2A_ARBFP_STANDALONE
#include "mac_nv2a_draw_api.h"
#endif

/* ---------- the NV2A's codes (the values of port/linux/src/nv2a_psh.c) */

enum {
    REG_ZERO = 0, REG_C0 = 1, REG_C1 = 2, REG_FOG = 3, REG_V0 = 4, REG_V1 = 5,
    REG_T0 = 8, REG_T1 = 9, REG_T2 = 10, REG_T3 = 11, REG_R0 = 12, REG_R1 = 13,
    REG_V1R0_SUM = 14, REG_EF_PRODUCT = 15
};
enum {
    MODE_NONE = 0, MODE_PROJECT2D = 1, MODE_PROJECT3D = 2, MODE_CUBEMAP = 3, MODE_PASSTHRU = 4,
    MODE_CLIPPLANE = 5, MODE_BUMPENVMAP = 6, MODE_BUMPENVMAP_LUMINANCE = 7, MODE_BRDF = 8,
    MODE_DOT_ST = 9, MODE_DOT_ZW = 10, MODE_DOT_REFLECT_DIFFUSE = 11, MODE_DOT_REFLECT_SPECULAR = 12,
    MODE_DOT_STR_3D = 13, MODE_DOT_STR_CUBE = 14, MODE_DEPENDENT_AR = 15, MODE_DEPENDENT_GB = 16,
    MODE_DOT_PRODUCT = 17, MODE_DOT_REFLECT_SPECULAR_CONSTANT = 18
};
enum { SAMPLER_NONE = 0, SAMPLER_2D = 1, SAMPLER_3D = 2, SAMPLER_CUBE = 3 };
/* the D3DCMPFUNC values */
enum {
    CMP_NEVER = 512, CMP_LESS = 513, CMP_EQUAL = 514, CMP_LESSEQUAL = 515, CMP_GREATER = 516,
    CMP_NOTEQUAL = 517, CMP_GREATEREQUAL = 518, CMP_ALWAYS = 519
};
enum { FOG_NONE = 0, FOG_EXP = 1, FOG_EXP2 = 2, FOG_LINEAR = 3 };

/* mapping codes beyond the combiner input mappings (0x00..0xe0) */
#define MAP_SAT 0x100   /* clamp(x, 0, 1)         */
#define MAP_REAL 0x200  /* the stored value as it is, clamp applied */

/* ---------- values, operands, instructions */

enum { V_CONST, V_ATTR, V_ENV, V_TEMP };
enum {
    OP_MOV, OP_ADD, OP_MUL, OP_MAD, OP_MAX, OP_MIN, OP_DP3, OP_LRP, OP_CMP, OP_SGE, OP_FLR,
    OP_FRC, OP_RCP, OP_EX2, OP_ABS, OP_TEX, OP_TXP, OP_KIL, OP_COMMENT
};
static const char *const op_names[] = {
    "MOV", "ADD", "MUL", "MAD", "MAX", "MIN", "DP3", "LRP", "CMP", "SGE", "FLR",
    "FRC", "RCP", "EX2", "ABS", "TEX", "TXP", "KIL", ""
};
#define DST_NONE (-1)
#define DST_COLOR (-2)

struct val {
    int kind;
    int index;      /* V_ATTR: the texcoord slot; V_ENV: the parameter */
    float c;        /* V_CONST */
    int lanes;      /* V_TEMP: bit mask of the components it occupies */
};

/* A view of a stored value. lo..hi is the range of the value the program
   means (after the clamp that is pending); rlo..rhi that of what is stored.
   The scalar kind of view has comp 0..3; comp -1 is the vector xyz. */
struct opnd {
    int val, comp, neg;
    float lo, hi, rlo, rhi;
    const char *sw;     /* explicit swizzle (scratch sources); NULL: from comp */
};

struct src {
    int val, comp, neg;
    const char *sw;
};

struct ins {
    int op, sat, dst;
    unsigned mask;
    int nsrc;
    struct src s[3];
    int tex_unit, tex_target;
    int deleted;
    int comment, comment_number;
};

#define MAX_VALS 900
#define MAX_INS 1400
#define MAX_MEMO 400
#define MAX_TEMPS 128

struct memo {
    struct opnd key, result;
    int mapping;
};

struct regs { struct opnd rgb, a; };

struct ctx {
    const struct mac_nv2a_arbfp_state *s;
    const struct mac_nv2a_arb_layout *layout;
    unsigned long projective;
    struct val vals[MAX_VALS];
    int nvals;
    struct ins ins[MAX_INS];
    int nins;
    struct memo memo[MAX_MEMO];
    int nmemo;
    struct regs r[16];
    int fog_ready;
    struct opnd fog_a;
    int dot_val;
    int clean[8];       /* per texcoord slot: the copy with the fog component restored, or -1 */
    int failed;
    char message[160];
    char swpool[32][5];
    int nsw;
};

static float fmin2(float a, float b) { return a < b ? a : b; }
static float fmax2(float a, float b) { return a > b ? a : b; }
static float clampf(float v, float l, float h) { return v < l ? l : (v > h ? h : v); }

static void fail(struct ctx *x, const char *message)
{
    if (!x->failed) {
        x->failed = 1;
        strncpy(x->message, message, sizeof(x->message) - 1);
        x->message[sizeof(x->message) - 1] = 0;
    }
}

static int new_val(struct ctx *x, int kind, int index, float c, int lanes)
{
    int v;
    if (x->nvals >= MAX_VALS) { fail(x, "pixel shader needs too many values"); return 0; }
    v = x->nvals++;
    x->vals[v].kind = kind; x->vals[v].index = index; x->vals[v].c = c; x->vals[v].lanes = lanes;
    return v;
}

static struct opnd o_make(int val, int comp, int neg, float lo, float hi)
{
    struct opnd o;
    o.val = val; o.comp = comp; o.neg = neg;
    o.lo = o.rlo = lo; o.hi = o.rhi = hi; o.sw = NULL;
    return o;
}

static int pending(const struct opnd *o) { return o->lo != o->rlo || o->hi != o->rhi; }
static int is_const(const struct ctx *x, const struct opnd *o) { return x->vals[o->val].kind == V_CONST; }
static float cval(const struct ctx *x, const struct opnd *o) { return x->vals[o->val].c; }

static struct opnd o_const(struct ctx *x, float c)
{
    int i;
    if (c == 0.0f) c = 0.0f; /* (no negative zero) */
    for (i = 0; i < x->nvals; i++)
        if (x->vals[i].kind == V_CONST && x->vals[i].c == c) return o_make(i, -1, 0, c, c);
    i = new_val(x, V_CONST, 0, c, 0);
    return o_make(i, -1, 0, c, c);
}

static int attr_val(struct ctx *x, int slot)
{
    int i;
    for (i = 0; i < x->nvals; i++)
        if (x->vals[i].kind == V_ATTR && x->vals[i].index == slot) return i;
    return new_val(x, V_ATTR, slot, 0.0f, 0);
}

static int env_val(struct ctx *x, int index)
{
    int i;
    for (i = 0; i < x->nvals; i++)
        if (x->vals[i].kind == V_ENV && x->vals[i].index == index) return i;
    return new_val(x, V_ENV, index, 0.0f, 0);
}

static struct opnd o_attr(struct ctx *x, int slot, int comp, float lo, float hi)
{ return o_make(attr_val(x, slot), comp, 0, lo, hi); }
static struct opnd o_env(struct ctx *x, int index, int comp, float lo, float hi)
{ return o_make(env_val(x, index), comp, 0, lo, hi); }
static int emit(struct ctx *x, int op, int sat, int dst, unsigned mask,
    const struct opnd *a, const struct opnd *b, const struct opnd *c);

/* A texcoord slot as the pixel shader sees it. The component that carries oFog (the layout) is not the
   vertex program's output for that slot: nv2a_psh.c's prelude (mac_nv2a_arb_fragment_prelude) reads it as
   the default of an unwritten component, (0,0,0,1)[c], and so does this. `mask` is the components the
   reader uses (bit n = component n); a reader of one component (comp >= 0) gets the constant, a vector
   reader a copy with the component put back (made once). */
static struct opnd o_slot(struct ctx *x, int slot, int comp, unsigned mask, float lo, float hi)
{
    static const float defaults[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    int fog_slot = x->layout ? x->layout->fog_texcoord : -1, fc;
    if (fog_slot != slot) return o_attr(x, slot, comp, lo, hi);
    fc = x->layout->fog_component & 3;
    if (comp >= 0) return comp == fc ? o_const(x, defaults[fc]) : o_attr(x, slot, comp, lo, hi);
    if (!(mask & (1u << fc))) return o_attr(x, slot, -1, lo, hi);
    if (x->clean[slot] < 0) {
        struct opnd whole = o_attr(x, slot, -1, lo, hi), d = o_const(x, defaults[fc]);
        int c = new_val(x, V_TEMP, 0, 0.0f, 15);
        whole.sw = "";
        emit(x, OP_MOV, 0, c, 15u, &whole, NULL, NULL);
        emit(x, OP_MOV, 0, c, 1u << fc, &d, NULL, NULL);
        x->clean[slot] = c;
    }
    {
        struct opnd o = o_make(x->clean[slot], -1, 0, lo, hi);
        return o;
    }
}

/* a scratch value's component; its range is not tracked */
static struct opnd o_raw(int val, int comp) { return o_make(val, comp, 0, -1.0e30f, 1.0e30f); }
static struct opnd o_sw(struct opnd o, const char *sw) { o.sw = sw; return o; }

static struct opnd o_neg(struct ctx *x, struct opnd o)
{
    float t;
    if (is_const(x, &o)) return o_const(x, -cval(x, &o));
    o.neg = !o.neg;
    t = o.lo; o.lo = -o.hi; o.hi = -t;
    t = o.rlo; o.rlo = -o.rhi; o.rhi = -t;
    return o;
}

static struct opnd new_temp(struct ctx *x, int vec, float lo, float hi)
{
    int v = new_val(x, V_TEMP, 0, 0.0f, vec ? 7 : 8);
    return o_make(v, vec ? -1 : 3, 0, lo, hi);
}

static const char *make_sw(struct ctx *x, char a, char b, char c, char d)
{
    char *p;
    if (x->nsw >= 32) { fail(x, "too many swizzles"); return "xyzw"; }
    p = x->swpool[x->nsw++];
    p[0] = a; p[1] = b; p[2] = c; p[3] = d; p[4] = 0;
    return p;
}

/* ---------- instruction emission */

static void src_of(struct src *s, const struct opnd *o)
{
    s->val = o->val; s->comp = o->comp; s->neg = o->neg; s->sw = o->sw;
}

static int emit(struct ctx *x, int op, int sat, int dst, unsigned mask,
    const struct opnd *a, const struct opnd *b, const struct opnd *c)
{
    struct ins *i;
    if (x->nins >= MAX_INS) { fail(x, "pixel shader needs too many instructions"); return 0; }
    i = &x->ins[x->nins];
    memset(i, 0, sizeof(*i));
    i->op = op; i->sat = sat; i->dst = dst; i->mask = mask;
    if (a) { src_of(&i->s[0], a); i->nsrc = 1; }
    if (b) { src_of(&i->s[1], b); i->nsrc = 2; }
    if (c) { src_of(&i->s[2], c); i->nsrc = 3; }
    return x->nins++;
}

static void comment(struct ctx *x, int kind, int number)
{
    int k = emit(x, OP_COMMENT, 0, DST_NONE, 0, NULL, NULL, NULL);
    x->ins[k].comment = kind; x->ins[k].comment_number = number;
}

/* ---------- operand arithmetic with folding. All take real (not pending)
   operands; vec: the result is a vector (xyz) rather than a scalar (w). */

static struct opnd fin(struct ctx *x, struct opnd o, int sat);

static struct opnd range_result(struct ctx *x, int vec, float lo, float hi, int sat)
{
    struct opnd r;
    if (sat) { lo = clampf(lo, 0.0f, 1.0f); hi = clampf(hi, 0.0f, 1.0f); }
    r = new_temp(x, vec, lo, hi);
    return r;
}

static void product_range(const struct opnd *a, const struct opnd *b, float *lo, float *hi)
{
    float p0 = a->lo * b->lo, p1 = a->lo * b->hi, p2 = a->hi * b->lo, p3 = a->hi * b->hi;
    *lo = fmin2(fmin2(p0, p1), fmin2(p2, p3));
    *hi = fmax2(fmax2(p0, p1), fmax2(p2, p3));
}

static struct opnd mk_mul(struct ctx *x, struct opnd a, struct opnd b, int vec, int sat)
{
    struct opnd r;
    float lo, hi;
    if (is_const(x, &a) && is_const(x, &b)) {
        float c = cval(x, &a) * cval(x, &b);
        return o_const(x, sat ? clampf(c, 0.0f, 1.0f) : c);
    }
    if (is_const(x, &a)) { r = a; a = b; b = r; }
    if (is_const(x, &b)) {
        float k = cval(x, &b);
        if (k == 0.0f) return o_const(x, 0.0f);
        if (k == 1.0f) return fin(x, a, sat);
        if (k == -1.0f) return fin(x, o_neg(x, a), sat);
    }
    product_range(&a, &b, &lo, &hi);
    r = range_result(x, vec, lo, hi, sat);
    emit(x, OP_MUL, sat, r.val, vec ? 7u : 8u, &a, &b, NULL);
    return r;
}

static struct opnd mk_add(struct ctx *x, struct opnd a, struct opnd b, int vec, int sat)
{
    struct opnd r;
    float lo, hi;
    if (is_const(x, &a) && is_const(x, &b)) {
        float c = cval(x, &a) + cval(x, &b);
        return o_const(x, sat ? clampf(c, 0.0f, 1.0f) : c);
    }
    if (is_const(x, &a) && cval(x, &a) == 0.0f) return fin(x, b, sat);
    if (is_const(x, &b) && cval(x, &b) == 0.0f) return fin(x, a, sat);
    lo = a.lo + b.lo; hi = a.hi + b.hi;
    r = range_result(x, vec, lo, hi, sat);
    emit(x, OP_ADD, sat, r.val, vec ? 7u : 8u, &a, &b, NULL);
    return r;
}

static struct opnd mk_mad(struct ctx *x, struct opnd a, struct opnd b, struct opnd c, int vec, int sat)
{
    struct opnd r;
    float lo, hi, plo, phi;
    if (is_const(x, &a) && is_const(x, &b))
        return mk_add(x, o_const(x, cval(x, &a) * cval(x, &b)), c, vec, sat);
    if (is_const(x, &a)) { r = a; a = b; b = r; }
    if (is_const(x, &b)) {
        float k = cval(x, &b);
        if (k == 0.0f) return fin(x, c, sat);
        if (k == 1.0f) return mk_add(x, a, c, vec, sat);
        if (k == -1.0f) return mk_add(x, o_neg(x, a), c, vec, sat);
    }
    if (is_const(x, &c) && cval(x, &c) == 0.0f) return mk_mul(x, a, b, vec, sat);
    product_range(&a, &b, &plo, &phi);
    lo = plo + c.lo; hi = phi + c.hi;
    r = range_result(x, vec, lo, hi, sat);
    emit(x, OP_MAD, sat, r.val, vec ? 7u : 8u, &a, &b, &c);
    return r;
}

static struct opnd mk_dp3(struct ctx *x, struct opnd a, struct opnd b)
{
    struct opnd r;
    float lo, hi;
    if ((is_const(x, &a) && cval(x, &a) == 0.0f) || (is_const(x, &b) && cval(x, &b) == 0.0f))
        return o_const(x, 0.0f);
    if (is_const(x, &a) && is_const(x, &b)) return o_const(x, 3.0f * cval(x, &a) * cval(x, &b));
    product_range(&a, &b, &lo, &hi);
    r = range_result(x, 1, 3.0f * lo, 3.0f * hi, 0);
    emit(x, OP_DP3, 0, r.val, 7u, &a, &b, NULL);
    return r;
}

/* a * b + (1 - a) * c, then optionally saturated */
static int same_value(const struct ctx *x, const struct opnd *a, const struct opnd *b);
static struct opnd mk_lrp(struct ctx *x, struct opnd a, struct opnd b, struct opnd c, int vec, int sat)
{
    struct opnd r;
    float lo, hi;
    if (is_const(x, &a)) {
        float k = cval(x, &a);
        if (k == 0.0f) return fin(x, c, sat);
        if (k == 1.0f) return fin(x, b, sat);
        return mk_mad(x, b, o_const(x, k), mk_mul(x, c, o_const(x, 1.0f - k), vec, 0), vec, sat);
    }
    if (same_value(x, &b, &c)) return fin(x, b, sat);
    lo = fmin2(b.lo, c.lo); hi = fmax2(b.hi, c.hi);
    if (a.lo < 0.0f || a.hi > 1.0f) { lo = -1.0e30f; hi = 1.0e30f; }
    r = range_result(x, vec, lo, hi, sat);
    emit(x, OP_LRP, sat, r.val, vec ? 7u : 8u, &a, &b, &c);
    return r;
}

/* the value itself, saturated if sat */
static struct opnd fin(struct ctx *x, struct opnd o, int sat)
{
    struct opnd r;
    int vec;
    if (!sat) return o;
    if (is_const(x, &o)) return o_const(x, clampf(cval(x, &o), 0.0f, 1.0f));
    if (!pending(&o) && o.lo >= 0.0f && o.hi <= 1.0f) return o;
    vec = o.comp < 0;
    r = new_temp(x, vec, clampf(o.lo, 0.0f, 1.0f), clampf(o.hi, 0.0f, 1.0f));
    emit(x, OP_MOV, 1, r.val, vec ? 7u : 8u, &o, NULL, NULL);
    return r;
}

/* a one-operand instruction into a new temporary, with the given range */
static struct opnd mk_unary(struct ctx *x, int op, struct opnd a, int vec, float lo, float hi)
{
    struct opnd r = new_temp(x, vec, lo, hi);
    emit(x, op, 0, r.val, vec ? 7u : 8u, &a, NULL, NULL);
    return r;
}

/* ---------- reading a register as a combiner input */

static int same_view(const struct opnd *a, const struct opnd *b)
{
    return a->val == b->val && a->comp == b->comp && a->neg == b->neg && a->lo == b->lo &&
        a->hi == b->hi && a->rlo == b->rlo && a->rhi == b->rhi;
}

static int same_value(const struct ctx *x, const struct opnd *a, const struct opnd *b)
{
    if (is_const(x, a) && is_const(x, b)) return cval(x, a) == cval(x, b);
    return a->val == b->val && a->comp == b->comp && a->neg == b->neg && a->sw == b->sw &&
        x->vals[a->val].kind != V_CONST && x->vals[b->val].kind != V_CONST;
}

static struct opnd map_operand(struct ctx *x, struct opnd o, int mapping);

/* clamp(x, 0, 1) of the stored value (the pending clamp changes nothing) */
static struct opnd op_sat(struct ctx *x, struct opnd o)
{
    struct opnd r;
    int vec;
    if (is_const(x, &o)) return o_const(x, clampf(cval(x, &o), 0.0f, 1.0f));
    if (o.hi <= 0.0f) return o_const(x, 0.0f);
    if (o.lo >= 1.0f) return o_const(x, 1.0f);
    if (!pending(&o) && o.lo >= 0.0f && o.hi <= 1.0f) return o;
    vec = o.comp < 0;
    r = new_temp(x, vec, clampf(o.lo, 0.0f, 1.0f), clampf(o.hi, 0.0f, 1.0f));
    emit(x, OP_MOV, 1, r.val, vec ? 7u : 8u, &o, NULL, NULL);
    return r;
}

/* clamp(x, -1, 1): the pending clamp applied */
static struct opnd op_real(struct ctx *x, struct opnd o)
{
    struct opnd r, one, minus_one;
    int vec;
    if (is_const(x, &o) || !pending(&o)) return o;
    vec = o.comp < 0;
    r = new_temp(x, vec, o.lo, o.hi);
    one = o_const(x, 1.0f); minus_one = o_const(x, -1.0f);
    if (o.rlo >= 0.0f) {
        emit(x, OP_MOV, 1, r.val, vec ? 7u : 8u, &o, NULL, NULL);
    } else if (o.rlo >= -1.0f) {
        emit(x, OP_MIN, 0, r.val, vec ? 7u : 8u, &o, &one, NULL);
    } else if (o.rhi <= 1.0f) {
        emit(x, OP_MAX, 0, r.val, vec ? 7u : 8u, &o, &minus_one, NULL);
    } else {
        struct opnd t = r;
        emit(x, OP_MAX, 0, r.val, vec ? 7u : 8u, &o, &minus_one, NULL);
        emit(x, OP_MIN, 0, r.val, vec ? 7u : 8u, &t, &one, NULL);
    }
    r.rlo = r.lo; r.rhi = r.hi;
    return r;
}

/* max(x, 0) of the value the program means (x = clamp(stored, -1, 1)) */
static struct opnd op_nonneg(struct ctx *x, struct opnd o)
{
    struct opnd r, zero;
    int vec;
    if (is_const(x, &o)) return o_const(x, fmax2(cval(x, &o), 0.0f));
    if (pending(&o)) return op_sat(x, o);  /* max(clamp(s,-1,1),0) = sat(s) */
    if (o.lo >= 0.0f) return o;
    if (o.hi <= 0.0f) return o_const(x, 0.0f);
    vec = o.comp < 0;
    zero = o_const(x, 0.0f);
    r = new_temp(x, vec, 0.0f, o.hi);
    emit(x, OP_MAX, 0, r.val, vec ? 7u : 8u, &o, &zero, NULL);
    return r;
}

/* 1 - clamp(x, 0, 1) */
static struct opnd op_invert(struct ctx *x, struct opnd o)
{
    struct opnd r, one, n;
    int vec, unit;
    float slo, shi;
    if (is_const(x, &o)) return o_const(x, 1.0f - clampf(cval(x, &o), 0.0f, 1.0f));
    slo = clampf(o.lo, 0.0f, 1.0f); shi = clampf(o.hi, 0.0f, 1.0f);
    if (slo == shi) return o_const(x, 1.0f - slo);
    unit = !pending(&o) && o.lo >= 0.0f && o.hi <= 1.0f;
    vec = o.comp < 0;
    one = o_const(x, 1.0f);
    n = o_neg(x, o);
    r = new_temp(x, vec, 1.0f - shi, 1.0f - slo);
    emit(x, OP_ADD, !unit, r.val, vec ? 7u : 8u, &one, &n, NULL);  /* sat(1 - x) = 1 - sat(x) */
    return r;
}

static struct opnd map_operand(struct ctx *x, struct opnd o, int mapping)
{
    struct opnd r, u;
    int i;
    for (i = 0; i < x->nmemo; i++)
        if (x->memo[i].mapping == mapping && same_view(&x->memo[i].key, &o)) return x->memo[i].result;
    switch (mapping) {
    case MAP_SAT: r = op_sat(x, o); break;
    case MAP_REAL: r = op_real(x, o); break;
    case 0x00: r = op_nonneg(x, o); break;
    case 0x20: r = op_invert(x, o); break;
    case 0x40:  /* 2 * max(x, 0) - 1 */
        u = map_operand(x, o, 0x00);
        r = mk_mad(x, u, o_const(x, 2.0f), o_const(x, -1.0f), o.comp < 0, 0);
        break;
    case 0x60:  /* 1 - 2 * max(x, 0) */
        u = map_operand(x, o, 0x00);
        r = mk_mad(x, u, o_const(x, -2.0f), o_const(x, 1.0f), o.comp < 0, 0);
        break;
    case 0x80:  /* max(x, 0) - 0.5 */
        u = map_operand(x, o, 0x00);
        r = mk_add(x, u, o_const(x, -0.5f), o.comp < 0, 0);
        break;
    case 0xa0:  /* 0.5 - max(x, 0) */
        u = map_operand(x, o, 0x00);
        r = mk_add(x, o_neg(x, u), o_const(x, 0.5f), o.comp < 0, 0);
        break;
    case 0xc0: r = op_real(x, o); break;                 /* x */
    default: r = o_neg(x, op_real(x, o)); break;         /* -x (0xe0) */
    }
    if (x->nmemo < MAX_MEMO) {
        x->memo[x->nmemo].key = o; x->memo[x->nmemo].result = r; x->memo[x->nmemo].mapping = mapping;
        x->nmemo++;
    }
    return r;
}

/* ---------- registers */

static struct opnd zero_opnd(struct ctx *x) { return o_const(x, 0.0f); }

static struct opnd mk_max(struct ctx *x, struct opnd a, struct opnd b, int vec)
{
    struct opnd r;
    if (is_const(x, &a) && is_const(x, &b)) return o_const(x, fmax2(cval(x, &a), cval(x, &b)));
    r = new_temp(x, vec, fmax2(a.lo, b.lo), fmax2(a.hi, b.hi));
    emit(x, OP_MAX, 0, r.val, vec ? 7u : 8u, &a, &b, NULL);
    return r;
}

/* the fog register's alpha: clamp(fog_factor, 0, 1) as nv2a_psh.c computes it */
static void fog_factor(struct ctx *x)
{
    const struct mac_nv2a_arbfp_state *s = x->s;
    struct opnd a, xf, t, z, y, st, d, inv, num;
    int fog_slot = x->layout ? x->layout->fog_texcoord : -1;
    if (x->fog_ready) return;
    x->fog_ready = 1;
    if (!s->fog_enable) { x->fog_a = o_const(x, 1.0f); return; }
    if (fog_slot < 0) xf = o_const(x, 1.0f);
    else xf = o_attr(x, fog_slot, x->layout->fog_component, -1.0e30f, 1.0e30f);
    z = o_env(x, MAC_NV2A_ARBFP_ENV_FOG_PARAMETERS, 2, 0.0f, 1.0e30f);
    y = o_env(x, MAC_NV2A_ARBFP_ENV_FOG_PARAMETERS, 1, -1.0e30f, 1.0e30f);
    st = o_env(x, MAC_NV2A_ARBFP_ENV_FOG_PARAMETERS, 0, -1.0e30f, 1.0e30f);
    switch (s->fog_table_mode) {
    case FOG_EXP:
    case FOG_EXP2:
        /* exp(-k) = 2^(-k log2(e)) */
        t = mk_mul(x, z, xf, 0, 0);
        if (s->fog_table_mode == FOG_EXP2) t = mk_mul(x, t, t, 0, 0);
        t = mk_mul(x, t, o_const(x, -1.4426950408889634f), 0, 0);
        a = new_temp(x, 0, 0.0f, 1.0f);
        emit(x, OP_EX2, 1, a.val, 8u, &t, NULL, NULL);
        break;
    case FOG_LINEAR:
        d = mk_max(x, mk_add(x, y, o_neg(x, st), 0, 0), o_const(x, 1.0e-6f), 0);
        inv = mk_unary(x, OP_RCP, d, 0, 0.0f, 1.0e30f);
        num = mk_add(x, y, o_neg(x, xf), 0, 0);
        a = mk_mul(x, num, inv, 0, 1);
        break;
    default:  /* the fog factor is the interpolated coordinate */
        a = op_sat(x, xf);
        break;
    }
    x->fog_a = a;
}

static int writable(unsigned long reg)
{
    return reg == REG_V0 || reg == REG_V1 || (reg >= REG_T0 && reg <= REG_R1);
}

/* the register `reg` as read by combiner stage `stage` (-1: the final combiner) */
static void get_reg(struct ctx *x, unsigned long reg, int stage, struct regs *out)
{
    const struct mac_nv2a_arbfp_state *s = x->s;
    int unique_c0 = (s->combiner_count & 0x1000) != 0, unique_c1 = (s->combiner_count & 0x10000) != 0;
    int index;
    out->rgb = out->a = zero_opnd(x);
    switch (reg) {
    case REG_C0:
        index = stage < 0 ? MAC_NV2A_ARBFP_ENV_FINAL_C0 : MAC_NV2A_ARBFP_ENV_C0 + (unique_c0 ? stage : 0);
        out->rgb = o_env(x, index, -1, 0.0f, 1.0f); out->a = o_env(x, index, 3, 0.0f, 1.0f);
        break;
    case REG_C1:
        index = stage < 0 ? MAC_NV2A_ARBFP_ENV_FINAL_C1 : MAC_NV2A_ARBFP_ENV_C1 + (unique_c1 ? stage : 0);
        out->rgb = o_env(x, index, -1, 0.0f, 1.0f); out->a = o_env(x, index, 3, 0.0f, 1.0f);
        break;
    case REG_FOG:
        fog_factor(x);
        out->rgb = o_env(x, MAC_NV2A_ARBFP_ENV_FOG_COLOR, -1, 0.0f, 1.0f);
        out->a = x->fog_a;
        break;
    case REG_V0: case REG_V1: case REG_T0: case REG_T1: case REG_T2: case REG_T3: case REG_R0: case REG_R1:
        *out = x->r[reg];
        break;
    case REG_V1R0_SUM: case REG_EF_PRODUCT:
        if (stage < 0) *out = x->r[reg];
        break;
    default:
        break;
    }
}

/* one general combiner input byte as an operand */
static struct opnd read_input(struct ctx *x, unsigned long input, int alpha_portion, int stage)
{
    unsigned long reg = input & 0x0f;
    int alpha_channel = (input & 0x10) != 0;
    int mapping = (int)(input & 0xe0);
    struct regs g;
    struct opnd b;
    get_reg(x, reg, stage, &g);
    if (alpha_channel) b = g.a;
    else if (alpha_portion) { b = g.rgb; if (!is_const(x, &b) && b.comp < 0) b.comp = 2; }
    else b = g.rgb;
    return map_operand(x, b, mapping);
}

/* a final combiner input: the unsigned identity or invert mapping */
static struct opnd read_final(struct ctx *x, unsigned long input, int alpha_portion)
{
    unsigned long reg = input & 0x0f;
    int alpha_channel = (input & 0x10) != 0;
    struct regs g;
    struct opnd b;
    get_reg(x, reg, -1, &g);
    if (alpha_channel) b = g.a;
    else if (alpha_portion) { b = g.rgb; if (!is_const(x, &b) && b.comp < 0) b.comp = 2; }
    else b = g.rgb;
    return map_operand(x, b, (input & 0x20) ? 0x20 : MAP_SAT);
}

/* ---------- texture stages */

static unsigned long stage_mode(const struct mac_nv2a_arbfp_state *s, int stage)
{ return (s->texture_modes >> (5 * stage)) & 0x1f; }

static int stage_input(const struct mac_nv2a_arbfp_state *s, int stage)
{
    switch (stage) {
    case 2: return (int)((s->input_texture >> 16) & 1);
    case 3: return (int)((s->input_texture >> 20) & 3);
    default: return 0;
    }
}

static void set_texture(struct ctx *x, int stage, int val)
{
    x->r[REG_T0 + stage].rgb = o_make(val, -1, 0, 0.0f, 1.0f);
    x->r[REG_T0 + stage].a = o_make(val, 3, 0, 0.0f, 1.0f);
}

static void set_texture_zero(struct ctx *x, int stage)
{
    x->r[REG_T0 + stage].rgb = x->r[REG_T0 + stage].a = zero_opnd(x);
}

static int new_scratch(struct ctx *x) { return new_val(x, V_TEMP, 0, 0.0f, 15); }

static int tex_target_of(int sampler)
{ return sampler == SAMPLER_3D ? 1 : (sampler == SAMPLER_CUBE ? 2 : 0); }

/* sample stage `stage`'s texture at the coordinates in scratch value `c`
   (xyz; for a 2D texture xy, scaled by texture_scale first, in place) */
static int sample_scratch(struct ctx *x, int stage, int c)
{
    int sampler = x->s->sampler_type[stage];
    int t, k;
    struct opnd co = o_raw(c, -1);
    if (sampler == SAMPLER_2D) {
        struct opnd scale = o_env(x, MAC_NV2A_ARBFP_ENV_TEXTURE_SCALE + stage, -1, -1.0e30f, 1.0e30f);
        emit(x, OP_MUL, 0, c, 3u, &co, &scale, NULL);
    }
    co.sw = "";
    t = new_scratch(x);
    k = emit(x, OP_TEX, 0, t, 15u, &co, NULL, NULL);
    x->ins[k].tex_unit = stage; x->ins[k].tex_target = tex_target_of(sampler);
    return t;
}

/* the signed byte value of a color: (b >= 128 ? b - 256 : b) / 127, b = round(255 c) */
static float signed_byte_f(float v)
{
    float b = (float)(int)(v * 255.0f + 0.5f);
    return (b >= 128.0f ? b - 256.0f : b) / 127.0f;
}

static struct opnd signed_bytes(struct ctx *x, struct opnd t)
{
    struct opnd b, m, r;
    if (is_const(x, &t)) return o_const(x, signed_byte_f(cval(x, &t)));
    b = mk_mad(x, t, o_const(x, 255.0f), o_const(x, 0.5f), 1, 0);
    b = mk_unary(x, OP_FLR, b, 1, 0.0f, 255.0f);
    m = new_temp(x, 1, 0.0f, 1.0f);
    {
        struct opnd k = o_const(x, 128.0f);
        emit(x, OP_SGE, 0, m.val, 7u, &b, &k, NULL);
    }
    r = mk_mad(x, m, o_const(x, -256.0f), b, 1, 0);
    return mk_mul(x, r, o_const(x, 1.0f / 127.0f), 1, 0);
}

/* the input texel of a dot product stage, mapped per PSDotMapping */
static struct opnd dot_input(struct ctx *x, int stage)
{
    unsigned long mapping = (x->s->dot_mapping >> ((stage - 1) * 4)) & 7;
    int input = stage_input(x->s, stage);
    struct opnd t = x->r[REG_T0 + input].rgb;
    switch (mapping) {
    case 0: return t;
    case 1: return mk_mad(x, t, o_const(x, 255.0f / 127.0f), o_const(x, -128.0f / 127.0f), 1, 0);
    case 3: return signed_bytes(x, t);
    default: return mk_mad(x, t, o_const(x, 2.0f), o_const(x, -1.0f), 1, 0);
    }
}

static void dot_lane(struct ctx *x, int stage, struct opnd input)
{
    struct opnd xt = o_slot(x, 2 + stage, -1, 0x7u, -1.0e30f, 1.0e30f);
    if (x->dot_val < 0) {
        struct opnd z = o_const(x, 0.0f);
        x->dot_val = new_scratch(x);
        emit(x, OP_MOV, 0, x->dot_val, 15u, &z, NULL, NULL);
    }
    emit(x, OP_DP3, 0, x->dot_val, 1u << stage, &xt, &input, NULL);
}

/* scratch xyz = (dot1, dot2, dot3) */
static int dot_vector(struct ctx *x)
{
    int c = new_scratch(x);
    struct opnd d = o_sw(o_raw(x->dot_val, -1), "yzww");
    emit(x, OP_MOV, 0, c, 7u, &d, NULL, NULL);
    return c;
}

static int texture_stage(struct ctx *x, int stage)
{
    const struct mac_nv2a_arbfp_state *s = x->s;
    unsigned long mode = stage_mode(s, stage);
    int sampler = s->sampler_type[stage];
    int unit = stage;
    static const char components[] = "xyzw";
    int t, k, c;
    struct opnd xt, co;

    comment(x, 1, stage);
    if (sampler == SAMPLER_NONE && mode != MODE_PASSTHRU && mode != MODE_CLIPPLANE &&
        mode != MODE_DOT_PRODUCT && mode != MODE_DOT_ZW)
        mode = MODE_NONE;
    switch (mode) {
    case MODE_PROJECT2D:
    case MODE_PROJECT3D:
        if (sampler == SAMPLER_CUBE) { fail(x, "project mode on a cube map"); return 0; }
        if (sampler == SAMPLER_3D || ((x->projective >> stage) & 1)) {
            t = new_scratch(x);
            co = o_slot(x, 2 + stage, -1, sampler == SAMPLER_3D ? 0xfu : 0xbu, -1.0e30f, 1.0e30f); co.sw = "";
            k = emit(x, OP_TXP, 0, t, 15u, &co, NULL, NULL);
            x->ins[k].tex_unit = unit; x->ins[k].tex_target = tex_target_of(sampler);
        } else {
            /* xy / (w != 0 ? w : 1) * texture_scale */
            struct opnd w = o_slot(x, 2 + stage, 3, 0x8u, -1.0e30f, 1.0e30f), one = o_const(x, 1.0f);
            struct opnd sw_, nsw, scale, sx;
            xt = o_slot(x, 2 + stage, -1, 0xbu, -1.0e30f, 1.0e30f);
            c = new_scratch(x);
            sw_ = o_raw(c, 3);
            emit(x, OP_ABS, 0, c, 8u, &w, NULL, NULL);
            nsw = o_neg(x, sw_);
            emit(x, OP_CMP, 0, c, 8u, &nsw, &w, &one);
            emit(x, OP_RCP, 0, c, 8u, &sw_, NULL, NULL);
            emit(x, OP_MUL, 0, c, 3u, &xt, &sw_, NULL);
            scale = o_env(x, MAC_NV2A_ARBFP_ENV_TEXTURE_SCALE + stage, -1, -1.0e30f, 1.0e30f);
            sx = o_raw(c, -1);
            emit(x, OP_MUL, 0, c, 3u, &sx, &scale, NULL);
            sx.sw = "";
            t = new_scratch(x);
            k = emit(x, OP_TEX, 0, t, 15u, &sx, NULL, NULL);
            x->ins[k].tex_unit = unit; x->ins[k].tex_target = tex_target_of(sampler);
        }
        set_texture(x, stage, t);
        break;
    case MODE_CUBEMAP:
        if (sampler == SAMPLER_2D) {
            xt = o_slot(x, 2 + stage, -1, 0x3u, -1.0e30f, 1.0e30f);
            c = new_scratch(x);
            emit(x, OP_MOV, 0, c, 7u, &xt, NULL, NULL);
            t = sample_scratch(x, stage, c);
        } else {
            t = new_scratch(x);
            co = o_slot(x, 2 + stage, -1, 0x7u, -1.0e30f, 1.0e30f); co.sw = "";
            k = emit(x, OP_TEX, 0, t, 15u, &co, NULL, NULL);
            x->ins[k].tex_unit = unit; x->ins[k].tex_target = tex_target_of(sampler);
        }
        set_texture(x, stage, t);
        break;
    case MODE_PASSTHRU:
        t = new_scratch(x);
        co = o_slot(x, 2 + stage, -1, 0xfu, -1.0e30f, 1.0e30f); co.sw = "";
        emit(x, OP_MOV, 1, t, 15u, &co, NULL, NULL);
        set_texture(x, stage, t);
        break;
    case MODE_CLIPPLANE:
    {
        unsigned long compare = (s->compare_mode >> (4 * stage)) & 0xf;
        unsigned lt_mask = 0, ge_mask = 0;
        int component;
        struct opnd v, vn, zero = o_const(x, 0.0f), half = o_const(x, 0.5f), kv;
        for (component = 0; component < 4; component++) {
            if (compare & (1u << component)) ge_mask |= 1u << component;
            else lt_mask |= 1u << component;
        }
        /* nv2a_psh.c discards where a component is < 0 (compare bit clear)
           or >= 0 (set). KIL discards where a component is < 0, so the
           first kind is the component itself and the second 0.5 - (x >= 0). */
        c = new_scratch(x);
        co = o_slot(x, 2 + stage, -1, 0xfu, -1.0e30f, 1.0e30f); co.sw = "xyzw";
        if (lt_mask) emit(x, OP_MOV, 0, c, lt_mask, &co, NULL, NULL);
        if (ge_mask) {
            emit(x, OP_SGE, 0, c, ge_mask, &co, &zero, NULL);
            v = o_raw(c, -1); v.sw = "xyzw";
            vn = o_neg(x, v);
            emit(x, OP_ADD, 0, c, ge_mask, &vn, &half, NULL);
        }
        kv = o_raw(c, -1);
        kv.sw = "";
        emit(x, OP_KIL, 0, DST_NONE, 0, &kv, NULL, NULL);
        set_texture_zero(x, stage);
        break;
    }
    case MODE_BUMPENVMAP:
    case MODE_BUMPENVMAP_LUMINANCE:
    {
        int input = stage - 1;
        struct opnd d, m, tin, zero = o_const(x, 0.0f);
        if (stage == 0) { fail(x, "bump mapping in stage 0"); return 0; }
        d = signed_bytes(x, x->r[REG_T0 + input].rgb);
        c = new_scratch(x);
        m = o_env(x, MAC_NV2A_ARBFP_ENV_BUMP_MATRIX + stage, -1, -1.0e30f, 1.0e30f);
        {
            /* (a constant input, the zero of an unfetched stage, is a splat) */
            struct opnd dx = is_const(x, &d) ? d : o_sw(d, "xxxx"), dy = is_const(x, &d) ? d : o_sw(d, "yyyy");
            struct opnd mz = o_sw(m, "zwww"), sc, xy = o_slot(x, 2 + stage, -1, 0x3u, -1.0e30f, 1.0e30f);
            emit(x, OP_MAD, 0, c, 3u, &m, &dx, &xy);
            sc = o_raw(c, -1);
            emit(x, OP_MAD, 0, c, 3u, &mz, &dy, &sc);
        }
        if (sampler != SAMPLER_2D) emit(x, OP_MOV, 0, c, 4u, &zero, NULL, NULL);
        t = sample_scratch(x, stage, c);
        if (mode == MODE_BUMPENVMAP_LUMINANCE) {
            struct opnd lum = o_env(x, MAC_NV2A_ARBFP_ENV_BUMP_LUMINANCE + stage, 0, -1.0e30f, 1.0e30f);
            struct opnd lumy = o_env(x, MAC_NV2A_ARBFP_ENV_BUMP_LUMINANCE + stage, 1, -1.0e30f, 1.0e30f);
            struct opnd l, tv, nt;
            tin = x->r[REG_T0 + input].rgb;
            if (!is_const(x, &tin) && tin.comp < 0) tin.comp = 2;
            l = new_temp(x, 0, 0.0f, 1.0f);
            emit(x, OP_MAD, 1, l.val, 8u, &lum, &tin, &lumy);
            tv = o_raw(t, -1);
            nt = new_temp(x, 1, 0.0f, 1.0f);
            emit(x, OP_MUL, 0, nt.val, 7u, &tv, &l, NULL);
            x->r[REG_T0 + stage].rgb = nt;
            x->r[REG_T0 + stage].a = o_make(t, 3, 0, 0.0f, 1.0f);
        } else {
            set_texture(x, stage, t);
        }
        break;
    }
    case MODE_DOT_PRODUCT:
    case MODE_DOT_ZW:
        if (stage == 0) { fail(x, "dot product in stage 0"); return 0; }
        dot_lane(x, stage, dot_input(x, stage));
        set_texture_zero(x, stage);
        break;
    case MODE_DOT_ST:
    {
        struct opnd z = o_const(x, 0.0f), d;
        char a, b;
        if (stage == 0) { fail(x, "dot product in stage 0"); return 0; }
        dot_lane(x, stage, dot_input(x, stage));
        c = new_scratch(x);
        a = components[stage - 1]; b = components[stage];
        d = o_sw(o_raw(x->dot_val, -1), make_sw(x, a, b, b, b));
        emit(x, OP_MOV, 0, c, 3u, &d, NULL, NULL);
        emit(x, OP_MOV, 0, c, 4u, &z, NULL, NULL);
        t = sample_scratch(x, stage, c);
        set_texture(x, stage, t);
        break;
    }
    case MODE_DOT_REFLECT_DIFFUSE:
    case MODE_DOT_STR_3D:
    case MODE_DOT_STR_CUBE:
        if (stage == 0) { fail(x, "dot product in stage 0"); return 0; }
        dot_lane(x, stage, dot_input(x, stage));
        if (mode == MODE_DOT_REFLECT_DIFFUSE) dot_lane(x, 3, dot_input(x, 3));
        c = dot_vector(x);
        t = sample_scratch(x, stage, c);
        set_texture(x, stage, t);
        break;
    case MODE_DOT_REFLECT_SPECULAR:
    case MODE_DOT_REFLECT_SPECULAR_CONSTANT:
    {
        int e = new_scratch(x), q = new_scratch(x);
        struct opnd n, ev, qx, qy, tiny = o_const(x, 1.0e-20f), two = o_const(x, 2.0f), nev;
        if (stage == 0) { fail(x, "dot product in stage 0"); return 0; }
        dot_lane(x, stage, dot_input(x, stage));
        c = dot_vector(x);
        n = o_raw(c, -1);
        ev = o_raw(e, -1);
        if (mode == MODE_DOT_REFLECT_SPECULAR) {
            int j;
            for (j = 1; j <= 3; j++) {
                struct opnd w = o_slot(x, 2 + j, 3, 0x8u, -1.0e30f, 1.0e30f);
                emit(x, OP_MOV, 0, e, 1u << (j - 1), &w, NULL, NULL);
            }
        } else {
            struct opnd c0 = o_env(x, MAC_NV2A_ARBFP_ENV_C0, -1, 0.0f, 1.0f);
            emit(x, OP_MOV, 0, e, 7u, &c0, NULL, NULL);
        }
        qx = o_raw(q, 0); qy = o_raw(q, 1);
        emit(x, OP_DP3, 0, q, 1u, &n, &ev, NULL);
        emit(x, OP_DP3, 0, q, 2u, &n, &n, NULL);
        emit(x, OP_MAX, 0, q, 2u, &qy, &tiny, NULL);
        emit(x, OP_RCP, 0, q, 2u, &qy, NULL, NULL);
        emit(x, OP_MUL, 0, q, 1u, &qx, &qy, NULL);
        emit(x, OP_MUL, 0, q, 1u, &qx, &two, NULL);
        nev = o_neg(x, ev);
        emit(x, OP_MAD, 0, c, 7u, &n, &qx, &nev);
        t = sample_scratch(x, stage, c);
        set_texture(x, stage, t);
        break;
    }
    case MODE_DEPENDENT_AR:
    case MODE_DEPENDENT_GB:
    {
        int input = stage_input(s, stage);
        struct opnd tin_rgb = x->r[REG_T0 + input].rgb, tin_a = x->r[REG_T0 + input].a;
        struct opnd z = o_const(x, 0.0f);
        c = new_scratch(x);
        if (mode == MODE_DEPENDENT_AR) {
            struct opnd first = tin_a, second = tin_rgb;
            if (!is_const(x, &second) && second.comp < 0) second.comp = 0;
            emit(x, OP_MOV, 0, c, 1u, &first, NULL, NULL);
            emit(x, OP_MOV, 0, c, 2u, &second, NULL, NULL);
        } else {
            struct opnd g = tin_rgb, b = tin_rgb;
            if (!is_const(x, &g) && g.comp < 0) g.comp = 1;
            if (!is_const(x, &b) && b.comp < 0) b.comp = 2;
            emit(x, OP_MOV, 0, c, 1u, &g, NULL, NULL);
            emit(x, OP_MOV, 0, c, 2u, &b, NULL, NULL);
        }
        emit(x, OP_MOV, 0, c, 4u, &z, NULL, NULL);
        t = sample_scratch(x, stage, c);
        set_texture(x, stage, t);
        break;
    }
    case MODE_NONE:
        set_texture_zero(x, stage);
        break;
    default:
        fail(x, "texture mode not supported (brdf or undefined)");
        return 0;
    }
    if (s->alpha_kill[stage] && mode != MODE_NONE) {
        /* nv2a_psh.c: discard where the texture's alpha is 0. KIL discards
           where a component is < 0, so alpha less half a level of 8 bits. */
        struct opnd ka = x->r[REG_T0 + stage].a;
        struct opnd kt = mk_add(x, ka, o_const(x, -0.5f / 255.0f), 0, 0);
        emit(x, OP_KIL, 0, DST_NONE, 0, &kt, NULL, NULL);
    }
    if (s->color_sign[stage] && mode != MODE_NONE) { fail(x, "signed texture channels are not supported"); return 0; }
    return !x->failed;
}

/* ---------- one general combiner stage */


struct portion_result {
    struct opnd ab, cd, sum;
    int have_ab, have_cd, have_sum;
};

static struct opnd output_mapped(struct ctx *x, struct opnd raw, unsigned long flags, int vec)
{
    float k = 1.0f, b = 0.0f;
    struct opnd r;
    switch (flags & 0x38) {
    case 0x08: b = -0.5f; break;
    case 0x10: k = 2.0f; break;
    case 0x18: k = 2.0f; b = -1.0f; break;
    case 0x20: k = 4.0f; break;
    case 0x30: k = 0.5f; break;
    default: break;
    }
    if (k != 1.0f || b != 0.0f) raw = mk_mad(x, raw, o_const(x, k), o_const(x, b), vec, 0);
    if (is_const(x, &raw)) return o_const(x, clampf(cval(x, &raw), -1.0f, 1.0f));
    r = raw;
    r.lo = fmax2(r.rlo, -1.0f);
    r.hi = fmin2(r.rhi, 1.0f);
    if (r.lo == r.hi) return o_const(x, r.lo);
    return r;
}

/* cond ? cd : ab for the mux flag, the condition on r0.a */
static struct opnd mux_select(struct ctx *x, struct opnd ab, struct opnd cd, int vec)
{
    int msb = (x->s->combiner_count & 0x100) != 0;
    struct opnd a = x->r[REG_R0].a, t, cmp, half = o_const(x, -0.5f), r;
    float lo, hi;
    lo = fmin2(ab.lo, cd.lo); hi = fmax2(ab.hi, cd.hi);
    if (is_const(x, &a)) {
        float v = cval(x, &a);
        int cond;
        if (msb) cond = v >= 0.5f;
        else cond = ((int)(v * 255.0f + 0.5f) & 1) != 0;
        return cond ? cd : ab;
    }
    if (msb) {
        t = mk_add(x, a, half, 0, 0);   /* r0.a - 0.5: >= 0 selects cd */
    } else {
        struct opnd real = map_operand(x, a, MAP_REAL);
        struct opnd s, q;
        s = mk_mad(x, real, o_const(x, 255.0f), o_const(x, 0.5f), 0, 0);
        s = mk_unary(x, OP_ABS, s, 0, 0.0f, 256.0f);
        s = mk_unary(x, OP_FLR, s, 0, 0.0f, 256.0f);
        s = mk_mul(x, s, o_const(x, 0.5f), 0, 0);
        q = mk_unary(x, OP_FRC, s, 0, 0.0f, 1.0f);       /* 0.5 for an odd count, 0 for even */
        t = mk_add(x, q, o_const(x, -0.25f), 0, 0);
    }
    r = new_temp(x, vec, lo, hi);
    cmp = t;
    emit(x, OP_CMP, 0, r.val, vec ? 7u : 8u, &cmp, &ab, &cd);
    return r;
}

static int combiner_stage(struct ctx *x, int stage)
{
    const struct mac_nv2a_arbfp_state *s = x->s;
    struct portion_result res[2];
    unsigned long dest[2][3], flagsv[2];
    int portion;
    memset(res, 0, sizeof(res));
    comment(x, 2, stage);
    for (portion = 0; portion < 2; portion++) {
        int alpha = portion == 1, vec = !alpha;
        unsigned long inputs = alpha ? s->alpha_inputs[stage] : s->rgb_inputs[stage];
        unsigned long outputs = alpha ? s->alpha_outputs[stage] : s->rgb_outputs[stage];
        unsigned long flags = outputs >> 12;
        unsigned long dab = (outputs >> 4) & 0x0f, dcd = outputs & 0x0f, dsum = (outputs >> 8) & 0x0f;
        int use_ab = writable(dab), use_cd = writable(dcd), use_sum = writable(dsum);
        int need_ab = use_ab || use_sum, need_cd = use_cd || use_sum;
        int dot_ab = !alpha && (flags & 0x02), dot_cd = !alpha && (flags & 0x01);
        int mux = (flags & 0x04) != 0;
        struct opnd A, B, C, D, ab, cd, sum;
        struct portion_result *p = &res[portion];
        dest[portion][0] = dab; dest[portion][1] = dcd; dest[portion][2] = dsum;
        flagsv[portion] = flags;
        if (!need_ab && !need_cd) continue;
        A = B = C = D = zero_opnd(x);
        ab = cd = sum = zero_opnd(x);
        if (need_ab) {
            A = read_input(x, (inputs >> 24) & 0xff, alpha, stage);
            B = read_input(x, (inputs >> 16) & 0xff, alpha, stage);
        }
        if (need_cd) {
            C = read_input(x, (inputs >> 8) & 0xff, alpha, stage);
            D = read_input(x, inputs & 0xff, alpha, stage);
        }
        if (use_sum && !mux && !use_ab && !use_cd) {
            /* only the sum: A*B + C*D as a multiply and a multiply-add */
            if (!dot_ab) {
                cd = dot_cd ? mk_dp3(x, C, D) : mk_mul(x, C, D, vec, 0);
                sum = mk_mad(x, A, B, cd, vec, 0);
            } else if (!dot_cd) {
                ab = mk_dp3(x, A, B);
                sum = mk_mad(x, C, D, ab, vec, 0);
            } else {
                ab = mk_dp3(x, A, B);
                cd = mk_dp3(x, C, D);
                sum = mk_add(x, ab, cd, vec, 0);
            }
        } else {
            if (need_ab) ab = dot_ab ? mk_dp3(x, A, B) : mk_mul(x, A, B, vec, 0);
            if (need_cd) cd = dot_cd ? mk_dp3(x, C, D) : mk_mul(x, C, D, vec, 0);
            if (use_sum) sum = mux ? mux_select(x, ab, cd, vec) : mk_add(x, ab, cd, vec, 0);
        }
        if (use_ab) { p->ab = output_mapped(x, ab, flags, vec); p->have_ab = 1; }
        if (use_cd) { p->cd = output_mapped(x, cd, flags, vec); p->have_cd = 1; }
        if (use_sum) { p->sum = output_mapped(x, sum, flags, vec); p->have_sum = 1; }
        if (x->failed) return 0;
    }
    /* write back only after both portions have read their inputs */
    for (portion = 0; portion < 2; portion++) {
        int alpha = portion == 1, which;
        unsigned long flags = flagsv[portion];
        for (which = 0; which < 3; which++) {
            struct opnd v;
            int have;
            unsigned long reg = dest[portion][which];
            if (which == 0) { have = res[portion].have_ab; v = res[portion].ab; }
            else if (which == 1) { have = res[portion].have_cd; v = res[portion].cd; }
            else { have = res[portion].have_sum; v = res[portion].sum; }
            if (!have) continue;
            if (alpha) {
                x->r[reg].a = v;
            } else {
                x->r[reg].rgb = v;
                if ((which == 0 && (flags & 0x80)) || (which == 1 && (flags & 0x40))) {
                    struct opnd bl = v;
                    if (!is_const(x, &bl) && bl.comp < 0) bl.comp = 2;
                    x->r[reg].a = bl;
                }
            }
        }
    }
    return !x->failed;
}

/* ---------- the final combiner */

static int final_uses(const struct mac_nv2a_arbfp_state *s, unsigned long reg)
{
    unsigned long bytes[5];
    int i;
    bytes[0] = (s->final_abcd >> 24) & 0xff; bytes[1] = (s->final_abcd >> 16) & 0xff;
    bytes[2] = (s->final_abcd >> 8) & 0xff; bytes[3] = s->final_abcd & 0xff;
    bytes[4] = (s->final_efg >> 8) & 0xff;
    for (i = 0; i < 5; i++)
        if ((bytes[i] & 0x0f) == reg) return 1;
    return 0;
}

/* result.rgb and result.a before the output clamp */
static void final_combiner(struct ctx *x, struct opnd *rgb, struct opnd *a)
{
    const struct mac_nv2a_arbfp_state *s = x->s;
    unsigned long abcd = s->final_abcd, efg = s->final_efg, settings = efg & 0xff;
    struct opnd A, B, C, D, G, lerp;
    if (abcd == 0 && efg == 0) {
        *rgb = x->r[REG_R0].rgb;
        *a = x->r[REG_R0].a;
        return;
    }
    x->r[REG_V1R0_SUM].rgb = x->r[REG_V1R0_SUM].a = zero_opnd(x);
    x->r[REG_EF_PRODUCT].rgb = x->r[REG_EF_PRODUCT].a = zero_opnd(x);
    if (final_uses(s, REG_V1R0_SUM)) {
        struct opnd t1 = map_operand(x, x->r[REG_V1].rgb, (settings & 0x40) ? 0x20 : MAP_SAT);
        struct opnd t2 = map_operand(x, x->r[REG_R0].rgb, (settings & 0x20) ? 0x20 : MAP_SAT);
        x->r[REG_V1R0_SUM].rgb = mk_add(x, t1, t2, 1, (settings & 0x80) != 0);
    }
    if (final_uses(s, REG_EF_PRODUCT)) {
        struct opnd e = read_final(x, (efg >> 24) & 0xff, 0);
        struct opnd f = read_final(x, (efg >> 16) & 0xff, 0);
        x->r[REG_EF_PRODUCT].rgb = mk_mul(x, e, f, 1, 0);
    }
    A = read_final(x, (abcd >> 24) & 0xff, 0);
    B = read_final(x, (abcd >> 16) & 0xff, 0);
    C = read_final(x, (abcd >> 8) & 0xff, 0);
    D = read_final(x, abcd & 0xff, 0);
    G = read_final(x, (efg >> 8) & 0xff, 1);
    /* A * B + (1 - A) * C + D, saturated: the output clamp */
    if (is_const(x, &D) && cval(x, &D) == 0.0f) {
        *rgb = mk_lrp(x, A, B, C, 1, 1);
    } else {
        lerp = mk_lrp(x, A, B, C, 1, 0);
        *rgb = mk_add(x, lerp, D, 1, 1);
    }
    *a = G;
}

/* ---------- alpha test, output */

/* discard where !(floor(sat(alpha) * 255 + 0.5) OP alpha_reference). With the
   integer a8 = floor(sat(a) * 255 + 0.5) and the integer reference R every
   comparison is a sign test of a difference, KIL's: the strict ones move the
   half step into the floor. */
static void alpha_test(struct ctx *x, struct opnd a)
{
    unsigned long function = x->s->alpha_test_function;
    struct opnd sa, q, f, t, r, d, kv;
    float offset = 0.5f;
    if (!function) return;
    switch (function) {
    case CMP_NEVER:
    {
        struct opnd minus_one = o_const(x, -1.0f);
        int c = new_scratch(x);
        emit(x, OP_MOV, 0, c, 1u, &minus_one, NULL, NULL);
        kv = o_raw(c, 0);
        emit(x, OP_KIL, 0, DST_NONE, 0, &kv, NULL, NULL);
        return;
    }
    case CMP_LESS: offset = 1.5f; break;
    case CMP_GREATER: offset = -0.5f; break;
    case CMP_EQUAL: case CMP_LESSEQUAL: case CMP_NOTEQUAL: case CMP_GREATEREQUAL: break;
    default: return;  /* D3DCMP_ALWAYS */
    }
    sa = map_operand(x, a, MAP_SAT);
    q = mk_mad(x, sa, o_const(x, 255.0f), o_const(x, offset), 0, 0);
    f = mk_unary(x, OP_FLR, q, 0, -1.0f, 257.0f);
    r = o_env(x, MAC_NV2A_ARBFP_ENV_ALPHA_REFERENCE, 0, 0.0f, 255.0f);
    switch (function) {
    case CMP_GREATER: case CMP_GREATEREQUAL:
        t = mk_add(x, f, o_neg(x, r), 0, 0);              /* kept where >= 0 */
        break;
    case CMP_LESS: case CMP_LESSEQUAL:
        t = mk_add(x, r, o_neg(x, f), 0, 0);
        break;
    case CMP_EQUAL:
        d = mk_add(x, f, o_neg(x, r), 0, 0);
        d = mk_unary(x, OP_ABS, d, 0, 0.0f, 257.0f);
        t = mk_add(x, o_neg(x, d), o_const(x, 0.5f), 0, 0);   /* 0.5 - |d|: kept where d == 0 */
        break;
    default:  /* D3DCMP_NOTEQUAL */
        d = mk_add(x, f, o_neg(x, r), 0, 0);
        d = mk_unary(x, OP_ABS, d, 0, 0.0f, 257.0f);
        t = mk_add(x, d, o_const(x, -0.5f), 0, 0);            /* |d| - 0.5: kept where d != 0 */
        break;
    }
    emit(x, OP_KIL, 0, DST_NONE, 0, &t, NULL, NULL);
}

static void write_output(struct ctx *x, struct opnd rgb, struct opnd a)
{
    if (!is_const(x, &rgb) && !is_const(x, &a) && rgb.val == a.val && rgb.comp < 0 && a.comp == 3 &&
        !rgb.neg && !a.neg && !rgb.sw && !a.sw && x->vals[rgb.val].lanes == 15) {
        struct opnd whole = rgb;
        whole.sw = "";
        emit(x, OP_MOV, 1, DST_COLOR, 15u, &whole, NULL, NULL);
        return;
    }
    emit(x, OP_MOV, 1, DST_COLOR, 7u, &rgb, NULL, NULL);
    emit(x, OP_MOV, 1, DST_COLOR, 8u, &a, NULL, NULL);
}

/* ---------- finishing: drop what nothing reads, then fold the final MOV_SAT into its producer */

/* An instruction whose temporary is never read (a combiner output no later stage or the final combiner uses,
   a value overwritten before use, a texture fetch nothing reads) is dropped, until none is left. */
static void remove_dead(struct ctx *x)
{
    int changed;
    unsigned char *read = (unsigned char *)calloc(MAX_VALS, 1);
    if (!read) return;
    do {
        int i, k;
        changed = 0;
        memset(read, 0, MAX_VALS);
        for (i = 0; i < x->nins; i++) {
            struct ins *n = &x->ins[i];
            if (n->deleted || n->op == OP_COMMENT) continue;
            for (k = 0; k < n->nsrc; k++)
                if (n->s[k].val != n->dst) read[n->s[k].val] = 1;  /* (a read by its own writer keeps nothing alive) */
        }
        for (i = 0; i < x->nins; i++) {
            struct ins *n = &x->ins[i];
            if (n->deleted || n->op == OP_COMMENT || n->op == OP_KIL || n->dst < 0) continue;
            if (!read[n->dst]) { n->deleted = 1; changed = 1; }
        }
    } while (changed);
    free(read);
}


static void peephole(struct ctx *x)
{
    int i, j;
    for (i = 0; i < x->nins; i++) {
        struct ins *m = &x->ins[i];
        int v, uses = 0, writers = 0, def = -1;
        if (m->deleted || m->op != OP_MOV || !m->sat || m->dst != DST_COLOR) continue;
        if (m->s[0].neg || m->s[0].sw || x->vals[m->s[0].val].kind != V_TEMP) continue;
        v = m->s[0].val;
        for (j = 0; j < x->nins; j++) {
            struct ins *n = &x->ins[j];
            int k;
            if (n->deleted || n->op == OP_COMMENT) continue;
            if (j != i)
                for (k = 0; k < n->nsrc; k++)
                    if (n->s[k].val == v) uses++;
            if (n->dst == v) { writers++; def = j; }
        }
        if (uses || writers != 1 || def > i) continue;
        {
            struct ins *d = &x->ins[def];
            if (d->op == OP_TEX || d->op == OP_TXP || d->op == OP_KIL || d->op == OP_COMMENT) continue;
            if (d->mask != m->mask) continue;
            if (m->mask == 7u && m->s[0].comp >= 0) continue;
            if (m->mask == 8u && m->s[0].comp != 3) continue;
            if (m->mask != 7u && m->mask != 8u) continue;
            d->dst = DST_COLOR; d->sat = 1;
            m->deleted = 1;
        }
    }
}

/* ---------- text */

struct sbuf { char *data; size_t length, capacity; int failed; };

static void sb_add(struct sbuf *b, const char *format, ...)
{
    va_list args;
    char piece[256];
    size_t n;
    int written;
    if (b->failed) return;
    va_start(args, format);
    written = vsnprintf(piece, sizeof(piece), format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= sizeof(piece)) { b->failed = 1; return; }
    n = (size_t)written;
    if (b->length + n + 1 > b->capacity) {
        size_t capacity = b->capacity ? b->capacity * 2 : 4096;
        char *grown;
        while (capacity < b->length + n + 1) capacity *= 2;
        grown = (char *)realloc(b->data, capacity);
        if (!grown) { b->failed = 1; return; }
        b->data = grown; b->capacity = capacity;
    }
    memcpy(b->data + b->length, piece, n + 1);
    b->length += n;
}

static void sb_float(struct sbuf *b, float v)
{
    char text[40];
    int i, plain = 1;
    if (v == 0.0f) v = 0.0f;
    sprintf(text, "%.9g", (double)v);
    for (i = 0; text[i]; i++)
        if (text[i] == '.' || text[i] == 'e' || text[i] == 'n' || text[i] == 'i') plain = 0;
    if (plain) strcat(text, ".0");
    sb_add(b, "%s", text);
}

static void print_src(struct sbuf *b, const struct ctx *x, const struct src *s, const int *phys)
{
    const struct val *v = &x->vals[s->val];
    static const char comps[] = "xyzw";
    if (v->kind == V_CONST) { sb_float(b, s->neg ? -v->c : v->c); return; }
    if (s->neg) sb_add(b, "-");
    switch (v->kind) {
    case V_ATTR: sb_add(b, "fragment.texcoord[%d]", v->index); break;
    case V_ENV: sb_add(b, "program.env[%d]", v->index); break;
    default: sb_add(b, "R%d", phys[s->val]); break;
    }
    if (s->sw) { if (s->sw[0]) sb_add(b, ".%s", s->sw); }
    else if (s->comp < 0) sb_add(b, ".xyzz");
    else sb_add(b, ".%c", comps[s->comp & 3]);
}

static void print_mask(struct sbuf *b, unsigned mask)
{
    static const char comps[] = "xyzw";
    int i;
    if (mask == 15u) return;
    sb_add(b, ".");
    for (i = 0; i < 4; i++)
        if (mask & (1u << i)) sb_add(b, "%c", comps[i]);
}

static const char *const target_names[] = { "2D", "3D", "CUBE" };

static int emit_text(struct ctx *x, struct sbuf *b, struct mac_nv2a_arbfp_info *info)
{
    int *first, *last, *phys;
    int busy[MAX_TEMPS][4];
    int i, k, ntemps = 0;
    unsigned alu = 0, tex = 0;
    first = (int *)malloc(sizeof(int) * 3 * MAX_VALS);
    if (!first) return -1;
    last = first + MAX_VALS; phys = last + MAX_VALS;
    for (i = 0; i < MAX_VALS; i++) { first[i] = -1; last[i] = -1; phys[i] = -1; }
    for (i = 0; i < x->nins; i++) {
        struct ins *n = &x->ins[i];
        if (n->deleted || n->op == OP_COMMENT) continue;
        for (k = 0; k < n->nsrc; k++)
            if (x->vals[n->s[k].val].kind == V_TEMP) {
                int v = n->s[k].val;
                if (first[v] < 0) first[v] = i;
                last[v] = i;
            }
        if (n->dst >= 0) {
            if (first[n->dst] < 0) first[n->dst] = i;
            last[n->dst] = i;
        }
    }
    memset(busy, 0xff, sizeof(busy));
    for (i = 0; i < x->nins; i++) {
        struct ins *n = &x->ins[i];
        int v = n->dst;
        if (n->deleted || n->op == OP_COMMENT || v < 0 || phys[v] >= 0) continue;
        for (k = 0; k < MAX_TEMPS; k++) {
            int l, ok = 1;
            for (l = 0; l < 4; l++)
                if ((x->vals[v].lanes & (1 << l)) && busy[k][l] > i) ok = 0;
            if (ok) break;
        }
        if (k == MAX_TEMPS) { fail(x, "pixel shader needs too many temporaries"); free(first); return 0; }
        phys[v] = k;
        if (k + 1 > ntemps) ntemps = k + 1;
        {
            int l;
            for (l = 0; l < 4; l++)
                if (x->vals[v].lanes & (1 << l)) busy[k][l] = last[v];
        }
    }
    sb_add(b, "!!ARBfp1.0\n");
    for (i = 0; i < ntemps; i++) {
        if (i % 8 == 0) sb_add(b, "%sTEMP R%d", i ? ";\n" : "", i);
        else sb_add(b, ", R%d", i);
    }
    if (ntemps) sb_add(b, ";\n");
    for (i = 0; i < x->nins; i++) {
        struct ins *n = &x->ins[i];
        if (n->deleted) continue;
        if (n->op == OP_COMMENT) {
            sb_add(b, n->comment == 1 ? "# texture stage %d\n" : "# combiner stage %d\n", n->comment_number);
            continue;
        }
        sb_add(b, "%s%s ", op_names[n->op], n->sat ? "_SAT" : "");
        if (n->op == OP_KIL) {
            print_src(b, x, &n->s[0], phys);
            info->uses_kil = 1;
        } else {
            if (n->dst == DST_COLOR) sb_add(b, "result.color");
            else sb_add(b, "R%d", phys[n->dst]);
            print_mask(b, n->mask);
            for (k = 0; k < n->nsrc; k++) {
                sb_add(b, ", ");
                print_src(b, x, &n->s[k], phys);
            }
            if (n->op == OP_TEX || n->op == OP_TXP) sb_add(b, ", texture[%d], %s", n->tex_unit, target_names[n->tex_target]);
        }
        sb_add(b, ";\n");
        if (n->op == OP_TEX || n->op == OP_TXP) tex++;
        else if (n->op != OP_KIL) alu++;
        for (k = 0; k < n->nsrc; k++)
            if (x->vals[n->s[k].val].kind == V_ENV && x->vals[n->s[k].val].index < MAC_NV2A_ARBFP_ENV_COUNT)
                info->env_used[x->vals[n->s[k].val].index] = 1;
    }
    sb_add(b, "END\n");
    info->alu_instructions = alu; info->tex_instructions = tex; info->temporaries = (unsigned)ntemps;
    free(first);
    return b->failed ? -1 : 1;
}

/* ---------- entry points */

static void set_error(char *error, size_t size, const char *message)
{
    if (error && size) { strncpy(error, message, size - 1); error[size - 1] = 0; }
}

/* The combiner state as instructions (dead ones dropped, the output clamp
   folded): 1 with *x_out, 0 when the state uses something not supported
   (error says what), -1 on a bad argument or allocation failure. */
static int build_program(const struct mac_nv2a_arbfp_state *state,
    const struct mac_nv2a_arb_layout *layout, unsigned long projective_mask,
    struct ctx **x_out, char *error, size_t error_size)
{
    struct ctx *x;
    int stage, count, i;
    struct opnd rgb, a;
    *x_out = NULL;
    if (error && error_size) error[0] = 0;
    if (!state) return -1;
    if (state->count_samples) { set_error(error, error_size, "visibility sample counting is not supported"); return 0; }
    if (state->point_threshold) { set_error(error, error_size, "point-sampled thresholds (texelFetch) are not supported"); return 0; }
    if (state->per_pixel_lighting) { set_error(error, error_size, "per-pixel lighting is not supported"); return 0; }
    if (state->alpha_test_samples) { set_error(error, error_size, "multisampled alpha test is not supported"); return 0; }
    if (state->source_blend) { set_error(error, error_size, "source blend factors are not supported"); return 0; }
    if (layout && layout->fog_texcoord >= 0 && (layout->fog_texcoord > 7 || layout->fog_component < 0 || layout->fog_component > 3)) {
        set_error(error, error_size, "invalid fog layout"); return -1;
    }
    x = (struct ctx *)calloc(1, sizeof(*x));
    if (!x) return -1;
    x->s = state; x->layout = layout; x->projective = projective_mask; x->dot_val = -1;
    for (i = 0; i < 16; i++) x->r[i].rgb = x->r[i].a = zero_opnd(x);
    for (i = 0; i < 8; i++) x->clean[i] = -1;
    x->r[REG_V0].rgb = o_slot(x, 6, -1, 0xfu, 0.0f, 1.0f); x->r[REG_V0].a = o_slot(x, 6, 3, 0x8u, 0.0f, 1.0f);
    x->r[REG_V1].rgb = o_slot(x, 7, -1, 0xfu, 0.0f, 1.0f); x->r[REG_V1].a = o_slot(x, 7, 3, 0x8u, 0.0f, 1.0f);
    for (stage = 0; stage < 4 && !x->failed; stage++) texture_stage(x, stage);
    /* r0 = (0, 0, 0, t0.a), r1 = 0 */
    x->r[REG_R0].a = x->r[REG_T0].a;
    count = (int)(state->combiner_count & 0xff);
    if (count > 8) count = 8;
    for (stage = 0; stage < count && !x->failed; stage++) combiner_stage(x, stage);
    if (!x->failed) {
        final_combiner(x, &rgb, &a);
        if (state->coverage_alpha) {
            struct opnd g = x->r[REG_T0].rgb;
            if (!is_const(x, &g) && g.comp < 0) g.comp = 1;
            a = mk_lrp(x, g, map_operand(x, a, MAP_REAL), o_const(x, 1.0f), 0, 0);
        }
        alpha_test(x, a);
        write_output(x, rgb, a);
    }
    if (x->failed) {
        set_error(error, error_size, x->message);
        free(x);
        return 0;
    }
    remove_dead(x);
    peephole(x);
    *x_out = x;
    return 1;
}

int mac_nv2a_arbfp_translate_state(const struct mac_nv2a_arbfp_state *state,
    const struct mac_nv2a_arb_layout *layout, unsigned long projective_mask,
    char **arb_out, struct mac_nv2a_arbfp_info *info, char *error, size_t error_size)
{
    struct ctx *x;
    struct sbuf b;
    struct mac_nv2a_arbfp_info local;
    int result;
    if (arb_out) *arb_out = NULL;
    if (error && error_size) error[0] = 0;
    if (!state || !arb_out) return -1;
    if (!info) info = &local;
    memset(info, 0, sizeof(*info));
    result = build_program(state, layout, projective_mask, &x, error, error_size);
    if (result != 1) return result;
    memset(&b, 0, sizeof(b));
    result = emit_text(x, &b, info);
    if (result == 0) set_error(error, error_size, x->message);
    if (result != 1) { free(b.data); free(x); return result; }
    free(x);
    *arb_out = b.data;
    return 1;
}

/* ---------- fixed-function lowering (see mac_nv2a_arbfp.h)

   The instructions, in order, become expression nodes for the rgb lanes and
   the alpha lane separately; the expression of each output is then cut into
   a chain of combine stages in which each stage's PREVIOUS is the stage
   before. Anything a combine stage cannot do exactly is refused: a negative,
   a swizzle, a scale, a dot product, the secondary colour (the combine unit
   has no such source), a value that leaves [0,1] without a saturating
   instruction (each stage clamps), two computed operands in one operation,
   or a second constant in one stage. */

enum { FN_LEAF, FN_LIT, FN_MUL, FN_ADD, FN_SUB, FN_MAD, FN_LRP, FN_SCL /* a * lit (2 or 4) */ };
#define FN_MAX 160
struct fnode {
    int kind, a, b, c;
    int src, alpha, inv, env;   /* FN_LEAF: unit 0..3, primary, constant */
    float lit;                  /* FN_LIT */
    float lo, hi;
    int sat;
};
struct fftx {
    struct ctx *x;
    const struct mac_nv2a_arbfp_state *s;
    struct fnode n[FN_MAX];
    int nn;
    int (*tn)[2];               /* per value: the node of its rgb lanes and of its alpha lane */
    signed char *tex;           /* per value: the texture unit it holds, or -1 */
    int out_rgb, out_a;
    int kil_state, kil_node, kil_t1, kil_t2, kil_t3;
    float kil_off;
    int failed;
    char message[160];
    unsigned char unit_2d;
    int needs_combine3;
};

static int ffail(struct fftx *t, const char *message)
{
    if (!t->failed) {
        t->failed = 1;
        strncpy(t->message, message, sizeof(t->message) - 1);
        t->message[sizeof(t->message) - 1] = 0;
    }
    return -1;
}

static int fn_new(struct fftx *t, int kind)
{
    struct fnode *n;
    if (t->nn >= FN_MAX) return ffail(t, "too many expression nodes");
    n = &t->n[t->nn];
    memset(n, 0, sizeof(*n));
    n->kind = kind; n->a = n->b = n->c = -1;
    return t->nn++;
}

static int fn_leaf(struct fftx *t, int src, int alpha, int inv, int env)
{
    int i;
    for (i = 0; i < t->nn; i++) {
        const struct fnode *n = &t->n[i];
        if (n->kind == FN_LEAF && n->src == src && n->alpha == alpha && n->inv == inv && n->env == env) return i;
    }
    i = fn_new(t, FN_LEAF);
    if (i < 0) return -1;
    t->n[i].src = src; t->n[i].alpha = alpha; t->n[i].inv = inv; t->n[i].env = env;
    t->n[i].lo = 0.0f; t->n[i].hi = 1.0f;
    if (src < 4) t->unit_2d = (unsigned char)(t->unit_2d | (1u << src));
    return i;
}

static int fn_lit(struct fftx *t, float v)
{
    int i;
    for (i = 0; i < t->nn; i++)
        if (t->n[i].kind == FN_LIT && t->n[i].lit == v) return i;
    i = fn_new(t, FN_LIT);
    if (i < 0) return -1;
    t->n[i].lit = v; t->n[i].lo = t->n[i].hi = v;
    return i;
}

static int fn_leafy(const struct fftx *t, int i) { return t->n[i].kind == FN_LEAF || t->n[i].kind == FN_LIT; }

/* a node's value must be in [0,1] to be used by anything but a saturate */
static int fn_use(struct fftx *t, int i)
{
    if (i < 0) return -1;
    if (t->n[i].sat || (t->n[i].lo >= 0.0f && t->n[i].hi <= 1.0f)) return i;
    return ffail(t, "a value leaves [0,1] before it is saturated");
}

static int fn_src(struct fftx *t, const struct src *s, int g, int *neg)
{
    const struct val *v = &t->x->vals[s->val];
    int alpha, n;
    *neg = 0;
    if (v->kind == V_CONST) return fn_lit(t, s->neg ? -v->c : v->c);
    if (s->sw) return ffail(t, "swizzled source");
    if (s->comp < 0) { if (g != 0) return ffail(t, "vector read in the alpha lane"); alpha = 0; }
    else if (s->comp == 3) alpha = 1;
    else return ffail(t, "single colour channel read");
    *neg = s->neg != 0;
    switch (v->kind) {
    case V_ATTR:
        if (v->index == 7) return ffail(t, "secondary colour read");
        if (v->index != 6) return ffail(t, "varying other than the diffuse colour read");
        return fn_leaf(t, MAC_FF_SRC_PRIMARY, alpha, 0, 0);
    case V_ENV:
        if (v->index < 0 || v->index > MAC_NV2A_ARBFP_ENV_FINAL_C1) return ffail(t, "constant other than the combiner colours");
        return fn_leaf(t, MAC_FF_SRC_CONSTANT, alpha, 0, v->index);
    default:
        if (t->tex[s->val] >= 0) return fn_leaf(t, t->tex[s->val], alpha, 0, 0);
        n = t->tn[s->val][alpha];
        if (n < 0) return ffail(t, "value not computed by the combiner lowering");
        if (g == 0 && alpha && !fn_leafy(t, n)) return ffail(t, "computed alpha used as a colour scalar");
        return n;
    }
}

static int fn_sat(struct fftx *t, int i)
{
    struct fnode *n;
    if (i < 0) return -1;
    n = &t->n[i];
    if (n->lo >= 0.0f && n->hi <= 1.0f) return i;
    if (n->kind == FN_LIT) return fn_lit(t, clampf(n->lit, 0.0f, 1.0f));
    n->sat = 1; n->lo = clampf(n->lo, 0.0f, 1.0f); n->hi = clampf(n->hi, 0.0f, 1.0f);
    return i;
}

static int fn_make(struct fftx *t, int kind, int a, int b, int c, float lo, float hi, int sat)
{
    int i = fn_new(t, kind);
    if (i < 0) return -1;
    t->n[i].a = a; t->n[i].b = b; t->n[i].c = c;
    t->n[i].lo = lo; t->n[i].hi = hi;
    if (sat) fn_sat(t, i);
    return i;
}

static int fn_ins(struct fftx *t, const struct ins *n, int g)
{
    int a, b, c, na, nb, nc, i;
    struct fnode *A, *B, *C;
    float p[4];
    switch (n->op) {
    case OP_MOV:
        a = fn_src(t, &n->s[0], g, &na);
        if (a < 0) return -1;
        if (na) return ffail(t, "negated move");
        return n->sat ? fn_sat(t, a) : a;
    case OP_ADD:
        a = fn_src(t, &n->s[0], g, &na); b = fn_src(t, &n->s[1], g, &nb);
        if (a < 0 || b < 0) return -1;
        if (na && nb) return ffail(t, "two negated addends");
        if (fn_use(t, a) < 0 || fn_use(t, b) < 0) return -1;
        A = &t->n[a]; B = &t->n[b];
        if (!na && !nb) return fn_make(t, FN_ADD, a, b, -1, A->lo + B->lo, A->hi + B->hi, n->sat);
        if (na) { i = a; a = b; b = i; }   /* a - b */
        A = &t->n[a]; B = &t->n[b];
        if (A->kind == FN_LIT && A->lit == 1.0f && B->kind == FN_LEAF && !B->inv) {
            i = fn_leaf(t, B->src, B->alpha, 1, B->env);
            return n->sat ? fn_sat(t, i) : i;
        }
        return fn_make(t, FN_SUB, a, b, -1, A->lo - B->hi, A->hi - B->lo, n->sat);
    case OP_MUL:
        a = fn_src(t, &n->s[0], g, &na); b = fn_src(t, &n->s[1], g, &nb);
        if (a < 0 || b < 0) return -1;
        if (na || nb) return ffail(t, "negated factor");
        if ((t->n[a].kind == FN_LIT) != (t->n[b].kind == FN_LIT)) {
            /* x * 2 and x * 4 are the stage's scale (the clamp follows it) */
            int lit = t->n[a].kind == FN_LIT ? a : b, other = lit == a ? b : a;
            float k = t->n[lit].lit;
            if (k == 2.0f || k == 4.0f) {
                i = fn_make(t, FN_SCL, other, -1, -1, t->n[other].lo * k, t->n[other].hi * k, n->sat);
                if (i >= 0) t->n[i].lit = k;
                return i;
            }
        }
        if (fn_use(t, a) < 0 || fn_use(t, b) < 0) return -1;
        A = &t->n[a]; B = &t->n[b];
        p[0] = A->lo * B->lo; p[1] = A->lo * B->hi; p[2] = A->hi * B->lo; p[3] = A->hi * B->hi;
        return fn_make(t, FN_MUL, a, b, -1, fmin2(fmin2(p[0], p[1]), fmin2(p[2], p[3])),
            fmax2(fmax2(p[0], p[1]), fmax2(p[2], p[3])), n->sat);
    case OP_MAD:
        a = fn_src(t, &n->s[0], g, &na); b = fn_src(t, &n->s[1], g, &nb); c = fn_src(t, &n->s[2], g, &nc);
        if (a < 0 || b < 0 || c < 0) return -1;
        if (na || nb || nc) return ffail(t, "negated multiply-add operand");
        if (fn_use(t, a) < 0 || fn_use(t, b) < 0 || fn_use(t, c) < 0) return -1;
        if (t->n[c].kind == FN_MUL && ((t->n[c].a == a && t->n[c].b == b) || (t->n[c].a == b && t->n[c].b == a))) {
            /* a * b + a * b is the product with the stage's scale of 2 */
            i = fn_make(t, FN_SCL, c, -1, -1, t->n[c].lo * 2.0f, t->n[c].hi * 2.0f, n->sat);
            if (i >= 0) t->n[i].lit = 2.0f;
            return i;
        }
        {
            /* (1 - w) * x + w * k is an interpolation */
            int swap;
            for (swap = 0; swap < 2; swap++) {
                int ia = swap ? b : a, x = swap ? a : b, k;
                if (t->n[ia].kind == FN_LEAF && t->n[ia].inv && t->n[c].kind == FN_MUL) {
                    int side;
                    for (side = 0; side < 2; side++) {
                        int tl = side ? t->n[c].b : t->n[c].a;
                        k = side ? t->n[c].a : t->n[c].b;
                        if (t->n[tl].kind == FN_LEAF && !t->n[tl].inv && t->n[tl].src == t->n[ia].src &&
                            t->n[tl].alpha == t->n[ia].alpha && t->n[tl].env == t->n[ia].env) {
                            float lo = fmin2(t->n[k].lo, t->n[x].lo), hi = fmax2(t->n[k].hi, t->n[x].hi);
                            return fn_make(t, FN_LRP, tl, k, x, lo, hi, n->sat);
                        }
                    }
                }
            }
        }
        A = &t->n[a]; B = &t->n[b]; C = &t->n[c];
        p[0] = A->lo * B->lo; p[1] = A->lo * B->hi; p[2] = A->hi * B->lo; p[3] = A->hi * B->hi;
        return fn_make(t, FN_MAD, a, b, c, fmin2(fmin2(p[0], p[1]), fmin2(p[2], p[3])) + C->lo,
            fmax2(fmax2(p[0], p[1]), fmax2(p[2], p[3])) + C->hi, n->sat);
    case OP_LRP:
        a = fn_src(t, &n->s[0], g, &na); b = fn_src(t, &n->s[1], g, &nb); c = fn_src(t, &n->s[2], g, &nc);
        if (a < 0 || b < 0 || c < 0) return -1;
        if (na || nb || nc) return ffail(t, "negated interpolation operand");
        if (fn_use(t, a) < 0 || fn_use(t, b) < 0 || fn_use(t, c) < 0) return -1;
        return fn_make(t, FN_LRP, a, b, c, fmin2(t->n[b].lo, t->n[c].lo), fmax2(t->n[b].hi, t->n[c].hi), n->sat);
    default:
        return ffail(t, "instruction the combiners cannot express");
    }
}

/* ---- scheduling */

static void ff_default_stage(struct mac_nv2a_ff_stage *s)
{
    memset(s, 0, sizeof(*s));
    s->rgb_source[0] = s->a_source[0] = MAC_FF_SRC_PREVIOUS;
    s->rgb_operand[0] = MAC_FF_OP_COLOR; s->a_operand[0] = MAC_FF_OP_ALPHA;
    s->rgb_scale = s->a_scale = 1;
}

/* a stage's constant colour part wanted by an operand */
static int ff_demand(struct fftx *t, struct mac_nv2a_ff_stage *s, int group, int env, int literal, float value)
{
    unsigned char *kind = group ? &s->a_const_kind : &s->rgb_const_kind;
    unsigned char *e = group ? &s->a_env : &s->rgb_env;
    float *lv = group ? &s->a_literal : &s->rgb_literal;
    if (literal && (value < 0.0f || value > 1.0f)) return ffail(t, "literal outside [0,1]");
    if (*kind == MAC_FF_CONST_NONE) {
        *kind = literal ? MAC_FF_CONST_LITERAL : MAC_FF_CONST_ENV;
        if (literal) *lv = value; else *e = (unsigned char)env;
        return 0;
    }
    if (literal) { if (*kind == MAC_FF_CONST_LITERAL && *lv == value) return 0; }
    else if (*kind == MAC_FF_CONST_ENV && *e == env) return 0;
    return ffail(t, "two different constants in one stage");
}

static int ff_set_arg(struct fftx *t, struct mac_nv2a_ff_stage *s, int g, int k, int node, int previous)
{
    unsigned char *src = g ? s->a_source : s->rgb_source;
    unsigned char *op = g ? s->a_operand : s->rgb_operand;
    const struct fnode *n;
    if (previous) { src[k] = MAC_FF_SRC_PREVIOUS; op[k] = g ? MAC_FF_OP_ALPHA : MAC_FF_OP_COLOR; return 0; }
    n = &t->n[node];
    if (n->kind == FN_LIT) {
        src[k] = MAC_FF_SRC_CONSTANT; op[k] = g ? MAC_FF_OP_ALPHA : MAC_FF_OP_COLOR;
        return ff_demand(t, s, g, 0, 1, n->lit);
    }
    src[k] = (unsigned char)n->src;
    if (g && !n->alpha) return ffail(t, "colour operand in the alpha pipeline");
    op[k] = (unsigned char)((n->alpha ? MAC_FF_OP_ALPHA : MAC_FF_OP_COLOR) + (n->inv ? 1 : 0));
    if (n->src == MAC_FF_SRC_CONSTANT) return ff_demand(t, s, n->alpha, n->env, 0, 0.0f);
    return 0;
}

/* The chain of operations that computes `root` in pipeline g, one per stage,
   written into stages from the first stage that lets every operation read
   only textures of its own or earlier units (earlier stages pass the previous
   value on); the number of stages used is returned. A scale (a literal 2 or 4
   factor) is the scale of the stage that computes the factor's operand, unless
   that value was saturated before it was scaled. */
struct ffent { int node, scale, pass, top; };
/* 1: a stage may read the texture of a later unit (the crossbar allows it, the
   unit being enabled); 0: only its own and earlier units' (padding stages are
   put in front of the chain as needed) */
static int ff_forward_textures;
void mac_nv2a_arbfp_set_ff_forward(int on) { ff_forward_textures = on != 0; }
static int ff_chain(struct fftx *t, int root, int g, struct mac_nv2a_ff_stage *stages)
{
    struct ffent rev[MAC_FF_MAX_STAGES + 1], chain[MAC_FF_MAX_STAGES + 1];
    int args[MAC_FF_MAX_STAGES + 1][3], nargs[MAC_FF_MAX_STAGES + 1], func[MAC_FF_MAX_STAGES + 1];
    int count = 0, cur = root, scale = 1, i, k, first = 0, top = root, unit_need = 0;
    for (;;) {
        int child = -1, ids[3], nids, j;
        const struct fnode *nd = &t->n[cur];
        if (count > MAC_FF_MAX_STAGES) return ffail(t, "more combiner stages than texture units");
        if (nd->kind == FN_SCL) {
            int factor = (int)nd->lit, sub = nd->a;
            scale *= factor;
            if (scale > 4) return ffail(t, "scale beyond 4");
            if (fn_leafy(t, sub)) { rev[count].node = sub; rev[count].scale = scale; rev[count].top = top; rev[count++].pass = 0; break; }
            if (!t->n[sub].sat) { cur = sub; continue; }
            rev[count].node = -1; rev[count].scale = scale; rev[count].top = top; rev[count++].pass = 1;
            scale = 1; cur = sub; top = sub;
            continue;
        }
        rev[count].node = cur; rev[count].scale = scale; rev[count].top = top; rev[count++].pass = 0;
        scale = 1;
        if (fn_leafy(t, cur)) break;
        ids[0] = nd->a; ids[1] = nd->b; ids[2] = nd->c;
        nids = nd->kind == FN_MUL || nd->kind == FN_ADD || nd->kind == FN_SUB ? 2 : 3;
        for (j = 0; j < nids; j++) {
            if (fn_leafy(t, ids[j])) continue;
            if (child >= 0 && child != ids[j]) return ffail(t, "two computed operands in one operation");
            child = ids[j];
        }
        if (child < 0) break;
        cur = child; top = child;
    }
    if (count > MAC_FF_MAX_STAGES) return ffail(t, "more combiner stages than texture units");
    for (i = 0; i < count; i++) chain[i] = rev[count - 1 - i];
    for (i = 0; i < count; i++) {
        const struct fnode *n;
        if (chain[i].pass) { func[i] = MAC_FF_REPLACE; args[i][0] = -1; nargs[i] = 1; continue; }
        n = &t->n[chain[i].node];
        switch (n->kind) {
        case FN_LEAF: case FN_LIT: func[i] = MAC_FF_REPLACE; args[i][0] = chain[i].node; nargs[i] = 1; break;
        case FN_MUL: func[i] = MAC_FF_MODULATE; args[i][0] = n->a; args[i][1] = n->b; nargs[i] = 2; break;
        case FN_ADD: func[i] = MAC_FF_ADD; args[i][0] = n->a; args[i][1] = n->b; nargs[i] = 2; break;
        case FN_SUB: func[i] = MAC_FF_SUBTRACT; args[i][0] = n->a; args[i][1] = n->b; nargs[i] = 2; break;
        case FN_MAD:
            func[i] = MAC_FF_MODULATE_ADD; t->needs_combine3 = 1;
            args[i][0] = n->a; args[i][1] = n->c; args[i][2] = n->b; nargs[i] = 3; break;
        default:
            func[i] = MAC_FF_INTERPOLATE; args[i][0] = n->b; args[i][1] = n->c; args[i][2] = n->a; nargs[i] = 3; break;
        }
    }
    /* the first stage: operation i is at first + i and reads textures of units up to its stage */
    for (i = 0; i < count; i++)
        for (k = 0; k < nargs[i]; k++) {
            const struct fnode *a;
            if (args[i][k] < 0) continue;
            a = &t->n[args[i][k]];
            if (a->kind == FN_LEAF && a->src < 4) {
                if (!ff_forward_textures && a->src - i > first) first = a->src - i;
                if (a->src + 1 > unit_need) unit_need = a->src + 1;
            }
        }
    if (first + count > MAC_FF_MAX_STAGES) return ffail(t, "more combiner stages than texture units");
    for (i = 0; i < count; i++) {
        struct mac_nv2a_ff_stage *s = &stages[first + i];
        if (g) { s->a_function = (unsigned char)func[i]; s->a_scale = (unsigned char)chain[i].scale; }
        else { s->rgb_function = (unsigned char)func[i]; s->rgb_scale = (unsigned char)chain[i].scale; }
        for (k = 0; k < nargs[i]; k++)
            if (ff_set_arg(t, s, g, k, args[i][k], args[i][k] < 0 || (i > 0 && args[i][k] == chain[i - 1].top)) < 0) return -1;
    }
    return first + count > unit_need ? first + count : unit_need;
}

int mac_nv2a_arbfp_lower_fixed(const struct mac_nv2a_arbfp_state *state,
    const struct mac_nv2a_arb_layout *layout, unsigned long projective_mask,
    struct mac_nv2a_ff_program *out, char *error, size_t error_size)
{
    struct ctx *x = NULL;
    struct fftx *t;
    int result, i, g, rgb_stages, a_stages;
    if (out) memset(out, 0, sizeof(*out));
    if (!out) return -1;
    result = build_program(state, layout, projective_mask, &x, error, error_size);
    if (result != 1) return result;
    t = (struct fftx *)calloc(1, sizeof(*t));
    if (t) { t->tn = (int (*)[2])malloc(sizeof(int) * 2 * MAX_VALS); t->tex = (signed char *)malloc(MAX_VALS); }
    if (!t || !t->tn || !t->tex) {
        if (t) { free(t->tn); free(t->tex); free(t); }
        free(x);
        return -1;
    }
    t->x = x; t->s = state; t->out_rgb = t->out_a = -1;
    for (i = 0; i < MAX_VALS; i++) { t->tn[i][0] = t->tn[i][1] = -1; t->tex[i] = -1; }
    for (i = 0; i < x->nins && !t->failed; i++) {
        const struct ins *n = &x->ins[i];
        if (n->deleted || n->op == OP_COMMENT) continue;
        if (n->op == OP_TEX) { ffail(t, "unscaled coordinate fetch (non-projective)"); break; }
        if (n->op == OP_TXP) {
            const struct val *c = &x->vals[n->s[0].val];
            if (n->tex_target != 0 || n->tex_unit < 0 || n->tex_unit > 3 || state->sampler_type[n->tex_unit] != SAMPLER_2D ||
                c->kind != V_ATTR || c->index != 2 + n->tex_unit) { ffail(t, "texture fetch other than a unit's own 2D coordinates"); break; }
            t->tex[n->dst] = (signed char)n->tex_unit;
            continue;
        }
        /* the alpha test: MAD a, 255, +-0.5; FLR; ADD - reference; KIL */
        if (n->op == OP_MAD && n->nsrc == 3 && x->vals[n->s[1].val].kind == V_CONST && x->vals[n->s[1].val].c == 255.0f &&
            x->vals[n->s[2].val].kind == V_CONST && t->kil_state == 0 && n->mask == 8u && n->dst >= 0) {
            int neg;
            t->kil_node = fn_src(t, &n->s[0], 1, &neg);
            if (t->kil_node < 0 || neg) { ffail(t, "alpha test source"); break; }
            t->kil_off = x->vals[n->s[2].val].c;
            if (t->kil_off != 0.5f && t->kil_off != -0.5f) { ffail(t, "alpha test offset"); break; }
            t->kil_t1 = n->dst; t->kil_state = 1;
            continue;
        }
        if (t->kil_state == 1 && n->op == OP_FLR && n->s[0].val == t->kil_t1 && n->dst >= 0) { t->kil_t2 = n->dst; t->kil_state = 2; continue; }
        if (t->kil_state == 2 && n->op == OP_ADD && n->s[0].val == t->kil_t2 && n->dst >= 0 &&
            x->vals[n->s[1].val].kind == V_ENV && x->vals[n->s[1].val].index == MAC_NV2A_ARBFP_ENV_ALPHA_REFERENCE &&
            n->s[1].neg && n->s[1].comp == 0) { t->kil_t3 = n->dst; t->kil_state = 3; continue; }
        if (t->kil_state == 3 && n->op == OP_KIL && n->s[0].val == t->kil_t3) { t->kil_state = 4; continue; }
        if (n->op == OP_KIL || n->dst == DST_NONE) { ffail(t, "kill other than the alpha test"); break; }
        {
            int lanes = n->mask == 7u ? 1 : n->mask == 8u ? 2 : n->mask == 15u ? 3 : 0;
            if (!lanes) { ffail(t, "partial write"); break; }
            if (lanes == 3 && !(n->op == OP_MOV && n->s[0].sw && !n->s[0].sw[0])) { ffail(t, "whole-vector arithmetic"); break; }
            for (g = 0; g < 2; g++) {
                int node, neg;
                if (!(lanes & (1 << g))) continue;
                if (lanes == 3) {
                    struct src whole = n->s[0];
                    struct ins one = *n;
                    whole.sw = NULL; whole.comp = g ? 3 : -1;
                    one.s[0] = whole;
                    node = fn_ins(t, &one, g);
                } else node = fn_ins(t, n, g);
                (void)neg;
                if (node < 0) break;
                if (n->dst == DST_COLOR) { if (g) t->out_a = node; else t->out_rgb = node; }
                else t->tn[n->dst][g] = node;
            }
        }
    }
    if (!t->failed && (t->out_rgb < 0 || t->out_a < 0)) ffail(t, "no colour output");
    if (!t->failed && t->kil_state != 0 && t->kil_state != 4) ffail(t, "unrecognized alpha test");
    if (!t->failed && t->kil_state == 4 && t->kil_node != t->out_a) ffail(t, "alpha test of another value than the output");
    if (!t->failed) {
        for (i = 0; i < MAC_FF_MAX_STAGES; i++) ff_default_stage(&out->stage[i]);
        rgb_stages = ff_chain(t, t->out_rgb, 0, out->stage);
        a_stages = t->failed ? -1 : ff_chain(t, t->out_a, 1, out->stage);
        if (!t->failed) {
            out->stage_count = (unsigned)(rgb_stages > a_stages ? rgb_stages : a_stages);
            out->alpha_test = t->kil_state == 4;
            out->alpha_offset = t->kil_off;
            out->needs_combine3 = t->needs_combine3;
            out->unit_2d = t->unit_2d;
            out->ok = 1;
        }
    }
    result = t->failed ? 0 : 1;
    if (t->failed) set_error(error, error_size, t->message);
    free(t->tn); free(t->tex); free(t);
    free(x);
    return result;
}

#ifndef MAC_NV2A_ARBFP_STANDALONE
void mac_nv2a_arbfp_state_from_key(const struct nv2a_pixel_shader_key *key,
    struct mac_nv2a_arbfp_state *state)
{
    int n;
    memset(state, 0, sizeof(*state));
    for (n = 0; n < 8; n++) {
        state->rgb_inputs[n] = key->combiner_state[D3DRS_PSRGBINPUTS0 + n];
        state->alpha_inputs[n] = key->combiner_state[D3DRS_PSALPHAINPUTS0 + n];
        state->rgb_outputs[n] = key->combiner_state[D3DRS_PSRGBOUTPUTS0 + n];
        state->alpha_outputs[n] = key->combiner_state[D3DRS_PSALPHAOUTPUTS0 + n];
    }
    state->combiner_count = key->combiner_state[D3DRS_PSCOMBINERCOUNT];
    state->final_abcd = key->combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD];
    state->final_efg = key->combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG];
    state->compare_mode = key->combiner_state[D3DRS_PSCOMPAREMODE];
    state->dot_mapping = key->combiner_state[D3DRS_PSDOTMAPPING];
    state->input_texture = key->combiner_state[D3DRS_PSINPUTTEXTURE];
    state->texture_modes = key->texture_modes;
    for (n = 0; n < 4; n++) {
        state->sampler_type[n] = key->sampler_type[n];
        state->alpha_kill[n] = key->alpha_kill[n];
        state->color_sign[n] = key->color_sign[n];
    }
    state->alpha_test_function = key->alpha_test_function;
    state->fog_enable = key->fog_enable; state->fog_table_mode = key->fog_table_mode;
    state->count_samples = key->count_samples; state->coverage_alpha = key->coverage_alpha;
    state->point_threshold = key->point_threshold; state->per_pixel_lighting = key->per_pixel_lighting;
    state->alpha_test_samples = key->alpha_test_samples; state->source_blend = key->source_blend;
}

int mac_nv2a_arbfp_translate_info(const struct nv2a_pixel_shader_key *key,
    const struct mac_nv2a_arb_layout *layout, unsigned long projective_mask,
    char **arb_out, struct mac_nv2a_arbfp_info *info, char *error, size_t error_size)
{
    struct mac_nv2a_arbfp_state state;
    if (!key) return -1;
    mac_nv2a_arbfp_state_from_key(key, &state);
    return mac_nv2a_arbfp_translate_state(&state, layout, projective_mask, arb_out, info, error, error_size);
}

int mac_nv2a_arbfp_translate(const struct nv2a_pixel_shader_key *key,
    const struct mac_nv2a_arb_layout *layout, unsigned long projective_mask,
    char **arb_out, char *error, size_t error_size)
{
    return mac_nv2a_arbfp_translate_info(key, layout, projective_mask, arb_out, NULL, error, error_size);
}
#endif
