#include "harness.h"
#include <math.h>
#define __stdcall
#define TICKS_PER_SECOND 30
#define BOTS_SEARCHES_A_TICK 3
#define BOTS_PATHFINDING_WALKABLE_BIT 0
#define MAXIMUM_PATH_EDGES_PER_COLLISION_SURFACE 64
#define INFINITE 0xffffffffUL
#define WAIT_OBJECT_0 0
#define csmemset memset
#define csmemcpy memcpy
#define SET_FLAG(f,b,v) ((v)?((f)|=FLAG(b)):((f)&=~FLAG(b)))
#define BIT_VECTOR_TEST_FLAG(p,b) TEST_FLAG((p)[(b)/32],(b)%32)
#define platform_log(...) ((void)0)
struct collision_surface { byte flags, breakable_surface_index; long first_edge_index; };
struct collision_edge { long vertex_indices[2], surface_indices[2], edge_indices[2]; };
struct collision_bsp { struct { long count; struct collision_surface *address; } surfaces;
    struct { long count; struct collision_edge *address; } edges; struct { long count; } vertices; };
struct structure_bsp {
    struct { long count; byte *address; } pathfinding_surfaces;
    struct { long count; struct collision_bsp *address; } collision_bsp;
    struct { long count; } breakable_surfaces;
};
#define TAG_BLOCK_GET_ELEMENT(block,i,type) (&(block)->address[i])
struct path_state { struct structure_bsp *structure; struct { boolean ignore_broken_surfaces; } input; };
struct bots_bot { long unit_index; short goal_kind; real_point3d goal_point, last_position; };
struct bots_nav_route { short phase, link; };
static struct { struct bots_bot bots[63]; } bots_globals;
static struct { struct bots_nav_route routes[63]; } bots_navigation;
static int bots_worker_enabled = 1, handles;
static long now = 100, broken;
static long game_time_get(void) { return now; }
static void *breakable_surface_flags_get(void) { return &broken; }
static real bots_distance3d(real_point3d const *a, real_point3d const *b)
{ real x=a->x-b->x,y=a->y-b->y,z=a->z-b->z; return (real)sqrt(x*x+y*y+z*z); }
struct event { int signaled, manual; };
static void *CreateEventA(void *a, int manual, int signaled, void *b)
{ struct event *e=calloc(1,sizeof(*e)); (void)a;(void)b; e->manual=manual;e->signaled=signaled;handles++;return e; }
static int SetEvent(void *p) { ((struct event*)p)->signaled=1;return 1; }
static int ResetEvent(void *p) { ((struct event*)p)->signaled=0;return 1; }
static unsigned long WaitForSingleObject(void *p, unsigned long delay)
{ struct event *e=p;(void)delay;if (!e->signaled)return 258;if(!e->manual)e->signaled=0;return 0; }
static int CloseHandle(void *p) { CHECK(p!=NULL,"null handle");free(p);handles--;return 1; }
static void *CreateThread(void *a,long stack,unsigned long (*entry)(void*),void *arg,long flags,void *id)
{ (void)a;(void)stack;(void)entry;(void)arg;(void)flags;(void)id;return CreateEventA(NULL,1,1,NULL); }
static void bots_path_find_snapshot(struct path_state *state) { (void)state; }
static void *allocations[16];
static int fail_after = -1;
static void *checked_malloc(size_t size)
{
    int index;
    if (fail_after == 0) return NULL;
    if (fail_after > 0) fail_after--;
    for (index=0; index<16; index++) if (!allocations[index])
        return allocations[index]=malloc(size);
    CHECK(0,"fixture allocation limit"); return NULL;
}
static void checked_free(void *pointer)
{
    int index;
    CHECK(pointer!=NULL,"engine free received NULL");
    for (index=0; index<16; index++) if (allocations[index]==pointer)
    { free(pointer); allocations[index]=NULL; return; }
    CHECK(0,"worker allocation does not belong to engine allocator");
}
#define malloc checked_malloc
#define free checked_free
#include "under_test.inc"
int main(int argc, char **argv)
{
    struct bots_bot *bot=&bots_globals.bots[0];
    struct structure_bsp structure;
    struct collision_bsp bsp;
    struct collision_surface surfaces[2]={{0,0,0},{8,0,0}};
    struct collision_edge edges[1]={{{0,1},{0,1},{0,0}}};
    byte walkable[2]={1,129};
    struct path_state state;
    real_point3d start={0,0,0},goal={20,0,0};
    struct bots_path_job *job;
    CHECK(argc==2,"case");
    memset(&structure,0,sizeof(structure));memset(&state,0,sizeof(state));
    structure.pathfinding_surfaces.address=walkable;structure.pathfinding_surfaces.count=2;
    structure.collision_bsp.address=&bsp;structure.collision_bsp.count=1;
    structure.breakable_surfaces.count=1;bsp.surfaces.address=surfaces;bsp.surfaces.count=2;
    bsp.edges.address=edges;bsp.edges.count=1;bsp.vertices.count=2;
    state.structure=&structure;bot->unit_index=10;bot->goal_point=goal;
    if (!strcmp(argv[1],"allocation") || !strcmp(argv[1],"graph_allocation"))
    {
        fail_after = !strcmp(argv[1],"allocation") ? 0 : 1;
        CHECK(!bots_worker_submit(bot,&state,&start,&goal,1,FALSE,FALSE,.25f),"allocation failure submitted");
        bots_worker_stop();bots_worker_stop();
        CHECK(!handles && !allocations[0],"failed allocation retained ownership");
        return 0;
    }
    if (!strcmp(argv[1],"disabled") || !strcmp(argv[1],"malformed"))
    {
        if (!strcmp(argv[1],"disabled")) bots_worker_enabled=0;
        else edges[0].edge_indices[0]=9;
        CHECK(!bots_worker_submit(bot,&state,&start,&goal,1,FALSE,FALSE,.25f),"worker changed default");
        CHECK(!handles && !bots_worker.jobs,"disabled worker allocated");
        return 0;
    }
    CHECK(bots_worker_submit(bot,&state,&start,&goal,1,FALSE,FALSE,.25f),"submission failed");
    job=bots_worker_job(bot);CHECK(job && job->pending,"pending ownership lost");
    if (!strcmp(argv[1],"snapshot"))
    {
        CHECK(job->state.input.ignore_broken_surfaces,"worker still reads live breakage");
        CHECK(job->walkable[-1]==0,"open-edge sentinel missing");
        CHECK(!(job->walkable[1]&1) && walkable[1]==129,"breakage snapshot changed live graph");
        walkable[0]=0;broken=1;
        CHECK(job->walkable[0]==1 && !(job->walkable[1]&1),"snapshot reads mutable state");
    }
    else if (!strcmp(argv[1],"stale"))
    {
        bot->unit_index=11;
        CHECK(bots_worker_job(bot)==job && job->pending,"in-flight answer overwritten on respawn");
        SetEvent(job->done);
        CHECK(!bots_worker_job(bot) && !job->pending,"respawn consumed stale answer");
    }
    else CHECK(!strcmp(argv[1],"cleanup"),"case");
    bots_worker_stop();bots_worker_stop();
    CHECK(!handles && !bots_worker.jobs && !bots_worker.thread,"worker cleanup leaked ownership");
    CHECK(!allocations[0] && !allocations[1],"worker allocations leaked");
    return 0;
}
