/* Real GL draws, synthetic stock texture: no game assets. */
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
/* GL POINTERS */
typedef struct {unsigned int Width,Height,Format;} D3DSURFACE_DESC;
typedef struct {int Pitch;void *pBits;} D3DLOCKED_RECT;
typedef int IDirect3DTexture8;
#define D3DFMT_LIN_A8B8G8R8 1
#define D3DLOCK_READONLY 1
static unsigned char pixels[320*240*4];
static int enabled, ready=1, invalidations, invalid_format;
static unsigned long width,height;
static GLuint framebuffer, target;
static int config_boolean(const char *name) {assert(!strcmp(name,"display.widescreen_loading"));return enabled;}
static int xgpu_gl_ready(void) {return ready;}
static void xgpu_gl_state_invalidate(void) {invalidations++;}
static void xgpu_gl_texture_deleted(GLuint id) {(void)id;}
static void platform_log(const char *format,...) {(void)format;}
static void IDirect3DTexture8_GetLevelDesc(void *texture,int level,D3DSURFACE_DESC *desc) {
    (void)texture;(void)level;desc->Width=320;desc->Height=240;desc->Format=invalid_format?0:1;
}
static void IDirect3DTexture8_LockRect(void *texture,int level,D3DLOCKED_RECT *rect,void *area,int flags) {
    (void)texture;(void)level;(void)area;(void)flags;rect->Pitch=1280;rect->pBits=pixels;
}
static void IDirect3DTexture8_UnlockRect(void *texture,int level) {(void)texture;(void)level;}
static int xgpu_presentation_target(unsigned long *w,unsigned long *h) {
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);*w=width;*h=height;return 1;
}
static GLuint xgpu_compile_shader(GLenum type,const char *text,const char *label) {
    GLuint shader=glCreateShader(type);GLint ok;char log[4096];(void)label;
    glShaderSource(shader,1,&text,NULL);glCompileShader(shader);glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok){glGetShaderInfoLog(shader,sizeof(log),NULL,log);fprintf(stderr,"%s\n",log);exit(1);}return shader;
}
static GLuint xgpu_link_program(GLuint vertex,GLuint fragment,const char *label) {
    GLuint program=glCreateProgram();GLint ok;(void)label;
    glAttachShader(program,vertex);glAttachShader(program,fragment);glLinkProgram(program);
    glGetProgramiv(program,GL_LINK_STATUS,&ok);assert(ok);return program;
}
/* PRODUCTION */
static void resize(unsigned long w,unsigned long h) {
    width=w;height=h;glBindTexture(GL_TEXTURE_2D,target);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
}
static unsigned int intensity(int x,int y) {
    unsigned char sample[4];glReadPixels(x,y,1,1,GL_RGBA,GL_UNSIGNED_BYTE,sample);
    assert(sample[3]==255);return sample[0];
}
int main(void) {
    SDL_Window *window;SDL_GLContext context;int stock=1,before,x,y;GLuint texture;
    unsigned int sizes[][2]={{640,480},{1920,1080},{2560,1080},{600,800}};
    unsigned int i;
    assert(SDL_Init(SDL_INIT_VIDEO));
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,4);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    window=SDL_CreateWindow("Loading presenter test",64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);assert(window);
    context=SDL_GL_CreateContext(window);assert(context);
    /* GL BINDINGS */
    glGenTextures(1,&target);glGenFramebuffers(1,&framebuffer);
    for(y=0;y<240;y++)for(x=0;x<320;x++) {
        unsigned char *pixel=pixels+(y*320+x)*4;
        pixel[0]=180;pixel[1]=(unsigned char)y;pixel[2]=(unsigned char)(x/2);pixel[3]=255;
    }
    before=invalidations;assert(!loading_widescreen_draw(.5f,&stock));assert(before==invalidations);
    enabled=1;ready=0;assert(!loading_widescreen_draw(.5f,&stock));ready=1;
    for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++) {
        resize(sizes[i][0],sizes[i][1]);
        /* Stale game state must not clip the pass to a central/split view. */
        glViewport(10,10,20,20);glEnable(GL_SCISSOR_TEST);glScissor(0,0,1,1);
        glEnable(GL_CULL_FACE);glColorMask(0,0,0,0);
        assert(loading_widescreen_draw(.5f,&stock));assert(glGetError()==GL_NO_ERROR);
        assert(intensity(0,0)>20 && intensity(width-1,height-1)>20);
        assert(intensity(width/2,height/2)>intensity(width/10,height/2)*2);
        /* Uniform cover: top row samples the cropped source row, not zero. */
        if(i==1) {
            unsigned char sample[4];glReadPixels(width/2,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,sample);
            assert(sample[1]>=5 && sample[1]<=8);
        }
        assert(loading_widescreen_draw(1.f,&stock));assert(intensity(width/2,height/2)==0);
        assert(loading_widescreen_draw(NAN,&stock));assert(intensity(width/2,height/2)==0);
    }
    texture=loading.texture;enabled=0;assert(!loading_widescreen_draw(.5f,&stock));
    assert(!loading.texture && !loading.program && !loading.vertex_array && !glIsTexture(texture));
    enabled=1;invalid_format=1;assert(!loading_widescreen_draw(.5f,&stock));assert(loading.failed);
    loading_widescreen_end();invalid_format=0;assert(loading_widescreen_draw(.5f,&stock));
    texture=loading.texture;loading_widescreen_end();assert(!glIsTexture(texture));
    loading_widescreen_end();assert(glGetError()==GL_NO_ERROR);
    glDeleteTextures(1,&target);glDeleteFramebuffers(1,&framebuffer);
    SDL_GL_DestroyContext(context);SDL_DestroyWindow(window);SDL_Quit();
    puts("full coverage, crop, sweep, resize, disabled path, failure recovery and cleanup passed");return 0;
}
