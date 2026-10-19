/*
STRUCTURE_CULLING.C

Asset-free geometry fixtures; the tested functions and math come from the game.
*/

#define boolean harness_boolean
#include "harness.h"
#undef boolean
typedef byte boolean;
#include <math.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#define NUMBEROF(array) (sizeof(array)/sizeof((array)[0]))
#define csstrcmp strcmp

typedef struct { real_vector3d n; real d; } real_plane3d;
typedef unsigned short word;
typedef union point2d point2d;
typedef struct { short y0, x0, y1, x1; } rectangle2d;
typedef struct { real r, g, b; } real_rgb_color;
typedef struct { real a, r, g, b; } real_argb_color;
#define MAXIMUM_RENDERED_DISTANT_LIGHTS 2
#define MAXIMUM_RENDERED_POINT_LIGHTS 2
#define NUMBER_OF_VERTICES_PER_TRIANGLE 3
#define SHADER_DEFINITION_TAG 0
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&((type *)(block)->address)[index])
typedef struct { real x0, x1, y0, y1, z0, z1; } real_rectangle3d;
#define square_root(x) sqrtf(x)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#include "types.inc"
/* the fake frame keeps the camera, frusta and visible-surface state read by the walk */
#define MAXIMUM_RENDERED_CLUSTERS 128
#define MAXIMUM_SURFACES_PER_STRUCTURE 131072
#define BIT_VECTOR_SIZE_IN_LONGS(count) (((count)+31)/32)
#define MAXIMUM_RENDERED_ENVIRONMENT_SURFACES 32768
static long pairing_visits;
#define BIT_VECTOR_TEST_FLAG(flags, index) ((void)((flags)==render.environment_surface_flags ? ++pairing_visits : 0), (((flags)[(index)/32] >> ((index)%32)) & 1UL))
#define BIT_VECTOR_SET_FLAG(flags, index, value) ((value) ? ((flags)[(index)/32] |= 1UL << ((index)%32)) : ((flags)[(index)/32] &= ~(1UL << ((index)%32))))
struct render_frustum { real_plane3d world_planes[6]; real_rectangle3d world_bounds; real_point3d world_vertices[5]; };
static long profile_entries[2], profile_exits[2];
struct profile_section { short index; };
static struct profile_section render_structure_visibility_subcluster_traversal = {0};
static struct profile_section render_structure_visibility_surface_traversal = {1};
#define profile_enter(section) (++profile_entries[(section).index])
#define profile_exit(section) (++profile_exits[(section).index])
struct rendered_cluster { short cluster_index; struct render_frustum frustum; };
static struct {
    struct { real_point3d position; real_vector3d forward; real z_far, z_near; rectangle2d viewport_bounds; } camera;
    struct render_frustum frustum;
    short cluster_index, rendered_cluster_count;
    struct rendered_cluster rendered_clusters[MAXIMUM_RENDERED_CLUSTERS];
    unsigned long environment_surface_flags[4096];
    long environment_surface_count;
    long frame_index;
} render;
static struct { real far_clip_distance; } rasterizer_globals;
static boolean structures_use_pvs_for_vs;
#define rendered_cluster_get(index) (&render.rendered_clusters[index])
static struct structure_bsp fixture_bsp;
static struct shader fixture_shader;
static boolean verify_ok=TRUE, allocation_fails;
static long log_count, fixture_malloc_calls, fixture_fail_malloc_call;
static size_t fixture_malloc_size;
static char build_log[512];
static long debug_triangles, debug_strings;
static char debug_text[256];
static void render_debug_triangle(boolean immediate, real_point3d const *a, real_point3d const *b, real_point3d const *c, real_argb_color const *color)
{
    static long frame=NONE, queued;
    if(frame!=render.frame_index) { frame=render.frame_index; queued=0; }
    CHECK(++queued<=8192,"shared rasterizer debug primitive capacity");
    CHECK(immediate && a->z==6 && b->z==6 && c->z==6 && color->a==0.5f && (color->r==1 || color->b==1),"debug geometry and tint");
    ++debug_triangles;
}
#define SHORT_MAX SHRT_MAX
#define _interface_font_terminal 0
#define _interface_color_table_dialog 0
static void interface_set_bitmap_text_draw_mode(
    short font, short style, short justification, unsigned long flags, short color_table, short color)
{
    CHECK(font == _interface_font_terminal && style == NONE && !justification && !flags &&
        color_table == _interface_color_table_dialog && !color, "terminal text mode");
}
static void rasterizer_draw_string(rectangle2d const *bounds, rectangle2d const *clip,
    point2d *cursor_reference, short height_adjust, char const *text)
{
    CHECK(bounds && bounds->y0 >= (render.camera.viewport_bounds.y1 - render.camera.viewport_bounds.y0) / 8,
        "debug text below maximum letterbox");
    CHECK(!bounds->x0 && bounds->x1 == SHORT_MAX && bounds->y1 == SHORT_MAX &&
        !clip && !cursor_reference && !height_adjust, "viewport-relative terminal text bounds");
    ++debug_strings;
    snprintf(debug_text, sizeof(debug_text), "%s", text);
}
struct fixture_vertex { real_point3d point; real_vector3d normal; byte pad[8]; };
/* the engine's accessor halts while no bsp is loaded (scenario.c) */
static short global_structure_bsp_index;
static struct structure_bsp *fixture_bsp_get(void) { CHECK(global_structure_bsp_index!=NONE,"bsp read while none is loaded"); return &fixture_bsp; }
#define global_structure_bsp_get() fixture_bsp_get()
#define structure_bsp_port_verify(bsp) (verify_ok)
#define tag_index_is_group(index, group) ((index)==0)
#define shader_definition_get(index) (&fixture_shader)
#define system_milliseconds() 100UL
#define environment_vertex_compressed_get_normal(vertex, out) (*(out)=((struct fixture_vertex const *)(vertex))->normal)
#define _error_silent 0
#define error(level, ...) (log_count++, snprintf(build_log,sizeof(build_log),__VA_ARGS__))
static void *fixture_malloc(size_t size) { CHECK(size<0x10000000UL,"engine allocation limit"); ++fixture_malloc_calls; fixture_malloc_size=MAX(fixture_malloc_size,size); return allocation_fails || fixture_malloc_calls==fixture_fail_malloc_call ? NULL : malloc(size); }
static void *fixture_realloc(void *data, size_t size) { CHECK(FALSE,"engine realloc forbidden for culling cache"); return NULL; }
static void fixture_free(void *data) { CHECK(data!=NULL,"engine debug_free rejects NULL"); free(data); }
#define malloc fixture_malloc
#define realloc fixture_realloc
#define free fixture_free
#include "under_test.inc"
#undef malloc
#undef realloc
#undef free

