/* Walks every tag of Xbox maps with the layout tables (port/mac/xbox_tag_tables.c, xbox_tag_walk.c) in
   check mode and reports, per map, what it proves: every block, reference and data field lies in the tag
   data, every structure fits, no byte is walked twice, floats are finite; and how much of the tag data is
   covered (the vertices and indices count: they are xbox_geometry.c's). Exit 1 if any map fails.
   usage: mac_xbox_tag_check [-v] [-g] map...   (-g lists the gaps of 4 bytes or more) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbox_map.h"
#include "xbox_tag_tables.h"
#include "xbox_tag_walk.h"
#include "xbox_geometry.h"

static struct mac_xbox_walk *current;
static uint32_t current_base;
static uint32_t geometry_marked;
static void visit(int kind, uint32_t address, uint32_t count)
{
    uint32_t bytes = kind == 100 ? count * 2 : kind == 3 ? count * 8 : count * 32, offset = address - current_base;
    if (address >= current_base && mac_xbox_walk_mark(current, offset, bytes)) geometry_marked += bytes;
}
/* (without xbox_geometry.c linked, nothing marks the vertices: weak stand-ins) */
int __attribute__((weak)) mac_xbox_geometry_swap_enabled(void) { return 0; }
void __attribute__((weak)) mac_xbox_geometry_force(int on) { (void)on; }
void __attribute__((weak)) mac_xbox_geometry_set_visitor(void (*proc)(int, uint32_t, uint32_t)) { (void)proc; }
int __attribute__((weak)) mac_xbox_geometry_tags_to_native(unsigned char *d, uint32_t s, const struct mac_xbox_map_info *i, const char **e) { (void)d; (void)s; (void)i; (void)e; return 1; }
int __attribute__((weak)) mac_xbox_geometry_bsp_to_native(unsigned char *d, uint32_t s, const struct mac_xbox_bsp *b, const char **e) { (void)d; (void)s; (void)b; (void)e; return 1; }

/* -b: which runs of bytes the tables leave as bytes hold non-zero data (padding is zero; floats or words left as bytes are not) */
struct byte_run { const char *layout; uint32_t offset, count, seen, nonzero, words_like; unsigned char example[16]; struct byte_run *next; };
static struct byte_run *byte_runs[4096];
static void bytes_probe(const char *layout, uint32_t offset, uint32_t count, const unsigned char *p)
{
    unsigned h = (unsigned)(((size_t)layout >> 3) * 31 + offset) & 4095, i;
    struct byte_run *r;
    for (r = byte_runs[h]; r; r = r->next) if (r->layout == layout && r->offset == offset) break;
    if (!r) { r = (struct byte_run *)calloc(1, sizeof(*r)); r->layout = layout; r->offset = offset; r->count = count; r->next = byte_runs[h]; byte_runs[h] = r; }
    ++r->seen;
    for (i = 0; i < count; ++i) if (p[i]) { if (!r->nonzero) memcpy(r->example, p, count < 16 ? count : 16); ++r->nonzero; break; }
    /* words that read as plain floats (0x30800000 .. 0x4e800000 in magnitude): real data left as bytes looks like this */
    for (i = 0; i + 4 <= count; i += 4) {
        uint32_t v = (uint32_t)p[i] | ((uint32_t)p[i + 1] << 8) | ((uint32_t)p[i + 2] << 16) | ((uint32_t)p[i + 3] << 24), a = v & 0x7fffffffu;
        if (a >= 0x30800000u && a <= 0x4e800000u) ++r->words_like;
    }
}
static void bytes_report(void)
{
    unsigned h;
    for (h = 0; h < 4096; ++h) { struct byte_run *r; for (r = byte_runs[h]; r; r = r->next) if (r->nonzero)
        printf("  bytes %-40s +%-5lu x%-5lu non-zero in %lu of %lu, float-like words %lu: %02x%02x%02x%02x %02x%02x%02x%02x\n", r->layout, (unsigned long)r->offset, (unsigned long)r->count,
            (unsigned long)r->nonzero, (unsigned long)r->seen, (unsigned long)r->words_like, r->example[0], r->example[1], r->example[2], r->example[3], r->example[4], r->example[5], r->example[6], r->example[7]); }
}

