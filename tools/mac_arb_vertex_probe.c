#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); } } while (0)

#ifndef GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#endif
#ifndef GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#endif
#ifndef GL_MAX_PROGRAM_INSTRUCTIONS_ARB
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB 0x88A1
#endif
#ifndef GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A3
#endif
#ifndef GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A7
#endif
#ifndef GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AB
#endif
#ifndef GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A2
#endif
#ifndef GL_PROGRAM_NATIVE_TEMPORARIES_ARB
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#endif
#ifndef GL_PROGRAM_NATIVE_PARAMETERS_ARB
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AA
#endif
#ifndef GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AF
#endif
#ifndef GL_PROGRAM_NATIVE_ATTRIBS_ARB
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AE
#endif
#ifndef GL_MAX_VARYING_FLOATS
#define GL_MAX_VARYING_FLOATS 0x8B4B
#endif
#ifndef GL_PROGRAM_ERROR_POSITION_ARB
#define GL_PROGRAM_ERROR_POSITION_ARB 0x864B
#endif
#ifndef GL_PROGRAM_ERROR_STRING_ARB
#define GL_PROGRAM_ERROR_STRING_ARB 0x8874
#endif

static const char vertex_program[] =
    "!!ARBvp1.0\n"
    "ATTRIB position = vertex.position;\n"
    "ATTRIB address = vertex.attrib[1];\n"
    "OUTPUT clip = result.position;\n"
    "OUTPUT color = result.color;\n"
    "PARAM palette[4] = { program.local[0..3] };\n"
    "TEMP bounded;\n"
    "ADDRESS index;\n"
    "MOV clip, position;\n"
    "MAX bounded.x, address.x, 0.0;\n"
    "MIN bounded.x, bounded.x, 3.0;\n"
    "ARL index.x, bounded.x;\n"
    "MOV color, palette[index.x];\n"
    "END\n";

static int has_extension(const char *list, const char *name)
{
    size_t n = strlen(name);
    const char *p = list;
    while ((p = strstr(p, name)) != NULL) {
        if ((p == list || p[-1] == ' ') && (p[n] == '\0' || p[n] == ' ')) return 1;
        p += n;
    }
    return 0;
}

static GLuint compile_fragment(void)
{
    static const char source[] =
        "#version 120\n"
        "void main() { gl_FragColor = gl_Color; }\n";
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint program;
    GLint ok = GL_FALSE;
    const GLchar *src = source;
    if (!shader) return 0;
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        char log[1024]; GLint len = 0;
        glGetShaderInfoLog(shader, sizeof(log), &len, log);
        if (len < 0) len = 0; if ((unsigned)len >= sizeof(log)) len = sizeof(log)-1; log[len] = 0;
        fprintf(stderr, "fragment-only GLSL compile failed: %s\n", log);
        glDeleteShader(shader); return 0;
    }
    program = glCreateProgram();
    if (!program) { glDeleteShader(shader); return 0; }
    glAttachShader(program, shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        char log[1024]; GLint len = 0;
        glGetProgramInfoLog(program, sizeof(log), &len, log);
        if (len < 0) len = 0; if ((unsigned)len >= sizeof(log)) len = sizeof(log)-1; log[len] = 0;
        fprintf(stderr, "fragment-only GLSL link failed: %s\n", log);
        glDeleteProgram(program); program = 0;
    }
    glDeleteShader(shader);
    return program;
}

