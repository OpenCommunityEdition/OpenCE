#include "../port/mac/mac_glsl120.h"
#include "../port/mac/mac_index_decode_cache.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, failures;
#define CHECK(value) do { ++checks; if (!(value)) { ++failures; \
    fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#value); } } while (0)

#define WIDTH 64U
#define HEIGHT 64U
#define VERTEX_COUNT 12U
#define RECORD_FLOATS 12U
#define PIXEL_BYTES (WIDTH * HEIGHT * 4U)

static const char modern_vertex[]=
    "#version 450 core\n"
    "layout(location = 0) in vec4 v0_in;\n"
    "layout(location = 1) in vec4 v1_in;\n"
    "layout(location = 2) in vec4 v2_in;\n"
    "out vec4 xD0;\n"
    "invariant gl_Position;\n"
    "void main()\n{\n"
    "\tvec4 v0 = v0_in;\n"
    "\tvec4 v1 = v1_in;\n"
    "\tvec4 v2 = v2_in;\n"
    "\tgl_Position = v0;\n"
    "\txD0 = clamp(v1 + v2 * 0.125, 0.0, 1.0);\n}\n";
static const char modern_pixel[]=
    "#version 450 core\n"
    "in vec4 xD0;\n"
    "layout(location = 0) out vec4 fragment_color;\n"
    "void main()\n{\n fragment_color = xD0;\n}\n";

static GLuint compile_shader(GLenum kind,const char *source)
{
    GLuint shader=glCreateShader(kind);GLint compiled=0,length=0;char log[2048];
    glShaderSource(shader,1,&source,NULL);glCompileShader(shader);
    glGetShaderiv(shader,GL_COMPILE_STATUS,&compiled);
    if(!compiled){glGetShaderInfoLog(shader,sizeof(log),&length,log);
        fprintf(stderr,"shader compile failed: %s\n",log);glDeleteShader(shader);return 0;}
    return shader;
}

static void make_record(unsigned short key,float record[RECORD_FLOATS])
{
    static const float positions[4][2]={
        {-0.82f,-0.62f},{-0.72f,0.72f},{0.72f,-0.62f},{0.82f,0.72f}};
    unsigned which;
    if(key==0)which=0;else if(key==2)which=1;else if(key==5)which=2;else which=3;
    record[0]=positions[which][0];record[1]=positions[which][1];
    record[2]=0.1f;record[3]=1.0f;
    record[4]=(key==0||key==9)?1.0f:0.05f;
    record[5]=(key==2||key==9)?1.0f:0.05f;
    record[6]=(key==5||key==9)?1.0f:0.05f;record[7]=1.0f;
    record[8]=(float)key*(1.0f/16.0f);
    record[9]=(float)(9U-key)*(1.0f/16.0f);
    record[10]=(float)(key&3U)*(1.0f/8.0f);record[11]=0.25f;
}

static void draw_elements_preserving_binding(GLenum primitive,GLsizei count,
    const unsigned short *indices)
{
    GLint old_binding=0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&old_binding);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    glDrawElements(primitive,count,GL_UNSIGNED_SHORT,indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,(GLuint)old_binding);
}

static unsigned changed_pixels(const unsigned char *pixels)
{
    unsigned long at;unsigned count=0;
    for(at=0;at<PIXEL_BYTES;at+=4)
        if(pixels[at]||pixels[at+1]||pixels[at+2])++count;
    return count;
}