int main(int argc, char **argv)
{
    int a, bad = 0, verbose = 0, gaps_list = 0, do_convert = 1, show_coverage = 0;
    uint32_t gap_min = 4, gap_limit = 40;
    int gaps_other_only = 0;
    uint32_t *layout_bytes = (uint32_t *)calloc(mac_xbox_tag_layout_count, 4), *layout_elements = (uint32_t *)calloc(mac_xbox_tag_layout_count, 4);
    for (a = 1; a < argc; ++a) {
        struct mac_mapped_file map;
        struct mac_xbox_map_info info;
        const char *error = NULL;
        uint32_t b, unread = 0, hashes = 0, geometry_bytes = 0, total_bytes = 0, covered = 0, gaps_small = 0, gaps_fill = 0, gap_bytes_fill = 0, gaps_big = 0, gap_bytes_big = 0, biggest = 0, biggest_at = 0, elements = 0, floats = 0, nones = 0, bad_floats = 0, huge = 0, opaque = 0, opaque_el = 0, files = 0;
        int ok = 1;
        if (!strcmp(argv[a], "-v")) { verbose = 1; continue; }
        if (!strcmp(argv[a], "-b")) { mac_xbox_walk_bytes_probe = bytes_probe; continue; }
        if (!strcmp(argv[a], "-g")) { gaps_list = 1; show_coverage = 1; continue; }
        if (!strcmp(argv[a], "-c")) { show_coverage = 1; continue; }
        if (!strcmp(argv[a], "-o")) { gaps_other_only = 1; continue; }
        if (!strncmp(argv[a], "-m", 2)) { gap_min = (uint32_t)atoi(argv[a] + 2); continue; }
        if (!strncmp(argv[a], "-n", 2)) { gap_limit = (uint32_t)atoi(argv[a] + 2); continue; }
        if (!mac_xbox_map_open(argv[a], &map, &error) || !mac_xbox_map_inspect(map.data, map.size, &info, &error)) { printf("%s: %s\n", argv[a], error); ++bad; continue; }
        for (b = 0; b <= info.bsp_count; ++b) {
            uint32_t offset = b ? info.bsps[b - 1].file_offset : info.tag_data_offset, size = b ? info.bsps[b - 1].size : info.tag_data_size;
            uint32_t base = b ? info.bsps[b - 1].base : MAC_XBOX_TAG_BASE, i;
            struct mac_xbox_walk w;
            uint32_t region_geometry = 0;
            unsigned char *copy = (unsigned char *)malloc(size);
            memset(&w, 0, sizeof(w));
            w.layout_bytes = layout_bytes; w.layout_elements = layout_elements;
            memcpy(copy, map.data + offset, size);
            if (!mac_xbox_walk_begin(&w, copy, base, size, 0)) { ++bad; free(copy); continue; }
            if (getenv("XBOX_WALK_OWNERS")) w.owners = (const char **)calloc(size, sizeof(char *));
            if (b ? !mac_xbox_walk_bsp(&w, &info.bsps[b - 1]) : !mac_xbox_walk_tags(&w, &info)) {
                printf("%s: region %lu refused: %s at %08lx (%s, first by %s)\n", argv[a], (unsigned long)b, w.error, (unsigned long)(w.virtual_base + w.error_offset), w.error_layout ? w.error_layout : "?", w.error_other ? w.error_other : "?");
                ok = 0;
            } else {
                /* the vertices and indices */
                current = &w; current_base = base;
                geometry_marked = 0;
                mac_xbox_geometry_set_visitor(visit);
                { union { uint32_t v; unsigned char c[4]; } u; u.v = 1;
                  if (u.c[0] == 1) {    /* (a big-endian host cannot read the map's tags natively: its coverage is taken without the geometry) */
                    mac_xbox_geometry_force(1);
                    if (b) mac_xbox_geometry_bsp_to_native(copy, size, &info.bsps[b - 1], &error); else mac_xbox_geometry_tags_to_native(copy, size, &info, &error);
                    mac_xbox_geometry_force(0);
                  } }
                if (gaps_list && b == 0) {
                    /* the gaps of 4 bytes or more, with the tag whose data starts last before them */
                    uint32_t count = (uint32_t)(copy[0xc] | copy[0xd] << 8 | copy[0xe] << 16 | copy[0xf] << 24);
                    uint32_t table = ((uint32_t)(copy[0] | copy[1] << 8 | copy[2] << 16 | copy[3] << 24)) - base, t, j, listed = 0;
                    for (i = 0; i < size && listed < gap_limit;) {
                        if (mac_xbox_walk_is_covered(&w, i)) { ++i; continue; }
                        for (j = i; j < size && !mac_xbox_walk_is_covered(&w, j); ++j) { }
                        if (j - i >= gap_min && (!gaps_other_only || (copy[i] != 0 && copy[i] != 0xff))) {
                            uint32_t best = 0, best_t = 0;
                            for (t = 0; t < count; ++t) {
                                const unsigned char *q = copy + table + t * 32;
                                uint32_t root = ((uint32_t)(q[20] | q[21] << 8 | q[22] << 16 | q[23] << 24));
                                if (root >= base && root - base <= i && root - base >= best) { best = root - base; best_t = t; }
                            }
                            { const unsigned char *q = copy + table + best_t * 32; uint32_t nm = ((uint32_t)(q[16] | q[17] << 8 | q[18] << 16 | q[19] << 24)) - base;
                              printf("    gap %08lx +%lu (after %c%c%c%c %s at +%lu; previous byte of %s) %02x %02x %02x %02x %02x %02x\n", (unsigned long)i, (unsigned long)(j - i), q[3], q[2], q[1], q[0], copy + nm, (unsigned long)(i - best), i && w.owners && w.owners[i - 1] ? w.owners[i - 1] : "?", copy[i], copy[i + 1], copy[i + 2], copy[i + 3], i + 4 < size ? copy[i + 4] : 0, i + 5 < size ? copy[i + 5] : 0); }
                            ++listed;
                        }
                        i = j;
                    }
                }
                region_geometry = geometry_marked;
                for (i = 0; i < size;) {
                    if (mac_xbox_walk_is_covered(&w, i)) { ++i; continue; }
                    { uint32_t j = i; while (j < size && !mac_xbox_walk_is_covered(&w, j)) ++j;
                      if (j - i < 4) gaps_small += j - i;
                      else {
                          uint32_t q, filler = 1;
                          for (q = i; q < j && filler; ++q) if (copy[q] != 0 && copy[q] != 0xff) filler = 0;
                          if (filler) { ++gaps_fill; gap_bytes_fill += j - i; }
                          else { ++gaps_big; gap_bytes_big += j - i; if (j - i > biggest) { biggest = j - i; biggest_at = i; } }
                      }
                      i = j; }
                }
            }
            /* the conversion: on a copy, as the engine's hook does it (xbox_swap.c), then read back as big endian */
            mac_xbox_geometry_set_visitor(NULL);
            if (ok && do_convert) {
                struct mac_xbox_walk c;
                unsigned char *conv = (unsigned char *)malloc(size);
                int big_host = 0;
                { union { uint32_t v; unsigned char b[4]; } u; u.v = 1; big_host = u.b[0] == 0; }
                memcpy(conv, map.data + offset, size);
                if (!big_host) mac_xbox_geometry_force(1);
                if (!big_host) { if (b) mac_xbox_geometry_bsp_to_native(conv, size, &info.bsps[b - 1], &error); else mac_xbox_geometry_tags_to_native(conv, size, &info, &error); }
                memset(&c, 0, sizeof(c));
                if (!mac_xbox_walk_begin(&c, conv, base, size, 1)) { ok = 0; }
                else {
                    int good = b ? mac_xbox_walk_bsp(&c, &info.bsps[b - 1]) : mac_xbox_walk_tags(&c, &info);
                    if (!good) { printf("%s: region %lu: the conversion refused: %s\n", argv[a], (unsigned long)b, c.error); ok = 0; }
                    mac_xbox_walk_end(&c);
                }
                if (ok && big_host) { if (b) mac_xbox_geometry_bsp_to_native(conv, size, &info.bsps[b - 1], &error); else mac_xbox_geometry_tags_to_native(conv, size, &info, &error); }
                mac_xbox_geometry_force(0);
                if (ok) {
                    struct mac_xbox_walk v;
                    memset(&v, 0, sizeof(v)); v.read_big = 1;
                    if (!mac_xbox_walk_begin(&v, conv, base, size, 0)) ok = 0;
                    else {
                        int good = b ? mac_xbox_walk_bsp(&v, &info.bsps[b - 1]) : mac_xbox_walk_tags(&v, &info);
                        if (!good || v.elements != w.elements || v.bytes_covered + region_geometry != w.bytes_covered || v.floats != w.floats || v.none_floats != w.none_floats ||
                            v.opaque_elements != w.opaque_elements || v.file_data != w.file_data) {
                            printf("%s: region %lu: the converted tags do not read as the map's do (%s; %lu/%lu structures, %lu/%lu bytes)\n", argv[a], (unsigned long)b, v.error ? v.error : "counts",
                                (unsigned long)v.elements, (unsigned long)w.elements, (unsigned long)v.bytes_covered, (unsigned long)(w.bytes_covered - region_geometry));
                            ok = 0;
                        }
                        mac_xbox_walk_end(&v);
                    }
                }
                { uint32_t h = 2166136261u, k; for (k = 0; k < size; ++k) h = (h ^ conv[k]) * 16777619u; hashes ^= h * (b + 1); }
                free(conv);
            }
            total_bytes += size; covered += w.bytes_covered; elements += w.elements; floats += w.floats; nones += w.none_floats; bad_floats += w.bad_floats; huge += w.huge_floats;
            unread += w.unread_tags; opaque += w.opaque_blocks; opaque_el += w.opaque_elements; files += w.file_data;
            if (w.bad_float_layout) printf("  first non-finite float: %s at %08lx = %08lx\n", w.bad_float_layout, (unsigned long)(w.virtual_base + w.bad_float_offset), (unsigned long)w.bad_float_bits);
            free(w.owners);
            mac_xbox_walk_end(&w);
            free(copy);
        }
        if (bad_floats) ok = 0;
        printf("%s: %s: %lu structures, %lu floats (%lu not finite, %lu all ones), %lu unread blocks (%lu elements), %lu tags of groups the engine does not read, converted data hash %08lx\n",
            argv[a], ok ? "ok" : "FAILED", (unsigned long)elements, (unsigned long)floats, (unsigned long)bad_floats, (unsigned long)nones, (unsigned long)opaque, (unsigned long)opaque_el,
            (unsigned long)unread, (unsigned long)hashes);
        if (show_coverage)
            printf("    coverage: %lu of %lu bytes walked (%.3f%%), padding under 4 bytes: %lu, zero or ff fill: %lu gaps (%lu bytes), other: %lu gaps (%lu bytes, largest %lu at %08lx), %lu data fields in the map file, %lu floats beyond 1e10\n",
                (unsigned long)covered, (unsigned long)total_bytes, 100.0 * covered / total_bytes, (unsigned long)gaps_small, (unsigned long)gaps_fill, (unsigned long)gap_bytes_fill,
                (unsigned long)gaps_big, (unsigned long)gap_bytes_big, (unsigned long)biggest, (unsigned long)biggest_at, (unsigned long)files, (unsigned long)huge);
        if (!ok) ++bad;
        mac_mapped_file_close(&map);
    }
    if (verbose) {
        unsigned i;
        for (i = 0; i < mac_xbox_tag_layout_count; ++i)
            if (layout_elements[i]) printf("  layout %-40s %9lu structures %12lu bytes\n", mac_xbox_tag_layouts[i].name, (unsigned long)layout_elements[i], (unsigned long)layout_bytes[i]);
    }
    if (mac_xbox_walk_bytes_probe) bytes_report();
    return bad ? 1 : 0;
}
