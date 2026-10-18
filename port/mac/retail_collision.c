#include "retail_collision.h"
#include <stdlib.h>
#include <string.h>
/* Layouts: physics/collision_bsp_definitions.h/.c, collision_bsp.c, bsp3d.c.
   All file reads are bytewise; native floats preserve their IEEE bit patterns. */
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t u16(const unsigned char *p) { return (uint16_t)(p[0]|p[1]<<8); }
static float f32(const unsigned char *p) { uint32_t b=u32(p); float f; memcpy(&f,&b,4); return f; }
static int finite_float(float f) { uint32_t b; memcpy(&b,&f,4); return (b&UINT32_C(0x7fffffff))<UINT32_C(0x7f800000); }
static const unsigned char *span(const struct mac_retail_cache *c,
    const struct mac_retail_bsp *b, uint32_t address, uint32_t size)
{
    uint32_t offset;
    if (address<b->base || (address&3)) return NULL;
    offset=address-b->base;
    if (offset<24 || offset>b->size || size>b->size-offset) return NULL;
    return c->file+b->file_offset+offset;
}
static int child_valid(uint32_t child, uint32_t nodes, uint32_t leaves)
{ return child==UINT32_MAX || ((child&UINT32_C(0x80000000)) ? (child&UINT32_C(0x7fffffff))<leaves : child<nodes); }
void mac_retail_collision_free(struct mac_retail_collision *o)
{
    if (!o) return;
    free(o->nodes); free(o->planes); free(o->leaves); free(o->references);
    free(o->nodes2); free(o->surfaces); free(o->edges); free(o->vertices);
    memset(o,0,sizeof(*o));
}
int mac_retail_collision_decode(const struct mac_retail_cache *c, uint32_t bsp_index,
    struct mac_retail_collision *o, const char **error)
{
    static const uint32_t stride[8]={12,16,8,8,20,12,24,16};
    static const uint32_t limit[8]={131072,65536,65536,131072,65535,131072,262144,131072};
    const struct mac_retail_bsp *b;
    const unsigned char *root,*header,*raw[8],*p;
    uint32_t i,j,k,n,index,side,first,edge;
    if (error) *error=NULL;
    if (!o) return 0;
    memset(o,0,sizeof(*o));
#define FAIL(s) do { if(error)*error=(s); mac_retail_collision_free(o); return 0; } while(0)
    if (!c || !c->file || bsp_index>=c->bsp_count) FAIL("collision arguments");
    b=&c->bsps[bsp_index];
    if (b->file_offset>c->file_size || b->size>c->file_size-b->file_offset) FAIL("collision BSP span");
    root=span(c,b,b->root,0xbc);
    if (!root || u32(root+0xb0)!=1) FAIL("collision BSP count");
    header=span(c,b,u32(root+0xb4),96);
    if (!header) FAIL("collision header span");
    for(i=0;i<8;++i) {
        n=u32(header+i*12); o->count[i]=n;
        if(n>limit[i] || n>b->size/stride[i]) FAIL("collision block count");
        raw[i]=n?span(c,b,u32(header+i*12+4),n*stride[i]):NULL;
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
    return 1;
#undef FAIL
}
int32_t mac_retail_collision_leaf(const struct mac_retail_collision *c, const float p[3])
{
    uint32_t index=0,depth; const struct mac_collision_node *n; const struct mac_collision_plane *plane; float distance;
    if(!c || !p || !finite_float(p[0]) || !finite_float(p[1]) || !finite_float(p[2])) return -2;
    for(depth=0;depth<128;++depth) {
        if(index>=c->count[0]) return -2;
        n=&c->nodes[index]; plane=&c->planes[n->plane];
        distance=((plane->n[0]*p[0]+plane->n[1]*p[1])+plane->n[2]*p[2])-plane->d;
        if(!finite_float(distance)) return -2;
        index=n->child[distance>=0.f];
        if(index==UINT32_MAX) return -1;
        if(index&UINT32_C(0x80000000)) return (int32_t)(index&UINT32_C(0x7fffffff));
    }
    return -2;
}
int32_t mac_retail_collision_ground(const struct mac_retail_collision *c,
    float x,float y,float top,float bottom,float *height)
{
    uint32_t i,k,edge,side; int32_t found=-1; int positive,negative;
    float z,nz,cross; const struct mac_collision_surface *s; const struct mac_collision_plane *p;
    const float *a,*b;
    if(!c || !height || !finite_float(x) || !finite_float(y) || !finite_float(top) || !finite_float(bottom) || bottom>top) return -1;
    for(i=0;i<c->count[5];++i) {
        s=&c->surfaces[i]; p=&c->planes[s->plane&UINT32_C(0x7fffffff)];
        nz=(s->plane&UINT32_C(0x80000000))?-p->n[2]:p->n[2];
        /* Exclude downward-facing/vertical and breakable surfaces. Invisible
           surfaces are still collision geometry; do not skip them. */
        if(nz<=0.00001f || (s->flags&8)) continue;
        z=(p->d-p->n[0]*x-p->n[1]*y)/p->n[2];
        if(!finite_float(z) || z>top || z<bottom || (found>=0 && z<=*height)) continue;
        positive=negative=0; edge=s->edge;
        for(k=0;k<8;++k) {
            side=c->edges[edge].surface[1]==i;
            a=c->vertices[c->edges[edge].vertex[side]].p;
            b=c->vertices[c->edges[edge].vertex[1-side]].p;
            cross=(b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0]);
            if(!finite_float(cross)) { positive=negative=1; break; }
            if(cross>0.00001f) positive=1; if(cross< -0.00001f) negative=1;
            edge=c->edges[edge].edge[side]; if(edge==s->edge) break;
        }
        if(!(positive && negative)) { found=(int32_t)i; *height=z; }
    }
    return found;
}
