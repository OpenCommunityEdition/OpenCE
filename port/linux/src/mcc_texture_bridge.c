/* MCC pixels already live in immutable, independently owned storage. Keep
 * their GPU copies here rather than staging them through the Xbox LRU heap.
 * A header's exact registered identity, never its Data word, grants access. */
#include "xgpu.h"
#include "mcc_texture_bridge.h"
#include <stdlib.h>
#include <string.h>

#define MCC_TEXTURE_BUCKETS 512
struct mcc_gpu_texture {
    struct mcc_gpu_texture *next;
    struct mcc_gpu_texture *hardware_next;
    void const *bitmap;
    DWORD header[5];
    void const *pixels;
    unsigned long bytes;
    GLuint texture;
    D3DCOLOR palette[256];
    BOOL palette_present;
    GLenum target;
    struct xgpu_texture_description description;
};
static struct mcc_gpu_texture *mcc_gpu_textures[MCC_TEXTURE_BUCKETS];
static struct mcc_gpu_texture *mcc_gpu_hardware[MCC_TEXTURE_BUCKETS];

static unsigned long mcc_texture_bucket(void const *bitmap)
{
    return (((unsigned long)bitmap >> 4) * 2654435761UL) % MCC_TEXTURE_BUCKETS;
}

void *mcc_texture_bridge_find(void const *bitmap)
{
    struct mcc_gpu_texture *entry;
    for (entry=mcc_gpu_textures[mcc_texture_bucket(bitmap)];entry;entry=entry->next)
        if (entry->bitmap==bitmap) return entry->header;
    return NULL;
}

void *mcc_texture_bridge_register(void const *bitmap,unsigned long format,
    unsigned long size,void const *pixels,unsigned long bytes)
{
    struct mcc_gpu_texture *entry;
    struct xgpu_texture_description description;
    unsigned long face_bytes,faces,bucket;
    void *existing=mcc_texture_bridge_find(bitmap);
    if (existing) return existing;
    if (!bitmap || !pixels || !bytes) return NULL;
    xgpu_texture_describe(format,size,&description);
    if (!description.width || !description.height || !description.depth ||
        description.width>8192 || description.height>8192 || description.depth>512 ||
        (description.depth>1 && (description.width>512 || description.height>512)) ||
        !description.levels || description.levels>14) return NULL;
    face_bytes=xgpu_texture_face_size(&description);
    faces=description.cube_map ? 6 : 1;
    if (!face_bytes || face_bytes>bytes/faces) return NULL;
    entry=calloc(1,sizeof(*entry));
    if (!entry) return NULL;
    entry->bitmap=bitmap;entry->pixels=pixels;entry->bytes=bytes;
    entry->description=description;
    entry->target=description.cube_map ? GL_TEXTURE_CUBE_MAP :
        description.depth>1 ? GL_TEXTURE_3D : GL_TEXTURE_2D;
    entry->header[0]=D3DCOMMON_TYPE_TEXTURE|1;
    /* Outside the guest physical arena and never dereferenced. The device
     * requires nonzero Data; the exact header registry resolves this token. */
    entry->header[1]=0xffffffffUL;
    entry->header[3]=format;entry->header[4]=size;
    bucket=mcc_texture_bucket(bitmap);
    entry->next=mcc_gpu_textures[bucket];mcc_gpu_textures[bucket]=entry;
    bucket=mcc_texture_bucket(entry->header);
    entry->hardware_next=mcc_gpu_hardware[bucket];mcc_gpu_hardware[bucket]=entry;
    return entry->header;
}

BOOL xgpu_mcc_texture_get(DWORD const *resource,D3DCOLOR const *palette,
    GLuint *texture,GLenum *target,struct xgpu_texture_description *description)
{
    struct mcc_gpu_texture *entry;
    /* Resource identity is checked without dereferencing arbitrary addresses;
     * hardware and bitmap lookups both use independent MCC hash tables. */
    for (entry=mcc_gpu_hardware[mcc_texture_bucket(resource)];entry;entry=entry->hardware_next) {
        if (entry->header!=resource) continue;
        *description=entry->description;*target=entry->target;
        if (entry->texture && entry->description.format==0x0b &&
            (entry->palette_present!=(palette!=NULL) ||
             (palette && memcmp(entry->palette,palette,sizeof(entry->palette))))) {
            glDeleteTextures(1,&entry->texture);entry->texture=0;xgpu_gl_state_invalidate();
        }
        if (!entry->texture) {
            GLuint created=0;
            glGenTextures(1,&created);
            if (created && xgpu_mcc_texture_upload(created,entry->target,
                &entry->description,entry->pixels,entry->bytes,palette)) {
                entry->texture=created;
                entry->palette_present=palette!=NULL;
                if (palette && entry->description.format==0x0b)
                    memcpy(entry->palette,palette,sizeof(entry->palette));
            }
            else if (created) {glDeleteTextures(1,&created);xgpu_gl_state_invalidate();}
        }
        *texture=entry->texture;
        return TRUE;
    }
    return FALSE;
}

void mcc_texture_bridge_dispose(void)
{
    unsigned long bucket;
    memset(mcc_gpu_hardware,0,sizeof(mcc_gpu_hardware));
    for (bucket=0;bucket<MCC_TEXTURE_BUCKETS;bucket++) {
        struct mcc_gpu_texture *entry=mcc_gpu_textures[bucket];
        mcc_gpu_textures[bucket]=NULL;
        while (entry) {
            struct mcc_gpu_texture *next=entry->next;
            xgpu_mcc_texture_unbind(entry->header);
            if (entry->texture) {glDeleteTextures(1,&entry->texture);xgpu_gl_state_invalidate();}
            free(entry);entry=next;
        }
    }
}
