/* Reads real Xbox maps' tags the way the engine does, through its own structures, after they are converted
   (on a big-endian host) or as they are (little endian), and prints what it finds: the scenario's start points,
   trigger volumes and placements, weapons, bitmaps, models, animations (with the blobs), bsp surfaces and
   visibility data, strings and scripts. Run on an Intel Mac and on a PowerPC (under Rosetta) the two
   outputs must be identical: that is the engine's own reading agreeing in both byte orders.
   usage: mac_xbox_tag_values map... */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cseries.h"
#include "real_math.h"
#include "tag_files/tag_groups.h"
#include "memory/data.h"
#include "hs/hs_scenario_definitions.h"
struct location;
#include "scenario/scenario_definitions.h"
#include "scenario/sky_definitions.h"
#include "cutscene/recorded_animation_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "items/weapon_definitions.h"
#include "bitmaps/bitmap_group.h"
#include "models/model_definitions.h"
#include "models/model_animation_definitions.h"
#include "text/text_group.h"
#include "xbox_map.h"
#include "xbox_tag_walk.h"
#include "xbox_geometry.h"
#undef printf
#undef memcpy
#undef memset
#undef free
#undef malloc
#undef calloc

/* (the conversions of xbox_blob.c ask whether the host needs them) */
int mac_xbox_geometry_swap_enabled(void) { union { uint32_t v; unsigned char c[4]; } u; u.v = 1; return u.c[0] == 0; }

static unsigned char *region;
static uint32_t base, region_size;
#define P(address) ((void *)(region + ((uint32_t)(address) - base)))
#define BLOCK(b, type, i) ((type *)P((uint32_t)(b).address) + (i))
static int big_host(void) { union { uint32_t v; unsigned char c[4]; } u; u.v = 1; return u.c[0] == 0; }
static void real_out(const char *key, real v) { printf("%s %.9g\n", key, (double)v); }

