#include "cseries.h"
#include "physics/bsp3d.h"
#include "engine_collision.h"
#include <stdlib.h>
#include <string.h>

struct mac_engine_collision_state {
    struct bsp3d bsp;
    struct bsp3d_node *nodes;
    real_plane3d *planes;
};
typedef char mac_engine_long_is_32[sizeof(long)==4 ? 1 : -1];
typedef char mac_engine_float_is_32[sizeof(real)==4 ? 1 : -1];

static int finite_float(float f)
{
    uint32_t bits;
    memcpy(&bits,&f,4);
    return (bits & UINT32_C(0x7f800000)) != UINT32_C(0x7f800000);
}
void mac_engine_collision_free(struct mac_engine_collision *bridge)
{
    struct mac_engine_collision_state *s;
    if (!bridge) return;
    s=(struct mac_engine_collision_state *)bridge->state;
    if (s) { free(s->nodes); free(s->planes); free(s); }
    bridge->state=NULL;
}
int mac_engine_collision_create(const struct mac_retail_collision *input,
    struct mac_engine_collision *out, const char **error)
{
    struct mac_engine_collision_state *s;
    uint32_t i,j,child;
    if (error) *error="invalid engine collision input";
    if (!out) return 0;
    out->state=NULL;
    if (!input || !input->nodes || !input->planes || !input->count[0] ||
        !input->count[1] || input->count[0]>UINT32_C(0x7fffffff)/sizeof(struct bsp3d_node) ||
        input->count[1]>UINT32_C(0x7fffffff)/sizeof(real_plane3d)) return 0;
    for (i=0;i<input->count[0];++i) {
        if (input->nodes[i].plane>=input->count[1]) return 0;
        for (j=0;j<2;++j) {
            child=input->nodes[i].child[j];
            if (child==UINT32_MAX) continue;
            if (child & UINT32_C(0x80000000)) {
                if ((child & UINT32_C(0x7fffffff))>=input->count[2]) return 0;
            } else if (child>=input->count[0]) return 0;
        }
    }
    for (i=0;i<input->count[1];++i) {
        for (j=0;j<3;++j) if (!finite_float(input->planes[i].n[j])) return 0;
        if (!finite_float(input->planes[i].d)) return 0;
    }
    s=(struct mac_engine_collision_state *)calloc(1,sizeof(*s));
    if (!s) { if (error) *error="engine collision allocation failed"; return 0; }
    out->state=s;
    s->nodes=(struct bsp3d_node *)malloc(input->count[0]*sizeof(*s->nodes));
    s->planes=(real_plane3d *)malloc(input->count[1]*sizeof(*s->planes));
    if (!s->nodes || !s->planes) {
        mac_engine_collision_free(out);
        if (error) *error="engine collision allocation failed";
        return 0;
    }
    for (i=0;i<input->count[0];++i) {
        s->nodes[i].plane_designator=(long)input->nodes[i].plane;
        /* Preserve signed designator bits without an out-of-range cast. */
        memcpy(&s->nodes[i].children[0],&input->nodes[i].child[0],4);
        memcpy(&s->nodes[i].children[1],&input->nodes[i].child[1],4);
    }
    for (i=0;i<input->count[1];++i) {
        s->planes[i].n.i=input->planes[i].n[0];
        s->planes[i].n.j=input->planes[i].n[1];
        s->planes[i].n.k=input->planes[i].n[2];
        s->planes[i].d=input->planes[i].d;
    }
    s->bsp.nodes.count=(long)input->count[0];s->bsp.nodes.address=s->nodes;
    s->bsp.planes.count=(long)input->count[1];s->bsp.planes.address=s->planes;
    if (error) *error=NULL;
    return 1;
}
int32_t mac_engine_collision_leaf(const struct mac_engine_collision *bridge,const float p[3])
{
    struct mac_engine_collision_state *s;
    real_point3d point;
    if (!bridge || !bridge->state || !p || !finite_float(p[0]) ||
        !finite_float(p[1]) || !finite_float(p[2])) return -2;
    s=(struct mac_engine_collision_state *)bridge->state;
    point.x=p[0];point.y=p[1];point.z=p[2];
    return (int32_t)bsp3d_test_point(&s->bsp,0,&point);
}
