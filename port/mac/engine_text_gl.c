/* Real shared font atlas upload using APIs available on Tiger/Leopard.
   This optional variant owns its GL texture in the caller's CGL context. */
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#define mac_text_dispose mac_text_cpu_dispose
#include "engine_text.c"
#undef mac_text_dispose
#include "engine_text_gl.h"
static GLuint atlas_texture;
static CGLContextObj atlas_context;
static void (*invalidate_cache)(void);
static void mac_text_gl_invalidate(void){if(invalidate_cache)invalidate_cache();}
#include "mac_text_gl_body.h"
/* port/mac/mac_gpu_textures.c forgets the sampler parameters it recorded for
   a deleted texture name; this weak no-op serves builds without it. */
void mac_gpu_textures_forget(unsigned int name) __attribute__((weak));
void mac_gpu_textures_forget(unsigned int name) { (void)name; }
int mac_text_gl_start(void (*invalidate)(void))
{
    CGLContextObj current=CGLGetCurrentContext();
    if(!current || atlas_context)return 0;
    atlas_context=current;invalidate_cache=invalidate;return 1;
}
unsigned int text_hires_atlas_texture(unsigned long data)
{
    if(!atlas_context || CGLGetCurrentContext()!=atlas_context)return 0;
    return mac_text_gl_upload_shared(data);
}
int mac_text_gl_resolve(unsigned long data,unsigned int *name,float scale[4])
{
    GLint binding,unpack;
    GLenum error;
    if(!data || data!=placeholder_data || !atlas)return 0;
    if(!name || !scale || !placeholder_width || !placeholder_height ||
       !atlas_context || CGLGetCurrentContext()!=atlas_context)return -1;
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpack);
    *name=text_hires_atlas_texture(data);
    error=glGetError();
    glBindTexture(GL_TEXTURE_2D,(GLuint)binding);
    glPixelStorei(GL_UNPACK_ALIGNMENT,unpack);
    if(!*name || dirty_top<dirty_bottom || error!=GL_NO_ERROR)return -1;
    scale[0]=1.0f/(float)placeholder_width;
    scale[1]=1.0f/(float)placeholder_height;
    scale[2]=scale[3]=1.0f;
    return 1;
}
void mac_text_dispose(void)
{
    if(atlas_texture){
        if(CGLGetCurrentContext()!=atlas_context){fprintf(stderr,"native font atlas disposed without its GL context\n");abort();}
        mac_gpu_textures_forget(atlas_texture);glDeleteTextures(1,&atlas_texture);atlas_texture=0;mac_text_gl_invalidate();
    }
    mac_text_cpu_dispose();atlas_context=NULL;invalidate_cache=NULL;
}
