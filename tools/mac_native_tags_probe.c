/* Registry ownership/type-boundary tests. Synthetic roots are opaque fixture
   allocations, never passed to game functions or claimed to be real tags. */
#include "native_tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures,destroyed;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL native registry %d\n",__LINE__);}}while(0)
static void put(unsigned char*p,uint32_t n)
{ unsigned i;for(i=0;i<4;++i)p[i]=(unsigned char)(n>>(i*8)); }
static void release(void *p) { ++destroyed;free(p); }
static void synthetic(void)
{
    unsigned char bytes[512];struct mac_retail_cache c;struct mac_native_tags tags;
    const char *error;void *root;uint32_t cursor,checksum,h=UINT32_C(0x12340000);
    memset(&c,0,sizeof(c));memset(bytes,0,sizeof(bytes));
    c.tags=bytes;c.tag_size=sizeof(bytes);c.tag_count=2;c.tags_end=64;
    put(bytes,UINT32_C(0x62697064));put(bytes+4,UINT32_C(0x756e6974));
    put(bytes+8,UINT32_C(0x6f626a65));put(bytes+12,h);put(bytes+16,MAC_RETAIL_BASE+64);
    strcpy((char *)bytes+64,"fixture");
    put(bytes+32,UINT32_C(0x77656170));put(bytes+36,UINT32_C(0x6974656d));
    put(bytes+40,UINT32_C(0x6f626a65));put(bytes+44,h+1);put(bytes+48,MAC_RETAIL_BASE+80);
    strcpy((char *)bytes+80,"second fixture");
    CHECK(mac_native_tags_create(&c,&tags,&error));
    CHECK(mac_native_tags_checksum(&tags,&checksum) && checksum==0);
    CHECK(mac_native_tags_handle_at_index(&tags,0)==UINT32_MAX);
    CHECK(mac_native_tags_handle_at_index(&tags,1)==UINT32_MAX);
    CHECK(!strcmp(mac_native_tags_name(&tags,h),"fixture"));
    CHECK(mac_native_tags_find_metadata(&tags,UINT32_C(0x62697064),"fixture")==h);
    CHECK(mac_native_tags_find_metadata(&tags,UINT32_C(0x62697064),"missing")==UINT32_MAX);
    CHECK(!mac_native_tags_name(&tags,h^65536));
    CHECK(!mac_native_tags_get(&tags,UINT32_C(0x62697064),h,1,&error) && error);
    root=calloc(1,1280);if(!root)exit(2);
    CHECK(!mac_native_tags_publish(&tags,h,UINT32_C(0x77656170),root,1280,1280,root,release,&error));
    CHECK(!mac_native_tags_publish(&tags,h^65536,UINT32_C(0x62697064),root,1280,1280,root,release,&error));
    CHECK(!mac_native_tags_publish(&tags,h,UINT32_C(0x62697064),root,1280,1279,root,release,&error));
    CHECK(!mac_native_tags_publish(&tags,h,UINT32_C(0x62697064),root,1280,257U*1024U*1024U,root,release,&error));
    CHECK(destroyed==0);
    CHECK(mac_native_tags_publish(&tags,h,UINT32_C(0x62697064),root,1280,1280,root,release,&error));
    CHECK(mac_native_tags_owned_bytes(&tags)==1280);
    CHECK(mac_native_tags_handle_at_index(&tags,0)==h);
    CHECK(mac_native_tags_handle_at_index(&tags,1)==UINT32_MAX);
    CHECK(mac_native_tags_handle_at_index(&tags,2)==UINT32_MAX);
    cursor=0;CHECK(mac_native_tags_next(&tags,UINT32_MAX,&cursor)==h);
    CHECK(mac_native_tags_next(&tags,UINT32_MAX,&cursor)==UINT32_MAX && cursor==2);
    CHECK(mac_native_tags_next(&tags,UINT32_MAX,&cursor)==UINT32_MAX && cursor==2);
    cursor=0;CHECK(mac_native_tags_next(&tags,UINT32_C(0x756e6974),&cursor)==h);
    cursor=0;CHECK(mac_native_tags_next(&tags,UINT32_C(0x77656170),&cursor)==UINT32_MAX);
    cursor=UINT32_MAX;CHECK(mac_native_tags_next(&tags,UINT32_MAX,&cursor)==UINT32_MAX);
    CHECK(mac_native_tags_next(&tags,UINT32_MAX,NULL)==UINT32_MAX);
    CHECK(mac_native_tags_next(NULL,UINT32_MAX,&cursor)==UINT32_MAX);
    CHECK(!mac_native_tags_publish(&tags,h+1,UINT32_C(0x77656170),root,1280,1280,root,release,&error));
    CHECK(mac_native_tags_get(&tags,UINT32_C(0x62697064),h,1280,&error)==root);
    CHECK(mac_native_tags_get(&tags,UINT32_C(0x756e6974),h,1280,&error)==root);
    CHECK(mac_native_tags_get(&tags,UINT32_C(0x6f626a65),h,1280,&error)==root);
    CHECK(!mac_native_tags_get(&tags,UINT32_C(0x77656170),h,1,&error));
    CHECK(!mac_native_tags_get(&tags,UINT32_MAX,h,1,&error));
    CHECK(!mac_native_tags_get(&tags,UINT32_C(0x62697064),h^65536,1,&error));
    CHECK(!mac_native_tags_get(&tags,UINT32_C(0x62697064),h,1281,&error));
    CHECK(!mac_native_tags_publish(&tags,h,UINT32_C(0x62697064),root,1280,1280,root,release,&error));
    CHECK(destroyed==0);
    memset(bytes,0,sizeof(bytes));
    CHECK(!strcmp(mac_native_tags_name(&tags,h),"fixture"));
    CHECK(mac_native_tags_get(&tags,UINT32_C(0x62697064),h,1280,&error)==root);
    mac_native_tags_free(&tags);mac_native_tags_free(&tags);
    CHECK(destroyed==1 && !tags.state && !mac_native_tags_owned_bytes(&tags));
    /* A malicious table cannot grant biped inheritance to a different family. */
    put(bytes,UINT32_C(0x77656170));put(bytes+4,UINT32_C(0x756e6974));
    put(bytes+8,UINT32_C(0x6f626a65));put(bytes+12,h);put(bytes+16,MAC_RETAIL_BASE+64);
    CHECK(!mac_native_tags_create(&c,&tags,&error) && !tags.state);
}
static void transformed_model(void)
{
    unsigned char bytes[128];struct mac_retail_cache c;struct mac_native_tags tags;
    const char *error;void *root;uint32_t cursor,h=UINT32_C(0x12340000);
    memset(&c,0,sizeof(c));memset(bytes,0,sizeof(bytes));c.tags=bytes;c.tag_size=sizeof(bytes);c.tag_count=1;
    put(bytes,UINT32_C(0x6d6f6432));put(bytes+4,UINT32_MAX);put(bytes+8,UINT32_MAX);
    put(bytes+12,h);put(bytes+16,MAC_RETAIL_BASE+32);strcpy((char *)bytes+32,"fixture model");
    CHECK(mac_native_tags_create(&c,&tags,&error));
    CHECK(mac_native_tags_group(&tags,h)==UINT32_MAX);
    CHECK(mac_native_tags_find(&tags,UINT32_C(0x6d6f6432),"fixture model")==UINT32_MAX);
    root=calloc(1,256);if(!root)exit(2);
    CHECK(!mac_native_tags_publish(&tags,h,UINT32_C(0x6d6f6465),root,256,256,root,release,&error));
    CHECK(mac_native_tags_publish_model(&tags,h,root,256,256,root,release,&error));
    CHECK(mac_native_tags_group(&tags,h)==UINT32_C(0x6d6f6465));
    cursor=0;CHECK(mac_native_tags_next(&tags,UINT32_C(0x6d6f6465),&cursor)==h);
    cursor=0;CHECK(mac_native_tags_next(&tags,UINT32_C(0x6d6f6432),&cursor)==UINT32_MAX);
    CHECK(mac_native_tags_get(&tags,UINT32_C(0x6d6f6465),h,256,&error)==root);
    CHECK(!mac_native_tags_get(&tags,UINT32_C(0x6d6f6432),h,1,&error));
    CHECK(mac_native_tags_find(&tags,UINT32_C(0x6d6f6465),"FIXTURE MODEL")==h);
    CHECK(mac_native_tags_find(&tags,UINT32_C(0x6d6f6432),"fixture model")==UINT32_MAX);
    CHECK(mac_native_tags_find(&tags,UINT32_C(0x6d6f6465),"fixture")==UINT32_MAX);
    CHECK(mac_native_tags_group(&tags,h^65536)==UINT32_MAX);
    mac_native_tags_free(&tags);CHECK(destroyed==2);
}
int main(int argc,char **argv)
{
    FILE*f;long size;unsigned char*bytes;struct mac_retail_cache c;
    struct mac_native_tags tags;struct mac_retail_tag tag;const char *error;uint32_t i,checksum;
    synthetic();transformed_model();
    if(argc==2) {
        f=fopen(argv[1],"rb");if(!f)return 2;
        if(fseek(f,0,SEEK_END) || (size=ftell(f))<2048 || size>0x30000000L || fseek(f,0,SEEK_SET))return 2;
        bytes=(unsigned char *)malloc(size);if(!bytes)return 2;
        if(fread(bytes,1,size,f)!=(size_t)size)return 2;fclose(f);
        if(!mac_retail_open(bytes,size,&c,&error) || !mac_native_tags_create(&c,&tags,&error)) { fprintf(stderr,"registry: %s\n",error);return 1; }
        CHECK(mac_native_tags_checksum(&tags,&checksum) && checksum==
            ((uint32_t)c.tags[8]|(uint32_t)c.tags[9]<<8|(uint32_t)c.tags[10]<<16|(uint32_t)c.tags[11]<<24));
        for(i=0;i<c.tag_count;++i) {
            CHECK(mac_retail_tag_get(&c,i,&tag));
            CHECK(!strcmp(tag.name,mac_native_tags_name(&tags,tag.handle)));
            CHECK(!mac_native_tags_get(&tags,tag.group,tag.handle,1,&error) && error);
        }
        CHECK(mac_native_tags_owned_bytes(&tags)==0);
        printf("native registry metadata=%lu, all unconverted roots refused\n",(unsigned long)c.tag_count);
        mac_native_tags_free(&tags);free(bytes);
    }
    printf("native registry: checks=%u failures=%u destructor_calls=%u\n",checks,failures,destroyed);
    return failures?1:0;
}