static void check_topology(GLenum primitive,const char *name,int quads,
    const unsigned short keys[VERTEX_COUNT],GLuint sentinel_buffer,
    unsigned char *baseline,unsigned char *remapped)
{
    struct mac_index_decode_cache cache={0};
    unsigned short map[VERTEX_COUNT],quad_indices[(VERTEX_COUNT/4U)*6U];
    unsigned i,slot,minimum=0xffffU,maximum=0;int result;GLint binding=0;
    unsigned long before_pixels,after_pixels;

    for(i=0;i<VERTEX_COUNT;i++){
        if(keys[i]<minimum)minimum=keys[i];if(keys[i]>maximum)maximum=keys[i];
    }
    CHECK(mac_index_decode_cache_begin(&cache,VERTEX_COUNT,minimum,maximum)==1);
    for(i=0;i<VERTEX_COUNT;i++){
        result=mac_index_decode_cache_lookup(&cache,keys[i],&slot);
        if(result==1)map[i]=(unsigned short)slot;
        else {
            CHECK(result==0);map[i]=(unsigned short)i;
            CHECK(mac_index_decode_cache_record(&cache,keys[i],i)==1);
        }
    }
    CHECK(map[0]==0&&map[1]==0&&map[2]==2&&map[3]==2&&
        map[4]==4&&map[5]==4&&map[6]==6&&map[7]==6&&
        map[8]==0&&map[9]==2&&map[10]==4&&map[11]==6);
    CHECK(cache.unique_count==4&&cache.reuse_count==VERTEX_COUNT-4);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sentinel_buffer);
    glClear(GL_COLOR_BUFFER_BIT);
    if(quads){
        unsigned q,out;
        for(q=0;q<VERTEX_COUNT/4U;q++){
            unsigned first=q*4U;out=q*6U;
            quad_indices[out]=(unsigned short)first;
            quad_indices[out+1]=(unsigned short)(first+1U);
            quad_indices[out+2]=(unsigned short)(first+2U);
            quad_indices[out+3]=(unsigned short)first;
            quad_indices[out+4]=(unsigned short)(first+2U);
            quad_indices[out+5]=(unsigned short)(first+3U);
        }
        draw_elements_preserving_binding(GL_TRIANGLES,(GLsizei)((VERTEX_COUNT/4U)*6U),quad_indices);
    }else glDrawArrays(primitive,0,(GLsizei)VERTEX_COUNT);
    glFinish();glReadPixels(0,0,WIDTH,HEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,baseline);
    CHECK(glGetError()==GL_NO_ERROR);
    before_pixels=changed_pixels(baseline);

    glClear(GL_COLOR_BUFFER_BIT);
    if(quads){
        unsigned q,out;
        for(q=0;q<VERTEX_COUNT/4U;q++){
            unsigned first=q*4U;out=q*6U;
            quad_indices[out]=map[first];quad_indices[out+1]=map[first+1U];
            quad_indices[out+2]=map[first+2U];quad_indices[out+3]=map[first];
            quad_indices[out+4]=map[first+2U];quad_indices[out+5]=map[first+3U];
        }
        draw_elements_preserving_binding(GL_TRIANGLES,(GLsizei)((VERTEX_COUNT/4U)*6U),quad_indices);
    }else draw_elements_preserving_binding(primitive,(GLsizei)VERTEX_COUNT,map);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&binding);
    CHECK(binding==(GLint)sentinel_buffer);
    glFinish();glReadPixels(0,0,WIDTH,HEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,remapped);
    CHECK(glGetError()==GL_NO_ERROR);
    after_pixels=changed_pixels(remapped);
    CHECK(before_pixels>0&&after_pixels==before_pixels);
    CHECK(!memcmp(baseline,remapped,PIXEL_BYTES));
    printf("indexed submit %s: coverage=%lu bytes_match=%d\n",name,
        before_pixels,!memcmp(baseline,remapped,PIXEL_BYTES));
    mac_index_decode_cache_dispose(&cache);
}

