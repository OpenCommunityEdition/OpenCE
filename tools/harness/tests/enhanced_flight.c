#define real_point3d harness_point3d
#include "harness.h"
#undef real_point3d
#include <math.h>
#include <float.h>
typedef union { struct { real x,y,z; }; real n[3]; } real_point3d;
#define BOTS_MAXIMUM 63
#define TICKS_PER_SECOND 30
#define REAL_MAX FLT_MAX
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(a,l,h) MIN(MAX(a,l),h)
#define csmemset memset
enum { _collision_test_structure_bit, _collision_test_front_facing_surfaces_bit,
_collision_test_objects_bit, _collision_test_objects_scenery_bit,
_collision_test_objects_machines_bit, _collision_test_objects_vehicles_bit };
enum { _unit_control_weapon_primary_trigger_bit, _unit_control_jump_bit, _unit_control_crouch_modifier_bit };
struct collision_result { real t; real_point3d point; struct { real_vector3d n; } plane; };
struct object_datum { struct { real_point3d position, bounding_sphere_center; real bounding_sphere_radius, scale, body_vitality;
real_vector3d up, forward, translational_velocity; } object; long definition_index; struct { byte on_ground_ticks; } vehicle; } craft;
#define vehicle_datum object_datum
#define vehicle_datum_get object_get
struct player_action { struct { real i,j; } throttle; struct { real yaw,pitch; } desired_facing;
real primary_trigger; unsigned long control_flags; };
static long now, calls;
static int wall, sealed, ceiling, distant, object_wall, ridge, roof;
static long game_time_get(void) { return now; }
static struct object_datum *object_get(long index) { CHECK(index==1,"craft id"); return &craft; }
static void vector_from_points3d(real_point3d const *a, real_point3d const *b, real_vector3d *v)
{ v->i=b->x-a->x; v->j=b->y-a->y; v->k=b->z-a->z; }
static real magnitude3d(real_vector3d const *v) { return (real)sqrt(v->i*v->i+v->j*v->j+v->k*v->k); }
static boolean collision_test_vector(unsigned long flags, real_point3d const *a, real_vector3d const *d,
long ignore, struct collision_result *hit)
{
    real t, y, z;
    calls++;
    memset(hit,0,sizeof(*hit));
    CHECK(ignore==1,"did not ignore own hull");
    if (object_wall && TEST_FLAG(flags,_collision_test_objects_bit)) return TRUE;
    hit->t=0.5f;
    if (roof && a->z>10 && a->z+d->k<10 && fabs(a->x)<10 && fabs(a->y)<10) return TRUE;
    if (sealed || (ceiling && (a->z > 12 || a->z+d->k > 12))) return TRUE;
    if (ridge && d->k < -1000 && a->x >= 20 && a->x <= 40) { hit->point.z=40;hit->plane.n.k=1;return TRUE; }
    if (a->z >= 0 && a->z+d->k < 0) { hit->point.z=0; hit->plane.n.k=1; return TRUE; }
    if (wall && fabs(d->i)>0.001f) {
        t=(20-a->x)/d->i; y=a->y+t*d->j; z=a->z+t*d->k;
        if (t>0 && t<1 && fabs(y)<(distant?1000:10) && z<(distant?1000:35)) return TRUE;
    }
    return FALSE;
}
#define MAXIMUM_MASS_POINTS_PER_PHYSICS 32
struct unit_definition { struct { struct { long index; } physics; } object; } unit_tag;
struct mass_point_definition { real_point3d position; real radius; } mass[32];
struct physics_definition { struct { long count; } mass_points; real_point3d center_of_mass; real radius; } physics_tag;
#define TAG_BLOCK_GET_ELEMENT(block,index,type) (&mass[index])
static struct unit_definition *unit_definition_get(long index) { return &unit_tag; }
static struct physics_definition *physics_definition_get(long index) { return &physics_tag; }
static long config_integer(char const *name) { return 0; }
static void platform_log(char const *format, ...) { }
#include "under_test.inc"
int main(int argc, char **argv)
{
    struct player_action action;
    real_point3d start={{0,0,20}}, goal={{60,0,20}};
    struct flight_state *state=&flights[0];
    int i;
    CHECK(argc==2,"case");
    memset(&action,0,sizeof(action));
    craft.object.position=craft.object.bounding_sphere_center=start;
    craft.object.bounding_sphere_radius=2;craft.object.scale=1;unit_tag.object.physics.index=NONE;
    craft.object.up.k=craft.object.forward.i=1;
    enhanced_flight_reset(0); state->vehicle=1; state->radius=state->height=2; state->spacing=8;state->cruise_height=28;
    if (!strcmp(argv[1],"clear") || !strcmp(argv[1],"wall") || !strcmp(argv[1],"sealed")) {
        wall=!strcmp(argv[1],"wall"); sealed=!strcmp(argv[1],"sealed");
        flight_begin(state,&start,&goal);
        for(i=0;i<1000 && state->searching;i++) {
            now=i; calls=0; flight_search(0,state);
            CHECK(calls<=784,"unbounded per-tick collision work %ld",calls);
            CHECK(state->count<=FLIGHT_NODES && state->expanded<=256,"node or expansion overflow");
        }
        CHECK(!state->searching,"search never completed");
        if(sealed) CHECK(!state->step_count && state->retry>now,"unreachable craft never backs off");
        else {
            real_point3d previous=start;
            CHECK(state->step_count>0,"no route");
            if(wall) CHECK(state->step_count>1,"wall ignored");
            for(i=0;i<state->step_count;i++) {
                CHECK(flight_clear(1,&previous,&state->steps[i],2,2),"route clips wall");
                previous=state->steps[i];
            }
            CHECK(flight_distance(&previous,&goal)<0.01f,"wrong destination");
        }
    } else if (!strcmp(argv[1],"vertical")) {
        goal=start;goal.z+=30;
        enhanced_flight_reset(0);
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(!action.throttle.i && TEST_FLAG(action.control_flags,_unit_control_jump_bit),"hovering aircraft missed vertical climb");
        goal.z=start.z-15;now=8;
        enhanced_flight_action(0,1,TRUE,&goal,TRUE,FALSE,&action);
        CHECK(!action.throttle.i && TEST_FLAG(action.control_flags,_unit_control_crouch_modifier_bit),"hovering aircraft missed vertical descent");
    } else if (!strcmp(argv[1],"forward_flight")) {
        real_point3d previous=start;
        goal=start;goal.z+=16;
        state->hovering=FALSE;
        flight_begin(state,&start,&goal);
        for(i=0;i<2000 && state->searching;i++) { now=i;calls=0;flight_search(0,state);CHECK(calls<=784,"work budget"); }
        CHECK(!state->searching && state->step_count>1,"forward-flight aircraft took vertical shortcut");
        for(i=0;i<state->step_count;i++) {
            real dx=state->steps[i].x-previous.x,dy=state->steps[i].y-previous.y;
            CHECK(fabs(state->steps[i].z-previous.z)<=0.5f*sqrt(dx*dx+dy*dy)+.001f,"unflyable vertical or steep route");
            previous=state->steps[i];
        }
        CHECK(flight_distance(&previous,&goal)<.01f,"sloped route missed destination");
    } else if (!strcmp(argv[1],"fighter_landing")) {
        goal=start;goal.z=0;
        enhanced_flight_action(0,1,FALSE,&goal,TRUE,FALSE,&action);
        CHECK(action.throttle.i>0 && action.desired_facing.pitch<0,"fighter tried unsupported vertical descent");
        CHECK(!TEST_FLAG(action.control_flags,_unit_control_crouch_modifier_bit),"fighter relied on crouch lift");
        sealed=1;now++;
        enhanced_flight_action(0,1,FALSE,&goal,TRUE,FALSE,&action);
        CHECK(!action.throttle.i,"landing drove into wall");
        sealed=0;craft.object.position.z=craft.object.bounding_sphere_center.z=1;now++;
        enhanced_flight_action(0,1,FALSE,&goal,TRUE,FALSE,&action);
        CHECK(enhanced_flight_landing(0) && enhanced_flight_can_exit(1),"low fighter cannot finish landing");
        craft.object.translational_velocity.i=.2f;craft.object.translational_velocity.k=-.15f;now++;
        enhanced_flight_action(0,1,FALSE,&goal,TRUE,FALSE,&action);
        CHECK(action.throttle.i>0 && action.throttle.i<.6f && action.desired_facing.pitch>0,"low fighter lost velocity correction while braking");
        craft.object.position.z=craft.object.bounding_sphere_center.z=4;now++;
        enhanced_flight_action(0,1,FALSE,&goal,TRUE,FALSE,&action);
        CHECK(action.throttle.i>0 && action.throttle.i<.6f && action.desired_facing.pitch>0,"low fighter failed to slow before touchdown");
        start.z=2;goal=start;goal.x+=10;goal.z=-1;
        CHECK(!flight_clear(1,&start,&goal,2,2),"ordinary route crossed ground");
        CHECK(flight_sweep(1,&start,&goal,2,2,TRUE),"touchdown rejected upward ground");
    } else if (!strcmp(argv[1],"low_approach")) {
        goal.x=45;goal.z=0;craft.object.translational_velocity.i=.6f;craft.object.translational_velocity.k=-.15f;
        enhanced_flight_action(0,1,FALSE,&goal,TRUE,FALSE,&action);
        CHECK(enhanced_flight_landing(0) && action.throttle.i<.6f && action.desired_facing.pitch>0,
            "low fighter waited until directly over goal before arresting descent");
    } else if (!strcmp(argv[1],"dynamic_hull")) {
        unit_tag.object.physics.index=0;physics_tag.mass_points.count=1;mass[0].radius=1;
        CHECK(flight_clear(1,&start,&goal,2,2),"authored mass point route was blocked");
        object_wall=1;
        CHECK(!flight_clear(1,&start,&goal,2,2),"authored hull ignored dynamic obstruction");
    } else if (!strcmp(argv[1],"terrain")) {
        ridge=1;goal.z=0;
        CHECK(flight_cruise_height(1,&start,&goal,2)==48,"cruise route ignored intervening ridge");
        start.z=80;
        CHECK(flight_cruise_height(1,&start,&goal,2)==80,"cruise descended early toward floor");
    } else if (!strcmp(argv[1],"landing_site")) {
        roof=1;goal=start;goal.z=0;
        enhanced_flight_action(0,1,TRUE,&goal,TRUE,FALSE,&action);
        CHECK(state->landing_checked && state->landing_ready,"roofed objective had no nearby landing site");
        CHECK(fabs(state->landing.x)>=10 || fabs(state->landing.y)>=10,"aircraft tried to descend through base roof");
        CHECK(flight_distance(&state->landing,&goal)<30,"landing site too far from objective");
        CHECK(!TEST_FLAG(action.control_flags,_unit_control_crouch_modifier_bit),"hover approach descended before aligning with open column");
        enhanced_flight_reset(0);sealed=1;now++;
        enhanced_flight_action(0,1,TRUE,&goal,TRUE,FALSE,&action);
        CHECK(state->failed && state->landing_checked,"unreachable landing site did not enter recovery");
    } else if (!strcmp(argv[1],"winged")) {
        unit_tag.object.physics.index=0;physics_tag.mass_points.count=1;physics_tag.radius=2;
        craft.object.translational_velocity.i=.5f;craft.object.translational_velocity.k=-.4f;
        goal.x=100;goal.z=0;
        enhanced_flight_action(0,1,FALSE,&goal,TRUE,FALSE,&action);
        CHECK(!state->hovering && action.throttle.i>=.6f,"speed-dependent lift lost during approach");
        CHECK(action.desired_facing.pitch>0,"falling fighter did not arrest descent");
        state->phase=_flight_land;now++;
        enhanced_flight_action(0,1,FALSE,&goal,TRUE,FALSE,&action);
        CHECK(action.throttle.i>=.6f && action.desired_facing.pitch>0,"landing cut lift before touchdown");
        craft.object.position.x=craft.object.bounding_sphere_center.x=goal.x-12;now++;
        enhanced_flight_action(0,1,FALSE,&goal,TRUE,FALSE,&action);
        CHECK(fabs(action.desired_facing.yaw)>1,"landing left the objective on an unbounded straight glide");
        craft.vehicle.on_ground_ticks=5;now++;
        enhanced_flight_action(0,1,FALSE,&goal,TRUE,FALSE,&action);
        CHECK(action.throttle.i<0,"landed fighter did not brake");
    } else if (!strcmp(argv[1],"climb_progress")) {
        goal=start;goal.x=500;goal.z=0;state->destination=goal;
        state->step_count=1;state->steps[0]=start;state->steps[0].z=200;
        state->remaining=1;state->progress_tick=0;
        for(i=0;i<60;i++) {
            now=i*TICKS_PER_SECOND;craft.object.position.z=craft.object.bounding_sphere_center.z=20+i;
            enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
            CHECK(!state->failed,"active climb mistaken for a stuck objective");
        }
    } else if (!strcmp(argv[1],"lookahead")) {
        state->destination=goal;state->hovering=TRUE;state->step_count=4;
        for(i=0;i<4;i++) { state->steps[i]=start;state->steps[i].x=8*(i+1); }
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(state->step==3 && action.throttle.i>0,"transport chased every small grid cell");
        wall=1;state->step=0;now=8;
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(state->step==1,"lookahead shortcut crossed a wall");
    } else if (!strcmp(argv[1],"hold")) {
        wall=1;craft.object.translational_velocity.k=-.02f;
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(!action.throttle.i && TEST_FLAG(action.control_flags,_unit_control_jump_bit),"pending route let hover craft fall out of flight");
        craft.object.translational_velocity.k=.03f;now=1;
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(!action.throttle.i && TEST_FLAG(action.control_flags,_unit_control_crouch_modifier_bit),"waiting craft did not damp upward drift");
    } else if (!strcmp(argv[1],"mass_budget")) {
        unit_tag.object.physics.index=0;physics_tag.mass_points.count=32;wall=1;
        flight_begin(state,&start,&goal);
        for(i=0;i<1000 && state->searching;i++) {
            now=i;calls=0;flight_search(0,state);
            CHECK(calls<=3520,"unbounded authored-hull collision work %ld",calls);
        }
        CHECK(!state->searching && state->step_count>0,"authored-hull planner did not complete");
    } else if (!strcmp(argv[1],"partial")) {
        wall=distant=1;goal.x=6000;
        flight_begin(state,&start,&goal);
        for(i=0;i<1000 && state->searching;i++) { now=i;flight_search(0,state); }
        CHECK(!state->searching && state->step_count>0,"bounded search discarded useful partial progress");
        CHECK(state->count<=FLIGHT_NODES && state->step_count<=FLIGHT_STEPS,"partial route exceeded bounds");
        for(i=0;i<state->step_count;i++) {
            real_point3d previous=i?state->steps[i-1]:start;
            CHECK(flight_clear(1,&previous,&state->steps[i],2,2),"partial route crosses barrier");
        }
        CHECK(flight_distance(&state->steps[state->step_count-1],&goal)<flight_distance(&start,&goal),"partial route made no progress");
    } else if (!strcmp(argv[1],"landing")) {
        CHECK(!enhanced_flight_can_exit(1),"airborne exit");
        goal=start; goal.z=0;
        enhanced_flight_action(0,1,TRUE,&goal,TRUE,FALSE,&action);
        CHECK(TEST_FLAG(action.control_flags,_unit_control_crouch_modifier_bit) && action.throttle.i==0,"no descent");
        craft.object.position.z=1;
        CHECK(enhanced_flight_can_exit(1),"landed craft cannot exit");
        craft.object.position.z=10;craft.vehicle.on_ground_ticks=5;
        CHECK(enhanced_flight_can_exit(1),"large hull ground contact ignored");
        craft.object.translational_velocity.i=.1f;
        CHECK(!enhanced_flight_can_exit(1),"moving exit");
        craft.vehicle.on_ground_ticks=0;craft.object.position.z=1;
        craft.object.translational_velocity.i=.3f;craft.object.translational_velocity.k=-.03f;
        CHECK(enhanced_flight_can_exit(1),"controlled low approach could not dismount before impact");
        craft.object.translational_velocity.k=-.1f;
        CHECK(!enhanced_flight_can_exit(1),"falling aircraft allowed unsafe dismount");
        craft.object.translational_velocity.k=0;craft.object.translational_velocity.i=.5f;
        CHECK(!enhanced_flight_can_exit(1),"fast flyby allowed unsafe dismount");
    } else if (!strcmp(argv[1],"takeoff")) {
        craft.object.position.z=craft.object.bounding_sphere_center.z=3;
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(action.throttle.i>0 && action.desired_facing.pitch==0 && TEST_FLAG(action.control_flags,_unit_control_jump_bit),"no takeoff");
        craft.object.forward.i=-1;
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(action.throttle.i==0 && fabs(action.desired_facing.yaw)<0.01f && TEST_FLAG(action.control_flags,_unit_control_jump_bit),"parked aircraft drove into wall instead of turning toward exit");
        craft.object.forward.i=1;craft.vehicle.on_ground_ticks=5;
        enhanced_flight_action(0,1,FALSE,&goal,FALSE,FALSE,&action);
        CHECK(action.throttle.i>=.8f && action.desired_facing.pitch>0,"grounded fighter could not build takeoff airspeed");
        craft.object.forward.i=-1;craft.vehicle.on_ground_ticks=0;
        sealed=1; action.throttle.i=1;action.desired_facing.yaw=0;
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(action.throttle.i==0 && !TEST_FLAG(action.control_flags,_unit_control_jump_bit),"takeoff through obstruction");
        CHECK(cos(action.desired_facing.yaw)<-0.999f && action.desired_facing.pitch==0,"route wait leaked combat steering into aircraft");
    } else if (!strcmp(argv[1],"clearance")) {
        wall=1; start.y=goal.y=11;
        CHECK(flight_clear(1,&start,&goal,0,0),"center ray blocked");
        CHECK(!flight_clear(1,&start,&goal,2,2),"wide custom hull clipped wall");
    } else if (!strcmp(argv[1],"retarget")) {
        action.desired_facing.yaw=3.0f; now=0;
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(fabs(action.desired_facing.yaw)<.1f,"combat aim hijacked flight");
        goal.y=60; now=8;
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(flights[0].destination.y==60,"stale destination");
        CHECK(fabs(action.desired_facing.yaw-0.785398f)<0.01f,"aircraft did not receive the absolute route heading");
        enhanced_flight_reset(0);
        CHECK(flights[0].vehicle==NONE && !flights[0].step_count,"flight leaked across respawn");
    } else if (!strcmp(argv[1],"braking")) {
        craft.object.translational_velocity.i=1.0f;
        enhanced_flight_action(0,1,TRUE,&goal,TRUE,FALSE,&action);
        CHECK(state->phase==_flight_approach && action.throttle.i==0,"fast custom craft did not brake early");
        craft.object.translational_velocity.i=.01f;now=1;
        enhanced_flight_action(0,1,TRUE,&goal,TRUE,FALSE,&action);
        CHECK(state->phase==_flight_approach,"slowing craft switched back to cruise");
    } else if (!strcmp(argv[1],"recovery")) {
        enhanced_flight_action(0,1,TRUE,&goal,TRUE,FALSE,&action);
        now=46*TICKS_PER_SECOND;
        enhanced_flight_action(0,1,TRUE,&goal,TRUE,FALSE,&action);
        CHECK(enhanced_flight_failed(0) && !action.throttle.i && TEST_FLAG(action.control_flags,_unit_control_crouch_modifier_bit),"blocked aircraft never recovered to ground");
        enhanced_flight_reset(0);
        CHECK(!enhanced_flight_failed(0),"recovery leaked into a later vehicle");
    } else if (!strcmp(argv[1],"shape")) {
        craft.object.bounding_sphere_radius=12;
        CHECK(flight_height(1)==12,"missing shape did not retain sphere");
        unit_tag.object.physics.index=0;physics_tag.mass_points.count=2;
        mass[0].position.y=11;mass[0].radius=1;
        mass[1].position.z=-1;mass[1].radius=1;
        CHECK(flight_height(1)==2,"flat aircraft required wingspan in vertical clearance");
        craft.object.scale=2;
        CHECK(flight_height(1)==2,"render scale incorrectly changed engine physics hull");
        craft.object.scale=0;
        CHECK(flight_height(1)==2,"engine default scale was treated as missing physics");
        ceiling=1;start.z=goal.z=6;
        unit_tag.object.physics.index=NONE;
        CHECK(!flight_clear(1,&start,&goal,12,12),"sphere fixture did not meet hangar roof");
        unit_tag.object.physics.index=0;
        CHECK(flight_clear(1,&start,&goal,12,flight_height(1)),"thin aircraft could not leave low hangar");
        physics_tag.center_of_mass.z=-10;
        CHECK(!flight_clear(1,&start,&goal,12,flight_height(1)),"center of mass offset omitted from hull sweep");
        physics_tag.center_of_mass.z=-3;
        CHECK(flight_height(1)==4,"center of mass offset omitted from height");
        physics_tag.center_of_mass.z=0;
        craft.object.up.j=1;craft.object.up.k=0;
        CHECK(flight_height(1)==12,"banked wing clearance ignored attitude");
        craft.object.up.j=0;craft.object.up.k=1;
        craft.object.position.z=craft.object.bounding_sphere_center.z=6;
        goal.z=0;state->radius=12;state->cruise_height=8;
        enhanced_flight_action(0,1,TRUE,&goal,FALSE,FALSE,&action);
        CHECK(state->target.z==8 && state->step_count>0 && action.throttle.i>0,"flat craft cruise target was above the hangar ceiling");
        physics_tag.mass_points.count=33;
        CHECK(flight_height(1)==12,"invalid shape count was used");
    } else CHECK(0,"unknown case");
    return 0;
}
