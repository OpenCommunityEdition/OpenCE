/* Diagnostic drawing of a real engine model instance. The records below
   mirror private declarations in source/models/models.c and are anchored by
   the boundary checker. */
#include "mac_object_renderer.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer_model_types.h"
#include "rasterizer/rasterizer_geometry.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"
#include <OpenGL/gl.h>
#include <string.h>

struct mac_model_shader_reference {
    struct tag_reference shader;
    short permutation_index;
    word pad;
    long unused[3];
};
struct mac_model_geometry { byte reserved[0x24]; struct tag_block parts; };
struct mac_model_geometry_part {
    unsigned long flags;
    short shader_index;
    char previous_part_index;
    char next_part_index;
    short centroid_primary_node_index;
    short centroid_secondary_node_index;
    real centroid_primary_node_weight;
    real centroid_secondary_node_weight;
    real_point3d centroid;
    struct tag_block uncompressed_vertices;
    struct tag_block compressed_vertices;
    struct tag_block triangles;
    struct triangle_buffer triangle_buffer;
    struct vertex_buffer vertex_buffer;
};
typedef char mac_model_part_size_check[sizeof(struct mac_model_geometry_part)==0x68?1:-1];
typedef char mac_model_shader_size_check[sizeof(struct mac_model_shader_reference)==0x20?1:-1];

#define MAC_MODEL_PART_STRIPPED 1UL
#define MAC_MAX_MODEL_REGIONS 32
#define MAC_MAX_MODEL_GEOMETRIES 256
#define MAC_MAX_MODEL_PARTS 32
#define MAC_MAX_MODEL_VERTICES 65535
#define MAC_MAX_MODEL_INDICES 196607
#define MAC_MODEL_HIGHEST_DETAIL_SLOT 4

static int finite_real(real value)
{ return value==value&&value<=1000000.0f&&value>=-1000000.0f; }

real mac_object_renderer_decode_uv(short value)
{ return ((real)value*2.0f+1.0f)*(1.0f/65535.0f); }

static int tag_block_sane(const struct tag_block *block,unsigned int maximum)
{ return block&&block->count>=0&&(unsigned long)block->count<=maximum&&(!block->count||block->address!=NULL); }

static int draw_part(struct mac_bsp_renderer *renderer,const struct model *model,
    const struct mac_model_geometry_part *part,const real_matrix4x3 *matrices,
    unsigned int matrix_count,struct mac_object_renderer_stats *stats)
{
    const struct mac_model_shader_reference *shader;
    const struct model_vertex_compressed *vertices;
    const uint16_t *indices;uint32_t vertex_count,index_count,i;int textured;float shader_u_scale=1.0f,shader_v_scale=1.0f;
    if(part->flags&MAC_MODEL_PART_STRIPPED)return 1;
    if(part->shader_index<0||part->shader_index>=model->shaders.count||!model->shaders.address||
       !part->vertex_buffer.base_address||!part->triangle_buffer.base_address||
       part->vertex_buffer.count<=0||(unsigned long)part->vertex_buffer.count>MAC_MAX_MODEL_VERTICES||
       part->triangle_buffer.count<=0||part->triangle_buffer.count>MAC_MAX_MODEL_INDICES)goto invalid;
    if(part->triangle_buffer.type==_triangle_buffer_type_precompiled_strip) {
        if(part->triangle_buffer.count>MAC_MAX_MODEL_INDICES-2)goto invalid;
        index_count=(uint32_t)part->triangle_buffer.count+2;
    } else if(part->triangle_buffer.type==_triangle_buffer_type_triangles) {
        if((uint32_t)part->triangle_buffer.count>MAC_MAX_MODEL_INDICES/3)goto invalid;
        index_count=(uint32_t)part->triangle_buffer.count*3;
    } else goto invalid;
    vertex_count=(uint32_t)part->vertex_buffer.count;
    vertices=(const struct model_vertex_compressed *)part->vertex_buffer.base_address;
    indices=(const uint16_t *)part->triangle_buffer.base_address;
    for(i=0;i<index_count;++i)if(indices[i]>=vertex_count)goto invalid;
    shader=(const struct mac_model_shader_reference *)model->shaders.address+part->shader_index;
    if((uint32_t)shader->shader.group_tag==UINT32_C(0x73636869)) {
        /* The chicago base layer can be sampled for this bounded view, but
           its following layers, alpha/blend state and animated functions are
           absent. Keep that semantic gap visible even when the bitmap binds. */
        if(!stats->unsupported_shader_semantics) {
            stats->first_semantic_shader=(uint32_t)shader->shader.index;
            stats->first_semantic_group=(uint32_t)shader->shader.group_tag;
        }
        ++stats->unsupported_shader_semantics;
    }
    textured=mac_bsp_renderer_bind_model_texture(renderer,(uint32_t)shader->shader.index,
        &shader_u_scale,&shader_v_scale);
    if(!textured) {
        if(!stats->unsupported_shaders) {
            stats->first_unsupported_shader=(uint32_t)shader->shader.index;
            stats->first_unsupported_group=(uint32_t)shader->shader.group_tag;
        }
        ++stats->unsupported_shaders;glActiveTexture(GL_TEXTURE0);glDisable(GL_TEXTURE_2D);glColor4f(1,0,1,1);
    } else {++stats->textured_parts;glColor4f(1,1,1,1);}
    glBegin(part->triangle_buffer.type==_triangle_buffer_type_precompiled_strip?GL_TRIANGLE_STRIP:GL_TRIANGLES);
    for(i=0;i<index_count;++i) {
        const struct model_vertex_compressed *v=&vertices[indices[i]];
        unsigned int n0=(unsigned int)v->nodes[0]/3U,n1=(unsigned int)v->nodes[1]/3U;
        real weight0=(real)v->node_weight*(1.0f/32767.0f),weight1=1.0f-weight0;
        real_point3d a,b,p;
        real uvx=mac_object_renderer_decode_uv(v->texcoord.x),uvy=mac_object_renderer_decode_uv(v->texcoord.y);
        if(v->nodes[0]%3||v->nodes[1]%3||n0>=matrix_count||n1>=matrix_count||
           v->node_weight<0||!finite_real(v->position.x)||
           !finite_real(v->position.y)||!finite_real(v->position.z)||
           !finite_real(uvx)||!finite_real(uvy)) { glEnd();goto invalid; }
        matrix4x3_transform_point(&matrices[n0],&v->position,&a);
        matrix4x3_transform_point(&matrices[n1],&v->position,&b);
        p.x=a.x*weight0+b.x*weight1;p.y=a.y*weight0+b.y*weight1;p.z=a.z*weight0+b.z*weight1;
        if(!finite_real(p.x)||!finite_real(p.y)||!finite_real(p.z)) { glEnd();goto invalid; }
        if(textured)glTexCoord2f(uvx*model->base_map_scale.i*shader_u_scale,
            uvy*model->base_map_scale.j*shader_v_scale);
        glVertex3f(p.x,p.y,p.z);++stats->submitted_vertices;
    }
    glEnd();if(glGetError()!=GL_NO_ERROR)return 0;
    ++stats->drawn_parts;return 1;
invalid:
    ++stats->invalid_parts;return 1;
}

