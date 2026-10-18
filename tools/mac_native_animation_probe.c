#include "cseries.h"
#include "models/model_animation_definitions.h"
#include "native_animation_tags.h"
#include "retail_animation_graph.h"
#include "native_tags.h"
#include "engine_tags.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef memcmp
#undef strcmp
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s\n",#x);}}while(0)
static uint32_t digest=UINT32_C(2166136261);
struct animation_sound_probe { struct tag_reference sound;long unused; };
typedef char animation_sound_probe_size[sizeof(struct animation_sound_probe)==20?1:-1];
struct sound_visit_state { unsigned calls;int invalid,fail; };
static int visit_sound(void *opaque,uint32_t group,uint32_t handle)
{
    struct sound_visit_state *state=opaque;
    ++state->calls;
    if(group!=UINT32_C(0x736e6421)||handle==UINT32_MAX)state->invalid=1;
    return !state->fail;
}
static void check_sound_visitor(struct animation_graph *graph)
{
    struct animation_graph malformed;
    struct animation_graph salted;
    struct sound_visit_state state;
    struct animation_sound_probe *sounds=graph->sound_references.address;
    struct animation_sound_probe one_sound;
    unsigned i,expected=0,first=UINT_MAX;long saved;
    uint32_t salted_bits=UINT32_C(0xe76205ee);int32_t salted_index;
    for(i=0;i<(unsigned)graph->sound_references.count;++i)if(sounds[i].sound.index!=-1){++expected;if(first==UINT_MAX)first=i;}
    memset(&state,0,sizeof(state));
    CHECK(mac_native_animation_visit_sounds(graph,visit_sound,&state));
    CHECK(!state.invalid&&state.calls==expected);
    if(first!=UINT_MAX) {
        saved=sounds[first].sound.index;sounds[first].sound.index=-1;
        memset(&state,0,sizeof(state));
        CHECK(mac_native_animation_visit_sounds(graph,visit_sound,&state));
        CHECK(!state.invalid&&state.calls+1==expected);
        sounds[first].sound.index=saved;
        memset(&state,0,sizeof(state));state.fail=1;
        CHECK(!mac_native_animation_visit_sounds(graph,visit_sound,&state));
        CHECK(state.calls==1);
    }
    malformed=*graph;malformed.sound_references.count=258;
    CHECK(!mac_native_animation_visit_sounds(&malformed,visit_sound,&state));
    malformed=*graph;malformed.sound_references.address=NULL;
    CHECK(!mac_native_animation_visit_sounds(&malformed,visit_sound,&state));
    CHECK(!mac_native_animation_visit_sounds(graph,NULL,&state));
    memset(&one_sound,0,sizeof(one_sound));memset(&salted,0,sizeof(salted));
    memcpy(&salted_index,&salted_bits,sizeof(salted_bits));
    one_sound.sound.group_tag=UINT32_C(0x736e6421);one_sound.sound.index=(long)salted_index;
    salted.sound_references.count=1;salted.sound_references.address=&one_sound;
    memset(&state,0,sizeof(state));
    CHECK(mac_native_animation_visit_sounds(&salted,visit_sound,&state));
    CHECK(!state.invalid&&state.calls==1);
}
static void hash_word(uint32_t v) { digest=(digest^v)*UINT32_C(16777619); }
static void stream_digest(const struct animation *a,const unsigned char *p,int animated)
{
    const unsigned long *masks[3];
    static const unsigned sizes[3]={8,12,4};
    unsigned i,j,k,offset=0;uint16_t half;uint32_t bits;
    masks[0]=a->nodes_with_rotation_flags;masks[1]=a->nodes_with_translation_flags;masks[2]=(const unsigned long *)a->nodes_with_scale_flags;
    for(i=0;i<(unsigned)a->node_count;++i)for(j=0;j<3;++j) {
        if((int)((masks[j][i/32]>>(i%32))&1)!=animated)continue;
        if(!j)for(k=0;k<8;k+=2){memcpy(&half,p+offset+k,2);hash_word(half);}
        else for(k=0;k<sizes[j];k+=4){memcpy(&bits,p+offset+k,4);hash_word(bits);}
        offset+=sizes[j];
    }
}

