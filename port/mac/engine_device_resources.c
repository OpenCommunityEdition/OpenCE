/* The native rasterizer's bounded resource-creation entrypoints. Storage,
   Xbox tiling/swizzling, locks, surface views, and release remain the shared
   d3d8_resources implementation; this layer validates the device contract. */
#include "engine_device_resources.h"
#include "engine_device_factory.h"
#undef malloc
#undef calloc
#undef free
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
/* port/mac/mac_gpu_textures.c: forget recorded sampler parameters of a deleted name */
void mac_gpu_textures_forget(unsigned int name);

/* The native prefix redirects the clean XDK wrappers to the checked entry
   points above. These calls are the shared storage implementation. */
#undef D3DDevice_CreateTexture
#undef D3DDevice_CreateCubeTexture
#undef D3DDevice_CreateVolumeTexture
extern HRESULT WINAPI D3DDevice_CreateTexture(UINT,UINT,UINT,DWORD,D3DFORMAT,
    D3DPOOL,D3DTexture **);
extern HRESULT WINAPI D3DDevice_CreateCubeTexture(UINT,UINT,DWORD,D3DFORMAT,
    D3DPOOL,D3DCubeTexture **);
extern HRESULT WINAPI D3DDevice_CreateVolumeTexture(UINT,UINT,UINT,UINT,DWORD,
    D3DFORMAT,D3DPOOL,D3DVolumeTexture **);
extern ULONG WINAPI D3DResource_Release(D3DResource *);
extern HRESULT WINAPI D3DTexture_GetSurfaceLevel(D3DTexture *,UINT,D3DSurface **);

#define MAC_NATIVE_RENDER_TARGET_LIMIT 16
/* The water's ripple texture (128x128, 4 levels) is drawn level by level;
   each level past the first has a framebuffer of its own on that level of
   the same GL texture, found by its surface's data address. */
#define MAC_NATIVE_RENDER_TARGET_LEVELS 4
struct native_render_target {
    D3DTexture *resource;
    GLuint framebuffer,texture,depth_buffer;
    unsigned width,height;
    unsigned level_count;
    GLuint level_framebuffer[MAC_NATIVE_RENDER_TARGET_LEVELS];
    GLuint level_depth_buffer[MAC_NATIVE_RENDER_TARGET_LEVELS];
    DWORD level_data[MAC_NATIVE_RENDER_TARGET_LEVELS];
    struct native_render_target *next;
};
static struct native_render_target *native_render_targets;
static unsigned native_render_target_count;

static int framebuffer_extension_available(void)
{
    const char *extensions=(const char *)glGetString(GL_EXTENSIONS);
    const char *position;
    static const char name[]="GL_EXT_framebuffer_object";
    if(!extensions)return 0;
    position=extensions;
    while((position=strstr(position,name))!=NULL) {
        if((position==extensions||position[-1]==' ')&&
           (position[sizeof(name)-1]==' '||position[sizeof(name)-1]=='\0'))
            return 1;
        position+=sizeof(name)-1;
    }
    return 0;
}

/* port: GL_EXT_packed_depth_stencil, asked once. The targets' depth buffers are depth 24 / stencil 8 in one
   renderbuffer; a driver without the extension (Mac OS X 10.4's, on some cards) refuses that format and the
   target is incomplete. Without it the buffer is depth only, and the stencil attachment is left out: a pass
   that needs the stencil draws without it, and the rest of the target works. */
static int packed_depth_stencil_available(void)
{
    static int known=-1;
    if(known<0) {
        const char *extensions=(const char *)glGetString(GL_EXTENSIONS);
        const char *position=extensions;
        static const char name[]="GL_EXT_packed_depth_stencil";
        known=0;
        while(position&&(position=strstr(position,name))!=NULL) {
            if((position==extensions||position[-1]==' ')&&
               (position[sizeof(name)-1]==' '||position[sizeof(name)-1]=='\0')){known=1;break;}
            position+=sizeof(name)-1;
        }
        /* (HALO_MAC_NO_PACKED_DEPTH_STENCIL=1 takes the fallback on any driver, to try it) */
        if(known&&getenv("HALO_MAC_NO_PACKED_DEPTH_STENCIL"))known=0;
        if(!known)fprintf(stderr,"native render targets: depth buffers without stencil (no GL_EXT_packed_depth_stencil, or HALO_MAC_NO_PACKED_DEPTH_STENCIL)\n");
    }
    return known;
}
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
static GLenum depth_buffer_format(void)
{ return packed_depth_stencil_available()?GL_DEPTH24_STENCIL8_EXT:GL_DEPTH_COMPONENT24; }
static void attach_stencil(GLuint renderbuffer)
{
    if(packed_depth_stencil_available())
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,renderbuffer);
}
static void delete_level_targets(struct native_render_target *entry)
{
    unsigned level;
    for(level=1;level<MAC_NATIVE_RENDER_TARGET_LEVELS;++level) {
        if(entry->level_framebuffer[level])glDeleteFramebuffersEXT(1,&entry->level_framebuffer[level]);
        if(entry->level_depth_buffer[level])glDeleteRenderbuffersEXT(1,&entry->level_depth_buffer[level]);
        entry->level_framebuffer[level]=entry->level_depth_buffer[level]=0;
    }
    entry->level_count=1;
}

