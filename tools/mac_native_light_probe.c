#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "native_light_tags.h"
#include "native_tags.h"
#include "engine_tags.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef memcmp
#undef strcmp
#undef strlen
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s\n",#x);}}while(0)
static int visit(void *context,uint32_t group,uint32_t handle)
{
    unsigned *count=context;
    CHECK((group==UINT32_C(0x6269746d)&&handle==UINT32_C(0xe24900d5)) ||
          (group==UINT32_C(0x6c656e73)&&handle==UINT32_C(0xe24a00d6)));
    ++*count;return 1;
}
static int refuse(void *context,uint32_t group,uint32_t handle)
{(void)context;(void)group;(void)handle;return 0;}
static void reject(struct mac_retail_cache *c,uint32_t h,unsigned char *p,unsigned n,uint32_t v)
{
    unsigned char saved[4];unsigned i;struct mac_native_light_tag a,z;const char *e;int ok;
    memcpy(saved,p,n);for(i=0;i<n;++i)p[i]=(unsigned char)(v>>(i*8));memset(&z,0,sizeof(z));memset(&a,0xa5,sizeof(a));
    ok=mac_native_light_decode(c,h,&a,&e);CHECK(!ok && !memcmp(&a,&z,sizeof(a)));mac_native_light_tag_free(&a);memcpy(p,saved,n);
}
int main(int argc,char **argv)
{
    FILE *f;long size;unsigned char *bytes,*raw;struct mac_retail_cache cache;struct mac_retail_tag t;
    struct mac_native_light_tag light;struct mac_native_tags registry;struct tag_reference *gel,*lens;const char *e;uint32_t flags,h=0xe24800d4U;
    void *root;unsigned references=0;
    if(argc!=2 || !(f=fopen(argv[1],"rb")))return 2;
    if(fseek(f,0,SEEK_END) || (size=ftell(f))<2048 || size>0x30000000 || fseek(f,0,SEEK_SET))return 2;
    bytes=(unsigned char *)malloc(size);if(!bytes || fread(bytes,1,size,f)!=(size_t)size)return 2;fclose(f);
    if(!mac_retail_open(bytes,size,&cache,&e) || !mac_native_light_decode(&cache,h,&light,&e)){fprintf(stderr,"light: %s\n",e);return 1;}
    CHECK(mac_native_light_visit_references(&light,visit,&references));CHECK(references==2);
    CHECK(!mac_native_light_visit_references(&light,refuse,NULL));
    CHECK(!mac_native_light_visit_references(&light,NULL,NULL));
    light.group=0;CHECK(!mac_native_light_visit_references(&light,visit,&references));light.group=UINT32_C(0x6c696768);
    root=light.root;gel=(struct tag_reference *)((char *)root+100);lens=(struct tag_reference *)((char *)root+172);memcpy(&flags,root,4);
    CHECK(flags==0x19);CHECK(light.root_bytes==352);CHECK(gel->index==(long)0xe24900d5U);CHECK(lens->index==(long)0xe24a00d6U);
    CHECK(mac_retail_tag_get(&cache,h&65535,&t));raw=(unsigned char *)mac_retail_tag_span(&cache,t.address,352);
    reject(&cache,h,raw+4,4,0x7fc00000U);reject(&cache,h,raw+76,4,0x7f800000U);
    reject(&cache,h,raw+112,4,0);reject(&cache,h,raw+184,4,0);reject(&cache,h,raw+104,4,0xffffffffU);
    reject(&cache,h,raw+142,2,12);reject(&cache,h,raw+250,2,6);
    CHECK(mac_native_tags_create(&cache,&registry,&e));CHECK(mac_native_tags_publish(&registry,h,light.group,light.root,light.root_bytes,light.bytes_owned,light.owner,mac_native_light_owner_free,&e));
    light.owner=NULL;CHECK(mac_engine_tags_bind(&registry));CHECK(tag_get(0x6c696768L,(long)h)==root);
    free(bytes);CHECK(gel->name && strlen(gel->name)>0);CHECK(lens->name && strlen(lens->name)>0);
    references=0;CHECK(mac_native_light_visit_references(&light,visit,&references));CHECK(references==2);
    printf("native flashlight: root_bytes=%lu owned_bytes=%lu digest=%08lx checks=%u failures=%u\n",(unsigned long)light.root_bytes,(unsigned long)light.bytes_owned,(unsigned long)light.field_digest,checks,failures);
    mac_engine_tags_unbind();mac_native_tags_free(&registry);mac_native_light_tag_free(&light);mac_native_light_tag_free(&light);return failures?1:0;
}
