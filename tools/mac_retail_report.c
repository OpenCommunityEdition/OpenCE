#include "retail_cache.h"
#include "cache_endian.h"
#include "retail_geometry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
struct geometry_summary { uint32_t materials, vertices, light_vertices, triangles, digest; };
static void digest_word(struct geometry_summary *s, uint32_t word)
{ s->digest = (s->digest ^ word) * UINT32_C(16777619); }
static int geometry_material(void *context, const struct mac_retail_material *m)
{
    struct geometry_summary *s = (struct geometry_summary *)context;
    uint32_t i, bits;
    const unsigned char *p;
    ++s->materials; s->vertices+=m->vertex_count;
    s->light_vertices+=m->light_vertex_count; s->triangles+=m->triangle_count;
    digest_word(s,m->shader_handle); digest_word(s,(uint32_t)m->lightmap_bitmap);
    digest_word(s,m->vertex_count); digest_word(s,m->light_vertex_count); digest_word(s,m->triangle_count);
    p=(const unsigned char *)m->vertices;
    for (i=0;i<m->vertex_count*14;++i) { memcpy(&bits,p+i*4,4); digest_word(s,bits); }
    p=(const unsigned char *)m->light_vertices;
    for (i=0;i<m->light_vertex_count*5;++i) { memcpy(&bits,p+i*4,4); digest_word(s,bits); }
    for (i=0;i<m->triangle_count*3;++i) digest_word(s,m->indices[i]);
    return 1;
}
static uint32_t file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) || st.st_size < 0 || (uint64_t)st.st_size > UINT32_MAX) return 0;
    return (uint32_t)st.st_size;
}
static int parse_size(const char *text, uint32_t *out)
{
    char *end;
    unsigned long value;
    if (!text[0] || text[0] < '0' || text[0] > '9') return 0;
    errno = 0; value = strtoul(text, &end, 10);
    if (errno || *end || !value || value > UINT32_MAX) return 0;
    *out = (uint32_t)value; return 1;
}
int main(int argc, char **argv)
{
    struct mac_retail_cache cache;
    struct mac_retail_resources resources;
    struct mac_retail_tag tag;
    struct geometry_summary geometry = {0,0,0,0,UINT32_C(2166136261)};
    unsigned char *bytes, *copy;
    const unsigned char *raw, *scenario;
    const char *error;
    uint32_t size, i, count = 0, syntax_size, syntax_address, bitmap_size, sound_size;
    FILE *f;
    if (argc == 4) { bitmap_size = file_size(argv[2]); sound_size = file_size(argv[3]); }
    else if (argc == 5 && !strcmp(argv[2], "--resource-sizes") &&
        parse_size(argv[3], &bitmap_size) && parse_size(argv[4], &sound_size)) {
        printf("Resource bounds supplied from source-machine inventory; payload files not opened here\n");
    } else {
        fprintf(stderr, "usage: %s map bitmaps.map sounds.map OR map --resource-sizes bitmap_bytes sound_bytes\n", argv[0]); return 2;
    }
    size = file_size(argv[1]);
    if (size < 0x800 || size > UINT32_C(0x30000000)) return 2;
    bytes = (unsigned char *)malloc(size);
    f = fopen(argv[1], "rb");
    if (!bytes || !f) { free(bytes); if (f) fclose(f); return 2; }
    if (fread(bytes, 1, size, f) != size) { free(bytes); fclose(f); return 2; }
    fclose(f);
    if (!mac_retail_open(bytes, size, &cache, &error)) {
        fprintf(stderr, "retail open: %s\n", error); free(bytes); return 1;
    }
    printf("Retail v7 %s: tags=%lu BSPs=%lu model_bytes=%lu\n", cache.name,
        (unsigned long)cache.tag_count, (unsigned long)cache.bsp_count, (unsigned long)cache.model_size);
    if (!mac_retail_resources_check(&cache, bitmap_size, sound_size, &resources, &error)) {
        fprintf(stderr, "resources: %s\n", error); free(bytes); return 1;
    }
    printf("Payload ranges: bitmaps=%lu (%lu external), sounds=%lu (%lu external)\n",
        (unsigned long)resources.bitmap_entries, (unsigned long)resources.external_bitmaps,
        (unsigned long)resources.sound_entries, (unsigned long)resources.external_sounds);
    for (i = 0; i < cache.tag_count; ++i) {
        enum mac_cache_record record;
        unsigned int record_size;
        unsigned char scratch[0x48];
        mac_retail_tag_get(&cache, i, &tag);
        if (tag.group == UINT32_C(0x70706879)) { record = MAC_CACHE_POINT_PHYSICS; record_size = 0x40; }
        else if (tag.group == UINT32_C(0x77696e64)) { record = MAC_CACHE_WIND; record_size = 0x40; }
        else if (tag.group == UINT32_C(0x736e6465)) { record = MAC_CACHE_SOUND_ENVIRONMENT; record_size = 0x48; }
        else continue;
        raw = mac_retail_tag_span(&cache, tag.address, record_size);
        if (!raw) { free(bytes); return 1; }
        memcpy(scratch, raw, record_size);
        if (!mac_cache_record_convert(scratch, record_size, record, 0) ||
            !mac_cache_record_convert(scratch, record_size, record, 1) || memcmp(scratch, raw, record_size)) {
            fprintf(stderr, "tag conversion: %s\n", tag.name); free(bytes); return 1;
        }
        ++count;
    }
    printf("Real point-physics/wind/sound-environment tags round-tripped: %lu\n", (unsigned long)count);
    scenario = cache.tags+cache.scenario_offset;
    syntax_size = u32(scenario+0x474); syntax_address = u32(scenario+0x480);
    raw = mac_retail_tag_span(&cache, syntax_address, syntax_size);
    if (!raw || syntax_size != 0x38+19001*20) { fprintf(stderr, "script allocation range/size\n"); free(bytes); return 1; }
    copy = (unsigned char *)malloc(syntax_size);
    if (!copy) { free(bytes); return 2; }
    memcpy(copy, raw, syntax_size);
    if (!mac_cache_syntax_convert(copy, syntax_size, 19001, 0) ||
        !mac_cache_syntax_convert(copy, syntax_size, 19001, 1) || memcmp(copy, raw, syntax_size)) {
        fprintf(stderr, "script conversion\n"); free(copy); free(bytes); return 1;
    }
    printf("Real script allocation: 19001 slots round-tripped; metadata only, gameplay not enabled\n");
    for (i=0;i<cache.bsp_count;++i) {
        if (!mac_retail_bsp_geometry(&cache,i,geometry_material,&geometry,&error)) {
            fprintf(stderr,"geometry: %s\n",error); free(copy); free(bytes); return 1;
        }
    }
    printf("Native BSP geometry: materials=%lu vertices=%lu light_vertices=%lu triangles=%lu digest=%08lx\n",
        (unsigned long)geometry.materials,(unsigned long)geometry.vertices,
        (unsigned long)geometry.light_vertices,(unsigned long)geometry.triangles,(unsigned long)geometry.digest);
    free(copy); free(bytes);
    return 0;
}