/* The levels past the first of a mipmapped target, each with a framebuffer
   on its level and a depth buffer of its size; with the texture bound. On
   failure the target keeps level 0 only, and draws to the others are
   skipped as before. */
static void create_level_targets(struct native_render_target *entry,
    D3DTexture *resource,unsigned levels,D3DFORMAT format)
{
    unsigned level,width,height;
    GLenum status;
    GLint internal_format=(format==D3DFMT_R5G6B5)?GL_RGB5:GL_RGBA8;
    D3DSurface *surface;
    entry->level_count=1;
    if(levels>MAC_NATIVE_RENDER_TARGET_LEVELS)levels=MAC_NATIVE_RENDER_TARGET_LEVELS;
    for(level=1;level<levels;++level) {
        width=entry->width>>level;height=entry->height>>level;
        if(!width)width=1;
        if(!height)height=1;
        glTexImage2D(GL_TEXTURE_2D,(GLint)level,internal_format,(GLsizei)width,(GLsizei)height,
            0,(format==D3DFMT_R5G6B5)?GL_RGB:GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    }
    if(levels<=1)return;
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,(GLint)(levels-1));
    for(level=1;level<levels;++level) {
        surface=NULL;
        if(D3DTexture_GetSurfaceLevel(resource,level,&surface)!=S_OK||!surface)break;
        entry->level_data[level]=surface->Data;
        D3DResource_Release((D3DResource *)surface);
        width=entry->width>>level;height=entry->height>>level;
        if(!width)width=1;
        if(!height)height=1;
        glGenFramebuffersEXT(1,&entry->level_framebuffer[level]);
        glGenRenderbuffersEXT(1,&entry->level_depth_buffer[level]);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,entry->level_depth_buffer[level]);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,depth_buffer_format(),(GLsizei)width,(GLsizei)height);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,entry->level_framebuffer[level]);
        glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
            GL_TEXTURE_2D,entry->texture,(GLint)level);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
            GL_RENDERBUFFER_EXT,entry->level_depth_buffer[level]);
        attach_stencil(entry->level_depth_buffer[level]);
        status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        if(status!=GL_FRAMEBUFFER_COMPLETE_EXT||glGetError()!=GL_NO_ERROR) {
            fprintf(stderr,"native render target %ux%u: level %u framebuffer incomplete (0x%x); its draws are skipped\n",
                entry->width,entry->height,level,(unsigned)status);
            delete_level_targets(entry);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
            return;
        }
        entry->level_count=level+1;
    }
}

