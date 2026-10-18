/* Host checks of the NV2A pixel shader -> ARBfp1.0 translator (port/mac/mac_nv2a_arbfp.c).

   - reads the pixel shader keys of a real frame (tools/mac_nv2a_arbfp_keys.txt, recovered from the
     dumped shaders by tools/mac_arbfp_keys_from_glsl.py) and, when given the dump directory, checks
     that the real generator reproduces each dumped shader from its recovered key (so that the keys
     are the frame's);
   - translates each key with every texture stage projective and with none, and checks the program
     with a validator of the ARBfp1.0 subset the translator writes: statement syntax, operand
     counts, declared temporaries, swizzles, and that every component an instruction reads has been
     written (a dataflow error would read garbage on a GPU);
   - synthetic states: every texture mode, fog mode, alpha-test function, output mapping, mux and
     dot flags, and the states that must be refused;
   - random combiner words: translating never fails other than by refusing, and every program
     validates.
   No GL calls. usage: probe [keys.txt [dump_dir [out_dir]]] */
#include "../port/mac/mac_nv2a_shader_api.h"
#include "../port/mac/mac_glsl120.h"
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
#include <ctype.h>

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); } } while (0)

/* ---------- the validator */

struct vtemp { int declared; unsigned defined; };

static const char component_names[] = "xyzw";

static int valid_swizzle(const char *s, int *comps, int *count)
{
    int n = 0;
    while (*s) {
        const char *p = strchr(component_names, *s);
        if (!p) return 0;
        if (n >= 4) return 0;
        comps[n++] = (int)(p - component_names);
        s++;
    }
    if (n != 1 && n != 4) return 0;
    *count = n;
    return 1;
}

/* a source operand: returns 0 on a syntax error; fills the components read (4 slots) and the temp */
static int parse_source(const char *text, int *temp, int *comps, int *count, int *kind)
{
    const char *p = text;
    char *end;
    *temp = -1; *count = 0; *kind = 0;
    if (*p == '-') p++;
    if (*p == 'R') {
        long n = strtol(p + 1, &end, 10);
        if (end == p + 1 || n < 0 || n > 127) return 0;
        *temp = (int)n; *kind = 1;
        if (*end == '.') return valid_swizzle(end + 1, comps, count);
        comps[0] = 0; comps[1] = 1; comps[2] = 2; comps[3] = 3; *count = 4;
        return *end == 0;
    }
    if (!strncmp(p, "fragment.texcoord[", 18)) {
        long n = strtol(p + 18, &end, 10);
        if (end == p + 18 || n < 0 || n > 7 || *end != ']') return 0;
        *kind = 2;
        end++;
        if (*end == '.') return valid_swizzle(end + 1, comps, count);
        comps[0] = 0; comps[1] = 1; comps[2] = 2; comps[3] = 3; *count = 4;
        return *end == 0;
    }
    if (!strncmp(p, "program.env[", 12)) {
        long n = strtol(p + 12, &end, 10);
        if (end == p + 12 || n < 0 || n >= MAC_NV2A_ARBFP_ENV_COUNT || *end != ']') return 0;
        *kind = 3;
        end++;
        if (*end == '.') return valid_swizzle(end + 1, comps, count);
        comps[0] = 0; comps[1] = 1; comps[2] = 2; comps[3] = 3; *count = 4;
        return *end == 0;
    }
    /* a scalar literal */
    {
        double v = strtod(p, &end);
        (void)v;
        if (end == p || *end != 0) return 0;
        *kind = 4;
        return 1;
    }
}

