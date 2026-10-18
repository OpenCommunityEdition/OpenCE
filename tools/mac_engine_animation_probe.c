#include "engine_animation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern unsigned long mac_engine_service_errors;
static unsigned long checks, failures;
static uint32_t digest = UINT32_C(2166136261);
#define CHECK(x) do { ++checks; if (!(x)) { if (failures < 12) printf("FAIL line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
static void hash_word(uint32_t value)
{
    unsigned i;
    for(i=0;i<4;++i) { digest ^= (value >> (i*8)) & 255; digest *= UINT32_C(16777619); }
}
static void compare_float(float actual, float expected)
{
    uint32_t a, b;
    memcpy(&a,&actual,4); memcpy(&b,&expected,4);
    CHECK(a == b); hash_word(a);
}
/* Independent traversal of the decoder's native streams. No engine fallback
   can pass this merely by returning the same rest pose on both architectures. */
static void reference(const struct mac_retail_animation *animation, unsigned frame,
    const struct mac_engine_orientation *orientations)
{
    const unsigned char *animated = animation->data;
    const unsigned char *defaults = animation->defaults, *p;
    unsigned node, component;
    if (animation->frame_size) animated += frame * animation->frame_size;
    for (node=0;node<(unsigned)animation->node_count;++node) {
        unsigned bit=1U<<(node&31), bank=node>>5;
        int moved=(animation->rotation_flags[bank]&bit)!=0;
        p=moved?animated:defaults;
        for(component=0;component<4;++component) {
            int16_t q;
            memcpy(&q,p+2*component,2);
            compare_float(orientations[node].rotation[component],q*(1.f/32767));
        }
        if(moved) animated+=8; else defaults+=8;
        moved=(animation->translation_flags[bank]&bit)!=0;
        p=moved?animated:defaults;
        for(component=0;component<3;++component) {
            float value;
            memcpy(&value,p+4*component,4);
            compare_float(orientations[node].translation[component],value);
        }
        if(moved) animated+=12; else defaults+=12;
        moved=(animation->scale_flags[bank]&bit)!=0;
        p=moved?animated:defaults;
        {
            float value;
            memcpy(&value,p,4);compare_float(orientations[node].scale,value);
        }
        if(moved) animated+=4; else defaults+=4;
    }
}
int main(int argc,char **argv)
{
    FILE *file;
    long length;
    unsigned char *bytes;
    struct mac_retail_cache cache;
    struct mac_retail_animation animation;
    struct mac_retail_skeleton skeleton;
    uint32_t biped_handle, model_handle;
    struct mac_engine_orientation out[64];
    const char *error=NULL;
    uint32_t graph=UINT32_C(0xe211009d), count,index,frame;
    unsigned base=0, skipped=0;
    unsigned long sampled_frames=0,sampled_nodes=0;
    {
        unsigned mode;
        union { unsigned long align; unsigned char bytes[24]; } stream;
        int16_t quaternion[4] = { 0, 0, 0, 32767 };
        float components[4] = { 2.f, -3.f, 4.f, 1.f };
        memcpy(stream.bytes,quaternion,8);memcpy(stream.bytes+8,components,16);
        for (mode=0;mode<2;++mode) {
            memset(&animation,0,sizeof(animation));memset(&skeleton,0,sizeof(skeleton));
            animation.node_count=1;animation.frame_count=1;skeleton.count=1;
            if (mode) {
                animation.frame_size=24;animation.data_size=24;animation.data=stream.bytes;
                animation.translation_flags[0]=animation.rotation_flags[0]=animation.scale_flags[0]=1;
            } else { animation.default_size=24;animation.defaults=stream.bytes; }
            CHECK(mac_engine_animation_sample(&animation,&skeleton,0,out,64,&error));
            reference(&animation,0,out);
        }
        digest=UINT32_C(2166136261);
    }
    if(argc<2) { fprintf(stderr,"usage: %s bloodgulch.map [graph_handle]\n",argv[0]);return 2; }
    if(argc>2) graph=(uint32_t)strtoul(argv[2],NULL,0);
    file=fopen(argv[1],"rb"); if(!file) { perror(argv[1]); return 2; }
    if(fseek(file,0,SEEK_END) || (length=ftell(file))<0 || length>0x30000000L || fseek(file,0,SEEK_SET)) { fclose(file);return 2; }
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes) { fclose(file);return 2; }
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length) { fclose(file);free(bytes);return 2; }
    fclose(file);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error) ||
        !mac_retail_animation_count(&cache,graph,&count,&error) ||
        !mac_retail_multiplayer_model(&cache,&biped_handle,&model_handle,&error) ||
        !mac_retail_skeleton_decode(&cache,model_handle,&skeleton,&error)) {
        fprintf(stderr,"map animation error: %s\n",error);free(bytes);return 1;
    }
    for(index=0;index<count;++index) {
        if(!mac_retail_animation_decode(&cache,graph,index,&animation,&error)) {
            fprintf(stderr,"animation %u: %s\n",(unsigned)index,error);free(bytes);return 1;
        }
        if(animation.type!=0) { ++skipped;mac_retail_animation_free(&animation);continue; }
        ++base;
        CHECK(skeleton.count == (unsigned)animation.node_count);
        CHECK(!skeleton.checksum || !animation.node_list_checksum || skeleton.checksum == animation.node_list_checksum);
        ++skeleton.count;
        CHECK(!mac_engine_animation_sample(&animation,&skeleton,0,out,64,&error));
        --skeleton.count;
        if (animation.node_list_checksum) {
            uint32_t saved = skeleton.checksum;
            skeleton.checksum = animation.node_list_checksum == 1 ? 2 : 1;
            CHECK(!mac_engine_animation_sample(&animation,&skeleton,0,out,64,&error));
            skeleton.checksum = saved;
        }
        CHECK(animation.flags==0);
        for(frame=0;frame<(unsigned)animation.frame_count;++frame) {
            CHECK(mac_engine_animation_sample(&animation,&skeleton,frame,out,64,&error));
            if(error) { fprintf(stderr,"sample error: %s\n",error);mac_retail_animation_free(&animation);free(bytes);return 1; }
            reference(&animation,frame,out);
            ++sampled_frames;sampled_nodes+=animation.node_count;
        }
        CHECK(!mac_engine_animation_sample(&animation,&skeleton,animation.frame_count,out,64,&error));
        CHECK(!mac_engine_animation_sample(&animation,&skeleton,0,out,0,&error));
        animation.flags|=1;
        CHECK(!mac_engine_animation_sample(&animation,&skeleton,0,out,64,&error));
        mac_retail_animation_free(&animation);
    }
    CHECK(base>0 && sampled_frames>0 && mac_engine_service_errors==0);
    printf("engine animation: %lu checks, %lu failures; %u base clips, %u skipped overlays/replacements; %lu frames, %lu node poses; digest %08lx; engine errors %lu\n",
        checks,failures,base,skipped,sampled_frames,sampled_nodes,(unsigned long)digest,mac_engine_service_errors);
    free(bytes);return failures?1:0;
}
