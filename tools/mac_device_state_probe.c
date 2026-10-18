#include "engine_device_state.h"
#include "engine_device_factory.h"
#include "engine_device_resources.h"
#include "engine_screen.h"
#include "native_resource_memory.h"
#include "native_bitmap_tags.h"
#include "mac_gpu_textures.h"
#include "retail_geometry.h"
#include "bitmaps/bitmap_group.h"
#include "source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.h"
/* Test-owned file buffers and native tag owners use libc allocation. */
#undef malloc
#undef free
#undef memset
#undef memcpy
#undef memcmp
#undef strcmp
#include <stdlib.h>
#include <string.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <stdarg.h>
void platform_log(const char *format,...){va_list a;va_start(a,format);vfprintf(stderr,format,a);va_end(a);fputc('\n',stderr);}
/* Platform boundary uses native libc, outside engine memory instrumentation. */
#undef memset
#undef memcpy
#undef memcmp
#undef strcmp
static unsigned checks,failures,callback_count;
static float observed_depth;
static DWORD callback_context;
static D3DBaseTexture *texture_to_draw;
static unsigned char expected_texture_pixel[4];
static int retail_dxt_gpu_only;
static struct mac_native_bitmap_tag *texture_bitmap_tag;
static D3DBaseTexture *volume_to_upload;
static unsigned char *expected_volume_rgba;
static unsigned volume_width,volume_height,volume_depth;
static struct mac_native_bitmap_tag volume_tag;
struct probe_environment_vertex { float position[3];uint32_t normal,binormal,tangent;float texcoord[2]; };
struct probe_model_vertex { float position[3];uint32_t normal,binormal,tangent;short texcoord[2];unsigned char nodes[2];short node_weight; };
static struct probe_environment_vertex actual_environment_triangle[3];
static WORD actual_environment_indices[3]={0,1,2};
static unsigned actual_environment_projection[2];
static int actual_environment_ready;
typedef char probe_environment_vertex_size[sizeof(struct probe_environment_vertex)==32?1:-1];
typedef char probe_model_vertex_size[sizeof(struct probe_model_vertex)==32?1:-1];
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__);}}while(0)
static void WINAPI callback(DWORD context){++callback_count;callback_context=context;}
static long probe_floor(float value)
{ long n=(long)value;if((float)n>value)--n;return n; }
static uint32_t pack_norm_component(float value,unsigned bits)
{ long n=probe_floor(value*(bits==11?1023.5f:511.5f));return (uint32_t)n&((1UL<<bits)-1); }
static uint32_t pack_norm3(const float value[3])
{ return (pack_norm_component(value[2],10)<<22)|(pack_norm_component(value[1],11)<<11)|pack_norm_component(value[0],11); }
static double probe_triangle_area(const struct mac_retail_vertex *v0,const struct mac_retail_vertex *v1,
    const struct mac_retail_vertex *v2,unsigned a,unsigned b)
{ return ((double)v1->position[a]-v0->position[a])*((double)v2->position[b]-v0->position[b])-
    ((double)v1->position[b]-v0->position[b])*((double)v2->position[a]-v0->position[a]); }
