/* NV2A vertex program -> ARBvp1.0 translator for the legacy Mac renderer.

   Production form of the prototype in tools/mac_arb_retail_probe.c. The
   semantics follow port/linux/src/nv2a_vsh.c (the instruction decode, the
   write order, the clip-position capture and the epilogue) and the Mac
   adaptation of its GLSL (mac_glsl120.c: depth remap; mac_bone_gather.c: the
   six relative reads become attributes 10..15). See mac_nv2a_arb.h and
   docs/ppc-mac-arb-translator.md. */

/* This source owns transient compiler text, not game tracked allocations. */
#undef memset
#undef memcpy
#undef memcmp
#undef memmove
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef strlen
#undef strcmp
#undef strncmp
#undef strncpy
#undef strcpy
#undef strcat
#undef sprintf
#undef snprintf
#include "mac_nv2a_arb.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    MAC_NOP, MAC_MOV, MAC_MUL, MAC_ADD, MAC_MAD, MAC_DP3, MAC_DPH, MAC_DP4,
    MAC_DST, MAC_MIN, MAC_MAX, MAC_SLT, MAC_SGE, MAC_ARL
};
enum { ILU_NOP, ILU_MOV, ILU_RCP, ILU_RCC, ILU_RSQ, ILU_EXP, ILU_LOG, ILU_LIT };
enum { MUX_UNKNOWN, MUX_TEMPORARY, MUX_INPUT, MUX_CONSTANT };

#define CONSTANT_COUNT 192
#define SLOT_COUNT 8

/* the varying slot of each texcoord, in result.texcoord[] order */
static const char *const slot_names[SLOT_COUNT] = {
    "xB0", "xB1", "xT0", "xT1", "xT2", "xT3", "xD0", "xD1"
};
/* the ARB identifiers of the NV2A outputs of each slot */
static const char *const slot_outputs[SLOT_COUNT] = {
    "ob0", "ob1", "ot0", "ot1", "ot2", "ot3", "od0", "od1"
};
/* B0, B1, D0 and D1 are clamped to [0,1] by the GLSL path; T0..T3 are not */
static const int slot_clamped[SLOT_COUNT] = { 1, 1, 0, 0, 0, 0, 1, 1 };
/* the order in which a free component is looked for to carry the fog */
static const int fog_slot_order[SLOT_COUNT] = { 1, 0, 7, 6, 5, 4, 3, 2 };

/* ---------- text */

struct text {
    char *data;
    size_t used, capacity;
    int failed;
};

static void text_raw(struct text *t, const char *data, size_t length)
{
    if (t->failed) return;
    if (t->used + length + 1 > t->capacity) {
        size_t capacity = t->capacity ? t->capacity : 4096;
        char *grown;
        while (capacity < t->used + length + 1) capacity *= 2;
        grown = (char *)realloc(t->data, capacity);
        if (!grown) { t->failed = 1; return; }
        t->data = grown;
        t->capacity = capacity;
    }
    memcpy(t->data + t->used, data, length);
    t->used += length;
    t->data[t->used] = 0;
}

static void text_append(struct text *t, const char *format, ...)
{
    char line[512];
    va_list args;
    int n;
    if (t->failed) return;
    va_start(args, format);
    n = vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= sizeof(line)) { t->failed = 1; return; }
    text_raw(t, line, (size_t)n);
}

/* ---------- instruction decode */

static unsigned long field(const unsigned long *p, int word, int low, int count)
{
    return ((p[word] & 0xFFFFFFFFUL) >> low) & ((1UL << count) - 1);
}

struct operand {
    unsigned long negate, swizzle[4], index, mux;
};

struct inst {
    unsigned long mac, ilu, mac_mask, temporary, ilu_mask, out_mask;
    unsigned long out_register, out_address, out_from_ilu, relative;
    unsigned long input, constant, final;
    struct operand op[3];
};

static void decode_operand(const unsigned long *p, unsigned which, struct operand *f)
{
    if (which == 0) {
        f->negate = field(p, 1, 8, 1);
        f->swizzle[0] = field(p, 1, 6, 2); f->swizzle[1] = field(p, 1, 4, 2);
        f->swizzle[2] = field(p, 1, 2, 2); f->swizzle[3] = field(p, 1, 0, 2);
        f->index = field(p, 2, 28, 4); f->mux = field(p, 2, 26, 2);
    } else if (which == 1) {
        f->negate = field(p, 2, 25, 1);
        f->swizzle[0] = field(p, 2, 23, 2); f->swizzle[1] = field(p, 2, 21, 2);
        f->swizzle[2] = field(p, 2, 19, 2); f->swizzle[3] = field(p, 2, 17, 2);
        f->index = field(p, 2, 13, 4); f->mux = field(p, 2, 11, 2);
    } else {
        f->negate = field(p, 2, 10, 1);
        f->swizzle[0] = field(p, 2, 8, 2); f->swizzle[1] = field(p, 2, 6, 2);
        f->swizzle[2] = field(p, 2, 4, 2); f->swizzle[3] = field(p, 2, 2, 2);
        f->index = (field(p, 2, 0, 2) << 2) | field(p, 3, 30, 2);
        f->mux = field(p, 3, 28, 2);
    }
}

static void decode(const unsigned long *p, struct inst *in)
{
    unsigned which;
    in->mac = field(p, 1, 21, 4);
    in->ilu = field(p, 1, 25, 3);
    in->mac_mask = field(p, 3, 24, 4);
    in->temporary = field(p, 3, 20, 4);
    in->ilu_mask = field(p, 3, 16, 4);
    in->out_mask = field(p, 3, 12, 4);
    in->out_register = field(p, 3, 11, 1);
    in->out_address = field(p, 3, 3, 8);
    in->out_from_ilu = field(p, 3, 2, 1);
    in->relative = field(p, 3, 1, 1);
    in->final = field(p, 3, 0, 1);
    in->input = field(p, 1, 9, 4);
    in->constant = field(p, 1, 13, 8);
    for (which = 0; which < 3; ++which) decode_operand(p, which, &in->op[which]);
}

