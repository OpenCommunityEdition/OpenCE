#include "cseries.h"
#include "../port/mac/mac_glsl120.h"
#include "../port/mac/mac_bone_gather.h"
#include "../port/mac/mac_nv2a_shader_api.h"
#include "../source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.h"
#undef memset
#undef memcpy
#undef memcmp
#undef malloc
#undef realloc
#undef free
#undef strcmp
#undef strlen
#undef strcpy
#undef strcat
#undef sprintf
#include <float.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); } } while (0)

struct shader_case { const char *name; const char *expression; int fixed; };
static const struct shader_case cases[] = {
    {"production-bounded-int", "int a0 = int(floor(v1_in.x + 0.001));\n xD0 = c[clamp(a0 + 60, 0, 191)];", 0},
    {"float-offset-before-clamp", "int a0 = int(floor(v1_in.x + 0.001));\n xD0 = c[int(clamp(float(a0) + 60.0, 0.0, 191.0))];", 0},
    {"old-int-add-then-float", "int a0 = int(floor(v1_in.x + 0.001));\n xD0 = c[int(clamp(float(a0 + 60), 0.0, 191.0))];", 0},
    {"float-address-before-index", "float a0 = floor(v1_in.x + 0.001);\n xD0 = c[int(clamp(a0 + 60.0, 0.0, 191.0))];", 0},
    {"float-address-preclamp", "float a0 = floor(v1_in.x + 0.001);\n a0 = clamp(a0, -60.0, 131.0);\n xD0 = c[int(clamp(a0 + 60.0, 0.0, 191.0))];", 0},
    {"fixed-constant-control", "xD0 = c[5];", 1}
};
static const float addresses[] = {-61.0f,-60.0f,131.0f,132.0f,189.0f,
    -60.002f,-59.998f,130.998f,130.9995f,0.0f};
static const unsigned expected_index[] = {0,0,191,191,191,0,0,190,191,60};

static GLuint compile_shader(GLenum kind,const char *source)
{
    GLuint shader=glCreateShader(kind);GLint compiled=GL_FALSE;GLint written=0;char log[2048];
    glShaderSource(shader,1,&source,NULL);glCompileShader(shader);glGetShaderiv(shader,GL_COMPILE_STATUS,&compiled);
    if(compiled!=GL_TRUE){glGetShaderInfoLog(shader,sizeof(log),&written,log);if(written<0)written=0;if((size_t)written>=sizeof(log))written=sizeof(log)-1;log[written]='\0';fprintf(stderr,"GLSL compile failed: %s\n",log);glDeleteShader(shader);return 0;}
    return shader;
}

static char *replace_text_all(const char *source,const char *needle,const char *replacement,
    unsigned *replacement_count)
{
    size_t source_size=strlen(source),needle_size=strlen(needle),replacement_size=strlen(replacement);
    const char *cursor=source,*match;size_t count=0,capacity,used=0;char *result;
    while((match=strstr(cursor,needle))!=NULL){++count;cursor=match+needle_size;}
    *replacement_count=(unsigned)count;
    if(!count)return NULL;
    if(replacement_size>needle_size){size_t extra=replacement_size-needle_size;if(count>((size_t)-1-source_size-1)/extra)return NULL;capacity=source_size+count*extra+1;}
    else capacity=source_size+1;
    result=(char *)malloc(capacity);if(!result)return NULL;
    cursor=source;
    while((match=strstr(cursor,needle))!=NULL){size_t prefix=(size_t)(match-cursor);
        memcpy(result+used,cursor,prefix);used+=prefix;
        memcpy(result+used,replacement,replacement_size);used+=replacement_size;
        cursor=match+needle_size;}
    strcpy(result+used,cursor);return result;
}

static char *old_relative_source(const char *source)
{
    const char *cursor=source,*match;size_t source_size=strlen(source),used=0;
    char *result=(char *)malloc(source_size+2048);if(!result)return NULL;
    while((match=strstr(cursor,"c[clamp(a0 + "))!=NULL){unsigned long offset;int consumed=0,length;char replacement[96];size_t prefix=(size_t)(match-cursor);
        if(sscanf(match,"c[clamp(a0 + %lu, 0, 191)]%n",&offset,&consumed)!=1||!consumed||offset>255){free(result);return NULL;}
        memcpy(result+used,cursor,prefix);used+=prefix;
        length=snprintf(replacement,sizeof(replacement),"c[int(clamp(float(a0 + %lu), 0.0, 191.0))]",offset);
        if(length<0||(size_t)length>=sizeof(replacement)||used+(size_t)length>=source_size+2048){free(result);return NULL;}
        memcpy(result+used,replacement,(size_t)length);used+=(size_t)length;cursor=match+consumed;
    }
    strcpy(result+used,cursor);return result;
}

static unsigned identifier_count(const char *source,const char *identifier)
{
    size_t length=strlen(identifier);const char *cursor=source;unsigned count=0;
    while((cursor=strstr(cursor,identifier))!=NULL){
        int left=(cursor==source)||!( (cursor[-1]>='a'&&cursor[-1]<='z') ||
            (cursor[-1]>='A'&&cursor[-1]<='Z') ||
            (cursor[-1]>='0'&&cursor[-1]<='9') || cursor[-1]=='_');
        int right=!((cursor[length]>='a'&&cursor[length]<='z') ||
            (cursor[length]>='A'&&cursor[length]<='Z') ||
            (cursor[length]>='0'&&cursor[length]<='9') || cursor[length]=='_');
        if(left&&right)++count;
        cursor+=length;
    }
    return count;
}
static unsigned substring_count(const char *source,const char *needle)
{
    unsigned count=0;size_t length=strlen(needle);const char *cursor=source;
    if(!length)return 0;
    while((cursor=strstr(cursor,needle))!=NULL){++count;cursor+=length;}
    return count;
}
static void gpu_status(CGLContextObj context,const char *name,const char *phase);

