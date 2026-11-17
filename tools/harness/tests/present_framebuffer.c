#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef int GLint;
typedef int BOOL;
enum
{
    FALSE, GL_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER, GL_READ_FRAMEBUFFER,
    GL_COLOR_ATTACHMENT0, GL_NONE, GL_RENDERBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
    GL_TEXTURE_2D, GL_FRAMEBUFFER_COMPLETE, GL_SCISSOR_TEST, GL_TRUE,
    GL_COLOR_BUFFER_BIT, GL_RGBA16F, GL_LINEAR
};

struct framebuffer_entry
{
    struct framebuffer_entry *next;
    GLuint color, depth, framebuffer;
    BOOL renderbuffers;
};
static struct framebuffer_entry *framebuffers;
static GLuint draw_binding, read_binding, next_framebuffer, colors[16];
static unsigned int blits, clears, shader_presents;
static int shader_success;
static GLuint expected_texture;

static void glGenFramebuffers(int count, GLuint *value)
{
    assert(count == 1 && next_framebuffer + 1 < 16);
    *value = ++next_framebuffer;
}

static void glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
        draw_binding = framebuffer;
    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER)
        read_binding = framebuffer;
}

static void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum type, GLuint texture, int level)
{
    assert(target == GL_FRAMEBUFFER && draw_binding != 0);
    if (attachment == GL_COLOR_ATTACHMENT0)
        colors[draw_binding] = texture;
}

static void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum type, GLuint buffer)
{
    glFramebufferTexture2D(target, attachment, type, buffer, 0);
}

static GLenum glCheckFramebufferStatus(GLenum target) { return GL_FRAMEBUFFER_COMPLETE; }
static void glDrawBuffers(int count, const GLenum *buffers) { }
static void xgpu_gl_state_invalidate(void) { }
#define platform_log(...) ((void)0)
#include "cache.inc"

static void glDisable(GLenum state) { }
static void glColorMask(int r, int g, int b, int a) { }
static void glClearColor(float r, float g, float b, float a) { }
static void glClear(GLenum mask)
{
    assert(draw_binding == 0);
    clears++;
}

static void glBlitFramebuffer(int sx0, int sy0, int sx1, int sy1,
    int dx0, int dy0, int dx1, int dy1, GLenum mask, GLenum filter)
{
    if (draw_binding != 0 || read_binding == 0 || colors[read_binding] != expected_texture)
    {
        fprintf(stderr, "presentation blit reads FBO %u and draws FBO %u; expected window 0\n",
            read_binding, draw_binding);
        exit(1);
    }
    blits++;
}

static int xgpu_post_present(GLuint texture, int x, int y, int width, int height, float white)
{
    assert(draw_binding == 0 && texture == expected_texture);
    if (shader_success)
        shader_presents++;
    return shader_success;
}

static void present(GLuint texture, int floating)
{
    struct test_buffer { struct { GLuint texture; GLenum format; int gl_width, gl_height; } target; } buffer;
    struct test_buffer *back_buffer = &buffer;
    struct { int float_window; float sdr_white; } hdr = { 0, 1.0f };
    int x = 0, y = 0, width = 640, height = 480;

    buffer.target.texture = expected_texture = texture;
    buffer.target.format = floating ? GL_RGBA16F : 0;
    buffer.target.gl_width = width;
    buffer.target.gl_height = height;
#include "present.inc"
}

int main(int argc, char **argv)
{
    int floating;

    assert(argc == 2);
    floating = !strncmp(argv[1], "hdr", 3);
    shader_success = !strcmp(argv[1], "hdr");
    if (!strcmp(argv[1], "primed"))
        framebuffer_get(11, 0);
    if (!strcmp(argv[1], "changed"))
        framebuffer_get(9, 0);
    present(11, floating);
    present(11, floating);
    present(12, floating);
    assert(clears == 3);
    assert(shader_success ? shader_presents == 3 && blits == 0 : blits == 3);
    while (framebuffers)
    {
        struct framebuffer_entry *next = framebuffers->next;
        free(framebuffers);
        framebuffers = next;
    }
    return 0;
}
