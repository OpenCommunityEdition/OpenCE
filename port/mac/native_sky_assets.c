#include "cseries.h"
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
#include "scenario/sky_definitions.h"
#include "models/model_definitions.h"
#include "models/model_animation_definitions.h"
#include "bitmaps/bitmap_group.h"
#include "native_sky_assets.h"
#include "native_effect_assets.h"
#include "native_sky_tags.h"
#include "native_model_tags.h"
#include "native_animation_tags.h"
#undef malloc
#undef calloc
#undef free
#undef memcpy
#undef memset
#include <stdlib.h>
#include <string.h>

#define SKY UINT32_C(0x736b7920)
#define MODE UINT32_C(0x6d6f6465)
#define ANTR UINT32_C(0x616e7472)
#define LENS UINT32_C(0x6c656e73)
#define BITM UINT32_C(0x6269746d)
#define NONE32 UINT32_MAX
#define LENS_REFLECTION_MAX 32U
#define LENS_OWNER_MAX (1024U*1024U)
#define WITNESS(name,test) typedef char name[(test)?1:-1]

struct native_sky_asset_root {
    struct tag_reference model,animation_graph;
    byte pad20[0x38];
    struct sky_atmospheric_fog outdoor_fog,indoor_fog;
    struct tag_reference indoor_fog_screen;
    long unusedA8;
    struct tag_block render_model_regions,animations,lights;
};
struct native_lens_reflection {
    word flags;short type,bitmap_index;word pad06;byte reserved08[0x14];
    real offset,rotation_offset;byte reserved24[4];
    real radius_lower_bound,radius_upper_bound;short radius_scale_function;word pad32;
    real brightness_lower_bound,brightness_upper_bound;short brightness_scale_function;word pad3E;
    real_argb_color tint_color,animation_color_lower_bound,animation_color_upper_bound;
    word animation_flags;short animation_function;real animation_period,animation_phase;byte reserved7C[4];
};
struct native_lens_root {
    real falloff_angle,cutoff_angle,runtime_cosine_falloff_angle,runtime_cosine_cutoff_angle;
    real occlusion_radius;short occlusion_offset_direction;word pad16;
    real near_fade_distance,far_fade_distance;struct tag_reference primary_map;
    word flags,pad32;byte reserved34[0x4c];
    short corona_rotation_function;word pad82;real corona_rotation_function_scale;
    byte reserved88[0x18];real_vector2d corona_radius_scale;byte reservedA8[0x1c];
    struct tag_block reflections;byte reservedD0[0x20];
};
struct lens_owner { struct native_lens_root root;struct native_lens_reflection *reflections;char *primary_name;size_t bytes; };

WITNESS(sky_asset_root_size,sizeof(struct native_sky_asset_root)==0xd0);
WITNESS(lens_reflection_size,sizeof(struct native_lens_reflection)==0x80);
WITNESS(lens_reflection_bitmap_offset,offsetof(struct native_lens_reflection,bitmap_index)==4);
WITNESS(lens_root_reference_offset,offsetof(struct native_lens_root,primary_map)==0x20);
WITNESS(lens_root_reflections_offset,offsetof(struct native_lens_root,reflections)==0xc4);
WITNESS(lens_root_size,sizeof(struct native_lens_root)==0xf0);