static void expect_gather_refused(const char *source,const char *before,const char *after)
{
    unsigned changed=0;char *mutated,*output=NULL,error[128];struct mac_bone_gather_plan plan;
    mutated=replace_text_all(source,before,after,&changed);
    CHECK(mutated!=NULL&&changed>0);
    if(mutated){int result=mac_bone_gather_rewrite(mutated,&plan,&output,error,sizeof(error));
        CHECK(result==0&&output==NULL&&plan.version==0);}
    free(mutated);free(output);
}

static char *live_varying_pixel_source(const char *vertex_source,int varying_mode)
{
    const char *cursor=vertex_source,*start,*end,*vec4_decl,*float_decl;
    char names[64][24],types[64];unsigned count=0,i;size_t capacity=8192,used=0;
    char *source=(char *)malloc(capacity);float weight;
    if(!source)return NULL;
    while(1){
        vec4_decl=strstr(cursor,"out vec4 ");float_decl=strstr(cursor,"out float ");
        if(!vec4_decl&&!float_decl)break;
        if(count>=64){free(source);return NULL;}
        if(vec4_decl&&(!float_decl||vec4_decl<float_decl)){cursor=vec4_decl;types[count]='v';start=cursor+9;}
        else {cursor=float_decl;types[count]='f';start=cursor+10;}
        end=start;
        while((*end>='a'&&*end<='z')||(*end>='A'&&*end<='Z')||
            (*end>='0'&&*end<='9')||*end=='_')++end;
        if(end==start||count>=64||(size_t)(end-start)>=sizeof(names[0])){free(source);return NULL;}
        memcpy(names[count],start,(size_t)(end-start));names[count][end-start]='\0';
        if(varying_mode==2){if(!strcmp(names[count],"xD0")||!strcmp(names[count],"xT0"))++count;}
        else if(types[count]=='v'||varying_mode==1)++count;
        cursor=end;
    }
    if(!count){free(source);return NULL;}
    used+=(size_t)sprintf(source+used,"#version 450 core\nlayout(location = 0) out vec4 fragment_color;\n");
    for(i=0;i<count;++i)used+=(size_t)sprintf(source+used,"in %s %s;\n",types[i]=='v'?"vec4":"float",names[i]);
    used+=(size_t)sprintf(source+used,"void main()\n{\n fragment_color = vec4(0.0);\n");
    weight=1.0f/(float)count;
    for(i=0;i<count;++i)used+=(size_t)sprintf(source+used,
        types[i]=='v'?" fragment_color += %s * %.9g;\n":" fragment_color += vec4(%s) * %.9g;\n",
        names[i],(double)weight);
    used+=(size_t)sprintf(source+used," fragment_color = clamp(fragment_color, 0.0, 1.0);\n}\n");
    if(used>=capacity){free(source);return NULL;}
    return source;
}

static unsigned varying_output_count(const char *source)
{
    const char *cursor=source;unsigned count=0;
    while((cursor=strstr(cursor,"out vec4 "))!=NULL){++count;cursor+=9;}
    cursor=source;while((cursor=strstr(cursor,"out float "))!=NULL){++count;cursor+=10;}
    return count;
}

/* Diagnostic-only: remove a generated local/input pair only when exact token
 * counts prove the local appears solely in its copy declaration and the
 * input appears solely in that declaration plus its attribute declaration. */
static char *prune_unused_inputs(const char *source,unsigned *removed)
{
    const char *cursor=source,*line;size_t source_size=strlen(source),used=0;
    char *result=(char *)malloc(source_size+1);unsigned total=0,i;int drop[16]={0};
    if(!result)return NULL;
    for(i=0;i<16;++i){char local[24],input[24],assignment[64],declaration[64];
        sprintf(local,"v%u",i);sprintf(input,"v%u_in",i);
        sprintf(assignment,"vec4 %s = %s;",local,input);
        sprintf(declaration,"vec4 %s;",input);
        if(identifier_count(source,local)==1&&identifier_count(source,input)==2&&
            strstr(source,assignment)&&strstr(source,declaration))drop[i]=1;
    }
    while(*cursor){
        unsigned i;int skip=0;char line_text[4096];size_t line_size;
        line=cursor;while(*cursor&&*cursor!='\n')++cursor;
        if(*cursor=='\n')++cursor;
        line_size=(size_t)(cursor-line);if(line_size>=sizeof(line_text)){free(result);return NULL;}
        memcpy(line_text,line,line_size);line_text[line_size]='\0';
        for(i=0;i<16&&!skip;++i)if(drop[i]){char local[24],input[24],assignment[64],declaration[64];
            sprintf(local,"v%u",i);sprintf(input,"v%u_in",i);
            sprintf(assignment,"vec4 %s = %s;",local,input);
        sprintf(declaration,"vec4 %s;",input);
            if(strstr(line_text,assignment)||strstr(line_text,declaration))skip=1;
        }
        if(skip){++total;continue;}
        memcpy(result+used,line,(size_t)(cursor-line));used+=(size_t)(cursor-line);
    }
    result[used]='\0';*removed=total/2;return result;
}