static struct structure_visibility_cluster fixture_clusters[2];
static struct structure_visibility_subcluster fixture_subclusters[1];
static struct structure_surface fixture_surfaces[4];
static struct structure_material fixture_materials[2];
static struct structure_lightmap fixture_lightmaps[1];
static struct fixture_vertex fixture_vertices[3];
static long fixture_indices[7];

static void fixture_load(boolean subclusters)
{
    long i;
    memset(&fixture_bsp,0,sizeof(fixture_bsp)); memset(fixture_clusters,0,sizeof(fixture_clusters));
    memset(fixture_materials,0,sizeof(fixture_materials));
    fixture_shader.base.type=3; verify_ok=TRUE; allocation_fails=FALSE;
    fixture_bsp.surfaces=(struct tag_block){4,fixture_surfaces,0};
    fixture_bsp.clusters=(struct tag_block){1,fixture_clusters,0};
    fixture_bsp.lightmaps=(struct tag_block){1,fixture_lightmaps,0};
    fixture_bsp.world_bounds=(real_rectangle3d){-10,10,-10,10,-10,10};
    fixture_lightmaps[0].materials=(struct tag_block){2,fixture_materials,0};
    fixture_materials[0].shader.index=0; fixture_materials[0].first_surface_index=0; fixture_materials[0].surface_count=2;
    fixture_materials[0].compressed_vertex_data.size=96; fixture_materials[0].compressed_vertex_data.address=fixture_vertices;
    fixture_materials[1]=fixture_materials[0]; fixture_materials[1].shader.index=NONE;
    fixture_materials[1].first_surface_index=0; fixture_materials[1].surface_count=3;
    for(i=0;i<4;++i) { fixture_surfaces[i].vertex_indices[0]=0; fixture_surfaces[i].vertex_indices[1]=1; fixture_surfaces[i].vertex_indices[2]=2; }
    fixture_surfaces[1].vertex_indices[2]=3;
    fixture_vertices[0].point=(real_point3d){0,0,0}; fixture_vertices[1].point=(real_point3d){1,0,0}; fixture_vertices[2].point=(real_point3d){0,1,0};
    for(i=0;i<3;++i) fixture_vertices[i].normal=(real_vector3d){0,0,-1};
    if(subclusters) {
        fixture_clusters[0].subclusters=(struct tag_block){1,fixture_subclusters,0};
        fixture_subclusters[0].world_bounds=fixture_bsp.world_bounds;
        fixture_subclusters[0].surface_indices=(struct tag_block){4,fixture_indices,0};
        for(i=0;i<4;++i) fixture_indices[i]=i;
    } else {
        fixture_clusters[0].surface_indices=(struct tag_block){7,fixture_indices,0};
        fixture_indices[0]=0; fixture_indices[1]=1; fixture_indices[2]=4;
        for(i=0;i<4;++i) fixture_indices[i+3]=i;
    }
    structure_culling_structure_bsp_loaded();
}

/* side walls at +/-1, near at 1, GPU far at 10 (fog may end at 4) */
static real_plane3d planes[6] = {
    {{-1, 0, 0}, 1}, {{1, 0, 0}, 1}, {{0, -1, 0}, 1}, {{0, 1, 0}, 1},
    {{0, 0, -1}, -1}, {{0, 0, 1}, 10}
};

static void fixture_frame(void)
{
    long i;
    memset(&render,0,sizeof(render));
    structures_use_pvs_for_vs=TRUE;
    render.camera.forward.k=1; render.camera.z_far=4; render.camera.z_near=1; rasterizer_globals.far_clip_distance=10;
    memcpy(render.frustum.world_planes,planes,sizeof(planes));
    render.frustum.world_planes[5].d=4;
    render.frustum.world_bounds=(real_rectangle3d){-1,1,-1,1,1,4};
    render.rendered_cluster_count=1;
    for(i=0;i<3;++i) fixture_vertices[i].point.z=6;
    structure_culling_structure_bsp_loaded();
}

static unsigned random_state = 0x712a901d;
static real random_real(real low, real high)
{
    random_state = random_state * 1664525u + 1013904223u;
    return low + (high - low) * (real)(random_state >> 8) / 16777216.0f;
}

static real_vector3d unit(real_vector3d v)
{
    real length = sqrtf(v.i*v.i + v.j*v.j + v.k*v.k);
    v.i /= length; v.j /= length; v.k /= length;
    return v;
}

static real_vector3d cross(real_vector3d a, real_vector3d b)
{
    return (real_vector3d){a.j*b.k-a.k*b.j, a.k*b.i-a.i*b.k, a.i*b.j-a.j*b.i};
}

static void check_verdict(real_point3d *points, long count, struct structure_culling_cone *cone, real_point3d camera, long bucket, long sample)
{
    long t;
    short verdict = structure_culling_cone_facing(cone, &camera);
    for (t = 0; t < count; ++t)
    {
        boolean away = structure_culling_triangle_faces_away(&camera, points+3*t, points+3*t+1, points+3*t+2);
        CHECK(verdict != _structure_culling_facing_away || away,
            "away verdict over-culls bucket %ld camera %ld triangle %ld", bucket, sample, t);
        CHECK(verdict != _structure_culling_facing_toward || !away,
            "toward verdict contradicts bucket %ld camera %ld triangle %ld", bucket, sample, t);
    }
}

