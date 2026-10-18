#include "retail_spawns.h"
#include <string.h>
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static int16_t s16(const unsigned char *p)
{
    int n=(int)p[0] | (int)p[1]<<8;
    return (int16_t)(n<32768?n:n-65536);
}
int mac_retail_spawns(const struct mac_retail_cache *c,
    struct mac_retail_spawn out[MAC_RETAIL_MAX_SPAWNS], uint32_t *count,
    const char **error)
{
    struct mac_retail_spawn decoded[MAC_RETAIL_MAX_SPAWNS];
    const unsigned char *scenario, *raw, *p;
    uint32_t n, address, i, j, bits;
    if (error) *error=NULL;
    if (count) *count=0;
#define FAIL(s) do { if (error) *error=(s); return 0; } while (0)
    if (!c || !out || !count || c->scenario_offset>UINT32_MAX-MAC_RETAIL_BASE)
        FAIL("spawn arguments");
    /* scenario_definitions.h: scenario.players at 0x354, each record 0x34.
       Only typed fields are decoded; editor padding is deliberately ignored. */
    scenario=mac_retail_tag_span(c,MAC_RETAIL_BASE+c->scenario_offset,0x360);
    if (!scenario) FAIL("spawn scenario range");
    n=u32(scenario+0x354); address=u32(scenario+0x358);
    if (n>MAC_RETAIL_MAX_SPAWNS) FAIL("spawn count");
    if (!n) return 1;
    raw=mac_retail_tag_span(c,address,n*52);
    if (!raw || (address&3)) FAIL("spawn block range");
    memset(decoded,0,sizeof(decoded));
    for (i=0; i<n; ++i) {
        p=raw+i*52;
        for (j=0; j<4; ++j) {
            bits=u32(p+j*4);
            if ((bits&UINT32_C(0x7fffffff))>=UINT32_C(0x7f800000)) FAIL("nonfinite spawn");
            if (j<3) memcpy(&decoded[i].position[j],&bits,4);
            else memcpy(&decoded[i].facing,&bits,4);
        }
        decoded[i].team=s16(p+16); decoded[i].bsp=s16(p+18);
        if (decoded[i].bsp < -1 || (decoded[i].bsp>=0 &&
            (uint32_t)decoded[i].bsp>=c->bsp_count)) FAIL("spawn BSP index");
        for (j=0; j<4; ++j) decoded[i].game_types[j]=s16(p+20+j*2);
    }
    memcpy(out,decoded,n*sizeof(*out)); *count=n; return 1;
#undef FAIL
}
int mac_retail_spawn_game_matches(const struct mac_retail_spawn *s, int type,
    int running)
{
    unsigned int i;
    if (!s) return 0;
    for (i=0; i<4; ++i) {
        int entry=s->game_types[i];
        if (!running) { if (entry!=0) return 0; }
        else if (entry==type || entry==12 || (entry==13 && type!=1) ||
            (entry==14 && type!=1 && type!=5)) return 1;
    }
    return !running;
}
