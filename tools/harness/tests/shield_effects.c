#include "harness.h"
#include "enums.inc"
/* The shield effects of a network game (port/linux/game/network_shield_effects.c):
the host's choice for every machine, read from network.shield_effects, or
"players", each machine's own settings; the host's message a client takes,
checked whole. */
typedef unsigned short word;
typedef word message_header;
#define GET_MESSAGE_TYPE(message) (((message) >> 2) & 3)
struct distributed_message_header { message_header header; byte type; byte count; long game_time; };
static char const *setting = "stock";
static unsigned long changes = 1;
static short connection = _game_connection_local;
static boolean unreliable;
static boolean coop;
static boolean network_coop_active(void){return coop;}
static char const *config_string(char const *name){(void)name;return setting;}
static unsigned long config_changes(void){return changes;}
static short game_connection(void){return connection;}
static boolean network_connection_last_read_was_unreliable(void){return unreliable;}
#include "under_test.inc"

/* the host's message, as shield_effects_message_make makes it, from the host's stream */
static struct shield_effects_message message_of(word effects)
{
 struct shield_effects_message message;
 memset(&message,0,sizeof(message));
 message.header.header=2<<2;message.header.type=_distributed_message_shield_effects;message.header.count=1;
 message.effects=effects;
 return message;
}

static void take(struct shield_effects_message message)
{
 network_shield_effects_handle_message(NONE,(word const *)&message,sizeof(message));
}

int main(int argc,char **argv)
{
 const char *case_name=argc>1?argv[1]:"";
 CASE("offline") {
  setting="light_show";changes++;
  CHECK(network_shield_effects()==NONE,"offline: each player's own settings");
 } else CASE("host") {
  connection=_game_connection_network_server;
  setting="stock";changes++;
  CHECK(network_shield_effects()==_shield_effects_stock,"the default: stock, for everyone");
  setting="light_show_overshield";changes++;
  CHECK(network_shield_effects()==_shield_effects_light_show_overshield,"the host's choice");
  setting="players";changes++;
  CHECK(network_shield_effects()==NONE,"players: the host draws its own settings'");
  setting="nonsense";changes++;
  CHECK(network_shield_effects()==_shield_effects_stock,"an unknown value: stock");
 } else CASE("client") {
  connection=_game_connection_network_client;
  network_shield_effects_reset();
  CHECK(network_shield_effects()==_shield_effects_stock,"stock until the host's choice arrives");
  take(message_of(_shield_effects_glow));
  CHECK(network_shield_effects()==_shield_effects_glow,"the host's choice");
  take(message_of(_shield_effects_players));
  CHECK(network_shield_effects()==NONE,"players: the client draws its own settings'");
 } else CASE("client_untrusted") {
  struct shield_effects_message message;
  connection=_game_connection_network_client;
  network_shield_effects_reset();
  take(message_of(_shield_effects_glow));
  take(message_of(NUMBER_OF_SHIELD_EFFECTS));
  CHECK(network_shield_effects()==_shield_effects_glow,"a value past the last refused");
  message=message_of(_shield_effects_players);message.reserved=1;take(message);
  CHECK(network_shield_effects()==_shield_effects_glow,"reserved bytes set refused");
  unreliable=TRUE;take(message_of(_shield_effects_players));unreliable=FALSE;
  CHECK(network_shield_effects()==_shield_effects_glow,"an unreliable read refused");
  message=message_of(_shield_effects_players);
  network_shield_effects_handle_message(3,(word const *)&message,sizeof(message));
  CHECK(network_shield_effects()==_shield_effects_glow,"another machine than the host refused");
 } else CASE("coop") {
  /* an online co-op game: each player's own, whatever the host chose */
  connection=_game_connection_network_server;
  setting="light_show_overshield";changes++;
  coop=TRUE;
  CHECK(network_shield_effects()==NONE,"co-op host: each player's own");
  connection=_game_connection_network_client;
  network_shield_effects_reset();
  take(message_of(_shield_effects_glow));
  CHECK(network_shield_effects()==NONE,"co-op client: each player's own");
  coop=FALSE;
  CHECK(network_shield_effects()==_shield_effects_glow,"not co-op: the host's choice");
 } else return 2;
 return 0;
}
