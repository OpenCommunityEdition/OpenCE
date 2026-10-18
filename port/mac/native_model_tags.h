#ifndef HALO_MAC_NATIVE_MODEL_TAGS_H
#define HALO_MAC_NATIVE_MODEL_TAGS_H
#include "retail_cache.h"
struct model;
struct vertex_buffer;
struct triangle_buffer;
struct model_vertex_uncompressed;
/* These callbacks must create/release genuine renderer resources. The vertex
   callback receives model-global node indices and must compress with the
   engine routine before constructing its hardware buffer. No dummy service
   or CPU pointer may masquerade as hardware_format. */
struct mac_native_model_buffers {
    void *context;
    int (*create)(void *,struct vertex_buffer *,struct triangle_buffer *,
        const struct model_vertex_uncompressed *,uint32_t,const uint16_t *,uint32_t);
    size_t (*bytes_owned)(void *,const struct vertex_buffer *,const struct triangle_buffer *);
    void (*destroy)(void *,struct vertex_buffer *,struct triangle_buffer *);
};
struct mac_native_model_tag {
    void *owner;
    /* NULL until all real hardware buffers have been made successfully. */
    struct model *root;
    uint32_t handle,geometry_count,part_count,vertex_count,index_count;
    size_t root_bytes,bytes_owned;
};
/* Complete owned CPU conversion; no published engine root yet. Unused padding
   is zero. Nonempty cache-only debug vertex/triangle blocks are rejected.
   Shader handles/names are validated and owned, but their payloads remain
   dependencies. Models above the renderer's node palette limit require a
   separate palette bridge and are rejected. */
int mac_native_model_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_model_tag *,const char **);
/* Read-only staging inspection, explicitly not a registry-publication root. */
const struct model *mac_native_model_staging(const struct mac_native_model_tag *);
typedef int (*mac_native_model_stream_visitor)(void *, const struct model_vertex_uncompressed *,
    uint32_t, const uint16_t *, uint32_t);
int mac_native_model_visit_streams(const struct mac_native_model_tag *,
    mac_native_model_stream_visitor, void *);
/* Enumerate shaders from an unchanged owned native model. References are
   already validated by decode; their payload ownership remains separate. */
typedef int (*mac_native_model_shader_visitor)(void *,uint32_t,uint32_t);
int mac_native_model_visit_shaders(const struct model *,
    mac_native_model_shader_visitor,void *);
int mac_native_model_finalize(struct mac_native_model_tag *,
    const struct mac_native_model_buffers *,const char **);
void mac_native_model_owner_free(void *);
void mac_native_model_tag_free(struct mac_native_model_tag *);
#endif
