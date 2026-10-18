#include "engine_collision.h"
#include "retail_spawns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,failures;
static uint32_t digest=UINT32_C(2166136261);
static void check(int pass) { ++checks; if(!pass) ++failures; }
static void compare(const struct mac_retail_collision *c,
    const struct mac_engine_collision *bridge,const float p[3])
{
    int32_t expected=mac_retail_collision_leaf(c,p);
    int32_t actual=mac_engine_collision_leaf(bridge,p);
    check(expected>=-1 && expected==actual);
    digest=(digest^(uint32_t)actual)*UINT32_C(16777619);
}
static void synthetic(void)
{
    struct mac_collision_node node;
    struct mac_collision_plane plane;
    struct mac_retail_collision c;
    struct mac_engine_collision bridge;
    const char *error;
    float p[3]={0,0,1};
    memset(&c,0,sizeof(c));memset(&plane,0,sizeof(plane));
    node.plane=0;node.child[0]=UINT32_MAX;node.child[1]=UINT32_C(0x80000000);
    plane.n[2]=1;c.nodes=&node;c.planes=&plane;c.count[0]=c.count[1]=c.count[2]=1;
    check(mac_engine_collision_create(&c,&bridge,&error));
    if(bridge.state) {
        check(mac_engine_collision_leaf(&bridge,p)==0);
        p[2]=-1;check(mac_engine_collision_leaf(&bridge,p)==-1);
        p[2]=0;check(mac_engine_collision_leaf(&bridge,p)==0);
        node.child[1]=UINT32_MAX;p[2]=1;
        check(mac_engine_collision_leaf(&bridge,p)==0);
        check(mac_engine_collision_leaf(&bridge,NULL)==-2);
        mac_engine_collision_free(&bridge);
        check(!bridge.state);
    }
    node.plane=1;check(!mac_engine_collision_create(&c,&bridge,&error));
    check(!bridge.state);
    node.plane=0;node.child[1]=UINT32_C(0x80000001);
    check(!mac_engine_collision_create(&c,&bridge,&error));
    node.child[1]=1;check(!mac_engine_collision_create(&c,&bridge,&error));
}
int main(int argc,char **argv)
{
    FILE *f;
    long size;
    unsigned char *bytes;
    struct mac_retail_cache cache;
    struct mac_retail_collision c;
    struct mac_engine_collision bridge;
    struct mac_retail_spawn starts[MAC_RETAIL_MAX_SPAWNS];
    const char *error;
    uint32_t count,i,j,k,start_count=0;
    float low[3],high[3],p[3];
    synthetic();
    if(argc!=2 || failures) {
        printf("engine collision synthetic checks=%u failures=%u\n",checks,failures);
        return failures?1:(argc==1?0:2);
    }
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END) || (size=ftell(f))<2048 || size>0x30000000L || fseek(f,0,SEEK_SET)) { fclose(f);return 2; }
    bytes=(unsigned char *)malloc((size_t)size);if(!bytes) { fclose(f);return 2; }
    if(fread(bytes,1,(size_t)size,f)!=(size_t)size) { free(bytes);fclose(f);return 2; }fclose(f);
    if(!mac_retail_open(bytes,(size_t)size,&cache,&error) ||
       !mac_retail_collision_decode(&cache,0,&c,&error)) {
        fprintf(stderr,"engine collision decode: %s\n",error);free(bytes);return 1;
    }
    if(!mac_engine_collision_create(&c,&bridge,&error)) {
        fprintf(stderr,"engine collision bridge: %s\n",error);
        mac_retail_collision_free(&c);free(bytes);return 1;
    }
    if(!mac_retail_spawns(&cache,starts,&count,&error)) {
        fprintf(stderr,"engine collision spawns: %s\n",error);
        mac_engine_collision_free(&bridge);mac_retail_collision_free(&c);free(bytes);return 1;
    }
    for(i=0;i<count;++i) {
        if(starts[i].bsp>0) continue;
        compare(&c,&bridge,starts[i].position);++start_count;
    }
    check(c.count[7]>0);
    if(c.count[7]) {
        for(k=0;k<3;++k) low[k]=high[k]=c.vertices[0].p[k];
        for(i=0;i<c.count[7];++i) for(k=0;k<3;++k) {
            if(c.vertices[i].p[k]<low[k])low[k]=c.vertices[i].p[k];
            if(c.vertices[i].p[k]>high[k])high[k]=c.vertices[i].p[k];
        }
        /* Multiple heights include both solid and open samples. */
        for(i=0;i<32;++i)for(j=0;j<32;++j)for(k=0;k<5;++k) {
            p[0]=low[0]+(high[0]-low[0])*(float)i/31.f;
            p[1]=low[1]+(high[1]-low[1])*(float)j/31.f;
            p[2]=low[2]-1.f+(high[2]-low[2]+2.f)*(float)k/4.f;
            compare(&c,&bridge,p);
        }
    }
    printf("actual engine bsp3d_test_point: starts=%lu grid=%u checks=%u failures=%u digest=%08lx\n",
        (unsigned long)start_count,c.count[7]?5120:0,checks,failures,(unsigned long)digest);
    mac_engine_collision_free(&bridge);mac_retail_collision_free(&c);free(bytes);
    return failures?1:0;
}
