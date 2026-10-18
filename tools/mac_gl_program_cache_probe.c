#include "../port/mac/mac_gl_program_cache.h"
#include <OpenGL/OpenGL.h>
#include <stdio.h>
#include <string.h>

static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); } } while (0)

static const char vertex_template[] =
    "#version 120\nattribute vec4 v%u_in;\nvoid main(){gl_Position=v%u_in;}\n";
static const char fragment_source[] =
    "#version 120\nuniform vec4 tint;\nvoid main(){gl_FragColor=tint;}\n";
static const char fragment_source_alt[] =
    "#version 120\nvoid main(){gl_FragColor=vec4(0.75,0.5,0.25,1.0);}\n";

static int get_program(struct mac_gl_program_cache *cache, const char *vertex,
    unsigned long mask, GLuint *program)
{
    char error[1024];
    if (!mac_gl_program_cache_get(cache, vertex, fragment_source, mask,
            program, error, sizeof(error))) {
        fprintf(stderr,"program cache: %s\n",error);
        return 0;
    }
    return 1;
}

int main(void)
{
    CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAColorSize,(CGLPixelFormatAttribute)24,0};
    CGLPixelFormatObj format=NULL;CGLContextObj context=NULL;GLint formats=0,location;
    struct mac_gl_program_cache cache;struct mac_gl_program_cache_stats stats;
    char source[256],error[1024];GLuint first=0,again=0,second=0,program=0,newest=0;
    GLuint old_programs[MAC_GL_PROGRAM_CACHE_CAPACITY];unsigned i;
    GLuint recycled_candidate=0,identity_program=0,reloaded_identity_program=0;
    struct mac_shader_source_identity eviction_identity={19,23,29};
    unsigned long misses_before_reload;
    unsigned long active_mask = 0;

    CHECK(CGLChoosePixelFormat(attrs,&format,&formats)==kCGLNoError&&format);
    if(!format)goto done;
    CHECK(CGLCreateContext(format,NULL,&context)==kCGLNoError&&context);
    if(!context)goto done;
    CHECK(CGLSetCurrentContext(context)==kCGLNoError);
    memset(old_programs,0,sizeof(old_programs));
    mac_gl_program_cache_init(&cache);
    sprintf(source,vertex_template,0,0);
    CHECK(get_program(&cache,source,1,&first));
    CHECK(first&&glIsProgram(first));
    location=mac_gl_program_cache_uniform_location(&cache,first,"tint");
    CHECK(location>=0&&glGetUniformLocation(first,"tint")==location);
    CHECK(mac_gl_program_cache_uniform_location(&cache,first,"not_present")==-1);
    CHECK(mac_gl_program_cache_uniform_location(&cache,first,"not_present")==-1);
    mac_gl_program_cache_get_stats(&cache,&stats);
    CHECK(stats.uniform_lookups==2&&stats.uniform_hits==1);
    location=glGetAttribLocation(first,"v0_in");CHECK(location==0);
    CHECK(mac_gl_program_cache_active_attributes(&cache,first,&active_mask)&&active_mask==1);
    active_mask=123;
    CHECK(!mac_gl_program_cache_active_attributes(&cache,0,&active_mask)&&active_mask==0);
    CHECK(get_program(&cache,source,1,&again));CHECK(again==first);
    CHECK(mac_gl_program_cache_get(&cache,source,fragment_source_alt,1,
        &second,error,sizeof(error)));CHECK(second!=first);
    CHECK(get_program(&cache,source,1,&program));CHECK(program==first);
    CHECK(get_program(&cache,source,2,&program));CHECK(program!=first);
    sprintf(source,vertex_template,1,1);
    CHECK(get_program(&cache,source,2,&program));CHECK(program!=first);
    location=glGetAttribLocation(program,"v1_in");CHECK(location==1);
    CHECK(mac_gl_program_cache_active_attributes(&cache,program,&active_mask)&&active_mask==2);
    mac_gl_program_cache_get_stats(&cache,&stats);
    CHECK(stats.entries==4&&stats.hits==2&&stats.misses==4&&stats.evictions==0);
    {
        struct mac_shader_source_identity identity_a={7,11,13};
        struct mac_shader_source_identity identity_b={7,11,14};
        struct mac_shader_source_identity partial_identity={0,11,14};
        struct mac_shader_source_identity zero_identity={0,0,0};
        struct mac_shader_source_identity identity_c={7,11,15};
        struct mac_shader_source_identity aliases[10];
        char alias_vertex[10][256], alias_fragment[10][256];
        char stale_vertex[256];
        GLuint identified=0, before_overflow=0;
        unsigned long before_alternating=0, after_alternating=0;
        unsigned long before_pointer_alias=0;
        unsigned long j;
        CHECK(mac_gl_program_cache_get_identified(&cache,source,fragment_source,2,
            &identity_a,&identified,error,sizeof(error))&&identified==program);
        CHECK(mac_gl_program_cache_get_identified(&cache,source,fragment_source,2,
            &identity_a,&identified,error,sizeof(error))&&identified==program);
        /* A distinct issuer token is attached only after exact source comparison. */
        CHECK(mac_gl_program_cache_get_identified(&cache,source,fragment_source,2,
            &identity_b,&identified,error,sizeof(error))&&identified==program);
        CHECK(mac_gl_program_cache_get_identified(&cache,source,fragment_source,2,
            &identity_b,&identified,error,sizeof(error))&&identified==program);
        /* Alternating immutable source identities should retain both aliases. */
        mac_gl_program_cache_get_stats(&cache,&stats);
        before_alternating=stats.identity_hits;
        CHECK(mac_gl_program_cache_get_identified(&cache,source,fragment_source,2,
            &identity_a,&identified,error,sizeof(error))&&identified==program);
        CHECK(mac_gl_program_cache_get_identified(&cache,source,fragment_source,2,
            &identity_b,&identified,error,sizeof(error))&&identified==program);
        mac_gl_program_cache_get_stats(&cache,&stats);
        after_alternating=stats.identity_hits;
        CHECK(after_alternating==before_alternating+2);
        /* A different pointer pair with identical text is adopted only after
           the normal content comparison, then its exact alias is reusable. */
        strcpy(alias_vertex[0],source);strcpy(alias_fragment[0],fragment_source);
        CHECK(alias_vertex[0]!=source&&alias_fragment[0]!=fragment_source);
        mac_gl_program_cache_get_stats(&cache,&stats);
        before_pointer_alias=stats.identity_hits;
        CHECK(mac_gl_program_cache_get_identified(&cache,alias_vertex[0],
            alias_fragment[0],2,&identity_c,&identified,error,sizeof(error))&&
            identified==program);
        mac_gl_program_cache_get_stats(&cache,&stats);
        CHECK(stats.identity_hits==before_pointer_alias);
        CHECK(mac_gl_program_cache_get_identified(&cache,alias_vertex[0],
            alias_fragment[0],2,&identity_c,&identified,error,sizeof(error))&&
            identified==program);
        mac_gl_program_cache_get_stats(&cache,&stats);
        CHECK(stats.identity_hits==before_pointer_alias+1);
        /* Tokens cannot select a program for different source addresses/text. */
        strcpy(stale_vertex,source);
        strcat(stale_vertex,"\n/* stale alias must not match */\n");
        CHECK(mac_gl_program_cache_get_identified(&cache,stale_vertex,
            fragment_source,2,&identity_a,&identified,error,sizeof(error))&&
            identified!=program);
        /* Keep more identities than the fixed alias ring. The oldest alias
           must fall back to full content comparison after eviction. */
        for(j=0;j<10;++j) {
            aliases[j].cache_epoch=9;
            aliases[j].vertex_entry=100+j;
            aliases[j].pixel_entry=200+j;
            strcpy(alias_vertex[j],source);
            strcpy(alias_fragment[j],fragment_source);
            CHECK(mac_gl_program_cache_get_identified(&cache,alias_vertex[j],
                alias_fragment[j],2,&aliases[j],&identified,error,sizeof(error))&&
                identified==program);
            CHECK(mac_gl_program_cache_get_identified(&cache,alias_vertex[j],
                alias_fragment[j],2,&aliases[j],&identified,error,sizeof(error))&&
                identified==program);
        }
        mac_gl_program_cache_get_stats(&cache,&stats);
        before_overflow=stats.identity_hits;
        CHECK(mac_gl_program_cache_get_identified(&cache,alias_vertex[0],
            alias_fragment[0],2,&aliases[0],&identified,error,sizeof(error))&&
            identified==program);
        mac_gl_program_cache_get_stats(&cache,&stats);
        CHECK(stats.identity_hits==before_overflow);
        CHECK(mac_gl_program_cache_get_identified(&cache,alias_vertex[0],
            alias_fragment[0],2,&aliases[0],&identified,error,sizeof(error))&&
            identified==program);
        mac_gl_program_cache_get_stats(&cache,&stats);
        CHECK(stats.identity_hits==before_overflow+1);
        CHECK(mac_gl_program_cache_get_identified(&cache,source,fragment_source,2,
            &partial_identity,&identified,error,sizeof(error))&&identified==program);
        CHECK(mac_gl_program_cache_get_identified(&cache,source,fragment_source,2,
            &zero_identity,&identified,error,sizeof(error))&&identified==program);
        mac_gl_program_cache_get_stats(&cache,&stats);
        CHECK(stats.identity_hits==before_overflow+1);
    }
    glUseProgram(0);
    mac_gl_program_cache_dispose(&cache);
    CHECK(!glIsProgram(first)&&!glIsProgram(second)&&!glIsProgram(program));
    CHECK(!mac_gl_program_cache_active_attributes(&cache,first,&active_mask)&&active_mask==0);
    mac_gl_program_cache_init(&cache);

    /* The same caller-owned buffers may hold different programs later. A
       source-address hint must always verify contents, including fragments. */
    {
        char mutable_fragment[256],same_vertex[256];GLuint a=0,b=0,c=0;
        sprintf(source,vertex_template,0,0);
        strcpy(mutable_fragment,fragment_source);
        CHECK(mac_gl_program_cache_get(&cache,source,mutable_fragment,1,&a,error,sizeof(error)));
        strcpy(mutable_fragment,fragment_source_alt);
        CHECK(mac_gl_program_cache_get(&cache,source,mutable_fragment,1,&b,error,sizeof(error)));
        CHECK(a!=b);
        strcpy(mutable_fragment,fragment_source);
        CHECK(mac_gl_program_cache_get(&cache,source,mutable_fragment,1,&c,error,sizeof(error))&&c==a);
        strcpy(same_vertex,source);
        CHECK(mac_gl_program_cache_get(&cache,same_vertex,mutable_fragment,1,&c,error,sizeof(error))&&c==a);
        mac_gl_program_cache_dispose(&cache);
        CHECK(!glIsProgram(a)&&!glIsProgram(b));
        mac_gl_program_cache_init(&cache);
    }

    for(i=0;i<MAC_GL_PROGRAM_CACHE_CAPACITY;++i) {
        sprintf(source,"#version 120\n/* unique cache program %u */\nattribute vec4 v0_in;\nuniform vec4 tint;\nvoid main(){gl_Position=v0_in+tint*0.000001;}\n",i);
        if(i==1) {
            CHECK(mac_gl_program_cache_get_identified(&cache,source,fragment_source,
                1,&eviction_identity,&old_programs[i],error,sizeof(error)));
            identity_program=old_programs[i];
        } else CHECK(get_program(&cache,source,1,&old_programs[i]));
        if(!old_programs[i])goto cache_done;
    }
    /* Touch the oldest entry so insertion evicts the next least-recently used. */
    sprintf(source,"#version 120\n/* unique cache program 0 */\nattribute vec4 v0_in;\nuniform vec4 tint;\nvoid main(){gl_Position=v0_in+tint*0.000001;}\n");
    CHECK(get_program(&cache,source,1,&program));CHECK(program==old_programs[0]);
    location=mac_gl_program_cache_uniform_location(&cache,old_programs[1],"tint");
    CHECK(location>=0&&glGetUniformLocation(old_programs[1],"tint")==location);
    CHECK(mac_gl_program_cache_uniform_location(&cache,old_programs[1],"removed")==-1);
    mac_gl_program_cache_get_stats(&cache,&stats);
    CHECK(stats.uniform_lookups==2&&stats.uniform_hits==0);
    sprintf(source,"#version 120\n/* unique cache program %u */\nattribute vec4 v0_in;\nvoid main(){gl_Position=v0_in;}\n",MAC_GL_PROGRAM_CACHE_CAPACITY);
    CHECK(get_program(&cache,source,1,&newest));CHECK(newest!=old_programs[0]);
    CHECK(!glIsProgram(old_programs[1]));
    mac_gl_program_cache_get_stats(&cache,&stats);
    misses_before_reload=stats.misses;
    sprintf(source,"#version 120\n/* unique cache program 1 */\nattribute vec4 v0_in;\nuniform vec4 tint;\nvoid main(){gl_Position=v0_in+tint*0.000001;}\n");
    CHECK(mac_gl_program_cache_get_identified(&cache,source,fragment_source,1,
        &eviction_identity,&reloaded_identity_program,error,sizeof(error)));
    mac_gl_program_cache_get_stats(&cache,&stats);
    CHECK(identity_program==old_programs[1]&&stats.misses==misses_before_reload+1&&
        glIsProgram(reloaded_identity_program));
    sprintf(source,"#version 120\n/* replacement after eviction */\nattribute vec4 v0_in;\nuniform vec4 fresh;\nvoid main(){gl_Position=v0_in+fresh*0.000001;}\n");
    CHECK(mac_gl_program_cache_get(&cache,source,fragment_source_alt,1,
        &recycled_candidate,error,sizeof(error)));
    if(recycled_candidate==old_programs[1]) {
        CHECK(mac_gl_program_cache_uniform_location(&cache,recycled_candidate,"fresh")>=0);
        CHECK(mac_gl_program_cache_uniform_location(&cache,recycled_candidate,"tint")==-1);
    }
    mac_gl_program_cache_get_stats(&cache,&stats);
    CHECK(stats.entries==MAC_GL_PROGRAM_CACHE_CAPACITY&&stats.evictions==3);

    /* Compilation failure reports its driver log and leaves the cache intact. */
    CHECK(!mac_gl_program_cache_get(&cache,"#version 120\nnot valid GLSL\n",
        fragment_source,0,&program,error,sizeof(error)));
    CHECK(program==0&&error[0]!='\0');
    mac_gl_program_cache_get_stats(&cache,&stats);
    CHECK(stats.entries==MAC_GL_PROGRAM_CACHE_CAPACITY&&stats.evictions==3);

cache_done:
    glUseProgram(0);
    mac_gl_program_cache_dispose(&cache);
    CHECK(glGetError()==GL_NO_ERROR);
    for(i=0;i<MAC_GL_PROGRAM_CACHE_CAPACITY;++i)
        if(old_programs[i])CHECK(!glIsProgram(old_programs[i]));
    CHECK(!glIsProgram(newest));
    CHECK(!glIsProgram(reloaded_identity_program));
    CHECK(!glIsProgram(recycled_candidate));
    mac_gl_program_cache_get_stats(&cache,&stats);
    CHECK(stats.entries==0&&stats.hits==0&&stats.misses==0&&stats.evictions==0);
    CHECK(stats.identity_hits==0);
    CHECK(stats.uniform_lookups==0&&stats.uniform_hits==0);
done:
    if(context){CGLSetCurrentContext(NULL);CGLDestroyContext(context);}
    if(format)CGLDestroyPixelFormat(format);
    printf("native GL linked-program cache: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
