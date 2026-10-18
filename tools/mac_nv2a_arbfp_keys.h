/* Reading tools/mac_nv2a_arbfp_keys.txt (the pixel shader keys of the real frame, written by
   tools/mac_arbfp_keys_from_glsl.py): shared by tools/mac_nv2a_arbfp_probe.c and
   tools/mac_nv2a_arbfp_gpu_probe.c. Include after the NV2A shader API header and <stdio.h>/<stdlib.h>/<string.h>. */
#ifndef HALO_MAC_NV2A_ARBFP_KEYS_H
#define HALO_MAC_NV2A_ARBFP_KEYS_H

struct record {
    char name[32];
    unsigned long draws, samples;
    int fog_tex, fog_comp;
    struct nv2a_pixel_shader_key key;
};

static void fill_key(struct nv2a_pixel_shader_key *key, const unsigned long w[32])
{
    int n;
    memset(key, 0, sizeof(*key));
    for (n = 0; n < 8; n++) {
        key->combiner_state[D3DRS_PSRGBINPUTS0 + n] = w[n];
        key->combiner_state[D3DRS_PSALPHAINPUTS0 + n] = w[8 + n];
        key->combiner_state[D3DRS_PSRGBOUTPUTS0 + n] = w[16 + n];
        key->combiner_state[D3DRS_PSALPHAOUTPUTS0 + n] = w[24 + n];
    }
}

static int read_keys(const char *path, struct record **out, int *count)
{
    FILE *f = fopen(path, "r");
    char line[2048];
    struct record *list = NULL;
    int n = 0, cap = 0;
    if (!f) { perror(path); return 0; }
    while (fgets(line, sizeof(line), f)) {
        struct record r;
        unsigned long count_word, abcd, efg, compare, dotmap, inputtex, modes, w[32], alpha_func, fog_enable, fog_mode;
        unsigned s0, s1, s2, s3;
        char *p = line;
        int i, used;
        if (line[0] == '#' || line[0] == '\n') continue;
        memset(&r, 0, sizeof(r));
        if (sscanf(p, "%31s %lu %lu %d %d %lx %lx %lx %lx %lx %lx %lx %u %u %u %u %lu %lu %lu%n", r.name, &r.draws,
            &r.samples, &r.fog_tex, &r.fog_comp, &count_word, &abcd, &efg, &compare, &dotmap, &inputtex, &modes,
            &s0, &s1, &s2, &s3, &alpha_func, &fog_enable, &fog_mode, &used) < 19) {
            fprintf(stderr, "bad key line: %s", line); fclose(f); return 0;
        }
        p += used;
        for (i = 0; i < 32; i++) {
            int u2;
            if (sscanf(p, " %lx%n", &w[i], &u2) != 1) { fprintf(stderr, "short key line\n"); fclose(f); return 0; }
            p += u2;
        }
        fill_key(&r.key, w);
        r.key.combiner_state[D3DRS_PSCOMBINERCOUNT] = count_word;
        r.key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = abcd;
        r.key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = efg;
        r.key.combiner_state[D3DRS_PSCOMPAREMODE] = compare;
        r.key.combiner_state[D3DRS_PSDOTMAPPING] = dotmap;
        r.key.combiner_state[D3DRS_PSINPUTTEXTURE] = inputtex;
        r.key.texture_modes = modes;
        r.key.sampler_type[0] = (unsigned char)s0; r.key.sampler_type[1] = (unsigned char)s1;
        r.key.sampler_type[2] = (unsigned char)s2; r.key.sampler_type[3] = (unsigned char)s3;
        r.key.alpha_test_function = alpha_func;
        r.key.fog_enable = (unsigned char)fog_enable; r.key.fog_table_mode = (unsigned char)fog_mode;
        if (n == cap) { cap = cap ? cap * 2 : 64; list = (struct record *)realloc(list, cap * sizeof(*list)); }
        list[n++] = r;
    }
    fclose(f);
    *out = list; *count = n;
    return 1;
}

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *buffer;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    buffer = (char *)malloc((size_t)n + 1);
    if (fread(buffer, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(buffer); return NULL; }
    buffer[n] = 0; fclose(f);
    return buffer;
}


#endif