static int validate(const char *program, char *why, size_t why_size)
{
    struct vtemp temps[128];
    char *copy, *p, *statement;
    int ntemps = 0, saw_end = 0, wrote_color = 0, line = 0;
    unsigned color_written = 0;
    static const struct { const char *name; int nsrc; int scalar; } ops[] = {
        { "MOV", 1, 0 }, { "ADD", 2, 0 }, { "MUL", 2, 0 }, { "MAD", 3, 0 }, { "MAX", 2, 0 }, { "MIN", 2, 0 },
        { "DP3", 2, 0 }, { "LRP", 3, 0 }, { "CMP", 3, 0 }, { "SGE", 2, 0 }, { "FLR", 1, 0 }, { "FRC", 1, 0 },
        { "RCP", 1, 1 }, { "EX2", 1, 1 }, { "ABS", 1, 0 }, { 0, 0, 0 }
    };
    memset(temps, 0, sizeof(temps));
    why[0] = 0;
    copy = (char *)malloc(strlen(program) + 1);
    strcpy(copy, program);
    /* strip comments */
    for (p = copy; *p; p++)
        if (*p == '#') while (*p && *p != '\n') *p++ = ' ';
    p = copy;
    while (*p && isspace((unsigned char)*p)) p++;
    if (strncmp(p, "!!ARBfp1.0", 10)) { sprintf(why, "no header"); free(copy); return 0; }
    p += 10;
    statement = strtok(p, ";");
    while (statement) {
        char *s = statement;
        char op[16], *args;
        int k, nargs = 0;
        char *arg[8];
        ++line;
        while (*s && isspace((unsigned char)*s)) s++;
        if (!strncmp(s, "END", 3)) { saw_end = 1; break; }
        if (!strncmp(s, "TEMP", 4)) {
            /* the list is walked by hand: strtok is in use for the statements */
            {
                char *q = s + 4;
                while (*q) {
                    while (*q && (isspace((unsigned char)*q) || *q == ',')) q++;
                    if (*q == 'R') {
                        long n = strtol(q + 1, &q, 10);
                        if (n < 0 || n >= 128) { sprintf(why, "bad TEMP"); free(copy); return 0; }
                        temps[n].declared = 1;
                        if (n + 1 > ntemps) ntemps = (int)n + 1;
                    } else if (*q) { sprintf(why, "bad TEMP list"); free(copy); return 0; }
                }
            }
            statement = strtok(NULL, ";");
            continue;
        }
        /* opcode */
        k = 0;
        while (*s && !isspace((unsigned char)*s) && k < 15) op[k++] = *s++;
        op[k] = 0;
        args = s;
        while (*args && isspace((unsigned char)*args)) args++;
        /* split the arguments at commas */
        {
            char *q = args;
            arg[nargs++] = q;
            while (*q) {
                if (*q == ',') { *q = 0; if (nargs < 8) arg[nargs++] = q + 1; }
                q++;
            }
            for (k = 0; k < nargs; k++) {
                char *a = arg[k], *e;
                while (*a && isspace((unsigned char)*a)) a++;
                e = a + strlen(a);
                while (e > a && isspace((unsigned char)e[-1])) *--e = 0;
                arg[k] = a;
            }
        }
        {
            char base[16];
            int sat = 0, oi = -1, i;
            char *us;
            strcpy(base, op);
            us = strstr(base, "_SAT");
            if (us) { *us = 0; sat = 1; }
            (void)sat;
            if (!strcmp(base, "KIL")) {
                int temp, comps[4], count, kind;
                if (nargs != 1 || !parse_source(arg[0], &temp, comps, &count, &kind) || kind == 4) {
                    sprintf(why, "stmt %d: bad KIL", line); free(copy); return 0;
                }
                if (temp >= 0) {
                    int j;
                    if (!temps[temp].declared) { sprintf(why, "stmt %d: undeclared temp", line); free(copy); return 0; }
                    for (j = 0; j < count; j++)
                        if (!(temps[temp].defined & (1u << comps[j]))) {
                            sprintf(why, "stmt %d: KIL reads undefined R%d.%c", line, temp, "xyzw"[comps[j]]); free(copy); return 0;
                        }
                }
                statement = strtok(NULL, ";");
                continue;
            }
            if (!strcmp(base, "TEX") || !strcmp(base, "TXP")) {
                int temp, comps[4], count, kind, dtemp = -1, j;
                unsigned mask = 15u;
                int target3;
                if (nargs != 4) { sprintf(why, "stmt %d: TEX operands", line); free(copy); return 0; }
                if (arg[0][0] != 'R') { sprintf(why, "stmt %d: TEX destination", line); free(copy); return 0; }
                dtemp = atoi(arg[0] + 1);
                if (strchr(arg[0], '.')) { sprintf(why, "stmt %d: TEX with a write mask", line); free(copy); return 0; }
                if (!parse_source(arg[1], &temp, comps, &count, &kind) || kind == 4 || kind == 3) {
                    sprintf(why, "stmt %d: bad TEX coordinate", line); free(copy); return 0;
                }
                if (strncmp(arg[2], "texture[", 8) || arg[2][8] < '0' || arg[2][8] > '3' || arg[2][9] != ']') {
                    sprintf(why, "stmt %d: bad texture unit", line); free(copy); return 0;
                }
                if (strcmp(arg[3], "2D") && strcmp(arg[3], "3D") && strcmp(arg[3], "CUBE")) {
                    sprintf(why, "stmt %d: bad target", line); free(copy); return 0;
                }
                target3 = strcmp(arg[3], "2D") != 0;
                if (temp >= 0) {
                    if (!temps[temp].declared) { sprintf(why, "stmt %d: undeclared temp", line); free(copy); return 0; }
                    for (j = 0; j < (target3 ? 3 : 2); j++)
                        if (!(temps[temp].defined & (1u << comps[j]))) {
                            sprintf(why, "stmt %d: TEX reads undefined R%d.%c", line, temp, "xyzw"[comps[j]]); free(copy); return 0;
                        }
                }
                if (dtemp < 0 || !temps[dtemp].declared) { sprintf(why, "stmt %d: undeclared TEX destination", line); free(copy); return 0; }
                temps[dtemp].defined |= mask;
                statement = strtok(NULL, ";");
                continue;
            }
            for (i = 0; ops[i].name; i++) if (!strcmp(ops[i].name, base)) { oi = i; break; }
            if (oi < 0) { sprintf(why, "stmt %d: unknown opcode %s", line, op); free(copy); return 0; }
            if (nargs != 1 + ops[oi].nsrc) { sprintf(why, "stmt %d: %s operand count", line, op); free(copy); return 0; }
            {
                unsigned mask = 0;
                int dtemp = -1, is_color = 0, j, s_i;
                char *dot = strncmp(arg[0], "result.color", 12) ? strchr(arg[0], '.') : (arg[0][12] == '.' ? arg[0] + 12 : NULL);
                char dstname[32];
                if (!dot) {
                    strcpy(dstname, arg[0]);
                    mask = 15u;
                } else {
                    char *m;
                    int last = -1;
                    size_t len = (size_t)(dot - arg[0]);
                    if (len >= sizeof(dstname)) { sprintf(why, "stmt %d: dst", line); free(copy); return 0; }
                    memcpy(dstname, arg[0], len); dstname[len] = 0;
                    /* the mask: components in order, no repeats */
                    for (m = dot + 1; *m; m++) {
                        const char *c = strchr(component_names, *m);
                        int idx = c ? (int)(c - component_names) : -1;
                        if (idx < 0 || idx <= last) { sprintf(why, "stmt %d: bad write mask %s", line, arg[0]); free(copy); return 0; }
                        mask |= 1u << idx; last = idx;
                    }
                    if (!mask) { sprintf(why, "stmt %d: empty mask", line); free(copy); return 0; }
                }
                if (!strcmp(dstname, "result.color")) is_color = 1;
                else if (dstname[0] == 'R' && dstname[1]) dtemp = atoi(dstname + 1);
                else { sprintf(why, "stmt %d: bad destination %s", line, dstname); free(copy); return 0; }
                if (dtemp >= 0 && (dtemp >= 128 || !temps[dtemp].declared)) { sprintf(why, "stmt %d: undeclared destination", line); free(copy); return 0; }
                for (s_i = 0; s_i < ops[oi].nsrc; s_i++) {
                    int temp, comps[4], count, kind;
                    if (!parse_source(arg[1 + s_i], &temp, comps, &count, &kind)) {
                        sprintf(why, "stmt %d: bad source %s", line, arg[1 + s_i]); free(copy); return 0;
                    }
                    if (temp >= 0) {
                        if (!temps[temp].declared) { sprintf(why, "stmt %d: undeclared source R%d", line, temp); free(copy); return 0; }
                        for (j = 0; j < 4; j++) {
                            int need;
                            if (!(mask & (1u << j)) && !(!strcmp(base, "DP3") && j < 3)) continue;
                            if (!strcmp(base, "DP3")) need = j < 3 ? (count == 1 ? comps[0] : comps[j]) : -1;
                            else need = count == 1 ? comps[0] : comps[j];
                            if (need < 0) continue;
                            if (!(temps[temp].defined & (1u << need))) {
                                sprintf(why, "stmt %d: %s reads undefined R%d.%c", line, op, temp, "xyzw"[need]); free(copy); return 0;
                            }
                            if (ops[oi].scalar) break;
                        }
                    }
                }
                if (dtemp >= 0) temps[dtemp].defined |= mask;
                if (is_color) { wrote_color = 1; color_written |= mask; }
            }
        }
        statement = strtok(NULL, ";");
    }
    free(copy);
    if (!saw_end) { sprintf(why, "no END"); return 0; }
    if (!wrote_color || color_written != 15u) { sprintf(why, "result.color not fully written (%u)", color_written); return 0; }
    (void)why_size;
    return 1;
}