int mac_object_renderer_draw_instance(struct mac_bsp_renderer *renderer,const struct model *model,
    const byte *region_permutations,unsigned int permutation_count,const real_matrix4x3 *node_matrices,
    unsigned int matrix_count,struct mac_object_renderer_stats *out_stats)
{
    struct mac_object_renderer_stats stats;unsigned int region_index;
    real_matrix4x3 skin_matrices[64];unsigned int node_index;
    byte seen[MAC_MAX_MODEL_GEOMETRIES];
    memset(&stats,0,sizeof(stats));memset(seen,0,sizeof(seen));if(out_stats)*out_stats=stats;
    if(!renderer||!model||!node_matrices||!matrix_count||model->nodes.count<=0||
       model->nodes.count>64||matrix_count!=(unsigned int)model->nodes.count||
       !tag_block_sane(&model->nodes,64)||
       !tag_block_sane(&model->regions,MAC_MAX_MODEL_REGIONS)||!tag_block_sane(&model->geometries,MAC_MAX_MODEL_GEOMETRIES)||
       !tag_block_sane(&model->shaders,32)||
       (model->regions.count&&(!region_permutations||permutation_count<(unsigned int)model->regions.count)))return 0;
    /* models.c builds each rasterizer palette entry as the live object node
       matrix multiplied by the model node's inverse bind matrix. Applying
       live matrices alone works for rigid one-node scenery, but explodes a
       skinned biped back into its unposed model-space offsets. */
    for(node_index=0;node_index<(unsigned int)model->nodes.count;++node_index) {
        const struct model_node *node=(const struct model_node *)model->nodes.address+node_index;
        matrix4x3_multiply(&node_matrices[node_index],&node->runtime_default_inverse_matrix,
            &skin_matrices[node_index]);
    }
    /* BSPs may leave unit 1 sampling a lightmap. Model diagnostics use only
       the true base map, so clear that inherited stage explicitly. */
    glActiveTexture(GL_TEXTURE1);glDisable(GL_TEXTURE_2D);glActiveTexture(GL_TEXTURE0);
    for(region_index=0;region_index<(unsigned int)model->regions.count;++region_index) {
        const struct model_region *region=(const struct model_region *)model->regions.address+region_index;
        const struct model_region_permutation *permutation;unsigned int selected;
        ++stats.regions;if(region_permutations[region_index]==0xFF)continue;
        if(!tag_block_sane(&region->permutations,32)||region_permutations[region_index]>=region->permutations.count) {
            ++stats.invalid_parts;continue;
        }
        permutation=(const struct model_region_permutation *)region->permutations.address+region_permutations[region_index];
        selected=(unsigned short)permutation->geometry_indices[MAC_MODEL_HIGHEST_DETAIL_SLOT];if((short)selected==-1)continue;
        if(selected>=(unsigned int)model->geometries.count||selected>=MAC_MAX_MODEL_GEOMETRIES) { ++stats.invalid_parts;continue; }
        if(seen[selected])continue;seen[selected]=1;
        {
            const struct mac_model_geometry *geometry=(const struct mac_model_geometry *)model->geometries.address+selected;
            unsigned int part_index;
            if(!tag_block_sane(&geometry->parts,MAC_MAX_MODEL_PARTS)) { ++stats.invalid_parts;continue; }
            for(part_index=0;part_index<(unsigned int)geometry->parts.count;++part_index) {
                const struct mac_model_geometry_part *part=(const struct mac_model_geometry_part *)geometry->parts.address+part_index;
                ++stats.parts;if(!draw_part(renderer,model,part,skin_matrices,matrix_count,&stats)) {
                    if(out_stats)*out_stats=stats;return 0;
                }
            }
        }
    }
    if(out_stats)*out_stats=stats;
    return stats.invalid_parts==0&&glGetError()==GL_NO_ERROR;
}
