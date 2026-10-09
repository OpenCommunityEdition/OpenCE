#include "harness.h"
#include <math.h>
#include <stdint.h>
typedef unsigned short word;
typedef struct { real x, y; } real_vector2d;
typedef struct { real red, green, blue; } real_rgb_color;
typedef struct { real alpha, red, green, blue; } real_argb_color;
typedef struct { real_vector3d n; real d; } real_plane3d;
typedef void real_matrix4x3;
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define VALID_INDEX(i,n) ((i) >= 0 && (i) < (n))
#define square_root sqrt
#include "types.inc"

static struct model model;
static struct model_geometry geometry;
static struct model_geometry_part parts[4];
static struct model_shader_reference references[4];
static struct shader_transparent_generic_definition energy_material;
static struct shader_transparent_glass_definition glass_material;
static struct shader opaque_material;
static struct model_region region;
static struct model_region_permutation permutation;
static int locks, submissions, iterator_position;
static short previous[32], next[32];
static real shell[5][8] = {{-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0},{0,0,2}};
static word indices[64] = {0,1,2,0,2,3,0,4,1,1,4,2,2,4,3,3,4,0};
static real sphere[6][8] = {{-.22f,0,.7f},{.22f,0,.7f},{0,-.22f,.7f},{0,.22f,.7f},{0,0,.48f},{0,0,.92f}};

#define TAG_BLOCK_GET_ELEMENT(b,i,t) ((t *)(b)->address + (i))
#define shader_definition_get(i) ((i)==0 ? (struct shader *)&energy_material : (i)==1 ? (struct shader *)&glass_material : &opaque_material)
#define model_definition_get(i) test_model_get()
#define shader_type_is_transparent(t) ((t)>=_shader_type_transparent_generic)
#define shader_type_is_valid_for_model(t) ((t)>=_shader_type_model)
#define shader_get_and_verify_type(s,t) (s)
#define model_data_error(m,e) ((void)0)
#define render_model_no_geometry FALSE
#define matrix4x3_transform_point(m,p,o) (*(o)=*(p))
#define rasterizer_model_draw(...) ((void)0)
#define rasterizer_environment_shadow_model_draw(...) ((void)0)
#define rasterizer_debug_model_vertices(...) ((void)0)
#define D3DLOCK_READONLY 128

static void tag_iterator_new(struct tag_iterator *iterator, long tag)
{
    iterator_position = 0;
}
static struct model *test_model_get(void)
{
    return &model;
}
static long tag_iterator_next(struct tag_iterator *iterator)
{
    return iterator_position++ == 0 ? 0 : NONE;
}
static long rasterizer_geometry_get_vertex_size(short type)
{
    return type == _rasterizer_vertex_type_model_compressed ? 32 : 68;
}
static void IDirect3DVertexBuffer8_Lock(void *buffer, long offset, long size, byte **out, long flags)
{
    locks++;
    *out = buffer;
}
#define IDirect3DIndexBuffer8_Lock IDirect3DVertexBuffer8_Lock
#define IDirect3DVertexBuffer8_Unlock(b) ((void)0)
#define IDirect3DIndexBuffer8_Unlock(b) ((void)0)
static void rasterizer_model_transparent_geometry_submit(struct shader *shader, short permutation_index,
    struct triangle_buffer const *triangles, long first_triangle, long triangle_count,
    struct vertex_buffer const *vertices, long offset, real_point3d const *centroid,
    struct render_sort_filth *sort)
{
    int i = submissions++;
    previous[i] = next[i] = NONE;
    if (sort)
    {
        sort->group_index = i;
        sort->previous_group_presorted_index_reference = previous + i;
        sort->next_group_presorted_index_reference = next + i;
    }
}
#include "under_test.inc"

static void initialize(void)
{
    int i;
    memset(&model, 0, sizeof(model));
    memset(parts, 0, sizeof(parts));
    memset(&energy_material, 0, sizeof(energy_material));
    memset(&glass_material, 0, sizeof(glass_material));
    locks = 0;
    energy_material.shader.type = _shader_type_transparent_generic;
    energy_material.generic.flags = FLAG(_shader_transparent_flag_two_sided_bit);
    energy_material.generic.framebuffer_blend_function = _framebuffer_blend_function_add;
    energy_material.generic.lens_flare.index = NONE;
    glass_material.shader.base.type = _shader_type_transparent_glass;
    glass_material.flags = FLAG(_shader_transparent_glass_flag_two_sided_bit);
    opaque_material.base.type = _shader_type_model;
    model.nodes.count = 1;
    model.geometries.count = 1; model.geometries.address = &geometry;
    model.shaders.count = 4; model.shaders.address = references;
    geometry.parts.count = 2; geometry.parts.address = parts;
    for (i=0; i<4; i++)
    {
        parts[i].shader_index = i;
        references[i].shader.index = i;
        parts[i].previous_part_index = parts[i].next_part_index = NONE;
        parts[i].vertex_buffer.type = _rasterizer_vertex_type_model_compressed;
    }
    parts[0].vertex_buffer.count = 6; parts[0].vertex_buffer.hardware_format = sphere;
    parts[1].vertex_buffer.count = 5; parts[1].vertex_buffer.hardware_format = shell;
    parts[1].triangle_buffer.type = _triangle_buffer_type_triangles;
    parts[1].triangle_buffer.count = 6; parts[1].triangle_buffer.hardware_format = indices;
    region.permutations.count = 1; region.permutations.address = &permutation;
    model.regions.count = 1; model.regions.address = &region;
    memset(&permutation, 0, sizeof(permutation));
}
static boolean recognizes(void)
{
    return rasterizer_transparent_geometry_is_enclosure((struct shader *)&glass_material,
        &parts[1].vertex_buffer, &parts[1].triangle_buffer,
        (struct shader *)&energy_material, &parts[0].vertex_buffer);
}
static void draw_frame(void)
{
    struct render_skinning skinning = {NULL, 1};
    char permutation_indices[] = {0};
    submissions = 0;
    render_model_parts(&model, permutation_indices, 1, &skinning, 1, 0, 0, 0);
    CHECK(submissions == 2, "two transparent parts");
    CHECK(next[0] == 1 && previous[1] == 0, "native links must include part zero");
    CHECK(glass_material.flags == FLAG(_shader_transparent_glass_flag_two_sided_bit), "retail two-sided glass retained");
}

