#include "native_model_gpu_probe.h"
#include "engine_device_state.h"
#include "native_resource_memory.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer_model_types.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/xbox/rasterizer_xbox_vertex_shaders.h"
#include <OpenGL/gl.h>
#include <string.h>
#include <stdlib.h>

struct mac_gpu_model_shader_ref { struct tag_reference shader; short permutation; word pad; long unused[3]; };
struct mac_gpu_model_geometry { byte reserved[0x24]; struct tag_block parts; };
struct mac_gpu_model_part {
    unsigned long flags; short shader; char previous,next; short node0,node1;
    real weight0,weight1; real_point3d centroid; struct tag_block uncompressed,compressed,triangles;
    struct triangle_buffer triangle_buffer; struct vertex_buffer vertex_buffer;
};
struct mac_gpu_model_vertex {
    real_point3d position; unsigned long normal,binormal,tangent; point2d texcoord;
    byte nodes[2]; short weight;
};
struct mac_gpu_probe_vertex { real_point3d position; float color[4]; };
typedef char mac_gpu_model_vertex_size[sizeof(struct mac_gpu_model_vertex)==0x20?1:-1];
typedef char mac_gpu_model_part_size[sizeof(struct mac_gpu_model_part)==0x68?1:-1];
#define MAC_GPU_MODEL_PART_STRIPPED 1UL
#define MAC_GPU_MAX_NODES 64
#define MAC_GPU_MAX_GEOMETRIES 256
#define MAC_GPU_MAX_REGIONS 32
#define MAC_GPU_MAX_PARTS 32
#define MAC_GPU_MAX_VERTICES 65535
#define MAC_GPU_MAX_INDICES 196607

