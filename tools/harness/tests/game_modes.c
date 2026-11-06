#include "harness.h"
#include <wchar.h>
#include "enums.inc"
/* OpenCE's gametypes (port/linux/game/game_modes.c, infection.c, gun_game.c,
one_in_the_chamber.c, frag_nation.c): a gametype's options kept in range, the
modes each game engine and teams allow, Infection's round result and the
state a client takes, Gun Game's list of weapons, One in the Chamber's kills
and rounds, and Frag Nation's grenades back. */
typedef unsigned short word;
#define VALID_INDEX(index, count) ((index) >= 0 && (index) < (count))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define NUMBEROF(array) ((long)(sizeof(array) / sizeof((array)[0])))
#define csmemset memset
#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS 4
#define INFECTION_MAXIMUM_SCORE 1000000
#include "structures.inc"

/* Gun Game's world: the map's weapons, the host or a client, the draw */
static struct game_variant_options options;
static boolean map_has[NUMBER_OF_LOADOUT_WEAPONS];
static boolean client;
static short draws;
static struct { byte levels[8]; short level_count; } gun_game;
static struct game_variant_options const *game_variant_options_get(void){return &options;}
static long game_engine_loadout_weapon_definition(byte weapon){return map_has[weapon]?100+weapon:NONE;}
static boolean network_game_distributed_client(void){return client;}
/* (the draw: the first of the range each time, so each weapon from the last
swaps with the first) */
static short local_random_range(short lower,short upper){draws++;(void)upper;return lower;}
/* Hardcore's world: a killer's unit, the host or a client */
enum { _object_dead_bit = 0, _object_shield_depleted_bit, _object_shield_charging_bit };
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))
struct player_datum { long unit_index; };
struct unit_datum { struct { real body_vitality, shield_vitality, maximum_shield_vitality; long damage_flags; } object;
 struct { long player_index; } unit; };
static struct player_datum killer_player = { 7 };
static struct unit_datum killer_unit;
static boolean hardcore=TRUE;
static boolean game_modes_hardcore(void){return hardcore;}
static struct player_datum *player_try_and_get(long index){return index==1?&killer_player:NULL;}
static struct unit_datum *unit_try_and_get(long index){return index==7?&killer_unit:NULL;}
/* the game's mode, and a biped hit (object 9: player 2's unit; object 8: no
player's) */
static short mode;
static short game_modes_mode(void){return mode;}
#define platform_log(...) ((void)0)
enum { _object_mask_biped = 1 };
static struct unit_datum target_unit, ai_unit;
static void *object_try_and_get_and_verify_type(long index,long mask)
 {(void)mask;return index==9?&target_unit:index==8?&ai_unit:NULL;}
#include "under_test.inc"

static void wounded_killer(real body,real shield)
{
 memset(&killer_unit,0,sizeof(killer_unit));
 killer_unit.object.body_vitality=body;killer_unit.object.shield_vitality=shield;
 killer_unit.object.maximum_shield_vitality=1.0f;
 SET_FLAG(killer_unit.object.damage_flags,_object_shield_depleted_bit,TRUE);
 hardcore=TRUE;client=FALSE;
}

static void map_with_every_weapon(void)
{
 for(int weapon=0;weapon<NUMBER_OF_LOADOUT_WEAPONS;weapon++) map_has[weapon]=weapon>=_loadout_weapon_assault_rifle;
}

static struct distributed_infection_state valid_state(void)
{
 struct distributed_infection_state state;
 memset(&state,0,sizeof(state));
 state.game_number=3;state.revision=5;state.phase=_distributed_infection_active;state.winner=NONE;
 state.players[0].identity=11;state.players[0].role=_distributed_infection_role_human;state.players[0].score=2;
 state.players[1].identity=12;state.players[1].role=_distributed_infection_role_zombie;
 return state;
}

