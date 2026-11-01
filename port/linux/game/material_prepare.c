/* Ask the existing model renderer for material variants during loading.
 * Descriptors supply sampler types without loading pixels or spending the
 * independent texture preparation budget. All state is local to this pass. */
#include "cseries.h"
#include "cseries/errors.h"
#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "math/real_math.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_console_vars.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_models.h"
#include "rasterizer/rasterizer_model_types.h"
#include "rasterizer/rasterizer_transparent_geometry.h"
#include "shaders/shader_definitions.h"
#include "material_prepare.h"
#include "../src/port_config.h"
#include <xtl.h>
#include <stdlib.h>
#include <string.h>

#ifndef HALO_GLES
#define MATERIAL_PREPARE_LIMIT 512
#define MATERIAL_BITMAP_LIMIT 4096
#define MATERIAL_DRAW_LIMIT 8192
static int preparing;
static unsigned prepared_draws;
static struct material_bitmap {
    struct bitmap_data *bitmap;
    D3DBaseTexture header;
} *bitmaps;
static unsigned bitmap_count;

/* Native draw-packet layout from rasterizer_xbox_transparent_geometry.c.
 * This standalone packet never enters the engine's sorted geometry queues. */
struct transparent_geometry_group {
    unsigned long geometry_flags;
    long object_index,source_object_index;
    struct shader *shader;
    short shader_permutation_index,pad12,effect_type,pad16;
    real effect_intensity;
    byte reserved1C[0x20];
    real_vector2d model_base_map_scale;
    long dynamic_triangle_buffer_index;
    struct triangle_buffer const *triangle_buffer;
    long first_triangle_index,triangle_count,dynamic_vertex_buffer_index;
    struct vertex_buffer const *vertex_buffer;
    struct bitmap_data const *lightmap;
    void const *node_matrices;
    short node_matrix_count;
    word pad66;
    struct render_lighting const *lighting;
    struct render_animation const *animation;
    real z_sort;
    real_point3d centroid;
    real_plane3d plane;
    long sorted_index;
    short previous_group_presorted_index,next_group_presorted_index;
    long active_camouflage_transparent_source_object_index;
    byte reserved9C;
    boolean cortana_hack;
    byte reserved9E[2];
};
typedef char material_packet_size[sizeof(struct transparent_geometry_group)==0xA0 ? 1 : -1];
#endif

int material_prepare_skip_shader(struct shader const *shader)
{
#ifndef HALO_GLES
    /* Extra layers can reference water, glass or effects requiring actual
     * geometry/render targets. They retain their observed-recipe path. */
    return preparing && (!shader || (shader->base.type!=5 && shader->base.type!=6));
#else
    (void)shader;
    return 0;
#endif
}

int material_prepare_bitmap(struct bitmap_data *bitmap, void **header)
{
#ifndef HALO_GLES
    unsigned index;
    if (!preparing) return 0;
    *header=NULL;
    if (!bitmap) return 1;
    for (index=0;index<bitmap_count;index++)
        if (bitmaps[index].bitmap==bitmap) {*header=&bitmaps[index].header;return 1;}
    if (bitmap_count==MATERIAL_BITMAP_LIMIT) return 1;
    bitmaps[index].bitmap=bitmap;
    if (!texture_cache_prepare_descriptor(bitmap,&bitmaps[index].header)) return 1;
    /* No pixels are dereferenced while the backend is in preparation mode. */
    bitmaps[index].header.Data=1;
    bitmap_count++;
    *header=&bitmaps[index].header;
    return 1;
#else
    (void)bitmap;(void)header;
    return 0;
#endif
}

int material_prepare_draw(void)
{
#ifndef HALO_GLES
    if (preparing) {
        if (prepared_draws<MATERIAL_DRAW_LIMIT) {
            prepared_draws++;
            render_material_prepare_draw();
        }
        return 1;
    }
#endif
    return 0;
}