static uint32_t le32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t le16(const unsigned char *p)
{ return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
static int finite_bits(uint32_t v) { return (v&UINT32_C(0x7fffffff))<UINT32_C(0x7f800000); }
static int real_at(const unsigned char *p,real *out)
{ uint32_t v=le32(p);if(!finite_bits(v))return 0;memcpy(out,&v,4);return 1; }
static int copy_ref(const struct mac_retail_cache *cache,const unsigned char *raw,
    uint32_t expected,struct tag_reference *out,char **owned_name,int required)
{
    uint32_t group=le32(raw),name_address=le32(raw+4),name_length=le32(raw+8),handle=le32(raw+12);
    struct mac_retail_tag tag;size_t n;
    out->group_tag=group;out->name=NULL;out->name_length=(long)name_length;
    out->index=0;memcpy(&out->index,&handle,4);
    if(group!=expected || name_length>255)return 0;
    if(handle==NONE32)return !required&&!name_address&&!name_length;
    if((handle&65535)>=cache->tag_count||!mac_retail_tag_get(cache,handle&65535,&tag)||
       tag.handle!=handle||tag.group!=expected||!name_address||
       name_address!=le32(cache->tags+cache->instances_offset+(handle&65535)*32+16))return 0;
    for(n=0;n<256&&tag.name[n];++n){}
    if(n==256||(name_length&&name_length!=n))return 0;
    *owned_name=(char *)malloc(n+1);if(!*owned_name)return 0;
    memcpy(*owned_name,tag.name,n+1);out->name=*owned_name;out->name_length=(long)n;return 1;
}
static void lens_free(void *opaque)
{
    struct lens_owner *owner=(struct lens_owner *)opaque;
    if(!owner)return;free(owner->primary_name);free(owner->reflections);free(owner);
}
static int convert_reflection(const unsigned char *raw,struct native_lens_reflection *out)
{
    unsigned i;uint16_t v;
    v=le16(raw);memcpy(&out->flags,&v,2);v=le16(raw+2);memcpy(&out->type,&v,2);
    v=le16(raw+4);memcpy(&out->bitmap_index,&v,2);v=le16(raw+6);memcpy(&out->pad06,&v,2);
    for(i=0;i<2;++i)if(!real_at(raw+0x20+i*4,(&out->offset)+i))return 0;
    for(i=0;i<2;++i)if(!real_at(raw+0x28+i*4,(&out->radius_lower_bound)+i))return 0;
    v=le16(raw+0x30);memcpy(&out->radius_scale_function,&v,2);v=le16(raw+0x32);memcpy(&out->pad32,&v,2);
    for(i=0;i<2;++i)if(!real_at(raw+0x34+i*4,(&out->brightness_lower_bound)+i))return 0;
    v=le16(raw+0x3c);memcpy(&out->brightness_scale_function,&v,2);v=le16(raw+0x3e);memcpy(&out->pad3E,&v,2);
    for(i=0;i<12;++i)if(!real_at(raw+0x40+i*4,(real *)&out->tint_color+i))return 0;
    v=le16(raw+0x70);memcpy(&out->animation_flags,&v,2);v=le16(raw+0x72);memcpy(&out->animation_function,&v,2);
    if(!real_at(raw+0x74,&out->animation_period)||!real_at(raw+0x78,&out->animation_phase))return 0;
    return 1;
}
static int convert_lens(const struct mac_retail_cache *cache,uint32_t handle,
    const void *bitmaps,size_t bitmap_bytes,struct mac_actor_assets *assets,const char **error)
{
    struct mac_retail_tag tag;const unsigned char *raw,*rows;uint32_t count,address,i;
    struct lens_owner *owner;struct native_lens_root *root;struct tag_reference *map;
    struct bitmap_group *bitmap_root;uint32_t bitmap_handle;
    if(error)*error="native sky lens flare fields or bitmap dependency unsupported";
    if(mac_native_tags_group(&assets->tags,handle)!=NONE32)
        return mac_native_tags_group(&assets->tags,handle)==LENS;
    if(!mac_retail_tag_get(cache,handle&65535,&tag)||tag.handle!=handle||tag.group!=LENS)goto bad;
    raw=mac_retail_tag_span(cache,tag.address,sizeof(struct native_lens_root));if(!raw)goto bad;
    count=le32(raw+0xc4);address=le32(raw+0xc8);
    if(count>LENS_REFLECTION_MAX || (count&&(!address||address==NONE32||(address&3))) ||
       (!count&&address&&address!=NONE32&&((address&3)||!mac_retail_tag_span(cache,address,1))))goto bad;
    rows=count?mac_retail_tag_span(cache,address,count*0x80):(const unsigned char *)1;
    if(!rows)goto bad;
    owner=(struct lens_owner *)calloc(1,sizeof(*owner));if(!owner)goto bad;
    owner->bytes=sizeof(*owner);root=&owner->root;memcpy(root,raw,sizeof(*root));
#define LENS_FAIL() do { lens_free(owner);goto bad; } while(0)
    {
        static const unsigned floats[]={0,4,8,12,16,24,28,132,160,164};
        unsigned j;uint32_t bits;
        for(j=0;j<sizeof(floats)/sizeof(floats[0]);++j) {
            bits=le32(raw+floats[j]);if(!finite_bits(bits))LENS_FAIL();memcpy((char *)root+floats[j],&bits,4);
        }
        { uint16_t v=le16(raw+20);memcpy(&root->occlusion_offset_direction,&v,2);
          v=le16(raw+48);memcpy(&root->flags,&v,2);v=le16(raw+128);memcpy(&root->corona_rotation_function,&v,2); }
        /* Recheck enum fields after converting from their actual little-endian words. */
        if(root->falloff_angle<0.f||root->cutoff_angle<0.f||root->occlusion_radius<0.f||
           root->near_fade_distance<0.f||root->far_fade_distance<root->near_fade_distance||
           root->occlusion_offset_direction<0||root->occlusion_offset_direction>2||
           root->corona_rotation_function<0||root->corona_rotation_function>4||
           (root->flags&~1U))LENS_FAIL();
        if(!copy_ref(cache,raw+0x20,BITM,&root->primary_map,&owner->primary_name,count!=0))LENS_FAIL();
        if(owner->primary_name)owner->bytes+=strlen(owner->primary_name)+1;
        root->reflections.count=(long)count;root->reflections.definition=NULL;root->reflections.address=NULL;
        owner->bytes+=count*sizeof(*owner->reflections);
        bitmap_root=NULL;
        if(root->primary_map.index!=NONE) {
            bitmap_handle=(uint32_t)root->primary_map.index;
            if(!mac_actor_assets_load_bitmap(cache,bitmaps,bitmap_bytes,assets,bitmap_handle,error))LENS_FAIL();
            bitmap_root=(struct bitmap_group *)mac_native_tags_get(&assets->tags,BITM,bitmap_handle,sizeof(*bitmap_root),error);
            if(!bitmap_root)LENS_FAIL();
        }
        if(count) {
            map=&root->primary_map;if((uint32_t)map->index==NONE32||!bitmap_root)LENS_FAIL();
            owner->reflections=(struct native_lens_reflection *)calloc(count,sizeof(*owner->reflections));
            if(!owner->reflections)LENS_FAIL();root->reflections.address=owner->reflections;
            for(i=0;i<count;++i) {
                const unsigned char *src=rows+(size_t)i*0x80;struct native_lens_reflection *dst=owner->reflections+i;
                memcpy(dst,src,sizeof(*dst));if(!convert_reflection(src,dst))LENS_FAIL();
                if((dst->bitmap_index!=-1&&(dst->bitmap_index<0||dst->bitmap_index>=bitmap_root->bitmaps.count))||
                   dst->radius_lower_bound<0.f||dst->radius_upper_bound<dst->radius_lower_bound||
                   dst->brightness_lower_bound<0.f||dst->brightness_upper_bound<dst->brightness_lower_bound||
                   dst->radius_scale_function<0||dst->radius_scale_function>3||
                   dst->brightness_scale_function<0||dst->brightness_scale_function>3||
                   dst->animation_period<0.f||dst->animation_function < -1||dst->animation_function>=12||
                   (dst->flags&~0x0fU)||(dst->animation_flags&~3U))LENS_FAIL();
                { unsigned c;const real *colors=(const real *)&dst->tint_color;
                  for(c=0;c<12;++c)if(colors[c]<0.f||colors[c]>1.f)LENS_FAIL(); }
            }
        }
        if(!mac_native_tags_publish(&assets->tags,handle,LENS,root,sizeof(*root),owner->bytes,owner,lens_free,error))LENS_FAIL();
    }
    ++assets->published_count;if(error)*error=NULL;return 1;
#undef LENS_FAIL
bad:
    if(error && !*error)*error="native sky lens flare fields or bitmap dependency unsupported";
    return 0;
}
static int validate_hierarchy(const struct mac_native_sky_tag *sky,struct mac_actor_assets *assets,const char **error)
{
    struct native_sky_asset_root *root=(struct native_sky_asset_root *)sky->root;
    struct model *model;struct animation_graph *graph=NULL;struct sky_animation_local { short animation_index;word pad;real period;byte unused[0x1c]; } *animations;
    uint32_t mh=(uint32_t)root->model.index,ah=(uint32_t)root->animation_graph.index;long i;
    /* (an indoor map's sky has no model, and then nothing to animate) */
    if(mh==NONE32) {
        if(ah!=NONE32 || root->animations.count)goto incompatible;
        return 1;
    }
    model=(struct model *)mac_native_tags_get(&assets->tags,MODE,mh,sizeof(*model),error);if(!model)return 0;
    if(ah!=NONE32) {
        struct animation *clips;
        graph=(struct animation_graph *)mac_native_tags_get(&assets->tags,ANTR,ah,sizeof(*graph),error);if(!graph)return 0;
        if(graph->nodes.count && graph->nodes.count!=model->nodes.count)goto incompatible;
        clips=graph->animations.address;animations=root->animations.address;
        if(root->animations.count>graph->animations.count)goto incompatible;
        for(i=0;i<root->animations.count;++i)if(animations[i].animation_index<0||
            animations[i].animation_index>=graph->animations.count||i>=graph->animations.count||
            clips[i].node_count!=model->nodes.count||
            (clips[i].node_list_checksum&&model->node_list_checksum&&
             clips[i].node_list_checksum!=model->node_list_checksum))goto incompatible;
    } else if(root->animations.count)goto incompatible;
    return 1;
incompatible:
    if(error)*error="sky model and animation graph hierarchy mismatch";return 0;
}
static int validate_published_sky(struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    struct native_sky_asset_root *root=(struct native_sky_asset_root *)mac_native_tags_get(
        &assets->tags,SKY,handle,sizeof(*root),error);
    struct native_sky_light_local { struct tag_reference lens;char marker[TAG_STRING_LENGTH+1];byte pad[0x37];real_euler_angles2d direction;byte tail[4]; } *lights;
    uint32_t i;
    if(!root)return 0;
    if(root->indoor_fog_screen.index!=NONE){if(error)*error="sky fog-screen resource is not supported";return 0;}
    {
        struct mac_native_sky_tag view;memset(&view,0,sizeof(view));view.root=root;
        if(!validate_hierarchy(&view,assets,error))return 0;
    }
    lights=(struct native_sky_light_local *)root->lights.address;
    for(i=0;i<(uint32_t)root->lights.count;++i)if(lights[i].lens.index!=NONE) {
        struct native_lens_root *lens=(struct native_lens_root *)mac_native_tags_get(
            &assets->tags,LENS,(uint32_t)lights[i].lens.index,sizeof(*lens),error);
        struct bitmap_group *bitmap=NULL;struct native_lens_reflection *reflection;long j;
        if(!lens)return 0;
        if(lens->primary_map.index!=NONE) {
            uint32_t map_handle=(uint32_t)lens->primary_map.index;
            bitmap=(struct bitmap_group *)mac_native_tags_get(&assets->tags,BITM,map_handle,sizeof(*bitmap),error);
            if(!bitmap)return 0;
        }
        if(lens->reflections.count) {
            if(!bitmap){if(error)*error="published sky lens lacks primary bitmap";return 0;}
            reflection=lens->reflections.address;
            for(j=0;j<lens->reflections.count;++j)
                if(reflection[j].bitmap_index!=-1&&
                   (reflection[j].bitmap_index<0||reflection[j].bitmap_index>=bitmap->bitmaps.count)) {
                    if(error)*error="published sky lens bitmap index out of range";return 0;
                }
        }
    }
    if(error)*error=NULL;return 1;
}
int mac_native_sky_assets_load(const struct mac_retail_cache *cache,const void *bitmaps,size_t bitmap_bytes,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    struct mac_native_sky_tag staging;struct native_sky_asset_root *root;
    struct native_sky_light_local { struct tag_reference lens;char marker[TAG_STRING_LENGTH+1];byte pad[0x37];real_euler_angles2d direction;byte tail[4]; } *light;
    const char *why=NULL;uint32_t i;int result;
    if(error)*error="native sky asset arguments";
    if(!cache||!assets||!assets->tags.state||handle==NONE32)return 0;
    if(mac_native_tags_group(&assets->tags,handle)!=NONE32) {
        if(mac_native_tags_group(&assets->tags,handle)!=SKY) {if(error)*error="native sky handle already has another family";return 0;}
        return validate_published_sky(assets,handle,error);
    }
    if(!mac_native_sky_decode(cache,handle,&staging,&why)){if(error)*error=why;return 0;}
    root=(struct native_sky_asset_root *)staging.root;
    if(root->indoor_fog_screen.index!=NONE &&
       !mac_native_effect_assets_load(cache,bitmaps,bitmap_bytes,NULL,0,assets,
           (uint32_t)root->indoor_fog_screen.index,error)) {mac_native_sky_tag_free(&staging);return 0;}
    if(root->model.index!=NONE &&
       !mac_actor_assets_load_model(cache,assets,(uint32_t)root->model.index,error)) {mac_native_sky_tag_free(&staging);return 0;}
    if(root->animation_graph.index!=NONE && !mac_actor_assets_load_animation(cache,assets,(uint32_t)root->animation_graph.index,error)) {
        mac_native_sky_tag_free(&staging);return 0;
    }
    if(!validate_hierarchy(&staging,assets,error)){mac_native_sky_tag_free(&staging);return 0;}
    light=(struct native_sky_light_local *)root->lights.address;
    for(i=0;i<(uint32_t)root->lights.count;++i) {
        if(light[i].lens.index!=NONE && !convert_lens(cache,(uint32_t)light[i].lens.index,bitmaps,bitmap_bytes,assets,error)) {
            mac_native_sky_tag_free(&staging);return 0;
        }
    }
    mac_native_sky_tag_free(&staging);
    result=mac_native_sky_publish(cache,&assets->tags,handle,error);
    if(result) {
        struct native_sky_asset_root *published=(struct native_sky_asset_root *)mac_native_tags_get(
            &assets->tags,SKY,handle,sizeof(*published),error);
        if(!published)return 0;
        /* The engine's native model-reference family is `mode`; the cache
           stores the render-model leaf as `mod2`. */
        if(published->model.index!=NONE)published->model.group_tag=MODE;
        ++assets->published_count;
    }
    if(result&&error)*error=NULL;return result;
}
