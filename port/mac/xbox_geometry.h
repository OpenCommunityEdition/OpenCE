#ifndef HALO_MAC_XBOX_GEOMETRY_H
#define HALO_MAC_XBOX_GEOMETRY_H
#include <stdint.h>
#include "xbox_map.h"

/* The raw geometry an Xbox map keeps outside its tag structures: vertex and index
   data that the engine and the GPU path read as machine words. In the map it is
   little endian. These convert it in place to the host's order (a no-op on a
   little-endian host; HALO_MAC_XBOX_FORCE_SWAP=1 reverses the bytes of every
   word there too, so that a test on an Intel Mac sees what a PowerPC sees).

   Formats (all vertex data is compressed in a map):
     model part vertex (32 bytes):  6 x 32-bit (position xyz as floats, normal,
         binormal, tangent as packed 11:11:10), 2 x 16-bit (texcoord), 2 bytes
         (the node indices, left alone), 1 x 16-bit (node weight)
     bsp environment vertex (32 bytes): 8 x 32-bit (floats; the three packed
         vectors; texcoord floats)
     bsp lightmap vertex (8 bytes): 1 x 32-bit (packed incident radiosity),
         2 x 16-bit (lightmap u, v)
     model part indices: 16-bit, (count + 2) of them (a strip)
   A buffer can be named by more than one descriptor: each address is converted
   once (a table of the addresses done is kept per call).

   They must run after the tags and the buffer descriptor tables are in the host's
   order (the tag tables, xbox_swap.h) and before anything reads the vertices:
   mac_cache_files_read calls them right after mac_xbox_tag_data_to_native and
   mac_xbox_bsp_to_native. Return 1 on success; 0 (with *error) if a count or an
   address of the map lies outside the tag cache it was read into. */
int mac_xbox_geometry_swap_enabled(void);
/* converts on a little-endian host too, for as long as it is on (the self test of xbox_swap.c, on a copy) */
void mac_xbox_geometry_force(int on);
int mac_xbox_geometry_tags_to_native(unsigned char *data, uint32_t size,
    const struct mac_xbox_map_info *info, const char **error);
int mac_xbox_geometry_bsp_to_native(unsigned char *data, uint32_t size,
    const struct mac_xbox_bsp *bsp, const char **error);

/* the converters on their own (the buffer is count elements) */
void mac_xbox_swap_model_vertices(void *p, uint32_t count);
void mac_xbox_swap_environment_vertices(void *p, uint32_t count);
void mac_xbox_swap_lightmap_vertices(void *p, uint32_t count);
void mac_xbox_swap_indices16(void *p, uint32_t count);
/* tests: called with each buffer before it is converted (kind: the vertex type, or 100 for model indices) */
void mac_xbox_geometry_set_visitor(void (*proc)(int kind, uint32_t address, uint32_t count));
/* how many buffers the last call converted, for tests */
void mac_xbox_geometry_counts(uint32_t *model_vertices, uint32_t *environment, uint32_t *lightmap, uint32_t *indices);
#endif
