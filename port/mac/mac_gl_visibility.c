#include "mac_gl_visibility.h"

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#undef memset
#undef strlen
#undef strstr
#include <stdint.h>
#include <string.h>

#ifndef GL_SAMPLES_PASSED_ARB
#define GL_SAMPLES_PASSED_ARB 0x8914
#endif
#ifndef GL_QUERY_RESULT_ARB
#define GL_QUERY_RESULT_ARB 0x8866
#endif
#ifndef GL_QUERY_RESULT_AVAILABLE_ARB
#define GL_QUERY_RESULT_AVAILABLE_ARB 0x8867
#endif
#ifndef GL_CURRENT_QUERY_ARB
#define GL_CURRENT_QUERY_ARB 0x8865
#endif

#define MAC_GL_VISIBILITY_SLOTS 4096
#define MAC_GL_VISIBILITY_QUERY_COUNT (MAC_GL_VISIBILITY_SLOTS + 1)

struct mac_gl_visibility_state {
    GLuint queries[MAC_GL_VISIBILITY_QUERY_COUNT];
    GLuint scratch_query;
    uint32_t known[MAC_GL_VISIBILITY_SLOTS];
    float samples_per_game_pixel[MAC_GL_VISIBILITY_SLOTS];
    unsigned char pending[MAC_GL_VISIBILITY_SLOTS];
    unsigned char known_valid[MAC_GL_VISIBILITY_SLOTS];
    unsigned initialized:1;
    unsigned active:1;
};

static struct mac_gl_visibility_state state;

static int positive_finite_float(float value)
{
    union { float f; uint32_t u; } bits;
    bits.f=value;
    return value>0.0f&&(bits.u&0x7f800000U)!=0x7f800000U;
}

static int extension_present(const char *name)
{
    const char *extensions=(const char *)glGetString(GL_EXTENSIONS);
    const char *p;
    size_t length;
    if(!extensions||!name||!name[0])return 0;
    length=strlen(name);
    p=extensions;
    while((p=strstr(p,name))!=NULL) {
        if((p==extensions||p[-1]==' ')&&(p[length]==' '||p[length]=='\0'))return 1;
        p+=length;
    }
    return 0;
}

int mac_gl_visibility_start(void)
{
    unsigned i;
    GLint current_query=0;
    if(state.initialized||state.active||!extension_present("GL_ARB_occlusion_query")||
       glGetError()!=GL_NO_ERROR)return 0;
    glGetQueryivARB(GL_SAMPLES_PASSED_ARB,GL_CURRENT_QUERY_ARB,&current_query);
    if(glGetError()!=GL_NO_ERROR||current_query)return 0;
    memset(&state,0,sizeof(state));
    glGenQueriesARB(MAC_GL_VISIBILITY_QUERY_COUNT,state.queries);
    if(glGetError()!=GL_NO_ERROR)goto fail;
    for(i=0;i<MAC_GL_VISIBILITY_QUERY_COUNT;i++)if(!state.queries[i])goto fail;
    state.scratch_query=state.queries[MAC_GL_VISIBILITY_SLOTS];
    state.initialized=1;
    return 1;
fail:
    glDeleteQueriesARB(MAC_GL_VISIBILITY_QUERY_COUNT,state.queries);
    memset(&state,0,sizeof(state));
    return 0;
}

void mac_gl_visibility_stop(void)
{
    if(!state.initialized)return;
    if(state.active) {
        glEndQueryARB(GL_SAMPLES_PASSED_ARB);
        state.active=0;
    }
    /* Swapping completed queries leaves the original scratch entry stale. */
    glDeleteQueriesARB(MAC_GL_VISIBILITY_SLOTS,state.queries);
    glDeleteQueriesARB(1,&state.scratch_query);
    memset(&state,0,sizeof(state));
}

int mac_gl_visibility_begin(void)
{
    GLint current_query=0;
    if(!state.initialized||state.active)return 0;
    if(glGetError()!=GL_NO_ERROR)return 0;
    glGetQueryivARB(GL_SAMPLES_PASSED_ARB,GL_CURRENT_QUERY_ARB,&current_query);
    if(glGetError()!=GL_NO_ERROR||current_query)return 0;
    glBeginQueryARB(GL_SAMPLES_PASSED_ARB,state.scratch_query);
    if(glGetError()!=GL_NO_ERROR)return 0;
    state.active=1;
    return 1;
}

int mac_gl_visibility_end(uint32_t index,float samples_per_game_pixel)
{
    GLuint old_query;
    unsigned slot=(unsigned)(index%MAC_GL_VISIBILITY_SLOTS);
    if(!state.initialized||!state.active)return MAC_GL_VISIBILITY_INVALID;
    if(!positive_finite_float(samples_per_game_pixel)) {
        glEndQueryARB(GL_SAMPLES_PASSED_ARB);
        state.active=0;
        if(glGetError()!=GL_NO_ERROR)return MAC_GL_VISIBILITY_GL_ERROR;
        return MAC_GL_VISIBILITY_INVALID;
    }
    glEndQueryARB(GL_SAMPLES_PASSED_ARB);
    if(glGetError()!=GL_NO_ERROR) {
        state.active=0;
        return MAC_GL_VISIBILITY_GL_ERROR;
    }
    state.active=0;
    old_query=state.queries[slot];
    state.queries[slot]=state.scratch_query;
    state.scratch_query=old_query;
    state.pending[slot]=1;
    state.samples_per_game_pixel[slot]=samples_per_game_pixel;
    return MAC_GL_VISIBILITY_OK;
}

int mac_gl_visibility_result(uint32_t index,uint32_t *result,uint64_t *time_stamp)
{
    unsigned slot=(unsigned)(index%MAC_GL_VISIBILITY_SLOTS);
    GLuint available=0,samples=0;
    if(time_stamp)*time_stamp=0;
    if(!state.initialized) {
        if(result)*result=0;
        return MAC_GL_VISIBILITY_UNAVAILABLE;
    }
    if(state.pending[slot]) {
        glGetQueryObjectuivARB(state.queries[slot],GL_QUERY_RESULT_AVAILABLE_ARB,&available);
        if(glGetError()!=GL_NO_ERROR)return MAC_GL_VISIBILITY_GL_ERROR;
        if(available) {
            float area=state.samples_per_game_pixel[slot];
            glGetQueryObjectuivARB(state.queries[slot],GL_QUERY_RESULT_ARB,&samples);
            if(glGetError()!=GL_NO_ERROR)return MAC_GL_VISIBILITY_GL_ERROR;
            if(area>1.0f)samples=(GLuint)((float)samples/area+0.5f);
            state.known[slot]=(uint32_t)samples;
            state.known_valid[slot]=1;
            state.pending[slot]=0;
        }
    }
    if(result)*result=state.known_valid[slot]?state.known[slot]:0;
    return MAC_GL_VISIBILITY_OK;
}

#ifdef MAC_GL_VISIBILITY_TEST
int mac_gl_visibility_test_query_ids(unsigned int *ids,unsigned capacity)
{
    unsigned i;
    if(!state.initialized||!ids||capacity<MAC_GL_VISIBILITY_QUERY_COUNT)return 0;
    for(i=0;i<MAC_GL_VISIBILITY_SLOTS;i++)ids[i]=(unsigned int)state.queries[i];
    ids[MAC_GL_VISIBILITY_SLOTS]=(unsigned int)state.scratch_query;
    return MAC_GL_VISIBILITY_QUERY_COUNT;
}
#endif