/* operands (bit 0 A, bit 1 B, bit 2 C) that a MAC operation reads */
static unsigned mac_uses(unsigned long mac)
{
    switch (mac) {
    case MAC_MOV: case MAC_ARL: return 1;
    case MAC_MUL: case MAC_DP3: case MAC_DPH: case MAC_DP4: case MAC_DST:
    case MAC_MIN: case MAC_MAX: case MAC_SLT: case MAC_SGE: return 3;
    case MAC_ADD: return 5;
    case MAC_MAD: return 7;
    default: return 0;
    }
}

/* the components of a write mask in the NV2A's order (bit 3 is x), as bits
   with x the lowest, and as a swizzle suffix */
static unsigned component_bits(unsigned long mask)
{
    return ((mask & 8) ? 1u : 0u) | ((mask & 4) ? 2u : 0u) |
        ((mask & 2) ? 4u : 0u) | ((mask & 1) ? 8u : 0u);
}

static void mask_text(unsigned long mask, char out[5])
{
    unsigned n = 0;
    if (mask & 8) out[n++] = 'x';
    if (mask & 4) out[n++] = 'y';
    if (mask & 2) out[n++] = 'z';
    if (mask & 1) out[n++] = 'w';
    out[n] = 0;
}

/* ---------- translation state */

struct context {
    struct inst ins[MAC_NV2A_ARB_MAX_INSTRUCTIONS];
    signed char rel[MAC_NV2A_ARB_MAX_INSTRUCTIONS][3]; /* gathered attribute, or -1 */
    unsigned long count;
    int gather;
    /* native relative addressing (MAC_NV2A_ARB_RELATIVE): the relative reads
       are cr[A0.x + (constant - a0_shift)], cr being program.env[0..191]; a0_low and a0_high bound
       the float that is loaded into A0 */
    int relative;
    long a0_shift, a0_add, a0_low, a0_high; /* a0_add: what is added to the float loaded into A0 (a0_shift, except in a window) */
    int window_lo, window_count, skin;
    /* the relative constant reads, in program order (every printed operand
       of an instruction with the relative bit, as the GLSL does) */
    unsigned read_count;
    unsigned long read_inst[8];
    unsigned read_which[8];
    unsigned char attr_used[16];
    unsigned char const_used[CONSTANT_COUNT];
    unsigned char temp_used[12];
    unsigned char temp_init[12];   /* read before fully written */
    unsigned char temp_defined[12]; /* components written so far (bits) */
    unsigned char slot_written[SLOT_COUNT];
    int fog_written, pts_written, clip_seen, uses_rcc, uses_wide;
    int ff;     /* MAC_NV2A_ARB_RELATIVE_FF: the fixed-function fragment layout */
    char error[160];
};

static int refuse(struct context *x, const char *message)
{
    strncpy(x->error, message, sizeof(x->error) - 1);
    x->error[sizeof(x->error) - 1] = 0;
    return 0;
}

static int swizzle_is(const struct operand *o, const char *letters)
{
    unsigned k;
    for (k = 0; k < 4; ++k)
        if (o->swizzle[k] != (unsigned long)(strchr("xyzw", letters[k]) - "xyzw")) return 0;
    return 1;
}

/* the text of operand `which` of instruction i: vector form (four swizzled
   components) or scalar form (the first component, for the ILU) */
static int operand_text(struct context *x, unsigned long i, unsigned which,
    int scalar, char *out)
{
    const struct inst *in = &x->ins[i];
    const struct operand *o = &in->op[which];
    char base[32], swizzle[8];
    unsigned k, reads = 0;
    base[0] = 0;
    switch (o->mux) {
    case MUX_TEMPORARY:
        if (o->index == 12) strcpy(base, "pos");
        else if (o->index < 12) { sprintf(base, "r%lu", o->index); x->temp_used[o->index] = 1; }
        else return refuse(x, "temporary register out of range");
        break;
    case MUX_INPUT:
        sprintf(base, "v%lu", in->input);
        x->attr_used[in->input] = 1;
        break;
    case MUX_CONSTANT:
        if (in->relative) {
            if (x->relative) {
                sprintf(base, "cr[A0.x + %ld]", (long)in->constant - x->a0_shift);
            } else {
                if (!x->gather || x->rel[i][which] < 0)
                    return refuse(x, "relative constant read without the bone gather");
                sprintf(base, "v%d", (int)x->rel[i][which]);
                x->attr_used[x->rel[i][which]] = 1;
            }
        } else {
            if (in->constant >= CONSTANT_COUNT) return refuse(x, "constant register out of range");
            sprintf(base, "c%lu", in->constant);
            x->const_used[in->constant] = 1;
        }
        break;
    default:
        /* an unconnected operand reads as zero */
        strcpy(base, "kc");
        break;
    }
    if (o->mux == MUX_UNKNOWN) {
        strcpy(swizzle, scalar ? "x" : "xxxx");
    } else {
        for (k = 0; k < (scalar ? 1u : 4u); ++k) {
            swizzle[k] = "xyzw"[o->swizzle[k]];
            reads |= 1u << o->swizzle[k];
        }
        swizzle[k] = 0;
    }
    if (o->mux == MUX_TEMPORARY && o->index < 12 && (reads & ~(unsigned)x->temp_defined[o->index]))
        x->temp_init[o->index] = 1;
    sprintf(out, "%s%s.%s", o->negate ? "-" : "", base, swizzle);
    return 1;
}

/* ---------- the bone gather recognizer

   Mirrors mac_bone_gather_rewrite() on the program words instead of the
   GLSL text, so that attributes 10..15 are used exactly where the GLSL path
   would use them and exactly when the same plan applies:
     - one product `r.xy = v5.xy * c[7].w` (MUL, no ILU, mask xy);
     - exactly two ARL instructions, the first (after the product) from
       r.x (the current form also moves v4.zzzz through the ILU, the legacy
       one does not), the second from r.y;
     - r.x and r.y are not written between the product and the second ARL;
     - exactly six relative reads, at offsets 60, 61, 62, 60, 61, 62, all
       xyzw, the first three after the first ARL and before the second, the
       last three after the second;
     - no ordinary operand reads attributes 10..15 (instruction operands
       are all printed by the GLSL, so used or not). */