int main(void)
{
    static const unsigned short keys[VERTEX_COUNT]={5,5,2,2,9,9,0,0,5,2,9,0};
    static const struct {GLenum primitive;const char *name;int quads;} cases[]={
        {GL_POINTS,"points",0},{GL_LINES,"lines",0},{GL_LINE_STRIP,"line-strip",0},
        {GL_TRIANGLES,"triangles",0},{GL_TRIANGLE_STRIP,"triangle-strip",0},
        {GL_TRIANGLE_FAN,"triangle-fan",0},{GL_TRIANGLES,"quad-triangulation",1}};
    CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAColorSize,
        (CGLPixelFormatAttribute)24,0};
    CGLPixelFormatObj format=NULL;CGLContextObj context=NULL;GLint format_count=0,status=0;
    char *vertex_source=NULL,*pixel_source=NULL,error[160];unsigned long mask=0;
    GLuint vs=0,fs=0,program=0,color_texture=0,framebuffer=0,sentinel=0;
    unsigned char *baseline=NULL,*remapped=NULL;unsigned i;
    CHECK(mac_glsl120_vertex(modern_vertex,&vertex_source,&mask,error,sizeof(error)));
    CHECK(mac_glsl120_pixel(modern_pixel,&pixel_source,error,sizeof(error)));
    CHECK(mask==7UL);
    if(!vertex_source||!pixel_source)goto done;
    CHECK(CGLChoosePixelFormat(attrs,&format,&format_count)==kCGLNoError&&format);
    if(!format)goto done;
    CHECK(CGLCreateContext(format,NULL,&context)==kCGLNoError&&context);
    if(!context)goto done;
    CHECK(CGLSetCurrentContext(context)==kCGLNoError);
    vs=compile_shader(GL_VERTEX_SHADER,vertex_source);fs=compile_shader(GL_FRAGMENT_SHADER,pixel_source);
    CHECK(vs&&fs);if(!vs||!fs)goto done;
    program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);
    glBindAttribLocation(program,0,"v0_in");glBindAttribLocation(program,1,"v1_in");
    glBindAttribLocation(program,2,"v2_in");glLinkProgram(program);
    glGetProgramiv(program,GL_LINK_STATUS,&status);
    if(!status){char log[2048];GLint length=0;glGetProgramInfoLog(program,sizeof(log),&length,log);fprintf(stderr,"program link failed: %s\n",log);}
    CHECK(status);if(!status)goto done;
    glGenTextures(1,&color_texture);glBindTexture(GL_TEXTURE_2D,color_texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,WIDTH,HEIGHT,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glGenFramebuffersEXT(1,&framebuffer);glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,framebuffer);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
        GL_TEXTURE_2D,color_texture,0);
    CHECK(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT);
    glViewport(0,0,WIDTH,HEIGHT);glDisable(GL_DITHER);glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_SCISSOR_TEST);
    glClearColor(0,0,0,0);glUseProgram(program);
    baseline=(unsigned char *)malloc(PIXEL_BYTES);remapped=(unsigned char *)malloc(PIXEL_BYTES);
    CHECK(baseline&&remapped);if(!baseline||!remapped)goto done;
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glEnableVertexAttribArray(2);
    /* Each topology gets its own client records immediately before its draws. */
    glGenBuffers(1,&sentinel);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sentinel);
    {unsigned short sentinel_word=0;glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(sentinel_word),&sentinel_word,GL_STATIC_DRAW);}
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        float records_for_case[VERTEX_COUNT*RECORD_FLOATS];unsigned v;
        for(v=0;v<VERTEX_COUNT;v++)make_record(keys[v],records_for_case+v*RECORD_FLOATS);
        glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,RECORD_FLOATS*sizeof(float),records_for_case);
        glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,RECORD_FLOATS*sizeof(float),records_for_case+4);
        glVertexAttribPointer(2,4,GL_FLOAT,GL_FALSE,RECORD_FLOATS*sizeof(float),records_for_case+8);
        check_topology(cases[i].primitive,cases[i].name,cases[i].quads,
            keys,sentinel,baseline,remapped);
    }
done:
    if(context){CGLSetCurrentContext(context);if(sentinel)glDeleteBuffers(1,&sentinel);
        if(framebuffer)glDeleteFramebuffersEXT(1,&framebuffer);
        if(color_texture)glDeleteTextures(1,&color_texture);
        if(program)glDeleteProgram(program);if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);
        CGLSetCurrentContext(NULL);CGLDestroyContext(context);}
    if(format)CGLDestroyPixelFormat(format);
    free(vertex_source);free(pixel_source);free(baseline);free(remapped);
    printf("mac indexed submit parity: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