static void random_buckets(void)
{
    long b, t, s, count;
    real_point3d points[64*3];
    for (b = 0; b < 200; ++b)
    {
        struct structure_culling_cone cone;
        real_vector3d axis = unit((real_vector3d){random_real(-1, 1), random_real(-1, 1), random_real(-1, 1)});
        real_vector3d u = unit(cross(axis, fabsf(axis.k) < .9f ? (real_vector3d){0, 0, 1} : (real_vector3d){1, 0, 0}));
        real_vector3d v = cross(axis, u);
        real_point3d center = {random_real(-20, 20), random_real(-20, 20), random_real(-20, 20)};
        real size = random_real(.5f, 5);
        count = 1 + (long)random_real(0, 64);
        for (t = 0; t < count; ++t)
        {
            real angle = random_real(0, 80) * .01745329252f;
            real azimuth = random_real(0, 6.283185307f);
            real_vector3d n = {axis.i*cosf(angle) + sinf(angle)*(u.i*cosf(azimuth)+v.i*sinf(azimuth)),
                axis.j*cosf(angle) + sinf(angle)*(u.j*cosf(azimuth)+v.j*sinf(azimuth)),
                axis.k*cosf(angle) + sinf(angle)*(u.k*cosf(azimuth)+v.k*sinf(azimuth))};
            real_vector3d a = unit(cross(n, fabsf(n.k) < .9f ? (real_vector3d){0, 0, 1} : (real_vector3d){1, 0, 0}));
            real_vector3d c = cross(n, a);
            real_point3d p = {center.x+random_real(-size+.2f, size-.2f), center.y+random_real(-size+.2f, size-.2f), center.z+random_real(-size+.2f, size-.2f)};
            points[3*t] = p;
            points[3*t+1] = (real_point3d){p.x+.1f*a.i, p.y+.1f*a.j, p.z+.1f*a.k};
            points[3*t+2] = (real_point3d){p.x+.1f*c.i*STRUCTURE_CULLING_FRONT_SIGN, p.y+.1f*c.j*STRUCTURE_CULLING_FRONT_SIGN, p.z+.1f*c.k*STRUCTURE_CULLING_FRONT_SIGN};
        }
        structure_culling_cone_build(&cone, count, points);
        for (s = 0; s < 2000; ++s)
        {
            real_vector3d direction = unit((real_vector3d){random_real(-1, 1), random_real(-1, 1), random_real(-1, 1)});
            real distance = s < 500 ? cone.radius * 1.0001f : s < 1000 ? cone.radius * random_real(0, 1) : cone.radius * random_real(1, 20);
            real_point3d camera = {cone.center.x+direction.i*distance, cone.center.y+direction.j*distance, cone.center.z+direction.k*distance};
            check_verdict(points, count, &cone, camera, b, s);
        }
    }
}