int main(void)
{
    CGLPixelFormatAttribute attrs[] = { kCGLPFAAccelerated, kCGLPFAColorSize,
        (CGLPixelFormatAttribute)24, 0 };
    CGLPixelFormatObj format = NULL;
    CGLContextObj context = NULL;
    GLint formats = 0, max_local = 0, max_instructions = 0, under_native = 0;
    GLint max_native_instructions = 0, max_native_parameters = 0, max_native_temporaries = 0;
    GLint native_instructions = 0, native_parameters = 0, native_temporaries = 0;
    GLint max_native_attribs = 0, native_attribs = 0;
    GLint max_vertex_attribs = 0, max_texture_units = 0, max_varying_floats = 0;
    GLint error_position = -1;
    const GLubyte *extensions, *vendor, *renderer, *version;
    GLuint vp = 0, fp = 0, fbo = 0, tex = 0;
    GLenum status = 0, gl_error = GL_NO_ERROR;
    GLfloat vertices[] = {
        -0.71875f,-0.71875f,0.0f,1.0f, -20.0f,
        -0.34375f,-0.71875f,0.0f,1.0f,   0.0f,
         0.03125f,-0.71875f,0.0f,1.0f,   1.0f,
         0.40625f,-0.71875f,0.0f,1.0f,   2.0f,
         0.78125f,-0.71875f,0.0f,1.0f,   3.0f,
         0.03125f, 0.78125f,0.0f,1.0f,  20.0f
    };
    GLfloat palette[4][4] = {
        {1.0f,0.0f,0.0f,1.0f}, {0.0f,1.0f,0.0f,1.0f},
        {0.0f,0.0f,1.0f,1.0f}, {1.0f,1.0f,0.0f,1.0f}
    };
    unsigned i;
    unsigned char pixels[6][4];
    static const int sample_xy[6][2] = {{4,4},{10,4},{16,4},{22,4},{28,4},{16,28}};
    static const unsigned char expected[6][4] = {
        {255,0,0,255}, {255,0,0,255}, {0,255,0,255},
        {0,0,255,255}, {255,255,0,255}, {255,255,0,255}
    };

    CHECK(CGLChoosePixelFormat(attrs, &format, &formats) == kCGLNoError && format != NULL);
    if (!format) goto done;
    CHECK(CGLCreateContext(format, NULL, &context) == kCGLNoError && context != NULL);
    if (!context) goto done;
    CHECK(CGLSetCurrentContext(context) == kCGLNoError);
    extensions = glGetString(GL_EXTENSIONS);
    vendor = glGetString(GL_VENDOR); renderer = glGetString(GL_RENDERER); version = glGetString(GL_VERSION);
    printf("ARB vertex-program probe: vendor=%s renderer=%s version=%s\n",
        vendor ? (const char *)vendor : "?", renderer ? (const char *)renderer : "?",
        version ? (const char *)version : "?");
    if (!extensions || !has_extension((const char *)extensions, "GL_ARB_vertex_program")) {
        printf("capability absent: GL_ARB_vertex_program\n");
        goto done;
    }
    CHECK(glGetError() == GL_NO_ERROR);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &max_local);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_INSTRUCTIONS_ARB, &max_instructions);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &max_native_instructions);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &max_native_parameters);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &max_native_temporaries);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB, &max_native_attribs);
    CHECK(glGetError() == GL_NO_ERROR);
    printf("limits: virtual_local_parameters=%d virtual_instructions=%d native_instructions=%d native_parameters=%d native_temporaries=%d native_attribs=%d\n",
        (int)max_local, (int)max_instructions, (int)max_native_instructions,
        (int)max_native_parameters, (int)max_native_temporaries, (int)max_native_attribs);
    if (max_local < 4) { fprintf(stderr, "insufficient local parameter capacity\n"); goto done; }

    glGenProgramsARB(1, &vp);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vp);
    glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
        (GLsizei)(sizeof(vertex_program)-1), vertex_program);
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &error_position);
    if (error_position >= 0) {
        const GLubyte *msg = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
        fprintf(stderr, "ARBvp compile error position=%d text=%s\n", (int)error_position,
            msg ? (const char *)msg : "?");
    }
    CHECK(error_position < 0);
    CHECK(glGetError() == GL_NO_ERROR);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &under_native);
    CHECK(glGetError() == GL_NO_ERROR);
    printf("ARBvp status: under_native_limits=%d\n", (int)under_native);
    CHECK(under_native == GL_TRUE);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_INSTRUCTIONS_ARB, &max_instructions);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &native_instructions);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_PARAMETERS_ARB, &native_parameters);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEMPORARIES_ARB, &native_temporaries);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_ATTRIBS_ARB, &native_attribs);
    printf("ARBvp resources: virtual_instructions=%d native_instructions=%d native_parameters=%d native_temporaries=%d native_attribs=%d\n",
        (int)max_instructions, (int)native_instructions, (int)native_parameters,
        (int)native_temporaries, (int)native_attribs);
    CHECK(glGetError() == GL_NO_ERROR);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_texture_units);
    glGetIntegerv(GL_MAX_VARYING_FLOATS, &max_varying_floats);
    CHECK(glGetError() == GL_NO_ERROR);
    printf("fixed/GLSL limits: vertex_attribs=%d texture_units=%d varying_floats=%d\n",
        (int)max_vertex_attribs, (int)max_texture_units, (int)max_varying_floats);

    fp = compile_fragment(); CHECK(fp != 0); if (!fp) goto done;
    glUseProgram(fp);
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 0, palette[0]);
    glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 1, palette[1]);
    glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 2, palette[2]);
    glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 3, palette[3]);
    CHECK(glGetError() == GL_NO_ERROR);

    glGenFramebuffersEXT(1, &fbo); glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0);
    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    CHECK(status == GL_FRAMEBUFFER_COMPLETE_EXT);
    glViewport(0, 0, 32, 32); glDisable(GL_DITHER); glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST);
    glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), vertices);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), vertices+4);
    glDrawArrays(GL_TRIANGLES, 0, 3); glFinish();
    glPointSize(1.0f);
    glDrawArrays(GL_POINTS, 0, 6); glFinish();
    {
        GLint vertex_gpu = -1, fragment_gpu = -1;
        CGLError vertex_error, fragment_error;
        vertex_error = CGLGetParameter(context, kCGLCPGPUVertexProcessing, &vertex_gpu);
        fragment_error = CGLGetParameter(context, kCGLCPGPUFragmentProcessing, &fragment_gpu);
        printf("CGL execution: vertex_gpu=%d vertex_error=%d fragment_gpu=%d fragment_error=%d\n",
            (int)vertex_gpu, (int)vertex_error, (int)fragment_gpu, (int)fragment_error);
        CHECK(vertex_error == kCGLNoError && fragment_error == kCGLNoError);
    }
    for (i=0; i<6; ++i)
        glReadPixels(sample_xy[i][0], sample_xy[i][1], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels[i]);
    gl_error = glGetError(); CHECK(gl_error == GL_NO_ERROR);
    for (i=0; i<6; ++i) {
        printf("readback[%u]=%u,%u,%u,%u expected=%u,%u,%u,%u\n", i,
            pixels[i][0],pixels[i][1],pixels[i][2],pixels[i][3],
            expected[i][0],expected[i][1],expected[i][2],expected[i][3]);
        CHECK(memcmp(pixels[i], expected[i], 4) == 0);
    }
    CHECK(glGetError() == GL_NO_ERROR);

done:
    if (context) {
        CGLSetCurrentContext(context);
        if (fp) { glUseProgram(0); glDeleteProgram(fp); }
        glDisable(GL_VERTEX_PROGRAM_ARB);
        if (vp) glDeleteProgramsARB(1, &vp);
        if (fbo) glDeleteFramebuffersEXT(1, &fbo);
        if (tex) glDeleteTextures(1, &tex);
        CGLSetCurrentContext(NULL);
        CGLDestroyContext(context);
    }
    if (format) CGLDestroyPixelFormat(format);
    printf("ARB vertex-program feasibility: %u checks, %u failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
