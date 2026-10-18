/* Standalone diagnostic for Tiger/Leopard. No SDL or game data needed.
 * Build on the target: cc -std=c89 -Wall -Wextra tools/mac_capabilities.c
 *   -framework OpenGL -o mac_capabilities
 * Creates an accelerated CGL context without opening a window.
 */
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>
#include <sys/utsname.h>

struct double_layout { char first; double value; };

static void triangle(float z)
{
    glBegin(GL_TRIANGLES);
    glVertex3f(-1.0f, -1.0f, z);
    glVertex3f(1.0f, -1.0f, z);
    glVertex3f(0.0f, 1.0f, z);
    glEnd();
}

/* Exercise the constant bank size used by the NV2A translator, with dynamic
 * indexing so the shader compiler cannot discard the unused entries. */
static GLuint compile_shader(GLenum kind, const char *source)
{
    GLuint shader;
    GLint ok;
    char log[4096];
    GLsizei length;
    shader = glCreateShader(kind);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    glGetShaderInfoLog(shader, sizeof(log), &length, log);
    if (length) printf("shader_log=%s\n", log);
    if (!ok) { glDeleteShader(shader); return 0; }
    return shader;
}

static int check_shader_texture(void)
{
    const char *vertex = "#version 120\n"
        "uniform vec4 bank[192]; uniform int slot; varying vec2 uv;\n"
        "void main(){ gl_Position=gl_Vertex+bank[slot];"
        "uv=gl_MultiTexCoord0.xy; }\n";
    const char *fragment = "#version 120\n"
        "uniform sampler2D image; varying vec2 uv;\n"
        "void main(){ gl_FragColor=texture2D(image,uv); }\n";
    GLuint vs, fs, program, texture;
    GLint ok, location, limit;
    GLsizei length;
    char log[4096];
    GLfloat bank[192 * 4];
    unsigned char texel[4] = { 17, 83, 201, 255 };
    unsigned char pixel[4];
    int passed;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &limit);
    printf("vertex_uniform_components=%d\n", (int)limit);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &limit);
    printf("fragment_uniform_components=%d\n", (int)limit);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &limit);
    printf("fragment_texture_units=%d\n", (int)limit);
    vs = compile_shader(GL_VERTEX_SHADER, vertex);
    fs = compile_shader(GL_FRAGMENT_SHADER, fragment);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        printf("glsl120_compile=FAIL\n");
        return 0;
    }
    program = glCreateProgram();
    glAttachShader(program, vs); glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    glGetProgramInfoLog(program, sizeof(log), &length, log);
    if (length) printf("program_log=%s\n", log);
    passed = 0;
    if (ok) {
        glUseProgram(program);
        memset(bank, 0, sizeof(bank));
        location = glGetUniformLocation(program, "bank[0]");
        glUniform4fv(location, 192, bank);
        glUniform1i(glGetUniformLocation(program, "slot"), 191);
        glUniform1i(glGetUniformLocation(program, "image"), 0);
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, texel);
        glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        glTexCoord2f(0.5f, 0.5f);
        triangle(0);
        glReadPixels(4, 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        passed = location >= 0 && memcmp(pixel, texel, 4) == 0;
        printf("glsl120_texture_bank192=%s rgba=%u,%u,%u,%u\n",
            passed ? "pass" : "FAIL", pixel[0], pixel[1], pixel[2], pixel[3]);
        /* A nonzero final bank entry must move the triangle offscreen.
         * This checks that dynamic indexing actually affects the draw. */
        bank[191 * 4] = 4.0f;
        glUniform4fv(location, 192, bank);
        glClear(GL_COLOR_BUFFER_BIT);
        triangle(0);
        glReadPixels(4, 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        ok = pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0;
        printf("glsl120_bank191_displacement=%s rgba=%u,%u,%u,%u\n",
            ok ? "pass" : "FAIL", pixel[0], pixel[1], pixel[2], pixel[3]);
        passed = passed && ok;
        glUseProgram(0);
        glDeleteTextures(1, &texture);
    } else printf("glsl120_link=FAIL\n");
    glDeleteProgram(program); glDeleteShader(vs); glDeleteShader(fs);
    return passed;
}

