/* The real timing and point states, with allocation and attachment placement
   supplied by a small fake world. No renderer, map assets or physics engine. */
#include "harness.h"
typedef unsigned short word;
typedef struct { real alpha, red, green, blue; } real_argb_color;
struct location { short cluster_index; };
struct tag_reference { long index; };
#include "config.inc"
#define SET_FLAG(v,b,on) ((on) ? ((v) |= FLAG(b)) : ((v) &= ~FLAG(b)))
#define TICKS_PER_SECOND 30
#define MAXIMUM_CONTRAIL_INSTANCES 4
#define MAXIMUM_CONTRAIL_POINTS 512
#define TAG_BLOCK_GET_ELEMENT(block,index,type) (&((type *)(block)->address)[(int)(index)])
struct tag_block { long count; void *address; };
struct contrail_definition {
    unsigned long scale_flags;
    real point_generation_rate, frames_per_second, texture_animation_u, texture_animation_v;
    struct tag_block states;
};
static struct data_array *contrail_data, *contrail_point_data;
static struct contrail_definition test_definition;
static struct contrail_point_state states[3];
static long tick, trail_index;
static int active = TRUE;
static real physics_dt;
static boolean double_speed;
static boolean game_players_are_double_speed(void) { return double_speed; }
#define contrail_get(i) ((struct contrail_datum *)datum_get(contrail_data,i))
#define contrail_point_get(i) ((struct contrail_point_datum *)datum_get(contrail_point_data,i))
#define contrail_definition_get(i) (&test_definition)
#define game_time_get() tick
#define contrail_verify(i) ((void)0)
#define data_make_valid(d) ((void)0)
#define data_make_invalid(d) ((void)0)
#define point_physics_update(a,b,c,d,e,f,g,h,i,j,dt) (physics_dt += (dt))
static real real_local_random_range(real a, real b) { return a; }
static boolean object_get_function_value(long i, short f, real *density) { *density=1; return active; }
static void contrail_next_frame(struct contrail_datum *trail) { trail->frame_time=0; ++trail->frame_index; }
static long data_next_index(struct data_array *data, long previous)
{
    long i=previous==NONE ? 0 : DATUM_INDEX_TO_ABSOLUTE_INDEX(previous)+1;
    for(;i<data->maximum_count;++i)
    {
        struct datum_header *h=(struct datum_header *)(data->data+i*data->size);
        if(h->identifier) return ((long)(unsigned short)h->identifier<<16)|i;
    }
    return NONE;
}
static void contrail_delete(long i) { datum_delete(contrail_data,i); }
static void contrail_add_points(long i, short count, boolean force)
{
    struct contrail_datum *trail = contrail_get(i);
    short n;
    for (n=0; n<count; ++n)
    {
        long id = datum_new(contrail_point_data);
        struct contrail_point_datum *point;
        CHECK(id != NONE, "point pool");
        point=contrail_point_get(id);
        point->flags=FLAG(_contrail_point_new_bit)|FLAG(_contrail_point_transitioning_bit);
        point->state_index=NONE;
        point->density=trail->density;
        point->next_contrail_point_index=trail->first_contrail_point_indices[0];
        trail->first_contrail_point_indices[0]=id;
        ++trail->contrail_point_counts[0];
    }
}
#include "under_test.inc"
#include "after_load.inc"

static void setup(void)
{
    struct contrail_datum *trail;
    int i;
    if (contrail_data) { free(contrail_data->data); free(contrail_data); }
    if (contrail_point_data) { free(contrail_point_data->data); free(contrail_point_data); }
    contrail_data=game_state_data_new("contrails",4,sizeof(struct contrail_datum));
    contrail_point_data=game_state_data_new("points",512,sizeof(struct contrail_point_datum));
    contrails_initialize_for_new_map();
    memset(&test_definition,0,sizeof(test_definition));
    memset(states,0,sizeof(states));
    test_definition.point_generation_rate=30;
    test_definition.frames_per_second=10;
    test_definition.texture_animation_u=2;
    test_definition.states.count=3;
    test_definition.states.address=states;
    /* Short bright state and fade reproduce the Shade's timing contract. */
    states[0].duration_lower_bound=states[0].duration_upper_bound=0.01f;
    states[0].transition_duration_lower_bound=states[0].transition_duration_upper_bound=0.05f;
    states[0].width=0.15f;
    states[1].transition_duration_lower_bound=states[1].transition_duration_upper_bound=0.005f;
    for(i=0;i<3;++i) states[i].physics.index=NONE;
    tick=0; active=TRUE; physics_dt=0; double_speed=FALSE;
    trail_index=datum_new(contrail_data);
    trail=contrail_get(trail_index);
    trail->object_index=1; trail->density=1; trail->flags=FLAG(_contrail_active_bit);
    for(i=0;i<4;++i) trail->first_contrail_point_indices[i]=NONE;
    contrail_add_points(trail_index,1,TRUE);
}