static int is_product(const struct inst *in)
{
    return in->mac == MAC_MUL && in->ilu == ILU_NOP && !in->relative &&
        in->op[0].mux == MUX_INPUT && !in->op[0].negate && swizzle_is(&in->op[0], "xyzw") &&
        in->input == 5 &&
        in->op[1].mux == MUX_CONSTANT && !in->op[1].negate && swizzle_is(&in->op[1], "wwww") &&
        in->constant == 7 &&
        in->op[2].mux == MUX_INPUT && !in->op[2].negate && swizzle_is(&in->op[2], "xyzw");
}

static int is_address_load(const struct inst *in, unsigned long reg, const char *a_swizzle,
    unsigned long ilu, unsigned long input, const char *c_swizzle)
{
    return in->mac == MAC_ARL && in->ilu == ilu &&
        in->op[0].mux == MUX_TEMPORARY && in->op[0].index == reg && !in->op[0].negate &&
        swizzle_is(&in->op[0], a_swizzle) &&
        in->op[1].mux == MUX_INPUT && !in->op[1].negate && swizzle_is(&in->op[1], "xyzw") &&
        in->op[2].mux == MUX_INPUT && !in->op[2].negate && swizzle_is(&in->op[2], c_swizzle) &&
        in->input == input;
}

static int recognize_gather(struct context *x)
{
    static const unsigned long offsets[6] = { 60, 61, 62, 60, 61, 62 };
    unsigned long i, arl[3] = { 0, 0, 0 };
    unsigned long product = 0, reg;
    unsigned products = 0, arls = 0, k;
    for (i = 0; i < x->count; ++i) {
        const struct inst *in = &x->ins[i];
        if (is_product(in)) { ++products; product = i; }
        if (in->mac == MAC_ARL) { if (arls < 3) arl[arls] = i; ++arls; }
    }
    if (products != 1 || arls != 2 || x->read_count != 6) return 0;
    reg = x->ins[product].temporary;
    if (x->ins[product].mac_mask != 0xC || reg > 11 || arl[0] < product) return 0;
    if (!is_address_load(&x->ins[arl[0]], reg, "xyzw", ILU_MOV, 4, "zzzz") &&
        !is_address_load(&x->ins[arl[0]], reg, "xxxx", ILU_NOP, 0, "xyzw")) return 0;
    if (!is_address_load(&x->ins[arl[1]], reg, "yyyy", ILU_NOP, 0, "xyzw")) return 0;
    /* r.x and r.y keep the product from the product to the second ARL */
    for (i = product + 1; i < arl[1]; ++i) {
        const struct inst *in = &x->ins[i];
        if (in->mac != MAC_NOP && in->mac != MAC_ARL && in->temporary == reg && (in->mac_mask & 0xC)) return 0;
        if (in->ilu != ILU_NOP && (in->mac != MAC_NOP ? 1UL : in->temporary) == reg && (in->ilu_mask & 0xC)) return 0;
    }
    for (k = 0; k < 6; ++k) {
        const struct inst *in = &x->ins[x->read_inst[k]];
        if (in->constant != offsets[k] || !swizzle_is(&in->op[x->read_which[k]], "xyzw")) return 0;
        if (k < 3 ? (x->read_inst[k] <= arl[0] || x->read_inst[k] >= arl[1]) : x->read_inst[k] <= arl[1]) return 0;
    }
    for (i = 0; i < x->count; ++i)
        for (k = 0; k < 3; ++k)
            if (x->ins[i].op[k].mux == MUX_INPUT && x->ins[i].input >= 10) return 0;
    return 1;
}

/* ---------- operation text */

static void emit_mac(struct text *t, unsigned long op, const char *dest,
    const char *a, const char *b, const char *c)
{
    switch (op) {
    case MAC_MOV: text_append(t, "MOV %s, %s;\n", dest, a); break;
    case MAC_MUL: text_append(t, "MUL %s, %s, %s;\n", dest, a, b); break;
    case MAC_ADD: text_append(t, "ADD %s, %s, %s;\n", dest, a, c); break;
    case MAC_MAD: text_append(t, "MAD %s, %s, %s, %s;\n", dest, a, b, c); break;
    case MAC_DP3: text_append(t, "DP3 %s, %s, %s;\n", dest, a, b); break;
    case MAC_DPH: text_append(t, "DPH %s, %s, %s;\n", dest, a, b); break;
    case MAC_DP4: text_append(t, "DP4 %s, %s, %s;\n", dest, a, b); break;
    case MAC_DST: text_append(t, "DST %s, %s, %s;\n", dest, a, b); break;
    case MAC_MIN: text_append(t, "MIN %s, %s, %s;\n", dest, a, b); break;
    case MAC_MAX: text_append(t, "MAX %s, %s, %s;\n", dest, a, b); break;
    case MAC_SLT: text_append(t, "SLT %s, %s, %s;\n", dest, a, b); break;
    case MAC_SGE: text_append(t, "SGE %s, %s, %s;\n", dest, a, b); break;
    default: t->failed = 1; break;
    }
}

/* The ILU operations into the ARB register `u` (all four components, as the
   GLSL's `ilu`), from the vector operand c and its first component cs.
   Scratch registers t0..t3 are free here. kr, kx and kc are the constants
   declared in the program. */