static int create_gl_render_target(D3DTexture *resource,unsigned width,
    unsigned height,unsigned levels,D3DFORMAT format)
{
    struct native_render_target *entry;
    GLint old_framebuffer=0,old_texture=0,old_renderbuffer=0,internal_format;
    GLint red_size=0,green_size=0,blue_size=0,alpha_size=0;
    GLenum error,status;
    if(native_render_target_count>=MAC_NATIVE_RENDER_TARGET_LIMIT||
       !framebuffer_extension_available())return 0;
    while(glGetError()!=GL_NO_ERROR){}
    entry=(struct native_render_target *)calloc(1,sizeof(*entry));
    if(!entry)return 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&old_framebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&old_texture);
    glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT,&old_renderbuffer);
    internal_format=(format==D3DFMT_R5G6B5)?GL_RGB5:GL_RGBA8;
    glGenTextures(1,&entry->texture);
    glBindTexture(GL_TEXTURE_2D,entry->texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,internal_format,(GLsizei)width,(GLsizei)height,
        0,(format==D3DFMT_R5G6B5)?GL_RGB:GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    entry->width=width;entry->height=height;
    create_level_targets(entry,resource,levels,format);
    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_RED_SIZE,&red_size);
    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_GREEN_SIZE,&green_size);
    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_BLUE_SIZE,&blue_size);
    if(format!=D3DFMT_R5G6B5) {
        glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_ALPHA_SIZE,&alpha_size);
    }
    glGenFramebuffersEXT(1,&entry->framebuffer);
    glGenRenderbuffersEXT(1,&entry->depth_buffer);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,entry->depth_buffer);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,depth_buffer_format(),
        (GLsizei)width,(GLsizei)height);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,entry->framebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
        GL_TEXTURE_2D,entry->texture,0);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
        GL_RENDERBUFFER_EXT,entry->depth_buffer);
    attach_stencil(entry->depth_buffer);
    status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    error=glGetError();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,(GLuint)old_framebuffer);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,(GLuint)old_renderbuffer);
    glBindTexture(GL_TEXTURE_2D,(GLuint)old_texture);
    if(error!=GL_NO_ERROR||status!=GL_FRAMEBUFFER_COMPLETE_EXT||
       (format==D3DFMT_R5G6B5&&(red_size!=5||green_size!=6||blue_size!=5))||
       (format!=D3DFMT_R5G6B5&&
        (red_size!=8||green_size!=8||blue_size!=8||alpha_size!=8))) {
        if(entry->framebuffer)glDeleteFramebuffersEXT(1,&entry->framebuffer);
        if(entry->depth_buffer)glDeleteRenderbuffersEXT(1,&entry->depth_buffer);
        delete_level_targets(entry);
        if(entry->texture){mac_gpu_textures_forget(entry->texture);glDeleteTextures(1,&entry->texture);}
        free(entry);return 0;
    }
    entry->resource=resource;
    if(getenv("HALO_MAC_DIAG_TARGETS"))
        fprintf(stderr,"native render target: %ux%u format=0x%x levels=%u (with depth/stencil)\n",width,height,(unsigned)format,entry->level_count);
    entry->next=native_render_targets;native_render_targets=entry;
    ++native_render_target_count;
    return 1;
}

static int native_texture_format(D3DFORMAT format)
{
    switch((int)format) {
    case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:
    case 0x06: case 0x07: case 0x0b: case 0x0c: case 0x0e: case 0x0f:
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x16: case 0x17:
    case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e:
    case 0x1f: case 0x20: case 0x24: case 0x25: case 0x27: case 0x28:
    case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e:
    case 0x2f: case 0x30: case 0x31: case 0x32: case 0x33: case 0x35:
    case 0x36: case 0x37: case 0x38: case 0x39: case 0x3a: case 0x3b:
    case 0x3c: case 0x3d: case 0x3e: case 0x3f: case 0x40: case 0x41:
        return 1;
    default:
        return 0;
    }
}

static unsigned maximum_dimension(unsigned a,unsigned b,unsigned c)
{
    unsigned result=a;
    if(b>result)result=b;
    if(c>result)result=c;
    return result;
}

static unsigned maximum_mip_levels(unsigned dimension)
{
    unsigned levels=1;
    while(dimension>1){dimension>>=1;++levels;}
    return levels;
}

static int valid_texture_request(D3DDevice *device,unsigned width,unsigned height,
    unsigned depth,unsigned levels,DWORD usage,D3DFORMAT format,D3DPOOL pool,
    int cube,int volume)
{
    D3DCAPS8 caps;
    unsigned largest,limit;
    if(!mac_engine_device_factory_get_caps(device,&caps)||!width||!height||!depth)
        return 0;
    largest=maximum_dimension(width,height,depth);
    if(!native_texture_format(format)||largest>4096||
       (levels&&levels>maximum_mip_levels(largest)))return 0;
    if(volume&&(width>512||height>512||depth>512||cube))return 0;
    if(!volume&&depth!=1)return 0;
    limit=volume?512:(unsigned)caps.MaxTextureWidth;
    if(!limit||width>limit||height>limit||depth>limit)return 0;
    if(usage==D3DUSAGE_RENDERTARGET) {
        if(pool!=D3DPOOL_DEFAULT||cube||volume)return 0;
        /* These are precisely the rasterizer_initialize render-target
           formats. The shared backend allocates native Xbox-format storage;
           drawing to these resources remains a separate GPU capability. */
        if(format!=D3DFMT_LIN_A8R8G8B8&&format!=D3DFMT_A8R8G8B8&&
           format!=D3DFMT_R5G6B5)return 0;
    } else if(usage==0) {
        if(pool!=D3DPOOL_MANAGED)return 0;
    } else return 0;
    return 1;
}