static real width_sum(void)
{
    long id=contrail_get(trail_index)->first_contrail_point_indices[0];
    real sum=0;
    while(id!=NONE)
    {
        struct contrail_point_datum *p=contrail_point_get(id);
        real width;
        CHECK(p->state_index>=0 && p->state_index<3, "renderable point state");
        width=states[(int)p->state_index].width;
        if(TEST_FLAG(p->flags,_contrail_point_transitioning_bit))
            width=width*(1-p->time)+states[p->state_index+1].width*p->time;
        sum+=width;
        id=p->next_contrail_point_index;
    }
    return sum;
}
static boolean close_to(real a, real b) { real d=a-b; return d<0.0001f && d> -0.0001f; }
static void rates(void)
{
    int subdivisions[] = {1,2,4,8};
    real reference[30];
    int counts[30], rate, t, frame;
    for(rate=0;rate<4;++rate)
    {
        setup();
        /* Frame zero initializes at the same phase for every refresh rate. */
        contrails_update(0);
        for(t=0;t<30;++t)
        {
            for(frame=0;frame<subdivisions[rate];++frame)
            {
                if(frame==subdivisions[rate]-1) ++tick;
                contrails_update((1.0f/30)/subdivisions[rate]);
            }
            if(!rate) { reference[t]=width_sum(); counts[t]=contrail_get(trail_index)->contrail_point_counts[0]; }
            CHECK(close_to(width_sum(),reference[t]), "rate %d tick %d width %f expected %f",30*subdivisions[rate],t,width_sum(),reference[t]);
            CHECK(contrail_get(trail_index)->contrail_point_counts[0]==counts[t], "point count rate %d tick %d",rate,t);
        }
        CHECK(close_to(contrail_get(trail_index)->texture_offset_u,-2), "texture animation remains per frame");
    }
    CHECK(reference[15]>0.19f, "preserve the bright 30 Hz profile");
}
static void irregular_rate(void)
{
    int frame;
    real total=0;
    setup(); contrails_update(0);
    for(frame=1;frame<=1440;++frame)
    {
        tick=frame*30/144;
        contrails_update(1.0f/144);
        if(frame>144) total+=width_sum();
    }
    /* The tick/render phase varies, but must retain the 30 Hz bright body.
       The frame-aged negative control averages below 0.20. */
    CHECK(total/(1440-144)>0.22f, "144 Hz average trail width %f",total/(1440-144));
}
static void lifecycle(void)
{
    short before;
    setup(); contrails_update(0); contrails_update(0.02f);
    before=contrail_get(trail_index)->contrail_point_counts[0];
    contrails_dispose_from_old_map();
    CHECK(last_point_game_time==NONE, "unload resets clock");
    last_point_game_time=0;
    contrails_initialize_for_new_map();
    CHECK(last_point_game_time==NONE, "new map resets clock even at same tick");
    contrails_update(1.0f/30);
    CHECK(contrail_get(trail_index)->contrail_point_counts[0]==before+1, "first draw after reset emits a tick of points");
}
static void new_point(void)
{
    struct contrail_point_datum *p;
    setup(); contrails_update(0);
    contrail_add_points(trail_index,1,TRUE);
    p=contrail_point_get(contrail_get(trail_index)->first_contrail_point_indices[0]);
    contrails_update(1.0f/120);
    CHECK(p->state_index==0 && p->delta>0, "new collision/attachment point initialized before rendering");
    CHECK(p->time==0, "render-only frame does not age point");
    states[0].physics.index=1;
    contrails_update(1.0f/120);
    CHECK(close_to(physics_dt,contrail_get(trail_index)->contrail_point_counts[0]/120.0f), "all initialized points move between ticks");
    CHECK(close_to(contrail_get(trail_index)->texture_offset_u,-4.0f/120), "texture moves between ticks");
}
static void pause_and_activation(void)
{
    real width;
    int i;
    setup(); contrails_update(0); ++tick; contrails_update(1.0f/30);
    width=width_sum();
    for(i=0;i<20;++i) contrails_update(0);
    CHECK(close_to(width_sum(),width), "paused clock preserves trail");
    active=FALSE; contrails_update(1.0f/120);
    CHECK(!TEST_FLAG(contrail_get(trail_index)->flags,_contrail_active_bit), "attachment stops emitting");
    active=TRUE; contrails_update(1.0f/120);
    CHECK(TEST_FLAG(contrail_get(trail_index)->flags,_contrail_active_bit), "attachment reactivates between ticks");
    width_sum(); /* All point states must be initialized for the renderer. */
}
static void detached(void)
{
    int i;
    setup(); contrails_update(0);
    contrail_get(trail_index)->object_index=NONE;
    for(i=0;i<120;++i) { if(i%4==3) ++tick; contrails_update(1.0f/120); }
    CHECK(contrail_data->count==0 && contrail_point_data->count==0, "detached trail expires and is reclaimed");
}
static void checkpoint(void)
{
    setup(); contrails_update(0);
    /* A saved trail can have a fresh point even at the current tick. */
    contrail_get(trail_index)->time_until_point=0;
    game_state_call_after_load_procs();
    contrails_update(1.0f/120);
    CHECK(last_point_game_time==tick, "restored clock catches up");
    CHECK(contrail_get(trail_index)->time_until_point>0, "same-tick restore emits on its first draw");
}
static void catch_up(void)
{
    struct contrail_point_datum *p;
    short state;
    real time, motion;
    setup(); contrails_update(0);
    contrail_get(trail_index)->object_index=NONE;
    p=contrail_point_get(contrail_get(trail_index)->first_contrail_point_indices[0]);
    states[0].physics.index=states[1].physics.index=1;
    contrail_update_points(trail_index,1.0f/30,1.0f/30);
    contrail_update_points(trail_index,1.0f/30,1.0f/30);
    state=p->state_index; time=p->time; motion=physics_dt;
    setup(); contrails_update(0);
    contrail_get(trail_index)->object_index=NONE;
    p=contrail_point_get(contrail_get(trail_index)->first_contrail_point_indices[0]);
    states[0].physics.index=states[1].physics.index=1;
    contrail_update_points(trail_index,2.0f/30,2.0f/30);
    CHECK(p->state_index==state && close_to(p->time,time), "two-tick frame follows individual state transitions");
    CHECK(close_to(physics_dt,motion), "catch-up moves points once for elapsed frame time");
}
static void double_speed_case(void)
{
    struct contrail_point_datum *p;
    setup(); double_speed=TRUE;
    states[0].duration_lower_bound=states[0].duration_upper_bound=1;
    contrail_get(trail_index)->time_until_point=2;
    contrails_update(0);
    p=contrail_point_get(contrail_get(trail_index)->first_contrail_point_indices[0]);
    ++tick; contrails_update(1.0f/60);
    CHECK(p->state_index==0 && close_to(p->time,1.0f/60), "double-speed tick ages half a normal tick");
}
int main(int argc,char **argv)
{
    CHECK(argc==2,"case required");
    if(!strcmp(argv[1],"rates")) rates();
    else if(!strcmp(argv[1],"irregular")) irregular_rate();
    else if(!strcmp(argv[1],"pause-activation")) pause_and_activation();
    else if(!strcmp(argv[1],"lifecycle")) lifecycle();
    else if(!strcmp(argv[1],"new-point")) new_point();
    else if(!strcmp(argv[1],"detached")) detached();
    else if(!strcmp(argv[1],"checkpoint")) checkpoint();
    else if(!strcmp(argv[1],"catch-up")) catch_up();
    else if(!strcmp(argv[1],"double-speed")) double_speed_case();
    else CHECK(FALSE,"unknown case");
    return 0;
}