static void dump_scenario(struct scenario *sc)
{
    long i;
    printf("scenario type %d flags %u north %.9g\n", (int)sc->type, (unsigned)sc->flags, (double)sc->local_north);
    printf("counts scenery %ld bipeds %ld vehicles %ld weapons %ld players %ld trigger_volumes %ld netgame_flags %ld hs_scripts %ld bsps %ld\n",
        (long)sc->scenery.count, (long)sc->bipeds.count, (long)sc->vehicles.count, (long)sc->weapons.count, (long)sc->players.count,
        (long)sc->trigger_volumes.count, (long)sc->netgame_flags.count, (long)sc->hs_scripts.count, (long)sc->structure_bsp_references.count);
    for (i = 0; i < sc->players.count && i < 40; ++i) {
        struct player_starting_location *p = BLOCK(sc->players, struct player_starting_location, i);
        printf("start %ld %.9g %.9g %.9g facing %.9g team %d bsp %d types %d %d %d %d\n", i, (double)p->position.x, (double)p->position.y, (double)p->position.z,
            (double)p->facing, (int)p->team_index, (int)p->structure_bsp_reference_index, p->game_types[0], p->game_types[1], p->game_types[2], p->game_types[3]);
    }
    for (i = 0; i < sc->trigger_volumes.count && i < 40; ++i) {
        struct scenario_trigger_volume *t = BLOCK(sc->trigger_volumes, struct scenario_trigger_volume, i);
        printf("volume %ld %s type %d pos %.9g %.9g %.9g ext %.9g %.9g %.9g fwd %.9g %.9g %.9g\n", i, t->name, (int)t->type, (double)t->position.x, (double)t->position.y,
            (double)t->position.z, (double)t->extents.i, (double)t->extents.j, (double)t->extents.k, (double)t->forward.i, (double)t->forward.j, (double)t->forward.k);
    }
    for (i = 0; i < sc->netgame_flags.count && i < 40; ++i) {
        struct scenario_netgame_flag *f = BLOCK(sc->netgame_flags, struct scenario_netgame_flag, i);
        printf("flag %ld %.9g %.9g %.9g facing %.9g type %d team %d\\n", i, (double)f->position.x, (double)f->position.y, (double)f->position.z, (double)f->facing, (int)f->type, (int)f->team_index);
    }
    for (i = 0; i < sc->scenery.count && i < 40; ++i) {
        /* (the placement is 0x48 bytes, a scenario_object_datum first) */
        struct scenario_object_datum *o = (struct scenario_object_datum *)((unsigned char *)P((uint32_t)sc->scenery.address) + i * 0x48);
        printf("scenery %ld palette %d name %d flags %u pos %.9g %.9g %.9g rot %.9g %.9g %.9g\n", i, (int)o->palette_entry_index, (int)o->name_index, (unsigned)o->placement_flags,
            (double)o->position.x, (double)o->position.y, (double)o->position.z, (double)o->rotation.yaw, (double)o->rotation.pitch, (double)o->rotation.roll);
    }
    for (i = 0; i < sc->hs_scripts.count && i < 60; ++i) {
        struct hs_script *h = BLOCK(sc->hs_scripts, struct hs_script, i);
        printf("script %ld %s type %d return %d root %ld\n", i, h->name, (int)h->script_type, (int)h->return_type, (long)h->root_expression_index);
    }
    for (i = 0; i < sc->structure_bsp_references.count && i < 16; ++i) {
        struct scenario_structure_bsp_reference *r = BLOCK(sc->structure_bsp_references, struct scenario_structure_bsp_reference, i);
        printf("bsp %ld file %ld size %ld base %08lx tag %08lx\n", i, (long)r->file_offset, (long)r->file_size, (unsigned long)r->base_address, (unsigned long)r->structure_bsp.index);
    }
    /* the script data array: its header and the first nodes, as the engine reads them */
    if (sc->hs_syntax_data.size >= 0x38) {
        unsigned char *d = (unsigned char *)P((uint32_t)sc->hs_syntax_data.address);
        struct data_array *header = (struct data_array *)d;
        struct hs_syntax_node *nodes = (struct hs_syntax_node *)(d + 0x38);
        printf("syntax size %ld capacity %d count %d first free %d\n", (long)sc->hs_syntax_data.size, (int)header->maximum_count, (int)header->actual_count, (int)header->first_free_absolute_index);
        for (i = 0; i < 24; ++i)
            printf("node %ld header %d index %d type %d flags %d next %ld value %08lx string %08lx\n", i, (int)nodes[i].datum_header, (int)nodes[i].index, (int)nodes[i].type, (int)nodes[i].flags,
                (long)nodes[i].next_node_index, (unsigned long)nodes[i].data, (unsigned long)nodes[i].string_offset);
    }
    /* the first recorded animation's event stream */
    if (sc->recorded_animations.count > 0) {
        long k;
        for (k = 0; k < sc->recorded_animations.count && k < 4; ++k) {
            struct recorded_animation_definition *a = BLOCK(sc->recorded_animations, struct recorded_animation_definition, k);
            const unsigned char *s = (const unsigned char *)P((uint32_t)a->event_stream.address);
            uint32_t n = (uint32_t)a->event_stream.size, j;
            printf("recorded %ld %s version %d ticks %d bytes %lu:", k, a->name, (int)a->unit_control_data_version, (int)a->length_in_ticks, (unsigned long)n);
            /* the unit control read natively (shorts, floats) and the first event's header through the engine's bit-field layout */
            if (n >= 80) {
                unsigned v = (unsigned)a->unit_control_data_version, at = 52 + (v >= 2 ? 4 : 0) + (v >= 3 ? 2 : 0) + (v >= 4 ? 2 : 0) + 12;
                struct { unsigned char time_delta : 2; unsigned char event_type : 6; } const *h = (const void *)(s + at);
                printf(" control %d %d %d %.4f %.4f %.4f", (int)((const short *)(s + 2))[0], (int)((const short *)(s + 2))[1], (int)((const short *)(s + 2))[2],
                    (double)((const float *)(s + 8))[0], (double)((const float *)(s + 8))[1], (double)((const float *)(s + 8))[2]);
                printf(" first header at %u time_delta %d event_type %d", at, (int)h->time_delta, (int)h->event_type);
            }
            (void)j;
            printf("\n");
        }
    }
}