static int test_cpu_gather_shader(CGLContextObj context,GLuint fbo,GLuint texture,unsigned shader_index,int varying_mode,int rsq_variant)
{
    const struct vertex_shader_entry *entry=&vertex_shader_table[shader_index];
    const DWORD *function=(const DWORD *)entry->code;unsigned long instructions=function[0]>>16,mask=0,i;
    char *modern=NULL,*gathered=NULL,*algebraic=NULL,*vertex_original=NULL,*vertex_gather=NULL,*pixel120=NULL,error[256];
    struct mac_bone_gather_plan gather_plan;
    const char *pixel450="#version 450 core\nin vec4 xD0;\nin vec4 xT0;\nlayout(location = 0) out vec4 fragment_color;\nvoid main()\n{\n fragment_color = clamp(xD0 + xT0, 0.0, 1.0);\n}\n";
    static const float address_pairs[4][2]={{-61.0f,-60.0f},{-60.0f,131.0f},{131.0f,189.0f},{0.0f,0.0f}};
    char *pixel450_live=NULL;
    GLuint programs[2]={0,0},shaders[2]={0,0},fs=0;GLint link=GL_FALSE,location;
    GLfloat constants[192][4],attributes[3][16][4];
    unsigned char reference[32*32*4],candidate[32*32*4],warm[32*32*4];
    GLenum gl_error;int recognized,ok=1,covered_any=0;
    modern=nv2a_vertex_shader_to_glsl(function+1,instructions,0,NULL);
    CHECK(modern!=NULL);if(!modern)return 0;
    if((shader_index==31||shader_index==50)&&getenv("HALO_DUMP_GATHER_SHADER")){
        fprintf(stderr,"BEGIN-GATHER-SHADER-%u\n%s\nEND-GATHER-SHADER-%u\n",
            shader_index,modern,shader_index);
    }
    {
        char *malformed=NULL,*rejected=NULL;unsigned replacements=0;struct mac_bone_gather_plan rejected_plan;
        unsigned product_register;char write_name[48],write_replacement[48],y_lane[40],wrong_lane[40],overwrite[64],y_lane_inserted[96];
        if(shader_index==9||shader_index==30){
        malformed=replace_text_all(modern,"c[clamp(a0 + 60, 0, 191)].xyzw",
            "c[clamp(a0 + 59, 0, 191)].xyzw",&replacements);
        CHECK(malformed!=NULL&&replacements==2);
        if(malformed){int rejected_result=mac_bone_gather_rewrite(malformed,&rejected_plan,
                &rejected,error,sizeof(error));
            CHECK(rejected_result==0&&rejected==NULL&&rejected_plan.version==0);}
        free(malformed);free(rejected);
        {
            const char *product_start=strstr(modern,"mac = A * B;\n\tr");int consumed=0;
            CHECK(product_start!=NULL);
            if(product_start&&sscanf(product_start,"mac = A * B;\n\tr%u.xy = mac.xy;%n",&product_register,&consumed)==1&&consumed){
                sprintf(write_name,"r%u.xy = mac.xy;",product_register);
                sprintf(write_replacement,"r%u.xy = vec2(0.0);",product_register);
                expect_gather_refused(modern,write_name,write_replacement);
                sprintf(y_lane,"A = r%u.yyyy;",product_register);sprintf(wrong_lane,"A = r%u.xxxx;",product_register);
                expect_gather_refused(modern,y_lane,wrong_lane);
                sprintf(overwrite,"r%u.x = 0.0;\n\tA = r%u.yyyy;",product_register,product_register);
                sprintf(y_lane_inserted,"A = r%u.yyyy;",product_register);
                expect_gather_refused(modern,y_lane_inserted,overwrite);
            }
        }
        expect_gather_refused(modern,"A = v5.xyzw;","A = v4.xyzw;");
        expect_gather_refused(modern,"int a0 = 0;","int a0 = 0;\n\ta0 = 1;");
        }
    }
    recognized=mac_bone_gather_rewrite(modern,&gather_plan,&gathered,error,sizeof(error));
    printf("bone gather inspect: shader=%u instructions=%lu recognized=%d address_attr=%u address_const=%u live_varyings=%u\n",
        shader_index,instructions,recognized,gather_plan.address_attribute,
        gather_plan.address_constant,(shader_index==31||shader_index==50)?
            (varying_mode==2?2U:varying_output_count(modern)-(varying_mode==0?1U:0U)):2U);
    CHECK(recognized==1&&gather_plan.version==1&&gather_plan.first_gather_attribute==10&&
        gather_plan.gather_attribute_count==6&&gather_plan.address_constant==7&&
        gather_plan.row_base==60&&gather_plan.row_limit==191);
    if(shader_index==9||shader_index==30)CHECK(gather_plan.address_attribute==5);
    if(recognized!=1||!gathered)goto done;
    if(rsq_variant){unsigned replacements=0;const char *from="inversesqrt(abs(C.x))";
        const char *to=rsq_variant==1?"(1.0 / sqrt(abs(C.x)))":"exp2(-0.5 * log2(abs(C.x)))";
        algebraic=replace_text_all(modern,from,to,&replacements);
        CHECK(algebraic!=NULL&&replacements==3);if(!algebraic||replacements!=3)goto done;
        printf("diagnostic RSQ rewrite: shader=%u variant=%s replacements=%u\n",shader_index,
            rsq_variant==1?"reciprocal-sqrt":"exp-log",replacements);
    }
    if(shader_index==31||shader_index==50){pixel450_live=live_varying_pixel_source(modern,varying_mode);CHECK(pixel450_live!=NULL);if(!pixel450_live)goto done;pixel450=pixel450_live;}
    CHECK(mac_glsl120_vertex(modern,&vertex_original,&mask,error,sizeof(error)));
    CHECK(mac_glsl120_vertex(rsq_variant?algebraic:gathered,&vertex_gather,&mask,error,sizeof(error)));
    CHECK(mac_glsl120_pixel(pixel450,&pixel120,error,sizeof(error)));
    if(!vertex_original||!vertex_gather||!pixel120)goto done;
    shaders[0]=compile_shader(GL_VERTEX_SHADER,vertex_original);
    shaders[1]=compile_shader(GL_VERTEX_SHADER,vertex_gather);fs=compile_shader(GL_FRAGMENT_SHADER,pixel120);
    CHECK(shaders[0]&&shaders[1]&&fs);if(!shaders[0]||!shaders[1]||!fs)goto done;
    for(i=0;i<2;++i){unsigned a;programs[i]=glCreateProgram();glAttachShader(programs[i],shaders[i]);glAttachShader(programs[i],fs);
        for(a=0;a<16;++a){char name[24];sprintf(name,"v%u_in",a);glBindAttribLocation(programs[i],a,name);}
        glLinkProgram(programs[i]);glGetProgramiv(programs[i],GL_LINK_STATUS,&link);CHECK(link==GL_TRUE);if(link!=GL_TRUE)goto done;
    }
    memset(constants,0,sizeof(constants));memset(attributes,0,sizeof(attributes));
    constants[0][0]=constants[1][1]=constants[2][2]=constants[3][3]=1.0f;
    constants[7][3]=1.0f;constants[13][0]=1.0f;constants[14][1]=1.0f;
    constants[28][0]=1.0f;constants[29][1]=1.0f;constants[30][2]=1.0f;constants[31][3]=1.0f;
    constants[58][0]=16.0f;constants[58][1]=-16.0f;constants[58][2]=1.0f;
    constants[59][0]=16.0f;constants[59][1]=16.0f;
    constants[60][0]=1.0f;constants[61][1]=1.0f;constants[62][2]=1.0f;
    {
        float guarded[16*4],before[16*4],saved_scale=constants[7][3];
        memset(guarded,0,sizeof(guarded));guarded[5*4]=2.0f;guarded[5*4+1]=0.0f;
        memcpy(before,guarded,sizeof(guarded));constants[7][3]=FLT_MAX;
        CHECK(mac_bone_gather_apply(&gather_plan,guarded,1,16,
            (const float (*)[4])constants,error,sizeof(error))==0);
        CHECK(!memcmp(guarded,before,sizeof(guarded)));
        constants[7][3]=saved_scale;
    }
    for(i=0;i<3;++i){unsigned a;attributes[i][0][0]=i==0?-0.5f:(i==1?0.5f:0.0f);
        attributes[i][0][1]=i==2?0.5f:-0.5f;attributes[i][0][2]=0.5f;attributes[i][0][3]=1.0f;
        attributes[i][1][2]=1.0f;attributes[i][1][3]=0.0f;attributes[i][4][2]=1.0f;attributes[i][4][3]=1.0f;
        attributes[i][5][3]=1.0f;attributes[i][6][0]=1.0f;attributes[i][6][3]=1.0f;
        for(a=0;a<16;++a)if(a!=1&&a!=4&&a!=5&&a!=6)attributes[i][a][3]=1.0f;
    }
    for(i=0;i<4;++i){unsigned vertex;
        for(vertex=0;vertex<3;++vertex){
            attributes[vertex][gather_plan.address_attribute][0]=address_pairs[i][0];
            attributes[vertex][gather_plan.address_attribute][1]=address_pairs[i][1];}
        CHECK(mac_bone_gather_apply(&gather_plan,&attributes[0][0][0],3,16,
            (const float (*)[4])constants,error,sizeof(error))==1);
        for(vertex=0;vertex<2;++vertex){unsigned a;const char *status_name;
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);glBindTexture(GL_TEXTURE_2D,texture);
            glViewport(0,0,32,32);glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
            glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glUseProgram(programs[vertex]);
            location=glGetUniformLocation(programs[vertex],"c");if(location>=0)glUniform4fv(location,192,&constants[0][0]);
            location=glGetUniformLocation(programs[vertex],"viewport_scale");if(location>=0)glUniform4f(location,16,-16,1,0);
            location=glGetUniformLocation(programs[vertex],"viewport_offset");if(location>=0)glUniform4f(location,16,16,0,0);
            location=glGetUniformLocation(programs[vertex],"screen_offset");if(location>=0)glUniform1f(location,0);
            location=glGetUniformLocation(programs[vertex],"point_size");if(location>=0)glUniform1f(location,1);
            for(a=0;a<16;++a){glEnableVertexAttribArray(a);glVertexAttribPointer(a,4,GL_FLOAT,GL_FALSE,sizeof(attributes[0]),&attributes[0][a][0]);}
            status_name=vertex?(rsq_variant?(rsq_variant==1?"rsq-reciprocal-sqrt":"rsq-exp-log"):"cpu-gather"):"original";
            glDrawArrays(GL_TRIANGLES,0,3);gpu_status(context,status_name,"first-draw");
            glFinish();glReadPixels(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,vertex?candidate:reference);
            gl_error=glGetError();CHECK(gl_error==GL_NO_ERROR);if(gl_error!=GL_NO_ERROR)ok=0;
            glDrawArrays(GL_TRIANGLES,0,3);gpu_status(context,status_name,"warm-draw");
            glFinish();glReadPixels(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,warm);
            gl_error=glGetError();CHECK(gl_error==GL_NO_ERROR);if(gl_error!=GL_NO_ERROR)ok=0;
            CHECK(!memcmp(warm,vertex?candidate:reference,sizeof(warm)));
            if(vertex==1){unsigned pixel,covered=0,max_delta=0;int exact;
                exact=!memcmp(reference,candidate,sizeof(reference));
                if(!rsq_variant)CHECK(exact);
                for(pixel=0;pixel<32*32;++pixel){unsigned channel;
                    if(reference[pixel*4]||reference[pixel*4+1]||reference[pixel*4+2]||reference[pixel*4+3])++covered;
                    if(candidate[pixel*4]||candidate[pixel*4+1]||candidate[pixel*4+2]||candidate[pixel*4+3])++covered;
                    for(channel=0;channel<4;++channel){unsigned x=reference[pixel*4+channel],y=candidate[pixel*4+channel];
                        unsigned delta=x>y?x-y:y-x;if(delta>max_delta)max_delta=delta;}
                }
                if(covered)covered_any=1;
                printf("%s A/B: shader=%u varying_mode=%d rsq_variant=%d address_pair=%.4g,%.4g covered_pixels=%u whole_frame_equal=%d max_channel_delta=%u\n",
                    rsq_variant?"RSQ diagnostic":"CPU gather",
                    shader_index,varying_mode,rsq_variant,address_pairs[i][0],address_pairs[i][1],covered,exact,max_delta);}
        }
    }
    CHECK(covered_any);
