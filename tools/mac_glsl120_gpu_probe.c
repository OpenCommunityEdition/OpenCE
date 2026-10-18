#include "../port/mac/mac_glsl120.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); } } while (0)

static const char vertex[]=
    "#version 450 core\n"
    "uniform vec4 c[192];\n"
    "out vec4 xD0;\n"
    "invariant gl_Position;\n"
    "vec4 unpack_normpacked3(uint p)\n{\n return vec4(0.0);\n}\n"
    "layout(location = 0) in vec4 v0_in;\n"
    "void main()\n{\n vec4 v0 = v0_in;\n gl_Position = v0;\n xD0 = vec4(0.25, 0.5, 0.75, 1.0);\n}\n";
static const char prune_vertex[]=
    "#version 450 core\n"
    "uniform vec4 c[192];\n"
    "out vec4 xD0;\n"
    "invariant gl_Position;\n"
    "layout(location = 0) in vec4 v0_in;\n"
    "layout(location = 1) in vec4 v1_in;\n"
    "layout(location = 10) in vec4 v10_in;\n"
    "void main()\n{\n"
    "\tvec4 v0 = v0_in;\n\tvec4 v1 = v1_in;\n\tvec4 v10 = v10_in;\n"
    "\tgl_Position = v0;\n\txD0 = v10;\n}\n";
static const char relative_vertex[]=
    "#version 450 core\n"
    "uniform vec4 c[192];\n"
    "uniform float address;\n"
    "out vec4 xD0;\n"
    "layout(location = 0) in vec4 v0_in;\n"
    "void main()\n{\n int a0 = int(address);\n gl_Position = v0_in;\n xD0 = c[clamp(a0 + 5, 0, 191)];\n}\n";
static const char pixel[]=
    "#version 450 core\n"
    "in vec4 xD0;\n"
    "layout(location = 0) out vec4 fragment_color;\n"
    "uniform vec4 fog_color;\n"
    "void main()\n{\n fragment_color = clamp(xD0, 0.0, 1.0);\n}\n";

static const char sampler_pixel[]=
    "#version 450 core\n"
    "layout(location = 0) out vec4 fragment_color;\n"
    "uniform sampler2D tex1;\n"
    "uniform sampler3D tex2;\n"
    "uniform samplerCube tex3;\n"
    "void main() { fragment_color = texture(tex1, vec2(0.5)) + texture(tex2, vec3(0.5)) + texture(tex3, vec3(1.0,0.0,0.0)); }\n";

static GLuint compile_shader(GLenum kind,const char *source)
{
    GLuint shader=glCreateShader(kind);GLint compiled=0;GLint length=0;char log[2048];
    glShaderSource(shader,1,&source,NULL);glCompileShader(shader);glGetShaderiv(shader,GL_COMPILE_STATUS,&compiled);
    if(!compiled){glGetShaderInfoLog(shader,sizeof(log),&length,log);fprintf(stderr,"shader compile: %s\n",log);glDeleteShader(shader);return 0;}
    return shader;
}