static void hit_world(void)
{
 memset(&target_unit,0,sizeof(target_unit));memset(&ai_unit,0,sizeof(ai_unit));
 target_unit.unit.player_index=2;ai_unit.unit.player_index=NONE;
 mode=_game_mode_one_in_the_chamber;client=FALSE;
}

int main(int argc,char **argv)
{
 const char *case_name=argc>1?argv[1]:"";
 CASE("sanitize_defaults") {
  /* (a gametype saved before OpenCE's: every byte 0) */
  memset(&options,0,sizeof(options));
  game_modes_options_sanitize(&options);
  CHECK(options.mode==_game_mode_none&&!options.hardcore,"no mode");
  CHECK(options.infection_initial_zombies==1&&options.infection_zombie_speed==150&&
   options.infection_skull_damage==200&&options.infection_respawn_time==5,"Infection's defaults");
  CHECK(options.infection_countdown==0,"a countdown of 0 is one");
 } else CASE("options_default") {
  memset(&options,0xAA,sizeof(options));
  game_modes_options_default(&options);
  CHECK(options.mode==_game_mode_none&&!options.hardcore,"no mode, not Hardcore");
  CHECK(options.infection_countdown==10&&options.infection_zombie_shields&&options.gun_game_melee_demotes,
   "a countdown, zombies' shields, melee demotes");
  CHECK(options.time_limit==(short)0xAAAA,"the game type's own options left alone");
  CHECK(!options.chamber_kill_resets&&!options.frag_nation_no_map_grenades,"a kill adds a round, the map's grenades on");
  for(int index=0;index<(int)sizeof(options.reserved);index++) CHECK(options.reserved[index]==0,"reserved cleared");
  game_modes_options_sanitize(&options);
  CHECK(options.infection_countdown==10&&options.infection_zombie_speed==150,"the defaults are in range");
 } else CASE("hardcore_restore") {
  wounded_killer(0.25f,0.0f);
  game_modes_player_killed(1,2,FALSE);
  CHECK(killer_unit.object.body_vitality==1.0f&&killer_unit.object.shield_vitality==1.0f,"health and shields restored");
  CHECK(!TEST_FLAG(killer_unit.object.damage_flags,_object_shield_depleted_bit),"the shields no longer depleted");
  wounded_killer(0.5f,2.0f);
  game_modes_player_killed(1,2,FALSE);
  CHECK(killer_unit.object.shield_vitality==2.0f,"an overshield kept");
 } else CASE("hardcore_no_restore") {
  wounded_killer(0.25f,0.0f);hardcore=FALSE;
  game_modes_player_killed(1,2,FALSE);
  CHECK(killer_unit.object.body_vitality==0.25f,"not Hardcore");
  wounded_killer(0.25f,0.0f);
  game_modes_player_killed(1,2,TRUE);
  CHECK(killer_unit.object.body_vitality==0.25f,"a betrayal");
  game_modes_player_killed(1,1,FALSE);
  CHECK(killer_unit.object.body_vitality==0.25f,"a suicide");
  game_modes_player_killed(NONE,2,FALSE);
  CHECK(killer_unit.object.body_vitality==0.25f,"no killer");
  client=TRUE;
  game_modes_player_killed(1,2,FALSE);
  CHECK(killer_unit.object.body_vitality==0.25f,"a client's copy of the kill");
  client=FALSE;
  SET_FLAG(killer_unit.object.damage_flags,_object_dead_bit,TRUE);
  game_modes_player_killed(1,2,FALSE);
  CHECK(killer_unit.object.body_vitality==0.25f,"a killer already dead");
 } else CASE("sanitize_range") {
  /* (another machine's, or a file's, out of range) */
  memset(&options,0xFF,sizeof(options));
  game_modes_options_sanitize(&options);
  CHECK(options.mode==_game_mode_none,"an unknown mode is none");
  CHECK(options.hardcore==TRUE&&options.infection_zombie_shields==TRUE&&options.gun_game_melee_demotes==TRUE,
   "booleans are 0 or 1");
  CHECK(options.infection_initial_zombies==1&&options.infection_zombie_speed==150&&options.infection_skull_damage==200&&
   options.infection_respawn_time==5&&options.infection_countdown==10,"out of range: the defaults");
  CHECK(options.gun_game_order==_gun_game_order_strongest_first,"an unknown order");
  CHECK(!options.chamber_kill_resets&&!options.frag_nation_no_map_grenades,"the bytes taken from reserved: unknown is off");
  for(int index=0;index<(int)sizeof(options.reserved);index++) CHECK(options.reserved[index]==0,"reserved cleared");
 } else CASE("sanitize_keeps") {
  memset(&options,0,sizeof(options));
  options.mode=_game_mode_infection;options.infection_initial_zombies=16;options.infection_zombie_speed=200;
  options.infection_skull_damage=100;options.infection_respawn_time=30;options.infection_countdown=30;
  options.gun_game_order=_gun_game_order_random;
  options.chamber_kill_resets=TRUE;options.frag_nation_no_map_grenades=TRUE;
  game_modes_options_sanitize(&options);
  CHECK(options.chamber_kill_resets==TRUE&&options.frag_nation_no_map_grenades==TRUE,"the new options kept");
  CHECK(options.mode==_game_mode_infection&&options.infection_initial_zombies==16&&options.infection_zombie_speed==200&&
   options.infection_skull_damage==100&&options.infection_respawn_time==30&&options.infection_countdown==30&&
   options.gun_game_order==_gun_game_order_random,"values in range are kept");
 } else CASE("variant_mode") {
  struct game_variant variant;
  memset(&variant,0,sizeof(variant));memset(&options,0,sizeof(options));
  variant.game_engine_index=game_engine_slayer;
  options.mode=_game_mode_swat;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_swat,"SWAT, free for all");
  variant.universal_variant.teams=TRUE;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_swat,"SWAT, teams");
  options.mode=_game_mode_infection;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_infection,"Infection on teams");
  variant.universal_variant.teams=FALSE;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_none,"no Infection without teams");
  options.mode=_game_mode_gun_game;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_gun_game,"Gun Game, free for all");
  variant.universal_variant.teams=TRUE;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_none,"no Gun Game on teams");
  options.mode=_game_mode_one_in_the_chamber;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_none,"no One in the Chamber on teams");
  variant.universal_variant.teams=FALSE;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_one_in_the_chamber,"One in the Chamber, free for all");
  options.mode=_game_mode_frag_nation;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_frag_nation,"Frag Nation, free for all");
  variant.universal_variant.teams=TRUE;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_frag_nation,"Frag Nation, teams");
  options.mode=_game_mode_snowball_fight;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_snowball_fight,"Snowball Fight, teams");
  variant.universal_variant.teams=FALSE;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_snowball_fight,"Snowball Fight, free for all");
  variant.game_engine_index=game_engine_ctf;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_none,"Snowball Fight is Slayer's");
  options.mode=_game_mode_gun_game;
  variant.universal_variant.teams=FALSE;variant.game_engine_index=game_engine_ctf;
  CHECK(game_modes_variant_mode(&variant,&options)==_game_mode_none,"Slayer's modes only");
  CHECK(game_modes_variant_mode(NULL,&options)==_game_mode_none&&game_modes_variant_mode(&variant,NULL)==_game_mode_none,
   "none without a gametype");
 } else CASE("round_result") {
  long limit=300*30;
  CHECK(infection_round_result(3,1,0,limit)==NONE,"humans and zombies left");
  CHECK(infection_round_result(0,4,10,limit)==1,"no human left: the zombies");
  CHECK(infection_round_result(2,1,limit,limit)==0,"the time out: the humans");
  CHECK(infection_round_result(0,4,limit,limit)==1,"the last human turned as the time runs out: the zombies");
  CHECK(infection_round_result(2,1,limit*10,0)==NONE,"no time limit");
  CHECK(infection_round_result(3,0,10,limit)==0,"every zombie gone: the humans");
  CHECK(infection_round_result(0,0,10,limit)==2,"everyone gone: no one");
 } else CASE("state_valid") {
  struct distributed_infection_state state=valid_state();
  CHECK(infection_state_valid(&state,3,5),"a state of the game, as new");
  CHECK(!infection_state_valid(&state,4,0),"another game's");
  CHECK(!infection_state_valid(&state,3,6),"older than the client's");
  state=valid_state();state.phase=_distributed_infection_finished+1;
  CHECK(!infection_state_valid(&state,3,0),"an unknown phase");
  state=valid_state();state.winner=1;
  CHECK(!infection_state_valid(&state,3,0),"a winner before the round is over");
  state.phase=_distributed_infection_finished;
  CHECK(infection_state_valid(&state,3,0),"a winner after");
  state.winner=2;
  CHECK(!infection_state_valid(&state,3,0),"no third side");
  state=valid_state();state.players[2].role=_distributed_infection_role_zombie+1;
  CHECK(!infection_state_valid(&state,3,0),"an unknown role");
  state=valid_state();state.players[2].role=_distributed_infection_role_human;
  CHECK(!infection_state_valid(&state,3,0),"a role with no player");
  state=valid_state();state.players[0].score=-1;
  CHECK(!infection_state_valid(&state,3,0),"a negative score");
  state=valid_state();state.players[0].score=INFECTION_MAXIMUM_SCORE+1;
  CHECK(!infection_state_valid(&state,3,0),"a score too large");
  state=valid_state();state.players[1].reserved[1]=1;
  CHECK(!infection_state_valid(&state,3,0),"reserved bytes set");
 } else CASE("gun_game_order") {
  map_with_every_weapon();memset(&options,0,sizeof(options));
  options.gun_game_order=_gun_game_order_strongest_first;
  gun_game_levels_build();
  CHECK(gun_game.level_count==8,"every weapon: %d",gun_game.level_count);
  CHECK(gun_game.levels[0]==_loadout_weapon_rocket_launcher&&gun_game.levels[7]==_loadout_weapon_pistol,
   "the rocket launcher first, the pistol last");
  options.gun_game_order=_gun_game_order_weakest_first;
  gun_game_levels_build();
  CHECK(gun_game.levels[0]==_loadout_weapon_pistol&&gun_game.levels[7]==_loadout_weapon_rocket_launcher,
   "the pistol first");
 } else CASE("gun_game_missing") {
  map_with_every_weapon();memset(&options,0,sizeof(options));
  map_has[_loadout_weapon_shotgun]=FALSE;map_has[_loadout_weapon_needler]=FALSE;
  gun_game_levels_build();
  CHECK(gun_game.level_count==6,"the map's weapons only: %d",gun_game.level_count);
  for(int level=0;level<gun_game.level_count;level++)
   CHECK(gun_game.levels[level]!=_loadout_weapon_shotgun&&gun_game.levels[level]!=_loadout_weapon_needler,"none missing");
 } else CASE("gun_game_random") {
  map_with_every_weapon();memset(&options,0,sizeof(options));
  options.gun_game_order=_gun_game_order_random;draws=0;
  gun_game_levels_build();
  CHECK(draws==7&&gun_game.level_count==8,"the host draws the order");
  CHECK(gun_game.levels[0]!=_loadout_weapon_rocket_launcher,"in another order");
  client=TRUE;draws=0;
  gun_game_levels_build();
  CHECK(draws==0&&gun_game.level_count==8,"a client draws nothing, its list as long");
  client=FALSE;
 } else CASE("kind") {
  struct game_variant variant;
  memset(&variant,0,sizeof(variant));memset(&options,0,sizeof(options));
  variant.game_engine_index=game_engine_slayer;variant.universal_variant.teams=TRUE;
  CHECK(game_modes_kind(&variant,&options)==0,"a stock gametype has no kind");
  CHECK(game_modes_kind_name(game_engine_slayer,0,TRUE)==NULL,"named by its engine");
  options.mode=_game_mode_infection;options.hardcore=TRUE;
  CHECK(game_modes_kind(&variant,&options)==(_game_mode_infection|0x80),"Infection, Hardcore");
  CHECK(!wcscmp(game_modes_kind_name(game_engine_slayer,game_modes_kind(&variant,&options),FALSE),
   L"HARDCORE INFECTION"),"in full");
  CHECK(!wcscmp(game_modes_kind_name(game_engine_slayer,game_modes_kind(&variant,&options),TRUE),L"HC INFECTION"),
   "abbreviated");
  options.mode=_game_mode_gun_game;
  CHECK(game_modes_kind(&variant,&options)==0x80,"a mode the teams do not allow is none");
  variant.game_engine_index=game_engine_ctf;options.mode=_game_mode_none;
  CHECK(!wcscmp(game_modes_kind_name(game_engine_ctf,game_modes_kind(&variant,&options),TRUE),L"HC CTF"),"HC CTF");
  CHECK(!wcscmp(game_modes_kind_name(game_engine_slayer,_game_mode_swat,TRUE),L"SWAT"),"SWAT");
  CHECK(!wcscmp(game_modes_kind_name(game_engine_slayer,_game_mode_one_in_the_chamber,FALSE),L"ONE IN THE CHAMBER"),
   "One in the Chamber");
  CHECK(!wcscmp(game_modes_kind_name(game_engine_slayer,_game_mode_frag_nation|0x80,TRUE),L"HC FRAG NATION"),
   "Hardcore Frag Nation, abbreviated");
  CHECK(!wcscmp(game_modes_kind_name(game_engine_slayer,_game_mode_snowball_fight,TRUE),L"SNOWBALL"),"Snowball Fight");
 } else CASE("kind_untrusted") {
  CHECK(game_modes_kind_name(game_engine_ctf,_game_mode_swat,TRUE)==NULL,"a mode of CTF is none");
  CHECK(!wcscmp(game_modes_kind_name(game_engine_oddball,_game_mode_infection|0x80,TRUE),L"HC ODDBALL"),
   "Hardcore kept, the mode not");
  CHECK(game_modes_kind_name(game_engine_slayer,0x7F,TRUE)==NULL,"a mode this build does not know");
  CHECK(game_modes_kind_name(game_engine_none,0x80,TRUE)==NULL,"no engine (co-op)");
  CHECK(game_modes_kind_name(99,0x80,TRUE)==NULL,"an engine this build does not know");
 } else CASE("chamber_round") {
  short loaded=0,carried=0;
  one_in_the_chamber_round_for_kill(&loaded,&carried,12,36,FALSE);
  CHECK(loaded==1&&carried==0,"a kill's round into the empty magazine: %d %d",loaded,carried);
  one_in_the_chamber_round_for_kill(&loaded,&carried,12,36,FALSE);
  CHECK(loaded==2&&carried==0,"one more: %d %d",loaded,carried);
  loaded=12;carried=0;
  one_in_the_chamber_round_for_kill(&loaded,&carried,12,36,FALSE);
  CHECK(loaded==12&&carried==1,"a full magazine: carried: %d %d",loaded,carried);
  loaded=12;carried=36;
  one_in_the_chamber_round_for_kill(&loaded,&carried,12,36,FALSE);
  CHECK(loaded==12&&carried==36,"no more than a pistol holds: %d %d",loaded,carried);
 } else CASE("chamber_round_resets") {
  short loaded=0,carried=0;
  one_in_the_chamber_round_for_kill(&loaded,&carried,12,36,TRUE);
  CHECK(loaded==1&&carried==0,"back to one round: %d %d",loaded,carried);
  loaded=5;carried=3;
  one_in_the_chamber_round_for_kill(&loaded,&carried,12,36,TRUE);
  CHECK(loaded==1&&carried==0,"from more, one round: %d %d",loaded,carried);
 } else CASE("chamber_hit_kills") {
  hit_world();
  CHECK(game_modes_hit_kills(1,9,TRUE),"another player's bullet kills");
  CHECK(!game_modes_hit_kills(1,9,FALSE),"a blow or an explosion does not");
  CHECK(!game_modes_hit_kills(2,9,TRUE),"nor a player's own bullet");
  CHECK(!game_modes_hit_kills(NONE,9,TRUE),"nor a bullet of no player's");
  CHECK(!game_modes_hit_kills(1,8,TRUE),"nor a hit on a biped of no player's");
  CHECK(!game_modes_hit_kills(1,7,TRUE),"nor on an object that is not a biped");
  client=TRUE;
  CHECK(!game_modes_hit_kills(1,9,TRUE),"a client's copy: the host decides");
  client=FALSE;mode=_game_mode_swat;
  CHECK(!game_modes_hit_kills(1,9,TRUE),"One in the Chamber only");
 } else CASE("map_grenades") {
  memset(&options,0,sizeof(options));
  mode=_game_mode_none;
  CHECK(!game_modes_map_grenades_off(),"no mode: the map's grenades");
  mode=_game_mode_one_in_the_chamber;
  CHECK(game_modes_map_grenades_off(),"One in the Chamber: none");
  mode=_game_mode_snowball_fight;
  CHECK(game_modes_map_grenades_off(),"Snowball Fight: none");
  mode=_game_mode_frag_nation;
  CHECK(!game_modes_map_grenades_off(),"Frag Nation: the map's");
  options.frag_nation_no_map_grenades=TRUE;
  CHECK(game_modes_map_grenades_off(),"Frag Nation without them");
 } else CASE("grenades_for_kill") {
  short frags,plasmas;
  frag_nation_grenades_for_kill(_frag_nation_kill_frag,FALSE,&frags,&plasmas);
  CHECK(frags==1&&plasmas==0,"a frag's kill: a frag");
  frag_nation_grenades_for_kill(_frag_nation_kill_plasma,FALSE,&frags,&plasmas);
  CHECK(frags==0&&plasmas==1,"a plasma's kill: a plasma");
  frag_nation_grenades_for_kill(_frag_nation_kill_plasma_stuck,FALSE,&frags,&plasmas);
  CHECK(frags==0&&plasmas==2,"a plasma stuck: two plasmas");
  frag_nation_grenades_for_kill(_frag_nation_kill_other,FALSE,&frags,&plasmas);
  CHECK(frags==0&&plasmas==0,"another kill: none");
  frag_nation_grenades_for_kill(_frag_nation_kill_frag,TRUE,&frags,&plasmas);
  CHECK(frags==0&&plasmas==0,"Snowball Fight: no frag");
  frag_nation_grenades_for_kill(_frag_nation_kill_plasma,TRUE,&frags,&plasmas);
  CHECK(frags==0&&plasmas==1,"Snowball Fight: a plasma's kill, a plasma");
 } else CASE("grenades_added") {
  CHECK(frag_nation_grenades_added(2,1,4)==3,"one more");
  CHECK(frag_nation_grenades_added(3,2,4)==4,"up to the most carried");
  CHECK(frag_nation_grenades_added(4,1,4)==4,"none past it");
  CHECK(frag_nation_grenades_added(5,1,4)==5,"a count past it kept");
  CHECK(frag_nation_grenades_added(0,4,2)==2,"a spawn's, capped");
  CHECK(frag_nation_grenades_added(1,0,4)==1,"nothing given");
 } else return 2;
 return 0;
}