done:
    if(programs[0]||programs[1])glUseProgram(0);
    for(i=0;i<2;++i){if(programs[i])glDeleteProgram(programs[i]);if(shaders[i])glDeleteShader(shaders[i]);}
    if(fs)glDeleteShader(fs);free(modern);free(gathered);free(algebraic);free(vertex_original);free(vertex_gather);free(pixel120);free(pixel450_live);
    return ok;
}

static GLuint make_program(unsigned case_index,unsigned long *attribute_mask)
{
    char modern[2048],body[512],error[256];char *vertex120=NULL,*pixel120=NULL;
    const char *pixel450="#version 450 core\nin vec4 xD0;\nlayout(location = 0) out vec4 fragment_color;\nvoid main()\n{\n fragment_color = clamp(xD0, 0.0, 1.0);\n}\n";
    GLuint vertex_shader,fragment_shader,program=0;GLint linked=GL_FALSE;unsigned long mask=0,i;
    const char *decls="#version 450 core\nuniform vec4 c[192];\nlayout(location = 0) in vec4 v0_in;\nlayout(location = 1) in vec4 v1_in;\nout vec4 xD0;\ninvariant gl_Position;\nvoid main()\n{\n gl_Position = v0_in;\n";
    sprintf(body,"%s\n}\n",cases[case_index].expression);
    if(strlen(decls)+strlen(body)>=sizeof(modern))return 0;
    strcpy(modern,decls);strcat(modern,body);
    if(!mac_glsl120_vertex(modern,&vertex120,&mask,error,sizeof(error))){fprintf(stderr,"translate %s: %s\n",cases[case_index].name,error);return 0;}
    if(!mac_glsl120_pixel(pixel450,&pixel120,error,sizeof(error))){fprintf(stderr,"pixel translate: %s\n",error);free(vertex120);return 0;}
    CHECK(mask==3UL);
    vertex_shader=compile_shader(GL_VERTEX_SHADER,vertex120);fragment_shader=compile_shader(GL_FRAGMENT_SHADER,pixel120);
    if(!vertex_shader||!fragment_shader){free(vertex120);free(pixel120);if(vertex_shader)glDeleteShader(vertex_shader);if(fragment_shader)glDeleteShader(fragment_shader);return 0;}
    program=glCreateProgram();glAttachShader(program,vertex_shader);glAttachShader(program,fragment_shader);
    for(i=0;i<2;++i){char name[24];sprintf(name,"v%lu_in",i);glBindAttribLocation(program,(GLuint)i,name);}
    glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&linked);
    if(linked!=GL_TRUE){char log[1024];GLint written=0;glGetProgramInfoLog(program,sizeof(log),&written,log);if(written<0)written=0;if((size_t)written>=sizeof(log))written=sizeof(log)-1;log[written]='\0';fprintf(stderr,"GLSL link failed (%s): %s\n",cases[case_index].name,log);glDeleteProgram(program);program=0;}
    glDeleteShader(vertex_shader);glDeleteShader(fragment_shader);free(vertex120);free(pixel120);
    if(program)*attribute_mask=mask;
    return program;
}

