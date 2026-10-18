#ifndef HALO_MAC_XBOX_TAG_TABLES_H
#define HALO_MAC_XBOX_TAG_TABLES_H
#include <stdint.h>

/* The layout of every tag group of an Xbox map, generated (tools/mac_xbox_tag_tables.py) from the
   engine's own tag structures, as the compiler lays them out for the console, and the validator's
   schemas (port/linux/game/tag_schema_*.c). A layout is the structure's size and a list of ops that
   cover every one of its bytes, in order: runs of scalars (bytes, 16-bit, 32-bit, floats, 64-bit),
   the tag's blocks, data and references, and arrays of structures. xbox_tag_walk.c follows them. */
enum {
    MAC_XBOX_OP_BYTES, MAC_XBOX_OP_SWAP16, MAC_XBOX_OP_SWAP32, MAC_XBOX_OP_FLOAT32, MAC_XBOX_OP_SWAP64,
    MAC_XBOX_OP_BLOCK,   /* a tag_block (12 bytes): arg is its element's layout, -1 if the engine never reads it */
    MAC_XBOX_OP_DATA,    /* a tag_data (20 bytes): arg is the kind of data (MAC_XBOX_DATA_*) */
    MAC_XBOX_OP_REF,     /* a tag_reference (16 bytes) */
    MAC_XBOX_OP_ARRAY    /* count structures of layout arg, one after another */
};
enum {
    MAC_XBOX_DATA_BYTES, MAC_XBOX_DATA_HS_SYNTAX, MAC_XBOX_DATA_EVENTS, MAC_XBOX_DATA_ANIM_INFO,
    MAC_XBOX_DATA_ANIM_DEFAULT, MAC_XBOX_DATA_ANIM_DATA, MAC_XBOX_DATA_WORDS32, MAC_XBOX_DATA_WORDS16
};
struct mac_xbox_tag_op { uint8_t kind; uint16_t count; uint32_t offset; int32_t arg; };
struct mac_xbox_tag_layout { uint32_t size; uint32_t first_op; uint32_t op_count; const char *name; };
struct mac_xbox_tag_group { uint32_t tag; int layout; };
/* the offsets the blob hooks read in the animation and recorded animation structures */
struct mac_xbox_tag_hooks {
    int animation_layout;
    uint32_t frame_count, frame_size, frame_info_type, node_count, flags, translation_flags, rotation_flags, scale_flags,
        frame_info, default_data, data;
    int recorded_animation_layout;
    uint32_t unit_control_data_version, event_stream;
};
extern const struct mac_xbox_tag_op mac_xbox_tag_ops[];
extern const struct mac_xbox_tag_layout mac_xbox_tag_layouts[];
extern const unsigned mac_xbox_tag_layout_count;
extern const struct mac_xbox_tag_group mac_xbox_tag_groups[];
extern const unsigned mac_xbox_tag_group_count;
extern const struct mac_xbox_tag_hooks mac_xbox_tag_hook_offsets;
#endif
