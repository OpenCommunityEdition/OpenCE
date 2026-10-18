#include "mac_vertex_upload.h"
#include "mac_gl_program_cache.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 32
#define HEIGHT 32
#define RECORD_FLOATS (MAC_VERTEX_UPLOAD_ATTRIBUTES * 4)

static unsigned checks, failures;
static void check(int condition, const char *what)
{
    ++checks;
    if (!condition) { ++failures; fprintf(stderr, "FAIL: %s\n", what); }
}

static GLuint make_program(struct mac_gl_program_cache *cache,
    unsigned long requested_mask, unsigned long *linked_mask)
{
    char vertex_source[4096];
    const char *fragment_source =
        "#version 120\n"
        "varying vec4 color;\n"
        "void main(){gl_FragColor=color;}\n";
    GLuint program = 0;
    char error[256];
    unsigned i;
    vertex_source[0] = '\0';
    strcat(vertex_source, "#version 120\n");
    for (i = 0; i < MAC_VERTEX_UPLOAD_ATTRIBUTES; ++i) if (requested_mask & (1UL << i)) {
        char line[48];
        sprintf(line, "attribute vec4 v%u_in;\n", i);
        strcat(vertex_source, line);
    }
    strcat(vertex_source, "varying vec4 color;\nvoid main(){\n");
    if (requested_mask == ((1UL << MAC_VERTEX_UPLOAD_ATTRIBUTES) - 1UL)) {
        strcat(vertex_source, "vec4 p=vec4(0.0); vec4 c=vec4(0.0);\n");
        for (i = 0; i < MAC_VERTEX_UPLOAD_ATTRIBUTES; ++i) {
            char line[96];
            sprintf(line, "p+=v%u_in; c+=v%u_in;\n", i, i);
            strcat(vertex_source, line);
        }
        strcat(vertex_source, "gl_Position=p; color=c;\n}\n");
    } else {
        strcat(vertex_source, "gl_Position=v1_in; color=v7_in;\n}\n");
    }
    if (!mac_gl_program_cache_get(cache,vertex_source,fragment_source,
        requested_mask,&program,error,sizeof(error))) {
        fprintf(stderr,"program cache: %s\n",error);return 0;
    }
    if (!mac_gl_program_cache_active_attributes(cache,program,linked_mask)) return 0;
    return program;
}

static void fill_records(float *records, unsigned long mask)
{
    unsigned v, a;
    static const float positions[3][4] = {
        {-1.0f,-1.0f,0.0f,1.0f}, {3.0f,-1.0f,0.0f,1.0f}, {-1.0f,3.0f,0.0f,1.0f}
    };
    static const float colors[4] = {0.25f,0.5f,0.75f,1.0f};
    for (v = 0; v < 3; ++v) for (a = 0; a < MAC_VERTEX_UPLOAD_ATTRIBUTES; ++a) {
        float *dst = records + v * RECORD_FLOATS + a * 4;
        if (mask == ((1UL << MAC_VERTEX_UPLOAD_ATTRIBUTES) - 1UL)) {
            /* Every input stays live; their sum reconstructs the triangle. */
            dst[0] = positions[v][0] / 16.0f;
            dst[1] = positions[v][1] / 16.0f;
            dst[2] = positions[v][2] / 16.0f;
            dst[3] = positions[v][3] / 16.0f;
        } else if (a == 1) memcpy(dst, positions[v], 4 * sizeof(float));
        else if (a == 7) memcpy(dst, colors, 4 * sizeof(float));
        else { dst[0] = 91.0f + a; dst[1] = -37.0f - a; dst[2] = 11.0f; dst[3] = 1.0f; }
    }
}

static int draw_readback(struct mac_vertex_upload *upload, const float *records,
    unsigned long count, GLuint program, unsigned long mask, int compact,
    unsigned char *pixels)
{
    unsigned i;
    struct mac_vertex_upload_layout layout;
    char error[128];
    GLint old_binding;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &old_binding);
    glUseProgram(program);
    glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0,0,WIDTH,HEIGHT);
    if (compact) {
        if (!mac_vertex_upload_begin(upload, records, count * RECORD_FLOATS * sizeof(float),
            count, mask, &layout, error, sizeof(error))) {
            fprintf(stderr,"compact upload failed: %s\n",error); return 0;
        }
        check(layout.stride_bytes == (unsigned)(16 * __builtin_popcount((unsigned)mask)),
            "compact stride equals active vec4 count");
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    for (i = 0; i < MAC_VERTEX_UPLOAD_ATTRIBUTES; ++i) if (mask & (1UL << i)) {
        const void *pointer;
        GLsizei stride;
        glEnableVertexAttribArray(i);
        if (compact) { pointer = (const void *)(uintptr_t)layout.offsets[i]; stride = (GLsizei)layout.stride_bytes; }
        else { pointer = records + i * 4; stride = RECORD_FLOATS * sizeof(float); }
        glVertexAttribPointer(i,4,GL_FLOAT,GL_FALSE,stride,pointer);
    }
    glDrawArrays(GL_TRIANGLES,0,3);
    glReadPixels(0,0,WIDTH,HEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    if (compact) for (i = 0; i < MAC_VERTEX_UPLOAD_ATTRIBUTES; ++i) if (mask & (1UL << i)) {
        GLint associated_buffer = 0;
        glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,&associated_buffer);
        check((GLuint)associated_buffer == layout.buffer,"attribute pointer is associated with the owned compact VBO");
    }
    for (i = 0; i < MAC_VERTEX_UPLOAD_ATTRIBUTES; ++i) glDisableVertexAttribArray(i);
    if (compact && !mac_vertex_upload_end(upload)) return 0;
    else if (!compact) glBindBuffer(GL_ARRAY_BUFFER,(GLuint)old_binding);
    if (compact) {
        glBindBuffer(GL_ARRAY_BUFFER,0);
        for (i = 0; i < MAC_VERTEX_UPLOAD_ATTRIBUTES; ++i) if (mask & (1UL << i))
            glVertexAttribPointer(i,4,GL_FLOAT,GL_FALSE,RECORD_FLOATS*sizeof(float),records+i*4);
        glBindBuffer(GL_ARRAY_BUFFER,(GLuint)old_binding);
    }
    check(glGetError() == GL_NO_ERROR, compact ? "compact GL draw/readback has no error" : "fallback GL draw/readback has no error");
    check(glIsBuffer((GLuint)old_binding) == GL_TRUE, "caller array-buffer binding restored");
    return 1;
}