static void gpu_status(CGLContextObj context,const char *name,const char *phase)
{
    GLint vertex=-1,fragment=-1;CGLError ve,fe;
    ve=CGLGetParameter(context,kCGLCPGPUVertexProcessing,&vertex);
    fe=CGLGetParameter(context,kCGLCPGPUFragmentProcessing,&fragment);
    CHECK(ve==kCGLNoError&&fe==kCGLNoError);
    printf("relative shader status: case=%s phase=%s vertex_gpu=%d vertex_query=%d fragment_gpu=%d fragment_query=%d\n",
        name,phase,(int)vertex,(int)ve,(int)fragment,(int)fe);
}

static void read_case(GLuint program,unsigned case_index,float address,
    GLuint fbo,GLuint texture,GLint constants_location,
    unsigned char first_pixel[4],unsigned char warm_pixel[4],CGLContextObj context)
{
    GLfloat vertices[3][8]={
        {-0.8f,-0.8f,0.0f,1.0f,0.0f,0.0f,0.0f,1.0f},
        { 0.8f,-0.8f,0.0f,1.0f,0.0f,0.0f,0.0f,1.0f},
        { 0.0f, 0.8f,0.0f,1.0f,0.0f,0.0f,0.0f,1.0f}
    };
    GLfloat constants[192][4];GLenum error;
    vertices[0][4]=vertices[1][4]=vertices[2][4]=address;
    printf("relative input: case=%s address=%.7g\n",cases[case_index].name,address);
    memset(constants,0,sizeof(constants));
    constants[0][0]=1.0f;constants[0][3]=1.0f;
    constants[5][1]=1.0f;constants[5][3]=1.0f;
    constants[191][2]=1.0f;constants[191][3]=1.0f;
    constants[60][1]=1.0f;constants[60][3]=1.0f;
    constants[190][0]=1.0f;constants[190][1]=1.0f;constants[190][3]=1.0f;
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);glBindTexture(GL_TEXTURE_2D,texture);
    glViewport(0,0,32,32);glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
    glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glUseProgram(program);
    glUniform4fv(constants_location,192,&constants[0][0]);
    glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
    glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,8*sizeof(float),&vertices[0][0]);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,8*sizeof(float),&vertices[0][4]);
    glDrawArrays(GL_TRIANGLES,0,3);
    gpu_status(context,cases[case_index].name,"first-draw");
    glFinish();
    glReadPixels(16,12,1,1,GL_RGBA,GL_UNSIGNED_BYTE,first_pixel);
    error=glGetError();CHECK(error==GL_NO_ERROR);
    glDrawArrays(GL_TRIANGLES,0,3);
    gpu_status(context,cases[case_index].name,"warm-draw");
    glFinish();
    glReadPixels(16,12,1,1,GL_RGBA,GL_UNSIGNED_BYTE,warm_pixel);
    error=glGetError();CHECK(error==GL_NO_ERROR);
}

