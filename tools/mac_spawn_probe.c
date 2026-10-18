/* Portable spawn regression and optional installed-map inventory. */
#include "retail_spawns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; printf("FAIL line %d: %s\n",__LINE__,#x); } } while (0)
static void put32(unsigned char *p,uint32_t n)
{ unsigned int i; for(i=0;i<4;++i) p[i]=(unsigned char)(n>>(i*8)); }
static void tests(void)
{
    unsigned char bytes[0x500];
    struct mac_retail_cache c;
    struct mac_retail_spawn s[MAC_RETAIL_MAX_SPAWNS], before;
    const char *error;
    uint32_t count;
    unsigned int slot,type,entry;
    memset(bytes,0,sizeof(bytes)); memset(&c,0,sizeof(c));
    c.tags=bytes; c.tag_size=sizeof(bytes); c.tags_end=0x20;
    c.scenario_offset=0x20; c.bsp_count=1;
    put32(bytes+0x374,1); put32(bytes+0x378,MAC_RETAIL_BASE+0x400);
    put32(bytes+0x400,0x3f800000); put32(bytes+0x404,0xc0000000);
    put32(bytes+0x408,0x3f000000); put32(bytes+0x40c,0x3fc00000);
    bytes[0x410]=0xff; bytes[0x411]=0xff;
    bytes[0x414]=14;
    CHECK(mac_retail_spawns(&c,s,&count,&error)); CHECK(count==1);
    CHECK(s[0].position[0]==1 && s[0].position[1]==-2 && s[0].position[2]==0.5f);
    CHECK(s[0].facing==1.5f && s[0].team==-1 && s[0].bsp==0);
    before=s[0];
    put32(bytes+0x374,257); CHECK(!mac_retail_spawns(&c,s,&count,&error)); CHECK(!count);
    CHECK(!memcmp(&before,s,sizeof(before))); put32(bytes+0x374,1);
    put32(bytes+0x378,MAC_RETAIL_BASE+0x4f0); CHECK(!mac_retail_spawns(&c,s,&count,&error));
    put32(bytes+0x378,MAC_RETAIL_BASE+0x401); CHECK(!mac_retail_spawns(&c,s,&count,&error));
    put32(bytes+0x378,MAC_RETAIL_BASE+0x400);
    put32(bytes+0x400,0x7fc00000); CHECK(!mac_retail_spawns(&c,s,&count,&error));
    put32(bytes+0x400,0x7f800000); CHECK(!mac_retail_spawns(&c,s,&count,&error));
    put32(bytes+0x400,0x3f800000);
    bytes[0x412]=1; CHECK(!mac_retail_spawns(&c,s,&count,&error));
    bytes[0x412]=0xfe; bytes[0x413]=0xff; CHECK(!mac_retail_spawns(&c,s,&count,&error));
    bytes[0x412]=0xff; CHECK(mac_retail_spawns(&c,s,&count,&error)); CHECK(s[0].bsp==-1);
    put32(bytes+0x374,0); CHECK(mac_retail_spawns(&c,s,&count,&error)); CHECK(!count);
    /* Exhaust all wildcard placements and game modes, including empty slots. */
    for(slot=0;slot<4;++slot) for(entry=0;entry<=15;++entry) for(type=1;type<=7;++type) {
        int expected=entry==type || entry==12 || (entry==13 && type!=1) ||
            (entry==14 && type!=1 && type!=5);
        memset(s,0,sizeof(s)); s[0].game_types[slot]=(int16_t)entry;
        CHECK(mac_retail_spawn_game_matches(s,(int)type,1)==expected);
        CHECK(mac_retail_spawn_game_matches(s,(int)type,0)==(entry==0));
    }
}
int main(int argc,char **argv)
{
    FILE *file;
    long size;
    unsigned char *bytes;
    struct mac_retail_cache c;
    struct mac_retail_spawn s[MAC_RETAIL_MAX_SPAWNS];
    const char *error=NULL;
    uint32_t n,i;
    unsigned int type,matches;
    tests();
    if(argc>1) {
        file=fopen(argv[1],"rb"); if(!file) { perror(argv[1]); return 2; }
        if(fseek(file,0,SEEK_END) || (size=ftell(file))<0 || size>0x30000000L ||
            fseek(file,0,SEEK_SET)) { fclose(file); return 2; }
        bytes=(unsigned char *)malloc((size_t)size);
        if(!bytes) { fclose(file); return 2; }
        if(fread(bytes,1,(size_t)size,file)!=(size_t)size) { free(bytes); fclose(file); return 2; }
        fclose(file);
        if(!mac_retail_open(bytes,(size_t)size,&c,&error) ||
            !mac_retail_spawns(&c,s,&n,&error)) {
            fprintf(stderr,"spawn decode: %s\n",error); free(bytes); return 1;
        }
        printf("map=%s starts=%lu\n",c.name,(unsigned long)n);
        for(type=0;type<=5;++type) {
            matches=0;
            for(i=0;i<n;++i) if(mac_retail_spawn_game_matches(s+i,(int)type,type!=0)) ++matches;
            printf("game_type=%u matches=%u\n",type,matches);
        }
        for(i=0;i<n;++i) printf("spawn=%lu pos=%.9g,%.9g,%.9g facing=%.9g team=%d bsp=%d types=%d,%d,%d,%d\n",
            (unsigned long)i,s[i].position[0],s[i].position[1],s[i].position[2],s[i].facing,
            s[i].team,s[i].bsp,s[i].game_types[0],s[i].game_types[1],s[i].game_types[2],s[i].game_types[3]);
        free(bytes);
    }
    printf("spawn checks=%u failures=%u\n",checks,failures);
    return failures?1:0;
}