struct draw_context { long object_index; const char *error; struct mac_native_model_gpu_result result; };
static int finite_value(real x){return x==x&&x<1000000.0f&&x> -1000000.0f;}
static int block_ok(const struct tag_block *b,unsigned n){return b&&b->count>=0&&(unsigned long)b->count<=n&&(!b->count||b->address!=NULL);}
static int get_geometry_indices(struct model *model,const struct object_datum *object,byte *seen)
{
    unsigned r;memset(seen,0,MAC_GPU_MAX_GEOMETRIES);
    if(!block_ok(&model->regions,MAC_GPU_MAX_REGIONS)||!block_ok(&model->geometries,MAC_GPU_MAX_GEOMETRIES))return 0;
    for(r=0;r<(unsigned)model->regions.count;++r){
        struct model_region *region=(struct model_region *)model->regions.address+r;
        struct model_region_permutation *p;short selected;
        if(object->object.region_permutations[r]==0xff)continue;
        if(!block_ok(&region->permutations,32)||object->object.region_permutations[r]>=region->permutations.count)return 0;
        p=(struct model_region_permutation *)region->permutations.address+object->object.region_permutations[r];
        selected=p->geometry_indices[4];if(selected==-1)continue;
        if(selected<0||selected>=model->geometries.count||selected>=MAC_GPU_MAX_GEOMETRIES)return 0;
        seen[selected]=1;
    }
    return 1;
}
static int skin_matrix_build(const struct model *model,const real_matrix4x3 *live,real_matrix4x3 *skin,unsigned count)
{
    unsigned n;if(!block_ok(&model->nodes,MAC_GPU_MAX_NODES)||count!=(unsigned)model->nodes.count||!count)return 0;
    for(n=0;n<count;++n){const struct model_node *node=(const struct model_node *)model->nodes.address+n;
        matrix4x3_multiply(live+n,&node->runtime_default_inverse_matrix,skin+n);}
    return 1;
}
static int skinned_vertex(const struct mac_gpu_model_vertex *v,const real_matrix4x3 *skin,unsigned nodes,real_point3d *out)
{
    unsigned n0=v->nodes[0]/3U,n1=v->nodes[1]/3U;real w0=(real)v->weight*(1.0f/32767.0f),w1=1.0f-w0;
    real_point3d p0,p1;
    if(v->nodes[0]%3||v->nodes[1]%3||n0>=nodes||n1>=nodes||w0<0||w0>1||!finite_value(v->position.x)||!finite_value(v->position.y)||!finite_value(v->position.z))return 0;
    matrix4x3_transform_point(skin+n0,&v->position,&p0);matrix4x3_transform_point(skin+n1,&v->position,&p1);
    out->x=p0.x*w0+p1.x*w1;out->y=p0.y*w0+p1.y*w1;out->z=p0.z*w0+p1.z*w1;
    return finite_value(out->x)&&finite_value(out->y)&&finite_value(out->z);
}
static int collect_bounds(struct model *model,const byte *seen,const real_matrix4x3 *skin,unsigned nodes,
    real lo[3],real hi[3],unsigned long *total)
{
    unsigned g,p,v,k;int first=1;struct mac_gpu_model_geometry *geoms=(struct mac_gpu_model_geometry *)model->geometries.address;
    *total=0;
    for(g=0;g<(unsigned)model->geometries.count;++g)if(seen[g]){
        if(!block_ok(&geoms[g].parts,MAC_GPU_MAX_PARTS))return 0;
        for(p=0;p<(unsigned)geoms[g].parts.count;++p){struct mac_gpu_model_part *part=(struct mac_gpu_model_part *)geoms[g].parts.address+p;
            struct mac_gpu_model_vertex *verts=(struct mac_gpu_model_vertex *)part->vertex_buffer.base_address;
            real_point3d world;if(part->flags&MAC_GPU_MODEL_PART_STRIPPED)continue;
            if(!verts||part->vertex_buffer.count<=0||(unsigned long)part->vertex_buffer.count>MAC_GPU_MAX_VERTICES)return 0;
            for(v=0;v<(unsigned)part->vertex_buffer.count;++v){if(!skinned_vertex(verts+v,skin,nodes,&world))return 0;
                for(k=0;k<3;++k){real c=((real *)&world)[k];if(first){lo[k]=hi[k]=c;}else{if(c<lo[k])lo[k]=c;if(c>hi[k])hi[k]=c;}}
                first=0;++*total;
            }
        }
    }
    return !first&&*total>0;
}
static int setup_diagnostic_shader(unsigned width,unsigned height)
{
    static const DWORD declaration[]={1UL<<29,(2UL<<29)|(D3DVSDT_FLOAT3<<16),
        (2UL<<29)|(D3DVSDT_FLOAT4<<16)|9,D3DVSD_END()};
    const DWORD *code=(const DWORD *)vertex_shader_table[0].code;DWORD shader;float c[60][4];
    memset(c,0,sizeof(c));c[0][0]=c[1][1]=c[2][2]=c[3][3]=1.0f;
    c[58][0]=(float)width*0.5f;c[58][1]=-(float)height*0.5f;c[58][2]=1.0f;
    c[59][0]=(float)width*0.5f;c[59][1]=(float)height*0.5f;
    if(D3DDevice_CreateVertexShader(declaration,code,&shader,0)!=S_OK)return 0;
    D3DDevice_SetVertexShader(shader);D3DDevice_SetVertexShaderConstant(-96,c,60);
    D3DDevice_SetRenderState_Deferred(D3DRS_PSTEXTUREMODES,0);D3DDevice_SetRenderState_Deferred(D3DRS_PSCOMBINERCOUNT,0);
    D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSABCD,0x04200000);
    D3DDevice_SetRenderState_Deferred(D3DRS_PSFINALCOMBINERINPUTSEFG,0x00001400);
    D3DDevice_SetRenderState_Deferred(D3DRS_CULLMODE,D3DCULL_NONE);
    D3DDevice_SetRenderState_Deferred(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALL);
    D3DDevice_SetRenderState_Deferred(D3DRS_ZENABLE,D3DZB_TRUE);
    D3DDevice_SetRenderState_Deferred(D3DRS_ZWRITEENABLE,TRUE);
    D3DDevice_SetRenderState_Deferred(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
    D3DDevice_SetRenderState_Deferred(D3DRS_ALPHABLENDENABLE,FALSE);
    D3DDevice_SetRenderState_Deferred(D3DRS_ALPHATESTENABLE,FALSE);
    D3DDevice_SetRenderState_Deferred(D3DRS_FOGENABLE,FALSE);
    return 1;
}
static int draw_actual_object(void *context,const struct mac_input_frame *input,unsigned width,unsigned height)
{
    struct draw_context *c=(struct draw_context *)context;struct object_datum *object=object_get(c->object_index);
    struct object_definition *definition;struct model *model;real_matrix4x3 *live,skin[MAC_GPU_MAX_NODES];
    byte seen[MAC_GPU_MAX_GEOMETRIES];real lo[3],hi[3];unsigned long total;unsigned axis_x=0,axis_y=0,g,p,v,k;float extent_x,extent_y;unsigned char *pixels;unsigned channel_max[4]={0,0,0,0};
    struct mac_gpu_model_geometry *geoms;int ok=1;(void)input;
    if(!object){c->error="live object disappeared";return 0;}
    definition=object_definition_get(object->definition_index);if(!definition||definition->object.model.index==NONE){c->error="live object has no model";return 0;}
    model=model_definition_get(definition->object.model.index);if(!model||model->nodes.count<=0||model->nodes.count>MAC_GPU_MAX_NODES){c->error="model node palette is unsupported";return 0;}
    live=object_get_node_matrices(c->object_index);if(!live||!skin_matrix_build(model,live,skin,(unsigned)model->nodes.count)){c->error="live node palette does not match the model";return 0;}
    if(!get_geometry_indices(model,object,seen)||!collect_bounds(model,seen,skin,(unsigned)model->nodes.count,lo,hi,&total)){c->error="active compressed model geometry is malformed or empty";return 0;}
    for(k=1;k<3;++k)if(hi[k]-lo[k]>hi[axis_x]-lo[axis_x])axis_x=k;
    for(k=0;k<3;++k)if(k!=axis_x&&(axis_y==axis_x||hi[k]-lo[k]>hi[axis_y]-lo[axis_y]))axis_y=k;
    extent_x=hi[axis_x]-lo[axis_x];extent_y=hi[axis_y]-lo[axis_y];
    if(!(extent_x>0.00001f&&extent_y>0.00001f)){c->error="live model bounds are degenerate in the diagnostic view";return 0;}
    D3DDevice_Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1.0f,0);
    if(!setup_diagnostic_shader(width,height)){c->error="retail position shader setup failed";return 0;}
    D3DDevice_SetVertexData4f(9,0.25f,0.75f,0.5f,1.0f);
    geoms=(struct mac_gpu_model_geometry *)model->geometries.address;
    for(g=0;g<(unsigned)model->geometries.count&&ok;++g)if(seen[g])for(p=0;p<(unsigned)geoms[g].parts.count&&ok;++p){
        struct mac_gpu_model_part *part=(struct mac_gpu_model_part *)geoms[g].parts.address+p;struct mac_gpu_model_vertex *source=(struct mac_gpu_model_vertex *)part->vertex_buffer.base_address;
        struct mac_gpu_probe_vertex *copy;D3DVertexBuffer stream;D3DIndexBuffer *indices=(D3DIndexBuffer *)part->triangle_buffer.hardware_format;unsigned count;
        if(part->flags&MAC_GPU_MODEL_PART_STRIPPED)continue;
        if(!source||!indices||part->vertex_buffer.count<=0||part->vertex_buffer.count>MAC_GPU_MAX_VERTICES||
           part->triangle_buffer.type!=_triangle_buffer_type_precompiled_strip||part->triangle_buffer.count<=0||part->triangle_buffer.count>65533){c->error="only bounded owned model strips are supported";ok=0;break;}
        copy=(struct mac_gpu_probe_vertex *)platform_contiguous_alloc((unsigned long)part->vertex_buffer.count*sizeof(*copy),16,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);
        if(!copy){c->error="temporary model stream allocation failed";ok=0;break;}
        for(v=0;v<(unsigned)part->vertex_buffer.count;++v){real_point3d world;if(!skinned_vertex(source+v,skin,(unsigned)model->nodes.count,&world)){ok=0;break;}
            copy[v].position.x=(((real *)&world)[axis_x]-lo[axis_x])/extent_x*1.6f-0.8f;
            copy[v].position.y=(((real *)&world)[axis_y]-lo[axis_y])/extent_y*1.6f-0.8f;copy[v].position.z=0.5f;
            copy[v].color[0]=0.25f;copy[v].color[1]=0.75f;copy[v].color[2]=0.5f;copy[v].color[3]=1.0f;
            if(!finite_value(copy[v].position.x)||!finite_value(copy[v].position.y)){ok=0;break;}}
        if(!ok){platform_contiguous_free(copy);c->error="live skinning produced invalid transformed vertices";break;}
        memset(&stream,0,sizeof(stream));stream.Common=D3DCOMMON_TYPE_VERTEXBUFFER|1;
        stream.Data=PLATFORM_VIRTUAL_TO_PHYSICAL((unsigned long)copy);
        D3DDevice_SetStreamSource(0,&stream,sizeof(*copy));D3DDevice_SetIndices(indices,0);
        count=(unsigned)part->triangle_buffer.count+2;
        if((unsigned long)indices->Data!=(unsigned long)part->triangle_buffer.base_address||
           !mac_resource_range_is_allocated((const void *)indices->Data,count*sizeof(WORD))) {
            fprintf(stderr,"native model GPU index owner mismatch: buffer=%p data=%08lx base=%p count=%u owned=%d/2,%d/16,%d/full\n",
                (void *)indices,(unsigned long)indices->Data,part->triangle_buffer.base_address,count,
                mac_resource_range_is_allocated((const void *)indices->Data,2),
                mac_resource_range_is_allocated((const void *)indices->Data,16),
                mac_resource_range_is_allocated((const void *)indices->Data,count*sizeof(WORD)));
            D3DDevice_SetStreamSource(0,NULL,0);D3DDevice_SetIndices(NULL,0);platform_contiguous_free(copy);
            c->error="native model strip is outside its live index allocation";ok=0;break;
        }
        D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLESTRIP,count,(const WORD *)part->triangle_buffer.base_address);
        D3DDevice_SetStreamSource(0,NULL,0);D3DDevice_SetIndices(NULL,0);platform_contiguous_free(copy);
        ++c->result.parts;c->result.vertices+=(unsigned)part->vertex_buffer.count;
    }
    if(!ok){if(!c->error)c->error="indexed draw rejected a model part";return 0;}
    pixels=(unsigned char *)malloc((size_t)width*height*4);if(!pixels){c->error="readback allocation failed";return 0;}
    glReadPixels(0,0,(GLsizei)width,(GLsizei)height,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    for(v=0;v<width*height;++v){unsigned ch;for(ch=0;ch<4;++ch)if(pixels[v*4+ch]>channel_max[ch])channel_max[ch]=pixels[v*4+ch];
        if(pixels[v*4]||pixels[v*4+1]||pixels[v*4+2])++c->result.pixels;}
    if(!c->result.pixels)fprintf(stderr,"native model GPU empty readback: object=%ld max=%u,%u,%u,%u center=%u,%u,%u,%u\n",
        c->object_index,channel_max[0],channel_max[1],channel_max[2],channel_max[3],
        pixels[((height/2)*width+width/2)*4],pixels[((height/2)*width+width/2)*4+1],
        pixels[((height/2)*width+width/2)*4+2],pixels[((height/2)*width+width/2)*4+3]);
    free(pixels);return c->result.parts>0&&c->result.vertices>0;
}
int mac_native_model_gpu_probe_object(long object_index,struct mac_native_model_gpu_result *out,const char **error)
{
    struct draw_context c;if(out)memset(out,0,sizeof(*out));if(error)*error=NULL;memset(&c,0,sizeof(c));c.object_index=object_index;
    if(!mac_device_gpu_frame(draw_actual_object,&c)){if(error)*error=c.error?c.error:"native GPU frame or indexed model draw failed";if(out)*out=c.result;return 0;}
    if(out)*out=c.result;return 1;
}
