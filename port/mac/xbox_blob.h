#ifndef HALO_MAC_XBOX_BLOB_H
#define HALO_MAC_XBOX_BLOB_H
#include <stddef.h>
#include <stdint.h>

/* The raw data blobs (tag_data fields) of an Xbox map's tags that the engine reads as
   machine words, with the layouts the engine reads them in. In a map they are little
   endian; these convert one blob in place to the host's order (no-ops on a
   little-endian host unless HALO_MAC_XBOX_FORCE_SWAP=1, which reverses the bytes of
   each word there too, so that a test on Intel sees what PowerPC sees). The tag tables
   (xbox_swap.h) call them for the data fields of these groups once the tag's own fields
   are native (the sizes and flags below come from them):

     scenario  hs_syntax_data            mac_xbox_swap_hs_syntax: the script data array (a 0x38 header and
                                         19001 nodes of 20 bytes: cache_endian.c's codec)
     scenario  recorded_animations[].event_stream   mac_xbox_swap_event_stream
     animation_graph animations[] frame_info / default_data / data   mac_xbox_swap_animation
     structure_bsp cluster_data          mac_xbox_swap_words32: PVS bit vectors, 32-bit words
     unicode_string_list strings[].string, hud_message_text text_data   mac_xbox_swap_words16: UTF-16
   Bytes need nothing: hs_string_constants, sound_cluster_data (byte encoded), sound mouth and
   subtitle data, font pixels, the bitmaps' pixels and the sounds' samples (read as bytes). The
   vertex and index data are xbox_geometry.h's. Each returns 1; 0 with *error for a layout that
   does not fit its size (nothing is changed then).

   A recorded animation's event header is a byte of two bit fields (time_delta : 2, event_type : 6,
   the first in the low bits as the console's compiler lays them out). On a big-endian host GCC lays
   the first field in the high bits, so the conversion moves each header's bits to match. */
void mac_xbox_swap_words16(void *p, uint32_t bytes);
void mac_xbox_swap_words32(void *p, uint32_t bytes);
int mac_xbox_swap_hs_syntax(void *p, uint32_t bytes, const char **error);

struct mac_xbox_animation {
    int16_t frame_count, frame_size, frame_info_type, node_count;
    uint16_t flags;           /* bit 0: compressed (a cinematic's blob) */
    uint32_t translation_flags[2], rotation_flags[2], scale_flags[2];
};
/* the three data of one animation (any may be empty); the node flags are the animation's own */
int mac_xbox_swap_animation(const struct mac_xbox_animation *animation,
    void *frame_info, uint32_t frame_info_bytes, void *defaults, uint32_t default_bytes,
    void *data, uint32_t data_bytes, const char **error);
/* one recorded animation's event stream (version: its unit_control_data_version) */
int mac_xbox_swap_event_stream(void *p, uint32_t bytes, unsigned version, const char **error);

/* tests: every range converted is reported (what: a name; unit: 1, 2 or 4 bytes, or 0 for
   the bit-field byte), before it is converted */
void mac_xbox_blob_set_visitor(void (*proc)(const char *what, uint32_t offset, uint32_t bytes, unsigned unit, const void *base));
#endif
