#include "retail_geometry.h"
#include <stdlib.h>
#include <string.h>

/* Source layout: structure_bsp_definitions.h and the PC vertex validation in
   cache_file_formats.c. No native overlays of file bytes, no float arithmetic. */
typedef char vertex_size[(sizeof(struct mac_retail_vertex)==56)?1:-1];
typedef char light_size[(sizeof(struct mac_retail_light_vertex)==20)?1:-1];
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static uint16_t u16(const unsigned char *p)
{ return (uint16_t)((unsigned int)p[0] | (unsigned int)p[1]<<8); }
static const unsigned char *span(const unsigned char *bsp, const struct mac_retail_bsp *b,
    uint32_t address, uint32_t size)
{
    uint32_t offset;
    if (address < b->base) return NULL;
    offset = address-b->base;
    if (offset < 24 || offset > b->size || size > b->size-offset) return NULL;
    return bsp+offset;
}
static int block(const unsigned char *bsp, const struct mac_retail_bsp *b,
    const unsigned char *field, uint32_t stride, uint32_t max,
    uint32_t *count, const unsigned char **data)
{
    uint32_t address = u32(field+4);
    *count = u32(field); *data = NULL;
    if (*count > max || *count > b->size/stride) return 0;
    if (!*count) return 1;
    *data = span(bsp, b, address, *count*stride);
    return *data != NULL && !(address&3);
}
static int floats_valid(const unsigned char *raw, uint32_t count, unsigned int words,
    unsigned int unit_begin, unsigned int unit_end)
{
    uint32_t i, bits;
    unsigned int j;
    for (i=0; i<count; ++i) for (j=0; j<words; ++j) {
        bits = u32(raw+((size_t)i*words+j)*4) & UINT32_C(0x7fffffff);
        if (bits >= UINT32_C(0x7f800000)) return 0;
        /* Compare positive IEEE magnitudes to exact 1.005f bit pattern. */
        if (j>=unit_begin && j<unit_end && bits>UINT32_C(0x3f80a3d7)) return 0;
    }
    return 1;
}
static void floats_decode(void *out, const unsigned char *raw, uint32_t words)
{
    unsigned char *p = (unsigned char *)out;
    uint32_t i, bits;
    for (i=0; i<words; ++i) { bits=u32(raw+i*4); memcpy(p+i*4, &bits, 4); }
}
int mac_retail_bsp_geometry(const struct mac_retail_cache *c, uint32_t bsp_index,
    mac_retail_material_proc visit, void *context, const char **error)
{
    const struct mac_retail_bsp *b;
    const unsigned char *bsp, *root, *lights, *surfaces, *materials, *light, *m, *raw;
    uint32_t lc, sc, mc, i, j, k, first, triangles, vc, lvc, size, maxv=0, maxt=0;
    uint32_t totalv=0, totalt=0, totalm=0;
    struct mac_retail_vertex *vertices;
    struct mac_retail_light_vertex *light_vertices;
    uint16_t *indices;
    struct mac_retail_material output;
    if (error) *error=NULL;
#define FAIL(s) do { if (error) *error=(s); return 0; } while (0)
    if (!c || !c->file || bsp_index>=c->bsp_count || !visit) FAIL("geometry arguments");
    b=&c->bsps[bsp_index]; bsp=c->file+b->file_offset; root=bsp+(b->root-b->base);
    if (!block(bsp,b,root+0xf8,6,131072,&sc,&surfaces) ||
        !block(bsp,b,root+0x104,32,128,&lc,&lights)) FAIL("surface/lightmap block");
    /* Full preflight: do not expose even earlier valid materials if any later
       material contains malformed counts, vectors or triangle indices. */
    for (i=0; i<lc; ++i) {
        light=lights+i*32;
        if (!block(bsp,b,light+20,256,2048,&mc,&materials)) FAIL("material block");
        if (mc>65536-totalm) FAIL("material work limit");
        totalm+=mc;
        for (j=0; j<mc; ++j) {
            m=materials+j*256; first=u32(m+20); triangles=u32(m+24);
            vc=u32(m+0xb4); lvc=u32(m+0xc8); size=u32(m+0xd8);
            if (u16(m+0xb0)!=0 || (u16(m+0xc4)!=0 && u16(m+0xc4)!=2) ||
                (u16(light)>32767 && u16(light)!=65535) || triangles>20000 || vc>64000 || lvc!=(u16(light)!=65535?vc:0) ||
                size!=vc*56+lvc*20 || first>sc || triangles>sc-first ||
                vc>1000000-totalv || triangles>2000000-totalt) FAIL("material counts/type");
            totalv+=vc; totalt+=triangles;
            raw=size?span(bsp,b,u32(m+0xe4),size):NULL;
            if (size && (!raw || (u32(m+0xe4)&3))) FAIL("vertex data range");
            if (!floats_valid(raw,vc,14,3,12) || !floats_valid(lvc?raw+vc*56:NULL,lvc,5,0,3)) FAIL("vertex float/vector");
            for (k=0; k<triangles*3; ++k)
                if (u16(surfaces+first*6+k*2)>=vc) FAIL("triangle vertex index");
            if (vc>maxv) maxv=vc;
            if (triangles>maxt) maxt=triangles;
        }
    }
    vertices=(struct mac_retail_vertex *)malloc((maxv?maxv:1)*sizeof(*vertices));
    light_vertices=(struct mac_retail_light_vertex *)malloc((maxv?maxv:1)*sizeof(*light_vertices));
    indices=(uint16_t *)malloc((maxt?maxt:1)*3*sizeof(*indices));
    if (!vertices || !light_vertices || !indices) {
        free(vertices); free(light_vertices); free(indices); FAIL("geometry allocation");
    }
    for (i=0; i<lc; ++i) {
        light=lights+i*32;
        block(bsp,b,light+20,256,2048,&mc,&materials);
        for (j=0; j<mc; ++j) {
            m=materials+j*256; vc=u32(m+0xb4); lvc=u32(m+0xc8);
            first=u32(m+20); triangles=u32(m+24); size=u32(m+0xd8);
            raw=size?span(bsp,b,u32(m+0xe4),size):NULL;
            floats_decode(vertices,raw,vc*14);
            floats_decode(light_vertices,lvc?raw+vc*56:NULL,lvc*5);
            for (k=0; k<triangles*3; ++k) indices[k]=u16(surfaces+first*6+k*2);
            output.shader_handle=u32(m+12); output.lightmap_bitmap=u16(light)==65535?-1:u16(light);
            output.vertex_count=vc; output.light_vertex_count=lvc; output.triangle_count=triangles;
            output.vertices=vertices; output.light_vertices=light_vertices; output.indices=indices;
            if (!visit(context,&output)) {
                free(vertices); free(light_vertices); free(indices); FAIL("geometry callback");
            }
        }
    }
    free(vertices); free(light_vertices); free(indices);
    return 1;
#undef FAIL
}