static void dump_bsp(const struct mac_xbox_bsp *bsp, unsigned char *data)
{
    struct structure_bsp *b;
    uint32_t root = *(uint32_t *)data;
    long i;
    b = (struct structure_bsp *)(data + (root - bsp->base));
    printf("sbsp clusters %ld surfaces %ld leaves %ld lightmaps %ld cluster_data %ld sound_cluster_data %ld\n", (long)b->clusters.count, (long)b->surfaces.count, (long)b->leaves.count,
        (long)b->lightmaps.count, (long)b->cluster_data.size, (long)b->sound_cluster_data.size);
    for (i = 0; i < b->surfaces.count && i < 24; ++i) {
        struct structure_surface *s = (struct structure_surface *)(data + ((uint32_t)b->surfaces.address - bsp->base)) + i;
        printf("surface %ld %u %u %u\n", i, (unsigned)s->vertex_indices[0], (unsigned)s->vertex_indices[1], (unsigned)s->vertex_indices[2]);
    }
    if (b->cluster_data.size >= 16) {
        const unsigned long *w = (const unsigned long *)(data + ((uint32_t)b->cluster_data.address - bsp->base));
        printf("pvs %08lx %08lx %08lx %08lx\n", w[0], w[1], w[2], w[3]);
    }
    for (i = 0; i < b->lightmaps.count && i < 4; ++i) {
        struct structure_lightmap *l = (struct structure_lightmap *)(data + ((uint32_t)b->lightmaps.address - bsp->base)) + i;
        long m;
        printf("lightmap %ld bitmap %d materials %ld\n", i, (int)l->bitmap_index, (long)l->materials.count);
        for (m = 0; m < l->materials.count && m < 3; ++m) {
            struct structure_material *mat = (struct structure_material *)(data + ((uint32_t)l->materials.address - bsp->base)) + m;
            printf("  material %ld shader %08lx surfaces %ld+%ld centroid %.9g %.9g %.9g vertices %ld/%ld\n", m, (unsigned long)mat->shader.index, (long)mat->first_surface_index, (long)mat->surface_count,
                (double)mat->centroid.x, (double)mat->centroid.y, (double)mat->centroid.z, (long)mat->vertices.count, (long)mat->lightmap_vertices.count);
        }
    }
}

