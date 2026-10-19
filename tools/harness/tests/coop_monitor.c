#include "harness.h"
#include <stdarg.h>
#define NONE (-1)
#define PIN(v,l,h) ((v)<(l)?(l):((v)>(h)?(h):(v)))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define VALID_INDEX(i,n) ((i)>=0&&(i)<(n))
#define COOP_ENEMIES_SPREAD_SPOTS 90
#define BIT_VECTOR_SIZE_IN_LONGS(n) (((n)+31)/32)
#define SET_FLAG(v,n,b) ((b)?((v)|=1UL<<(n)):((v)&=~(1UL<<(n))))
#define csmemset memset
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
enum {_game_difficulty_level_easy,_game_difficulty_level_normal,_game_difficulty_level_hard,_game_difficulty_level_impossible};
enum {_unique_leader_type_normal,_unique_leader_type_random,_unique_leader_type_sergeant_johnson,_unique_leader_type_sergeant_lehto,_unique_leader_type_none};
struct block {short count;void *address;};
struct tag_reference {long index;};
struct actor_starting_location {real_point3d position;short actor_variant_index;};
struct squad_definition {char *name;short actor_palette_index,min_count,max_count,platoon_index,unique_leader_type,major_upgrade;struct block starting_locations;};
struct platoon_definition {char *name;};
struct encounter_definition {char *name;struct block squads,platoons;};
struct encounter_datum {short unique_leader_count,original_count;};
struct actor_variant_definition {struct tag_reference actor_reference,major_upgrade_reference;};
struct actor_definition {short type;};
struct scenario {struct block ai_encounters,ai_actor_palette;};
static struct actor_starting_location locations[2];
static struct squad_definition squads[2];
static struct encounter_definition encounters[1];
static struct tag_reference palette[4];
static struct actor_variant_definition variants[4];
static struct actor_definition definitions[4];
static struct scenario scenario;
static struct encounter_datum encounter;
static struct {int ai_initialized_for_map;} globals={1},*ai_globals=&globals;
static struct {int print_placement;} ai_debug;
static int extra=3,extra_queries,placed[4],upgrade_choice;
static real_point3d last_position;
static struct scenario *global_scenario_get(void){return &scenario;}
static struct encounter_datum *encounter_get(long i){(void)i;return &encounter;}
static struct encounter_definition *encounter_get_definition(long i){(void)i;return encounters;}
static struct actor_variant_definition *actor_variant_definition_get(long i){CHECK(VALID_INDEX(i,4),"variant index");return &variants[i];}
static struct actor_definition *actor_definition_get(long i){CHECK(VALID_INDEX(i,4),"actor index");return &definitions[i];}
static short encounter_get_actor_starting_location(long e,short s,int spawning){(void)e;(void)s;(void)spawning;return 0;}
static int game_time_get(void){return 1;}
static int coop_enemies_spread_position(real_point3d const *p,short n,long *taken,real_point3d *out){(void)n;(void)taken;*out=*p;out->x+=1;return 1;}
static int coop_enemies_fallback_position(real_point3d const *p,short n,real_point3d *out){(void)n;*out=*p;return 1;}
static void ai_get_major_upgrade_chance(short choice,int *upgrade,int *random,float *chance){(void)choice;*upgrade=upgrade_choice;*random=0;*chance=0;}
static int ai_consider_major_upgrade(long e,short s,float c){(void)e;(void)s;(void)c;return 0;}
static long actor_place(long v,long e,long s,struct actor_starting_location *p,int upgrade,short initial){(void)e;(void)s;(void)initial;if(upgrade)v=variants[v].major_upgrade_reference.index;placed[v]++;last_position=p->position;return 1;}
static int game_difficulty_level_get(void){return _game_difficulty_level_normal;}
static void console_printf(int b,char const *fmt,...){(void)b;(void)fmt;}
static void error(int kind,char const *fmt,...){(void)kind;(void)fmt;}
#define _error_silent 0
static void match_vassert(char const *f,int line,int condition,void *p){(void)f;(void)line;(void)p;CHECK(condition,"assertion");}
static int random_range(int a,int b){(void)b;return a;}
static void encounter_update_status(long i){(void)i;}
static void encounters_test_activation(void){}
static short coop_enemies_extra_count(long e,short n){(void)e;extra_queries++;return extra*n;}
static short squad_get_actor_type(struct squad_definition *s);
#include "under_test.inc"
static short squad_get_actor_type(struct squad_definition *s){return definitions[variants[palette[s->actor_palette_index].index].actor_reference.index].type;}

int main(int argc,char **argv)
{
 const char *case_name=argc>1?argv[1]:"";
 for(int i=0;i<4;i++){palette[i].index=i;variants[i].actor_reference.index=i;variants[i].major_upgrade_reference.index=NONE;definitions[i].type=_actor_sentinel;}
 definitions[1].type=_actor_monitor;definitions[2].type=_actor_grunt;
 for(int i=0;i<2;i++){locations[i].actor_variant_index=NONE;locations[i].position.x=10+i;squads[i].min_count=squads[i].max_count=1;squads[i].unique_leader_type=_unique_leader_type_none;squads[i].starting_locations=(struct block){1,&locations[i]};}
 encounters[0].name="fixture";encounters[0].squads=(struct block){1,squads};
 scenario.ai_encounters=(struct block){1,encounters};scenario.ai_actor_palette=(struct block){4,palette};
 CASE("monitor") {squads[0].actor_palette_index=1;encounter_create(0,NONE,NONE);CHECK(placed[1]==1&&!extra_queries,"Monitor squad must retain exactly its authored actor without scaling queries");}
 else CASE("combat") {for(int i=0;i<4;i++){extra=i;memset(placed,0,sizeof(placed));encounter_create(0,NONE,NONE);CHECK(placed[0]==1+i,"Sentinel scaling changed");}}
 else CASE("mixed") {encounters[0].squads.count=2;squads[0].actor_palette_index=1;squads[1].actor_palette_index=2;encounter_create(0,NONE,NONE);CHECK(placed[1]==1&&placed[2]==4&&extra_queries==1,"Monitor exemption must not suppress combat squads in the same encounter");}
 else CASE("override") {locations[0].actor_variant_index=1;CHECK(!encounter_place_actor(0,0,0,FALSE,1)&&!placed[1],"starting-location override created an extra Monitor");}
 else CASE("upgrade") {variants[0].major_upgrade_reference.index=1;upgrade_choice=1;CHECK(!encounter_place_actor(0,0,0,FALSE,1)&&!placed[1],"major upgrade created an extra Monitor");upgrade_choice=0;CHECK(encounter_place_actor(0,0,0,FALSE,1)&&placed[0]==1,"unused major upgrade suppressed a Sentinel");}
 else CASE("original") {locations[0].actor_variant_index=1;CHECK(encounter_place_actor(0,0,0,TRUE,0)&&placed[1]==1&&last_position.x==10,"authored Monitor placement changed");}
 else CASE("inactive") {globals.ai_initialized_for_map=0;squads[0].actor_palette_index=1;encounter_create(0,NONE,NONE);CHECK(!placed[1]&&!extra_queries,"uninitialized AI placed actors");}
 else return 2;
 return 0;
}
