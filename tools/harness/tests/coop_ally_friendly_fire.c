#include "harness.h"
#include "enums.inc"
/* game_engine_friendly_damage (game_engine.c) in network co-op: a player's
hit of an AI ally, a player or a thing, with Server Setup's ALLY FRIENDLY FIRE
on and off. Objects 0 and 1 are the players' bipeds, 2 an AI ally (a Marine),
3 no unit */
struct game_variant_options { short friendly_fire; };
struct network_game { struct game_variant_options variant_options; };
struct player_datum { short identifier; long unit_index; };
struct unit_datum { struct { long player_index; } unit; };
enum { _object_mask_unit = 1 };
static struct game_variant_options variant_options;
static struct network_game game;
static struct { struct { boolean teams; } universal_variant; } global_variant;
static void *game_engine;
static void *player_data;
static boolean coop_active, players_friendly_fire, ally_friendly_fire;
static struct unit_datum units[3];
static struct player_datum players[2];
static struct game_variant_options const *game_variant_options_get(void){return &variant_options;}
static boolean network_coop_active(void){return coop_active;}
static struct network_game *network_game_get_game(void){return &game;}
static void *object_try_and_get_and_verify_type(long object_index,long mask)
{
 (void)mask;return object_index>=0&&object_index<3?&units[object_index]:NULL;
}
static boolean network_coop_players_friendly_fire(long attacker,long victim){(void)attacker;(void)victim;return players_friendly_fire;}
static boolean network_coop_ally_friendly_fire(void){return ally_friendly_fire;}
static void *datum_try_and_get(void *array,long index){(void)array;return index>=0&&index<2?&players[index]:NULL;}
#include "under_test.inc"
static void setup(void)
{
 coop_active=TRUE;players_friendly_fire=TRUE;ally_friendly_fire=TRUE;
 game.variant_options.friendly_fire=_friendly_fire_on;
 units[0].unit.player_index=0;units[1].unit.player_index=1;units[2].unit.player_index=NONE;
 players[0].unit_index=0;players[1].unit_index=1;
}
int main(int argc,char **argv)
{
 const char *case_name=argc>1?argv[1]:"";setup();
 CASE("ally_on") {
  for(int explosive=0;explosive<2;explosive++)
   CHECK(game_engine_friendly_damage(0,2,explosive)==_friendly_damage_all,"a player's hit of an ally, ON");
 } else CASE("ally_off") {
  ally_friendly_fire=FALSE;
  for(int explosive=0;explosive<2;explosive++)
   CHECK(game_engine_friendly_damage(0,2,explosive)==_friendly_damage_none,"a player's hit of an ally, OFF");
 } else CASE("ally_off_ai_hit") {
  ally_friendly_fire=FALSE;
  CHECK(game_engine_friendly_damage(NONE,2,TRUE)==_friendly_damage_all,"the AI's own hit of its ally");
 } else CASE("ally_off_players") {
  ally_friendly_fire=FALSE;
  CHECK(game_engine_friendly_damage(0,1,FALSE)==_friendly_damage_all,"a player's hit of a player, FRIENDLY FIRE ON");
  game.variant_options.friendly_fire=_friendly_fire_off;
  CHECK(game_engine_friendly_damage(0,1,FALSE)==_friendly_damage_none,"a player's hit of a player, FRIENDLY FIRE OFF");
  game.variant_options.friendly_fire=_friendly_fire_shields_only;
  CHECK(game_engine_friendly_damage(0,1,FALSE)==_friendly_damage_shields,"SHIELD ONLY");
  game.variant_options.friendly_fire=_friendly_fire_on;players_friendly_fire=FALSE;
  CHECK(game_engine_friendly_damage(0,1,FALSE)==_friendly_damage_none,"a player's own choice of no friendly fire");
 } else CASE("ally_off_not_unit") {
  ally_friendly_fire=FALSE;
  CHECK(game_engine_friendly_damage(0,3,FALSE)==_friendly_damage_all,"a player's hit of what is no unit");
 } else CASE("not_coop") {
  coop_active=FALSE;ally_friendly_fire=FALSE;
  CHECK(game_engine_friendly_damage(0,2,FALSE)==_friendly_damage_all,"the campaign alone, no game engine");
 } else return 2;
 return 0;
}