static void dump_tags(unsigned char *tags)
{
    uint32_t table = *(uint32_t *)tags, count = *(uint32_t *)(tags + 0xc), t, weapons = 0, bitmaps = 0, models = 0, graphs = 0, strings = 0;
    for (t = 0; t < count; ++t) {
        uint32_t *inst = (uint32_t *)P(table + t * 32);
        uint32_t group = inst[0], root = inst[5];
        const char *name = (const char *)P(inst[4]);
        if (!root) continue;
        if (group == 0x73636e72) dump_scenario((struct scenario *)P(root));
        else if (group == 0x736b7920) {
            /* a sky: its fog colors and densities (struct sky, sky_definitions.h) and its lights' directions (render_sky.c's struct sky_light, 116 bytes) */
            struct sky *k = (struct sky *)P(root);
            const unsigned char *raw = (const unsigned char *)k;
            uint32_t lights = *(const uint32_t *)(raw + 196), light_count = *(const uint32_t *)(raw + 192), l;
            printf("sky %s outdoor fog %.9g %.9g %.9g density %.9g %.9g %.9g indoor fog %.9g %.9g %.9g density %.9g %.9g %.9g\n", name,
                (double)k->outdoor_fog.color.red, (double)k->outdoor_fog.color.green, (double)k->outdoor_fog.color.blue, (double)k->outdoor_fog.maximum_density,
                (double)k->outdoor_fog.start_distance, (double)k->outdoor_fog.opaque_distance, (double)k->indoor_fog.color.red, (double)k->indoor_fog.color.green,
                (double)k->indoor_fog.color.blue, (double)k->indoor_fog.maximum_density, (double)k->indoor_fog.start_distance, (double)k->indoor_fog.opaque_distance);
            for (l = 0; l < light_count && l < 8 && lights; ++l) {
                const float *d = (const float *)((const unsigned char *)P(lights) + l * 116 + 104);
                printf("sky light %lu direction %.9g %.9g\n", (unsigned long)l, (double)d[0], (double)d[1]);
            }
        }
        else if (group == 0x77656170 && weapons < 3) {
            struct weapon_definition *w = (struct weapon_definition *)P(root);
            ++weapons;
            printf("weapon %s object type %d flags %u radius %.9g offset %.9g %.9g %.9g item flags %lu weapon flags %lu label %s ready %.9g heat %.9g %.9g magazines %ld triggers %ld\n", name,
                (int)w->object.type, (unsigned)w->object.flags, (double)w->object.bounding_radius, (double)w->object.bounding_offset.x, (double)w->object.bounding_offset.y,
                (double)w->object.bounding_offset.z, (unsigned long)w->item.flags, (unsigned long)w->weapon.flags, w->weapon.label, (double)w->weapon.ready_time,
                (double)w->weapon.heat_recovery_threshold, (double)w->weapon.heat_overheated_threshold, (long)w->weapon.magazines.count, (long)w->weapon.triggers.count);
        } else if (group == 0x6269746d && bitmaps < 12) {
            struct bitmap_group *g = (struct bitmap_group *)P(root);
            long i;
            ++bitmaps;
            printf("bitmap %s type %d format %d flags %u bitmaps %ld\n", name, (int)g->type, (int)g->format, (unsigned)g->flags, (long)g->bitmaps.count);
            for (i = 0; i < g->bitmaps.count && i < 3; ++i) {
                struct bitmap_data *d = (struct bitmap_data *)P((uint32_t)g->bitmaps.address) + i;
                printf("  data %ld %dx%d depth %d type %d format %d flags %u mips %d pixels %ld+%ld\n", i, (int)d->width, (int)d->height, (int)d->depth, (int)d->type, (int)d->format,
                    (unsigned)d->flags, (int)d->mipmap_count, (long)d->pixels_offset, (long)d->pixels_size);
            }
        } else if (group == 0x6d6f6465 && models < 6) {
            struct model *m = (struct model *)P(root);
            long i;
            ++models;
            printf("model %s nodes %ld regions %ld geometries %ld shaders %ld markers %ld cutoffs %.9g %.9g scale %.9g %.9g\n", name, (long)m->nodes.count, (long)m->regions.count,
                (long)m->geometries.count, (long)m->shaders.count, (long)m->markers.count, (double)m->detail_cutoff_pixels[0], (double)m->detail_cutoff_pixels[1],
                (double)m->base_map_scale.i, (double)m->base_map_scale.j);
            for (i = 0; i < m->nodes.count && i < 3; ++i) {
                struct model_node *n = (struct model_node *)P((uint32_t)m->nodes.address) + i;
                printf("  node %ld %s sibling %d child %d parent %d t %.9g %.9g %.9g r %.9g %.9g %.9g %.9g\n", i, n->name, (int)n->next_sibling_node_index, (int)n->first_child_node_index,
                    (int)n->parent_node_index, (double)n->default_translation.x, (double)n->default_translation.y, (double)n->default_translation.z,
                    (double)n->default_rotation.v.i, (double)n->default_rotation.v.j, (double)n->default_rotation.v.k, (double)n->default_rotation.w);
            }
        } else if (group == 0x616e7472 && graphs < 4) {
            struct animation_graph *g = (struct animation_graph *)P(root);
            long i;
            ++graphs;
            printf("graph %s animations %ld nodes %ld overlays %ld seats %ld\n", name, (long)g->animations.count, (long)g->nodes.count, (long)g->object_overlays.count, (long)g->unit_seats.count);
            for (i = 0; i < g->animations.count && i < 6; ++i) {
                struct animation *a = (struct animation *)P((uint32_t)g->animations.address) + i;
                printf("  animation %ld %s type %d frames %d size %d info %d nodes %d flags %u sound %d defaults %ld data %ld infosz %ld\n", i, a->name, (int)a->type, (int)a->frame_count,
                    (int)a->frame_size, (int)a->frame_info_type, (int)a->node_count, (unsigned)a->flags, (int)a->sound_index, (long)a->default_data.size, (long)a->data.size, (long)a->frame_info.size);
                if (a->frame_info.size >= 8) { const real *f = (const real *)P((uint32_t)a->frame_info.address); printf("    info %.9g %.9g\n", (double)f[0], (double)f[1]); }
                if (a->data.size >= 16 && !(a->flags & 1)) {
                    /* frame 0, node by node: each node's rotation (four shorts), translation and scale (reals) that it animates */
                    const unsigned char *f = (const unsigned char *)P((uint32_t)a->data.address);
                    long node, shown = 0;
                    for (node = 0; node < a->node_count && node < 64 && shown < 4; ++node) {
                        unsigned long bit = 1UL << (node % 32);
                        int word = (int)(node / 32);
                        if (a->nodes_with_rotation_flags[word] & bit) { const short *r = (const short *)f; printf("    node %ld rotation %d %d %d %d\n", node, (int)r[0], (int)r[1], (int)r[2], (int)r[3]); f += 8; ++shown; }
                        if (a->nodes_with_translation_flags[word] & bit) { const real *t = (const real *)f; printf("    node %ld translation %.9g %.9g %.9g\n", node, (double)t[0], (double)t[1], (double)t[2]); f += 12; ++shown; }
                        if (a->nodes_with_scale_flags[word] & bit) { const real *t = (const real *)f; printf("    node %ld scale %.9g\n", node, (double)t[0]); f += 4; ++shown; }
                    }
                }
                if (a->data.size >= 16 && (a->flags & 1)) { const unsigned long *s = (const unsigned long *)P((uint32_t)a->data.address); printf("    compressed %08lx %08lx %08lx %08lx\n", s[0], s[1], s[2], s[3]); }
            }
        } else if (group == 0x75737472 && strings < 3) {
            struct string_list *l = (struct string_list *)P(root);
            long i, k;
            ++strings;
            printf("ustr %s strings %ld\n", name, (long)l->strings.count);
            for (i = 0; i < l->strings.count && i < 4; ++i) {
                struct string_list_entry *e = (struct string_list_entry *)P((uint32_t)l->strings.address) + i;
                const unsigned short *w = (const unsigned short *)P((uint32_t)e->string.address);
                printf("  %ld size %ld:", i, (long)e->string.size);
                for (k = 0; k < e->string.size / 2 && k < 16; ++k) printf(" %04x", (unsigned)w[k]);
                printf("\n");
            }
        }
    }
}