static void emit_ilu(struct text *t, unsigned long op, const char *u,
    const char *c, const char *cs)
{
    switch (op) {
    case ILU_MOV:
        text_append(t, "MOV %s, %s;\n", u, c);
        break;
    case ILU_RCP:
        text_append(t, "RCP %s, %s;\n", u, cs);
        break;
    case ILU_RCC:
        /* nv2a_rcc: 1/x clamped to +-[5.42101e-20, 1.884467e+19] by its
           sign. The sign selects by 0/1 masks of exact products, so no
           infinity or NaN is multiplied by a zero except the reciprocal of
           a NaN, which no path defines. */
        text_append(t, "RCP %s.x, %s;\n", u, cs);
        text_append(t, "SLT t0.x, kc.x, %s.x;\n", u);
        text_append(t, "SGE t0.y, kc.x, %s.x;\n", u);
        text_append(t, "MAX t1.x, %s.x, kr.x;\nMIN t1.x, t1.x, kr.y;\n", u);
        text_append(t, "MAX t1.y, %s.x, kr.w;\nMIN t1.y, t1.y, kr.z;\n", u);
        text_append(t, "MUL t1.x, t1.x, t0.x;\nMAD %s.x, t1.y, t0.y, t1.x;\n", u);
        text_append(t, "MOV %s, %s.xxxx;\n", u, u);
        break;
    case ILU_RSQ:
        /* inversesqrt(abs(x)): ARB's RSQ takes the absolute value too */
        text_append(t, "RSQ %s, %s;\n", u, cs);
        break;
    case ILU_EXP:
        /* nv2a_exp: (exp2(floor(x)), fract(x), exp2(x), 1), with EX2 rather
           than ARB's approximate EXP */
        text_append(t, "FLR t0.x, %s;\n", cs);
        text_append(t, "EX2 %s.x, t0.x;\nSUB %s.y, %s, t0.x;\nEX2 %s.z, %s;\nMOV %s.w, kc.z;\n",
            u, u, cs, u, cs, u);
        break;
    case ILU_LOG:
        /* nv2a_log: with x = abs(x), (-1e30, 1, -1e30, 1) for zero, else
           (floor(log2 x), x / exp2(floor(log2 x)), log2 x, 1). Zero is
           replaced by 1 for the computation and its result selected by 0/1
           masks, so no infinity meets a zero. */
        text_append(t, "ABS t0.x, %s;\nSGE t1.x, kc.x, t0.x;\nADD t0.x, t0.x, t1.x;\n", cs);
        text_append(t, "LG2 t2.x, t0.x;\nFLR t3.x, t2.x;\nEX2 t0.y, t3.x;\nRCP t0.y, t0.y;\n");
        text_append(t, "MUL t0.y, t0.x, t0.y;\nSUB t1.y, kc.z, t1.x;\n");
        text_append(t, "MUL %s.x, t3.x, t1.y;\nMAD %s.x, t1.x, kx.w, %s.x;\n", u, u, u);
        text_append(t, "MUL %s.y, t0.y, t1.y;\nADD %s.y, %s.y, t1.x;\n", u, u, u);
        text_append(t, "MUL %s.z, t2.x, t1.y;\nMAD %s.z, t1.x, kx.w, %s.z;\nMOV %s.w, kc.z;\n", u, u, u, u);
        break;
    case ILU_LIT:
        /* nv2a_lit: (1, max(x,0), x > 0 ? pow(max(y,0), clamp(w, +-127.9961)) : 0, 1)
           with pow as exp2(w * log2(y)); the power is held below the
           largest float before the x > 0 mask, and log2 of zero held at
           -1e30, so that neither the mask nor the exponent meets an
           infinity (pow(0, 0) is 1) */
        text_append(t, "MOV t0, %s;\n", c);
        text_append(t, "SLT t1.x, kc.x, t0.x;\nMAX t1.y, t0.y, kc.x;\n");
        text_append(t, "MAX t1.w, t0.w, kx.y;\nMIN t1.w, t1.w, kx.x;\n");
        text_append(t, "LG2 t2.x, t1.y;\nMAX t2.x, t2.x, kx.w;\nMUL t2.x, t2.x, t1.w;\nEX2 t2.x, t2.x;\n");
        text_append(t, "MIN t2.x, t2.x, kx.z;\nMUL t2.x, t2.x, t1.x;\n");
        text_append(t, "MOV %s, kc.zzzz;\nMAX %s.y, t0.x, kc.x;\nMOV %s.z, t2.x;\n", u, u, u);
        break;
    default:
        t->failed = 1;
        break;
    }
}

/* ---------- translation */

/* the NV2A output register of an address, as the slot it feeds: 0..7 a
   varying slot, 8 oPos, 9 oFog, 10 oPts, -1 not an output the GLSL uses */
static int output_target(unsigned long address)
{
    switch (address) {
    case 0: return 8;
    case 3: return 6;
    case 4: return 7;
    case 5: return 9;
    case 6: return 10;
    case 7: return 0;
    case 8: return 1;
    case 9: return 2;
    case 10: return 3;
    case 11: return 4;
    case 12: return 5;
    default: return -1;
    }
}

static void temp_name(unsigned long index, char *out)
{
    if (index == 12) strcpy(out, "pos");
    else sprintf(out, "r%lu", index);
}

static void define_temporary(struct context *x, unsigned long index, unsigned bits)
{
    if (index < 12) { x->temp_defined[index] = (unsigned char)(x->temp_defined[index] | bits); x->temp_used[index] = 1; }
}