HRESULT WINAPI mac_engine_device_create_texture(UINT width,UINT height,UINT levels,
    DWORD usage,D3DFORMAT format,D3DPOOL pool,D3DTexture **texture)
{
    if(texture)*texture=NULL;
    HRESULT result;
    if(!texture||!valid_texture_request(mac_device_gpu_identity(),width,height,1,
       levels,usage,format,pool,0,0))return E_INVALIDARG;
    result=D3DDevice_CreateTexture(width,height,levels,usage,format,pool,texture);
    if(result!=S_OK||usage!=D3DUSAGE_RENDERTARGET)return result;
    if(!create_gl_render_target(*texture,width,height,levels,format)) {
        D3DResource_Release((D3DResource *)*texture);*texture=NULL;return E_FAIL;
    }
    return S_OK;
}

HRESULT WINAPI mac_engine_device_create_cube_texture(UINT edge_length,UINT levels,
    DWORD usage,D3DFORMAT format,D3DPOOL pool,D3DCubeTexture **texture)
{
    if(texture)*texture=NULL;
    if(!texture||!valid_texture_request(mac_device_gpu_identity(),edge_length,
       edge_length,1,levels,usage,format,pool,1,0))return E_INVALIDARG;
    return D3DDevice_CreateCubeTexture(edge_length,levels,usage,format,pool,texture);
}

HRESULT WINAPI mac_engine_device_create_volume_texture(UINT width,UINT height,
    UINT depth,UINT levels,DWORD usage,D3DFORMAT format,D3DPOOL pool,
    D3DVolumeTexture **texture)
{
    if(texture)*texture=NULL;
    if(!texture||!valid_texture_request(mac_device_gpu_identity(),width,height,
       depth,levels,usage,format,pool,0,1))return E_INVALIDARG;
    /* The shared Xbox allocator computes its level ceiling from X/Y; the
       native runtime's source-defined default volume has one 4x4x4 level. */
    if(levels!=1)return E_INVALIDARG;
    return D3DDevice_CreateVolumeTexture(width,height,depth,levels,usage,format,
        pool,texture);
}

#define NO_LEVEL 0xffffffffU
/* which level of the target the surface is, or NO_LEVEL */
static unsigned target_level(const struct native_render_target *entry,const D3DSurface *surface)
{
    unsigned level;
    if(surface->Data==entry->resource->Data)return 0;
    for(level=1;level<entry->level_count;++level)
        if(surface->Data==entry->level_data[level])return level;
    return NO_LEVEL;
}
/* port: sweep. The rasterizer's eighth target (render_primary_copy, which
   the cinematic screen effects blur and warp the screen through) is, on the
   Xbox, the depth buffer's memory used as a second color buffer: a surface
   and a texture made by the game with that memory's address. Here that
   address is the depth buffer's own, so the pair is recognised by it and
   given a render target of its own, the screen's size (made again when the
   screen's size changes). */
