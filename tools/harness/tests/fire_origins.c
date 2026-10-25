#include "harness.h"
#include <math.h>
/* network_damage.c's fire origins: where a client's players' shots leave from
when not from their units' cameras (a VR build's hand), which the host's line
of sight check also looks from. Machine 1 has players 0 and 2, machine 2 has
player 3; player 5 is in no game. Local players: 0 is player 0, 1 none. */
#define MAXIMUM_TRACKED_PLAYERS 16
#define MAXIMUM_LOCAL_PLAYERS 4
#define csmemcpy memcpy
#define csmemset memset
typedef unsigned short word;
typedef struct { short size; } message_header;
enum { _distributed_message_fire_origins = 92 };
enum { _distributed_to_clients, _distributed_to_host };
static long now = 300;
static long game_time_get(void) { return now; }
static int players[MAXIMUM_TRACKED_PLAYERS] = { 1, 0, 1, 1 };
static void *distributed_player(short player_index)
{
 return player_index >= 0 && player_index < MAXIMUM_TRACKED_PLAYERS && players[player_index] ? &players[player_index] : NULL;
}
static boolean distributed_machine_has_player(long machine_index, short player_index)
{
 return (machine_index == 1 && (player_index == 0 || player_index == 2)) || (machine_index == 2 && player_index == 3);
}
static long local_player_get_player_index(short local_player_index) { return local_player_index == 0 ? 0 : NONE; }
static byte distributed_player_to_byte(long player_index) { return (byte)player_index; }
#include "under_test.inc"
/* what the client sent */
static int sends, sent_type, sent_count, sent_to;
static struct distributed_fire_origin sent[MAXIMUM_LOCAL_PLAYERS];
static void distributed_send(void const *message, byte type, short count, unsigned short size, int to)
{
 sends++; sent_type = type; sent_count = count; sent_to = to;
 memcpy(sent, (byte const *)message + sizeof(struct distributed_message_header), count * sizeof(*sent));
 (void)size;
}
#include "under_test_send.inc"
static real wanted_i = 0.25f;
static boolean source_answer = TRUE;
static boolean source(short local_player_index, real_vector3d *offset)
{
 offset->i = wanted_i; offset->j = -0.1f; offset->k = 0.05f;
 return local_player_index == 0 && source_answer;
}
static void entry(struct distributed_fire_origin *origin, byte player_index, real i)
{
 real_vector3d offset = { i, 0.0f, 0.0f };
 memset(origin, 0, sizeof(*origin));
 origin->player_index = player_index;
 distributed_vector_pack(&offset, FIRE_ORIGIN_SCALE, &origin->offset);
}
int main(int argc, char **argv)
{
 const char *case_name = argc > 1 ? argv[1] : "";
 struct distributed_fire_origin origins[2];
 short index;
 for (index = 0; index < MAXIMUM_TRACKED_PLAYERS; index++)
  damage_fire_origins[index].time = NONE;
 CASE("own") {
  entry(&origins[0], 2, 0.3f);
  network_damage_handle_fire_origins(1, origins, 1);
  CHECK(damage_fire_origins[2].time == now, "a player of the machine's own, %ld", damage_fire_origins[2].time);
  CHECK(fabsf(damage_fire_origins[2].offset.i - 0.3f) < 0.001f, "where from, %f", damage_fire_origins[2].offset.i);
 } else CASE("foreign") {
  entry(&origins[0], 3, 0.3f);
  entry(&origins[1], 5, 0.3f);
  network_damage_handle_fire_origins(1, origins, 2);
  CHECK(damage_fire_origins[3].time == NONE, "another machine's player");
  CHECK(damage_fire_origins[5].time == NONE, "a player in no game");
  entry(&origins[0], 200, 0.3f);
  network_damage_handle_fire_origins(1, origins, 1);
 } else CASE("reach") {
  entry(&origins[0], 0, 0.45f);
  entry(&origins[1], 2, 0.39f);
  network_damage_handle_fire_origins(1, origins, 2);
  CHECK(damage_fire_origins[0].time == NONE, "further than a player reaches");
  CHECK(damage_fire_origins[2].time == now, "within reach");
 } else CASE("send") {
  network_damage_set_fire_origin_source(NULL);
  distributed_send_fire_origins();
  CHECK(sends == 0, "a build whose shots leave from the camera sends nothing");
  network_damage_set_fire_origin_source(source);
  distributed_send_fire_origins();
  CHECK(sends == 1 && sent_type == _distributed_message_fire_origins && sent_to == _distributed_to_host,
   "to the host, %d", sends);
  CHECK(sent_count == 1 && sent[0].player_index == 0, "the local player's, %d", sent_count);
  CHECK(sent[0].offset.i == (short)(0.25f * FIRE_ORIGIN_SCALE + 0.5f), "where from, %d", sent[0].offset.i);
  source_answer = FALSE;
  distributed_send_fire_origins();
  CHECK(sends == 1, "none when from the camera");
  source_answer = TRUE;
  wanted_i = 0.5f;
  distributed_send_fire_origins();
  CHECK(sends == 1, "none further than a player reaches");
 } else return 2;
 return 0;
}