static int emit_instructions(struct context *x, struct text *body)
{
    unsigned long i;
    for (i = 0; i < x->count && !body->failed; ++i) {
        const struct inst *in = &x->ins[i];
        char a[96], b[96], c[96], cs[96], mask[5], tdest[24], idest[24], odest[32], name[16];
        unsigned uses = 0, mac_bits = component_bits(in->mac_mask);
        unsigned ilu_bits = component_bits(in->ilu_mask), out_bits = component_bits(in->out_mask);
        int arl = in->mac == MAC_ARL;
        int temp_write = in->mac != MAC_NOP && !arl && in->mac_mask != 0;
        int ilu_write = in->ilu != ILU_NOP && in->ilu_mask != 0;
        int target = -1, out_active = 0, need_mac, need_ilu, direct_mac, direct_ilu;
        unsigned long ilu_temporary = in->mac != MAC_NOP ? 1UL : in->temporary;

        if (in->mac > MAC_ARL) return refuse(x, "unsupported MAC operation");
        if (temp_write && in->temporary > 12) return refuse(x, "temporary register out of range");
        if (ilu_write && ilu_temporary > 12) return refuse(x, "temporary register out of range");
        if (in->out_mask && in->out_register &&
            (in->out_from_ilu ? in->ilu != ILU_NOP : in->mac != MAC_NOP)) {
            target = output_target(in->out_address);
            out_active = target >= 0;
        }
        need_mac = temp_write || (out_active && !in->out_from_ilu) || (arl && x->relative);
        need_ilu = ilu_write || (out_active && in->out_from_ilu);

        /* the screen-space conversion multiplies by rcc(w) of r12: keep the
           position it converts (before this instruction writes anything) */
        if (in->ilu == ILU_RCC && in->op[2].mux == MUX_TEMPORARY && in->op[2].index == 12) {
            text_append(body, "MOV clip, pos;\n");
            x->clip_seen = 1;
        }

        if (need_mac) uses |= mac_uses(in->mac);
        if (need_ilu) uses |= 4;
        strcpy(a, "kc.xxxx"); strcpy(b, "kc.xxxx"); strcpy(c, "kc.xxxx"); strcpy(cs, "kc.x");
        if ((uses & 1) && !operand_text(x, i, 0, 0, a)) return 0;
        if ((uses & 2) && !operand_text(x, i, 1, 0, b)) return 0;
        if ((uses & 4) && (!operand_text(x, i, 2, 0, c) || !operand_text(x, i, 2, 1, cs))) return 0;
        if (need_ilu) {
            if (in->ilu == ILU_RCC) x->uses_rcc = 1;
            if (in->ilu == ILU_LOG || in->ilu == ILU_LIT) x->uses_wide = 1;
        }

        if (temp_write) { mask_text(in->mac_mask, mask); temp_name(in->temporary, name); sprintf(tdest, "%s.%s", name, mask); }
        if (ilu_write) { mask_text(in->ilu_mask, mask); temp_name(ilu_temporary, name); sprintf(idest, "%s.%s", name, mask); }
        if (out_active) {
            mask_text(in->out_mask, mask);
            if (target < SLOT_COUNT) { strcpy(name, slot_outputs[target]); x->slot_written[target] = (unsigned char)(x->slot_written[target] | out_bits); }
            else if (target == 8) strcpy(name, "pos");
            else if (target == 9) { strcpy(name, "fog"); x->fog_written = 1; }
            else { strcpy(name, "pts"); x->pts_written = 1; }
            sprintf(odest, "%s.%s", name, mask);
        }

        direct_mac = temp_write && !(out_active && !in->out_from_ilu) && !need_ilu;
        direct_ilu = ilu_write && !(out_active && in->out_from_ilu) && !need_mac &&
            (in->ilu == ILU_MOV || in->ilu == ILU_RCP || in->ilu == ILU_RSQ);
        if (direct_mac) {
            emit_mac(body, in->mac, tdest, a, b, c);
        } else if (direct_ilu) {
            if (in->ilu == ILU_MOV) text_append(body, "MOV %s, %s;\n", idest, c);
            else text_append(body, "%s %s, %s;\n", in->ilu == ILU_RCP ? "RCP" : "RSQ", idest, cs);
        } else {
            /* both units read the state before either writes */
            if (need_mac) {
                if (arl) text_append(body, "MOV mac, %s;\n", a);
                else emit_mac(body, in->mac, "mac", a, b, c);
            }
            if (need_ilu) emit_ilu(body, in->ilu, "ilu", c, cs);
            if (temp_write) text_append(body, "MOV %s, mac;\n", tdest);
            if (ilu_write) text_append(body, "MOV %s, ilu;\n", idest);
            if (out_active) text_append(body, "MOV %s, %s;\n", odest, in->out_from_ilu ? "ilu" : "mac");
            if (arl && x->relative) {
                /* a0 = int(floor(mac.x + 0.001)), after both units have read
                   their inputs; the float is held within the range that
                   keeps every read inside c[0..191] (floor of a clamped
                   float is the clamped floor, the bounds being integers) */
                text_append(body, "ADD t0.x, mac.x, ka.x;\n");
                if (x->a0_add) text_append(body, "ADD t0.x, t0.x, ka.y;\n");
                text_append(body, "MAX t0.x, t0.x, ka.z;\nMIN t0.x, t0.x, ka.w;\nARL A0.x, t0.x;\n");
            }
        }
        if (temp_write) define_temporary(x, in->temporary, mac_bits);
        if (ilu_write) define_temporary(x, ilu_temporary, ilu_bits);
        if (in->final) break;
    }
    return !body->failed;
}

/* the position, as nv2a_vsh.c's epilogue computes it and the Mac GLSL's depth
   remap finishes it. t0..t3, mac and ilu are free here. */
static void emit_position(struct context *x, struct text *t)
{
    /* a zero scale component stands for 1; the division is by reciprocal
       (the GLSL divides): exact for the powers of two the game's viewports use */
    text_append(t, "MOV t0, vscale;\nABS t1, t0;\nSLT t1, kc.x, t1;\nSUB t2, kc.z, t1;\nMAD t0, t0, t1, t2;\n");
    text_append(t, "RCP t1.x, t0.x;\nRCP t1.y, t0.y;\nRCP t1.z, t0.z;\n");
    /* 0.5 + screen_offset: Direct3D 8 puts pixel centres on integers */
    text_append(t, "ADD mac.x, sp.x, kc.y;\n");
    if (x->clip_seen) {
        /* (clip.xyz * c[58].xyz + (c[59].xyz + (0.5 + screen_offset, 0.5, 0) - viewport_offset.xyz) * clip.w) / scale */
        x->const_used[58] = 1; x->const_used[59] = 1;
        text_append(t, "MOV t2, c59;\nADD t2.x, t2.x, mac.x;\nADD t2.y, t2.y, kc.y;\n");
        text_append(t, "MOV t3, voff;\nSUB t2.xyz, t2, t3;\nMUL t2.xyz, t2, clip.w;\n");
        text_append(t, "MUL t3.xyz, clip, c58;\nADD t3.xyz, t3, t2;\nMUL t3.xyz, t3, t1;\nMOV t3.w, clip.w;\n");
    } else {
        /* ((oPos.xy + (0.5 + screen_offset, 0.5), oPos.z) - viewport_offset.xyz) / scale * oPos.w */
        text_append(t, "MOV t3, pos;\nADD t3.x, t3.x, mac.x;\nADD t3.y, t3.y, kc.y;\n");
        text_append(t, "MOV t2, voff;\nSUB t3.xyz, t3, t2;\nMUL t3.xyz, t3, t1;\nMUL t3.xyz, t3, pos.w;\nMOV t3.w, pos.w;\n");
    }
    /* a w of zero or NaN gives (0,0,0,-1) before the depth remap, so
       (0,0,1,-1) after it */
    text_append(t, "ABS t0.x, t3.w;\nSLT t0.x, kc.x, t0.x;\nSUB t0.y, kc.z, t0.x;\n");
    text_append(t, "MAD t3.z, t3.z, kc.w, -t3.w;\n");
    text_append(t, "MUL t2, t3, t0.x;\nMAD result.position, kg, t0.y, t2;\n");
}