int main(int argc, char **argv)
{
    const char *case_name = argc > 1 ? argv[1] : "closed";
    int i;
    initialize();
    CASE("closed") { CHECK(recognizes(), "closed shell"); }
    CASE("open") { parts[1].triangle_buffer.count = 5; CHECK(!recognizes(), "missing face"); }
    CASE("outside") { sphere[0][0] = 5; CHECK(!recognizes(), "outside core"); }
    CASE("reversed") { for(i=0;i<18;i+=3) { word swap=indices[i];indices[i]=indices[i+1];indices[i+1]=swap; } CHECK(!recognizes(), "reverse winding"); }
    CASE("invalid") {
        indices[0]=64; CHECK(!recognizes(), "bad index"); indices[0]=0;
        shell[0][0]=NAN; CHECK(!recognizes(), "invalid outer point"); shell[0][0]=-1;
        sphere[0][0]=NAN; CHECK(!recognizes(), "invalid inner point");
        CHECK(!rasterizer_transparent_encloses((byte*)shell,5,0,indices,6,FALSE,(byte*)sphere,6,32), "bad stride");
    }
    CASE("strips") {
        word triangles[18];long cursor=0;
        memcpy(triangles,indices,sizeof(triangles));
        for(i=0;i<6;i++) {
            word a=triangles[3*i],b=triangles[3*i+1];
            if(i) {
                if((cursor+2)&1){word swap=a;a=b;b=swap;}
                indices[cursor]=indices[cursor-1];cursor++;
                indices[cursor++]=a;
            }
            indices[cursor++]=a;indices[cursor++]=b;indices[cursor++]=triangles[3*i+2];
        }
        parts[1].triangle_buffer.type=_triangle_buffer_type_precompiled_strip;
        parts[1].triangle_buffer.count=cursor-2;CHECK(recognizes(), "triangle strip with degenerates");
    }
    CASE("uncompressed") {
        real outer[5][17],inner[6][17];
        for(i=0;i<5;i++)memcpy(outer[i],shell[i],12);
        for(i=0;i<6;i++)memcpy(inner[i],sphere[i],12);
        parts[1].vertex_buffer.hardware_format=outer;parts[0].vertex_buffer.hardware_format=inner;
        parts[1].vertex_buffer.type=parts[0].vertex_buffer.type=_rasterizer_vertex_type_model_uncompressed;
        CHECK(recognizes(), "uncompressed vertices");
    }
    CASE("materials") {
        energy_material.generic.framebuffer_blend_function=_framebuffer_blend_function_alpha_blend;
        CHECK(!recognizes(), "other blend untouched");initialize();
        glass_material.flags=0;CHECK(!recognizes(), "one-sided glass untouched");initialize();
        glass_material.reflection_type=_shader_transparent_glass_reflection_type_dynamic_mirror;
        CHECK(!recognizes(), "dynamic mirror untouched");initialize();
        energy_material.generic.extra_layers.count=1;CHECK(!recognizes(), "layers untouched");
    }
    CASE("part-zero") { models_fix_transparent_part_links(); CHECK(parts[0].next_part_index==1 && parts[1].previous_part_index==0, "link from part zero"); }
    CASE("glass-zero") {
        struct model_geometry_part swap=parts[0];parts[0]=parts[1];parts[1]=swap;
        models_fix_transparent_part_links();
        CHECK(parts[0].shader_index==0 && parts[1].shader_index==1 && parts[0].next_part_index==1, "no native link to zero");
    }
    CASE("opaque-part") {geometry.parts.count=3;models_fix_transparent_part_links();CHECK(parts[0].next_part_index==1 && parts[2].previous_part_index==NONE, "opaque base retained");}
    CASE("skinned") {model.nodes.count=2;models_fix_transparent_part_links();CHECK(!locks && parts[0].next_part_index==NONE, "skinned model retained");}
    CASE("authored-links") {parts[0].next_part_index=2;models_fix_transparent_part_links();CHECK(!locks && parts[0].next_part_index==2, "authored links retained");}
    CASE("additional-transparent") {geometry.parts.count=3;opaque_material.base.type=_shader_type_transparent_glass;models_fix_transparent_part_links();CHECK(!locks && parts[0].next_part_index==NONE, "ambiguous model retained");}
    CASE("frame-cost") {models_fix_transparent_part_links();CHECK(locks==3,"one recognizer call");for(i=0;i<1000;i++)draw_frame();CHECK(locks==3,"no per-frame recognition");models_fix_transparent_part_links();CHECK(locks==3,"already linked geometry retained");}
    CASE("map-reload") {models_fix_transparent_part_links();initialize();models_fix_transparent_part_links();CHECK(locks==3 && parts[0].next_part_index==1,"fresh map links");}
    CASE("render-links") {models_fix_transparent_part_links();draw_frame();}
    return 0;
}
