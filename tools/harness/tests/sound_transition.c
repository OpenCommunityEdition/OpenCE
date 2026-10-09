/* Real manager structures and functions; only the cache/channel hardware is fake. */
#include "harness.h"
#include <float.h>
#include "types.inc"
#define SET_FLAG(v,b,s) ((v)=(s)?((v)|FLAG(b)):((v)&~FLAG(b)))
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
static struct sound_datum sounds[2];
static struct sound_channel_datum channels[2];
static struct sound_definition test_definition;
static struct sound_pitch_range range;
static struct sound_permutation permutation;
static struct looping_sound_track track;
static struct looping_sound_definition loop_definition;
static struct looping_sound_datum loop;
static struct sound_class_definition sound_class;
static struct sound_manager_globals sound_manager_globals;
static int queues, property_writes, updates;
static void channel_update(short i) { (void)i; updates++; }
static struct sound_platform_definition platform = {.channel_update=channel_update};
static real sound_pitch_range_fade_time;
static struct sound_datum *checked_sound(long i) {
 CHECK(i>=0 && i<2 && sounds[i].identifier, "access to a retired voice"); return &sounds[i];
}
#define sound_get(i) checked_sound(i)
#define channel_get(i) (&channels[i])
#define sound_definition_get(...) (&test_definition)
#define looping_sound_get(...) (&loop)
#define looping_sound_definition_get(...) (&loop_definition)
#define sound_class_get(...) (&sound_class)
#define sound_valid_for_channel(...) TRUE
#define source_distance_squared(...) 0.f
#define sound_definition_find_pitch_range_by_pitch(...) 0
#define sound_definition_next_permutation(...) 0
#define sound_scale_value(a,...) (a)
#define sound_definition_get_minimum_distance(...) 0.f
#define sound_manager_master_gain(...) 1.f
#define limit_pitch(a,...) (a)
#define sound_cache_sound_loaded(...) TRUE
#define _sound_cache_sound_request(...) TRUE
#define channel_get_state(...) _sound_channel_playing
#define channel_set_properties_hardware(...) ((void)property_writes++)
#define channel_queue_sound(...) ((void)queues++)
#define update_potentially_audible_looping_sound(...) (abort(), (long)NONE)
#define sound_start_fade(...) abort()
static void sound_stop(long i) {
 sounds[i].identifier=0; channels[i].sound_index=NONE; sounds[i].playing_channel_index=NONE;
}
#include "under_test.inc"
static void reset(void) {
 memset(sounds,0,sizeof(sounds)); memset(channels,0,sizeof(channels)); memset(&test_definition,0,sizeof(test_definition));
 sounds[0].identifier=sounds[1].identifier=1;
 sounds[0].playing_channel_index=0; sounds[1].playing_channel_index=1;
 sounds[0].type=_sound_start_track; sounds[0].next_definition_index=1; sounds[0].pitch=1;
 sounds[1].definition_index=1; sounds[1].start_time=2000;
 sounds[0].source.gain=1; channels[0].sound_index=0; channels[1].sound_index=1;
 channels[0].playing_permutation=&permutation; permutation.next_permutation_index=NONE; permutation.gain=1;
 range.permutations.address=&permutation; range.permutations.count=1; range.playback_rate=range.natural_pitch=1;
 test_definition.pitch_ranges.address=&range; test_definition.pitch_ranges.count=1; test_definition.gain_modifier=1;
 loop_definition.tracks.address=&track; loop_definition.tracks.count=1; track.gain=1;
 loop.tracks[0].primary_sound_index=0;
 sound_manager_globals.platform_definition=&platform; sound_manager_globals.channel_count=2;
 sound_manager_globals.render_time=1000;
 sound_class.maximum_number_per_object=1; sound_class.maximum_number_per_definition=4;
 queues=property_writes=updates=0;
}
int main(int argc,char **argv) {
 CHECK(argc==2,"case name"); const char *case_name=argv[1]; reset();
 CASE("no-channel") {
  sounds[0].playing_channel_index=NONE; CHECK(sound_set_definition_end(0),"channel-less voice survives");
  CHECK(sounds[0].identifier && !queues && !property_writes && !updates,"no hardware writes"); return 0;
 }
 CASE("other-victim") { sounds[1].start_time=0; }
 CASE("no-limit") { sounds[1].definition_index=3; }
 CASE("definition-limit") {
  sounds[1].source_identifier=1; sound_class.maximum_number_per_object=2;
  sound_class.maximum_number_per_definition=1;
 }
 update_channel_for_looping_sound(0,1.f);
 CASE("self-retirement") {
  CHECK(!sounds[0].identifier && channels[0].sound_index==NONE,"self retired");
  CHECK(!queues && !property_writes && !updates,"retired voice must never refill"); return 0;
 }
 CASE("definition-limit") {
  CHECK(!sounds[0].identifier && !queues && !property_writes && !updates,"test_definition cap retires voice"); return 0;
 }
 CASE("other-victim") {
  CHECK(sounds[0].identifier && !sounds[1].identifier,"only other voice retired");
  CHECK(queues==1 && property_writes==1 && updates==1,"survivor still plays"); return 0;
 }
 CASE("no-limit") {
  CHECK(sounds[0].identifier && sounds[1].identifier,"summary excludes current voice");
  CHECK(queues==1 && property_writes==1 && updates==1,"ordinary transition still plays"); return 0;
 }
 CHECK(FALSE,"unknown case");
}