/* (MAC_NV2A_ARB_RELATIVE_FF) Where a slot's value goes for a fixed-function
   fragment stage: the textures' coordinates T0..T3 to texture unit 0..3's
   texcoord, D0 and D1 to the primary and secondary colour. B0, B1 and the
   fog have no place and are not written. The values are those of the usual
   layout (D0 and D1 clamped to [0,1], as the colour outputs are). */
static const char *ff_slot_output(unsigned slot, char *buffer)
{
    if (slot >= 2 && slot <= 5) { sprintf(buffer, "result.texcoord[%u]", slot - 2); return buffer; }
    if (slot == 6) return "result.color.primary";
    if (slot == 7) return "result.color.secondary";
    return NULL;
}

static void emit_varyings(struct context *x, struct text *t, int fog_slot, int fog_component)
{
    unsigned slot;
    const char *fog = x->fog_written ? "fog.x" : "kc.z";
    char name[32];
    if (x->ff) {
        for (slot = 2; slot < SLOT_COUNT; ++slot) {
            const char *out = ff_slot_output(slot, name);
            if (slot_clamped[slot]) {
                if (x->slot_written[slot]) {
                    text_append(t, "MAX t0, %s, kc.x;\nMIN %s, t0, kc.z;\n", slot_outputs[slot], out);
                } else text_append(t, "MOV %s, kd;\n", out);
            } else if (!x->slot_written[slot]) text_append(t, "MOV %s, kd;\n", out);
        }
        text_append(t, "MOV result.pointsize.x, %s;\n", x->pts_written ? "pts.x" : "sp.y");
        return;
    }
    for (slot = 0; slot < SLOT_COUNT; ++slot) {
        int stolen = fog_slot == (int)slot;
        char comp = "xyzw"[fog_component < 0 ? 0 : fog_component];
        if (slot_clamped[slot]) {
            if (x->slot_written[slot]) {
                text_append(t, "MAX t0, %s, kc.x;\n", slot_outputs[slot]);
                if (stolen) {
                    text_append(t, "MIN t0, t0, kc.z;\nMOV t0.%c, %s;\nMOV result.texcoord[%u], t0;\n", comp, fog, slot);
                } else {
                    text_append(t, "MIN result.texcoord[%u], t0, kc.z;\n", slot);
                }
            } else {
                text_append(t, "MOV result.texcoord[%u], kd;\n", slot);
                if (stolen) text_append(t, "MOV result.texcoord[%u].%c, %s;\n", slot, comp, fog);
            }
        } else if (x->slot_written[slot]) {
            if (stolen) text_append(t, "MOV %s.%c, %s;\n", slot_outputs[slot], comp, fog);
        } else {
            text_append(t, "MOV result.texcoord[%u], kd;\n", slot);
            if (stolen) text_append(t, "MOV result.texcoord[%u].%c, %s;\n", slot, comp, fog);
        }
    }
    text_append(t, "MOV result.pointsize.x, %s;\n", x->pts_written ? "pts.x" : "sp.y");
}

