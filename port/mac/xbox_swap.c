#include "xbox_swap.h"
#include "xbox_tag_walk.h"
#include "xbox_tag_tables.h"
#include "xbox_geometry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 1 once tools/mac_xbox_tag_check has passed every one of the 24 maps of the user's disc (bounds, overlaps,
   floats, coverage; both byte orders) and the conversions' checks have (tools/mac_xbox_*_checks.sh) */
#define XBOX_TABLES_VERIFIED 1

static int host_big(void)
{
    union { uint32_t value; unsigned char byte[4]; } u;
    u.value = 1;
    return u.byte[0] == 0;
}
static int env_is(const char *name, const char *value)
{
    const char *v = getenv(name);
    return v && !strcmp(v, value);
}

int mac_xbox_swap_supported(const char **why)
{
    if (!host_big()) return 1;
    if (env_is("HALO_MAC_XBOX_PPC", "refuse")) { if (why) *why = "Xbox maps are switched off on this processor (HALO_MAC_XBOX_PPC=refuse)"; return 0; }
    if (!XBOX_TABLES_VERIFIED && !getenv("HALO_MAC_XBOX_UNSWAPPED")) {
        if (why) *why = "Xbox maps' tag tables have not been verified for this processor";
        return 0;
    }
    return 1;
}

/* what a pass over a region found, for comparing the map's own reading with the converted data's */
struct summary { uint32_t elements, bytes, floats, opaque, opaque_elements, file_data, unread; int ok; };
static void summarize(const struct mac_xbox_walk *w, struct summary *s, int ok)
{
    s->elements = w->elements; s->bytes = w->bytes_covered; s->floats = w->floats; s->opaque = w->opaque_blocks;
    s->opaque_elements = w->opaque_elements; s->file_data = w->file_data; s->unread = w->unread_tags; s->ok = ok;
}
static int same(const struct summary *a, const struct summary *b)
{
    return a->ok && b->ok && a->elements == b->elements && a->bytes == b->bytes && a->floats == b->floats && a->opaque == b->opaque &&
        a->opaque_elements == b->opaque_elements && a->file_data == b->file_data && a->unread == b->unread;
}
static int run(unsigned char *base, uint32_t virtual_base, uint32_t size, const struct mac_xbox_map_info *info,
    const struct mac_xbox_bsp *bsp, int swap, int read_big, struct summary *s, const char **error)
{
    struct mac_xbox_walk w;
    int ok;
    memset(&w, 0, sizeof(w));
    w.read_big = read_big;
    if (!mac_xbox_walk_begin(&w, base, virtual_base, size, swap)) { if (error) *error = "no memory to walk the tags"; return 0; }
    ok = bsp ? mac_xbox_walk_bsp(&w, bsp) : mac_xbox_walk_tags(&w, info);
    if (!ok && error) {
        static char text[160];
        sprintf(text, "%s %s at %08lx (%s)", bsp ? "a structure bsp's" : "the tags'", w.error ? w.error : "?",
            (unsigned long)(w.virtual_base + w.error_offset), w.error_layout ? w.error_layout : "?");
        *error = text;
    }
    if (s) summarize(&w, s, ok);
    mac_xbox_walk_end(&w);
    return ok;
}

/* one region: tags (bsp NULL) or a bsp */
static int convert_region(unsigned char *data, uint32_t virtual_base, uint32_t size, const struct mac_xbox_map_info *info,
    const struct mac_xbox_bsp *bsp, const char **error)
{
    int big = host_big();
    int selftest = !big && env_is("HALO_MAC_XBOX_SELFTEST", "1");
    int verify = big && env_is("HALO_MAC_XBOX_VERIFY", "1");
    struct summary before, after;
    unsigned char *pristine = NULL, *scratch = NULL;
    int ok;
    if (!big && !selftest) return 1;
    if (error) *error = NULL;
    if (verify) {
        pristine = (unsigned char *)malloc(size);
        if (!pristine) { if (error) *error = "no memory to verify the conversion"; return 0; }
        memcpy(pristine, data, size);
        if (!run(pristine, virtual_base, size, info, bsp, 0, 0, &before, error)) { free(pristine); return 0; }
    }
    if (big) {
        ok = run(data, virtual_base, size, info, bsp, 1, 0, NULL, error);
        if (ok && verify) {
            ok = run(data, virtual_base, size, info, bsp, 0, 1, &after, error);
            if (ok && !same(&before, &after)) { ok = 0; if (error) *error = "the converted tags do not read as the map's do"; }
            if (ok) fprintf(stderr, "xbox swap verify ok: %s, %lu elements, %lu bytes, %lu floats\n", bsp ? "bsp" : "tags",
                (unsigned long)after.elements, (unsigned long)after.bytes, (unsigned long)after.floats);
        }
        free(pristine);
        return ok;
    }
    /* the self test: the same on a copy, on a host that needs none. The geometry's own conversion first
       (it reads the tags as they are), the tags next, then the verification */
    scratch = (unsigned char *)malloc(size);
    if (!scratch) { if (error) *error = "no memory for the self test"; return 0; }
    memcpy(scratch, data, size);
    mac_xbox_geometry_force(1);
    ok = run(scratch, virtual_base, size, info, bsp, 0, 0, &before, error);
    if (ok) {
        if (bsp) ok = mac_xbox_geometry_bsp_to_native(scratch, size, bsp, error);
        else ok = mac_xbox_geometry_tags_to_native(scratch, size, info, error);
    }
    if (ok) ok = run(scratch, virtual_base, size, info, bsp, 1, 0, NULL, error);
    if (ok) {
        ok = run(scratch, virtual_base, size, info, bsp, 0, 1, &after, error);
        if (ok && !same(&before, &after)) { ok = 0; if (error) *error = "the converted tags do not read as the map's do"; }
        if (ok) fprintf(stderr, "xbox swap selftest ok: %s, %lu elements, %lu bytes, %lu floats\n", bsp ? "bsp" : "tags",
            (unsigned long)after.elements, (unsigned long)after.bytes, (unsigned long)after.floats);
    }
    mac_xbox_geometry_force(0);
    free(scratch);
    return ok;
}

int mac_xbox_tag_data_to_native(unsigned char *data, uint32_t size, const struct mac_xbox_map_info *info, const char **error)
{
    return convert_region(data, MAC_XBOX_TAG_BASE, size, info, NULL, error);
}
int mac_xbox_bsp_to_native(unsigned char *data, uint32_t size, const struct mac_xbox_bsp *bsp,
    const struct mac_xbox_map_info *info, const char **error)
{
    return convert_region(data, bsp->base, size, info, bsp, error);
}