int main(int argc, char **argv)
{
    int a;
    for (a = 1; a < argc; ++a) {
        struct mac_mapped_file map;
        struct mac_xbox_map_info info;
        const char *error = NULL;
        uint32_t b;
        if (!mac_xbox_map_open(argv[a], &map, &error) || !mac_xbox_map_inspect(map.data, map.size, &info, &error)) { printf("%s: %s\n", argv[a], error); return 1; }
        printf("== %s\n", info.name);
        for (b = 0; b <= info.bsp_count && b < 2; ++b) {
            uint32_t offset = b ? info.bsps[b - 1].file_offset : info.tag_data_offset, size = b ? info.bsps[b - 1].size : info.tag_data_size;
            struct mac_xbox_walk w;
            region = (unsigned char *)malloc(size); region_size = size; base = b ? info.bsps[b - 1].base : MAC_XBOX_TAG_BASE;
            memcpy(region, map.data + offset, size);
            if (big_host()) {
                memset(&w, 0, sizeof(w));
                if (!mac_xbox_walk_begin(&w, region, base, size, 1)) return 1;
                if (!(b ? mac_xbox_walk_bsp(&w, &info.bsps[b - 1]) : mac_xbox_walk_tags(&w, &info))) { printf("conversion refused: %s\n", w.error); return 1; }
                mac_xbox_walk_end(&w);
            }
            if (b) dump_bsp(&info.bsps[b - 1], region); else dump_tags(region);
            free(region);
        }
        mac_mapped_file_close(&map);
    }
    return 0;
}
