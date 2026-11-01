#include "harness.h"
#include <math.h>
#include <float.h>

#define MAXIMUM_LOCAL_PLAYERS 4
#define MAXIMUM_INTERPOLATED_NODES 64
#define MAXIMUM_INTERPOLATED_OBJECTS 1
#define REAL_MAX FLT_MAX

typedef struct
{
    real scale;
    union
    {
        real n[4][3];
        struct { real_vector3d forward, left, up; real_point3d position; };
    };
} real_matrix4x3;
struct render_camera { real_point3d position; real_vector3d forward, up; };
static struct { long object_index; } *interpolated_objects;
static int interpolated_cameras[MAXIMUM_LOCAL_PLAYERS];
static long interpolation_tick;
static boolean interpolation_rendering = TRUE;
static real interpolation_fraction = 0.5f;
static void cross_product3d(real_vector3d const *a, real_vector3d const *b, real_vector3d *r)
{
    r->i = a->j*b->k-a->k*b->j;
    r->j = a->k*b->i-a->i*b->k;
    r->k = a->i*b->j-a->j*b->i;
}
static void set_real_point3d(real_point3d *p, real x, real y, real z) { p->x=x; p->y=y; p->z=z; }
#include "under_test.inc"

static struct render_camera camera = {{0,0,0},{1,0,0},{0,0,1}};
static real_matrix4x3 identity = {1, {{{1,0,0},{0,1,0},{0,0,1},{0,0,0}}}};
static real_matrix4x3 nodes[64];
static long weapon = 0x10001, graph = 0x20002;
static short local, count = 2;

static void pose(real position, real angle)
{
    short i;
    real_matrix4x3 camera_matrix, node = identity;
    matrix4x3_from_point_and_vectors(&camera_matrix, &camera.position, &camera.forward, &camera.up);
    node.position.x = position;
    node.forward.i = node.left.j = (real)cos(angle);
    node.forward.j = (real)sin(angle);
    node.left.i = -node.forward.j;
    for (i=0; i<64; i++) matrix4x3_multiply(&camera_matrix, &node, &nodes[i]);
}
static void draw(void)
{
    render_interpolation_first_person(local, weapon, graph, nodes, count, &camera);
}
static void tick(long tick_index, real position)
{
    interpolation_tick = tick_index;
    pose(position, 0);
    draw();
}
static void expect(real value)
{
    CHECK(fabs(nodes[0].position.x-value)<1e-5f, "position %f expected %f", nodes[0].position.x, value);
}

int main(int argc, char **argv)
{
    const char *case_name = argc>1 ? argv[1] : "";
    tick(10, 0);
    CASE("fast-motion")
    {
        int frame;
        /* A valid continuous pose can move beyond the former 0.25 limit.
           Sample the same tick at 120 Hz, including a rotating chamber. */
        for (frame=0; frame<4; frame++)
        {
            real qz, qw, expected_rotation;
            interpolation_tick = 11;
            interpolation_fraction = frame*0.25f;
            pose(0.6f, 1.5707963268f);
            draw();
            expect(0.6f*interpolation_fraction);
            qz=interpolation_fraction*(real)sqrt(.5);
            qw=1-interpolation_fraction+qz;
            expected_rotation=2*qw*qz/(qw*qw+qz*qz);
            CHECK(fabs(nodes[0].forward.j-expected_rotation)<1e-5f, "rotation not interpolated");
        }
        tick(12, 0.8f);
        expect(0.75f); /* Previous is the raw 0.6 pose, never the last blend. */
    }
    CASE("weapon-change") { weapon += 0x10000; tick(11, .1f); expect(.1f); tick(12,.2f); expect(.15f); }
    CASE("graph-change") { graph++; tick(11,.1f); expect(.1f); }
    CASE("same-tick-change")
    {
        tick(11,.1f); expect(.05f);
        weapon++; tick(11,.2f); expect(.2f);
        tick(12,.3f); expect(.25f);
    }
    CASE("node-count") { tick(11,.1f); count=3; tick(11,.2f); expect(.2f); tick(12,.3f); expect(.25f); }
    CASE("gap") { tick(13,.1f); expect(.1f); tick(14,.2f); expect(.15f); }
    CASE("disabled")
    {
        tick(11,.1f); interpolation_rendering=FALSE; tick(11,.12f); expect(.12f);
        interpolation_rendering=TRUE; tick(11,.14f); expect(.14f);
        tick(12,.16f); expect(.15f);
    }
    CASE("reset") { tick(11,.1f); render_interpolation_reset(); tick(11,.2f); expect(.2f); tick(12,.3f); expect(.25f); }
    CASE("players") { local=1; tick(10,.8f); tick(11,.9f); expect(.85f); local=0; tick(11,.1f); expect(.05f); }
    CASE("camera")
    {
        camera.position.x=40;
        camera.forward.i=0; camera.forward.j=1;
        tick(11,.2f);
        expect(40);
        CHECK(fabs(nodes[0].position.y-.1f)<1e-5f, "pose not blended in camera space");
    }
    CASE("invalid")
    {
        tick(11,.1f);
        local=-1; tick(11,.2f); expect(.2f);
        local=4; tick(11,.2f); expect(.2f);
        local=0; count=65; tick(11,.2f); expect(.2f);
        count=2; tick(12,.3f); expect(.3f);
        interpolation_tick=13; pose(.4f,0); nodes[1].position.x=NAN; draw(); expect(.4f);
        CHECK(isnan(nodes[1].position.x), "invalid node unexpectedly blended");
        tick(14,.5f); expect(.5f);
    }
    CASE("capacity")
    {
        count=64; tick(11,.1f); expect(.1f); tick(12,.2f); expect(.15f);
        CHECK(fabs(nodes[63].position.x-.15f)<1e-5f, "last valid node not blended");
        count=0; tick(12,.22f); expect(.22f); count=64; tick(13,.24f); expect(.24f);
    }
    CASE("missing")
    {
        weapon=NONE; tick(11,.1f); expect(.1f);
        weapon=0x10001; tick(12,.12f); expect(.12f);
        graph=NONE; tick(13,.14f); expect(.14f);
        graph=0x20002; tick(14,.16f); expect(.16f);
    }
    CASE("chamber-reset")
    {
        int frame;
        for (frame=0; frame<4; frame++)
        {
            interpolation_tick=11; interpolation_fraction=frame*.25f;
            pose(.1f,0);
            /* Only the tube node resets. The other node keeps moving. */
            nodes[1].forward.i=nodes[1].left.j=-1;
            draw(); expect(.1f*interpolation_fraction);
            CHECK(nodes[1].forward.i == -1 && nodes[1].forward.j == 0,
                  "authored half-turn reset generated an extra spin");
        }
        interpolation_tick=12; interpolation_fraction=.5f;
        pose(.2f,0); nodes[1].forward.i=nodes[1].left.j=-1;
        draw(); expect(.15f);
        CHECK(nodes[1].forward.i == -1, "half-turn reset poisoned next tick");
    }
    CASE("large-rotation")
    {
        interpolation_tick=11; pose(.1f,2.9670597284f); draw(); expect(.05f);
        CHECK(fabs(nodes[0].forward.j-.9961947f)<1e-5f, "ordinary 170-degree rotation snapped");
    }
    return 0;
}
