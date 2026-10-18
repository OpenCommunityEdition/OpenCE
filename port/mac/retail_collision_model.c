#include "retail_collision_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Source layouts: damage_resistances.h, damage.c, collision_model_definitions.h,
   collision_bsp_definitions.h and tag_schema_collision.c. No engine overlays. */
static uint32_t u32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t u16(const unsigned char*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static int16_t s16(const unsigned char*p){int n=u16(p);return (int16_t)(n<32768?n:n-65536);}
static float f32(const unsigned char*p){uint32_t n=u32(p);float f;memcpy(&f,&n,4);return f;}
static int finite_float(float f){uint32_t n;memcpy(&n,&f,4);return (n&UINT32_C(0x7fffffff))<UINT32_C(0x7f800000);}
static int child_valid(uint32_t child,uint32_t nodes,uint32_t leaves)
{return child==UINT32_MAX||((child&UINT32_C(0x80000000))?(child&UINT32_C(0x7fffffff))<leaves:child<nodes);}
static int tree_visit(const struct mac_retail_collision*c,int two,uint32_t index,unsigned depth,unsigned char*color)
{
    unsigned j;uint32_t child;
    if(color[index]==1||depth>=128)return 0;if(color[index]==2)return 1;color[index]=1;
    for(j=0;j<2;++j){child=two?c->nodes2[index].child[j]:c->nodes[index].child[j];
        if(!(child&UINT32_C(0x80000000))&&!tree_visit(c,two,child,depth+1,color))return 0;}
    color[index]=2;return 1;
}
static int trees_valid(const struct mac_retail_collision*c)
{
    int two;uint32_t i,n;unsigned char*color;
    for(two=0;two<2;++two){n=c->count[two?4:0];if(!n)continue;color=calloc(n,1);if(!color)return 0;
        for(i=0;i<n;++i)if(!tree_visit(c,two,i,0,color)){free(color);return 0;}free(color);}
    return 1;
}
static int decode_bsp(const struct mac_retail_cache *c, const unsigned char *header, uint32_t *budget,
    struct mac_retail_collision *o, const char **error)
{
    static const uint32_t stride[8]={12,16,8,8,20,12,24,16};
    static const uint32_t limit[8]={131072,65536,65536,131072,65535,131072,262144,131072};
    const unsigned char *raw[8],*p;
    uint32_t i,j,k,n,index,side,first,edge;
    if (error) *error=NULL;
    if (!o) return 0;
    memset(o,0,sizeof(*o));
#define FAIL(s) do { if(error)*error=(s); mac_retail_collision_free(o); return 0; } while(0)
    for(i=0;i<8;++i) {
        n=u32(header+i*12); o->count[i]=n;
        if(n>limit[i] || n>*budget/stride[i]) FAIL("collision block count");
        *budget-=n*stride[i];
        raw[i]=n?mac_retail_tag_span(c,u32(header+i*12+4),n*stride[i]):NULL;
        if(n && !raw[i]) FAIL("collision block span");
    }
#define ALLOC(field,slot) do { o->field=calloc(o->count[slot]?o->count[slot]:1,sizeof(*o->field)); if(!o->field) FAIL("collision allocation"); } while(0)
    ALLOC(nodes,0); ALLOC(planes,1); ALLOC(leaves,2); ALLOC(references,3);
    ALLOC(nodes2,4); ALLOC(surfaces,5); ALLOC(edges,6); ALLOC(vertices,7);
#undef ALLOC
    for(i=0;i<o->count[0];++i) {
        p=raw[0]+i*12; o->nodes[i].plane=u32(p);
        if(o->nodes[i].plane>=o->count[1]) FAIL("collision node plane");
        for(j=0;j<2;++j) { index=u32(p+4+j*4); o->nodes[i].child[j]=index;
            if(!child_valid(index,o->count[0],o->count[2])) FAIL("collision node child"); }
    }
    for(i=0;i<o->count[1];++i) {
        p=raw[1]+i*16;
        for(j=0;j<3;++j) { o->planes[i].n[j]=f32(p+j*4); if(!finite_float(o->planes[i].n[j])) FAIL("collision plane float"); }
        o->planes[i].d=f32(p+12); if(!finite_float(o->planes[i].d)) FAIL("collision plane distance");
    }
    for(i=0;i<o->count[2];++i) {
        p=raw[2]+i*8; o->leaves[i].flags=u16(p); n=u16(p+2); index=u32(p+4);
        o->leaves[i].count=(uint16_t)n; o->leaves[i].first=index;
        if(n>32767 || (n && (index>o->count[3] || n>o->count[3]-index))) FAIL("collision leaf references");
    }
    for(i=0;i<o->count[3];++i) {
        p=raw[3]+i*8; index=u32(p); o->references[i].plane=index; o->references[i].root=u32(p+4);
        if((index&UINT32_C(0x7fffffff))>=o->count[1] || !child_valid(u32(p+4),o->count[4],o->count[5])) FAIL("collision reference");
    }
    for(i=0;i<o->count[4];++i) {
        p=raw[4]+i*20;
        for(j=0;j<3;++j) { o->nodes2[i].plane[j]=f32(p+j*4); if(!finite_float(o->nodes2[i].plane[j])) FAIL("collision 2D plane"); }
        for(j=0;j<2;++j) { index=u32(p+12+j*4); o->nodes2[i].child[j]=index;
            if(!child_valid(index,o->count[4],o->count[5])) FAIL("collision 2D child"); }
    }
    for(i=0;i<o->count[5];++i) {
        p=raw[5]+i*12; o->surfaces[i].plane=u32(p); o->surfaces[i].edge=u32(p+4);
        o->surfaces[i].flags=p[8]; o->surfaces[i].breakable=p[9];
        n=u16(p+10); o->surfaces[i].material=(int16_t)(n<32768?(int32_t)n:(int32_t)n-65536);
        if((u32(p)&UINT32_C(0x7fffffff))>=o->count[1] || u32(p+4)>=o->count[6]) FAIL("collision surface");
    }
    for(i=0;i<o->count[6];++i) {
        p=raw[6]+i*24;
        for(j=0;j<2;++j) {
            o->edges[i].vertex[j]=u32(p+j*4); o->edges[i].edge[j]=u32(p+8+j*4); o->edges[i].surface[j]=u32(p+16+j*4);
            if(o->edges[i].vertex[j]>=o->count[7] ||
               (o->edges[i].edge[j]!=UINT32_MAX && o->edges[i].edge[j]>=o->count[6]) ||
               (o->edges[i].surface[j]!=UINT32_MAX && o->edges[i].surface[j]>=o->count[5])) FAIL("collision edge");
        }
    }
    for(i=0;i<o->count[7];++i) {
        p=raw[7]+i*16;
        for(j=0;j<3;++j) { o->vertices[i].p[j]=f32(p+j*4); if(!finite_float(o->vertices[i].p[j])) FAIL("collision vertex float"); }
        o->vertices[i].edge=u32(p+12);
        if(o->vertices[i].edge>=o->count[6]) FAIL("collision vertex edge");
    }
    /* Same directed edge choice as collision_surface_polygon, with complete
       closure, membership and continuity validation before exposing arrays. */
    for(i=0;i<o->count[5];++i) {
        first=o->surfaces[i].edge; edge=first;
        for(k=0;k<8;++k) {
            if(edge>=o->count[6]) FAIL("collision ring edge");
            side=o->edges[edge].surface[1]==i;
            if(o->edges[edge].surface[side]!=i) FAIL("collision ring membership");
            index=o->edges[edge].edge[side];
            if(index>=o->count[6]) FAIL("collision ring next");
            j=o->edges[index].surface[1]==i;
            if(o->edges[edge].vertex[1-side]!=o->edges[index].vertex[j]) FAIL("collision ring continuity");
            edge=index; if(edge==first) break;
        }
        if(edge!=first || k<2) FAIL("collision ring closure");
    }
    if(!trees_valid(o)) FAIL("collision tree cycle/depth");
    return 1;
#undef FAIL
}
static int index_valid(int index,uint32_t count){return index==-1||(index>=0&&(uint32_t)index<count);}
static int text32(char*out,const unsigned char*p){if(!memchr(p,0,32))return 0;memcpy(out,p,32);return 1;}
static int block(const struct mac_retail_cache*c,const unsigned char*p,uint32_t stride,uint32_t limit,uint32_t*n,const unsigned char**raw)
{uint32_t count=u32(p);*n=0;*raw=NULL;if(count>limit)return 0;if(!count)return 1;*raw=mac_retail_tag_span(c,u32(p+4),count*stride);if(!*raw||(u32(p+4)&3))return 0;*n=count;return 1;}
static int effect(const struct mac_retail_cache*c,const unsigned char*p,struct mac_collision_effect*out)
{
    struct mac_retail_tag t;out->group=u32(p);out->handle=u32(p+12);
    if(out->handle==UINT32_MAX)return 1;
    return out->group==UINT32_C(0x65666665)&&mac_retail_tag_get(c,out->handle&65535,&t)&&t.handle==out->handle&&t.group==out->group;
}
void mac_retail_collision_model_free(struct mac_retail_collision_model*m)
{
    uint32_t i,j;if(!m)return;
    for(i=0;i<m->node_count;++i){if(m->nodes[i].bsps)for(j=0;j<m->nodes[i].bsp_count;++j)mac_retail_collision_free(&m->nodes[i].bsps[j]);free(m->nodes[i].bsps);}
    memset(m,0,sizeof(*m));
}
int mac_retail_collision_model_decode(const struct mac_retail_cache*c,uint32_t handle,struct mac_retail_collision_model*out,const char**error)
{
    static const unsigned effect_offsets[8]={112,132,152,168,188,392,408,424};
    struct mac_retail_collision_model*m;struct mac_collision_resistance*r;struct mac_retail_tag tag;
    const unsigned char*root,*raw,*p,*permutations,*bsps;uint32_t i,j,k,budget=UINT32_C(64)*1024*1024;
    int index;unsigned char seen[64],owned[64];
    if(error)*error=NULL;if(!out)return 0;memset(out,0,sizeof(*out));
    if(!c||!mac_retail_tag_get(c,handle&65535,&tag)||tag.handle!=handle||tag.group!=UINT32_C(0x636f6c6c)||
       !(root=mac_retail_tag_span(c,tag.address,0x298))){if(error)*error="collision model handle/span";
        if(getenv("HALO_MAC_LEVEL_DIAG")){struct mac_retail_tag d;int ok=c&&mac_retail_tag_get(c,handle&65535,&d);fprintf(stderr,"collision diag: handle %08x ok %d group %08x thandle %08x addr %08x name %s\n",(unsigned)handle,ok,ok?(unsigned)d.group:0,ok?(unsigned)d.handle:0,ok?(unsigned)d.address:0,ok?d.name:"");}return 0;}
    m=calloc(1,sizeof(*m));if(!m){if(error)*error="collision model allocation";return 0;}m->handle=handle;r=&m->resistance;
#define FAIL(s) do{if(error)*error=(s);mac_retail_collision_model_free(m);free(m);return 0;}while(0)
#define REAL(field,base,offset) do{(field)=f32((base)+(offset));if(!finite_float(field))FAIL("collision model nonfinite scalar");}while(0)
    r->flags=u32(root);r->indirect_material=s16(root+4);r->shield_material=s16(root+210);r->shield_failure_function=s16(root+236);
    REAL(r->body_vitality,root,8);REAL(r->body_system_shock,root,12);REAL(r->friendly_resistance,root,68);
    REAL(r->area_threshold,root,128);REAL(r->body_damaged_threshold,root,148);REAL(r->body_destroyed_threshold,root,184);
    REAL(r->shield_vitality,root,204);REAL(r->shield_failure_threshold,root,240);REAL(r->maximum_shield_failure,root,244);
    REAL(r->minimum_shield_stun_damage,root,264);REAL(r->shield_stun_time,root,268);REAL(r->shield_recharge_time,root,272);
    REAL(r->shield_damaged_threshold,root,388);REAL(r->shield_recharge_velocity,root,448);
    for(i=0;i<8;++i)if(!effect(c,root+effect_offsets[i],&r->effects[i]))FAIL("collision resistance effect");
    if(u32(root+588))FAIL("collision obsolete modifiers unsupported");
    for(i=0;i<6;++i)REAL(m->bounds[i],root,616+i*4);
    for(i=0;i<3;++i)if(m->bounds[i*2]>m->bounds[i*2+1])FAIL("collision pathfinding bounds");
    if(!block(c,root+564,72,32,&m->material_count,&raw))FAIL("collision materials span");
    if(!index_valid(r->indirect_material,m->material_count)||r->shield_material<0||r->shield_material>=33||r->shield_failure_function<0||r->shield_failure_function>=6)FAIL("collision resistance indices");
    for(i=0;i<m->material_count;++i){struct mac_collision_material*v=&m->materials[i];p=raw+i*72;
        if(!text32(v->name,p))FAIL("collision material name");v->flags=u32(p+32);v->type=s16(p+36);
        if(v->type<0||v->type>=33)FAIL("collision material type");REAL(v->leak,p,40);REAL(v->shield_multiplier,p,44);REAL(v->body_multiplier,p,60);
    }
    if(!block(c,root+576,84,32,&m->region_count,&raw))FAIL("collision regions span");
    for(i=0;i<m->region_count;++i){struct mac_collision_region*v=&m->regions[i];p=raw+i*84;
        if(!text32(v->name,p)||!effect(c,p+56,&v->effect))FAIL("collision region name/effect");v->flags=u32(p+32);REAL(v->threshold,p,40);
        if(!block(c,p+72,32,32,&v->permutation_count,&permutations))FAIL("collision permutations span");
        for(j=0;j<v->permutation_count;++j)if(!text32(v->permutations[j],permutations+j*32))FAIL("collision permutation name");
    }
    if(!block(c,root+640,32,32,&m->sphere_count,&raw))FAIL("collision spheres span");
    for(i=0;i<m->sphere_count;++i){struct mac_collision_sphere*v=&m->spheres[i];p=raw+i*32;v->node=s16(p);
        if(!index_valid(v->node,64))FAIL("collision sphere node");for(j=0;j<3;++j)REAL(v->center[j],p,16+j*4);REAL(v->radius,p,28);if(v->radius<0)FAIL("collision sphere radius");
    }
    if(!block(c,root+652,64,64,&m->node_count,&raw))FAIL("collision model nodes span");
    for(i=0;i<m->node_count;++i){struct mac_collision_model_node*v=&m->nodes[i];p=raw+i*64;
        if(!text32(v->name,p))FAIL("collision node name");v->region=s16(p+32);v->parent=s16(p+34);v->sibling=s16(p+36);v->child=s16(p+38);v->runtime_damage_part=s16(p+50);
        if(!index_valid(v->region,m->region_count)||!index_valid(v->parent,m->node_count)||!index_valid(v->sibling,m->node_count)||!index_valid(v->child,m->node_count))FAIL("collision node indices");
        if(!block(c,p+52,96,32,&v->bsp_count,&bsps))FAIL("collision node BSP span");
        if(v->bsp_count){v->bsps=calloc(v->bsp_count,sizeof(*v->bsps));if(!v->bsps)FAIL("collision node BSP allocation");}
        for(j=0;j<v->bsp_count;++j){const char*why;
            if(!decode_bsp(c,bsps+j*96,&budget,&v->bsps[j],&why))FAIL(why);
            for(k=0;k<v->bsps[j].count[5];++k)if(!index_valid(v->bsps[j].surfaces[k].material,m->material_count))FAIL("collision surface material");
        }
    }
    memset(owned,0,sizeof(owned));
    for(i=0;i<m->node_count;++i){
        memset(seen,0,sizeof(seen));index=(int)i;while(index!=-1){if(seen[index])FAIL("collision parent cycle");seen[index]=1;index=m->nodes[index].parent;}
        memset(seen,0,sizeof(seen));index=(int)i;while(index!=-1){if(seen[index])FAIL("collision sibling cycle");seen[index]=1;if(m->nodes[index].parent!=m->nodes[i].parent)FAIL("collision sibling ownership");index=m->nodes[index].sibling;}
        index=m->nodes[i].child;while(index!=-1){if(owned[index]||m->nodes[index].parent!=(int)i)FAIL("collision child ownership");owned[index]=1;index=m->nodes[index].sibling;}
    }
    for(i=0;i<m->node_count;++i)if(!!owned[i]!=(m->nodes[i].parent!=-1))FAIL("collision missing child");
    *out=*m;free(m);return 1;
#undef REAL
#undef FAIL
}