int mac_nv2a_arb_translate(const unsigned long *instructions,
    unsigned long instruction_count, int bone_gather, char **arb_source_out,
    unsigned *env_parameter_count_out, struct mac_nv2a_arb_layout *layout_out,
    char *error, size_t error_size)
{
    struct context *x;
    struct text head, body, tail;
    unsigned long i, count = 0;
    unsigned k, slot, bit;
    int fog_slot = -1, fog_component = 0, result = 0, gather_used = 0, relative_used = 0;
    long relative_min = 0, relative_max = 0;
    unsigned skin_base = 0, skin_top = 0;
    char *program;
    int window = bone_gather >> 8;
    int ff;

    bone_gather &= 0xff;
    ff = bone_gather == MAC_NV2A_ARB_RELATIVE_FF;

    if (ff) bone_gather = MAC_NV2A_ARB_RELATIVE;
    if (error && error_size) error[0] = 0;
    if (arb_source_out) *arb_source_out = NULL;
    if (env_parameter_count_out) *env_parameter_count_out = 0;
    if (layout_out) memset(layout_out, 0, sizeof(*layout_out));
    if (!instructions || !instruction_count || !arb_source_out) {
        if (error && error_size) { strncpy(error, "invalid argument", error_size - 1); error[error_size - 1] = 0; }
        return -1;
    }
    x = (struct context *)calloc(1, sizeof(*x));
    if (!x) return -1;
    head.data = body.data = tail.data = NULL;
    head.used = head.capacity = body.used = body.capacity = tail.used = tail.capacity = 0;
    head.failed = body.failed = tail.failed = 0;

    /* to the instruction that ends the program, as the GLSL translator */
    for (count = 0; count < instruction_count;) {
        if (field(instructions + count++ * 4, 3, 0, 1)) break;
    }
    if (count > MAC_NV2A_ARB_MAX_INSTRUCTIONS) { refuse(x, "program longer than the NV2A's memory"); goto done; }
    x->count = count;
    x->gather = 0;
    x->ff = ff;
    memset(x->rel, -1, sizeof(x->rel));
    for (i = 0; i < count; ++i) {
        decode(instructions + i * 4, &x->ins[i]);
        if (x->ins[i].relative) {
            for (k = 0; k < 3; ++k) {
                if (x->ins[i].op[k].mux != MUX_CONSTANT) continue;
                if (bone_gather == MAC_NV2A_ARB_RELATIVE) {
                    long offset = (long)x->ins[i].constant;
                    if (!x->read_count || offset < relative_min) relative_min = offset;
                    if (!x->read_count || offset > relative_max) relative_max = offset;
                }
                if (x->read_count < 6) {
                    x->rel[i][k] = (signed char)(10 + x->read_count);
                    x->read_inst[x->read_count] = i;
                    x->read_which[x->read_count] = k;
                }
                ++x->read_count;
            }
        }
    }
    if (bone_gather == MAC_NV2A_ARB_RELATIVE) {
        if (x->read_count) {
            /* the immediate must lie in [-64, 63]: when the highest offset
               is above 63 the lowest is folded into the address instead
               (floor(v) + s == floor(v + s) for an integer s) */
            if (relative_max > CONSTANT_COUNT - 1) { refuse(x, "relative constant offset out of range"); goto done; }
            x->a0_shift = relative_max > 63 ? relative_min : 0;
            x->window_lo = 0; x->window_count = CONSTANT_COUNT;
            /* port: a skinning program (the bone gather's pattern) reads the node matrices
               only, the registers from relative_min up: with a window the array is
               program.env[relative_min .. relative_min + window - 1], so that the
               driver is not told the program reads all 192 (the caller asks for a window
               only when no vertex of the draw names a node past it) */
            x->skin = recognize_gather(x);
            if (window) {
                if (!x->skin || x->a0_shift || relative_min < 0 || window < relative_max - relative_min + 1 ||
                    relative_min + window > CONSTANT_COUNT) { refuse(x, "no such window"); goto done; }
                x->window_lo = (int)relative_min; x->window_count = window;
                /* the immediates are made relative to the window (c - first), the address is not moved:
                   no negative address reaches the vertex unit */
                x->a0_shift = relative_min;
            }
            if (relative_max - x->a0_shift > 63 || relative_min - x->a0_shift < -64) { refuse(x, "relative constant offsets span more than 64 registers"); goto done; }
            x->a0_add = x->a0_shift - x->window_lo; /* (the window's first register is index 0) */
            x->a0_low = x->a0_shift - relative_min;
            x->a0_high = (x->window_count - 1) - relative_max + x->a0_shift;
            x->relative = 1;
            skin_base = x->skin ? (unsigned)relative_min : 0; skin_top = x->skin ? (unsigned)relative_max : 0;
            relative_used = 1;
        }
    } else if (bone_gather && x->read_count) {
        if (!recognize_gather(x)) { refuse(x, "relative constant reads do not match the bone gather pattern"); goto done; }
        x->gather = 1;
        gather_used = 1;
    }
    if (!emit_instructions(x, &body)) {
        if (body.failed) { refuse(x, "out of memory"); result = -1; }
        goto done;
    }
    emit_position(x, &tail);

    /* oFog.x travels in a texcoord component the program never writes */
    if (x->fog_written && !x->ff) {
        for (k = 0; k < SLOT_COUNT && fog_slot < 0; ++k) {
            slot = (unsigned)fog_slot_order[k];
            for (bit = 4; bit-- > 0;) {
                if (!(x->slot_written[slot] & (1u << bit))) { fog_slot = (int)slot; fog_component = (int)bit; break; }
            }
        }
        if (fog_slot < 0) { refuse(x, "no free varying component for the fog"); goto done; }
    }
    emit_varyings(x, &tail, fog_slot, fog_component);
    text_append(&tail, "END\n");

    text_append(&head, "!!ARBvp1.0\n");
    for (k = 0; k < 16; ++k)
        if (x->attr_used[k]) text_append(&head, "ATTRIB v%u = vertex.attrib[%u];\n", k, k);
    for (k = 0; k < CONSTANT_COUNT; ++k)
        if (x->const_used[k]) text_append(&head, "PARAM c%u = program.env[%u];\n", k, k);
    text_append(&head, "PARAM vscale = program.env[%d];\nPARAM voff = program.env[%d];\nPARAM sp = program.env[%d];\n",
        MAC_NV2A_ARB_ENV_VIEWPORT_SCALE, MAC_NV2A_ARB_ENV_VIEWPORT_OFFSET, MAC_NV2A_ARB_ENV_SCREEN_POINT);
    text_append(&head, "PARAM kc = { 0.0, 0.5, 1.0, 2.0 };\nPARAM kd = { 0.0, 0.0, 0.0, 1.0 };\nPARAM kg = { 0.0, 0.0, 1.0, -1.0 };\n");
    if (x->uses_rcc) text_append(&head, "PARAM kr = { 5.42101e-20, 1.884467e+19, -5.42101e-20, -1.884467e+19 };\n");
    if (x->relative) text_append(&head, "PARAM cr[%d] = { program.env[%d..%d] };\nPARAM ka = { 0.001, %ld.0, %ld.0, %ld.0 };\nADDRESS A0;\n",
        x->window_count, x->window_lo, x->window_lo + x->window_count - 1, x->a0_add, x->a0_low, x->a0_high);
    if (x->uses_wide) text_append(&head, "PARAM kx = { 127.9961, -127.9961, 3.4028234e+38, -1.0e+30 };\n");
    for (k = 0; k < 12; ++k)
        if (x->temp_used[k]) text_append(&head, "TEMP r%u;\n", k);
    text_append(&head, "TEMP pos, mac, ilu, t0, t1, t2, t3;\n");
    if (x->clip_seen) text_append(&head, "TEMP clip;\n");
    if (x->fog_written) text_append(&head, "TEMP fog;\n");
    if (x->pts_written) text_append(&head, "TEMP pts;\n");
    for (slot = 0; slot < SLOT_COUNT; ++slot) {
        if (!x->slot_written[slot]) continue;
        if (slot_clamped[slot]) text_append(&head, "TEMP %s;\n", slot_outputs[slot]);
        else if (x->ff) text_append(&head, "OUTPUT %s = result.texcoord[%u];\n", slot_outputs[slot], slot - 2);
        else text_append(&head, "OUTPUT %s = result.texcoord[%u];\n", slot_outputs[slot], slot);
    }
    /* the registers start as the GLSL's: temporaries zero, outputs (0,0,0,1),
       fog 1, point size the point_size uniform; only those that are read
       before they are written, or written in part, need it */
    for (k = 0; k < 12; ++k)
        if (x->temp_init[k]) text_append(&head, "MOV r%u, kc.x;\n", k);
    if (x->relative) text_append(&head, "ARL A0.x, kc.x;\n");
    text_append(&head, "MOV pos, kd;\n");
    for (slot = 0; slot < SLOT_COUNT; ++slot)
        if (x->slot_written[slot]) text_append(&head, "MOV %s, kd;\n", slot_outputs[slot]);
    if (x->fog_written) text_append(&head, "MOV fog, kc.z;\n");
    if (x->pts_written) text_append(&head, "MOV pts, sp.y;\n");

    if (head.failed || body.failed || tail.failed) { refuse(x, "out of memory"); result = -1; goto done; }
    program = (char *)malloc(head.used + body.used + tail.used + 1);
    if (!program) { refuse(x, "out of memory"); result = -1; goto done; }
    memcpy(program, head.data, head.used);
    memcpy(program + head.used, body.data, body.used);
    memcpy(program + head.used + body.used, tail.data, tail.used + 1);
    *arb_source_out = program;
    if (env_parameter_count_out) *env_parameter_count_out = MAC_NV2A_ARB_ENV_COUNT;
    if (layout_out) {
        layout_out->env_count = MAC_NV2A_ARB_ENV_COUNT;
        layout_out->bone_gather_used = gather_used;
        layout_out->skin_base = skin_base;
        layout_out->skin_top = skin_top;
        layout_out->relative_used = relative_used;
        layout_out->fog_texcoord = fog_slot;
        layout_out->fog_component = fog_slot < 0 ? 0 : fog_component;
    }
    result = 1;
done:
    if (result != 1 && error && error_size) {
        strncpy(error, x->error[0] ? x->error : "out of memory", error_size - 1);
        error[error_size - 1] = 0;
    }
    free(head.data); free(body.data); free(tail.data); free(x);
    return result;
}

