/* Hidden-context check for native ARGB words from packed OpenGL readback.
 * Compare against the current byte-wise RGBA conversion on either endian. */
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned checks,failures;
#define CHECK(x) do { ++checks; if(!(x)){++failures;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);} } while(0)

int main(void)
{
    CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAColorSize,
        (CGLPixelFormatAttribute)24,0};
    CGLPixelFormatObj format=NULL;CGLContextObj context=NULL;GLint count=0;
    GLuint framebuffer=0,color=0;
    unsigned char rgba[7*5*4];uint32_t packed[7*5],expected,flipped[9*5];
    unsigned x,y,pass;GLenum status;
    CHECK(sizeof(uint32_t)==4);
    CHECK(CGLChoosePixelFormat(attrs,&format,&count)==kCGLNoError&&format);
    if(!format)goto done;
    CHECK(CGLCreateContext(format,NULL,&context)==kCGLNoError&&context);
    if(!context)goto done;
    CHECK(CGLSetCurrentContext(context)==kCGLNoError);
    glGenFramebuffersEXT(1,&framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,framebuffer);
    glGenRenderbuffersEXT(1,&color);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,color);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_RGBA8,7,5);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
        GL_RENDERBUFFER_EXT,color);
    status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    CHECK(status==GL_FRAMEBUFFER_COMPLETE_EXT);
    if(status!=GL_FRAMEBUFFER_COMPLETE_EXT)goto done;
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glDisable(GL_DITHER);glEnable(GL_SCISSOR_TEST);
    glPixelStorei(GL_PACK_ALIGNMENT,4);glPixelStorei(GL_PACK_SWAP_BYTES,GL_FALSE);
    for(pass=0;pass<4;++pass){
        for(y=0;y<5;++y)for(x=0;x<7;++x){
            glScissor((GLint)x,(GLint)y,1,1);
            glClearColor(((x*37+pass*13)%256)/255.0f,
                ((y*51+pass*29)%256)/255.0f,
                ((x*17+y*23+pass*41)%256)/255.0f,
                ((x*11+y*31+pass*67)%256)/255.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        glReadPixels(0,0,7,5,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
        CHECK(glGetError()==GL_NO_ERROR);
        /* No explicit glFinish: readback must itself provide completed data. */
        glReadPixels(0,0,7,5,GL_BGRA,GL_UNSIGNED_INT_8_8_8_8_REV,packed);
        CHECK(glGetError()==GL_NO_ERROR);
        memset(flipped,0xa5,sizeof(flipped));
        for(y=0;y<5;++y)memcpy(flipped+y*9,packed+(4-y)*7,7*sizeof(uint32_t));
        for(y=0;y<5;++y){
            for(x=0;x<7;++x){
                const unsigned char *p=rgba+((4-y)*7+x)*4;
                expected=((uint32_t)p[3]<<24)|((uint32_t)p[0]<<16)|
                    ((uint32_t)p[1]<<8)|(uint32_t)p[2];
                CHECK(flipped[y*9+x]==expected);
            }
            CHECK(flipped[y*9+7]==0xa5a5a5a5U&&flipped[y*9+8]==0xa5a5a5a5U);
        }
    }
done:
    if(context){
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
        if(color)glDeleteRenderbuffersEXT(1,&color);
        if(framebuffer)glDeleteFramebuffersEXT(1,&framebuffer);
        CGLSetCurrentContext(NULL);CGLDestroyContext(context);
    }
    if(format)CGLDestroyPixelFormat(format);
    printf("native packed readback: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