static uint32_t u32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void reject(struct mac_retail_cache *c,uint32_t h,unsigned char *p,unsigned size,uint32_t value)
{
    unsigned char saved[4];unsigned i;struct mac_native_animation_tag a,z;const char *e;int ok;
    memcpy(saved,p,size);for(i=0;i<size;++i)p[i]=(unsigned char)(value>>(i*8));
    memset(&a,0xa5,sizeof(a));memset(&z,0,sizeof(z));ok=mac_native_animation_decode(c,h,&a,&e);
    CHECK(!ok && !memcmp(&a,&z,sizeof(a)));mac_native_animation_tag_free(&a);memcpy(p,saved,size);
}
static void extra_graph(struct mac_retail_cache *cache,struct mac_native_tags *registry,uint32_t h,unsigned clips,unsigned nodes)
{
    struct mac_native_animation_tag owned;struct mac_retail_animation_graph portable;
    struct animation_graph *g;struct mac_retail_tag tag;unsigned char *raw,*list;const char *error;unsigned i,j;uint32_t bits,saved_digest=digest;
    int ok=mac_native_animation_decode(cache,h,&owned,&error);CHECK(ok);
    if(!ok){fprintf(stderr,"extra animation %08lx: %s\n",(unsigned long)h,error);return;}
    ok=mac_retail_animation_graph_decode(cache,h,&portable,&error);CHECK(ok);
    if(!ok){mac_native_animation_tag_free(&owned);return;}
    g=(struct animation_graph *)owned.root;CHECK(g->animations.count==(long)clips && g->nodes.count==(long)nodes);
    digest=2166136261U;
    for(i=0;i<clips;++i) {
        struct animation *a=(struct animation *)g->animations.address+i;struct mac_retail_animation *d=portable.animations+i;
        hash_word(a->node_list_checksum);hash_word(a->frame_count);hash_word(a->node_count);hash_word(a->frame_size);
        for(j=0;j<2;++j){hash_word(a->nodes_with_rotation_flags[j]);hash_word(a->nodes_with_translation_flags[j]);hash_word(a->nodes_with_scale_flags[j]);}
        stream_digest(a,a->default_data.address,0);
        if(a->frame_size)for(j=0;j<(unsigned)a->frame_count;++j)stream_digest(a,(const unsigned char *)a->data.address+j*a->frame_size,1);
        for(j=0;j<(unsigned)a->frame_info.size;j+=4){memcpy(&bits,(const char *)a->frame_info.address+j,4);hash_word(bits);}
        CHECK(a->data.size==(long)d->data_size && (!d->data_size || !memcmp(a->data.address,d->data,d->data_size)));
        CHECK(a->default_data.size==(long)d->default_size && (!d->default_size || !memcmp(a->default_data.address,d->defaults,d->default_size)));
    }
    if(g->first_person_weapon_animations.count) {
        struct tag_block *indices=(struct tag_block *)((char *)g->first_person_weapon_animations.address+16);
        CHECK(indices->count==(long)portable.first_person_weapon_animations.count);
        CHECK(!memcmp(indices->address,portable.first_person_weapon_animations.indices,indices->count*2));
        CHECK(mac_retail_tag_get(cache,h&65535,&tag));raw=(unsigned char *)mac_retail_tag_span(cache,tag.address,128);
        reject(cache,h,raw+72,4,2);reject(cache,h,raw+76,4,0xffffffffU);
        list=(unsigned char *)mac_retail_tag_span(cache,u32(raw+76),28);
        list=(unsigned char *)mac_retail_tag_span(cache,u32(list+20),2);
        reject(cache,h,list,2,clips);
    }
    ok=mac_native_tags_publish(registry,h,owned.group,owned.root,owned.root_bytes,owned.bytes_owned,owned.owner,mac_native_animation_owner_free,&error);CHECK(ok);
    printf("native additional animation: handle=%08lx clips=%u nodes=%u owned_bytes=%lu digest=%08lx\n",(unsigned long)h,clips,nodes,(unsigned long)owned.bytes_owned,(unsigned long)digest);
    if(ok)owned.owner=NULL;
    mac_native_animation_tag_free(&owned);mac_retail_animation_graph_free(&portable);digest=saved_digest;
}
int main(int argc,char **argv)
{
    FILE *f;long size;unsigned char *bytes,*root,*nodes;struct mac_retail_cache cache;
    struct mac_retail_tag tag;struct mac_native_animation_tag owned;struct mac_retail_animation_graph decoded;
    struct mac_native_tags registry;struct animation_graph *g;const char *error;uint32_t h=0,i,j,k;size_t owned_bytes;uint32_t bits;
    if(argc!=2 || !(f=fopen(argv[1],"rb")))return 2;
    if(fseek(f,0,SEEK_END) || (size=ftell(f))<2048 || size>0x30000000 || fseek(f,0,SEEK_SET))return 2;
    bytes=(unsigned char *)malloc(size);if(!bytes || fread(bytes,1,size,f)!=(size_t)size)return 2;fclose(f);
    if(!mac_retail_open(bytes,size,&cache,&error))return 1;
    for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&tag) && tag.group==0x616e7472U && !strcmp(tag.name,"characters\\cyborg\\cyborg")){h=tag.handle;break;}
    if(!h || !mac_native_animation_decode(&cache,h,&owned,&error)){fprintf(stderr,"native animation: %s\n",h?error:"missing");return 1;}
    if(!mac_retail_animation_graph_decode(&cache,h,&decoded,&error))return 1;
    owned_bytes=owned.bytes_owned;g=(struct animation_graph *)owned.root;
    check_sound_visitor(g);
    CHECK(g->animations.count==254);CHECK(g->unit_seats.count==24);CHECK(g->nodes.count==19);CHECK(g->sound_references.count==23);
    for(i=0;i<(unsigned)g->animations.count;++i) {
        struct animation *a=(struct animation *)g->animations.address+i;struct mac_retail_animation *d=decoded.animations+i;
        hash_word(a->node_list_checksum);hash_word(a->frame_count);hash_word(a->node_count);hash_word(a->frame_size);
        for(j=0;j<2;++j){hash_word(a->nodes_with_rotation_flags[j]);hash_word(a->nodes_with_translation_flags[j]);hash_word(a->nodes_with_scale_flags[j]);}
        stream_digest(a,a->default_data.address,0);
        if(a->frame_size)for(j=0;j<(unsigned)a->frame_count;++j)stream_digest(a,(const unsigned char *)a->data.address+j*a->frame_size,1);
        for(j=0;j<(unsigned)a->frame_info.size;j+=4){memcpy(&bits,(const char *)a->frame_info.address+j,4);hash_word(bits);}
        CHECK(!strcmp(a->name,d->name));CHECK(a->frame_count==d->frame_count && a->node_count==d->node_count);
        CHECK(a->type==d->type && a->frame_size==d->frame_size && a->frame_info_type==d->frame_info_type);
        CHECK(a->node_list_checksum==d->node_list_checksum && a->private_loop_frame_index==d->loop_frame_index);
        CHECK(a->weight==d->weight && a->runtime_normalized_weight==d->normalized_weight);
        CHECK(a->next_animation_index==d->next_animation_index && a->sound_index==d->sound_index);
        CHECK(a->frame_info.size==(long)d->frame_info_size && !memcmp(a->frame_info.address,d->frame_info,d->frame_info_size));
        CHECK(a->default_data.size==(long)d->default_size && !memcmp(a->default_data.address,d->defaults,d->default_size));
        CHECK(a->data.size==(long)d->data_size && !memcmp(a->data.address,d->data,d->data_size));
        for(j=0;j<2;++j)CHECK(a->nodes_with_translation_flags[j]==d->translation_flags[j] && a->nodes_with_rotation_flags[j]==d->rotation_flags[j] && a->nodes_with_scale_flags[j]==d->scale_flags[j]);
    }
    for(i=0;i<(unsigned)g->unit_seats.count;++i) {
        struct animation_graph_unit_seat *s=(struct animation_graph_unit_seat *)g->unit_seats.address+i;
        struct mac_animation_seat *d=decoded.seats+i;
        CHECK(s->looking_screen_bounds.negative_yaw_delta==d->looking.negative_yaw_delta);
        CHECK(s->looking_screen_bounds.positive_pitch_frame_count==d->looking.positive_pitch_frame_count);
        CHECK(s->ik_points.count==(long)d->ik_count && s->weapon_classes.count==(long)d->class_count);
        for(j=0;j<(unsigned)s->weapon_classes.count;++j) {
            struct animation_graph_weapon_class *c=(struct animation_graph_weapon_class *)s->weapon_classes.address+j;
            struct mac_animation_class *dc=d->classes+j;
            CHECK(!strcmp(c->grip_marker_name,dc->grip_marker_name));CHECK(c->aiming_screen_bounds.positive_pitch_delta==dc->aiming.positive_pitch_delta);
            CHECK(c->weapon_types.count==(long)dc->type_count && c->ik_points.count==(long)dc->ik_count);
            for(k=0;k<(unsigned)c->weapon_types.count;++k) {
                struct animation_graph_weapon_type *t=(struct animation_graph_weapon_type *)c->weapon_types.address+k;
                CHECK(t->animations.count==(long)dc->types[k].animations.count && !memcmp(t->animations.address,dc->types[k].animations.indices,t->animations.count*2));
            }
        }
    }
    CHECK(digest==0x1fe96678U);
    root=(unsigned char *)mac_retail_tag_span(&cache,tag.address,128);
    nodes=(unsigned char *)mac_retail_tag_span(&cache,u32(root+108),64*19);
    reject(&cache,h,root+104,4,65);reject(&cache,h,root+108,4,0xffffffffU);
    reject(&cache,h,nodes+32,2,0);reject(&cache,h,nodes+36,2,1);reject(&cache,h,nodes+44,4,0x7fc00000U);
    reject(&cache,h,root,4,1);
    CHECK(mac_native_tags_create(&cache,&registry,&error));
    CHECK(mac_native_tags_publish(&registry,h,owned.group,owned.root,owned.root_bytes,owned.bytes_owned,owned.owner,mac_native_animation_owner_free,&error));
    owned.owner=NULL;
    extra_graph(&cache,&registry,0xe35601e2U,1,7);
    extra_graph(&cache,&registry,0xe2880114U,13,42);
    extra_graph(&cache,&registry,0xe36001ecU,10,44);
    /* Blood Gulch's Ghost graph has four real vehicle clips but deliberately
       no graph-node block or object overlays. Its clips map onto the model's
       hierarchy; the actor owner validates that relationship. */
    extra_graph(&cache,&registry,0xe4c70353U,4,0);
    CHECK(mac_engine_tags_bind(&registry));CHECK(animation_graph_definition_get((long)h)==g);
    free(bytes);mac_retail_animation_graph_free(&decoded);
    CHECK(g->animations.count==254 && ((struct animation *)g->animations.address)[0].data.address!=NULL);
    mac_engine_tags_unbind();mac_native_tags_free(&registry);mac_native_animation_tag_free(&owned);mac_native_animation_tag_free(&owned);
    printf("native animation: handle=%08lx clips=254 owned_bytes=%lu digest=%08lx checks=%u failures=%u\n",(unsigned long)h,(unsigned long)owned_bytes,(unsigned long)digest,checks,failures);
    return failures?1:0;
}
