#include "mac_object_render_probe.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer_model_types.h"
#include "rasterizer/rasterizer_geometry.h"
#include <string.h>

int mac_object_render_probe_uv_check(void)
{
    static const short values[]={-32768,-32767,-1,0,1,16383,32767};
    struct model_vertex_compressed compressed[7];struct model_vertex_uncompressed decoded[7];
    unsigned int i;memset(compressed,0,sizeof(compressed));memset(decoded,0,sizeof(decoded));
    for(i=0;i<7;++i) { compressed[i].texcoord.x=values[i];compressed[i].texcoord.y=values[6-i]; }
    rasterizer_geometry_uncompress_vertices(_rasterizer_vertex_type_model_compressed,7,
        decoded,sizeof(decoded),compressed,sizeof(compressed));
    for(i=0;i<7;++i)
        if(decoded[i].texcoord.x!=mac_object_renderer_decode_uv(values[i])||
           decoded[i].texcoord.y!=mac_object_renderer_decode_uv(values[6-i]))return 0;
    return 1;
}

static int draw_live_object(void *context,struct mac_bsp_renderer *renderer)
{
    struct mac_object_render_probe *probe=(struct mac_object_render_probe *)context;
    unsigned int i;int result=1;
    if(!probe||probe->object_index==NONE||!probe->object_count||probe->object_count>4)return 0;
    memset(&probe->stats,0,sizeof(probe->stats));
    memset(probe->additional_stats,0,sizeof(probe->additional_stats));
    for(i=0;i<probe->object_count;++i) {
        long index=i?probe->additional_object_indices[i-1]:probe->object_index;
        struct mac_object_renderer_stats *stats=i?&probe->additional_stats[i-1]:&probe->stats;
        struct object_datum *object=object_get(index);struct object_definition *definition;
        struct model *model;real_matrix4x3 *matrices;
        if(!object)return 0;
        definition=object_definition_get(object->definition_index);if(!definition)return 0;
        if(definition->object.model.index==NONE)return 0;
        model=model_definition_get(definition->object.model.index);if(!model)return 0;
        if(model->nodes.count<=0||model->nodes.count>64)return 0;
        matrices=object_get_node_matrices(index);if(!matrices)return 0;
        if(!mac_object_renderer_draw_instance(renderer,model,object->object.region_permutations,
            MAXIMUM_REGIONS_PER_OBJECT,matrices,(unsigned int)model->nodes.count,stats))result=0;
    }
    ++probe->draw_calls;
    return result;
}

int mac_object_render_probe_attach(struct mac_runtime_view *view,struct mac_object_render_probe *probe,long object_index)
{
    return mac_object_render_probe_attach_many(view,probe,&object_index,1);
}

int mac_object_render_probe_attach_many(struct mac_runtime_view *view,struct mac_object_render_probe *probe,
    const long *object_indices,unsigned int count)
{
    unsigned int i;
    if(!view||!probe||!object_indices||!count||count>4)return 0;
    for(i=0;i<count;++i)if(object_indices[i]==NONE)return 0;
    memset(probe,0,sizeof(*probe));probe->object_index=object_indices[0];probe->object_count=count;
    for(i=1;i<count;++i)probe->additional_object_indices[i-1]=object_indices[i];
    if(!mac_runtime_view_set_overlay(view,draw_live_object,probe)) {
        probe->object_index=NONE;return 0;
    }
    return 1;
}

int mac_object_render_probe_detach(struct mac_runtime_view *view,struct mac_object_render_probe *probe)
{
    if(!view||!probe||!mac_runtime_view_set_overlay(view,NULL,NULL))return 0;
    probe->object_index=NONE;return 1;
}

int mac_object_render_probe_valid(const struct mac_object_render_probe *probe)
{
    unsigned int i;
    if(!probe||probe->object_index==NONE||!probe->object_count||probe->object_count>4||!probe->draw_calls)return 0;
    for(i=0;i<probe->object_count;++i)if(!mac_object_render_probe_object_valid(probe,i))return 0;
    return 1;
}

int mac_object_render_probe_object_valid(const struct mac_object_render_probe *probe,unsigned int index)
{
    const struct mac_object_renderer_stats *stats;
    if(!probe||index>=probe->object_count||index>3)return 0;
    stats=index?&probe->additional_stats[index-1]:&probe->stats;
    return stats->drawn_parts>0&&stats->submitted_vertices>0&&
        stats->invalid_parts==0&&stats->unsupported_shaders==0;
}

int mac_object_render_probe_object_textured(const struct mac_object_render_probe *probe,unsigned int index)
{
    const struct mac_object_renderer_stats *stats;
    if(!probe||index>=probe->object_count||index>3||!probe->draw_calls)return 0;
    stats=index?&probe->additional_stats[index-1]:&probe->stats;
    return stats->drawn_parts>0&&stats->textured_parts>0&&stats->submitted_vertices>0&&stats->invalid_parts==0;
}
