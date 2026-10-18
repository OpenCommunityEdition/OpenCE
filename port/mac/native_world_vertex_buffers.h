#ifndef HALO_MAC_NATIVE_WORLD_VERTEX_BUFFERS_H
#define HALO_MAC_NATIVE_WORLD_VERTEX_BUFFERS_H
struct structure_bsp;
/* Create and retire the real rasterizer vertex buffers for the converted BSP.
   CPU compressed_vertex_data remains owned by the world for visibility. */
int mac_native_world_vertex_buffers_prepare(struct structure_bsp *);
void mac_native_world_vertex_buffers_dispose(struct structure_bsp *);
/* An Xbox map's bsp: its buffers (the map's own, in the tag cache) get the
   owned ranges and certificates the draw path's world cache keys on. */
void mac_native_world_vertex_buffers_certify_xbox(struct structure_bsp *);
void mac_native_world_vertex_buffers_release_xbox(struct structure_bsp *);
#endif
