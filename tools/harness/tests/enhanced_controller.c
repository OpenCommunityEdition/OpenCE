#include "harness.h"
#include <math.h>
#define BOTS_MAXIMUM 63
#define TICKS_PER_SECOND 30
#define BOTS_SEARCHES_A_TICK 3
#define _object_mask_item 1
#define _unit_control_jump_bit 0
enum { _bots_goal_none, _bots_goal_enemy, _bots_goal_roam, _bots_goal_item, _bots_goal_enhanced, _bots_goal_vehicle };
enum { game_engine_slayer, game_engine_ctf, game_engine_king };
struct bots_bot {
    long player_index, goal_surface_index, goal_tick, goal_serial, path_failures, climb_until_tick, objective_retry_tick;
    long intent_surface, target_player_index, target_last_surface_index, target_seen_tick;
    long item_index, item_first_press_tick, item_check_tick;
    short goal_kind, intent_kind;
    boolean hunting;
    real_point3d goal_point, intent_point, target_last_position;
    struct { boolean valid; } path;
};
static struct { struct bots_bot bots[BOTS_MAXIMUM]; long search_cursor, tick_searches;
    long search_wait[BOTS_MAXIMUM], search_seen[BOTS_MAXIMUM];
    struct { real_point3d home[2]; } ctf;
} bots_globals;
static long now=100, parent=NONE;
static int seeking, carrying, hill, powerup_calls, urgent, failed;
struct player_datum { long unit_index; short team_index; };
static struct player_datum player;
struct unit_datum { int unused; };
static struct unit_datum unit;
struct object_datum { struct { long parent_object_index; real_point3d position; } object; };
struct item_datum { struct { long parent_object_index; real_point3d position; } object; };
static struct { int game_engine_index; } variant;
static long game_time_get(void) { return now; }
static struct player_datum *player_get(long i) { (void)i; return &player; }
static struct unit_datum *unit_get(long i) { (void)i; return &unit; }
static long bots_vehicle_parent(long i) { (void)i; return parent; }
static boolean enhanced_bots_carrying(long i) { (void)i; return carrying; }
static boolean bots_ctf_carrying(struct bots_bot *bot) { (void)bot; return carrying; }
static boolean enhanced_bots_holding_hill(long i) { (void)i; return hill; }
static boolean bots_vehicle_seeking(long i) { (void)i; return seeking; }
static void bots_vehicle_cancel(long i) { (void)i; seeking=FALSE; }
static boolean bots_vehicle_goal(long i,long u,real_point3d const *d,real_point3d *g,long *s)
{ (void)i;(void)u; CHECK(d->x == 100,"seat lost strategic destination");g->x=10;g->y=g->z=0;*s=2;return seeking; }
static boolean bots_vehicle_player_goal(long i,long u,real_point3d const *d,real_point3d *g,long *s)
{ (void)i;(void)u;(void)d;(void)g;(void)s;return FALSE; }
static boolean enhanced_bots_goal(long i,long p,real_point3d const *pos,boolean only,real_point3d *g,long *s)
{ (void)i;(void)p;(void)pos;if (only && !urgent && !carrying) return FALSE;g->x=100;g->y=g->z=0;*s=1;return TRUE; }
static boolean bots_powerup_goal(struct bots_bot *b,struct unit_datum *u,real_point3d const *p,boolean f)
{ (void)b;(void)u;(void)p;(void)f;powerup_calls++;return FALSE; }
#define game_engine_get_variant() (&variant)
static void *object_try_and_get_and_verify_type(long i,int t) { (void)i;(void)t;return NULL; }
static struct object_datum *object_get(long i) { (void)i;return NULL; }
static void bots_item_set_unreachable(struct bots_bot *b,long i) { (void)b;(void)i; }
static long bots_find_item(struct bots_bot *b,struct unit_datum *u,real_point3d const *p,real r)
{ (void)b;(void)u;(void)p;(void)r;return NONE; }
static long bots_surface_below(real_point3d const *p,real_point3d *g) { *g=*p;return 1; }
static void bots_pick_roam_goal(struct bots_bot *b,real_point3d const *p) { (void)p;b->goal_kind=_bots_goal_roam; }
static void enhanced_bots_failed(long i) { (void)i;failed++; }
static void bots_navigation_reset_bot(long i) { (void)i; }
enum { _collision_test_structure_bit, _collision_test_front_facing_surfaces_bit, _collision_test_objects_bit,
_collision_test_objects_scenery_bit, _collision_test_objects_machines_bit, _collision_test_objects_vehicles_bit };
struct collision_result { int unused; };
static int capture_wall;
static void vector_from_points3d(real_point3d const *a,real_point3d const *b,real_vector3d *d)
{ d->i=b->x-a->x;d->j=b->y-a->y;d->k=b->z-a->z; }
static boolean collision_test_vector(unsigned long f,real_point3d const *p,real_vector3d const *v,long u,struct collision_result *h)
{ (void)f;(void)p;(void)v;(void)u;(void)h;return capture_wall; }
#include "under_test.inc"
int main(int argc,char **argv)
{
    struct bots_bot *bot=&bots_globals.bots[0];
    real_point3d position={0,0,0}, goal={100,0,0};
    real direction=0;
    unsigned long flags=0;
    long i,t,served[BOTS_MAXIMUM]={0};
    CHECK(argc==2,"case");
    bot->target_player_index=NONE;bot->item_check_tick=1000;
    variant.game_engine_index=game_engine_ctf;
    if (!strcmp(argv[1],"seat")) {
        bots_set_goal(bot,_bots_goal_enhanced,&goal,1);
        seeking=TRUE;bots_choose_goal(bot,0,&position,FALSE);
        CHECK(bot->goal_kind==_bots_goal_vehicle,"seat not selected");
        bot->path.valid=TRUE;i=bot->goal_serial;
        for(t=0;t<60;t++){now++;bots_choose_goal(bot,0,&position,FALSE);}
        CHECK(bot->path.valid && bot->goal_serial==i,"seat path invalidated each tick");
        carrying=TRUE;bots_choose_goal(bot,0,&position,FALSE);
        CHECK(!seeking && bot->goal_kind==_bots_goal_enhanced,"carrier kept boarding intent");
    } else if (!strcmp(argv[1],"objectives")) {
        bot->target_player_index=1;bot->target_seen_tick=now;bot->target_last_surface_index=3;
        bots_choose_goal(bot,0,&position,FALSE);
        CHECK(bot->goal_kind==_bots_goal_enhanced,"CTF chased last seen enemy");
        hill=TRUE;variant.game_engine_index=game_engine_king;powerup_calls=0;
        bots_choose_goal(bot,0,&position,FALSE);
        CHECK(!powerup_calls,"hill holder diverted to powerup");
        bots_goal_failed(bot);bots_choose_goal(bot,0,&position,FALSE);
        CHECK(failed==1 && bot->goal_kind==_bots_goal_roam,"failed objective immediately reacquired");
        now+=301;bots_choose_goal(bot,0,&position,FALSE);
        CHECK(bot->goal_kind==_bots_goal_enhanced,"objective never retried");
    } else if (!strcmp(argv[1],"capture")) {
        carrying=TRUE;bots_globals.ctf.home[0].x=1;bots_globals.ctf.home[0].z=0.8f;now=112;
        CHECK(enhanced_bots_capture_approach(0,&position,&direction,&flags),"no stand approach");
        CHECK(TEST_FLAG(flags,_unit_control_jump_bit),"raised stand not approached with jump");
        capture_wall=1;CHECK(!enhanced_bots_capture_approach(0,&position,&direction,&flags),"capture cut through base wall");
        capture_wall=0;carrying=FALSE;CHECK(!enhanced_bots_capture_approach(0,&position,&direction,&flags),"noncarrier moved");
    } else if (!strcmp(argv[1],"fairness")) {
        for(t=0;t<100;t++){now++;bots_globals.tick_searches=0;
            for(i=0;i<BOTS_MAXIMUM;i++) if(bots_search_turn(i)){served[i]++;bots_globals.tick_searches++;}}
        for(i=0;i<BOTS_MAXIMUM;i++) CHECK(served[i]>=3,"search starvation at bot %ld",i);
        now+=4;bots_globals.tick_searches=0;CHECK(bots_search_turn(62),"abandoned request blocked searches");
    } else return 2;
    return 0;
}