static int test_retail_shader39_variant(CGLContextObj context,GLuint fbo,GLuint texture,unsigned variant)
{
    const struct vertex_shader_entry *entry=&vertex_shader_table[39];
    const DWORD *function=(const DWORD *)entry->code;
    unsigned long instruction_count=function[0]>>16,declared_mask=0,i;
    static const char *variant_names[]={"translator-float-offset","old-int-add-then-float","float-arl-storage","float-arl-pruned-inputs"};
    char error[256],*modern=NULL,*variant_modern=NULL,*float_setup=NULL,*pruned=NULL,*vertex120=NULL,*pixel120=NULL;
    const char *pixel450="#version 450 core\nin vec4 xD0;\nin vec4 xT0;\nlayout(location = 0) out vec4 fragment_color;\nvoid main()\n{\n fragment_color = clamp(xD0 + xT0, 0.0, 1.0);\n}\n";
    GLuint vertex_shader=0,fragment_shader=0,program=0;GLint linked=GL_FALSE;
    float constants[192][4],attributes[3][16][4];
    unsigned long active=0;unsigned vertex;unsigned char first[4]={0},warm[4]={0};
    GLint location;GLenum gl_error;
    CHECK(instruction_count==21);
    modern=nv2a_vertex_shader_to_glsl(function+1,instruction_count,0,NULL);
    CHECK(modern!=NULL);if(!modern)return 0;
    if(variant==1)variant_modern=old_relative_source(modern);
    else if(variant==2||variant==3){unsigned declaration_count=0,arl_count=0;
        float_setup=replace_text_all(modern,"int a0 = 0;","float a0 = 0.0;",&declaration_count);
        if(float_setup)variant_modern=replace_text_all(float_setup,"a0 = int(floor(mac.x + 0.001));","a0 = floor(mac.x + 0.001);",&arl_count);
        CHECK(declaration_count==1&&arl_count==2);
    }
    else variant_modern=modern;
    CHECK(variant_modern!=NULL);if(!variant_modern)goto done;
    CHECK(mac_glsl120_vertex(variant_modern,&vertex120,&declared_mask,error,sizeof(error)));
    if(!vertex120){fprintf(stderr,"shader39 vertex translation: %s\n",error);free(modern);return 0;}
    if(variant==3){unsigned removed=0;pruned=prune_unused_inputs(vertex120,&removed);
        CHECK(pruned!=NULL&&removed>0);
        if(pruned){free(vertex120);vertex120=pruned;pruned=NULL;}
        printf("retail shader 39 pruned unused input copies=%u\n",removed);
    }
    CHECK(mac_glsl120_pixel(pixel450,&pixel120,error,sizeof(error)));
    if(!pixel120){fprintf(stderr,"shader39 pixel translation: %s\n",error);free(modern);free(vertex120);return 0;}
    vertex_shader=compile_shader(GL_VERTEX_SHADER,vertex120);
    fragment_shader=compile_shader(GL_FRAGMENT_SHADER,pixel120);
    CHECK(vertex_shader!=0&&fragment_shader!=0);
    if(!vertex_shader||!fragment_shader)goto done;
    program=glCreateProgram();glAttachShader(program,vertex_shader);glAttachShader(program,fragment_shader);
    for(i=0;i<16;++i){char name[24];sprintf(name,"v%lu_in",i);glBindAttribLocation(program,(GLuint)i,name);}
    glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&linked);
    if(linked!=GL_TRUE){char log[2048];GLint written=0;glGetProgramInfoLog(program,sizeof(log),&written,log);if(written<0)written=0;if((size_t)written>=sizeof(log))written=sizeof(log)-1;log[written]='\0';fprintf(stderr,"shader39 program link: %s\n",log);}
    CHECK(linked==GL_TRUE);if(linked!=GL_TRUE)goto done;
    for(i=0;i<16;++i){char name[24];sprintf(name,"v%lu_in",i);if(glGetAttribLocation(program,name)>=0)active|=1UL<<i;}
    printf("retail shader 39 active attributes=0x%04lx declared=0x%04lx\n",active,declared_mask);
    CHECK((active&~declared_mask)==0);
    CHECK((active&((1UL<<0)|(1UL<<4)|(1UL<<5)|(1UL<<6)))==((1UL<<0)|(1UL<<4)|(1UL<<5)|(1UL<<6)));
    memset(constants,0,sizeof(constants));memset(attributes,0,sizeof(attributes));
    constants[7][3]=1.0f;
    constants[13][0]=1.0f;constants[14][1]=1.0f;
    constants[28][0]=1.0f;constants[29][1]=1.0f;constants[30][2]=1.0f;constants[31][3]=1.0f;
    constants[58][0]=16.0f;constants[58][1]=-16.0f;constants[58][2]=1.0f;
    constants[59][0]=16.0f;constants[59][1]=16.0f;
    constants[60][0]=1.0f;constants[61][1]=1.0f;constants[62][2]=1.0f;
    attributes[0][0][0]=-0.5f;attributes[0][0][1]=-0.5f;attributes[0][0][2]=0.5f;attributes[0][0][3]=1.0f;
    attributes[1][0][0]=0.5f;attributes[1][0][1]=-0.5f;attributes[1][0][2]=0.5f;attributes[1][0][3]=1.0f;
    attributes[2][0][0]=0.0f;attributes[2][0][1]=0.5f;attributes[2][0][2]=0.5f;attributes[2][0][3]=1.0f;
    for(vertex=0;vertex<3;++vertex){
        attributes[vertex][4][0]=0.25f;attributes[vertex][4][1]=0.5f;attributes[vertex][4][3]=1.0f;
        attributes[vertex][5][3]=1.0f;
        attributes[vertex][6][0]=0.5f;attributes[vertex][6][3]=1.0f;
        for(i=1;i<16;++i)if(i!=4&&i!=5&&i!=6)attributes[vertex][i][3]=1.0f;
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);glBindTexture(GL_TEXTURE_2D,texture);
    glViewport(0,0,32,32);glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
    glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glUseProgram(program);
    location=glGetUniformLocation(program,"c");CHECK(location>=0);if(location>=0)glUniform4fv(location,192,&constants[0][0]);
    location=glGetUniformLocation(program,"viewport_scale");if(location>=0)glUniform4f(location,16.0f,-16.0f,1.0f,0.0f);
    location=glGetUniformLocation(program,"viewport_offset");if(location>=0)glUniform4f(location,16.0f,16.0f,0.0f,0.0f);
    location=glGetUniformLocation(program,"screen_offset");if(location>=0)glUniform1f(location,0.0f);
    location=glGetUniformLocation(program,"point_size");if(location>=0)glUniform1f(location,1.0f);
    for(i=0;i<16;++i){glEnableVertexAttribArray((GLuint)i);glVertexAttribPointer((GLuint)i,4,GL_FLOAT,GL_FALSE,sizeof(attributes[0]),&attributes[0][i][0]);}
    glDrawArrays(GL_TRIANGLES,0,3);gpu_status(context,variant_names[variant],"retail-shader-39-first");
    glFinish();glReadPixels(16,12,1,1,GL_RGBA,GL_UNSIGNED_BYTE,first);gl_error=glGetError();CHECK(gl_error==GL_NO_ERROR);
    glDrawArrays(GL_TRIANGLES,0,3);gpu_status(context,variant_names[variant],"retail-shader-39-warm");
    glFinish();glReadPixels(16,12,1,1,GL_RGBA,GL_UNSIGNED_BYTE,warm);gl_error=glGetError();CHECK(gl_error==GL_NO_ERROR);
    printf("retail shader 39 variant=%s pixel first=%u,%u,%u,%u warm=%u,%u,%u,%u\n",variant_names[variant],first[0],first[1],first[2],first[3],warm[0],warm[1],warm[2],warm[3]);
    CHECK(first[0]>=63&&first[0]<=65&&first[1]>=127&&first[1]<=129&&first[2]<=1&&first[3]>=254);
    CHECK(!memcmp(first,warm,sizeof(first)));
