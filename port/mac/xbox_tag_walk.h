#ifndef HALO_MAC_XBOX_TAG_WALK_H
#define HALO_MAC_XBOX_TAG_WALK_H
#include <stdint.h>
#include "xbox_map.h"

/* Walks an Xbox map's tags (or one bsp) with the layout tables, converting each field to the host's
   order, or only reading them (swap = 0, the checker). Everything it follows is bounds-checked
   against the region it was given (the tag data, or a bsp, at its virtual address), every byte it
   converts is marked in a bitmap and a byte met twice refuses the map, and its depth and the number
   of elements it visits are limited. Values are read as little-endian bytes, whatever the host. */
struct mac_xbox_walk {
    unsigned char *base;        /* the region */
    uint32_t virtual_base, size;
    int swap;                   /* convert in place (else read only) */
    int read_big;               /* the words are big endian (already converted): the verifying pass */
    unsigned char *covered;     /* a bit for each byte of the region */
    /* results */
    const char *error;          /* why the walk refused the map */
    uint32_t error_offset;
    const char *error_layout;
    const char *error_other;    /* with owners: the layout that walked the byte first */
    const char **owners;        /* optional, for the checker: the layout that walked each byte */
    uint32_t elements;          /* structures visited */
    uint32_t bytes_covered;
    uint32_t floats, bad_floats, huge_floats, none_floats;
    uint32_t unread_tags;       /* tags of a group the engine does not read (no layout) */
    const char *bad_float_layout;   /* the first float that is not finite */
    uint32_t bad_float_offset, bad_float_bits;
    uint32_t opaque_blocks, opaque_elements;   /* blocks the engine never reads, non-empty, and their elements */
    uint32_t file_data;         /* data fields whose bytes are in the map file, not in the tags */
    uint32_t *layout_bytes;     /* per layout: bytes of tags walked with it (if not NULL) */
    uint32_t *layout_elements;
};
/* (the caller may set layout_bytes, layout_elements and read_big before it) */
int mac_xbox_walk_begin(struct mac_xbox_walk *w, unsigned char *base, uint32_t virtual_base, uint32_t size, int swap);
void mac_xbox_walk_end(struct mac_xbox_walk *w);
/* the tag header, tag instances, names and buffer tables, then every tag of a group the tables know */
int mac_xbox_walk_tags(struct mac_xbox_walk *w, const struct mac_xbox_map_info *info);
/* a bsp read to its virtual address: its header, buffer tables and structure_bsp */
int mac_xbox_walk_bsp(struct mac_xbox_walk *w, const struct mac_xbox_bsp *bsp);
/* bytes of the region not covered by anything walked, for the checker */
uint32_t mac_xbox_walk_uncovered(const struct mac_xbox_walk *w);
int mac_xbox_walk_is_covered(const struct mac_xbox_walk *w, uint32_t offset);
/* bytes something else converted (the geometry's vertices and indices), so that they count as walked */
int mac_xbox_walk_mark(struct mac_xbox_walk *w, uint32_t offset, uint32_t bytes);
/* for the checker: told of every run of bytes the tables leave as bytes (padding, reserved, names, byte arrays) */
extern void (*mac_xbox_walk_bytes_probe)(const char *layout, uint32_t op_offset, uint32_t count, const unsigned char *bytes);
#endif