/* Tiny offscreen workload; never binds a drawable or touches the desktop. */
static int check_rendering(void)
{
    GLuint framebuffer, color, depth;
    GLenum status, error;
    unsigned char pixel[4];
    int passed;
    glGenFramebuffersEXT(1, &framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 8, 8, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
        GL_TEXTURE_2D, color, 0);
    glGenRenderbuffersEXT(1, &depth);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, 8, 8);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
        GL_RENDERBUFFER_EXT, depth);
    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    passed = 0;
    if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
        glViewport(0, 0, 8, 8);
        glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
        glDisable(GL_DITHER);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glColor4f(1, 0, 0, 1);
        triangle(-0.5f);
        glColor4f(0, 1, 0, 1);
        triangle(0.5f);
        glReadPixels(4, 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        passed = pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 0;
        printf("offscreen_triangle_depth=%s rgba=%u,%u,%u,%u\n",
            passed ? "pass" : "FAIL", pixel[0], pixel[1], pixel[2], pixel[3]);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0, 0, 1, 0.5f);
        triangle(0);
        glReadPixels(4, 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        /* Allow two 8-bit levels of fixed-point blend quantization.
         * R300-era hardware measured 128,0,126 for the ideal 127.5,0,127.5.
         * Keep the actual bytes in the report; this is not bit-exact parity. */
        passed = passed && pixel[0] >= 126 && pixel[0] <= 129 &&
            pixel[1] == 0 && pixel[2] >= 126 && pixel[2] <= 129;
        printf("offscreen_blend=%s rgba=%u,%u,%u,%u\n",
            passed ? "pass" : "FAIL", pixel[0], pixel[1], pixel[2], pixel[3]);
    } else {
        printf("offscreen_framebuffer=FAIL status=0x%x\n", (unsigned)status);
    }
    if (status == GL_FRAMEBUFFER_COMPLETE_EXT)
        passed = check_shader_texture() && passed;
    error = glGetError();
    printf("offscreen_gl_error=0x%x\n", (unsigned)error);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glDeleteRenderbuffersEXT(1, &depth);
    glDeleteTextures(1, &color);
    glDeleteFramebuffersEXT(1, &framebuffer);
    return passed && error == GL_NO_ERROR;
}

static int has_extension(const char *list, const char *name)
{
    const char *at;
    size_t length;
    if (!list) return 0;
    length = strlen(name);
    at = list;
    while ((at = strstr(at, name)) != NULL) {
        if ((at == list || at[-1] == ' ') &&
            (at[length] == ' ' || at[length] == '\0')) return 1;
        at += length;
    }
    return 0;
}

static const char *gl_string(GLenum name)
{
    const GLubyte *value;
    value = glGetString(name);
    return value ? (const char *)value : "unavailable";
}

int main(void)
{
    CGLPixelFormatAttribute attributes[] = {
        kCGLPFAAccelerated, kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
        kCGLPFADepthSize, (CGLPixelFormatAttribute)24,
        (CGLPixelFormatAttribute)0
    };
    const char *extensions;
    const char *names[] = {
        "GL_ARB_multitexture", "GL_ARB_vertex_buffer_object",
        "GL_ARB_vertex_shader", "GL_ARB_fragment_shader",
        "GL_ARB_shading_language_100", "GL_ARB_vertex_program",
        "GL_ARB_fragment_program", "GL_EXT_framebuffer_object",
        "GL_EXT_texture_compression_s3tc", "GL_ARB_occlusion_query",
        "GL_APPLE_vertex_array_object", "GL_ARB_texture_non_power_of_two"
    };
    CGLPixelFormatObj format;
    CGLContextObj context;
    CGLError error;
    GLint count, texture_size, texture_units;
    unsigned int endian = 1;
    unsigned int i;
    int rendering_ok;
    struct utsname system;
    format = NULL;
    context = NULL;
    if (uname(&system) == 0)
        printf("system=%s %s %s\n", system.sysname, system.release, system.machine);
    printf("pointer_bytes=%lu long_bytes=%lu wchar_bytes=%lu double_offset=%lu\n",
        (unsigned long)sizeof(void *), (unsigned long)sizeof(long),
        (unsigned long)sizeof(wchar_t),
        (unsigned long)offsetof(struct double_layout, value));
    printf("byte_order=%s\n", *(unsigned char *)&endian ? "little" : "big");
    error = CGLChoosePixelFormat(attributes, &format, &count);
    if (error != kCGLNoError || !format) {
        fprintf(stderr, "accelerated pixel format unavailable: %s\n", CGLErrorString(error));
        return 1;
    }
    error = CGLCreateContext(format, NULL, &context);
    CGLDestroyPixelFormat(format);
    if (error != kCGLNoError) {
        fprintf(stderr, "context creation failed: %s\n", CGLErrorString(error));
        return 1;
    }
    error = CGLSetCurrentContext(context);
    if (error != kCGLNoError) {
        fprintf(stderr, "context activation failed: %s\n", CGLErrorString(error));
        CGLDestroyContext(context);
        return 1;
    }
    printf("vendor=%s\nrenderer=%s\nversion=%s\n",
        gl_string(GL_VENDOR), gl_string(GL_RENDERER), gl_string(GL_VERSION));
    extensions = (const char *)glGetString(GL_EXTENSIONS);
    if (has_extension(extensions, "GL_ARB_shading_language_100"))
        printf("shading_language=%s\n", gl_string(GL_SHADING_LANGUAGE_VERSION));
    texture_size = texture_units = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &texture_units);
    printf("max_texture_size=%d fixed_function_texture_units=%d\n",
        (int)texture_size, (int)texture_units);
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        printf("%s=%d\n", names[i], has_extension(extensions, names[i]));
    printf("extensions=%s\n", extensions ? extensions : "unavailable");
    rendering_ok = 1;
    if (has_extension(extensions, "GL_EXT_framebuffer_object"))
        rendering_ok = check_rendering();
    else
        printf("offscreen_rendering=SKIPPED (no EXT framebuffer support)\n");
    CGLSetCurrentContext(NULL);
    CGLDestroyContext(context);
    return rendering_ok ? 0 : 2;
}
