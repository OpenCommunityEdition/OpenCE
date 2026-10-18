#ifndef HALO_MAC_VERTEX_UPLOAD_H
#define HALO_MAC_VERTEX_UPLOAD_H

#include <OpenGL/gl.h>
#include <stddef.h>

#define MAC_VERTEX_UPLOAD_ATTRIBUTES 16
#define MAC_VERTEX_UPLOAD_MAX_COUNT 65535UL

/* A context-owned compact upload buffer. The caller supplies the effective
   linked-attribute mask, not the shader declaration mask. Attribute pointers
   retain their buffer association after end(); callers must detach/reset those
   pointers before dispose. Calls require a current owner context and a clean
   GL error state. Dispose while the owning GL context is current. */
struct mac_vertex_upload {
    GLuint buffer;
    float *packed;
    size_t packed_capacity;
    GLint saved_array_buffer;
    unsigned long active_mask;
    unsigned long vertex_count;
    unsigned stride_bytes;
    int support_checked;
    int supported;
    int active;
};

struct mac_vertex_upload_layout {
    GLuint buffer;
    unsigned long active_mask;
    unsigned long vertex_count;
    unsigned stride_bytes;
    unsigned offsets[MAC_VERTEX_UPLOAD_ATTRIBUTES];
};

void mac_vertex_upload_init(struct mac_vertex_upload *upload);
int mac_vertex_upload_begin(struct mac_vertex_upload *upload,
    const float *full_vec4_records, size_t source_bytes,
    unsigned long vertex_count,
    unsigned long linked_attribute_mask, struct mac_vertex_upload_layout *layout,
    char *error, size_t error_size);
int mac_vertex_upload_end(struct mac_vertex_upload *upload);
int mac_vertex_upload_dispose(struct mac_vertex_upload *upload);
int mac_vertex_upload_supported(void);

#endif