done:
    if(program){glUseProgram(0);glDeleteProgram(program);}
    if(vertex_shader)glDeleteShader(vertex_shader);if(fragment_shader)glDeleteShader(fragment_shader);
    if(variant_modern!=modern)free(variant_modern);free(float_setup);free(pruned);
    free(modern);free(vertex120);free(pixel120);
    return linked==GL_TRUE;
}

static void test_fallback_shader_variant(CGLContextObj context,GLuint fbo,GLuint texture,
    unsigned shader_index,int old_integer_address)
{
    const struct vertex_shader_entry *entry=&vertex_shader_table[shader_index];
    const DWORD *function=(const DWORD *)entry->code;
    unsigned long instruction_count=function[0]>>16,mask=0,i;
    char *modern=NULL,*variant_source=NULL,*vertex120=NULL,*pixel120=NULL,error[256];
    const char *pixel450="#version 450 core\nin vec4 xD0;\nin vec4 xT0;\nlayout(location = 0) out vec4 fragment_color;\nvoid main()\n{\n fragment_color = clamp(xD0 + xT0, 0.0, 1.0);\n}\n";
    GLuint vs=0,fs=0,program=0;GLint linked=GL_FALSE,location;
    GLfloat constants[192][4],attributes[3][16][4];
    unsigned char pixel[4]={0};GLenum gl_error;
    modern=nv2a_vertex_shader_to_glsl(function+1,instruction_count,0,NULL);
    CHECK(modern!=NULL);if(!modern)return;
    if(old_integer_address){
        variant_source=old_relative_source(modern);
        if(variant_source)strcat(variant_source,"\n// diagnostic keeps a0 integer\n");
    }
    else variant_source=modern;
    CHECK(variant_source!=NULL);if(!variant_source)goto cleanup;
    CHECK(mac_glsl120_vertex(variant_source,&vertex120,&mask,error,sizeof(error)));
    CHECK(mac_glsl120_pixel(pixel450,&pixel120,error,sizeof(error)));
    if(!vertex120||!pixel120)goto cleanup;
    vs=compile_shader(GL_VERTEX_SHADER,vertex120);fs=compile_shader(GL_FRAGMENT_SHADER,pixel120);
    CHECK(vs&&fs);if(!vs||!fs)goto cleanup;
    program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);
    for(i=0;i<16;++i){char name[24];sprintf(name,"v%lu_in",i);glBindAttribLocation(program,(GLuint)i,name);}
    glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&linked);
    CHECK(linked==GL_TRUE);if(linked!=GL_TRUE)goto cleanup;
    memset(constants,0,sizeof(constants));memset(attributes,0,sizeof(attributes));
    constants[0][0]=constants[1][1]=constants[2][2]=constants[3][3]=1.0f;
    constants[7][3]=1.0f;constants[13][0]=1.0f;constants[14][1]=1.0f;
    constants[28][0]=1.0f;constants[29][1]=1.0f;constants[30][2]=1.0f;constants[31][3]=1.0f;
    constants[58][0]=16.0f;constants[58][1]=-16.0f;constants[58][2]=1.0f;
    constants[59][0]=16.0f;constants[59][1]=16.0f;
    constants[60][0]=1.0f;constants[61][1]=1.0f;constants[62][2]=1.0f;
    for(i=0;i<3;++i){
        attributes[i][0][0]=i==0?-0.5f:(i==1?0.5f:0.0f);
        attributes[i][0][1]=i==2?0.5f:-0.5f;attributes[i][0][2]=0.5f;attributes[i][0][3]=1.0f;
        attributes[i][1][2]=1.0f;attributes[i][1][3]=0.0f;
        attributes[i][4][2]=1.0f;attributes[i][4][3]=1.0f;
        attributes[i][5][3]=1.0f;attributes[i][6][0]=1.0f;attributes[i][6][3]=1.0f;
        for(location=0;location<16;++location)if(location!=1&&location!=4&&location!=5&&location!=6)
            attributes[i][location][3]=1.0f;
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);glBindTexture(GL_TEXTURE_2D,texture);
    glViewport(0,0,32,32);glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
    glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glUseProgram(program);
    location=glGetUniformLocation(program,"c");CHECK(location>=0);if(location>=0)glUniform4fv(location,192,&constants[0][0]);
    location=glGetUniformLocation(program,"viewport_scale");if(location>=0)glUniform4f(location,16.0f,-16.0f,1.0f,0.0f);
    location=glGetUniformLocation(program,"viewport_offset");if(location>=0)glUniform4f(location,16.0f,16.0f,0.0f,0.0f);
    location=glGetUniformLocation(program,"screen_offset");if(location>=0)glUniform1f(location,0.0f);
    location=glGetUniformLocation(program,"point_size");if(location>=0)glUniform1f(location,1.0f);
    for(i=0;i<16;++i){glEnableVertexAttribArray((GLuint)i);glVertexAttribPointer((GLuint)i,4,GL_FLOAT,GL_FALSE,sizeof(attributes[0]),&attributes[0][i][0]);}
    glDrawArrays(GL_TRIANGLES,0,3);
    gpu_status(context,"fallback-vs","first-draw");
    glFinish();glReadPixels(16,12,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);gl_error=glGetError();CHECK(gl_error==GL_NO_ERROR);
    printf("fallback shader A/B: index=%u instructions=%lu relative_reads=%u address=%s pixel=%u,%u,%u,%u\n",
        shader_index,instruction_count,substring_count(modern,"c[clamp(a0 + "),
        old_integer_address?"old-int":"production-float",pixel[0],pixel[1],pixel[2],pixel[3]);
    glDrawArrays(GL_TRIANGLES,0,3);gpu_status(context,"fallback-vs","warm-draw");