static D3DTexture screen_copy_resource;
static struct native_render_target *screen_copy_entry;
static int screen_copy_size(DWORD size,unsigned *width,unsigned *height)
{
    if(!size)return 0;
    *width=(size&D3DSIZE_WIDTH_MASK)+1;
    *height=((size&D3DSIZE_HEIGHT_MASK)>>D3DSIZE_HEIGHT_SHIFT)+1;
    return *width<=4096&&*height<=4096;
}
static struct native_render_target *screen_copy_ensure(DWORD data,DWORD size)
{
    const struct mac_device_state *state=mac_device_state_get();
    struct native_render_target **link,*entry;
    unsigned width,height;
    if(!state||!data||data!=state->depth_buffer.Data||!screen_copy_size(size,&width,&height))return NULL;
    if(screen_copy_entry&&screen_copy_entry->width==width&&screen_copy_entry->height==height)
        return screen_copy_entry;
    if(screen_copy_entry) {
        for(link=&native_render_targets;*link;link=&(*link)->next)
            if(*link==screen_copy_entry) {
                entry=*link;*link=entry->next;--native_render_target_count;
                if(entry->framebuffer)glDeleteFramebuffersEXT(1,&entry->framebuffer);
                if(entry->depth_buffer)glDeleteRenderbuffersEXT(1,&entry->depth_buffer);
                delete_level_targets(entry);
                if(entry->texture){mac_gpu_textures_forget(entry->texture);glDeleteTextures(1,&entry->texture);}
                free(entry);break;
            }
        screen_copy_entry=NULL;
    }
    screen_copy_resource.Data=data;
    if(!create_gl_render_target(&screen_copy_resource,width,height,1,D3DFMT_LIN_A8R8G8B8))return NULL;
    screen_copy_entry=native_render_targets;
    return screen_copy_entry;
}
static int screen_copy_color(unsigned long format)
{
    return ((format&D3DFORMAT_FORMAT_MASK)>>D3DFORMAT_FORMAT_SHIFT)==D3DFMT_LIN_A8R8G8B8;
}
int mac_engine_device_resources_target_info(D3DSurface *surface,
    unsigned *framebuffer,unsigned *texture,unsigned *width,unsigned *height)
{
    struct native_render_target *entry;
    unsigned level;
    if(surface&&surface->Data&&screen_copy_color(surface->Format)&&
       mac_device_state_get()&&surface!=&mac_device_state_get()->depth_buffer&&
       surface->Data==mac_device_state_get()->depth_buffer.Data&&
       screen_copy_ensure(surface->Data,surface->Size))
        surface->Parent=(struct D3DBaseTexture *)&screen_copy_resource;
    if(!surface||!surface->Parent)return 0;
    for(entry=native_render_targets;entry;entry=entry->next) {
        if(entry->resource!=(D3DTexture *)surface->Parent)continue;
        level=target_level(entry,surface);
        if(level==NO_LEVEL)continue;
        if(framebuffer)*framebuffer=level?entry->level_framebuffer[level]:entry->framebuffer;
        if(texture)*texture=entry->texture;
        if(width)*width=entry->width>>level?entry->width>>level:1;
        if(height)*height=entry->height>>level?entry->height>>level:1;
        return 1;
    }
    return 0;
}

/* The GL texture of a render-target texture (what target_info reports for
   its level-0 surface), without making that surface. */
int mac_engine_device_resources_texture_target(const D3DTexture *resource,unsigned *texture)
{
    struct native_render_target *entry;
    if(!resource)return 0;
    if(resource->Data&&screen_copy_color(resource->Format)&&mac_device_state_get()&&
       resource->Data==mac_device_state_get()->depth_buffer.Data&&
       (const D3DTexture *)resource!=&screen_copy_resource)
        if(screen_copy_ensure(resource->Data,resource->Size))resource=&screen_copy_resource;
    for(entry=native_render_targets;entry;entry=entry->next)
        if(entry->resource==resource) {
            if(texture)*texture=entry->texture;
            return 1;
        }
    return 0;
}
int mac_engine_device_resources_target_depth(D3DSurface *surface,unsigned *depth_buffer)
{
    struct native_render_target *entry;
    if(!surface||!surface->Parent)return 0;
    for(entry=native_render_targets;entry;entry=entry->next) {
        unsigned level;
        if(entry->resource!=(D3DTexture *)surface->Parent)continue;
        level=target_level(entry,surface);
        if(level==NO_LEVEL)continue;
        if(depth_buffer)*depth_buffer=level?entry->level_depth_buffer[level]:entry->depth_buffer;
        return (level?entry->level_depth_buffer[level]:entry->depth_buffer)!=0;
    }
    return 0;
}

/* HALO_MAC_DIAG_TARGETS: what a level past the first of a target holds
   when it is left, a few times (are the ripples drawn into it?) */
