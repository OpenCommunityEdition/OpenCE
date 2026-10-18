#include "retail_skeleton.h"
#include <string.h>
/* Layout evidence: model_definitions.h, game_globals.h, object_definitions.h.
   Node stride0x9c: name0, hierarchy0x20, translation0x28, quaternion0x34,
   distance0x44, editor padding0x48, inverse matrix0x68. */
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static int16_t s16(const unsigned char *p)
{ int32_t n=(int32_t)p[0]|(int32_t)p[1]<<8; return (int16_t)(n<32768?n:n-65536); }
static int floats(float *out,const unsigned char *p,uint32_t count)
{
    uint32_t i,w;
    for(i=0;i<count;++i) {
        w=u32(p+i*4); if((w&UINT32_C(0x7fffffff))>=UINT32_C(0x7f800000))return 0;
        memcpy(out+i,&w,4);
    }
    return 1;
}
static const unsigned char *definition(const struct mac_retail_cache *c,
    uint32_t handle,uint32_t group,uint32_t size)
{
    struct mac_retail_tag t;
    if(handle==UINT32_MAX || !mac_retail_tag_get(c,handle&65535,&t) ||
        t.handle!=handle || t.group!=group || (t.address&3))return NULL;
    return mac_retail_tag_span(c,t.address,size);
}
int mac_retail_multiplayer_model(const struct mac_retail_cache *c,
    uint32_t *biped,uint32_t *model,const char **error)
{
    uint32_t i,globals=UINT32_MAX,address,bh,mh;
    struct mac_retail_tag t;
    const unsigned char *g,*mp,*unit;
    if(error)*error=NULL;
    if(biped)*biped=UINT32_MAX;if(model)*model=UINT32_MAX;
#define FAIL(s) do { if(error)*error=(s);return 0; }while(0)
    if(!c || !biped || !model)FAIL("multiplayer model arguments");
    for(i=0;i<c->tag_count;++i) {
        if(!mac_retail_tag_get(c,i,&t))FAIL("globals lookup");
        if(t.group==UINT32_C(0x6d617467)) {
            if(globals!=UINT32_MAX)FAIL("ambiguous globals tags");globals=t.handle;
        }
    }
    g=definition(c,globals,UINT32_C(0x6d617467),0x1ac);
    if(g && u32(g+0x164)==0 && u32(g+0x170)==1) {
        /* Campaign globals carry no multiplayer information; the player's unit
           is player_information[0].player_unit (reference at element +0,
           element size 244). */
        address=u32(g+0x174);mp=mac_retail_tag_span(c,address,244);
        if(!mp || (address&3) || u32(mp)!=UINT32_C(0x62697064))FAIL("campaign player unit reference");
        bh=u32(mp+12);
    } else {
        if(!g || u32(g+0x164)!=1)FAIL("multiplayer globals block");
        address=u32(g+0x168);mp=mac_retail_tag_span(c,address,0xa0);
        if(!mp || (address&3) || u32(mp+16)!=UINT32_C(0x62697064))FAIL("multiplayer unit reference");
        bh=u32(mp+28);
    }unit=definition(c,bh,UINT32_C(0x62697064),0x4f4);
    if(!unit || u32(unit+0x28)!=UINT32_C(0x6d6f6432))FAIL("multiplayer model reference");
    mh=u32(unit+0x34);
    if(!definition(c,mh,UINT32_C(0x6d6f6432),0xe8))FAIL("multiplayer model handle");
    *biped=bh;*model=mh;return 1;
#undef FAIL
}
static int index_valid(int16_t index,uint32_t count)
{ return index==-1 || (index>=0 && (uint32_t)index<count); }
int mac_retail_skeleton_decode(const struct mac_retail_cache *c,uint32_t handle,
    struct mac_retail_skeleton *out,const char **error)
{
    struct mac_retail_skeleton decoded;
    const unsigned char *root,*raw,*p;
    unsigned char seen[MAC_RETAIL_MAX_MODEL_NODES],owned[MAC_RETAIL_MAX_MODEL_NODES];
    uint32_t i,n,address;int16_t index;
    if(error)*error=NULL;
    if(!out)return 0;
    memset(out,0,sizeof(*out));memset(&decoded,0,sizeof(decoded));memset(owned,0,sizeof(owned));
#define FAIL(s) do { if(error)*error=(s);return 0; }while(0)
    root=definition(c,handle,UINT32_C(0x6d6f6432),0xe8);
    if(!root)FAIL("model definition handle/span");
    n=u32(root+0xb8);address=u32(root+0xbc);
    if(!n || n>MAC_RETAIL_MAX_MODEL_NODES)FAIL("model node count");
    raw=mac_retail_tag_span(c,address,n*0x9c);
    if(!raw || (address&3))FAIL("model node span");
    decoded.model_handle=handle;decoded.checksum=u32(root+4);decoded.count=n;
    for(i=0;i<n;++i) {
        struct mac_retail_model_node *v=&decoded.nodes[i];
        p=raw+i*0x9c;
        if(!memchr(p,0,32))FAIL("model node name termination");
        memcpy(v->name,p,32);v->sibling=s16(p+0x20);v->child=s16(p+0x22);v->parent=s16(p+0x24);
        if(!index_valid(v->sibling,n) || !index_valid(v->child,n) || !index_valid(v->parent,n))FAIL("model hierarchy index");
        if(!floats(v->translation,p+0x28,3) || !floats(v->rotation,p+0x34,4) ||
           !floats(&v->distance,p+0x44,1) || !floats(v->inverse,p+0x68,13))FAIL("model node finite floats");
    }
    for(i=0;i<n;++i) {
        memset(seen,0,sizeof(seen));index=(int16_t)i;
        while(index!=-1) {
            if(seen[index])FAIL("model parent cycle");seen[index]=1;index=decoded.nodes[index].parent;
        }
        memset(seen,0,sizeof(seen));index=(int16_t)i;
        while(index!=-1) {
            if(seen[index])FAIL("model sibling cycle");seen[index]=1;
            if(decoded.nodes[index].parent!=decoded.nodes[i].parent)FAIL("model sibling parent mismatch");
            index=decoded.nodes[index].sibling;
        }
        index=decoded.nodes[i].child;
        while(index!=-1) {
            if(owned[index] || decoded.nodes[index].parent!=(int16_t)i)FAIL("model child ownership");
            owned[index]=1;index=decoded.nodes[index].sibling;
        }
    }
    for(i=0;i<n;++i)if((decoded.nodes[i].parent!=-1)!=!!owned[i])FAIL("model missing child linkage");
    *out=decoded;return 1;
#undef FAIL
}
