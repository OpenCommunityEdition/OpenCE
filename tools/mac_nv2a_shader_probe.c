#include <OpenGL/OpenGL.h>
#include "../port/mac/mac_nv2a_shader_api.h"
#include "../port/mac/mac_glsl120.h"
#include "../source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.h"
#undef memset
#undef memcpy
#undef memcmp
#undef malloc
#undef realloc
#undef free
#undef strcmp
#undef strlen
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int checks,failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); } } while (0)
static unsigned occurrences(const char *text,const char *needle)
{
    unsigned count=0;size_t length=strlen(needle);const char *at=text;
    while((at=strstr(at,needle))!=NULL){++count;at+=length;}
    return count;
}
int main(int argc,char **argv)
{
    CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAColorSize,(CGLPixelFormatAttribute)24,0};
    CGLPixelFormatObj format=NULL;CGLContextObj context=NULL;GLint formats=0;
    unsigned long i,accepted=0,rejected=0,generated=0,compiled=0;
    unsigned long pruned_input_count=0,linked_active_count=0;
    unsigned long relative_shaders=0,relative_reads=0,promotion_failures=0;
    char error[160];
    FILE *dump=NULL;
    char *pixel_source=NULL,*pixel_legacy=NULL;
    GLuint pixel_shader=0,first_program=0;
    unsigned char pixels[32*32*4];
    static const float positions[3][4]={{-0.75f,-0.75f,0.5f,1.0f},{0.75f,-0.75f,0.5f,1.0f},{0.0f,0.75f,0.5f,1.0f}};
    float attributes[3][16][4];
    float constants[192][4];
    GLuint framebuffer=0,color_texture=0;
    if(argc>1){dump=fopen(argv[1],"wb");if(!dump){perror(argv[1]);return 2;}}
    memset(attributes,0,sizeof(attributes));memset(constants,0,sizeof(constants));
    for(i=0;i<3;i++){memcpy(attributes[i][0],positions[i],sizeof(positions[i]));
        attributes[i][9][0]=0.25f;attributes[i][9][1]=0.5f;attributes[i][9][2]=0.75f;attributes[i][9][3]=1.0f;
        attributes[i][0][3]=1.0f;}
    constants[0][0]=1.0f;constants[1][1]=1.0f;constants[2][2]=1.0f;constants[3][3]=1.0f;
    constants[58][0]=16.0f;constants[58][1]=-16.0f;constants[58][2]=1.0f;
    constants[59][0]=16.0f;constants[59][1]=16.0f;
    {
        struct nv2a_pixel_shader_key key;
        char *modern;
        memset(&key,0,sizeof(key));
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD]=0x04200000;
        key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG]=0x00001400;
        modern=nv2a_pixel_shader_to_glsl(&key);
        if(!modern||!mac_glsl120_pixel(modern,&pixel_legacy,error,sizeof(error))){
            fprintf(stderr,"untextured retail pixel shader rejected: %s\n",error);free(modern);return 1;}
        pixel_source=modern;
    }
    CHECK(CGLChoosePixelFormat(attrs,&format,&formats)==kCGLNoError && format);if(!format)return 1;
    CHECK(CGLCreateContext(format,NULL,&context)==kCGLNoError && context);if(!context)return 1;
    CHECK(CGLSetCurrentContext(context)==kCGLNoError);
    {
        GLuint shader=glCreateShader(GL_FRAGMENT_SHADER);GLint status=0;
        glShaderSource(shader,1,(const char **)&pixel_legacy,NULL);glCompileShader(shader);glGetShaderiv(shader,GL_COMPILE_STATUS,&status);
        if(!status){char log[2048];GLint n;glGetShaderInfoLog(shader,sizeof(log),&n,log);fprintf(stderr,"fragment compile: %s\n",log);}
        CHECK(status);if(status)pixel_shader=shader;else glDeleteShader(shader);
    }
    for(i=0;i<NUMBER_OF_VERTEX_SHADERS;i++){
        const struct vertex_shader_entry *entry=&vertex_shader_table[i];
        const DWORD *function=(const DWORD *)entry->code;
        unsigned long instruction_count=function[0]>>16;
        unsigned modern_reads,legacy_reads;int expects_promotion;
        char *modern,*legacy=NULL;unsigned long input_mask=0;GLuint shader=0,program=0;GLint status=0;
        modern=nv2a_vertex_shader_to_glsl(function+1,instruction_count,0,NULL);
        if(!modern){fprintf(stderr,"generator failed at retail VS %lu\n",i);return 1;}
        ++generated;
        modern_reads=occurrences(modern,"c[clamp(a0 + ");
        expects_promotion=modern_reads&&occurrences(modern,"int a0 = 0;")==1&&
            occurrences(modern,"a0 = int(floor(mac.x + 0.001));")>0;
        if(expects_promotion)++relative_shaders;
        if(mac_glsl120_vertex(modern,&legacy,&input_mask,error,sizeof(error))){
            ++accepted;
            legacy_reads=occurrences(legacy,"c[int(clamp(float(a0) + ");
            if(expects_promotion){
                ++relative_reads;
                if(legacy_reads!=modern_reads||occurrences(legacy,"float a0 = 0.0;")!=1||
                   occurrences(legacy,"int a0 = 0;")||
                   occurrences(legacy,"a0 = int(floor(mac.x + 0.001));")){
                    ++promotion_failures;
                    fprintf(stderr,"retail VS %lu did not promote all %u bounded relative reads (legacy=%u)\n",
                        i,modern_reads,legacy_reads);
                }
                CHECK(legacy_reads==modern_reads&&occurrences(legacy,"float a0 = 0.0;")==1&&
                    occurrences(legacy,"int a0 = 0;")==0&&
                    occurrences(legacy,"a0 = int(floor(mac.x + 0.001));")==0);
            } else CHECK(legacy_reads==0);
            if(dump&&i==0){fputs("--- generated retail VS 0, GLSL 4.50 ---\n",dump);fputs(modern,dump);
                fputs("\n--- converted GLSL 1.20 ---\n",dump);fputs(legacy,dump);}
            shader=glCreateShader(GL_VERTEX_SHADER);glShaderSource(shader,1,(const char **)&legacy,NULL);glCompileShader(shader);
            glGetShaderiv(shader,GL_COMPILE_STATUS,&status);
            if(status&&pixel_shader){
                unsigned long a;program=glCreateProgram();glAttachShader(program,shader);glAttachShader(program,pixel_shader);
                for(a=0;a<16;a++){char name[24];sprintf(name,"v%lu_in",a);glBindAttribLocation(program,(GLuint)a,name);}
                glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&status);
                if(status)for(a=0;a<16;a++){
                    char name[24];GLint location;
                    sprintf(name,"v%lu_in",a);location=glGetAttribLocation(program,name);
                    if(input_mask&(1UL<<a)){if(location>=0)++linked_active_count;}
                    else {++pruned_input_count;CHECK(location==-1);}
                }
            }
            if(!status){char log[2048];GLint n;glGetShaderInfoLog(shader,sizeof(log),&n,log);
                fprintf(stderr,"retail VS %lu did not compile/link: %s\n",i,log);}
            else ++compiled;
            if(i==0&&status)first_program=program;
            else if(program)glDeleteProgram(program);
            if(shader)glDeleteShader(shader);
        }else ++rejected;
        free(modern);free(legacy);
    }
    CHECK(accepted==NUMBER_OF_VERTEX_SHADERS && compiled==NUMBER_OF_VERTEX_SHADERS && rejected==0);
    CHECK(pruned_input_count>0);
    if(first_program){
        GLint location;glGenTextures(1,&color_texture);glBindTexture(GL_TEXTURE_2D,color_texture);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,32,32,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
        glGenFramebuffersEXT(1,&framebuffer);glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,framebuffer);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,color_texture,0);
        CHECK(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT);
        glViewport(0,0,32,32);glDisable(GL_DITHER);glDisable(GL_DEPTH_TEST);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(first_program);
        location=glGetUniformLocation(first_program,"c");if(location>=0)glUniform4fv(location,192,&constants[0][0]);
        location=glGetUniformLocation(first_program,"viewport_scale");if(location>=0)glUniform4f(location,16,-16,1,0);
        location=glGetUniformLocation(first_program,"viewport_offset");if(location>=0)glUniform4f(location,16,16,0,0);
        location=glGetUniformLocation(first_program,"screen_offset");if(location>=0)glUniform1f(location,0);
        location=glGetUniformLocation(first_program,"point_size");if(location>=0)glUniform1f(location,1);
        for(i=0;i<16;i++){unsigned long v;for(v=0;v<3;v++)attributes[v][i][3]=1.0f;
            glEnableVertexAttribArray((GLuint)i);glVertexAttribPointer((GLuint)i,4,GL_FLOAT,GL_FALSE,sizeof(attributes[0]),&attributes[0][i][0]);}
        glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        CHECK(glGetError()==GL_NO_ERROR);
        CHECK(pixels[0]>=63&&pixels[0]<=64);
        CHECK(pixels[1]>=127&&pixels[1]<=128);
        CHECK(pixels[2]>=191&&pixels[2]<=192&&pixels[3]==255);
    }else CHECK(0);
    if(dump)fclose(dump);
    printf("NV2A retail GPU probe: %lu generated, %lu GLSL120 accepted, %lu compiled/linked, %lu rejected; active_inputs=%lu pruned_input_slots=%lu relative-promoted=%lu reads=%lu failures=%lu; checks=%d failures=%d\n",
        generated,accepted,compiled,rejected,linked_active_count,pruned_input_count,relative_shaders,relative_reads,promotion_failures,checks,failures);
    if(context){CGLSetCurrentContext(context);if(framebuffer)glDeleteFramebuffersEXT(1,&framebuffer);if(color_texture)glDeleteTextures(1,&color_texture);
        if(first_program)glDeleteProgram(first_program);if(pixel_shader)glDeleteShader(pixel_shader);
        CGLSetCurrentContext(NULL);CGLDestroyContext(context);}if(format)CGLDestroyPixelFormat(format);
    free(pixel_source);free(pixel_legacy);
    return generated==NUMBER_OF_VERTEX_SHADERS&&compiled==NUMBER_OF_VERTEX_SHADERS&&failures==0?0:1;
}