int main(void)
{
    CGLPixelFormatObj format = NULL;
    CGLContextObj context = NULL;
    CGLPBufferObj pbuffer = NULL;
    CGLPixelFormatAttribute attrs[] = { kCGLPFAAccelerated, kCGLPFAColorSize,
        (CGLPixelFormatAttribute)24, kCGLPFADepthSize,
        (CGLPixelFormatAttribute)24, (CGLPixelFormatAttribute)0 };
    GLint formats = 0;
    GLuint framebuffer = 0, color = 0, sentinel = 0;
    struct mac_vertex_upload upload;
    struct mac_gl_program_cache program_cache;
    unsigned long masks[2] = { (1UL<<1)|(1UL<<7), (1UL<<MAC_VERTEX_UPLOAD_ATTRIBUTES)-1UL };
    unsigned long m;
    int overall_ok = 1;
    if (CGLChoosePixelFormat(attrs,&format,&formats)!=kCGLNoError || !format ||
        CGLCreateContext(format,NULL,&context)!=kCGLNoError || !context ||
        CGLSetCurrentContext(context)!=kCGLNoError) return 2;
    if (!mac_vertex_upload_supported()) { fprintf(stderr,"VBO API unavailable\n"); overall_ok=0; }
    glGenFramebuffersEXT(1,&framebuffer);glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,framebuffer);
    glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,WIDTH,HEIGHT,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,color,0);
    check(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT,
        "offscreen framebuffer complete");
    glGenBuffers(1,&sentinel);glBindBuffer(GL_ARRAY_BUFFER,sentinel);glBufferData(GL_ARRAY_BUFFER,16,NULL,GL_STATIC_DRAW);
    mac_vertex_upload_init(&upload);
    mac_gl_program_cache_init(&program_cache);
    for (m=0;m<2;++m) {
        unsigned long linked_mask=0;GLuint program;
        float records[3*RECORD_FLOATS];
        unsigned char *fallback=(unsigned char *)malloc(WIDTH*HEIGHT*4);
        unsigned char *compact=(unsigned char *)malloc(WIDTH*HEIGHT*4);
        fill_records(records,masks[m]);program=make_program(&program_cache,masks[m],&linked_mask);
        check(program!=0,"vertex/fragment program links");
        check(linked_mask==masks[m],"effective active-attribute mask matches the linked program");
        if (!program || !fallback || !compact) { overall_ok=0;free(fallback);free(compact);continue; }
        if (m == 0) {
            char error[128];struct mac_vertex_upload_layout rejected;
            check(!mac_vertex_upload_begin(&upload,records,sizeof(records)-sizeof(float),
                3,linked_mask,&rejected,error,sizeof(error)) && error[0],
                "short source span fails closed before reading");
            check(!mac_vertex_upload_begin(&upload,records,sizeof(records),3,
                1UL<<MAC_VERTEX_UPLOAD_ATTRIBUTES,&rejected,error,sizeof(error)) && error[0],
                "out-of-range attribute mask fails closed");
        }
        check(draw_readback(&upload,records,3,program,linked_mask,0,fallback),"legacy 256-byte-stride client-array fallback draws");
        check(draw_readback(&upload,records,3,program,linked_mask,1,compact),"compact VBO draw succeeds");
        check(memcmp(fallback,compact,WIDTH*HEIGHT*4)==0,"compact VBO pixels exactly match fallback pixels");
        free(fallback);free(compact);
    }
    check(mac_vertex_upload_dispose(&upload),"upload storage disposes while context is current");
    check(glIsBuffer(sentinel)==GL_TRUE,"caller-owned sentinel buffer remains live");
    glUseProgram(0);mac_gl_program_cache_dispose(&program_cache);
    glDeleteBuffers(1,&sentinel);glDeleteTextures(1,&color);glDeleteFramebuffersEXT(1,&framebuffer);
    CGLSetCurrentContext(NULL);if(context)CGLDestroyContext(context);if(format)CGLDestroyPixelFormat(format);
    printf("native compact VBO: %u checks, %u failures\n",checks,failures);
    return overall_ok && failures==0 ? 0 : 1;
}