void material_prepare_map(void)
{
#ifndef HALO_GLES
    struct rasterizer_window_begin_parameters saved_window;
    struct rasterizer_frame_statistics_globals saved_statistics;
    struct rasterizer_model_begin_parameters parameters;
    struct vertex_buffer vertices;
    struct transparent_geometry_group transparent;
    struct tag_iterator iterator;
    real_matrix4x3 nodes[2];
    real_rgb_color colors[16];
    real values[16];
    long index;
    unsigned count=0,variant;
    short saved_mode;
    if (!config_boolean("display.shader_prepare") || config_server_mode_enabled() ||
        config_boolean("debug.null_renderer")) return;
    bitmaps=malloc(MATERIAL_BITMAP_LIMIT*sizeof(*bitmaps));
    if (!bitmaps) return;
    memset(bitmaps,0,MATERIAL_BITMAP_LIMIT*sizeof(*bitmaps));
    if (!render_material_prepare_mode(1)) {free(bitmaps);bitmaps=NULL;return;}
    bitmap_count=0;
    prepared_draws=0;
    preparing=1;
    saved_window=global_window_parameters;
    saved_statistics=rasterizer_frame_statistics;
    saved_mode=rasterizer_debug_options.statistics_mode;
    rasterizer_debug_options.statistics_mode=0;
    memset(&global_window_parameters,0,sizeof(global_window_parameters));
    global_window_parameters.camera.forward.i=1.0f;
    global_window_parameters.fog.atmospheric_maximum_distance=1000.0f;
    global_window_parameters.fog.planar_maximum_distance=1000.0f;
    global_window_parameters.fog.planar_maximum_depth=1000.0f;
    global_window_parameters.fog.plane.n.k=1.0f;
    global_window_parameters.fog.plane.d=1.0f;
    memset(&parameters,0,sizeof(parameters));
    memset(values,0,sizeof(values));
    for (variant=0;variant<16;variant++) colors[variant]=*global_real_rgb_white;
    parameters.animation.colors=colors;
    parameters.animation.values=values;
    parameters.unique_identifier=NONE;
    nodes[0]=nodes[1]=*global_identity4x3;
    parameters.skinning.node_matrices=nodes;
    parameters.skinning.node_matrix_count=1;
    parameters.effect.source_object_index=NONE;
    parameters.radius=1.0f;
    parameters.base_map_scale.i=parameters.base_map_scale.j=1.0f;
    parameters.lighting.ambient_color.red=parameters.lighting.ambient_color.green=
        parameters.lighting.ambient_color.blue=0.5f;
    parameters.lighting.reflection_tint_color.alpha=1.0f;
    parameters.lighting.distant_light_count=2;
    for (variant=0;variant<NUMBEROF(parameters.lighting.point_light_indices);variant++)
        parameters.lighting.point_light_indices[variant]=NONE;
    memset(&vertices,0,sizeof(vertices));
    memset(&transparent,0,sizeof(transparent));
    transparent.object_index=transparent.source_object_index=NONE;
    transparent.dynamic_triangle_buffer_index=transparent.dynamic_vertex_buffer_index=NONE;
    transparent.sorted_index=NONE;
    transparent.previous_group_presorted_index=transparent.next_group_presorted_index=NONE;
    transparent.vertex_buffer=&vertices;
    transparent.node_matrices=global_identity4x3;
    transparent.node_matrix_count=1;
    transparent.lighting=&parameters.lighting;
    transparent.animation=&parameters.animation;
    transparent.model_base_map_scale=parameters.base_map_scale;
    tag_iterator_new(&iterator,'shdr');
    while ((index=tag_iterator_next(&iterator))!=NONE && count<MATERIAL_PREPARE_LIMIT && prepared_draws<MATERIAL_DRAW_LIMIT) {
        struct shader *shader=tag_get('shdr',index);
        unsigned variants;
        static short const types[]={0,1,4,5,6,7};
        if (!shader || shader->base.type<3 || shader->base.type>6) continue;
        count++;
        variants=shader->base.type<=4 ? 24 : 18;
        for (variant=0;variant<variants && prepared_draws<MATERIAL_DRAW_LIMIT;variant++) {
            unsigned choice=variant/3,fog=variant%3;
            vertices.type=shader->base.type<=4 ? types[2+choice/4] : types[choice];
            parameters.skinning.node_matrix_count=shader->base.type<=4 && (choice&1) ? 2 : 1;
            /* NONE is an accepted empty light slot. The count selects the
             * point-light program without borrowing or changing scene lights. */
            parameters.lighting.point_light_count=shader->base.type<=4 && (choice&2) ? 1 : 0;
            parameters.geometry_flags=fog ? 0 : (1UL<<2);
            global_window_parameters.fog.atmospheric_maximum_density=fog ? 1.0f : 0.0f;
            global_window_parameters.fog.planar_mode=fog==2;
            if (shader->base.type<=4) {
                rasterizer_models_begin(FALSE);
                rasterizer_model_begin(&parameters,FALSE);
                rasterizer_model_draw(shader,0,NULL,NONE,0,&vertices,NONE);
                rasterizer_model_end();
                rasterizer_models_end();
            } else {
                transparent.shader=shader;
                transparent.geometry_flags=parameters.geometry_flags;
                rasterizer_transparent_geometry_group_draw(&transparent,TRUE);
            }
        }
    }
    preparing=0;
    render_material_prepare_mode(0);
    global_window_parameters=saved_window;
    rasterizer_frame_statistics=saved_statistics;
    rasterizer_debug_options.statistics_mode=saved_mode;
    free(bitmaps);bitmaps=NULL;bitmap_count=0;
    error(_error_silent,"shader preparation: selected %u materials in %u draw variants",count,prepared_draws);
    prepared_draws=0;
#endif
}