static void diag_level_readback(GLuint framebuffer)
{
    static unsigned reported;
    struct native_render_target *entry;
    unsigned level,width,height,x,sum;
    unsigned char pixels[16*4];
    if(reported>=12||!framebuffer)return;
    for(entry=native_render_targets;entry;entry=entry->next)
        for(level=1;level<entry->level_count;++level)
            if(entry->level_framebuffer[level]==framebuffer) {
                width=entry->width>>level;height=entry->height>>level;
                glReadPixels(0,(GLint)(height/2),16<(int)width?16:(GLsizei)width,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
                for(sum=0,x=0;x<sizeof(pixels);++x)sum+=pixels[x];
                fprintf(stderr,"native render target %ux%u level %u (%ux%u): row sum %u, first %02x%02x%02x%02x\n",
                    entry->width,entry->height,level,width,height,sum,pixels[0],pixels[1],pixels[2],pixels[3]);
                ++reported;return;
            }
}

int mac_engine_device_resources_bind_target(D3DSurface *surface,D3DSurface *depth)
{
    unsigned framebuffer;
    unsigned width,height,depth_width,depth_height;
    unsigned long depth_format;
    const struct mac_device_state *state;
    GLuint depth_attachment;
    GLenum status;
    GLint old_framebuffer;
    unsigned target_depth_buffer;
    if(!mac_device_gpu_identity()||
       !mac_engine_device_resources_target_info(surface,&framebuffer,NULL,&width,&height)) {
        {static unsigned reported;if(reported<4){++reported;fprintf(stderr,"native render target %p: not a native target (format %08lx size %08lx)\n",(void *)surface,
            surface?(unsigned long)surface->Format:0UL,surface?(unsigned long)surface->Size:0UL);}}
        return 0;
    }
    if(!mac_engine_device_resources_target_depth(surface,&target_depth_buffer)) {
        fprintf(stderr,"native render target %ux%u: no depth buffer\n",width,height);return 0;
    }
    state=mac_device_state_get();
    if(depth) {
        depth_format=(depth->Format&D3DFORMAT_FORMAT_MASK)>>D3DFORMAT_FORMAT_SHIFT;
        if(depth_format!=D3DFMT_D24S8&&depth_format!=D3DFMT_LIN_D24S8) {
            fprintf(stderr,"native render target %ux%u: depth format %lu\n",width,height,depth_format);
            return 0;
        }
        if(depth->Size) {
            depth_width=(depth->Size&D3DSIZE_WIDTH_MASK)+1;
            depth_height=((depth->Size&D3DSIZE_HEIGHT_MASK)>>D3DSIZE_HEIGHT_SHIFT)+1;
        } else {
            depth_width=1UL<<((depth->Format&D3DFORMAT_USIZE_MASK)>>D3DFORMAT_USIZE_SHIFT);
            depth_height=1UL<<((depth->Format&D3DFORMAT_VSIZE_MASK)>>D3DFORMAT_VSIZE_SHIFT);
        }
        if(!state || depth!=(D3DSurface *)&state->depth_buffer) {
            fprintf(stderr,"native render target %ux%u: depth surface %p is not the screen's\n",width,height,(void *)depth);
            return 0;
        }
        if(depth_width!=width||depth_height!=height) {
            /* A smaller target drawn with the screen's depth surface (the
               water's ripple pass): the target's own depth buffer, its size,
               serves, as the pass does not test against the scene's. */
            static int reported;
            if(depth_width<width||depth_height<height)return 0;
            if(!reported) {
                fprintf(stderr,"native render target %ux%u drawn with its own depth buffer, not the %ux%u screen's\n",
                    width,height,depth_width,depth_height);
                reported=1;
            }
        }
    }
    depth_attachment=target_depth_buffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&old_framebuffer);
    if(getenv("HALO_MAC_DIAG_TARGETS"))diag_level_readback((GLuint)old_framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,(GLuint)framebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
        GL_RENDERBUFFER_EXT,depth_attachment);
    attach_stencil(depth_attachment);
    status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if(status!=GL_FRAMEBUFFER_COMPLETE_EXT||glGetError()!=GL_NO_ERROR) {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,(GLuint)old_framebuffer);
        return 0;
    }
    return 1;
}

void mac_engine_device_resources_stop(void)
{
    struct native_render_target *entry=native_render_targets,*next;
    while(entry) {
        next=entry->next;
        if(entry->framebuffer)glDeleteFramebuffersEXT(1,&entry->framebuffer);
        if(entry->depth_buffer)glDeleteRenderbuffersEXT(1,&entry->depth_buffer);
        delete_level_targets(entry);
        if(entry->texture){mac_gpu_textures_forget(entry->texture);glDeleteTextures(1,&entry->texture);}
        free(entry);entry=next;
    }
    native_render_targets=NULL;native_render_target_count=0;screen_copy_entry=NULL;
}
