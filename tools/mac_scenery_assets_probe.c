/* Real Blood Gulch scenery dependency walk. This checks storage/dependency
   conversion only; it does not call objects_place or claim shader readiness. */
#include "native_actor_assets.h"
#include "native_object_tags.h"
#include "objects/object_definitions.h"
#include "retail_cache.h"
#undef malloc
#undef free
#undef memcpy
#undef strcmp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures;
#define CHECK(x) do { ++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);} } while(0)
static uint32_t read32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static int has_word(const char *text,const char *word)
{
    size_t i,j,n=0,m=0;
    while(text[n])++n;while(word[m])++m;
    if(m>n)return 0;
    for(i=0;i+m<=n;++i) {
        for(j=0;j<m;++j) {
            char a=text[i+j],b=word[j];
            if(a>='A'&&a<='Z')a=(char)(a-'A'+'a');
            if(b>='A'&&b<='Z')b=(char)(b-'A'+'a');
            if(a!=b)break;
        }
        if(j==m)return 1;
    }
    return 0;
}
static void print_animation_families(const struct mac_retail_cache *cache,uint32_t scenery_handle)
{
    struct mac_retail_tag scenery,animation;const unsigned char *raw,*graph;uint32_t h;
    if(!mac_retail_tag_get(cache,scenery_handle&65535,&scenery))return;
    raw=mac_retail_tag_span(cache,scenery.address,0x58);if(!raw)return;
    h=read32(raw+0x44);if(h==UINT32_MAX||!mac_retail_tag_get(cache,h&65535,&animation))return;
    graph=mac_retail_tag_span(cache,animation.address,128);if(!graph)return;
    fprintf(stderr," animation=%s(%08x) families object=%u vehicle=%u device=%u\n",animation.name,
        (unsigned)h,(unsigned)read32(graph),(unsigned)read32(graph+36),(unsigned)read32(graph+48));
    { unsigned count=read32(graph+116),i;const unsigned char *clips=mac_retail_tag_span(cache,read32(graph+120),count*180);
      fprintf(stderr,"  animation count=%u ptr=%08x clips=%p\n",count,(unsigned)read32(graph+120),(void *)clips);
      if(clips)for(i=0;i<count;++i)fprintf(stderr,"  clip[%u] %.32s info_type=%d type=%d frames=%d frame_size=%d nodes=%d graph_nodes=%u flags=%04x\n",i,clips+i*180,(int16_t)(clips[i*180+38]|(unsigned)clips[i*180+39]<<8),(int16_t)(clips[i*180+32]|(unsigned)clips[i*180+33]<<8),(int16_t)(clips[i*180+34]|(unsigned)clips[i*180+35]<<8),(int16_t)(clips[i*180+36]|(unsigned)clips[i*180+37]<<8),(int16_t)(clips[i*180+44]|(unsigned)clips[i*180+45]<<8),read32(graph+104),(unsigned)(clips[i*180+58]|(unsigned)clips[i*180+59]<<8)); }
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes;struct mac_retail_cache cache;
    struct mac_actor_assets assets={0};const unsigned char *raw_palette,*raw_ssce;
    const char *error=NULL;uint32_t i,loaded=0,rejected=0;int shield_seen=0;
    if(argc!=2)return 2;
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,(size_t)length,f)!=(size_t)length){free(bytes);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)) {
        fprintf(stderr,"cache: %s\n",error?error:"open failed");free(bytes);return 1;
    }
    { const unsigned char *scenario=cache.tags+cache.scenario_offset;
      CHECK(read32(scenario+0x21c)==19 && read32(scenario+0x2e8)==1);
      raw_palette=mac_retail_tag_span(&cache,read32(scenario+0x220),19*48);
      raw_ssce=mac_retail_tag_span(&cache,read32(scenario+0x2ec),48);
      if(!raw_palette||!raw_ssce){free(bytes);return 1;} }
    if(!mac_actor_assets_load(&cache,&assets,&error)) {
        fprintf(stderr,"actor prerequisites: %s\n",error?error:"load failed");free(bytes);return 1;
    }
    for(i=0;i<19;++i) {
        uint32_t handle=read32(raw_palette+i*48+12);
        struct mac_retail_tag tag;int is_shield;
        CHECK(mac_retail_tag_get(&cache,handle&65535,&tag));
        is_shield=has_word(tag.name,"teleporter")&&has_word(tag.name,"shield");
        fprintf(stderr,"scenery[%u] %s (%08x): ",i,tag.name,(unsigned)handle);
        if(mac_actor_assets_load_scenery(&cache,&assets,handle,&error)) {
            ++loaded;fprintf(stderr,"loaded\n");
            if(is_shield) {
                const struct object_definition *scenery=(const struct object_definition *)
                    mac_native_tags_get(&assets.tags,UINT32_C(0x7363656e),handle,sizeof(*scenery),&error);
                long attachment_index;int looping_sound_seen=0;
                CHECK(scenery!=NULL);
                for(attachment_index=0;scenery&&attachment_index<scenery->object.attachments.count;++attachment_index) {
                    const struct object_attachment_definition *attachment=
                        (const struct object_attachment_definition *)scenery->object.attachments.address+attachment_index;
                    if((uint32_t)attachment->type.group_tag==UINT32_C(0x6c736e64)) {
                        CHECK(mac_native_tags_group(&assets.tags,(uint32_t)attachment->type.index)==UINT32_C(0x6c736e64));
                        looping_sound_seen=1;
                    }
                }
                CHECK(looping_sound_seen);
                shield_seen=1;
            }
            CHECK(mac_native_tags_group(&assets.tags,handle)==UINT32_C(0x7363656e));
        } else {
            ++rejected;fprintf(stderr,"rejected: %s\n",error?error:"no reason");
            print_animation_families(&cache,handle);
            CHECK(0);
            CHECK(mac_native_tags_group(&assets.tags,handle)==UINT32_MAX);
        }
    }
    CHECK(loaded==19);CHECK(rejected==0);CHECK(shield_seen);
    {
        uint32_t handle=read32(raw_ssce+12);
        struct mac_retail_tag tag;struct mac_native_object_tag ssce;
        CHECK(mac_retail_tag_get(&cache,handle&65535,&tag));
        CHECK(tag.group==UINT32_C(0x73736365));
        CHECK(!mac_native_scenery_decode(&cache,handle,&ssce,&error));
        CHECK(mac_actor_assets_load_sound_scenery(&cache,&assets,handle,&error));
        CHECK(mac_native_tags_group(&assets.tags,handle)==UINT32_C(0x73736365));
        fprintf(stderr,"sound scenery %s (%08x) constructor dependencies loaded; sound payloads remain unloaded\n",tag.name,(unsigned)handle);
    }
    printf("scenery assets: checks=%u failures=%u; loaded=%u rejected=%u; sound payloads not loaded; no placement/render\n",
        checks,failures,(unsigned)loaded,(unsigned)rejected);
    mac_actor_assets_free(&assets);free(bytes);
    return failures?1:0;
}
