/* Offscreen top-down BSP diagnostic; not the engine renderer or gameplay. */
#include "../port/mac/mac_bsp_renderer.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIDE 512

int main(int argc,char **argv)
{
    FILE *file=NULL;long length;unsigned char *data=NULL,*pixels=NULL;
    struct mac_retail_cache cache;struct mac_bsp_renderer *renderer=NULL;
    struct mac_bsp_renderer_stats stats;const char *error=NULL;
    CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAColorSize,(CGLPixelFormatAttribute)24,(CGLPixelFormatAttribute)0};
    CGLPixelFormatObj format=NULL;CGLContextObj context=NULL;GLint formats;
    GLuint fbo=0,color=0,depth=0;unsigned int i,j,nonblack=0;int result=1;
    if(argc!=3&&argc!=4){fprintf(stderr,"usage: %s map output.ppm [bitmaps.map]\n",argv[0]);return 2;}
    file=fopen(argv[1],"rb");
    if(!file||fseek(file,0,SEEK_END))goto done;
    length=ftell(file);if(length<0x800||length>0x30000000L||fseek(file,0,SEEK_SET))goto done;
    data=(unsigned char *)malloc((size_t)length);
    if(!data||fread(data,1,(size_t)length,file)!=(size_t)length)goto done;
    fclose(file);file=NULL;
    if(!mac_retail_open(data,(size_t)length,&cache,&error)){fprintf(stderr,"map: %s\n",error);goto done;}
    if(CGLChoosePixelFormat(attrs,&format,&formats)!=kCGLNoError||!format)goto done;
    if(CGLCreateContext(format,NULL,&context)!=kCGLNoError||!context)goto done;
    if(CGLSetCurrentContext(context)!=kCGLNoError)goto done;
    printf("renderer=%s\n",glGetString(GL_RENDERER));
    glGenFramebuffersEXT(1,&fbo);glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);
    glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,SIDE,SIDE,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,color,0);
    glGenRenderbuffersEXT(1,&depth);glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depth);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT24,SIDE,SIDE);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depth);
    if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)!=GL_FRAMEBUFFER_COMPLETE_EXT)goto done;
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    renderer=mac_bsp_renderer_open(&cache,0,argc==4?argv[3]:NULL,&error);
    if(!renderer){fprintf(stderr,"renderer: %s\n",error?error:"unknown error");goto done;}
    if(!mac_bsp_renderer_draw_topdown(renderer,SIDE,SIDE))goto done;
    pixels=(unsigned char *)malloc(SIDE*SIDE*3);if(!pixels)goto done;
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,SIDE,SIDE,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    if(glGetError()!=GL_NO_ERROR)goto done;
    for(i=0;i<SIDE*SIDE*3;++i)if(pixels[i])++nonblack;
    if(!nonblack)goto done;
    file=fopen(argv[2],"wb");if(!file)goto done;
    if(fprintf(file,"P6\n%d %d\n255\n",SIDE,SIDE)<0)goto done;
    for(j=SIDE;j>0;--j)if(fwrite(pixels+(j-1)*SIDE*3,1,SIDE*3,file)!=SIDE*3)goto done;
    if(fclose(file)){file=NULL;goto done;}file=NULL;
    mac_bsp_renderer_get_stats(renderer,&stats);
    printf("offscreen_BSP=pass vertices=%lu nonzero_channels=%u gl_errors=0\n",stats.vertices,nonblack);
    if(argc==4)printf("textures=%u RGBA_digest=%08lx unsupported_materials=%u (magenta)\n",
        stats.texture_count,(unsigned long)stats.texture_digest,stats.unsupported_materials);
    result=0;
done:
    if(file)fclose(file);
    if(context) {
        CGLSetCurrentContext(context);
        if(renderer)mac_bsp_renderer_close(renderer);
        if(fbo)glDeleteFramebuffersEXT(1,&fbo);
        if(color)glDeleteTextures(1,&color);
        if(depth)glDeleteRenderbuffersEXT(1,&depth);
        CGLSetCurrentContext(NULL);CGLDestroyContext(context);
    } else if(renderer)mac_bsp_renderer_close(renderer);
    if(format)CGLDestroyPixelFormat(format);
    free(pixels);free(data);return result;
}