static int capture_actual_environment(void *context,const struct mac_retail_material *material)
{
    double best=*(double *)context;uint32_t i;unsigned a,b;uint16_t selected[3];
    for(i=0;i<material->triangle_count;++i) {
        uint16_t ix[3]={material->indices[i*3],material->indices[i*3+1],material->indices[i*3+2]};
        const struct mac_retail_vertex *p=&material->vertices[ix[0]],*q=&material->vertices[ix[1]],*r=&material->vertices[ix[2]];
        double area;unsigned pair;
        for(pair=0;pair<3;++pair) {
            static const unsigned axes[3][2]={{0,1},{0,2},{1,2}};
            area=probe_triangle_area(p,q,r,axes[pair][0],axes[pair][1]);if(area<0)area=-area;
            if(area>best) { best=area;selected[0]=ix[0];selected[1]=ix[1];selected[2]=ix[2];
                actual_environment_projection[0]=axes[pair][0];actual_environment_projection[1]=axes[pair][1]; }
        }
    }
    if(best>*(double *)context) {
        uint32_t j;double lo[2]={0,0},hi[2]={0,0},center[2];float max_extent;unsigned ax=actual_environment_projection[0],ay=actual_environment_projection[1];
        for(j=0;j<3;++j) {
            const struct mac_retail_vertex *src=&material->vertices[selected[j]];struct probe_environment_vertex *dst=&actual_environment_triangle[j];
            unsigned k;for(k=0;k<3;++k)dst->position[k]=src->position[k];
            float decoded[3];unsigned component;
            dst->normal=pack_norm3(src->normal);dst->binormal=pack_norm3(src->binormal);dst->tangent=pack_norm3(src->tangent);
            mac_device_unpack_normpacked3(dst->normal,decoded);for(component=0;component<3;++component)CHECK(decoded[component]-src->normal[component]<0.01f&&decoded[component]-src->normal[component]>-0.01f);
            mac_device_unpack_normpacked3(dst->binormal,decoded);for(component=0;component<3;++component)CHECK(decoded[component]-src->binormal[component]<0.01f&&decoded[component]-src->binormal[component]>-0.01f);
            mac_device_unpack_normpacked3(dst->tangent,decoded);for(component=0;component<3;++component)CHECK(decoded[component]-src->tangent[component]<0.01f&&decoded[component]-src->tangent[component]>-0.01f);
            dst->texcoord[0]=src->uv[0];dst->texcoord[1]=src->uv[1];
            if(j==0){lo[0]=hi[0]=src->position[ax];lo[1]=hi[1]=src->position[ay];}
            else {if(src->position[ax]<lo[0])lo[0]=src->position[ax];if(src->position[ax]>hi[0])hi[0]=src->position[ax];
                if(src->position[ay]<lo[1])lo[1]=src->position[ay];if(src->position[ay]>hi[1])hi[1]=src->position[ay];}
        }
        center[0]=(lo[0]+hi[0])*0.5;center[1]=(lo[1]+hi[1])*0.5;
        max_extent=(float)((hi[0]-lo[0])>(hi[1]-lo[1])?(hi[0]-lo[0]):(hi[1]-lo[1]));
        if(!(max_extent>0.0f))return 0;
        for(j=0;j<3;++j) {
            float x=actual_environment_triangle[j].position[ax],y=actual_environment_triangle[j].position[ay];
            actual_environment_triangle[j].position[0]=(x-(float)center[0])*(1.5f/max_extent);
            actual_environment_triangle[j].position[1]=(y-(float)center[1])*(1.5f/max_extent);
            actual_environment_triangle[j].position[2]=0.5f;
        }
        *(double *)context=best;actual_environment_ready=1;
    }
    return 1;
}
static int clear_frame(void *context,const struct mac_input_frame *input,unsigned width,unsigned height)
{
    int mode=*(int *)context;(void)input;
    if(width!=16||height!=12)return 0;
    if(mode==23) {
        struct mac_gpu_texture_scope scope;
        const void *textures[4]={texture_to_draw,NULL,NULL,NULL};
        unsigned char samplers[4],*gpu_pixels;
        float scales[4][4];const char *error=NULL;
        const struct bitmap_data *bitmap=texture_bitmap_tag?
            (const struct bitmap_data *)texture_bitmap_tag->root->bitmaps.address:NULL;
        const unsigned char *expected;uint32_t w=0,h=0;size_t bytes=0,at;
        GLint name=0,compressed=0,image_bytes=0;unsigned max_delta=0,alpha_mismatches=0,mismatches=0;
        if(!bitmap||bitmap->format!=14||bitmap->type!=0)return 0;
        expected=mac_native_bitmap_cache_rgba(texture_to_draw,0,&w,&h,&bytes);
        if(!expected||!w||!h||bytes!=(size_t)w*h*4)return 0;
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MINFILTER,D3DTEXF_POINT);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MAGFILTER,D3DTEXF_POINT);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MIPFILTER,D3DTEXF_NONE);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_ADDRESSU,D3DTADDRESS_CLAMP);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_ADDRESSV,D3DTADDRESS_CLAMP);
        if(!mac_gpu_textures_bind(&scope,textures,1,samplers,scales,&error)) {
            fprintf(stderr,"DXT GPU bind: %s\n",error?error:"unknown");CHECK(0);return 0;
        }
        glActiveTextureARB(GL_TEXTURE0_ARB);
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&name);
        glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_COMPRESSED,&compressed);
        glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_COMPRESSED_IMAGE_SIZE,&image_bytes);
        CHECK(samplers[0]==1&&name!=0&&glIsTexture((GLuint)name));
        CHECK(compressed==GL_TRUE&&image_bytes>0);
        gpu_pixels=malloc(bytes);CHECK(gpu_pixels!=NULL);
        if(gpu_pixels) {
            glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,gpu_pixels);
            CHECK(glGetError()==GL_NO_ERROR);
            for(at=0;at<bytes;at+=4) {
                unsigned c;for(c=0;c<4;++c) {
                    unsigned a=gpu_pixels[at+c],b=expected[at+c];
                    unsigned delta=a>b?a-b:b-a;
                    if(delta>max_delta)max_delta=delta;
                    if(c==3&&delta)++alpha_mismatches;
                    if(delta)++mismatches;
                }
            }
            fprintf(stderr,"DXT1 GPU readback: %ux%u compressed-bytes=%d mismatched-channels=%u alpha-mismatches=%u max-delta=%u\n",
                (unsigned)w,(unsigned)h,(int)image_bytes,mismatches,alpha_mismatches,max_delta);
            CHECK(mismatches==0&&alpha_mismatches==0);
            free(gpu_pixels);
        }
        mac_gpu_textures_end(&scope);
        D3DDevice_Present(NULL,NULL,NULL,NULL);
        return 1;
    }
    if(mode==20) {
        struct mac_gpu_texture_scope scope;
        const void *textures[4]={NULL,NULL,NULL,NULL};
        unsigned char samplers[4];float scales[4][4];const char *error=NULL;
        unsigned char *pixels;GLint w=0,h=0,d=0,name0=0,name1=0;
        size_t bytes=(size_t)volume_width*volume_height*volume_depth*4;
        struct mac_gpu_texture_cache_stats before,after;
        textures[0]=volume_to_upload;
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MINFILTER,D3DTEXF_POINT);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MAGFILTER,D3DTEXF_POINT);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MIPFILTER,D3DTEXF_NONE);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_ADDRESSU,D3DTADDRESS_WRAP);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_ADDRESSV,D3DTADDRESS_WRAP);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_ADDRESSW,D3DTADDRESS_WRAP);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MAXMIPLEVEL,0);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MAXANISOTROPY,1);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MIPMAPLODBIAS,0);
        if(!mac_gpu_textures_bind(&scope,textures,2,samplers,scales,&error)) {
            fprintf(stderr,"volume upload: %s\n",error?error:"unknown");CHECK(0);return 0;
        }
        CHECK(samplers[0]==2&&scope.owned_count==0&&scope.cache_entry_count==1);
        glActiveTextureARB(GL_TEXTURE0_ARB);
        glGetIntegerv(GL_TEXTURE_BINDING_3D,&name0);
        CHECK(name0!=0&&glIsTexture((GLuint)name0));
        glGetTexLevelParameteriv(GL_TEXTURE_3D,0,GL_TEXTURE_WIDTH,&w);
        glGetTexLevelParameteriv(GL_TEXTURE_3D,0,GL_TEXTURE_HEIGHT,&h);
        glGetTexLevelParameteriv(GL_TEXTURE_3D,0,GL_TEXTURE_DEPTH,&d);
        CHECK(w==(GLint)volume_width&&h==(GLint)volume_height&&d==(GLint)volume_depth);
        pixels=malloc(bytes);CHECK(pixels!=NULL);
        if(pixels) {
            glGetTexImage(GL_TEXTURE_3D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
            CHECK(glGetError()==GL_NO_ERROR);
            CHECK(!memcmp(pixels,expected_volume_rgba,bytes));free(pixels);
        }
        mac_gpu_textures_end(&scope);
        CHECK(glIsTexture((GLuint)name0));

        /* An unpinned private image can change sampler state without another
           upload. Invalid state is rejected before the cached object's key
           or GL parameters are changed. */
        mac_gpu_textures_cache_get_stats(&before);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_ADDRESSU,D3DTADDRESS_CLAMP);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MAXANISOTROPY,2);
        error=NULL;
        CHECK(!mac_gpu_textures_bind(&scope,textures,2,samplers,scales,&error));
        CHECK(error!=NULL);
        mac_gpu_textures_cache_get_stats(&after);
        CHECK(after.entries==before.entries&&after.bytes==before.bytes&&
            after.misses==before.misses&&after.sampler_reuses==before.sampler_reuses);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MAXANISOTROPY,1);
        error=NULL;
        CHECK(mac_gpu_textures_bind(&scope,textures,2,samplers,scales,&error));
        glActiveTextureARB(GL_TEXTURE0_ARB);glGetIntegerv(GL_TEXTURE_BINDING_3D,&name1);
        CHECK(name1==name0&&scope.cache_entry_count==1&&scope.owned_count==0);
        glGetTexParameteriv(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,&w);
        CHECK(w==GL_CLAMP_TO_EDGE);
        mac_gpu_textures_end(&scope);
        mac_gpu_textures_cache_get_stats(&after);
        CHECK(after.entries==before.entries&&after.bytes==before.bytes&&
            after.misses==before.misses&&after.sampler_reuses==before.sampler_reuses+1);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_ADDRESSU,D3DTADDRESS_WRAP);
        error=NULL;
        CHECK(mac_gpu_textures_bind(&scope,textures,2,samplers,scales,&error));
        glActiveTextureARB(GL_TEXTURE0_ARB);glGetIntegerv(GL_TEXTURE_BINDING_3D,&name1);
        CHECK(name1==name0&&scope.cache_entry_count==1&&scope.owned_count==0);
        glGetTexParameteriv(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,&w);
        CHECK(w==GL_REPEAT);
        mac_gpu_textures_end(&scope);
        mac_gpu_textures_cache_get_stats(&after);
        CHECK(after.entries==before.entries&&after.bytes==before.bytes&&
            after.misses==before.misses&&after.sampler_reuses==before.sampler_reuses+2);

        /* The same native image with a different stage sampler must not share
           mutable GL sampler state. The stage-0 copy is a cache hit; stage 1
           gets a separate owned upload and the exact clamp mode. */
        mac_gpu_textures_cache_get_stats(&before);
        memcpy(D3D__TextureState[1],D3D__TextureState[0],sizeof(D3D__TextureState[1]));
        D3DDevice_SetTextureState_Deferred(1,D3DTSS_ADDRESSU,D3DTADDRESS_CLAMP);
        textures[1]=volume_to_upload;
        error=NULL;
        if(!mac_gpu_textures_bind(&scope,textures,2U|(2U<<5),samplers,scales,&error)) {
            fprintf(stderr,"two-stage volume bind: %s\n",error?error:"unknown");CHECK(0);return 0;
        }
        CHECK(scope.cache_entry_count==2&&scope.owned_count==0);
        glActiveTextureARB(GL_TEXTURE0_ARB);glGetIntegerv(GL_TEXTURE_BINDING_3D,&name0);
        glActiveTextureARB(GL_TEXTURE0_ARB+1);glGetIntegerv(GL_TEXTURE_BINDING_3D,&name1);
        CHECK(name0!=0&&name1!=0&&name0!=name1);
        glActiveTextureARB(GL_TEXTURE0_ARB);
        glGetTexParameteriv(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,&w);CHECK(w==GL_REPEAT);
        glActiveTextureARB(GL_TEXTURE0_ARB+1);
        glGetTexParameteriv(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,&w);CHECK(w==GL_CLAMP_TO_EDGE);
        mac_gpu_textures_end(&scope);
        CHECK(glIsTexture((GLuint)name0)&&glIsTexture((GLuint)name1));
        mac_gpu_textures_cache_get_stats(&after);
        CHECK(after.hits==before.hits+1&&after.misses==before.misses+1&&after.entries==2);

        /* Owner generation changes across unbind/rebind, even if the same tag
           object and hardware descriptor address are reused. The next bind
           must evict the prior GL copies and perform a fresh upload. */
        mac_native_bitmap_cache_unbind();
        CHECK(mac_native_bitmap_cache_bind(&volume_tag,1));
        mac_gpu_textures_cache_get_stats(&before);
        textures[1]=NULL;
        error=NULL;
        CHECK(mac_gpu_textures_bind(&scope,textures,2,samplers,scales,&error));
        CHECK(scope.cache_entry_count==1&&scope.owned_count==0);
        mac_gpu_textures_end(&scope);
        mac_gpu_textures_cache_get_stats(&after);
        CHECK(after.misses==before.misses+1&&after.entries==1);

        /* Per-stage LOD bias is independent of the private texture object's
           cache key: both stages may share the same uploaded image while the
           sampler environment remains different and scoped. */
        textures[1]=volume_to_upload;
        memcpy(D3D__TextureState[1],D3D__TextureState[0],sizeof(D3D__TextureState[1]));
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MIPMAPLODBIAS,0x3f000000U);
        D3DDevice_SetTextureState_Deferred(1,D3DTSS_MIPMAPLODBIAS,0xbf000000U);
        {
            float prior_bias[2],bias;unsigned stage;
            for(stage=0;stage<2;++stage) {
                glActiveTextureARB(GL_TEXTURE0_ARB+stage);
                glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS,&prior_bias[stage]);
            }
            mac_gpu_textures_cache_get_stats(&before);
            error=NULL;
            if(!mac_gpu_textures_bind(&scope,textures,2U|(2U<<5),samplers,scales,&error))
                fprintf(stderr,"LOD sampler bind: %s\n",error?error:"unknown");
            CHECK(error==NULL);
            CHECK(scope.cache_entry_count==1&&scope.owned_count==0);
            for(stage=0;stage<2;++stage) {
                glActiveTextureARB(GL_TEXTURE0_ARB+stage);
                glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS,&bias);
                CHECK(bias==(stage==0?0.5f:-0.5f));
            }
            mac_gpu_textures_end(&scope);
            for(stage=0;stage<2;++stage) {
                glActiveTextureARB(GL_TEXTURE0_ARB+stage);
                glGetTexEnvfv(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS,&bias);
                CHECK(bias==prior_bias[stage]);
            }
            mac_gpu_textures_cache_get_stats(&after);
            CHECK(after.entries==1&&after.misses==before.misses&&after.hits==before.hits+2);
        }
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MAXANISOTROPY,2);
        mac_gpu_textures_cache_get_stats(&before);error=NULL;
        CHECK(!mac_gpu_textures_bind(&scope,textures,2,samplers,scales,&error));
        CHECK(error!=NULL);
        mac_gpu_textures_cache_get_stats(&after);
        CHECK(after.hits==before.hits+1&&after.entries==1);
        D3DDevice_SetTextureState_Deferred(0,D3DTSS_MAXANISOTROPY,1);
        return 1;
    }
    if(mode==14){D3DDevice_DrawVertices(D3DPT_QUADLIST,65535,4);return 1;}
    if(mode==2 || (mode>=7 && mode<=13) || mode==15 || mode==16 || mode==21 || mode==22){
        DWORD shader=0;const DWORD *function=(const DWORD *)vertex_shader_table[0].code;
        float constants[60][4];unsigned char pixel[4];
        D3DTexture *target_texture=NULL;D3DSurface *target_surface=NULL;
        const struct mac_device_state *state=mac_device_state_get();
        unsigned draw_height=height;
        if(mode==12) {
            CHECK(mac_engine_device_create_texture(16,8,1,D3DUSAGE_RENDERTARGET,
                D3DFMT_LIN_A8R8G8B8,D3DPOOL_DEFAULT,&target_texture)==S_OK);
            if(!target_texture)return 0;
            CHECK(D3DTexture_GetSurfaceLevel(target_texture,0,&target_surface)==S_OK);
            if(!target_surface)return 0;
            D3DDevice_SetRenderTarget(target_surface,NULL);draw_height=8;
        }
        memset(constants,0,sizeof(constants));
        constants[0][0]=constants[1][1]=constants[2][2]=constants[3][3]=1.0f;
        constants[58][0]=8.0f;constants[58][1]=-(float)draw_height*0.5f;constants[58][2]=1.0f;
        constants[59][0]=8.0f;constants[59][1]=(float)draw_height*0.5f;
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        if(D3DDevice_CreateVertexShader(NULL,function,&shader,0)!=S_OK)return 0;
        D3DDevice_SetVertexShader(shader);
        D3DDevice_SetVertexShaderConstant(-96,constants,60);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSCOMBINERCOUNT,0);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSABCD,0x04200000);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSEFG,0x00001400);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERCONSTANT0,0);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERCONSTANT1,0);
        D3DDevice_SetRenderState_Deferred(D3DRS_CULLMODE,D3DCULL_NONE);
        D3DDevice_SetRenderState_Deferred(D3DRS_ALPHATESTENABLE,mode==7||mode==8);
        D3DDevice_SetRenderState_Deferred(D3DRS_ALPHAFUNC,D3DCMP_GREATER);
        D3DDevice_SetRenderState_Deferred(D3DRS_ALPHAREF,mode==7?255:128);
        D3DDevice_SetRenderState_Deferred(D3DRS_ALPHABLENDENABLE,mode==9);
        D3DDevice_SetRenderState_Deferred(D3DRS_SRCBLEND,D3DBLEND_CONSTANTALPHA);
        D3DDevice_SetRenderState_Deferred(D3DRS_DESTBLEND,D3DBLEND_INVCONSTANTALPHA);
        D3DDevice_SetRenderState_Deferred(D3DRS_BLENDCOLOR,0x80000000);
        D3DDevice_SetRenderState_Deferred(D3DRS_BLENDOP,D3DBLENDOP_ADD);
        D3DDevice_SetRenderState_Deferred(D3DRS_FOGENABLE,mode==10);
        /* Linear fog ends before the triangle. The final combiner must
           select the uniform fog color instead of the vertex color. */
        if(mode==10)D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSABCD,0x13040300);
        D3DDevice_SetRenderState_Deferred(D3DRS_FOGCOLOR,0xff112233);
        D3DDevice_SetRenderState_Deferred(D3DRS_FOGTABLEMODE,D3DFOG_LINEAR);
        D3DDevice_SetRenderState_Deferred(D3DRS_FOGDENSITY,0);
        D3DDevice_SetRenderState_Deferred(D3DRS_FOGSTART,0xbf800000);
        D3DDevice_SetRenderState_Deferred(D3DRS_FOGEND,0);
        D3DDevice_SetRenderState_Deferred(D3DRS_COLORWRITEENABLE,mode==11?
            D3DCOLORWRITEENABLE_RED:D3DCOLORWRITEENABLE_ALL);
        if(mode==15||mode==16) {
            D3DDevice_Clear(0,NULL,D3DCLEAR_STENCIL,0,1.0f,mode==16?7:0);
            D3DDevice_SetRenderState_Deferred(D3DRS_STENCILENABLE,TRUE);
            D3DDevice_SetRenderState_Deferred(D3DRS_STENCILFUNC,mode==16?D3DCMP_EQUAL:D3DCMP_ALWAYS);
            D3DDevice_SetRenderState_Deferred(D3DRS_STENCILREF,mode==16?8:7);
            D3DDevice_SetRenderState_Deferred(D3DRS_STENCILMASK,255);
            D3DDevice_SetRenderState_Deferred(D3DRS_STENCILWRITEMASK,255);
            D3DDevice_SetRenderState_Deferred(D3DRS_STENCILFAIL,D3DSTENCILOP_KEEP);
            D3DDevice_SetRenderState_Deferred(D3DRS_STENCILZFAIL,D3DSTENCILOP_KEEP);
            D3DDevice_SetRenderState_Deferred(D3DRS_STENCILPASS,D3DSTENCILOP_REPLACE);
        }
        if(mode==12||mode==15||mode==16) {
            UINT samples=999;ULONGLONG stamp=999;
            CHECK(D3DDevice_GetVisibilityTestResult((DWORD)mode,&samples,&stamp)==S_OK);
            CHECK(samples==0&&stamp==0);
            D3DDevice_BeginVisibilityTest();
        }
        if(mode==13){
            struct streamed_position {float position[4];};
            struct streamed_color {float color[4];short normalized2[2];short raw2[2];
                uint32_t d3dcolor;unsigned char byte3[3],raw1[2],normalized1[2];};
            struct streamed_position *vertices;struct streamed_color *colors;
            D3DVertexBuffer stream_buffer,color_buffer;
            DWORD declaration[11],stream_shader=0;const DWORD *stream_function=(const DWORD *)vertex_shader_table[0].code;
            const struct mac_device_state *state=mac_device_state_get();unsigned long long indexed_before;unsigned draw;
            vertices=(struct streamed_position *)platform_contiguous_alloc(8*sizeof(*vertices),16,
                PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);
            colors=(struct streamed_color *)platform_contiguous_alloc(8*sizeof(*colors),16,
                PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);
            if(!vertices||!colors)return 0;
            memset(vertices,0,8*sizeof(*vertices));memset(colors,0,8*sizeof(*colors));
            vertices[2].position[0]=-.75f;vertices[2].position[1]=-.75f;vertices[2].position[2]=.5f;vertices[2].position[3]=1.0f;
            vertices[3].position[0]=.75f;vertices[3].position[1]=-.75f;vertices[3].position[2]=.5f;vertices[3].position[3]=1.0f;
            vertices[4].position[0]=.75f;vertices[4].position[1]=.75f;vertices[4].position[2]=.5f;vertices[4].position[3]=1.0f;
            vertices[5].position[0]=-.75f;vertices[5].position[1]=.75f;vertices[5].position[2]=.5f;vertices[5].position[3]=1.0f;
            for(draw=2;draw<6;++draw){colors[draw].color[0]=.25f;colors[draw].color[1]=.5f;
                colors[draw].color[2]=.75f;colors[draw].color[3]=1.0f;
                colors[draw].normalized2[0]=-32768;colors[draw].normalized2[1]=32767;
                short raw1=-321;colors[draw].raw2[0]=-1234;colors[draw].raw2[1]=2345;
                memcpy(colors[draw].raw1,&raw1,sizeof(raw1));
                colors[draw].d3dcolor=0x80402010U;colors[draw].byte3[0]=10;
                colors[draw].byte3[1]=127;colors[draw].byte3[2]=255;
                {short normalized1=draw==2?(short)-32768:(short)32767;
                    memcpy(colors[draw].normalized1,&normalized1,sizeof(normalized1));}}
            memset(&stream_buffer,0,sizeof(stream_buffer));
            stream_buffer.Data=PLATFORM_VIRTUAL_TO_PHYSICAL((unsigned long)vertices);
            memset(&color_buffer,0,sizeof(color_buffer));
            color_buffer.Data=PLATFORM_VIRTUAL_TO_PHYSICAL((unsigned long)colors);
            declaration[0]=(1UL<<29)|0;declaration[1]=(2UL<<29)|(D3DVSDT_FLOAT4<<16)|0;
            declaration[2]=(1UL<<29)|1;declaration[3]=(2UL<<29)|(D3DVSDT_FLOAT4<<16)|9;
            declaration[4]=(2UL<<29)|(D3DVSDT_NORMSHORT2<<16)|4;
            declaration[5]=(2UL<<29)|(D3DVSDT_SHORT2<<16)|5;
            declaration[6]=(2UL<<29)|(D3DVSDT_D3DCOLOR<<16)|7;
            declaration[7]=(2UL<<29)|(D3DVSDT_PBYTE3<<16)|8;
            declaration[8]=(2UL<<29)|(D3DVSDT_SHORT1<<16)|6;
            declaration[9]=(2UL<<29)|(D3DVSDT_NORMSHORT1<<16)|10;
            declaration[10]=D3DVSD_END();
            CHECK(D3DDevice_CreateVertexShader(declaration,stream_function,&stream_shader,0)==S_OK);
            D3DDevice_SetVertexShader(stream_shader);
            D3DDevice_SetVertexData4f(2,3.0f,4.0f,5.0f,1.0f);
            D3DDevice_SetVertexData4f(3,6.0f,7.0f,8.0f,1.0f);
            D3DDevice_SetStreamSource(0,&stream_buffer,sizeof(*vertices));
            D3DDevice_SetStreamSource(1,&color_buffer,sizeof(*colors));
            D3DDevice_SetIndices(NULL,0);indexed_before=state->gpu_indexed_draw_count;
            D3DDevice_DrawVertices(D3DPT_QUADLIST,2,4);
            CHECK(state->gpu_indexed_draw_count==indexed_before);
            CHECK(state->gpu_stream_draw_active==0&&state->immediate_count==0);
            CHECK(state->immediate_vertices[8]==3.0f&&state->immediate_vertices[9]==4.0f&&
                state->immediate_vertices[10]==5.0f&&state->immediate_vertices[12]==6.0f&&
                state->immediate_vertices[13]==7.0f&&state->immediate_vertices[14]==8.0f);
            CHECK(state->immediate_vertices[16]==-1.0f&&state->immediate_vertices[17]==1.0f);
            CHECK(state->immediate_vertices[20]==-1234.0f&&state->immediate_vertices[21]==2345.0f);
            CHECK(state->immediate_vertices[24]==-321.0f);
            CHECK(state->immediate_vertices[28]-64.0f/255.0f<0.00001f&&state->immediate_vertices[28]-64.0f/255.0f>-0.00001f&&
                state->immediate_vertices[29]-32.0f/255.0f<0.00001f&&state->immediate_vertices[29]-32.0f/255.0f>-0.00001f&&
                state->immediate_vertices[30]-16.0f/255.0f<0.00001f&&state->immediate_vertices[30]-16.0f/255.0f>-0.00001f&&
                state->immediate_vertices[31]-128.0f/255.0f<0.00001f&&state->immediate_vertices[31]-128.0f/255.0f>-0.00001f);
            CHECK(state->immediate_vertices[32]-10.0f/255.0f<0.00001f&&state->immediate_vertices[32]-10.0f/255.0f>-0.00001f&&
                state->immediate_vertices[33]-127.0f/255.0f<0.00001f&&state->immediate_vertices[33]-127.0f/255.0f>-0.00001f&&
                state->immediate_vertices[34]==1.0f&&state->immediate_vertices[35]==1.0f);
            CHECK(state->immediate_vertices[40]==-1.0f&&state->immediate_vertices[104]==1.0f);
            D3DDevice_SetStreamSource(0,NULL,0);D3DDevice_SetStreamSource(1,NULL,0);
            platform_contiguous_free(colors);platform_contiguous_free(vertices);
        }else if(mode==22){
            D3DDevice_Begin(D3DPT_QUADLIST);
            D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);
            D3DDevice_SetVertexData4f(0,-0.75f,-0.75f,0.5f,1.0f);
            D3DDevice_SetVertexData4f(0,0.75f,-0.75f,0.5f,1.0f);
            D3DDevice_SetVertexData4f(0,0.75f,0.75f,0.5f,1.0f);
            D3DDevice_SetVertexData4f(0,-0.75f,0.75f,0.5f,1.0f);
            D3DDevice_End();
        }else if(mode==21){
            D3DDevice_Begin(D3DPT_POINTLIST);
            D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);
            D3DDevice_SetVertexData4f(0,0.0f,0.0f,0.5f,1.0f);
            D3DDevice_End();
        }else{
            D3DDevice_Begin(D3DPT_TRIANGLELIST);
            D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);D3DDevice_SetVertexData4f(0,-0.75f,-0.75f,0.5f,1.0f);
            D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);D3DDevice_SetVertexData4f(0,0.75f,-0.75f,0.5f,1.0f);
            D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);D3DDevice_SetVertexData4f(0,0.0f,0.75f,0.5f,1.0f);
            D3DDevice_End();
        }
        if(mode==12||mode==15||mode==16) {
            UINT samples=999;ULONGLONG stamp=999;
            CHECK(D3DDevice_EndVisibilityTest((DWORD)mode)==S_OK);
            glFinish();
            CHECK(D3DDevice_GetVisibilityTestResult((DWORD)mode,&samples,&stamp)==S_OK);
            CHECK(stamp==0);
            fprintf(stderr,"native visibility mode %d samples=%u\n",mode,(unsigned)samples);
            CHECK(mode==16?samples==0:samples>0);
        }
        glReadPixels(8,draw_height/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
        if(mode==15||mode==16) {
            unsigned char stencil=0;
            glReadPixels(8,draw_height/2,1,1,GL_STENCIL_INDEX,GL_UNSIGNED_BYTE,&stencil);
            CHECK(stencil==7);CHECK(glGetError()==GL_NO_ERROR);
            D3DDevice_SetRenderState_Deferred(D3DRS_STENCILENABLE,FALSE);
        }
        if(mode==21) {
            unsigned char image[16*12*4];unsigned n,colored=0;
            glReadPixels(0,0,16,12,GL_RGBA,GL_UNSIGNED_BYTE,image);
            for(n=0;n<16*12;++n)if(image[n*4]||image[n*4+1]||image[n*4+2]) {
                ++colored;
                CHECK(image[n*4]>=63&&image[n*4]<=64);
                CHECK(image[n*4+1]>=127&&image[n*4+1]<=128);
                CHECK(image[n*4+2]>=191&&image[n*4+2]<=192);
            }
            CHECK(colored==1);CHECK(glGetError()==GL_NO_ERROR);
        }
        else if(mode==7||mode==16) {CHECK(pixel[0]==0&&pixel[1]==0&&pixel[2]==0);}
        else if(mode==9) {CHECK(pixel[0]>=31&&pixel[0]<=33);CHECK(pixel[1]>=63&&pixel[1]<=65);CHECK(pixel[2]>=95&&pixel[2]<=97);}
        else if(mode==10) {CHECK(pixel[0]==17&&pixel[1]==34&&pixel[2]==51);}
        else if(mode==11) {CHECK(pixel[0]>=63&&pixel[0]<=64);CHECK(pixel[1]==0&&pixel[2]==0);}
        else {CHECK(pixel[0]>=63&&pixel[0]<=64);CHECK(pixel[1]>=127&&pixel[1]<=128);
            CHECK(pixel[2]>=191&&pixel[2]<=192&&pixel[3]==255);}
        if(mode==22) {
            unsigned char halves[4];
            glReadPixels(4,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,halves);
            CHECK(halves[0]>=63&&halves[0]<=64&&halves[1]>=127&&halves[1]<=128);
            glReadPixels(12,4,1,1,GL_RGBA,GL_UNSIGNED_BYTE,halves);
            CHECK(halves[0]>=63&&halves[0]<=64&&halves[1]>=127&&halves[1]<=128);
        }
        D3DDevice_SetRenderState_Deferred(D3DRS_ALPHATESTENABLE,FALSE);
        D3DDevice_SetRenderState_Deferred(D3DRS_ALPHABLENDENABLE,FALSE);
        D3DDevice_SetRenderState_Deferred(D3DRS_FOGENABLE,FALSE);
        D3DDevice_SetRenderState_Deferred(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALL);
        if(target_surface) {
            D3DDevice_SetRenderTarget((D3DSurface *)&state->back_buffer,(D3DSurface *)&state->depth_buffer);
            D3DResource_Release((D3DResource *)target_surface);
            D3DResource_Release((D3DResource *)target_texture);
        }
        D3DDevice_Present(NULL,NULL,NULL,NULL);
        return 1;
    }
    if(mode==3||(mode>=17&&mode<=19)){
        unsigned stage=mode==3?0:(unsigned)mode-16;
        DWORD shader=0;const DWORD *function=(const DWORD *)vertex_shader_table[0].code;
        float constants[60][4];unsigned char pixel[4];
        if(!texture_to_draw)return 0;
        memset(constants,0,sizeof(constants));
        constants[0][0]=constants[1][1]=constants[2][2]=constants[3][3]=1.0f;
        constants[58][0]=8.0f;constants[58][1]=-6.0f;constants[58][2]=1.0f;
        constants[59][0]=8.0f;constants[59][1]=6.0f;
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        if(D3DDevice_CreateVertexShader(NULL,function,&shader,0)!=S_OK)return 0;
        D3DDevice_SetVertexShader(shader);D3DDevice_SetVertexShaderConstant(-96,constants,60);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSTEXTUREMODES,1UL<<(stage*5));
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSABCD,((8UL+stage)<<24)|0x00200000);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSEFG,(0x18UL+stage)<<8);
        D3DDevice_SetRenderState_Deferred(D3DRS_CULLMODE,D3DCULL_NONE);
        D3DDevice_SetTexture(stage,texture_to_draw);
        D3DDevice_SetTextureState_Deferred(stage,D3DTSS_MINFILTER,D3DTEXF_POINT);
        D3DDevice_SetTextureState_Deferred(stage,D3DTSS_MAGFILTER,D3DTEXF_POINT);
        D3DDevice_SetTextureState_Deferred(stage,D3DTSS_MIPFILTER,D3DTEXF_NONE);
        D3DDevice_Begin(D3DPT_TRIANGLELIST);
        D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);D3DDevice_SetVertexData4f(0,-0.75f,-0.75f,0.5f,1.0f);
        D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);D3DDevice_SetVertexData4f(0,0.75f,-0.75f,0.5f,1.0f);
        D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);D3DDevice_SetVertexData4f(0,0.0f,0.75f,0.5f,1.0f);
        D3DDevice_End();glReadPixels(8,6,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
        CHECK(abs((int)pixel[0]-(int)expected_texture_pixel[0])<=1);
        CHECK(abs((int)pixel[1]-(int)expected_texture_pixel[1])<=1);
        CHECK(abs((int)pixel[2]-(int)expected_texture_pixel[2])<=1);
        CHECK(abs((int)pixel[3]-(int)expected_texture_pixel[3])<=1);
        D3DDevice_SetTexture(stage,NULL);D3DDevice_SetRenderState_Deferred(D3DRS_PSTEXTUREMODES,0);
        {GLenum e=glGetError();if(e!=GL_NO_ERROR)fprintf(stderr,"texture test left GL error 0x%lx before present\n",(unsigned long)e);CHECK(e==GL_NO_ERROR);}
        D3DDevice_Present(NULL,NULL,NULL,NULL);
        {GLenum e=glGetError();if(e!=GL_NO_ERROR)fprintf(stderr,"present left GL error 0x%lx\n",(unsigned long)e);CHECK(e==GL_NO_ERROR);}
        return 1;
    }
    if(mode==4){
        struct streamed_vertex {float position[4],color[4];};
        struct streamed_vertex *vertices;WORD *index_data;D3DVertexBuffer stream_buffer;D3DIndexBuffer index_buffer;
        DWORD declaration[4],shader=0;const DWORD *function=(const DWORD *)vertex_shader_table[0].code;
        float constants[60][4];unsigned char pixel[4];unsigned draw;
        vertices=(struct streamed_vertex *)platform_contiguous_alloc(4*sizeof(*vertices),16,
            PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);
        index_data=(WORD *)platform_contiguous_alloc(3*sizeof(*index_data),16,
            PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);if(!vertices||!index_data)return 0;
        vertices[0].position[0]=-.75f;vertices[0].position[1]=-.75f;vertices[0].position[2]=.5f;vertices[0].position[3]=1.0f;
        vertices[1].position[0]=.75f;vertices[1].position[1]=-.75f;vertices[1].position[2]=.5f;vertices[1].position[3]=1.0f;
        vertices[2].position[0]=0.0f;vertices[2].position[1]=.75f;vertices[2].position[2]=.5f;vertices[2].position[3]=1.0f;
        vertices[3].position[0]=.95f;vertices[3].position[1]=.95f;vertices[3].position[2]=.5f;vertices[3].position[3]=1.0f;
        for(draw=0;draw<4;++draw){vertices[draw].color[0]=.25f;vertices[draw].color[1]=.5f;vertices[draw].color[2]=.75f;vertices[draw].color[3]=1.0f;}
        memset(&stream_buffer,0,sizeof(stream_buffer));stream_buffer.Data=PLATFORM_VIRTUAL_TO_PHYSICAL((unsigned long)vertices);
        memset(&index_buffer,0,sizeof(index_buffer));index_buffer.Data=(DWORD)index_data;
        declaration[0]=1UL<<29;
        declaration[1]=(2UL<<29)|(D3DVSDT_FLOAT4<<16)|0;
        declaration[2]=(2UL<<29)|(D3DVSDT_FLOAT4<<16)|9;
        declaration[3]=D3DVSD_END();
        memset(constants,0,sizeof(constants));
        constants[0][0]=constants[1][1]=constants[2][2]=constants[3][3]=1.0f;
        constants[58][0]=8.0f;constants[58][1]=-6.0f;constants[58][2]=1.0f;constants[59][0]=8.0f;constants[59][1]=6.0f;
        if(D3DDevice_CreateVertexShader(declaration,function,&shader,0)!=S_OK)return 0;
        D3DDevice_SetVertexShader(shader);D3DDevice_SetVertexShaderConstant(-96,constants,60);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSTEXTUREMODES,0);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSCOMBINERCOUNT,0);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSABCD,0x04200000);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSEFG,0x00001400);
        D3DDevice_SetRenderState_Deferred(D3DRS_CULLMODE,D3DCULL_NONE);
        D3DDevice_SetStreamSource(0,&stream_buffer,sizeof(*vertices));D3DDevice_SetIndices(&index_buffer,0);
        for(draw=0;draw<2;++draw){
            D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
            index_data[0]=0;index_data[1]=1;index_data[2]=(WORD)(draw?3:2);
            D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLELIST,3,index_data);
            glReadPixels(8,6,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
            if(!draw){CHECK(pixel[0]>=63&&pixel[0]<=64);CHECK(pixel[1]>=127&&pixel[1]<=128);CHECK(pixel[2]>=191&&pixel[2]<=192&&pixel[3]==255);}
            else CHECK(pixel[0]<8&&pixel[1]<8&&pixel[2]<8&&pixel[3]==255);
        }
        D3DDevice_SetStreamSource(0,NULL,0);D3DDevice_SetIndices(NULL,0);
        platform_contiguous_free(index_data);platform_contiguous_free(vertices);
        D3DDevice_Present(NULL,NULL,NULL,NULL);return 1;
    }
    if(mode==5){
        struct probe_environment_vertex *vertices;WORD *indices;D3DVertexBuffer stream_buffer;D3DIndexBuffer index_buffer;
        DWORD declaration[7]={1UL<<29,(2UL<<29)|(D3DVSDT_FLOAT3<<16),
            (2UL<<29)|(D3DVSDT_NORMPACKED3<<16)|1,(2UL<<29)|(D3DVSDT_NORMPACKED3<<16)|2,
            (2UL<<29)|(D3DVSDT_NORMPACKED3<<16)|3,(2UL<<29)|(D3DVSDT_FLOAT2<<16)|4,D3DVSD_END()};
        DWORD shader=0;const DWORD *function=(const DWORD *)vertex_shader_table[0].code;float constants[60][4];unsigned char pixels[16*12*4];unsigned n,visible=0;
        if(!actual_environment_ready)return 0;
        vertices=(struct probe_environment_vertex *)platform_contiguous_alloc(sizeof(actual_environment_triangle),16,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);
        indices=(WORD *)platform_contiguous_alloc(sizeof(actual_environment_indices),16,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);
        if(!vertices||!indices)return 0;
        memcpy(vertices,actual_environment_triangle,sizeof(actual_environment_triangle));memcpy(indices,actual_environment_indices,sizeof(actual_environment_indices));
        memset(&stream_buffer,0,sizeof(stream_buffer));stream_buffer.Data=PLATFORM_VIRTUAL_TO_PHYSICAL((unsigned long)vertices);
        memset(&index_buffer,0,sizeof(index_buffer));index_buffer.Data=(DWORD)indices;
        memset(constants,0,sizeof(constants));constants[0][0]=constants[1][1]=constants[2][2]=constants[3][3]=1.0f;
        constants[58][0]=8.0f;constants[58][1]=-6.0f;constants[58][2]=1.0f;constants[59][0]=8.0f;constants[59][1]=6.0f;
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        if(D3DDevice_CreateVertexShader(declaration,function,&shader,0)!=S_OK)return 0;
        D3DDevice_SetVertexShader(shader);D3DDevice_SetVertexShaderConstant(-96,constants,60);
        D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSTEXTUREMODES,0);D3DDevice_SetRenderState_Deferred(D3DRS_PSCOMBINERCOUNT,0);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSABCD,0x04200000);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSEFG,0x00001400);
        D3DDevice_SetRenderState_Deferred(D3DRS_CULLMODE,D3DCULL_NONE);
        D3DDevice_SetStreamSource(0,&stream_buffer,sizeof(*vertices));D3DDevice_SetIndices(&index_buffer,0);
        D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLELIST,3,indices);
        glReadPixels(0,0,16,12,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        for(n=0;n<16*12;++n)if(pixels[n*4]>8||pixels[n*4+1]>8||pixels[n*4+2]>8)++visible;
        CHECK(visible>0);
        D3DDevice_SetStreamSource(0,NULL,0);D3DDevice_SetIndices(NULL,0);
        platform_contiguous_free(indices);platform_contiguous_free(vertices);D3DDevice_Present(NULL,NULL,NULL,NULL);return 1;
    }
    if(mode==6){
        struct probe_model_vertex *vertices;WORD *indices;float *auxiliary;D3DVertexBuffer stream_buffer,auxiliary_buffer;D3DIndexBuffer index_buffer;
        unsigned long long stream_certificate=0,auxiliary_certificate=0;
        struct mac_device_bsp_decode_cache_stats cache_stats,cache_start;
        int check_bsp_cache=0;
        GLuint sentinel_elements=0;
        GLint saved_elements=0,observed_elements=0;
        WORD dummy_element=0;
        unsigned char reference_pixels[16*12*4];
        float original_records[3][64];
        float modified_records[3][64];
        DWORD declaration[11]={1UL<<29,(2UL<<29)|(D3DVSDT_FLOAT3<<16),
            (2UL<<29)|(D3DVSDT_NORMPACKED3<<16)|1,(2UL<<29)|(D3DVSDT_NORMPACKED3<<16)|2,
            (2UL<<29)|(D3DVSDT_NORMPACKED3<<16)|3,(2UL<<29)|(D3DVSDT_NORMSHORT2<<16)|4,
            (2UL<<29)|(D3DVSDT_PBYTE2<<16)|5,(2UL<<29)|(D3DVSDT_NORMSHORT1<<16)|6,
            (1UL<<29)|1,(2UL<<29)|(D3DVSDT_FLOAT1<<16)|15,D3DVSD_END()};
        DWORD shader=0;const DWORD *function=(const DWORD *)vertex_shader_table[0].code;float constants[60][4];unsigned char pixels[16*12*4];unsigned n,visible=0;
        vertices=(struct probe_model_vertex *)platform_contiguous_alloc(6*sizeof(*vertices),16,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);
        auxiliary=(float *)platform_contiguous_alloc(6*4*sizeof(*auxiliary),16,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);
        indices=(WORD *)platform_contiguous_alloc(6*sizeof(*indices),16,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);
        if(!vertices||!auxiliary||!indices)return 0;
        memset(vertices,0,6*sizeof(*vertices));
        memset(auxiliary,0,6*4*sizeof(*auxiliary));
        auxiliary[3*4]=0.125f;auxiliary[4*4]=0.25f;auxiliary[5*4]=0.375f;
        vertices[3].position[0]=-.8f;vertices[3].position[1]=-.8f;vertices[3].position[2]=.5f;
        vertices[4].position[0]=.8f;vertices[4].position[1]=-.8f;vertices[4].position[2]=.5f;
        vertices[5].position[0]=0.0f;vertices[5].position[1]=.8f;vertices[5].position[2]=.5f;
        for(n=0;n<3;n++){static const short uv[3]={-32768,0,32767};
            unsigned source=n+3;
            vertices[source].normal=pack_norm3((float[3]){0,0,1});vertices[source].binormal=vertices[source].normal;
            vertices[source].tangent=vertices[source].normal;vertices[source].texcoord[0]=uv[n];
            vertices[source].texcoord[1]=uv[2-n];vertices[source].nodes[0]=0;vertices[source].nodes[1]=3;vertices[source].node_weight=16384;}
        indices[0]=0;indices[1]=1;indices[2]=2;
        memset(&stream_buffer,0,sizeof(stream_buffer));stream_buffer.Data=PLATFORM_VIRTUAL_TO_PHYSICAL((unsigned long)vertices);
        memset(&auxiliary_buffer,0,sizeof(auxiliary_buffer));auxiliary_buffer.Data=PLATFORM_VIRTUAL_TO_PHYSICAL((unsigned long)auxiliary);
        memset(&index_buffer,0,sizeof(index_buffer));index_buffer.Data=(DWORD)indices;
        {
            const char *enabled=getenv("HALO_MAC_BSP_DECODE_CACHE");
            check_bsp_cache=enabled&&enabled[0]=='1'&&enabled[1]=='\0';
        }
        mac_device_bsp_decode_cache_get_stats(&cache_start);
        CHECK(mac_resource_certificate_register(&stream_buffer,vertices,6*sizeof(*vertices),&stream_certificate));
        memset(constants,0,sizeof(constants));constants[0][0]=constants[1][1]=constants[2][2]=constants[3][3]=1.0f;
        constants[58][0]=8.0f;constants[58][1]=-6.0f;constants[58][2]=1.0f;constants[59][0]=8.0f;constants[59][1]=6.0f;
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        if(D3DDevice_CreateVertexShader(declaration,function,&shader,0)!=S_OK)return 0;
        D3DDevice_SetVertexShader(shader);D3DDevice_SetVertexShaderConstant(-96,constants,60);
        D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSTEXTUREMODES,0);D3DDevice_SetRenderState_Deferred(D3DRS_PSCOMBINERCOUNT,0);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSABCD,0x04200000);
        D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSEFG,0x00001400);
        D3DDevice_SetRenderState_Deferred(D3DRS_CULLMODE,D3DCULL_NONE);
        D3DDevice_SetRenderState_Deferred(D3DRS_ALPHABLENDENABLE,0);
        D3DDevice_SetStreamSource(0,&stream_buffer,sizeof(*vertices));
        D3DDevice_SetStreamSource(1,&auxiliary_buffer,4*sizeof(*auxiliary));D3DDevice_SetIndices(&index_buffer,3);
        D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLELIST,3,indices);
        CHECK(mac_device_state_get()->immediate_count==0);
        memcpy(original_records,mac_device_state_get()->immediate_vertices,sizeof(original_records));
        glReadPixels(0,0,16,12,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        for(n=0;n<16*12;++n)if(pixels[n*4]>8||pixels[n*4+1]>8||pixels[n*4+2]>8)++visible;
        CHECK(visible>0);
        memcpy(reference_pixels,pixels,sizeof(pixels));
        CHECK(original_records[0][16]==-1.0f&&original_records[0][17]==1.0f);
        CHECK(original_records[1][16]==0.0f&&original_records[1][17]==0.0f);
        CHECK(original_records[2][16]==1.0f&&original_records[2][17]==-1.0f);
        CHECK(original_records[0][24]>0.5f&&original_records[0][24]<0.501f);
        CHECK((vertices[3].nodes[0]%3)==0&&(vertices[3].nodes[1]%3)==0&&vertices[3].node_weight>=0);
        mac_device_bsp_decode_cache_get_stats(&cache_stats);
        if(check_bsp_cache)CHECK(cache_stats.draw_bypasses==cache_start.draw_bypasses+1&&
            cache_stats.draw_misses==cache_start.draw_misses&&
            cache_stats.vertex_misses==cache_start.vertex_misses);
        /* The optimized path uses client indices and must preserve a
           caller-owned element buffer instead of treating pointers as offsets. */
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&saved_elements);
        glGenBuffers(1,&sentinel_elements);CHECK(sentinel_elements!=0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sentinel_elements);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(dummy_element),&dummy_element,GL_STATIC_DRAW);
        CHECK(glGetError()==GL_NO_ERROR);
        indices[0]=2;indices[1]=0;indices[2]=1;indices[3]=1;indices[4]=2;indices[5]=0;
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLELIST,6,indices);
        glReadPixels(0,0,16,12,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        CHECK(memcmp(reference_pixels,pixels,sizeof(pixels))==0);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&observed_elements);
        CHECK(observed_elements==(GLint)sentinel_elements);
        CHECK(glGetError()==GL_NO_ERROR);
        CHECK(mac_device_state_get()->immediate_count==0);
        for(n=0;n<6;n++) {
            unsigned source=indices[n];
            CHECK(memcmp(mac_device_state_get()->immediate_vertices+n*64,
                original_records[source],sizeof(original_records[source]))==0);
        }
        /* The first draw bypasses because stream 1 is deliberately uncertified.
           Certify it, then the same complete two-stream bundle can be cached. */
        CHECK(mac_resource_certificate_register(&auxiliary_buffer,auxiliary,6*4*sizeof(*auxiliary),&auxiliary_certificate));
        D3DDevice_SetVertexData4f(9,0.25f,0.5f,0.75f,1.0f);
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        indices[0]=0;indices[1]=1;indices[2]=2;
        D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLELIST,3,indices);
        memcpy(original_records,mac_device_state_get()->immediate_vertices,sizeof(original_records));
        if(check_bsp_cache) {
            mac_device_bsp_decode_cache_get_stats(&cache_stats);
            CHECK(cache_stats.draw_misses==cache_start.draw_misses+1&&
                cache_stats.vertex_misses==cache_start.vertex_misses+3);
        }
        D3DDevice_SetVertexData4f(9,0.625f,0.375f,0.125f,1.0f);
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLELIST,3,indices);
        CHECK(mac_device_state_get()->immediate_vertices[36]==0.625f&&
            mac_device_state_get()->immediate_vertices[37]==0.375f&&
            mac_device_state_get()->immediate_vertices[38]==0.125f);
        if(check_bsp_cache) {
            mac_device_bsp_decode_cache_get_stats(&cache_stats);
            CHECK(cache_stats.draw_hits==cache_start.draw_hits+1&&
                cache_stats.vertex_hits==cache_start.vertex_hits+3);
        }
        memory_watch_prepare_write(&vertices[3],sizeof(vertices[3]));
        vertices[3].position[0]=-.55f;
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        indices[0]=0;indices[1]=1;indices[2]=2;
        D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLELIST,3,indices);
        CHECK(mac_device_state_get()->immediate_count==0);
        CHECK(mac_device_state_get()->immediate_vertices[0]==-.55f&&
            mac_device_state_get()->immediate_vertices[0]!=original_records[0][0]);
        memcpy(modified_records,mac_device_state_get()->immediate_vertices,sizeof(modified_records));
        glReadPixels(0,0,16,12,GL_RGBA,GL_UNSIGNED_BYTE,reference_pixels);
        visible=0;
        for(n=0;n<16*12;++n)if(reference_pixels[n*4]>8||reference_pixels[n*4+1]>8||reference_pixels[n*4+2]>8)++visible;
        CHECK(visible>0);
        if(check_bsp_cache) {
            mac_device_bsp_decode_cache_get_stats(&cache_stats);
            CHECK(cache_stats.draw_misses==cache_start.draw_misses+2&&
                cache_stats.vertex_misses==cache_start.vertex_misses+6);
        }
        indices[0]=2;indices[1]=0;indices[2]=1;indices[3]=1;indices[4]=2;indices[5]=0;
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLELIST,6,indices);
        glReadPixels(0,0,16,12,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        CHECK(memcmp(reference_pixels,pixels,sizeof(pixels))==0);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&observed_elements);
        CHECK(observed_elements==(GLint)sentinel_elements);
        CHECK(glGetError()==GL_NO_ERROR);
        CHECK(mac_device_state_get()->immediate_count==0);
        for(n=0;n<6;n++) {
            unsigned source=indices[n];
            CHECK(memcmp(mac_device_state_get()->immediate_vertices+n*64,
                modified_records[source],sizeof(modified_records[source]))==0);
        }
        memory_watch_prepare_write(&auxiliary[3*4],sizeof(auxiliary[3*4]));
        auxiliary[3*4]=0.5f;
        indices[0]=0;indices[1]=1;indices[2]=2;
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLELIST,3,indices);
        memcpy(modified_records,mac_device_state_get()->immediate_vertices,sizeof(modified_records));
        glReadPixels(0,0,16,12,GL_RGBA,GL_UNSIGNED_BYTE,reference_pixels);
        if(check_bsp_cache) {
            mac_device_bsp_decode_cache_get_stats(&cache_stats);
            CHECK(cache_stats.draw_misses==cache_start.draw_misses+3&&
                cache_stats.vertex_misses==cache_start.vertex_misses+9);
        }
        CHECK(mac_device_state_get()->immediate_vertices[60]==0.5f);
        /* Indexed quads must remap the two-triangle expansion too. */
        indices[0]=0;indices[1]=1;indices[2]=2;indices[3]=0;
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
        D3DDevice_DrawIndexedVertices(D3DPT_QUADLIST,4,indices);
        glReadPixels(0,0,16,12,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        CHECK(memcmp(reference_pixels,pixels,sizeof(pixels))==0);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&observed_elements);
        CHECK(observed_elements==(GLint)sentinel_elements);
        CHECK(glGetError()==GL_NO_ERROR);
        CHECK(mac_device_state_get()->immediate_count==0);
        for(n=0;n<4;n++)CHECK(memcmp(mac_device_state_get()->immediate_vertices+n*64,
            modified_records[indices[n]],sizeof(modified_records[0]))==0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,(GLuint)saved_elements);
        glDeleteBuffers(1,&sentinel_elements);CHECK(glGetError()==GL_NO_ERROR);
        D3DDevice_SetStreamSource(0,NULL,0);D3DDevice_SetStreamSource(1,NULL,0);D3DDevice_SetIndices(NULL,0);
        CHECK(mac_resource_certificate_revoke(&auxiliary_buffer));
        CHECK(mac_resource_certificate_revoke(&stream_buffer));
        platform_contiguous_free(indices);platform_contiguous_free(auxiliary);platform_contiguous_free(vertices);D3DDevice_Present(NULL,NULL,NULL,NULL);return 1;
    }
    if(mode==0) {
        D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff123456,0.25f,0);
        glReadPixels(8,6,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&observed_depth);
    }
    else {
        D3DRECT rectangle;rectangle.x1=2;rectangle.y1=3;rectangle.x2=5;rectangle.y2=7;
        D3DDevice_Clear(1,&rectangle,D3DCLEAR_TARGET,0xffabcdef,1.0f,0);
    }
    D3DDevice_Present(NULL,NULL,NULL,NULL);
    return 1;
}
static unsigned char *read_file(const char *path,size_t *size)
{FILE*f=fopen(path,"rb");long n;unsigned char*p;if(!f)return NULL;if(fseek(f,0,SEEK_END)||(n=ftell(f))<0||n>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}p=malloc((size_t)n);if(!p||fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return p;}
int main(int argc,char **argv)
{
    const struct mac_device_state *s;
    D3DTexture texture;D3DPalette palette;D3DVertexBuffer vb;D3DIndexBuffer ib;D3DMATRIX transform,readback;
    WORD indices[3];D3DPIXELSHADERDEF ps;float values[2][4],slope,offset;
    unsigned long i;unsigned long long serial;
    struct mac_gpu_texture_cache_stats texture_cache_stats;
    struct mac_native_bitmap_tag bitmap_tag;struct mac_retail_cache retail;unsigned char *map_bytes=NULL,*pixel_bytes=NULL;
    size_t map_size=0,pixel_size=0;const char *asset_error=NULL;
    int retail_cache_only=argc==4&&!strcmp(argv[3],"--retail-cache-only");
    retail_dxt_gpu_only=argc==4&&!strcmp(argv[3],"--dxt-gpu-only");
    memset(&bitmap_tag,0,sizeof(bitmap_tag));
    memset(&volume_tag,0,sizeof(volume_tag));
    if(argc!=1&&argc!=3&&!retail_cache_only&&!retail_dxt_gpu_only&&
        !(argc==2&&!strcmp(argv[1],"--invalid-quad-range")))return 2;
    if(argc==3||retail_cache_only||retail_dxt_gpu_only){struct bitmap_data *bitmap;const unsigned char *rgba;size_t bytes;uint32_t w,h;double largest_area=0.0;
        map_bytes=read_file(argv[1],&map_size);pixel_bytes=read_file(argv[2],&pixel_size);
        CHECK(map_bytes&&pixel_bytes);
        if(!map_bytes||!pixel_bytes||!mac_retail_open(map_bytes,map_size,&retail,&asset_error)||
           !mac_native_bitmap_decode(&retail,0xe2a1012dU,pixel_bytes,pixel_size,&bitmap_tag,&asset_error)){
            fprintf(stderr,"native texture fixture load: %s\n",asset_error?asset_error:"file read");return 1;}
        CHECK(mac_native_bitmap_cache_bind(&bitmap_tag,1));bitmap=bitmap_tag.root->bitmaps.address;
        texture_bitmap_tag=&bitmap_tag;
        texture_to_draw=bitmap->hardware_format;
        rgba=mac_native_bitmap_rgba(bitmap_tag.owner,0,0,&w,&h,&bytes);CHECK(rgba&&w==32&&h==32&&bytes==32*32*4);
        if(!rgba||bytes<4)return 1;memcpy(expected_texture_pixel,rgba,4);
        if(retail_dxt_gpu_only)CHECK(bitmap->format==14&&bitmap->type==0);
        else {
            CHECK(mac_retail_bsp_geometry(&retail,0,capture_actual_environment,&largest_area,&asset_error));
            CHECK(actual_environment_ready&&largest_area>0.0);
        }
    }
    CHECK(sizeof(void*)==4);CHECK(sizeof(DWORD)==4);
    CHECK(mac_device_state_get()==NULL);CHECK(mac_device_headless_identity()==NULL);
    {
        void *contiguous,*ordinary;contiguous=platform_contiguous_alloc(64,16,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);
        ordinary=malloc(64);CHECK(contiguous!=NULL&&ordinary!=NULL);
        if(contiguous){CHECK(mac_resource_range_is_allocated(contiguous,64));CHECK(mac_resource_range_is_allocated((unsigned char*)contiguous+32,32));CHECK(!mac_resource_range_is_allocated((unsigned char*)contiguous+63,2));platform_contiguous_free(contiguous);CHECK(!mac_resource_range_is_allocated(contiguous,1));}
        if(ordinary){CHECK(!mac_resource_range_is_allocated(ordinary,64));CHECK(mac_resource_register_owned_range(ordinary,64));CHECK(mac_resource_range_is_allocated(ordinary,64));CHECK(mac_resource_range_is_allocated((unsigned char*)ordinary+32,32));CHECK(!mac_resource_range_is_allocated((unsigned char*)ordinary+63,2));CHECK(!mac_resource_register_owned_range((unsigned char*)ordinary+32,8));mac_resource_unregister_owned_range(ordinary);CHECK(!mac_resource_range_is_allocated(ordinary,1));free(ordinary);CHECK(!mac_resource_range_is_allocated(ordinary,1));}
    }
    CHECK(halo_screen_commit()==640);halo_screen_ui_offset(1);CHECK(mac_engine_ui_offset()==0);
    CHECK(mac_device_state_start());CHECK(!mac_device_state_start());
    CHECK(mac_device_headless_identity()==NULL);
    s=mac_device_state_get();CHECK(s!=NULL);
    CHECK(D3D__RenderState[D3DRS_ZENABLE]==TRUE);
    CHECK(D3D__RenderState[D3DRS_STENCILMASK]==255);
    CHECK(D3D__TextureState[3][D3DTSS_ADDRESSU]==D3DTADDRESS_WRAP);
    for(i=0;i<D3DTS_MAX;++i){D3DDevice_GetTransform((D3DTRANSFORMSTATETYPE)i,&transform);CHECK(transform.m[0][0]==1.0f&&transform.m[1][1]==1.0f&&transform.m[2][2]==1.0f&&transform.m[3][3]==1.0f);}
    memset(&transform,0,sizeof(transform));transform._11=2.0f;transform._24=-3.5f;transform._42=7.25f;transform._44=9.0f;
    D3DDevice_SetTransform(D3DTS_WORLD,&transform);memset(&readback,0,sizeof(readback));
    D3DDevice_GetTransform(D3DTS_WORLD,&readback);CHECK(!memcmp(&transform,&readback,sizeof(transform)));
    D3DDevice_GetTransform(D3DTS_VIEW,&readback);CHECK(readback._11==1.0f&&readback._44==1.0f&&readback._24==0.0f);
    readback._11=43.0f;D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)-1,NULL);
    D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)D3DTS_MAX,NULL);
    D3DDevice_GetTransform((D3DTRANSFORMSTATETYPE)-1,NULL);D3DDevice_GetTransform((D3DTRANSFORMSTATETYPE)D3DTS_MAX,NULL);
    D3DDevice_GetTransform((D3DTRANSFORMSTATETYPE)-1,&readback);CHECK(readback._11==43.0f);
    D3DDevice_SetRenderState_CullMode(D3DCULL_NONE);CHECK(D3D__RenderState[D3DRS_CULLMODE]==D3DCULL_NONE);
    D3DDevice_SetRenderState_Deferred(D3DRS_FOGCOLOR,0x10203040);CHECK(D3D__RenderState[D3DRS_FOGCOLOR]==0x10203040);
    D3DDevice_SetRenderStateNotInline(D3DRS_ZBIAS,8);
    memcpy(&slope,&D3D__RenderState[D3DRS_POLYGONOFFSETZSLOPESCALE],4);
    memcpy(&offset,&D3D__RenderState[D3DRS_POLYGONOFFSETZOFFSET],4);
    CHECK(slope==-2.0f);CHECK(offset==-8.0f);CHECK(D3D__RenderState[D3DRS_SOLIDOFFSETENABLE]==1);
    D3DDevice_SetRenderState_ZBias(0);CHECK(D3D__RenderState[D3DRS_SOLIDOFFSETENABLE]==0);
    D3DDevice_SetTextureState_Deferred(2,D3DTSS_BORDERCOLOR,0xaabbccdd);CHECK(D3D__TextureState[2][D3DTSS_BORDERCOLOR]==0xaabbccdd);
    D3DDevice_SetTexture(2,(D3DBaseTexture*)&texture);D3DDevice_SetPalette(2,&palette);
    CHECK(s->textures[2]==(D3DBaseTexture*)&texture);CHECK(s->palettes[2]==&palette);
    D3DDevice_SetTexture(99,NULL);CHECK(s->textures[2]==(D3DBaseTexture*)&texture);
    memset(&vb,0,sizeof(vb));vb.Data=0x123450;D3DDevice_SetStreamSource(15,&vb,32);
    CHECK(s->streams[15].data==vb.Data);CHECK(s->streams[15].stride==32);
    D3DDevice_SetStreamSource(16,NULL,0);CHECK(s->streams[15].stride==32);
    memset(&ib,0,sizeof(ib));ib.Data=(DWORD)indices;D3DDevice_SetIndices(&ib,17);
    CHECK(D3D__IndexData==indices);CHECK(s->base_vertex_index==17);
    D3DDevice_SetIndices(NULL,0);CHECK(D3D__IndexData==NULL);
    for(i=0;i<8;++i)((float*)values)[i]=(float)i+1.0f;
    D3DDevice_SetVertexShaderConstant(-96,values,2);CHECK(!memcmp(s->constants,values,sizeof(values)));
    serial=mac_device_state_constant_serial();CHECK(serial==2);
    D3DDevice_SetVertexShaderConstant(-96,values,2);CHECK(mac_device_state_constant_serial()==serial);
    D3DDevice_SetVertexShaderConstant(95,values,2);CHECK(!memcmp(s->constants[191],values[0],16));
    serial=mac_device_state_constant_serial();D3DDevice_SetVertexShaderConstant(96,values,1);
    D3DDevice_SetVertexShaderConstant(-97,values,1);CHECK(mac_device_state_constant_serial()==serial);
    memset(&ps,0,sizeof(ps));ps.PSAlphaInputs[7]=0x12345678;ps.PSTextureModes=0x3456;
    D3DDevice_SetPixelShaderProgram(&ps);CHECK(D3D__RenderState[D3DRS_PSALPHAINPUTS0+7]==0x12345678);
    CHECK(D3D__RenderState[D3DRS_PSTEXTUREMODES]==0x3456);
    D3DDevice_InsertCallback((D3DCALLBACKTYPE)0,callback,0xfedcba98);CHECK(callback_count==1);CHECK(callback_context==0xfedcba98);
    D3DDevice_InsertCallback((D3DCALLBACKTYPE)0,NULL,0);CHECK(callback_count==1);
    mac_device_state_stop();CHECK(mac_device_state_get()==NULL);CHECK(mac_device_state_start());
    CHECK(mac_device_state_constant_serial()==0);CHECK(mac_device_state_get()->textures[2]==NULL);
    CHECK(D3D__RenderState[D3DRS_CULLMODE]==D3DCULL_CCW);
    D3DDevice_GetTransform(D3DTS_WORLD,&readback);CHECK(readback._11==1.0f&&readback._22==1.0f&&readback._33==1.0f&&readback._44==1.0f);
    mac_device_state_stop();
    CHECK(!mac_device_headless_start(0,480));CHECK(mac_device_state_get()==NULL);
    CHECK(mac_device_headless_start(640,480));s=mac_device_state_get();
    CHECK(mac_device_headless_identity()==(D3DDevice *)s);
    CHECK(s->headless);CHECK(mac_resource_allocation_count()==2);
    CHECK(s->viewport.Width==640&&s->viewport.Height==480);
    CHECK(s->viewport_scale[0]==320.0f&&s->viewport_scale[1]==-240.0f);
    CHECK(s->constants[58][0]==320.0f&&s->constants[59][1]==240.0f);
    {
        D3DSurface *back;D3DVIEWPORT8 viewport;DWORD shader;UINT size,samples=123;ULONGLONG stamp=123;
        DWORD code[5]={1UL<<16,0x12345678,0xabcdef01,0x76543210,0x0fedcba9};
        unsigned char *pixels=PLATFORM_PHYSICAL_TO_VIRTUAL(s->back_buffer.Data);
        D3DDevice_GetBackBuffer(0,(D3DBACKBUFFER_TYPE)0,&back);CHECK(back==&s->back_buffer);
        CHECK((back->Common&D3DCOMMON_REFCOUNT_MASK)==2);
        CHECK(D3DResource_Release((D3DResource*)back)==1);CHECK(mac_resource_allocation_count()==2);
        viewport=s->viewport;viewport.Width=320;viewport.Height=240;viewport.MinZ=0.25f;viewport.MaxZ=0.75f;
        D3DDevice_SetViewport(&viewport);CHECK(s->constants[58][0]==160.0f);
        D3DDevice_SetRenderTarget(back,NULL);CHECK(s->viewport.Width==640&&s->depth_stencil==NULL);
        CHECK(D3DDevice_CreateVertexShader(NULL,code,&shader,0)==S_OK);
        D3DDevice_GetVertexShaderSize(shader,&size);CHECK(size==1);
        D3DDevice_SetVertexShader(shader);CHECK((DWORD)s->vertex_shader==shader);
        D3DDevice_LoadVertexShader(shader,135);CHECK((DWORD)s->program_slots[135]==shader);
        D3DDevice_SelectVertexShader(shader,135);CHECK(s->program_address==135);
        D3DDevice_SelectVertexShader(0,136);CHECK(s->program_address==135);
        D3DDevice_GetVertexShaderSize(1,&size);CHECK(size==0);
        D3DDevice_Begin(D3DPT_TRIANGLELIST);
        D3DDevice_SetVertexDataColor(3,0xff804020);D3DDevice_SetVertexData2f(0,1.0f,2.0f);
        D3DDevice_SetVertexData2s(0,3,4);D3DDevice_SetVertexData4f(0,5,6,7,1);
        CHECK(s->immediate_count==3);CHECK(s->immediate_vertices[0]==1.0f);
        CHECK(s->immediate_vertices[64]==3.0f&&s->immediate_vertices[128+2]==7.0f);
        CHECK(s->immediate_vertices[12]==128.0f/255.0f&&s->immediate_vertices[15]==1.0f);
        D3DDevice_End();CHECK(!s->immediate_active&&s->skipped_draws==1);
        pixels[0]=0x5a;D3DDevice_Clear(0,NULL,0,0,1,0);CHECK(pixels[0]==0x5a&&s->skipped_clears==1);
        D3DDevice_DrawVertices(D3DPT_TRIANGLELIST,0,3);D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLELIST,3,indices);
        CHECK(s->skipped_draws==3);
        D3DDevice_BeginVisibilityTest();CHECK(D3DDevice_EndVisibilityTest(0)==S_OK);
        CHECK(D3DDevice_GetVisibilityTestResult(0,&samples,&stamp)==S_OK);CHECK(samples==0&&stamp==0);
        D3DDevice_DeleteVertexShader(shader);
    }
    mac_device_state_stop();CHECK(mac_resource_allocation_count()==0);
    CHECK(mac_device_state_get()==NULL);CHECK(mac_device_headless_identity()==NULL);
    CHECK(mac_device_headless_start(320,240));CHECK(mac_device_state_get()->owned_shaders==NULL);
    CHECK(mac_device_headless_identity()==(D3DDevice *)mac_device_state_get());
    mac_device_state_stop();CHECK(mac_resource_allocation_count()==0);
    CHECK(mac_device_headless_identity()==NULL);
    {
        int mode=0;unsigned long pitch;unsigned int *pixels;
        D3DPRESENT_PARAMETERS parameters;D3DDevice *created=NULL;
        memset(&parameters,0,sizeof(parameters));
        parameters.BackBufferWidth=16;parameters.BackBufferHeight=12;
        parameters.BackBufferFormat=D3DFMT_A8R8G8B8;parameters.SwapEffect=D3DSWAPEFFECT_DISCARD;
        parameters.Flags=D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;parameters.EnableAutoDepthStencil=TRUE;
        parameters.AutoDepthStencilFormat=D3DFMT_D24S8;
        CHECK(Direct3DCreate8(D3D_SDK_VERSION)!=NULL);
        CHECK(Direct3D_CreateDevice(0,D3DDEVTYPE_HAL,NULL,D3DCREATE_HARDWARE_VERTEXPROCESSING,&parameters,&created)==S_OK);
        s=mac_device_state_get();CHECK(s&&s->gpu&&!s->headless);
        if(argc==2){mode=14;mac_device_gpu_frame(clear_frame,&mode);return 0;}
        if(retail_dxt_gpu_only) {
            mode=23;CHECK(mac_device_gpu_frame(clear_frame,&mode));
            mac_device_state_stop();
            mac_gpu_textures_cache_get_stats(&texture_cache_stats);
            mac_native_bitmap_cache_unbind();mac_native_bitmap_tag_free(&bitmap_tag);
            free(map_bytes);free(pixel_bytes);
            printf("native DXT GPU readback: %u checks, %u failures\\n",checks,failures);
            return failures?1:0;
        }
        if(retail_cache_only) {
            struct bitmap_data *bitmap;
            int decoded=mac_native_bitmap_decode(&retail,0xe7c2064eU,pixel_bytes,
                pixel_size,&volume_tag,&asset_error);
            CHECK(decoded);
            if(decoded) {
                bitmap=(struct bitmap_data *)volume_tag.root->bitmaps.address;
                CHECK(bitmap&&bitmap->type==1&&bitmap->format==11&&bitmap->width==32&&
                    bitmap->height==32&&bitmap->depth==32);
                if(bitmap&&bitmap->type==1&&bitmap->format==11&&bitmap->width==32&&
                    bitmap->height==32&&bitmap->depth==32) {
                    size_t bytes,texel;
                    int native_match=1;
                    volume_width=bitmap->width;volume_height=bitmap->height;
                    volume_depth=bitmap->depth;volume_to_upload=bitmap->hardware_format;
                    bytes=(size_t)volume_width*volume_height*volume_depth*4;
                    expected_volume_rgba=malloc(bytes);CHECK(expected_volume_rgba!=NULL);
                    CHECK(bitmap->pixels_offset<=pixel_size&&bytes<=pixel_size-bitmap->pixels_offset);
                    if(expected_volume_rgba&&bitmap->base_address&&
                        bitmap->pixels_offset<=pixel_size&&
                        bytes<=pixel_size-bitmap->pixels_offset) {
                        const unsigned char *source=pixel_bytes+bitmap->pixels_offset;
                        const uint32_t *native=(const uint32_t *)bitmap->base_address;
                        for(texel=0;texel<bytes;texel+=4) {
                            uint32_t argb=(uint32_t)source[texel]|(uint32_t)source[texel+1]<<8|
                                (uint32_t)source[texel+2]<<16|(uint32_t)source[texel+3]<<24;
                            expected_volume_rgba[texel]=source[texel+2];
                            expected_volume_rgba[texel+1]=source[texel+1];
                            expected_volume_rgba[texel+2]=source[texel];
                            expected_volume_rgba[texel+3]=source[texel+3];
                            if(native[texel/4]!=argb)native_match=0;
                        }
                        CHECK(native_match);
                        mac_native_bitmap_cache_unbind();
                        CHECK(mac_native_bitmap_cache_bind(&volume_tag,1));
                        mode=20;CHECK(mac_device_gpu_frame(clear_frame,&mode));
                    } else {
                        CHECK(0);
                    }
                    free(expected_volume_rgba);expected_volume_rgba=NULL;
                }
            }
            mac_device_state_stop();
            mac_gpu_textures_cache_get_stats(&texture_cache_stats);
            CHECK(texture_cache_stats.entries==0&&texture_cache_stats.bytes==0);
            mac_native_bitmap_cache_unbind();
            mac_native_bitmap_tag_free(&volume_tag);
            if(bitmap_tag.owner)mac_native_bitmap_tag_free(&bitmap_tag);
            free(map_bytes);free(pixel_bytes);
            printf("native retail GPU texture cache: %u checks, %u failures\\n",checks,failures);
            return failures?1:0;
        }
        CHECK(mac_device_gpu_identity()==(D3DDevice *)s&&mac_device_headless_identity()==NULL);
        CHECK(s&&s->gpu_width==16&&s->gpu_height==12&&s->viewport.Width==16&&s->viewport.Height==12);
        CHECK(halo_screen_commit()==640);halo_screen_ui_offset(1);CHECK(mac_engine_ui_offset()==0);
        CHECK(s&&mac_resource_allocation_count()==2);
        CHECK(mac_device_gpu_frame(clear_frame,&mode));s=mac_device_state_get();
        CHECK(s&&s->gpu_present_count==1);
        CHECK(observed_depth>0.24f&&observed_depth<0.26f);
        pitch=(16UL*4+D3DTEXTURE_PITCH_ALIGNMENT-1)&~(unsigned long)(D3DTEXTURE_PITCH_ALIGNMENT-1);
        pixels=(unsigned int *)PLATFORM_PHYSICAL_TO_VIRTUAL(s->back_buffer.Data);
        CHECK(pixels[0]==0xff123456&&pixels[(pitch/4)*11+15]==0xff123456);
        mode=1;CHECK(mac_device_gpu_frame(clear_frame,&mode));s=mac_device_state_get();
        CHECK(s&&s->gpu_present_count==2);
        CHECK(pixels[(pitch/4)*3+2]==0xffabcdef&&pixels[(pitch/4)*6+4]==0xffabcdef);
        CHECK(pixels[(pitch/4)*2+2]==0xff123456&&pixels[(pitch/4)*3+5]==0xff123456);
        mode=2;CHECK(mac_device_gpu_frame(clear_frame,&mode));s=mac_device_state_get();
        CHECK(s&&s->gpu_present_count==3&&s->gpu_immediate_draw_count==1);
        if(argc==3){mode=3;CHECK(mac_device_gpu_frame(clear_frame,&mode));s=mac_device_state_get();
            CHECK(s&&s->gpu_present_count==4&&s->gpu_immediate_draw_count==2);}
        mode=4;CHECK(mac_device_gpu_frame(clear_frame,&mode));s=mac_device_state_get();
        CHECK(s&&s->gpu_present_count==(argc==3?5:4));CHECK(s&&s->gpu_indexed_draw_count==2);
        CHECK(s&&s->gpu_stream_draw_active==0);
        if(argc==3){mode=5;CHECK(mac_device_gpu_frame(clear_frame,&mode));s=mac_device_state_get();
            CHECK(s&&s->gpu_present_count==6&&s->gpu_indexed_draw_count==3);
            mode=6;CHECK(mac_device_gpu_frame(clear_frame,&mode));s=mac_device_state_get();
            CHECK(s&&s->gpu_present_count==7&&s->gpu_indexed_draw_count==11);}
        mode=13;CHECK(mac_device_gpu_frame(clear_frame,&mode));s=mac_device_state_get();
        CHECK(s&&s->gpu_present_count==(argc==3?8:5));
        CHECK(s&&s->gpu_indexed_draw_count==(argc==3?11:2));
        for(mode=7;mode<=12;++mode)CHECK(mac_device_gpu_frame(clear_frame,&mode));
        for(mode=15;mode<=16;++mode)CHECK(mac_device_gpu_frame(clear_frame,&mode));
        mode=21;CHECK(mac_device_gpu_frame(clear_frame,&mode));
        mode=22;CHECK(mac_device_gpu_frame(clear_frame,&mode));
        if(argc==3)for(mode=17;mode<=19;++mode)CHECK(mac_device_gpu_frame(clear_frame,&mode));
        if(argc==3) {
            int decoded=mac_native_bitmap_decode(&retail,0xe7c2064eU,pixel_bytes,pixel_size,&volume_tag,&asset_error);
            CHECK(decoded);
            if(!decoded)fprintf(stderr,"volume fixture: %s\n",asset_error?asset_error:"unknown");
            else {
                struct bitmap_data *bitmap=volume_tag.root->bitmaps.address;
                size_t bytes,texel;int native_match=1;
                CHECK(volume_tag.root->bitmaps.count==1&&bitmap->type==1&&bitmap->format==11);
                volume_width=bitmap->width;volume_height=bitmap->height;volume_depth=bitmap->depth;
                CHECK(volume_width==32&&volume_height==32&&volume_depth==32);
                bytes=(size_t)volume_width*volume_height*volume_depth*4;
                expected_volume_rgba=malloc(bytes);CHECK(expected_volume_rgba!=NULL);
                CHECK(bitmap->pixels_offset<=pixel_size&&bytes<=pixel_size-bitmap->pixels_offset);
                if(expected_volume_rgba&&bitmap->pixels_offset<=pixel_size&&bytes<=pixel_size-bitmap->pixels_offset) {
                    const unsigned char *source=pixel_bytes+bitmap->pixels_offset;
                    const uint32_t *native=(const uint32_t *)bitmap->base_address;
                    for(texel=0;texel<bytes;texel+=4) {
                        uint32_t argb=(uint32_t)source[texel]|(uint32_t)source[texel+1]<<8|
                            (uint32_t)source[texel+2]<<16|(uint32_t)source[texel+3]<<24;
                        expected_volume_rgba[texel]=source[texel+2];expected_volume_rgba[texel+1]=source[texel+1];
                        expected_volume_rgba[texel+2]=source[texel];expected_volume_rgba[texel+3]=source[texel+3];
                        if(native[texel/4]!=argb)native_match=0;
                    }
                    CHECK(native_match);
                    mac_native_bitmap_cache_unbind();CHECK(mac_native_bitmap_cache_bind(&volume_tag,1));
                    volume_to_upload=bitmap->hardware_format;
                    mode=20;CHECK(mac_device_gpu_frame(clear_frame,&mode));
                }
                free(expected_volume_rgba);expected_volume_rgba=NULL;
            }
        }
        {
            unsigned before=callback_count;
            D3DDevice_SetVerticalBlankCallback(callback);D3DDevice_BlockUntilVerticalBlank();
            D3DDevice_BlockUntilVerticalBlank();
            CHECK(callback_count>before&&*d3d_find_flipcount()>0);
        }
        mac_device_state_stop();CHECK(mac_resource_allocation_count()==0);
        mac_gpu_textures_cache_get_stats(&texture_cache_stats);
        CHECK(texture_cache_stats.entries==0&&texture_cache_stats.bytes==0);
    }
    if(bitmap_tag.owner){mac_native_bitmap_cache_unbind();mac_native_bitmap_tag_free(&bitmap_tag);}
    mac_native_bitmap_tag_free(&volume_tag);
    free(map_bytes);free(pixel_bytes);
    printf("native device state: %u checks, %u failures (headless state plus real NV2A-to-GLSL120 immediate draw/readback/present%s)\n",checks,failures,argc==3?"/native-texture":"");
    return failures?1:0;
}
