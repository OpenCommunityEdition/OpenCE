#include "retail_collision.h"
#include "retail_spawns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t digest=UINT32_C(2166136261);
static unsigned checks,failures;
static void word(uint32_t w) { digest=(digest^w)*UINT32_C(16777619); }
static void check(int ok) { ++checks; if(!ok) ++failures; }
static void put(unsigned char *p,uint32_t n) { p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24); }
static void putf(unsigned char *p,float f) { uint32_t w;memcpy(&w,&f,4);put(p,w); }
static void synthetic(void)
{
    unsigned char bytes[1024], saved[1024]; struct mac_retail_cache cache; struct mac_retail_collision c;
    const char *error; unsigned i; float point[3]={0.25f,0.25f,1.f},z=0;
    static const unsigned offsets[8]={400,412,428,0,0,436,448,520};
    static const unsigned counts[8]={1,1,1,0,0,1,3,3};
    memset(bytes,0,sizeof(bytes));memset(&cache,0,sizeof(cache));
    cache.file=bytes;cache.file_size=1024;cache.bsp_count=1;
    cache.bsps[0].base=0x1000;cache.bsps[0].root=0x1018;cache.bsps[0].size=1024;
    put(bytes+24+0xb0,1);put(bytes+24+0xb4,0x1100);
    for(i=0;i<8;++i) { put(bytes+256+i*12,counts[i]);put(bytes+260+i*12,0x1000+offsets[i]); }
    put(bytes+404,UINT32_MAX);put(bytes+408,UINT32_C(0x80000000));putf(bytes+420,1.f);
    for(i=0;i<3;++i) {
        put(bytes+448+i*24,i);put(bytes+452+i*24,(i+1)%3);
        put(bytes+456+i*24,(i+1)%3);put(bytes+460+i*24,UINT32_MAX);
        put(bytes+464+i*24,0);put(bytes+468+i*24,UINT32_MAX);
        put(bytes+532+i*16,i);
    }
    putf(bytes+536,1.f);putf(bytes+556,1.f);
    check(mac_retail_collision_decode(&cache,0,&c,&error));
    if(c.nodes) {
        check(c.nodes[0].child[1]==UINT32_C(0x80000000));
        check(mac_retail_collision_leaf(&c,point)==0);
        point[2]=-1;check(mac_retail_collision_leaf(&c,point)==-1);
        check(mac_retail_collision_ground(&c,.25f,.25f,1,-1,&z)==0 && z==0);
        check(mac_retail_collision_ground(&c,2,2,1,-1,&z)==-1);
        check(mac_retail_collision_ground(&c,.25f,.25f,2,1,&z)==-1);
        c.nodes[0].child[1]=0; point[2]=1;check(mac_retail_collision_leaf(&c,point)==-2);
        mac_retail_collision_free(&c);
    }
    memcpy(saved,bytes,sizeof(bytes));
    for(i=0;i<8;++i) {
        put(bytes+256+i*12,UINT32_MAX);
        check(!mac_retail_collision_decode(&cache,0,&c,&error));
        check(!c.nodes && !c.vertices && !c.count[0]);
        memcpy(bytes,saved,sizeof(bytes));
        if(counts[i]) {
            put(bytes+260+i*12,0x13fc);
            check(!mac_retail_collision_decode(&cache,0,&c,&error));
            memcpy(bytes,saved,sizeof(bytes));
        }
    }
    put(bytes+400,1);check(!mac_retail_collision_decode(&cache,0,&c,&error));memcpy(bytes,saved,sizeof(bytes));
    put(bytes+408,UINT32_C(0x80000001));check(!mac_retail_collision_decode(&cache,0,&c,&error));memcpy(bytes,saved,sizeof(bytes));
    bytes[430]=1;check(!mac_retail_collision_decode(&cache,0,&c,&error));memcpy(bytes,saved,sizeof(bytes));
    put(bytes+448,3);check(!mac_retail_collision_decode(&cache,0,&c,&error));memcpy(bytes,saved,sizeof(bytes));
    put(bytes+532,3);check(!mac_retail_collision_decode(&cache,0,&c,&error));memcpy(bytes,saved,sizeof(bytes));
    put(bytes+260,0x13fc);check(!mac_retail_collision_decode(&cache,0,&c,&error));put(bytes+260,0x1190);
    put(bytes+420,UINT32_C(0x7fc00000));check(!mac_retail_collision_decode(&cache,0,&c,&error));putf(bytes+420,1.f);
    put(bytes+456,0);check(!mac_retail_collision_decode(&cache,0,&c,&error));put(bytes+456,1);
    put(bytes+464,1);check(!mac_retail_collision_decode(&cache,0,&c,&error));
}
static int spawn_checks(const struct mac_retail_cache *cache, const struct mac_retail_collision *collision)
{
    struct mac_retail_spawn starts[MAC_RETAIL_MAX_SPAWNS];
    uint32_t count,i,considered=0,skipped=0,start_leaves=0,start_solid=0,start_bad=0;
    uint32_t hits=0,ground_leaves=0,ground_solid=0,ground_bad=0,worst_index=0;
    const char *error;
    float p[3],height,gap,min_gap=0,max_gap=0,worst_gap=0,absolute_gap;
    int32_t leaf;
    if(!mac_retail_spawns(cache,starts,&count,&error)) {
        fprintf(stderr,"collision spawn decode: %s\n",error);return 0;
    }
    for(i=0;i<count;++i) {
        /* This probe owns BSP0 only. An unspecified BSP (-1) is measured in
           BSP0 for this diagnostic, not assigned a production spawn policy. */
        if(starts[i].bsp>0) { ++skipped;continue; }
        ++considered;memcpy(p,starts[i].position,sizeof(p));
        leaf=mac_retail_collision_leaf(collision,p);
        if(leaf>=0)++start_leaves;else if(leaf==-1)++start_solid;else ++start_bad;
        if(mac_retail_collision_ground(collision,p[0],p[1],p[2]+.5f,p[2]-5.f,&height)>=0) {
            gap=p[2]-height;absolute_gap=gap<0?-gap:gap;
            if(!hits || gap<min_gap)min_gap=gap;
            if(!hits || gap>max_gap)max_gap=gap;
            if(!hits || absolute_gap>worst_gap) { worst_gap=absolute_gap;worst_index=i; }
            ++hits;p[2]=height+.1f;
            leaf=mac_retail_collision_leaf(collision,p);
            if(leaf>=0)++ground_leaves;else if(leaf==-1)++ground_solid;else ++ground_bad;
        }
    }
    printf("spawn collision BSP0: total=%lu considered=%lu other_BSP=%lu starts_open=%lu solid=%lu invalid=%lu\n",
        (unsigned long)count,(unsigned long)considered,(unsigned long)skipped,
        (unsigned long)start_leaves,(unsigned long)start_solid,(unsigned long)start_bad);
    printf("spawn ground [z-5,z+0.5]: hits=%lu misses=%lu above_open=%lu solid=%lu invalid=%lu\n",
        (unsigned long)hits,(unsigned long)(considered-hits),(unsigned long)ground_leaves,
        (unsigned long)ground_solid,(unsigned long)ground_bad);
    if(hits)printf("spawn vertical gap: min=%.9f max=%.9f worst_absolute=%.9f start=%lu\n",
        min_gap,max_gap,worst_gap,(unsigned long)worst_index);
    /* Misses/solid classifications are observations: this is not the engine's
       spawn-selection or capsule-clearance test. */
    return !start_bad && !ground_bad;
}
int main(int argc,char **argv)
{
    FILE *f; long size; unsigned char *bytes; struct mac_retail_cache cache;
    struct mac_retail_collision c; const char *error; uint32_t i,j,w; float p[3],z,minx=0,maxx=0,miny=0,maxy=0,minz=0,maxz=0;
    unsigned hits=0,leaves=0,solid=0,bad=0;
    synthetic(); if(failures) { printf("synthetic checks=%u failures=%u\n",checks,failures);return 1; }
    if(argc!=2) { printf("synthetic checks=%u failures=%u\n",checks,failures);return argc==1?0:2; }
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END) || (size=ftell(f))<2048 || size>0x30000000L || fseek(f,0,SEEK_SET)) { fclose(f);return 2; }
    bytes=malloc((size_t)size);if(!bytes) { fclose(f);return 2; }
    if(fread(bytes,1,(size_t)size,f)!=(size_t)size) { free(bytes);fclose(f);return 2; }fclose(f);
    if(!mac_retail_open(bytes,(size_t)size,&cache,&error) || !mac_retail_collision_decode(&cache,0,&c,&error)) {
        fprintf(stderr,"collision: %s\n",error);free(bytes);return 1;
    }
    printf("collision blocks:");for(i=0;i<8;++i) { printf(" %lu",(unsigned long)c.count[i]);word(c.count[i]); }puts("");
    /* Canonical numeric digest of every native decoded field, endian independent. */
    for(i=0;i<c.count[0];++i) { word(c.nodes[i].plane);word(c.nodes[i].child[0]);word(c.nodes[i].child[1]); }
    for(i=0;i<c.count[1];++i) { for(j=0;j<3;++j){memcpy(&w,&c.planes[i].n[j],4);word(w);}memcpy(&w,&c.planes[i].d,4);word(w); }
    for(i=0;i<c.count[2];++i) {word(c.leaves[i].flags);word(c.leaves[i].count);word(c.leaves[i].first);}
    for(i=0;i<c.count[3];++i) {word(c.references[i].plane);word(c.references[i].root);}
    for(i=0;i<c.count[4];++i) {for(j=0;j<3;++j){memcpy(&w,&c.nodes2[i].plane[j],4);word(w);}word(c.nodes2[i].child[0]);word(c.nodes2[i].child[1]);}
    for(i=0;i<c.count[5];++i) {word(c.surfaces[i].plane);word(c.surfaces[i].edge);word(c.surfaces[i].flags);word(c.surfaces[i].breakable);word((uint32_t)(int32_t)c.surfaces[i].material);}
    for(i=0;i<c.count[6];++i)for(j=0;j<2;++j){word(c.edges[i].vertex[j]);word(c.edges[i].edge[j]);word(c.edges[i].surface[j]);}
    for(i=0;i<c.count[7];++i) {
        for(j=0;j<3;++j){memcpy(&w,&c.vertices[i].p[j],4);word(w);}word(c.vertices[i].edge);
        p[0]=c.vertices[i].p[0];p[1]=c.vertices[i].p[1];p[2]=c.vertices[i].p[2];
        if(!i || p[0]<minx)minx=p[0];if(!i || p[0]>maxx)maxx=p[0];
        if(!i || p[1]<miny)miny=p[1];if(!i || p[1]>maxy)maxy=p[1];
        if(!i || p[2]<minz)minz=p[2];if(!i || p[2]>maxz)maxz=p[2];
    }
    printf("native digest=%08lx bounds=(%.6f %.6f) (%.6f %.6f) (%.6f %.6f)\n",(unsigned long)digest,minx,maxx,miny,maxy,minz,maxz);
    for(i=0;i<32;++i)for(j=0;j<32;++j) {
        int32_t leaf;
        p[0]=minx+(maxx-minx)*((float)i+.5f)/32.f;p[1]=miny+(maxy-miny)*((float)j+.5f)/32.f;
        if(mac_retail_collision_ground(&c,p[0],p[1],maxz+1,minz-1,&z)>=0) {
            ++hits;p[2]=z+.1f;leaf=mac_retail_collision_leaf(&c,p);
            if(leaf>=0)++leaves;else if(leaf==-1)++solid;else ++bad;
        }
    }
    printf("diagnostic grid=1024 ground_hits=%u above_ground_leaves=%u solid=%u invalid=%u synthetic_checks=%u failures=%u\n",hits,leaves,solid,bad,checks,failures);
    if(!spawn_checks(&cache,&c)) ++bad;
    mac_retail_collision_free(&c);free(bytes);return bad?1:0;
}
