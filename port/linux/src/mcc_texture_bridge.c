/* MCC pixels already live in immutable, independently owned storage. Keep
 * their GPU copies here rather than staging them through the Xbox LRU heap.
 * A header's exact registered identity, never its Data word, grants access. */
#include "xgpu.h"
#include "mcc_texture_bridge.h"
#include "hud_hires.h"
#include "zlib_prefixed.h"
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
    long hud_asset;
};
static struct mcc_gpu_texture *mcc_gpu_textures[MCC_TEXTURE_BUCKETS];
static struct mcc_gpu_texture *mcc_gpu_hardware[MCC_TEXTURE_BUCKETS];

int config_boolean(char const *name);

/* These are the complete, converted stock MCC sheets, including the _r
 * variants. Names alone cannot distinguish custom art. Match dimensions,
 * format and immutable pixel fingerprints before using an MCC-only atlas.
 * Registration happens once; the setting is checked at draw time so turning
 * High-Res HUD off immediately restores the original GPU texture. */
static long mcc_hud_texture_asset(struct xgpu_texture_description const *description,
    void const *pixels,unsigned long bytes)
{
    static struct {unsigned long format,bytes,crc;char const *tag;} const known[]={
        {D3DFMT_A8L8,131072,1892071729UL,"mcc\\hud\\rocket_reticle"},
        {D3DFMT_A8R8G8B8,262144,3704827890UL,"mcc\\hud\\rocket_reticle"},
        {D3DFMT_A8L8,131072,2149244368UL,"mcc\\hud\\sniper_scope"},
        {D3DFMT_A8R8G8B8,262144,1422780498UL,"mcc\\hud\\sniper_scope"}
    };
    unsigned long i,crc=0;
    int checked=0;
    long asset;
    if (!pixels || description->width!=256 || description->height!=256 ||
        description->depth!=1 || description->levels!=1 || description->cube_map || description->linear) return -1;
    for (i=0;i<sizeof(known)/sizeof(known[0]);i++) {
        if (description->format!=known[i].format || bytes!=known[i].bytes) continue;
        if (!checked) {crc=crc32(0L,pixels,(uInt)bytes);checked=1;}
        if (crc!=known[i].crc) continue;
        for (asset=0;asset<(long)hud_hires_embedded_count;asset++)
            if (!strcmp(hud_hires_embedded[asset].tag,known[i].tag)) return asset;
    }
    return -1;
}

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

/* Converted Gearbox meter clones carry shape coverage in green. Reuse the
 * renderer's coverage interface without changing any native texture state. */
void mcc_texture_bridge_meter(void const *bitmap)
{
    struct mcc_gpu_texture *entry;
    for (entry=mcc_gpu_textures[mcc_texture_bucket(bitmap)];entry;entry=entry->next)
        if (entry->bitmap==bitmap) {
            /* Filter the authored coverage like the adjacent HUD outline;
             * keep fill thresholds discrete so ammunition segments cannot
             * bleed into their neighbours. The Xbox meter sampler is point. */
            entry->description.hires=TRUE;
            entry->description.hires_coverage=TRUE;
            entry->description.hires_point_threshold=TRUE;
            return;
        }
}