#include "mac_nv2a_arbfp_keys.h"

static void save(const char *dir, const char *name, const char *suffix, const char *text)
{
    char path[512];
    FILE *f;
    if (!dir) return;
    sprintf(path, "%s/%s%s", dir, name, suffix);
    f = fopen(path, "wb");
    if (f) { fputs(text, f); fclose(f); }
}

static int translate_key(const struct nv2a_pixel_shader_key *key, int fog_tex, int fog_comp, unsigned long mask,
    char **text, struct mac_nv2a_arbfp_info *info, char *error)
{
    struct mac_nv2a_arb_layout layout;
    memset(&layout, 0, sizeof(layout));
    layout.fog_texcoord = fog_tex; layout.fog_component = fog_comp;
    return mac_nv2a_arbfp_translate_info(key, &layout, mask, text, info, error, 160);
}

static void expect_valid(const char *label, const struct nv2a_pixel_shader_key *key, unsigned long mask, int fog_tex)
{
    char *text = NULL, error[160], why[160];
    struct mac_nv2a_arbfp_info info;
    int r = translate_key(key, fog_tex, 3, mask, &text, &info, error);
    if (r != 1) { fprintf(stderr, "%s: translate returned %d (%s)\n", label, r, error); CHECK(r == 1); return; }
    if (!validate(text, why, sizeof(why))) {
        fprintf(stderr, "%s: invalid program: %s\n%s\n", label, why, text);
        CHECK(0);
    } else CHECK(1);
    free(text);
}

