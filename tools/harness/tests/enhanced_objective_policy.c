#include "harness.h"
enum { _bots_goal_objective=1 };
struct bots_bot { long player_index,goal_surface_index; short goal_kind; real_point3d goal_point; };
static struct { struct bots_bot bots[1]; } bots_globals;
struct player_datum { long unit_index; };
static struct player_datum player;
static int ctf=1,king,defender,carrier,on_hill,tactics;
static struct player_datum *player_get(long i) { (void)i; return &player; }
static void bots_tactics_check(long i,long p) { (void)i;(void)p; }
static boolean bots_ctf_active(void) { return ctf; }
static boolean bots_king_active(void) { return king; }
static boolean bots_ctf_defender(struct bots_bot *b,struct player_datum *p) { (void)b;(void)p;return defender; }
static boolean bots_ctf_carrying(struct bots_bot *b) { (void)b;return carrier; }
static boolean bots_ctf_urgent_goal(struct bots_bot *b,struct player_datum *p,real_point3d const *pos)
{ (void)b;(void)p;(void)pos;return carrier; }
static boolean bots_ctf_choose_goal(struct bots_bot *b,struct player_datum *p,real_point3d const *pos)
{ (void)p;(void)pos;b->goal_kind=_bots_goal_objective;b->goal_point.x=100;return TRUE; }
static boolean bots_king_choose_goal(struct bots_bot *b,struct player_datum *p,real_point3d const *pos)
{ (void)b;(void)p;(void)pos;return king; }
static boolean bots_king_on_hill(long u) { (void)u;return on_hill; }
static boolean infection_enabled(void) { return FALSE; }
static boolean infection_unit_is_zombie(long u) { (void)u;return FALSE; }
static boolean bots_infection_goal(struct bots_bot *b,struct player_datum *p,real_point3d const *pos)
{ (void)b;(void)p;(void)pos;return FALSE; }
static void bots_tactics_objective(long i) { (void)i; }
static boolean bots_tactics_goal(long i,long p,real_point3d const *pos,real_point3d *g,long *s)
{ (void)i;(void)p;(void)pos;g->x=20;*s=1;tactics++;return TRUE; }
#include "under_test.inc"
int main(int argc,char **argv) {
 real_point3d pos={0,0,0},goal;long surface;
 CHECK(argc==2,"case");
 defender=!strcmp(argv[1],"defender");carrier=!strcmp(argv[1],"carrier");
 if(!strcmp(argv[1],"hill")) {ctf=0;king=1;}
 CHECK(enhanced_bots_goal(0,0,&pos,FALSE,&goal,&surface),"no goal");
 if(!strcmp(argv[1],"attacker")) {
   CHECK(!tactics && goal.x==100,"attacker abandoned flag for vantage");
   CHECK(enhanced_bots_advancing(0,0),"attacker stopped advancing in combat");
 } else if(defender) { CHECK(tactics==1,"defender lost covering vantage");CHECK(!enhanced_bots_advancing(0,0),"defender became attacker"); }
 else if(carrier) CHECK(!tactics && enhanced_bots_advancing(0,0),"carrier abandoned return");
 else {CHECK(enhanced_bots_advancing(0,0),"off-hill bot stopped");on_hill=1;CHECK(!enhanced_bots_advancing(0,0),"hill holder left");}
 return 0;
}
