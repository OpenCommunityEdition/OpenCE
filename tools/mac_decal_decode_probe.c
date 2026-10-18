#include "retail_cache.h"
#include "native_decal_tags.h"
#include "native_tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef memcmp

static unsigned checks,failures;
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void put32(unsigned char *p,uint32_t v)
{ p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24); }
static void check(int pass,const char *label)
{ ++checks;if(!pass){++failures;fprintf(stderr,"FAIL %s\n",label);} }
struct refs { unsigned count,decals,bitmaps; };
static int count_reference(void *context,uint32_t group,uint32_t handle)
{
    struct refs *r=(struct refs *)context;(void)handle;++r->count;
    if(group==UINT32_C(0x64656361))++r->decals;
    if(group==UINT32_C(0x6269746d))++r->bitmaps;
    return 1;
}
static int rejects_and_clears(const struct mac_retail_cache *cache,uint32_t handle)
{
    struct mac_native_object_tag out,zero;const char *error=NULL;int ok;
    memset(&out,0xa5,sizeof(out));memset(&zero,0,sizeof(zero));
    ok=mac_native_decal_decode(cache,handle,&out,&error);
    check(!ok && !memcmp(&out,&zero,sizeof(out)),"malformed decal rejected with zero output");
    mac_native_object_tag_free(&out);return !ok;
}
static void mutate_reject(const struct mac_retail_cache *cache,uint32_t handle,
    unsigned char *field,uint32_t value,const char *label)
{
    unsigned char saved[4];
    memcpy(saved,field,4);put32(field,value);
    if(!rejects_and_clears(cache,handle)){++failures;fprintf(stderr,"FAIL %s\n",label);}
    else check(1,label);
    memcpy(field,saved,4);
}
static int run_mutations(const struct mac_retail_cache *cache,uint32_t handle,
    unsigned char *root)
{
    unsigned char saved[16];
    int ok=1;
    mutate_reject(cache,handle,root+2,4,"invalid decal type rejected");
    mutate_reject(cache,handle,root+0x18,UINT32_C(0x7fc00000),"NaN radius rejected");
    mutate_reject(cache,handle,root+0xd8,UINT32_C(0x736e6421),"wrong bitmap group rejected");
    memcpy(saved,root+8,16);
    put32(root+8,UINT32_C(0x64656361));put32(root+12,0);put32(root+16,0);put32(root+20,handle);
    if(!rejects_and_clears(cache,handle)){++failures;fprintf(stderr,"FAIL decal self-cycle rejected\n");ok=0;}
    else check(1,"decal self-cycle rejected");
    memcpy(root+8,saved,16);
    return ok;
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes,*scenario,*palette;uint32_t i,count,address;
    struct mac_retail_cache cache;struct mac_retail_tag tag;const char *error=NULL;
    unsigned valid=0;int mutated=0;
    if(argc!=2)return 2;
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,(size_t)length,f)!=(size_t)length){free(bytes);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)){fprintf(stderr,"map: %s\n",error);free(bytes);return 1;}
    scenario=(unsigned char *)cache.tags+cache.scenario_offset;
    /* Proven by engine_world.c's native scenario layout witness. */
    count=u32(scenario+0x3b4);address=u32(scenario+0x3b8);
    if(count>256 || (count && !(palette=(unsigned char *)mac_retail_tag_span(&cache,address,count*16)))) {
        fprintf(stderr,"scenario decal palette is out of bounds\n");free(bytes);return 1;
    }
    palette=count?(unsigned char *)mac_retail_tag_span(&cache,address,count*16):NULL;
    for(i=0;i<count;++i) {
        uint32_t handle=u32(palette+i*16+12);unsigned char *root;
        struct mac_native_object_tag decoded;struct refs refs={0,0,0};
        if(handle==UINT32_MAX)continue;
        if(!mac_retail_tag_get(&cache,handle&65535,&tag) || tag.handle!=handle ||
           tag.group!=UINT32_C(0x64656361)) { ++failures;fprintf(stderr,"invalid palette decal handle %08lx\n",(unsigned long)handle);continue; }
        if(!mac_native_decal_decode(&cache,handle,&decoded,&error)) {
            ++failures;fprintf(stderr,"palette decal %08lx: %s\n",(unsigned long)handle,error?error:"decode failed");continue;
        }
        ++valid;
        check(mac_native_object_visit_references(&decoded,count_reference,&refs),"decal references are visitable");
        check(refs.bitmaps>0,"decal exposes bitmap dependencies");
        check(refs.decals<=cache.tag_count,"decal chain references are bounded");
        printf("decal=%08lx fields=%lu refs=%u chain=%u bitmap=%u\n",(unsigned long)handle,
            (unsigned long)decoded.converted_values,refs.count,refs.decals,refs.bitmaps);
        root=(unsigned char *)mac_retail_tag_span(&cache,tag.address,0x10c);
        if(!mutated && root) { run_mutations(&cache,handle,root);mutated=1; }
        mac_native_object_tag_free(&decoded);
    }
    check(count==4,"Blood Gulch has four decal palette entries");
    check(valid>0,"at least one palette decal decoded");
    check(mutated,"malformed decal fixtures ran");
    printf("decal decoder checks=%u failures=%u\n",checks,failures);
    free(bytes);return failures?1:0;
}
