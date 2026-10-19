/* Bounded active-attribute compaction for the native GL stream path. */
#include "mac_vertex_upload.h"
#include <OpenGL/glext.h>
#undef malloc
#undef realloc
#undef free
#undef memcpy
#undef memset
#undef strlen
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAC_VERTEX_UPLOAD_MASK ((1UL << MAC_VERTEX_UPLOAD_ATTRIBUTES) - 1UL)
#define MAC_VERTEX_UPLOAD_MAX_BYTES ((size_t)MAC_VERTEX_UPLOAD_MAX_COUNT * \
    MAC_VERTEX_UPLOAD_ATTRIBUTES * 4U * sizeof(float))

static void upload_error(char *error, size_t size, const char *message)
{
    size_t length;
    if (!error || !size) return;
    length = strlen(message);
    if (length >= size) length = size - 1;
    memcpy(error, message, length);
    error[length] = '\0';
}

static unsigned active_count(unsigned long mask)
{
    unsigned count = 0;
    while (mask) { count += (unsigned)(mask & 1UL); mask >>= 1; }
    return count;
}

int mac_vertex_upload_supported(void)
{
    const char *version = (const char *)glGetString(GL_VERSION);
    unsigned major = 0, minor = 0;
    if (version) {
        while (*version >= '0' && *version <= '9') major = major * 10 + (unsigned)(*version++ - '0');
        if (*version == '.') {
            ++version;
            while (*version >= '0' && *version <= '9') minor = minor * 10 + (unsigned)(*version++ - '0');
        }
    }
    /* port: the upload calls OpenGL 1.5's own glGenBuffers, glBindBuffer, glBufferData
       and glBufferSubData. A driver of 1.4 with only GL_ARB_vertex_buffer_object has the
       ARB-named entries, and the core names are stubs there: it was accepted and then
       failed or crashed in the first call. */
    return major > 1 || (major == 1 && minor >= 5);
}

void mac_vertex_upload_init(struct mac_vertex_upload *upload)
{
    if (upload) memset(upload, 0, sizeof(*upload));
}

int mac_vertex_upload_begin(struct mac_vertex_upload *upload,
    const float *full_vec4_records, size_t source_bytes, unsigned long vertex_count,
    unsigned long linked_attribute_mask, struct mac_vertex_upload_layout *layout,
    char *error, size_t error_size)
{
    size_t bytes, needed, required_source_bytes;
    unsigned long vertex, attribute;
    unsigned packed_attribute, active_attributes, bit;
    GLint old_binding = 0;
    GLenum gl_error;
    if (error && error_size) error[0] = '\0';
    if (layout) memset(layout, 0, sizeof(*layout));
    if (!upload || !layout || upload->active || !full_vec4_records ||
        !vertex_count || vertex_count > MAC_VERTEX_UPLOAD_MAX_COUNT ||
        !linked_attribute_mask || (linked_attribute_mask & ~MAC_VERTEX_UPLOAD_MASK)) {
        upload_error(error, error_size, "invalid vertex upload arguments");
        return 0;
    }
    if (vertex_count > MAC_VERTEX_UPLOAD_MAX_BYTES /
        (MAC_VERTEX_UPLOAD_ATTRIBUTES * 4U * sizeof(float))) {
        upload_error(error, error_size, "full vertex record size overflows the bounded source range");
        return 0;
    }
    required_source_bytes = (size_t)vertex_count * MAC_VERTEX_UPLOAD_ATTRIBUTES *
        4U * sizeof(float);
    if (source_bytes < required_source_bytes) {
        upload_error(error, error_size, "full vertex record source range is too short");
        return 0;
    }
    if (glGetError() != GL_NO_ERROR) {
        upload_error(error, error_size, "OpenGL error was pending before vertex upload");
        return 0;
    }
    if (!upload->support_checked) {
        upload->supported = mac_vertex_upload_supported();
        upload->support_checked = 1;
    }
    if (!upload->supported) {
        upload_error(error, error_size, "OpenGL vertex buffer objects are unavailable");
        return 0;
    }
    active_attributes = active_count(linked_attribute_mask);
    if (!active_attributes || vertex_count > MAC_VERTEX_UPLOAD_MAX_BYTES /
        ((size_t)active_attributes * 4U * sizeof(float))) {
        upload_error(error, error_size, "compact vertex upload exceeds its bounded size");
        return 0;
    }
    bytes = (size_t)vertex_count * active_attributes * 4U * sizeof(float);
    if (bytes > MAC_VERTEX_UPLOAD_MAX_BYTES || bytes > 0x7fffffffU) {
        upload_error(error, error_size, "compact vertex byte count overflows GLsizeiptr");
        return 0;
    }
    if (bytes > upload->packed_capacity) {
        float *grown = (float *)realloc(upload->packed, bytes);
        if (!grown) {
            upload_error(error, error_size, "compact vertex scratch allocation failed");
            return 0;
        }
        upload->packed = grown;
        upload->packed_capacity = bytes;
    }
    for (vertex = 0; vertex < vertex_count; ++vertex) {
        packed_attribute = 0;
        for (attribute = 0; attribute < MAC_VERTEX_UPLOAD_ATTRIBUTES; ++attribute) {
            if (!(linked_attribute_mask & (1UL << attribute))) continue;
            memcpy(upload->packed + (size_t)vertex * active_attributes * 4U + packed_attribute * 4U,
                full_vec4_records + (size_t)vertex * MAC_VERTEX_UPLOAD_ATTRIBUTES * 4U + attribute * 4U,
                4U * sizeof(float));
            ++packed_attribute;
        }
    }
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &old_binding);
    if (!upload->buffer) glGenBuffers(1, &upload->buffer);
    if (!upload->buffer) {
        upload_error(error, error_size, "OpenGL vertex buffer allocation failed");
        return 0;
    }
    glBindBuffer(GL_ARRAY_BUFFER, upload->buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptrARB)bytes, upload->packed, GL_STREAM_DRAW);
    gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        glBindBuffer(GL_ARRAY_BUFFER, (GLuint)old_binding);
        upload_error(error, error_size, "OpenGL compact vertex upload failed");
        return 0;
    }
    upload->saved_array_buffer = old_binding;
    upload->active_mask = linked_attribute_mask;
    upload->vertex_count = vertex_count;
    upload->stride_bytes = active_attributes * 4U * sizeof(float);
    upload->active = 1;
    layout->buffer = upload->buffer;
    layout->active_mask = linked_attribute_mask;
    layout->vertex_count = vertex_count;
    layout->stride_bytes = upload->stride_bytes;
    needed = 0;
    for (bit = 0; bit < MAC_VERTEX_UPLOAD_ATTRIBUTES; ++bit) {
        if (linked_attribute_mask & (1UL << bit)) {
            layout->offsets[bit] = needed;
            needed += 4U * sizeof(float);
        }
    }
    return 1;
}

int mac_vertex_upload_end(struct mac_vertex_upload *upload)
{
    if (!upload || !upload->active) return 0;
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)upload->saved_array_buffer);
    upload->active = 0;
    upload->active_mask = 0;
    upload->vertex_count = 0;
    upload->stride_bytes = 0;
    return glGetError() == GL_NO_ERROR;
}

int mac_vertex_upload_dispose(struct mac_vertex_upload *upload)
{
    GLenum error;
    if (!upload || upload->active) return 0;
    if (upload->buffer) glDeleteBuffers(1, &upload->buffer);
    free(upload->packed);
    memset(upload, 0, sizeof(*upload));
    error = glGetError();
    return error == GL_NO_ERROR;
}