static void expect_refused(const char *label, const struct nv2a_pixel_shader_key *key)
{
    char *text = NULL, error[160];
    int r = translate_key(key, -1, 0, 0, &text, NULL, error);
    if (r != 0) fprintf(stderr, "%s: expected a refusal, got %d\n", label, r);
    CHECK(r == 0 && text == NULL && error[0]);
    free(text);
}

static unsigned long lcg_state = 12345;
static unsigned long rnd(void) { lcg_state = lcg_state * 1103515245UL + 12345UL; return (lcg_state >> 8) & 0xffffffUL; }

int main(int argc, char **argv)
{
    const char *keys_path = argc > 1 ? argv[1] : "tools/mac_nv2a_arbfp_keys.txt";
    const char *dump_dir = argc > 2 ? argv[2] : NULL;
    const char *out_dir = argc > 3 ? argv[3] : NULL;
    struct record *records = NULL;
    int count = 0, i, mode, stage;
    unsigned long total_samples = 0;
    double weighted_alu = 0, weighted_tex = 0;
    unsigned translated = 0;

    if (!read_keys(keys_path, &records, &count)) return 2;
    CHECK(count > 0);
    printf("%-12s %7s %9s | %5s %4s %5s | %5s %4s %5s\n", "shader", "draws", "samples", "alu", "tex", "temps",
        "alu_p", "tex_p", "temps");
    for (i = 0; i < count; i++) {
        struct record *r = &records[i];
        char *text = NULL, *text_p = NULL, error[160], why[160];
        struct mac_nv2a_arbfp_info info, info_p;
        int rc, rc_p;
        unsigned long projective = 0;
        int st;
        /* the generator reproduces the dumped shader from the key */
        if (dump_dir) {
            char path[512], *dumped, *modern, *legacy = NULL, *rewritten = NULL;
            struct mac_nv2a_arb_layout layout;
            sprintf(path, "%s/%s.frag", dump_dir, r->name);
            dumped = read_file(path);
            modern = nv2a_pixel_shader_to_glsl(&r->key);
            CHECK(dumped && modern);
            if (dumped && modern && mac_glsl120_pixel(modern, &legacy, error, sizeof(error))) {
                memset(&layout, 0, sizeof(layout));
                layout.fog_texcoord = r->fog_tex; layout.fog_component = r->fog_comp;
                CHECK(mac_nv2a_arb_fragment_rewrite(legacy, &layout, &rewritten, error, sizeof(error)) == 1);
                if (rewritten && strcmp(rewritten, dumped)) {
                    fprintf(stderr, "%s: regenerated GLSL differs from the dump\n", r->name);
                    save(out_dir, r->name, ".regen.frag", rewritten);
                    CHECK(0);
                } else CHECK(rewritten != NULL);
            } else CHECK(0);
            free(dumped); free(modern); free(legacy); free(rewritten);
        }
        for (st = 0; st < 4; st++) projective |= (1UL << st);
        rc = translate_key(&r->key, r->fog_tex, r->fog_comp, 0, &text, &info, error);
        rc_p = translate_key(&r->key, r->fog_tex, r->fog_comp, projective, &text_p, &info_p, error);
        if (rc != 1 || rc_p != 1) {
            printf("%-12s %7lu %9lu | refused: %s\n", r->name, r->draws, r->samples, error);
            free(text); free(text_p);
            continue;
        }
        ++translated;
        CHECK(validate(text, why, sizeof(why)) || (fprintf(stderr, "%s: %s\n", r->name, why), 0));
        CHECK(validate(text_p, why, sizeof(why)) || (fprintf(stderr, "%s (projective): %s\n", r->name, why), 0));
        save(out_dir, r->name, ".arb", text);
        save(out_dir, r->name, ".proj.arb", text_p);
        printf("%-12s %7lu %9lu | %5u %4u %5u | %5u %4u %5u\n", r->name, r->draws, r->samples, info.alu_instructions,
            info.tex_instructions, info.temporaries, info_p.alu_instructions, info_p.tex_instructions,
            info_p.temporaries);
        total_samples += r->samples;
        weighted_alu += (double)r->samples * info_p.alu_instructions;
        weighted_tex += (double)r->samples * info_p.tex_instructions;
        free(text); free(text_p);
    }
    printf("translated %u of %d; sample-weighted instructions per pixel (projective): alu %.1f tex %.1f\n", translated, count,
        total_samples ? weighted_alu / (double)total_samples : 0.0, total_samples ? weighted_tex / (double)total_samples : 0.0);
    CHECK((int)translated == count);

    /* synthetic states */
    {
        struct nv2a_pixel_shader_key key;
        char label[96];
        unsigned long m;
        for (mode = 0; mode <= 18; mode++) {
            int sampler;
            for (sampler = 0; sampler <= 3; sampler++) {
                for (stage = 0; stage < 4; stage++) {
                    memset(&key, 0, sizeof(key));
                    key.combiner_state[D3DRS_PSCOMBINERCOUNT] = 1;
                    key.combiner_state[D3DRS_PSRGBINPUTS0] = ((0x08 + stage) << 24) | 0x00200000;
                    key.combiner_state[D3DRS_PSRGBOUTPUTS0] = 0x0c00;
                    key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x0c;
                    key.combiner_state[D3DRS_PSDOTMAPPING] = 0x3210 + mode;
                    key.combiner_state[D3DRS_PSINPUTTEXTURE] = (1UL << 16) | (2UL << 20);
                    key.combiner_state[D3DRS_PSCOMPAREMODE] = 0x5aaaUL * (unsigned)(mode + 1);
                    key.texture_modes = (unsigned long)mode << (5 * stage);
                    if (stage) key.texture_modes |= 1UL; /* stage 0 samples a 2D texture for the dot modes to read */
                    if (stage) key.sampler_type[0] = 1;
                    key.sampler_type[stage] = (unsigned char)sampler;
                    {
                        char *text = NULL, error[160], why[160];
                        struct mac_nv2a_arbfp_info info;
                        int r = translate_key(&key, -1, 0, (stage & 1) ? 0xf : 0, &text, &info, error);
                        sprintf(label, "mode %d sampler %d stage %d", mode, sampler, stage);
                        if (r == 1) {
                            if (!validate(text, why, sizeof(why))) {
                                fprintf(stderr, "%s: %s\n%s\n", label, why, text);
                                CHECK(0);
                            } else CHECK(1);
                        } else {
                            /* refusals are by design: brdf, undefined modes, dot modes in stage 0, bump in stage 0, project on cube */
                            CHECK(r == 0 && error[0]);
                        }
                        free(text);
                    }
                }
            }
        }
        (void)m;
    }
    /* fog, alpha test, flags */
    {
        struct nv2a_pixel_shader_key key;
        unsigned long fog, func, mapping, flags;
        char label[96];
        for (fog = 0; fog < 5; fog++) {
            for (func = 0; func < 9; func++) {
                memset(&key, 0, sizeof(key));
                key.combiner_state[D3DRS_PSCOMBINERCOUNT] = 1;
                key.combiner_state[D3DRS_PSRGBINPUTS0] = 0x04030000; /* v0 * fog */
                key.combiner_state[D3DRS_PSRGBOUTPUTS0] = 0x0c00;
                key.combiner_state[D3DRS_PSALPHAINPUTS0] = 0x14130000;
                key.combiner_state[D3DRS_PSALPHAOUTPUTS0] = 0x0c00;
                key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x0c;
                key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = 0x1c00;
                key.fog_enable = fog != 0; key.fog_table_mode = (unsigned char)(fog > 0 ? fog - 1 : 0);
                key.alpha_test_function = func ? 511 + func : 0;
                sprintf(label, "fog %lu alpha %lu", fog, func);
                expect_valid(label, &key, 0, (fog & 1) ? 1 : -1);
            }
        }
        for (mapping = 0; mapping < 8; mapping++) {
            for (flags = 0; flags < 8; flags++) {
                int mux_msb;
                for (mux_msb = 0; mux_msb < 2; mux_msb++) {
                    memset(&key, 0, sizeof(key));
                    key.combiner_state[D3DRS_PSCOMBINERCOUNT] = 2 | (mux_msb ? 0x100 : 0) | 0x1000 | 0x10000;
                    key.combiner_state[D3DRS_PSRGBINPUTS0] = 0x080a0c0f; /* t0 t2 r0 ... */
                    key.combiner_state[D3DRS_PSRGBINPUTS0 + 1] = 0x01c2e0a0;
                    key.combiner_state[D3DRS_PSALPHAINPUTS0] = 0x18121000;
                    key.combiner_state[D3DRS_PSALPHAINPUTS0 + 1] = 0x1c0c0400;
                    key.combiner_state[D3DRS_PSRGBOUTPUTS0] = 0x0c00 | (flags << 12) | (mapping << 15) | 0x4c;
                    key.combiner_state[D3DRS_PSRGBOUTPUTS0 + 1] = 0xc00 | (flags << 12) | 0xc4;
                    key.combiner_state[D3DRS_PSALPHAOUTPUTS0] = 0x0c00 | (flags << 12) | (mapping << 15);
                    key.combiner_state[D3DRS_PSALPHAOUTPUTS0 + 1] = 0x0cd0 | ((flags & 4) << 12);
                    key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x0c0d0e0f;
                    key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = 0x0d0e1c00 | (flags << 5);
                    key.texture_modes = 1 | (1 << 5) | (3 << 10);
                    key.sampler_type[0] = key.sampler_type[1] = 1; key.sampler_type[2] = 3;
                    sprintf(label, "mapping %lu flags %lu msb %d", mapping, flags, mux_msb);
                    expect_valid(label, &key, 3, 2);
                }
            }
        }
    }
    /* refusals */
    {
        struct nv2a_pixel_shader_key key;
        memset(&key, 0, sizeof(key));
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x0c;
        key.count_samples = 1; expect_refused("count_samples", &key); key.count_samples = 0;
        key.point_threshold = 1; expect_refused("point_threshold", &key); key.point_threshold = 0;
        key.per_pixel_lighting = 1; expect_refused("per_pixel_lighting", &key); key.per_pixel_lighting = 0;
        key.alpha_test_samples = 1; expect_refused("alpha_test_samples", &key); key.alpha_test_samples = 0;
        key.source_blend = 1; expect_refused("source_blend", &key); key.source_blend = 0;
        key.texture_modes = 1; key.sampler_type[0] = 1; key.alpha_kill[0] = 1;
        expect_refused("alpha_kill", &key); key.alpha_kill[0] = 0;
        key.color_sign[0] = 1; expect_refused("color_sign", &key); key.color_sign[0] = 0;
        key.texture_modes = 1; key.sampler_type[0] = 3; expect_refused("project on cube", &key);
        key.texture_modes = 8; key.sampler_type[0] = 1; expect_refused("brdf", &key);
        key.texture_modes = 31; expect_refused("undefined texture mode", &key);
        key.texture_modes = 0; key.sampler_type[0] = 0;
        expect_valid("untextured", &key, 0, -1);
    }
    /* random words: refusal or a valid program */
    {
        int n;
        unsigned accepted = 0, refused = 0;
        for (n = 0; n < 4000; n++) {
            struct nv2a_pixel_shader_key key;
            char *text = NULL, error[160], why[160];
            struct mac_nv2a_arbfp_info info;
            int r, j;
            memset(&key, 0, sizeof(key));
            for (j = 0; j < 8; j++) {
                key.combiner_state[D3DRS_PSRGBINPUTS0 + j] = (rnd() << 8) ^ rnd();
                key.combiner_state[D3DRS_PSALPHAINPUTS0 + j] = (rnd() << 8) ^ rnd();
                key.combiner_state[D3DRS_PSRGBOUTPUTS0 + j] = (rnd() & 0xfff) | ((rnd() & 0xff) << 12);
                key.combiner_state[D3DRS_PSALPHAOUTPUTS0 + j] = (rnd() & 0xfff) | ((rnd() & 0xff) << 12);
            }
            key.combiner_state[D3DRS_PSCOMBINERCOUNT] = (rnd() % 9) | ((rnd() & 1) ? 0x100 : 0) | ((rnd() & 1) ? 0x1000 : 0) | ((rnd() & 1) ? 0x10000 : 0);
            key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = (rnd() << 8) ^ rnd();
            key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = (rnd() << 8) ^ rnd();
            key.combiner_state[D3DRS_PSDOTMAPPING] = rnd();
            key.combiner_state[D3DRS_PSINPUTTEXTURE] = rnd();
            key.combiner_state[D3DRS_PSCOMPAREMODE] = rnd();
            key.texture_modes = (rnd() % 5 == 0) ? rnd() : (unsigned long)((rnd() % 5) | ((rnd() % 5) << 5) | ((rnd() % 5) << 10) | ((rnd() % 5) << 15));
            for (j = 0; j < 4; j++) key.sampler_type[j] = (unsigned char)(rnd() % 4);
            key.alpha_test_function = (rnd() & 3) ? 512 + rnd() % 8 : 0;
            key.fog_enable = rnd() & 1; key.fog_table_mode = (unsigned char)(rnd() % 4);
            r = translate_key(&key, (int)(rnd() % 9) - 1, (int)(rnd() % 4), rnd() & 15, &text, &info, error);
            CHECK(r == 0 || r == 1);
            if (r == 1) {
                ++accepted;
                if (!validate(text, why, sizeof(why))) {
                    fprintf(stderr, "random %d: %s\n%s\n", n, why, text);
                    CHECK(0);
                }
            } else ++refused;
            free(text);
        }
        printf("random states: %u translated, %u refused\n", accepted, refused);
        CHECK(accepted > 500);
    }
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
