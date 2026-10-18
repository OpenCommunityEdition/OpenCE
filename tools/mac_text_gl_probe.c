#include "engine_text_gl.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned checks,failures,invalidations;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s line%d\n",#x,__LINE__);}}while(0)
static void invalidate(void){++invalidations;}
static void verify_atlas(GLuint texture,uint32_t *digest)
{
    size_t bytes,i;unsigned w,h;const unsigned char *coverage=mac_text_atlas(&bytes,&w,&h);
    unsigned char *rgba=malloc(bytes*4);unsigned mismatch=0;uint32_t sum=2166136261U;
    CHECK(coverage && rgba);if(!coverage||!rgba){free(rgba);return;}
    glBindTexture(GL_TEXTURE_2D,texture);glPixelStorei(GL_PACK_ALIGNMENT,1);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);CHECK(glGetError()==GL_NO_ERROR);
    for(i=0;i<bytes;++i){if(rgba[i*4]!=255||rgba[i*4+1]!=255||rgba[i*4+2]!=255||rgba[i*4+3]!=coverage[i])++mismatch;sum=(sum^rgba[i*4+3])*16777619U;}
    CHECK(mismatch==0);*digest=sum;free(rgba);
}
static void draw_glyph(GLuint texture)
{
    GLuint fbo=0,target=0;unsigned char output[64*64*4];size_t bytes;unsigned w,h,x,y,mismatch=0,covered=0;
    const unsigned char *coverage=mac_text_atlas(&bytes,&w,&h);
    glGenTextures(1,&target);glBindTexture(GL_TEXTURE_2D,target);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glGenFramebuffersEXT(1,&fbo);glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,target,0);
    CHECK(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT);
    glViewport(0,0,64,64);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,64,0,64,-1,1);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();glDisable(GL_DEPTH_TEST);glDisable(GL_DITHER);
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);glColor4f(1,1,1,1);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex2f(0,0);glTexCoord2f(64.0f/w,0);glVertex2f(64,0);
    glTexCoord2f(64.0f/w,64.0f/h);glVertex2f(64,64);glTexCoord2f(0,64.0f/h);glVertex2f(0,64);glEnd();
    glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,output);CHECK(glGetError()==GL_NO_ERROR);
    for(y=0;y<64;++y)for(x=0;x<64;++x){unsigned c=coverage[y*w+x],channel;
        if(c)++covered;for(channel=0;channel<3;++channel){int delta=(int)output[(y*64+x)*4+channel]-(int)c;if(delta < -1 || delta > 1)++mismatch;}}
    CHECK(covered>0);CHECK(mismatch==0);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);glDeleteFramebuffersEXT(1,&fbo);glDeleteTextures(1,&target);
}
int main(void)
{
    CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAColorSize,(CGLPixelFormatAttribute)24,0};
    CGLPixelFormatObj format=NULL;CGLContextObj context=NULL;GLint formats=0,maxsize=0;
    short widths[TEXT_HIRES_FIT_LETTERS];struct text_hires_glyph glyph;
    unsigned long placeholder[2]={0,0x1234};long font;unsigned i,before;GLuint texture;uint32_t digest=0;
    CHECK(!mac_text_gl_start(invalidate));CHECK(text_hires_atlas_texture(0x1234)==0);
    CHECK(CGLChoosePixelFormat(attrs,&format,&formats)==kCGLNoError && format);if(!format)return 1;
    CHECK(CGLCreateContext(format,NULL,&context)==kCGLNoError && context);if(!context)return 1;
    CHECK(CGLSetCurrentContext(context)==kCGLNoError);glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxsize);CHECK(maxsize>=2048);
    CHECK(mac_text_gl_start(invalidate));CHECK(!mac_text_gl_start(invalidate));
    for(i=0;i<TEXT_HIRES_FIT_LETTERS;++i)widths[i]=12;
    font=text_hires_font("ui\\large_ui",20,widths,1);CHECK(font>=0);
    text_hires_register_atlas(placeholder,256,256);CHECK(text_hires_glyph(font,'A',&glyph));
    CHECK(text_hires_atlas_texture(0x4321)==0);texture=text_hires_atlas_texture(0x1234);CHECK(texture && glIsTexture(texture));
    verify_atlas(texture,&digest);draw_glyph(texture);CHECK(invalidations==2);before=invalidations;
    CHECK(text_hires_atlas_texture(0x1234)==texture);CHECK(invalidations==before);
    CHECK(text_hires_glyph(font,'B',&glyph));CHECK(text_hires_atlas_texture(0x1234)==texture);CHECK(invalidations==before+1);verify_atlas(texture,&digest);
    CHECK(mac_text_set_pixel_scale(2));CHECK(text_hires_glyph(font,'A',&glyph));CHECK(text_hires_atlas_texture(0x1234)==texture);verify_atlas(texture,&digest);
    CGLSetCurrentContext(NULL);CHECK(text_hires_atlas_texture(0x1234)==0);CGLSetCurrentContext(context);
    mac_text_dispose();CHECK(!glIsTexture(texture));CHECK(glGetError()==GL_NO_ERROR);
    CGLSetCurrentContext(NULL);CGLDestroyContext(context);CGLDestroyPixelFormat(format);
    printf("native legacy GL font atlas/draw: %u checks %u failures; alpha digest=%08lx\n",checks,failures,(unsigned long)digest);
    return failures?1:0;
}