int main(int argc,char **argv)
{
    CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAColorSize,(CGLPixelFormatAttribute)24,0};
    CGLPixelFormatObj format=NULL;CGLContextObj context=NULL;GLint formats=0;
    char *vs=NULL,*fs=NULL,error[128];unsigned long attributes=0;
    GLuint vshader=0,fshader=0,program=0,fbo=0,texture=0,depth_buffer=0,sampler_textures[3]={0,0,0};
    GLfloat vertices[]={-0.8f,-0.8f,0,1, 0.8f,-0.8f,0,1, 0,0.8f,0,1};
    int relative=argc==2&&!strcmp(argv[1],"relative");
    int samplers=argc==2&&!strcmp(argv[1],"samplers");
    int depth=argc==2&&!strcmp(argv[1],"depth");
    int prune=argc==2&&!strcmp(argv[1],"prune");
    unsigned char pixel_value[4]={0,0,0,0};GLint status=0;
    CHECK(mac_glsl120_vertex(prune?prune_vertex:(relative?relative_vertex:vertex),
        &vs,&attributes,error,sizeof(error)) && attributes==(prune?1025UL:1UL));
    CHECK(mac_glsl120_pixel(samplers?sampler_pixel:pixel,&fs,error,sizeof(error)));
    if(!vs||!fs)goto done;
    CHECK(CGLChoosePixelFormat(attrs,&format,&formats)==kCGLNoError && format);if(!format)goto done;
    CHECK(CGLCreateContext(format,NULL,&context)==kCGLNoError && context);if(!context)goto done;
    CHECK(CGLSetCurrentContext(context)==kCGLNoError);
    vshader=compile_shader(GL_VERTEX_SHADER,vs);fshader=compile_shader(GL_FRAGMENT_SHADER,fs);
    CHECK(vshader && fshader);if(!vshader||!fshader)goto done;
    program=glCreateProgram();glAttachShader(program,vshader);glAttachShader(program,fshader);
    glBindAttribLocation(program,0,"v0_in");glBindAttribLocation(program,10,"v10_in");
    glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&status);
    if(!status){char log[2048];GLint length;glGetProgramInfoLog(program,sizeof(log),&length,log);fprintf(stderr,"program link: %s\n",log);}
    CHECK(status);
    if(prune) {
        CHECK(glGetAttribLocation(program,"v0_in")==0);
        CHECK(glGetAttribLocation(program,"v1_in")==-1);
        CHECK(glGetAttribLocation(program,"v10_in")==10);
    }
    glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,32,32,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glGenFramebuffersEXT(1,&fbo);glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,texture,0);
    if(depth) {
        glGenRenderbuffersEXT(1,&depth_buffer);glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depth_buffer);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT24,32,32);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depth_buffer);
    }
    CHECK(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT);
    glViewport(0,0,32,32);glDisable(GL_DITHER);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);glEnableVertexAttribArray(0);glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,0,vertices);
    if(prune)glVertexAttrib4f(10,0.25f,0.5f,0.75f,1.0f);
    if(samplers) {
        static const unsigned char colors[3][4]={{64,0,0,255},{0,128,0,0},{0,0,192,0}};
        static const GLenum targets[3]={GL_TEXTURE_2D,GL_TEXTURE_3D,GL_TEXTURE_CUBE_MAP};
        unsigned stage,face;
        glGenTextures(3,sampler_textures);
        for(stage=0;stage<3;++stage) {
            char name[8];sprintf(name,"tex%u",stage+1);
            glActiveTexture(GL_TEXTURE1+stage);glBindTexture(targets[stage],sampler_textures[stage]);
            glTexParameteri(targets[stage],GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(targets[stage],GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            if(stage==1)glTexImage3D(GL_TEXTURE_3D,0,GL_RGBA8,1,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,colors[stage]);
            else for(face=0;face<(stage==2?6U:1U);++face)
                glTexImage2D(stage==2?GL_TEXTURE_CUBE_MAP_POSITIVE_X+face:GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,colors[stage]);
            glUniform1i(glGetUniformLocation(program,name),stage+1);
        }
        glActiveTexture(GL_TEXTURE0);
    }
    if(depth) {
        static const GLfloat input_depths[]={0.0f,0.5f,1.0f};
        unsigned test;GLfloat read_depth;
        glDisable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_ALWAYS);glDepthMask(GL_TRUE);
        glClearDepth(1.0);
        for(test=0;test<3;++test) {
            vertices[2]=vertices[6]=vertices[10]=input_depths[test];
            glClear(GL_DEPTH_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);
            read_depth=1.0f;glReadPixels(16,12,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&read_depth);
            CHECK(glGetError()==GL_NO_ERROR);
            CHECK(read_depth>=input_depths[test]-0.002f&&read_depth<=input_depths[test]+0.002f);
        }
        goto done;
    }
    if(relative) {
        GLfloat constants[192][4];
        static const float addresses[5]={-300,-5,0,186,300};
        static const unsigned channels[5]={0,0,1,2,2};
        GLint address=glGetUniformLocation(program,"address");
        GLint constant=glGetUniformLocation(program,"c");
        unsigned test,channel;
        memset(constants,0,sizeof(constants));
        constants[0][0]=1;constants[0][3]=1;
        constants[5][1]=1;constants[5][3]=1;
        constants[191][2]=1;constants[191][3]=1;
        CHECK(address>=0&&constant>=0);
        glUniform4fv(constant,192,&constants[0][0]);
        for(test=0;test<5;++test) {
            glUniform1f(address,addresses[test]);glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES,0,3);
            glReadPixels(16,12,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel_value);
            CHECK(glGetError()==GL_NO_ERROR);
            for(channel=0;channel<4;++channel)
                CHECK(pixel_value[channel]==(channel==channels[test]||channel==3?255:0));
        }
        goto done;
    }
    glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(16,12,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel_value);
    CHECK(glGetError()==GL_NO_ERROR);
    CHECK(pixel_value[0]>=63&&pixel_value[0]<=64);
    CHECK(pixel_value[1]>=127&&pixel_value[1]<=128);
    CHECK(pixel_value[2]>=191&&pixel_value[2]<=192&&pixel_value[3]==255);
done:
    if(context){CGLSetCurrentContext(context);glDeleteTextures(3,sampler_textures);if(depth_buffer)glDeleteRenderbuffersEXT(1,&depth_buffer);if(fbo)glDeleteFramebuffersEXT(1,&fbo);if(texture)glDeleteTextures(1,&texture);
        if(program)glDeleteProgram(program);if(vshader)glDeleteShader(vshader);if(fshader)glDeleteShader(fshader);
        CGLSetCurrentContext(NULL);CGLDestroyContext(context);}
    if(format)CGLDestroyPixelFormat(format);free(vs);free(fs);
    printf("mac GLSL 1.20 bounded shader draw: %d checks, %d failures; rgba=%u,%u,%u,%u\n",
        checks,failures,pixel_value[0],pixel_value[1],pixel_value[2],pixel_value[3]);
    return failures?1:0;
}