cleanup:
    if(program){glUseProgram(0);glDeleteProgram(program);}
    if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);
    if(variant_source!=modern)free(variant_source);free(modern);free(vertex120);free(pixel120);
}

int main(void)
{
    CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAColorSize,
        (CGLPixelFormatAttribute)24,0};
    CGLPixelFormatObj format=NULL;CGLContextObj context=NULL;
    GLint formats=0;GLuint fbo=0,texture=0;GLenum status;unsigned ci,ai,baseline[10][4];
    unsigned long mask=0;GLuint program;int all_ok=1;
    CHECK(CGLChoosePixelFormat(attrs,&format,&formats)==kCGLNoError&&format);if(!format)goto done;
    CHECK(CGLCreateContext(format,NULL,&context)==kCGLNoError&&context);if(!context)goto done;
    CHECK(CGLSetCurrentContext(context)==kCGLNoError);
    glGenFramebuffersEXT(1,&fbo);glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);
    glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,32,32,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,texture,0);
    status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);CHECK(status==GL_FRAMEBUFFER_COMPLETE_EXT);
    for(ci=0;ci<sizeof(cases)/sizeof(cases[0]);++ci) {
        GLint address_location,constants_location;unsigned long active=0;
        program=make_program(ci,&mask);CHECK(program!=0);if(!program){all_ok=0;if(ci==0)goto done;continue;}
        for(ai=0;ai<2;++ai){char name[24];sprintf(name,"v%u_in",ai);if(glGetAttribLocation(program,name)>=0)active|=1UL<<ai;}
        CHECK(active==(cases[ci].fixed?1UL:mask));
        address_location=glGetUniformLocation(program,"address");
        constants_location=glGetUniformLocation(program,"c");
        CHECK(constants_location>=0);
        CHECK(address_location<0); /* Index comes from the per-vertex node/address attribute. */
        for(ai=0;ai<sizeof(addresses)/sizeof(addresses[0]);++ai) {
            unsigned selected=cases[ci].fixed?5:expected_index[ai];
            unsigned char first[4],warm[4],expected[4]={0,0,0,255};unsigned k;
            if(selected==0)expected[0]=255;
            else if(selected==60)expected[1]=255;
            else if(selected==190){expected[0]=255;expected[1]=255;}
            else if(selected==191)expected[2]=255;
            else if(selected==5)expected[1]=255;
            read_case(program,ci,addresses[ai],fbo,texture,constants_location,first,warm,context);
            for(k=0;k<4;++k){
                CHECK(first[k]==expected[k]);CHECK(warm[k]==expected[k]);
                if(ci==0)baseline[ai][k]=first[k];
                else if(!cases[ci].fixed) CHECK(baseline[ai][k]==first[k]);
                CHECK(warm[k]==first[k]);
            }
        }
        glUseProgram(0);glDeleteProgram(program);
    }
    for(ci=0;ci<4;++ci)if(!test_retail_shader39_variant(context,fbo,texture,ci))all_ok=0;
    test_fallback_shader_variant(context,fbo,texture,9,0);
    test_fallback_shader_variant(context,fbo,texture,9,1);
    test_fallback_shader_variant(context,fbo,texture,47,0);
    test_fallback_shader_variant(context,fbo,texture,47,1);
    if(!test_cpu_gather_shader(context,fbo,texture,9,1,0))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,30,1,0))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,31,1,0))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,31,0,0))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,31,2,0))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,31,1,1))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,31,1,2))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,50,1,0))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,50,0,0))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,50,2,0))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,50,1,1))all_ok=0;
    if(!test_cpu_gather_shader(context,fbo,texture,50,1,2))all_ok=0;
done:
    if(context){CGLSetCurrentContext(context);if(fbo)glDeleteFramebuffersEXT(1,&fbo);if(texture)glDeleteTextures(1,&texture);CGLSetCurrentContext(NULL);CGLDestroyContext(context);}
    if(format)CGLDestroyPixelFormat(format);
    printf("bounded relative-constant GPU probe: %u checks, %u failures\n",checks,failures);
    return all_ok&&failures==0?0:1;
}