static void *mcc_texture_register(void const *bitmap,unsigned long format,
    unsigned long size,void const *pixels,unsigned long bytes,
    unsigned long width,unsigned long height,unsigned long pitch)
{
    struct mcc_gpu_texture *entry;
    struct xgpu_texture_description description;
    unsigned long face_bytes,faces,bucket;
    void *existing=mcc_texture_bridge_find(bitmap);
    if (existing) return existing;
    if (!bitmap || !pixels || !bytes) return NULL;
    xgpu_texture_describe(format,size,&description);
    if (width || height || pitch) {
        if (description.format!=D3DFMT_LIN_A8R8G8B8 || description.cube_map ||
            description.depth!=1 || description.levels!=1 || !width || !height ||
            width>16384 || height>16384 || pitch<width*4 || (pitch&63) || pitch>65536) return NULL;
        description.width=width;description.height=height;description.pitch=pitch;
    }
    if (!description.width || !description.height || !description.depth ||
        description.width>(description.linear ? 16384u : 8192u) ||
        description.height>(description.linear ? 16384u : 8192u) || description.depth>512 ||
        (description.depth>1 && (description.width>512 || description.height>512)) ||
        !description.levels || description.levels>14) return NULL;
    face_bytes=xgpu_texture_face_size(&description);
    faces=description.cube_map ? 6 : 1;
    if (!face_bytes || face_bytes>bytes/faces) return NULL;
    entry=calloc(1,sizeof(*entry));
    if (!entry) return NULL;
    entry->bitmap=bitmap;entry->pixels=pixels;entry->bytes=bytes;
    entry->description=description;
    entry->hud_asset=mcc_hud_texture_asset(&description,pixels,bytes);
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

void *mcc_texture_bridge_register(void const *bitmap,unsigned long format,
    unsigned long size,void const *pixels,unsigned long bytes)
{
    return mcc_texture_register(bitmap,format,size,pixels,bytes,0,0,0);
}

/* Linear Xbox headers have only 12-bit dimensions and an 8-bit pitch.
 * Keep wider MCC dimensions in the identity-checked descriptor instead. */
void *mcc_texture_bridge_register_linear(void const *bitmap,unsigned long format,
    unsigned long width,unsigned long height,unsigned long pitch,void const *pixels,unsigned long bytes)
{
    return mcc_texture_register(bitmap,format,0,pixels,bytes,width,height,pitch);
}

/* Metadata-only lookup for loading-time shader selection. No texture upload. */
BOOL xgpu_mcc_texture_describe(DWORD const *resource,struct xgpu_texture_description *description)
{
    struct mcc_gpu_texture *entry;
    for (entry=mcc_gpu_hardware[mcc_texture_bucket(resource)];entry;entry=entry->hardware_next)
        if (entry->header==resource) {*description=entry->description;return TRUE;}
    return FALSE;
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
        if (entry->hud_asset>=0 && config_boolean("display.high_res_hud")) {
            GLuint replacement=hud_hires_override_texture(entry->hud_asset,&description->levels);
            if (replacement) {
                description->hires=TRUE;*texture=replacement;
                return TRUE;
            }
            /* Allocation/upload failure retains the original descriptor. */
            *description=entry->description;
        }
        if (entry->texture && entry->description.format==0x0b &&
            (entry->palette_present!=(palette!=NULL) ||
             (palette && memcmp(entry->palette,palette,sizeof(entry->palette))))) {
            glDeleteTextures(1,&entry->texture);entry->texture=0;xgpu_gl_state_invalidate();
        }
        if (!entry->texture) {
            GLuint created=0;
            created=xgpu_texture_name_new();
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

unsigned long mcc_texture_bridge_prepare(void const *bitmap, unsigned long budget)
{
#ifndef HALO_GLES
    struct mcc_gpu_texture *entry;
    for (entry=mcc_gpu_textures[mcc_texture_bucket(bitmap)];entry;entry=entry->next) {
        struct xgpu_texture_description const *description=&entry->description;
        unsigned long long estimate=0;
        unsigned long width,height,depth,level;
        GLuint created=0;
        if (entry->bitmap!=bitmap) continue;
        /* Paletted textures need their draw's palette. HUD replacements are
         * independently owned and must not be charged to this budget. */
        if (entry->texture || description->format==0x0b ||
            (entry->hud_asset>=0 && config_boolean("display.high_res_hud"))) return 0;
        width=description->width;height=description->height;depth=description->depth;
        if (!width || !height || !depth || !description->levels || description->levels>14) return 0;
        if (description->compressed) estimate=(unsigned long long)xgpu_texture_face_size(description)*(description->cube_map?6:1);
        else {
            for (level=0;level<description->levels;level++) {
                estimate+=(unsigned long long)width*height*depth*4*(description->cube_map?6:1);
                if (estimate>budget) return 0;
                if (width>1) width>>=1;
                if (height>1) height>>=1;
                if (depth>1) depth>>=1;
            }
        }
        if (estimate<256*1024 || estimate>budget) return 0;
        created=xgpu_texture_name_new();
        if (created && xgpu_mcc_texture_upload(created,entry->target,description,entry->pixels,entry->bytes,NULL)) {
            entry->texture=created;entry->palette_present=FALSE;
            return (unsigned long)estimate;
        }
        if (created) glDeleteTextures(1,&created);
        xgpu_gl_state_invalidate();
        return 0;
    }
#else
    (void)bitmap;(void)budget;
#endif
    return 0;
}
