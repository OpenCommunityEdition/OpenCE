/* Texture names do not allocate image storage. Batch their creation: separate
 * glGenTextures calls can synchronize the Windows driver on first-use draws.
 * Once a name is handed out, its texture cache owns it and its deletion. */
#include "xgpu.h"
#include <string.h>

#ifndef HALO_GLES
#define TEXTURE_NAMES_BATCH 64
#define TEXTURE_NAMES_PREPARED 1024
static GLuint texture_names[TEXTURE_NAMES_PREPARED];
static unsigned texture_name_count;

static void texture_names_fill(unsigned target)
{
    unsigned first = texture_name_count, index;
    if (first >= target) return;
    memset(texture_names + first, 0, (target - first) * sizeof(*texture_names));
    glGenTextures((GLsizei)(target - first), texture_names + first);
    for (index = first; index < target; index++) {
        if (texture_names[index]) texture_names[texture_name_count++] = texture_names[index];
    }
}
#endif

GLuint xgpu_texture_name_new(void)
{
#ifndef HALO_GLES
    if (!texture_name_count) texture_names_fill(TEXTURE_NAMES_BATCH);
    return texture_name_count ? texture_names[--texture_name_count] : 0;
#else
    GLuint texture = 0;
    glGenTextures(1, &texture);
    return texture;
#endif
}

void xgpu_texture_names_prepare(void)
{
#ifndef HALO_GLES
    texture_names_fill(TEXTURE_NAMES_PREPARED);
#endif
}

void xgpu_texture_names_dispose(void)
{
#ifndef HALO_GLES
    if (texture_name_count) glDeleteTextures((GLsizei)texture_name_count, texture_names);
    texture_name_count = 0;
#endif
}