int main(int argc, char **argv)
{
    char const *case_name;
    real_point3d p[3] = {{0, 0, 2}, {.5f, 0, 2}, {0, .5f, 2}};
    real_point3d camera = {0, 0, 2-STRUCTURE_CULLING_FRONT_SIGN};
    struct structure_culling_cone cone;
    real_rectangle3d box = {-.5f, .5f, -.5f, .5f, 2, 3};
    long i;
    CHECK(argc == 2, "one named case");
    case_name = argv[1];
    CASE("default-mask")
    {
        CHECK(structure_culling_tests==31,"all five tests enabled by default, got %ld",structure_culling_tests);
        return 0;
    }
    CASE("debug-letterbox")
    {
        fixture_load(TRUE); fixture_frame(); structure_culling_debug(TRUE);
        for (i = 0; i < 3; ++i) {
            render.camera.viewport_bounds.y0 = i * 120;
            render.camera.viewport_bounds.y1 = i * 120 + (i + 1) * 240;
            structure_culling_render_debug();
        }
        CHECK(debug_strings == 3, "debug text emitted for each viewport");
        return 0;
    }
    CASE("debug-stale-data")
    {
        fixture_load(TRUE); fixture_frame(); structure_culling_debug(TRUE);
        --fixture_bsp.surfaces.count;
        structure_culling_render_debug();
        CHECK(!debug_strings && !debug_triangles, "stale cache draws neither counters nor triangles");
        return 0;
    }
    CASE("walk-profile")
    {
        long mode;
        for (mode = 0; mode < 2; ++mode) {
            fixture_load(mode == 0); fixture_frame();
            CHECK(structure_culling_traverse(&fixture_bsp), "profiled walk handled");
            CHECK(profile_entries[mode] == 1 && profile_exits[mode] == 1,
                "matching traversal profile entered and exited");
            structure_culling_set_tests(0);
            CHECK(!structure_culling_traverse(&fixture_bsp), "disabled walk declines");
            CHECK(profile_entries[mode] == 1 && profile_exits[mode] == 1,
                "declined walk leaves original profiling alone");
            structure_culling_set_tests(31);
        }
        return 0;
    }
    CASE("missing-subcluster-warning")
    {
        long before;
        fixture_load(FALSE); fixture_frame(); before = log_count;
        fixture_visibility_surface_walk(&fixture_bsp);
        CHECK(log_count == before + 1 && strstr(build_log, "needs to be reimported"),
            "active culling preserves missing-subcluster warning");
        fixture_visibility_surface_walk(&fixture_bsp);
        CHECK(log_count == before + 1, "missing-subcluster warning only once");
        return 0;
    }
    CASE("overlay-truncation")
    {
        static struct structure_surface surfaces[6146];
        static struct structure_culling_pairing pairings[6146];
        fixture_load(TRUE); fixture_frame();
        for(i=0;i<6146;++i) { surfaces[i]=fixture_surfaces[0]; pairings[i]=(struct structure_culling_pairing){i,0,TRUE}; BIT_VECTOR_SET_FLAG(structure_culling_recorded[0],i,TRUE); }
        fixture_bsp.surfaces=(struct tag_block){6146,surfaces,0}; structure_culling.surface_address=surfaces; structure_culling.surface_count=6146;
        structure_culling.pairings=pairings; structure_culling.pairing_count=6146; structure_culling_debug(TRUE);
        structure_culling_render_debug();
        CHECK(debug_triangles==6144,"overlay draws exactly reserved cap, got %ld",debug_triangles);
        CHECK(strstr(debug_text,"(2 more not drawn)"),"overlay reports truncated count: %s",debug_text);
        return 0;
    }
    CASE("compact-allocation-failure")
    {
        real_point3d points[3];
        fixture_fail_malloc_call=2; fixture_load(FALSE);
        CHECK(fixture_malloc_calls==2 && structure_culling_data_matches(&fixture_bsp) && structure_culling.pairing_count==3,"failed compact malloc keeps valid build cache");
        CHECK(structure_culling.bytes==fixture_malloc_size,"fallback reports actual retained build bytes");
        structure_culling_pairing_points(&fixture_bsp,structure_culling.pairings,points);
        CHECK(points[1].x==1,"fallback cache pointers remain usable");
        fixture_fail_malloc_call=0; fixture_malloc_size=0; structure_culling_structure_bsp_loaded();
        CHECK(fixture_malloc_calls==4 && structure_culling.bytes<fixture_malloc_size,"successful compaction allocates smaller cache");
        CHECK((void *)structure_culling.materials==structure_culling.allocation && (void *)structure_culling.pairings==(byte *)structure_culling.allocation+24+32+72*structure_culling.bucket_count,"compact pointers rebased");
        structure_culling_dispose(); return 0;
    }
    CASE("rejected-bucket-visits")
    {
        real_rectangle3d outside={20,21,0,1,6,6};
        fixture_load(TRUE); fixture_frame(); structure_culling_tests=2;
        structure_culling.clusters[0].bucket_count=1; structure_culling.buckets[0].pairing_count=1;
        for(i=0;i<3;++i) {
            struct structure_culling_bucket *bucket=&structure_culling.buckets[0];
            bucket->source_bounds=i==0 ? &outside : NULL;
            bucket->bounds=i==1 ? outside : (real_rectangle3d){0,1,0,1,6,6};
            bucket->cone.axis=(real_vector3d){0,0,1}; bucket->cone.cutoff=1; bucket->facing_eligible=i==2;
            structure_culling_debug(FALSE); pairing_visits=0;
            CHECK(structure_culling_traverse(&fixture_bsp) && pairing_visits==0,"rejected bucket reason %ld visits no pairings while debug off",i);
            structure_culling_debug(TRUE); pairing_visits=0;
            CHECK(structure_culling_traverse(&fixture_bsp) && pairing_visits==1,"debug still visits rejected member");
        }
        return 0;
    }
    CASE("build-budget")
    {
        static struct structure_visibility_cluster clusters[8];
        static struct structure_visibility_subcluster subclusters[4096];
        long indices[128]={0}, pass, calls;
        fixture_load(TRUE); allocation_fails=TRUE; structure_culling_tests=0;
        for(i=0;i<4096;++i) { subclusters[i]=fixture_subclusters[0]; subclusters[i].surface_indices=(struct tag_block){128,indices,0}; }
        fixture_bsp.clusters=(struct tag_block){8,clusters,0};
        for(pass=0;pass<2;++pass) {
            for(i=0;i<8;++i) { clusters[i]=fixture_clusters[0]; clusters[i].subclusters=(struct tag_block){pass ? 4096 : 1024,subclusters,0}; }
            calls=fixture_malloc_calls; structure_culling_structure_bsp_loaded();
            CHECK(fixture_malloc_calls==calls && !structure_culling.allocation,"oversized build declines without malloc, even at mask zero");
        }
        return 0;
    }
    CASE("mirror-far")
    {
        long pass;
        fixture_load(TRUE); fixture_frame(); render.camera.z_near=0;
        for(i=0;i<3;++i) fixture_vertices[i].point.z=12;
        structure_culling_structure_bsp_loaded();
        structure_visibility_traverse_subclusters(&fixture_bsp);
        CHECK(render.environment_surface_flags[0]==15,"original submits mirror geometry past perpendicular far");
        for(pass=0;pass<2;++pass) {
            if(pass) for(i=0;i<structure_culling.bucket_count;++i) structure_culling.buckets[i].bounds=fixture_bsp.world_bounds;
            memset(render.environment_surface_flags,0,sizeof(render.environment_surface_flags)); render.environment_surface_count=0;
            CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_flags[0]==15,"mirror ignores synthesized far in %s test",pass ? "triangle" : "box");
        }
        return 0;
    }
    CASE("dispose-empty")
    {
        structure_culling_dispose();
        structure_culling_dispose();
        CHECK(!structure_culling.allocation && !structure_culling.structure,"empty disposal remains cleared");
        return 0;
    }
    CASE("debug-record")
    {
        struct structure_culling_cluster clusters[2];
        struct structure_culling_bucket buckets[3];
        struct structure_culling_pairing pairings[5]={{0,0,TRUE},{1,0,TRUE},{1,0,TRUE},{2,0,TRUE},{0,0,TRUE}};
        fixture_load(TRUE); fixture_surfaces[1].vertex_indices[2]=2; fixture_frame(); structure_culling_set_tests(3);
        for(i=0;i<3;++i) {
            buckets[i]=structure_culling.buckets[0]; buckets[i].source_bounds=NULL;
            buckets[i].first_pairing=i==0 ? 0 : i==1 ? 3 : 4; buckets[i].pairing_count=i==0 ? 3 : 1;
        }
        buckets[0].bounds=(real_rectangle3d){20,21,0,1,6,6};
        buckets[1].cone.axis=(real_vector3d){0,0,1}; buckets[1].cone.cutoff=1;
        buckets[2].facing_eligible=FALSE;
        for(i=0;i<2;++i) { clusters[i]=structure_culling.clusters[0]; clusters[i].first_bucket=i*2; clusters[i].bucket_count=i==0 ? 2 : 1; }
        structure_culling.clusters=clusters; structure_culling.buckets=buckets; structure_culling.pairings=pairings;
        structure_culling.pairing_count=5;
        render.rendered_cluster_count=2; render.rendered_clusters[1].cluster_index=1;
        structure_culling_debug(TRUE);
        CHECK(structure_culling_traverse(&fixture_bsp),"debug walk handled");
        CHECK(structure_culling_recorded[0][0]==2 && structure_culling_recorded[1][0]==4,"exact culled record, drawn-through-another-cluster excluded");
        CHECK(structure_culling_submitted==1 && structure_culling_outside==1 && structure_culling_facing==1,"unique surface counts, drawn wins");
        structure_culling_render_debug();
        CHECK(debug_triangles==2 && debug_strings==1 && strstr(debug_text,"1 drawn, 1 outside, 1 facing away"),"debug emits culled geometry and counts");
        render.environment_surface_flags[0]|=2; structure_culling_render_debug();
        CHECK(debug_triangles==3 && debug_strings==2,"renderer also excludes recorded surfaces marked drawn");
        for(i=0;i<6144;++i) structure_culling_render_debug();
        CHECK(debug_triangles==6144,"triangle cap is shared by windows in one frame");
        ++render.frame_index; structure_culling_render_debug();
        CHECK(debug_triangles==6145,"new frame resets triangle budget");
        structure_culling_debug(FALSE); memset(render.environment_surface_flags,0,sizeof(render.environment_surface_flags)); render.environment_surface_count=0;
        memset(structure_culling_recorded,0,sizeof(structure_culling_recorded));
        structure_culling_recorded[0][0]=1;
        structure_culling_recorded[0][4095]=0xa5;
        structure_culling_recorded[1][4095]=0x5a;
        CHECK(structure_culling_traverse(&fixture_bsp),"non-debug walk handled");
        CHECK(structure_culling_recorded[0][0]==1 && structure_culling_recorded[1][0]==0,"record is untouched while off, including submitted members");
        CHECK(structure_culling_recorded[0][4095]==0xa5 && structure_culling_recorded[1][4095]==0x5a,"no vector clears while off");
        structure_culling_render_debug(); CHECK(debug_triangles==6145 && debug_strings==6147,"no drawing while off");
        structure_culling_set_tests(0);
        CHECK(!structure_culling_traverse(&fixture_bsp),"mask zero declines");
        CHECK(structure_culling_recorded[0][4095]==0xa5 && structure_culling_recorded[1][4095]==0x5a,"mask zero does no vector work while off");
        CHECK(!structure_culling_submitted && !structure_culling_outside && !structure_culling_facing,"declined walk resets counts");
        return 0;
    }
    CASE("cluster-order")
    {
        short count, order[128], seen[128], j;
        real distances[128];
        for(count=0;count<=128;++count) {
            for(i=0;i<count;++i) { order[i]=count-1-i; distances[i]=(i*17)%11; seen[i]=0; }
            structure_culling_order_clusters(order,distances,count);
            for(i=0;i<count;++i) {
                CHECK(order[i]>=0 && order[i]<count,"order range"); ++seen[order[i]];
                if(i) CHECK(distances[order[i-1]]<=distances[order[i]],"ascending distance");
                if(i && distances[order[i-1]]==distances[order[i]]) CHECK(order[i-1]>order[i],"stable ties");
            }
            for(j=0;j<count;++j) CHECK(seen[j]==1,"unchanged entries");
        }
        return 0;
    }
    CASE("cluster-order-per-call")
    {
        short a[3]={0,1,2}, b[3]={0,1,2}; real first[3]={3,1,2}, second[3]={1,3,2};
        structure_culling_order_clusters(a,first,3); structure_culling_order_clusters(b,second,3);
        CHECK(a[0]==1 && a[1]==2 && a[2]==0,"first call");
        CHECK(b[0]==0 && b[1]==2 && b[2]==1,"independent second call"); return 0;
    }
    CASE("stale-structure")
    {
        struct structure_bsp other;
        fixture_load(TRUE); other=fixture_bsp;
        CHECK(structure_culling_data_matches(&fixture_bsp),"loaded identity matches");
        CHECK(!structure_culling_data_matches(&other),"different structure");
        fixture_bsp.surfaces.address=NULL; CHECK(!structure_culling_data_matches(&fixture_bsp),"reused structure address");
        fixture_bsp.surfaces.address=fixture_surfaces; --fixture_bsp.surfaces.count;
        CHECK(!structure_culling_data_matches(&fixture_bsp),"changed surface count");
        ++fixture_bsp.surfaces.count; structure_culling_dispose();
        CHECK(!structure_culling_data_matches(&fixture_bsp),"disposed data"); return 0;
    }
    CASE("walk-fallback")
    {
        fixture_load(TRUE); fixture_frame();
        CHECK(structure_culling_traverse(&fixture_bsp),"active walk handles frame");
        structure_culling_set_tests(0); render.environment_surface_count=17; render.environment_surface_flags[0]=0xa5;
        CHECK(!structure_culling_traverse(&fixture_bsp),"disabled walk with valid cached data");
        CHECK(!structure_culling_traverse(NULL),"disabled walk does not dereference structure");
        CHECK(render.environment_surface_count==17 && render.environment_surface_flags[0]==0xa5,"disabled walk touches nothing");
        structure_culling_set_tests(25); fixture_bsp.surfaces.count=3;
        CHECK(!structure_culling_traverse(&fixture_bsp),"stale walk falls back");
        CHECK(render.environment_surface_count==17 && render.environment_surface_flags[0]==0xa5,"stale walk touches nothing");
        structure_culling_dispose(); CHECK(!structure_culling_traverse(NULL),"no data falls back"); return 0;
    }
    CASE("walk-planes")
    {
        fixture_load(TRUE); fixture_frame();
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_count==4,"past fog, before GPU far, untestable kept");
        fixture_vertices[0].point.z=fixture_vertices[1].point.z=fixture_vertices[2].point.z=11;
        structure_culling_structure_bsp_loaded(); memset(render.environment_surface_flags,0,sizeof(render.environment_surface_flags)); render.environment_surface_count=0;
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_flags[0]==10,"past GPU far drops testable, keeps untestable");
        fixture_frame();
        for(i=0;i<structure_culling.bucket_count;++i) structure_culling.buckets[i].bounds=fixture_bsp.world_bounds;
        for(i=0;i<3;++i) fixture_vertices[i].point.z=0;
        memset(render.environment_surface_flags,0,sizeof(render.environment_surface_flags)); render.environment_surface_count=0;
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_flags[0]==10,"partial bucket triangle near-plane rejection");
        fixture_frame(); structures_use_pvs_for_vs=FALSE; render.rendered_clusters[0].frustum=render.frustum;
        render.rendered_clusters[0].frustum.world_planes[0].d=-100;
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_count==0,"clipped cluster frustum selected");
        render.cluster_index=NONE;
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_count==4,"camera outside uses full frustum"); return 0;
    }
    CASE("source-fog-rejected")
    {
        fixture_load(TRUE);
        fixture_frame();
        fixture_subclusters[0].world_bounds=(real_rectangle3d){0,1,0,1,5,7};
        structure_culling_set_tests(0);
        CHECK(!structure_culling_traverse(&fixture_bsp),"mask zero runs original walk");
        structure_visibility_traverse_subclusters(&fixture_bsp);
        CHECK(render.environment_surface_count==0,"original omits source wholly past fog");
        structure_culling_set_tests(25);
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_count==0,"culling also omits source wholly past fog");
        CHECK(structure_culling_outside==0,"debug rejection counts stay idle while off");
        return 0;
    }
    CASE("source-fog-straddling")
    {
        fixture_load(TRUE);
        fixture_frame();
        fixture_subclusters[0].world_bounds=(real_rectangle3d){0,1,0,1,3,7};
        structure_culling_set_tests(0);
        CHECK(!structure_culling_traverse(&fixture_bsp),"mask zero runs original walk");
        structure_visibility_traverse_subclusters(&fixture_bsp);
        CHECK(render.environment_surface_count==4,"original keeps whole source straddling fog");
        memset(render.environment_surface_flags,0,sizeof(render.environment_surface_flags));
        render.environment_surface_count=0;
        structure_culling_set_tests(25);
        CHECK(structure_culling.bucket_count>1 && structure_culling.buckets[0].bounds.z0==6,"split bucket is wholly past fog");
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_count==4,"source test keeps finer buckets past fog, inside GPU far");
        fixture_load(FALSE);
        fixture_frame();
        fixture_bsp.world_bounds.z0=5;
        fixture_bsp.world_bounds.z1=7;
        structure_culling_structure_bsp_loaded();
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_count==3,"surface lists have no clamped source-box test");
        return 0;
    }
    CASE("walk-unit-forward")
    {
        fixture_load(TRUE); fixture_frame();
        for(i=0;i<3;++i) fixture_vertices[i].point.z=9;
        structure_culling_structure_bsp_loaded();
        render.camera.position.z=3; render.camera.forward.k=2;
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_count==4,"unit forward keeps triangles before camera z=3 plus GPU far=10"); return 0;
    }
    CASE("walk-facing")
    {
        fixture_load(TRUE); fixture_frame(); structure_culling_set_tests(27);
        render.camera.position.z=20;
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_flags[0]==14,"eligible bucket away, other shaders and invalid kept");
        fixture_frame(); structure_culling_set_tests(29); render.camera.position.z=20;
        structure_culling.buckets[0].cone.cutoff=-1;
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_flags[0]==14,"mixed bucket triangle facing"); return 0;
    }
    CASE("walk-near-first")
    {
        struct structure_culling_cluster clusters[2]; struct structure_culling_bucket buckets[2];
        struct structure_culling_pairing pairings[2]={{0,0,TRUE},{2,1,TRUE}};
        fixture_load(TRUE); fixture_frame();
        for(i=0;i<2;++i) { clusters[i]=structure_culling.clusters[0]; clusters[i].first_bucket=i; clusters[i].bucket_count=1; buckets[i]=structure_culling.buckets[0]; buckets[i].first_pairing=i; buckets[i].pairing_count=1; }
        clusters[0].bounds=(real_rectangle3d){100,101,0,1,6,6}; clusters[1].bounds=(real_rectangle3d){0,1,0,1,6,6};
        structure_culling.clusters=clusters; structure_culling.buckets=buckets; structure_culling.pairings=pairings;
        fixture_bsp.clusters.count=2; render.rendered_cluster_count=2; render.rendered_clusters[1].cluster_index=1;
        render.environment_surface_count=MAXIMUM_RENDERED_ENVIRONMENT_SURFACES-1;
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_flags[0]==4,"near cluster wins last slot");
        CHECK(render.environment_surface_count==MAXIMUM_RENDERED_ENVIRONMENT_SURFACES,"submission cap");
        structure_culling_set_tests(9); render.environment_surface_count=MAXIMUM_RENDERED_ENVIRONMENT_SURFACES-1; render.environment_surface_flags[0]=0;
        CHECK(structure_culling_traverse(&fixture_bsp) && render.environment_surface_flags[0]==1,"mask without near-first preserves order"); return 0;
    }
    CASE("set-tests")
    {
        fixture_load(TRUE); structure_culling_set_tests(17);
        CHECK(structure_culling_tests==17 && log_count==2,"finer change rebuilds");
        structure_culling_set_tests(1); CHECK(log_count==2 && structure_culling_tests==1,"other bits do not rebuild"); return 0;
    }
    CASE("set-tests-unloaded")
    {
        global_structure_bsp_index=NONE; structure_culling_set_tests(0);
        CHECK(structure_culling_tests==0 && !log_count && !structure_culling.allocation,"no bsp: the mask is kept for the next load"); return 0;
    }
    CASE("console-local") { CHECK(hs_expression_changes_no_game("(structure_culling 25)") && hs_expression_changes_no_game("(debug_structure_culling 1)"),"console switches change only local rendering"); return 0; }
    CASE("load-small-group-scratch")
    {
        long mode;
        for(mode=0;mode<2;++mode) {
            long maximum_group_count=99;
            struct structure_visibility_subcluster subclusters[3];
            long indices[4]={0,2,3,0};
            long groups[16]={0,1,1,0, 0,1,4,0,2,3,0, 0,1,9,2,0};
            fixture_load(mode);
            if(mode) {
                for(i=0;i<3;++i) {
                    subclusters[i].world_bounds=fixture_bsp.world_bounds;
                    subclusters[i].surface_indices=(struct tag_block){i==0 ? 1 : i==1 ? 4 : 2,indices,0};
                }
                fixture_clusters[0].subclusters=(struct tag_block){3,subclusters,0};
            } else fixture_clusters[0].surface_indices=(struct tag_block){16,groups,0};
            CHECK(structure_culling_build_lists(&fixture_bsp,NULL,&maximum_group_count)==7,"count pass sums mixed group sizes");
            CHECK(maximum_group_count==4,"recorded maximum must be four, got %ld",maximum_group_count);
            structure_culling_structure_bsp_loaded();
            /* base 76, seven output slots at 80, four scratch slots at 80 */
            CHECK(fixture_malloc_size==956,"scratch must use largest group: %lu bytes, expected 956",(unsigned long)fixture_malloc_size);
            CHECK(structure_culling.allocation && structure_culling.pairing_count==7,"all mixed-group pairings retained");
            structure_culling_dispose();
        }
        return 0;
    }
    CASE("load-subclusters")
    {
        long j, found=0;
        fixture_load(TRUE);
        CHECK(structure_culling.allocation && structure_culling.pairing_count==4,"all subcluster pairings kept");
        CHECK(structure_culling.bucket_count>=2,"eligibility split includes invalid vertices");
        for(j=0;j<structure_culling.bucket_count;++j) {
            struct structure_culling_bucket *bucket=&structure_culling.buckets[j];
            long k;
            for(k=bucket->first_pairing;k<bucket->first_pairing+bucket->pairing_count;++k) {
                struct structure_culling_pairing *pairing=&structure_culling.pairings[k];
                CHECK(bucket->facing_eligible==(pairing->testable && pairing->material_index==0),"bucket eligibility matches pairing");
                if(!pairing->testable) CHECK(bucket->cone.cutoff==-1,"untestable bucket has no cone verdict");
            }
        }
        for(j=0;j<4;++j) {
            struct structure_culling_pairing *pairing=&structure_culling.pairings[j];
            if(pairing->surface_index==0) { CHECK(pairing->material_index==0 && pairing->testable,"first owner wins overlap"); ++found; }
            if(pairing->surface_index==3) { CHECK(pairing->material_index==NONE && !pairing->testable,"unowned retained"); ++found; }
        }
        CHECK(found==2,"owner fixtures present");
        CHECK(structure_culling.clusters[0].bounds.x0==-10 && structure_culling.clusters[0].bounds.x1==10,"invalid pairing retains parent bounds");
        CHECK(strstr(build_log,"0 of 1 triangles disagree")!=NULL,"front normal agrees with vertex normals: %s",build_log);
        structure_culling_dispose(); return 0;
    }
    CASE("load-surface-lists")
    {
        fixture_load(FALSE);
        CHECK(structure_culling.allocation && structure_culling.pairing_count==3,"group vertex limit applied");
        CHECK(structure_culling.bucket_count==1 && !structure_culling.buckets[0].facing_eligible,"group exemption used");
        for(i=0;i<3;++i) CHECK(structure_culling.pairings[i].material_index==1,"group material cached");
        CHECK(structure_culling.buckets[0].cone.cutoff>.99f,"cone from group triangles");
        CHECK(structure_culling.clusters[0].bounds.x0==0 && structure_culling.clusters[0].bounds.x1==1,"bounds from vertices");
        structure_culling_dispose(); return 0;
    }
    CASE("load-lifetime")
    {
        fixture_load(TRUE);
        CHECK(structure_culling.structure==&fixture_bsp && structure_culling.surface_address==fixture_surfaces && structure_culling.surface_count==4,"bsp identity stored");
        fixture_load(FALSE); CHECK(log_count==2 && structure_culling.pairing_count==3,"reload replaces data");
        verify_ok=FALSE; structure_culling_structure_bsp_loaded();
        CHECK(!structure_culling.allocation && !structure_culling.structure,"verify failure frees previous data");
        structure_culling_dispose(); structure_culling_dispose(); return 0;
    }
    CASE("load-allocation-failure")
    {
        fixture_load(TRUE); allocation_fails=TRUE; structure_culling_structure_bsp_loaded();
        CHECK(!structure_culling.allocation && !structure_culling.structure,"allocation failure leaves no cache"); return 0;
    }
    CASE("split-partition")
    {
        struct structure_culling_split_surface surfaces[1000];
        long ends[1000], seen[1000] = {0}, count, j;
        for (i=0;i<1000;++i) {
            surfaces[i].surface_index=i;
            surfaces[i].centroid=(real_point3d){random_real(-20,20),random_real(-20,20),random_real(-20,20)};
            surfaces[i].normal=unit((real_vector3d){random_real(-1,1),random_real(-1,1),random_real(-1,1)});
            surfaces[i].facing_eligible=i%2;
        }
        count=structure_culling_split(surfaces,1000,ends,1000);
        CHECK(count>0 && count<=1000,"partition bucket count");
        CHECK(ends[count-1]==1000,"partition lost final surface");
        for(i=0;i<count;++i) CHECK(ends[i]>(i ? ends[i-1] : 0),"nonempty bucket");
        for(j=0;j<1000;++j) { CHECK(surfaces[j].surface_index>=0 && surfaces[j].surface_index<1000,"invented index"); ++seen[surfaces[j].surface_index]; }
        for(j=0;j<1000;++j) CHECK(seen[j]==1,"partition index %ld appears %ld times",j,seen[j]);
        return 0;
    }
    CASE("split-limits")
    {
        struct structure_culling_split_surface surfaces[130];
        long ends[130], count, j, first=0;
        for(i=0;i<130;++i) { surfaces[i].surface_index=i; surfaces[i].centroid=(real_point3d){0,0,0}; surfaces[i].normal=(real_vector3d){0,0,1}; surfaces[i].facing_eligible=i%2; }
        count=structure_culling_split(surfaces,130,ends,130);
        for(i=0;i<count;++i) { CHECK(ends[i]-first<=64,"bucket too large"); for(j=first;j<ends[i];++j) CHECK(surfaces[j].facing_eligible==surfaces[first].facing_eligible,"mixed eligibility"); first=ends[i]; }
        for(i=0;i<64;++i) surfaces[i].facing_eligible=TRUE;
        count=structure_culling_split(surfaces,64,ends,130);
        CHECK(count==1 && ends[0]==64,"tight homogeneous 64 stays whole");
        return 0;
    }
    CASE("split-room")
    {
        struct structure_culling_split_surface surfaces[12];
        real_vector3d directions[6]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        long ends[12], count, first=0, j;
        for(i=0;i<12;++i) { surfaces[i].surface_index=i; surfaces[i].centroid=(real_point3d){0,0,0}; surfaces[i].normal=directions[i/2]; surfaces[i].facing_eligible=TRUE; }
        count=structure_culling_split(surfaces,12,ends,12);
        CHECK(count==6,"room needs six facing buckets, got %ld",count);
        for(i=0;i<count;++i) {
            real_point3d triangles[36];
            for(j=first;j<ends[i];++j) {
                real_vector3d n=surfaces[j].normal;
                real_vector3d u=unit(cross(n,fabsf(n.k)<.9f ? (real_vector3d){0,0,1} : (real_vector3d){1,0,0}));
                real_vector3d v=cross(n,u);
                long t=3*(j-first);
                triangles[t]=(real_point3d){0,0,0}; triangles[t+1]=(real_point3d){u.i,u.j,u.k}; triangles[t+2]=(real_point3d){-v.i,-v.j,-v.k};
            }
            structure_culling_cone_build(&cone,ends[i]-first,triangles);
            CHECK(cone.cutoff>.5f,"room bucket cone too wide"); first=ends[i];
        }
        return 0;
    }
    CASE("split-loose")
    {
        struct structure_culling_split_surface surfaces[16];
        long ends[16], count, first=0, j;
        for(i=0;i<16;++i) { surfaces[i].surface_index=i; surfaces[i].centroid=(real_point3d){i<8 ? 0 : 100,0,0}; surfaces[i].normal=(real_vector3d){0,0,1}; surfaces[i].facing_eligible=TRUE; }
        count=structure_culling_split(surfaces,16,ends,16);
        CHECK(count>=2,"loose bucket not split");
        for(i=0;i<count;++i) { real low=surfaces[first].centroid.x,high=low; for(j=first;j<ends[i];++j) { low=MIN(low,surfaces[j].centroid.x); high=MAX(high,surfaces[j].centroid.x); } CHECK(high-low<8,"loose child"); first=ends[i]; }
        return 0;
    }
    CASE("out-of-range-surfaces")
    {
        long list[5]={-1,0,1,2,3}, counts[1]={3}, count;
        short owners[3]={0,0,0}; word vertices[9]={0,1,2,0,1,3,0,1,2};
        struct structure_culling_pairing pairings[5];
        count=structure_culling_gather_pairings(list,5,TRUE,0,3,owners,counts,vertices,pairings);
        CHECK(count==2 && pairings[0].surface_index==0 && pairings[1].surface_index==2,"surface-list drops invalid indices and vertices");
        count=structure_culling_gather_pairings(list,5,FALSE,0,3,owners,counts,vertices,pairings);
        CHECK(count==3 && pairings[1].surface_index==1 && !pairings[1].testable,"subcluster invalid vertex kept untestable");
        return 0;
    }
    CASE("group-material")
    {
        long list[1]={0}, counts[2]={3,2}, count;
        short owners[1]={0}; word vertices[3]={0,1,2}; struct structure_culling_pairing pairing;
        count=structure_culling_gather_pairings(list,1,TRUE,1,1,owners,counts,vertices,&pairing);
        CHECK(count==0,"group material, not owner, rejects vertex");
        counts[0]=2; counts[1]=3;
        count=structure_culling_gather_pairings(list,1,TRUE,1,1,owners,counts,vertices,&pairing);
        CHECK(count==1 && pairing.material_index==1 && pairing.testable,"group material, not owner, accepts vertex");
        return 0;
    }
    CASE("missing-owner")
    {
        long list[1]={0}, counts[1]={3}, count;
        short owners[1]={-1}; word vertices[3]={0,1,2}; struct structure_culling_pairing pairing;
        count=structure_culling_gather_pairings(list,1,FALSE,0,1,owners,counts,vertices,&pairing);
        CHECK(count==1 && pairing.material_index==-1 && !pairing.testable,"ownerless subcluster pairing kept untestable");
        return 0;
    }
    CASE("outside-near") { for (i=0; i<3; ++i) p[i].z=0; CHECK(structure_culling_triangle_outside(planes,p,p+1,p+2), "behind near"); return 0; }
    CASE("outside-far") { for (i=0; i<3; ++i) p[i].z=11; CHECK(structure_culling_triangle_outside(planes,p,p+1,p+2), "past far"); return 0; }
    CASE("outside-side") { for (i=0; i<3; ++i) p[i].x=-2; CHECK(structure_culling_triangle_outside(planes,p,p+1,p+2), "left"); return 0; }
    CASE("crossing-kept")
    {
        p[1].x=-2; p[2].x=2;
        CHECK(!structure_culling_triangle_outside(planes,p,p+1,p+2), "one inside");
        p[0].z=0;
        CHECK(!structure_culling_triangle_outside(planes,p,p+1,p+2), "different planes and near crossing");
        p[1].x=p[2].x=0;
        CHECK(!structure_culling_triangle_outside(planes,p,p+1,p+2), "near crossing");
        p[0].z=p[1].z=p[2].z=1;
        CHECK(!structure_culling_triangle_outside(planes,p,p+1,p+2), "on plane is kept");
        return 0;
    }
    CASE("facing-away")
    {
        CHECK(structure_culling_triangle_faces_away(&camera,p,p+1,p+2), "back camera");
        CHECK(!structure_culling_triangle_faces_away(&camera,p,p+2,p+1), "swap flips back"); return 0;
    }
    CASE("facing-toward")
    {
        camera.z=2+STRUCTURE_CULLING_FRONT_SIGN;
        CHECK(!structure_culling_triangle_faces_away(&camera,p,p+1,p+2), "front camera");
        CHECK(structure_culling_triangle_faces_away(&camera,p,p+2,p+1), "swap flips front"); return 0;
    }
    CASE("edge-on-kept") { camera.z=2; CHECK(!structure_culling_triangle_faces_away(&camera,p,p+1,p+2), "coplanar kept"); return 0; }
    CASE("degenerate-kept")
    {
        p[2]=p[1]; CHECK(!structure_culling_triangle_faces_away(&camera,p,p+1,p+2), "duplicate kept");
        p[2].x=1; CHECK(!structure_culling_triangle_faces_away(&camera,p,p+1,p+2), "collinear kept");
        p[2].y=NAN; CHECK(!structure_culling_triangle_faces_away(&camera,p,p+1,p+2), "nonfinite kept");
        p[2].y=INFINITY; CHECK(!structure_culling_triangle_faces_away(&camera,p,p+1,p+2), "infinite kept"); return 0;
    }
    CASE("cone-never-more-aggressive") { random_buckets(); return 0; }
    CASE("cone-culls-something")
    {
        real_point3d grid[8*8*3];
        for (i=0; i<64; ++i)
        {
            grid[3*i]=(real_point3d){i%8, i/8, 0};
            grid[3*i+1]=(real_point3d){i%8+1, i/8, 0};
            grid[3*i+2]=(real_point3d){i%8, i/8+STRUCTURE_CULLING_FRONT_SIGN, 0};
        }
        structure_culling_cone_build(&cone,64,grid);
        camera=(real_point3d){3.5f, 3.5f, -10};
        CHECK(structure_culling_cone_facing(&cone,&camera)==_structure_culling_facing_away, "floor from below");
        camera.z=10;
        CHECK(structure_culling_cone_facing(&cone,&camera)==_structure_culling_facing_toward, "floor from above"); return 0;
    }
    CASE("cone-camera-inside")
    {
        structure_culling_cone_build(&cone,1,p);
        camera=cone.center;
        CHECK(structure_culling_cone_facing(&cone,&camera)==_structure_culling_facing_mixed, "sphere center");
        camera.z+=cone.radius;
        CHECK(structure_culling_cone_facing(&cone,&camera)==_structure_culling_facing_mixed, "sphere surface"); return 0;
    }
    CASE("cone-wide-no-verdict")
    {
        real_point3d cube[18] = {{1,0,0},{1,1,0},{1,0,1}, {-1,0,0},{-1,0,1},{-1,1,0},
            {0,1,0},{0,1,1},{1,1,0}, {0,-1,0},{1,-1,0},{0,-1,1},
            {0,0,1},{1,0,1},{0,1,1}, {0,0,-1},{0,1,-1},{1,0,-1}};
        structure_culling_cone_build(&cone,6,cube);
        CHECK(cone.cutoff<=0, "cube has no cone");
        for(i=0;i<100;++i) { camera=(real_point3d){random_real(-10,10),random_real(-10,10),random_real(-10,10)}; CHECK(structure_culling_cone_facing(&cone,&camera)==_structure_culling_facing_mixed,"wide mixed"); }
        return 0;
    }
    CASE("box-outside")
    {
        box.z0=11; box.z1=12; CHECK(structure_culling_box_test(planes,&box)==0,"past GPU far");
        box.z0=-2; box.z1=0; CHECK(structure_culling_box_test(planes,&box)==0,"behind near");
        box.z0=2; box.z1=3; box.x0=-3; box.x1=-2; CHECK(structure_culling_box_test(planes,&box)==0,"left box"); return 0;
    }
    CASE("box-partly") { box.z0=0; CHECK(structure_culling_box_test(planes,&box)==1,"near straddling box"); return 0; }
    CASE("box-inside")
    {
        CHECK(structure_culling_box_test(planes,&box)==2,"inside six planes");
        box.z0=5; box.z1=6; CHECK(structure_culling_box_test(planes,&box)==2,"fog is not GPU far"); return 0;
    }
    CHECK(FALSE,"unknown case %s",case_name);
    return 1;
}