/* ---------- the fragment side */

static const char default_component[4] = { '0', '0', '0', '1' };

int mac_nv2a_arb_fragment_prelude(const struct mac_nv2a_arb_layout *layout, char *out, size_t out_size)
{
    struct text t;
    unsigned slot;
    int ok;
    t.data = NULL; t.used = t.capacity = 0; t.failed = 0;
    for (slot = 0; slot < SLOT_COUNT; ++slot) {
        if (layout && layout->fog_texcoord == (int)slot) {
            unsigned k;
            text_append(&t, "#define %s (vec4(", slot_names[slot]);
            for (k = 0; k < 4; ++k) {
                /* the component that carried the fog reads as its default; the others are the slot's */
                if ((int)k == layout->fog_component) text_append(&t, "%s%c.0", k ? ", " : "", default_component[k]);
                else text_append(&t, "%sgl_TexCoord[%u].%c", k ? ", " : "", slot, "xyzw"[k]);
            }
            text_append(&t, "))\n");
        } else {
            text_append(&t, "#define %s gl_TexCoord[%u]\n", slot_names[slot], slot);
        }
    }
    if (layout && layout->fog_texcoord >= 0)
        text_append(&t, "#define xFog (gl_TexCoord[%d].%c)\n", layout->fog_texcoord, "xyzw"[layout->fog_component & 3]);
    else
        text_append(&t, "#define xFog 1.0\n");
    ok = !t.failed && t.used < out_size;
    if (ok) memcpy(out, t.data, t.used + 1);
    free(t.data);
    return ok;
}

static int declares(const char *line, size_t length, const char *type, const char *name)
{
    char text[64];
    size_t n = (size_t)sprintf(text, "varying %s %s;", type, name);
    while (length && (line[length - 1] == ' ' || line[length - 1] == '\t' || line[length - 1] == '\r')) --length;
    return length == n && !memcmp(line, text, n);
}

int mac_nv2a_arb_fragment_rewrite(const char *fragment_source,
    const struct mac_nv2a_arb_layout *layout, char **rewritten_out,
    char *error, size_t error_size)
{
    static const char version[] = "#version 120\n";
    struct text t;
    char prelude[1024];
    const char *line;
    unsigned slot;

    if (rewritten_out) *rewritten_out = NULL;
    if (error && error_size) error[0] = 0;
    if (!fragment_source || !rewritten_out || !layout) return -1;
    if (strncmp(fragment_source, version, sizeof(version) - 1) ||
        !mac_nv2a_arb_fragment_prelude(layout, prelude, sizeof(prelude))) {
        if (error && error_size) { strncpy(error, "fragment source is not GLSL 1.20 text", error_size - 1); error[error_size - 1] = 0; }
        return 0;
    }
    t.data = NULL; t.used = t.capacity = 0; t.failed = 0;
    text_raw(&t, version, sizeof(version) - 1);
    text_raw(&t, prelude, strlen(prelude));
    line = fragment_source + sizeof(version) - 1;
    while (*line && !t.failed) {
        const char *end = strchr(line, '\n');
        size_t length = end ? (size_t)(end - line) : strlen(line);
        int drop = 0;
        if (!strncmp(line, "varying", 7)) {
            for (slot = 0; slot < SLOT_COUNT && !drop; ++slot) drop = declares(line, length, "vec4", slot_names[slot]);
            if (!drop) drop = declares(line, length, "float", "xFog");
            if (!drop) {
                /* xWorldNormal and xWorldPosition (per-pixel lighting) have
                   no slot left, and nothing else is known */
                free(t.data);
                if (error && error_size) { strncpy(error, "fragment shader declares a varying that has no texcoord slot", error_size - 1); error[error_size - 1] = 0; }
                return 0;
            }
        }
        if (!drop) {
            text_raw(&t, line, length);
            if (end) text_raw(&t, "\n", 1);
        }
        line = end ? end + 1 : line + length;
    }
    if (t.failed) { free(t.data); return -1; }
    *rewritten_out = t.data;
    return 1;
}
