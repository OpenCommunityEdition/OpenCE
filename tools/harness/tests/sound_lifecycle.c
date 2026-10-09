#include "harness.h"
#include <math.h>
#include "types.inc"
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))
#define SET_FLAG(v,b,s) ((v)=(s)?((v)|FLAG(b)):((v)&~FLAG(b)))
#define match_vassert(...) ((void)0)
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
static struct sound_manager_globals sound_manager_globals;
static struct data_array *sound_data, *looping_sound_data;
static struct looping_sound_datum loops[2];
static struct looping_sound_track tracks[4];
static struct looping_sound_definition test_loop_definition;
static struct sound_definition test_definition;
static struct sound_pitch_range range;
static struct sound_permutation permutation;
static boolean loud_dialog_hack;
static struct sound_datum *checked_sound(long index) {
 struct sound_datum *sound=datum_get(sound_data,index);
 CHECK(index!=NONE && DATUM_INDEX_TO_ABSOLUTE_INDEX(index)<sound_data->maximum_count && sound->identifier,"live voice");
 return sound;
}
#define sound_get(i) checked_sound(i)
#define looping_sound_get(i) (&loops[i])
#define looping_sound_definition_get(...) (&test_loop_definition)
#define sound_definition_get(...) (&test_definition)
#define sound_permutation_get(...) (&permutation)
#define sound_definition_is_playable(...) TRUE
#define sound_is_active() TRUE
#define error(...) abort()
#define sound_definition_get_maximum_distance(...) 100.f
#define source_audible(...) 0
#define real_seed_random_range(...) 1.f
#define sound_definition_find_pitch_range_by_pitch(...) 0
#define sound_definition_next_permutation(...) 0
#define sound_scale_value(a,...) (a)
#define sound_scale_random_value(...) 1.f
#define real_seed_random(...) 1.f
#define game_time_get() 0
#define sound_definition_promote(...) _sound_promotion_dont
#define sound_travel_milliseconds(...) 0
#define source_distance(...) 0.f
#define valid_real_normal3d(...) TRUE
#define render_debug_looping_sound(...) ((void)0)
#define player_effect_continuous_refresh(...) ((void)0)
#define csmemcpy memcpy
static int _sound_cache_sound_request(struct sound_permutation *p,int a,int b,int c) {
 (void)p;(void)a;(void)b;(void)c;return TRUE;
}
static boolean track_loop_track_sound(long i,void const *d,struct sound_source *s) {
 (void)i;(void)d;(void)s;return TRUE;
}
static long data_next_index(struct data_array *data,long previous) {
 for(long i=previous==NONE?0:DATUM_INDEX_TO_ABSOLUTE_INDEX(previous)+1;i<data->maximum_count;i++) {
  struct datum_header *header=(void *)(data->data+i*data->size);
  if(header->identifier) return ((long)(unsigned short)header->identifier<<16)|i;
 }
 return NONE;
}
static long looping_sound_find(long identifier) { (void)identifier;return loops[0].identifier?0:NONE; }
static long looping_sound_new(long definition_index,long identifier,struct sound_source const *source) {
 (void)identifier; memset(&loops[0],0,sizeof(loops[0]));loops[0].identifier=1;
 loops[0].definition_index=definition_index;loops[0].source=*source;return 0;
}
static void sound_set_definition_begin(long i,long d) { sound_get(i)->next_definition_index=d; }
#include "under_test.inc"
static void reset(void) {
 sound_data=game_state_data_new("sounds",64,sizeof(struct sound_datum));
 looping_sound_data=game_state_data_new("loops",2,sizeof(struct looping_sound_datum));
 memset(loops,0,sizeof(loops));memset(tracks,0,sizeof(tracks));memset(&test_loop_definition,0,sizeof(test_loop_definition));
 test_loop_definition.tracks.count=1;test_loop_definition.tracks.address=tracks;
 test_loop_definition.continuous_damage_effect.index=NONE;
 for(int i=0;i<4;i++) {
  tracks[i].start_sound.index=1;tracks[i].loop_sound.index=2;tracks[i].stop_sound.index=NONE;
  tracks[i].alternate_loop_sound.index=tracks[i].alternate_stop_sound.index=NONE;
  tracks[i].flags=FLAG(_fade_out_at_stop_bit);tracks[i].fade_out_duration=.1f;
 }
 sound_manager_globals.render_time=1000;
 sound_manager_globals.initialized=sound_manager_globals.enabled=TRUE;
 test_definition.compression=_sound_compression_xbox_adpcm;test_definition.encoding=_sound_encoding_mono;
}
static void advance(int hz,int milliseconds) {
 long start=sound_manager_globals.render_time;
 for(int frame=1;frame<=hz*milliseconds/1000+1;frame++) {
  sound_manager_globals.render_time=start+frame*1000/hz;
  for(long i=data_next_index(sound_data,NONE);i!=NONE;i=data_next_index(sound_data,i)) {
   struct sound_datum *sound=sound_get(i);
   if(sound_calculate_fade(i)==0.f) {
    loops[sound->source_identifier].component_sound_count--;
    if(loops[sound->source_identifier].tracks[sound->loop_track_index].primary_sound_index==i)
     loops[sound->source_identifier].tracks[sound->loop_track_index].primary_sound_index=NONE;
    datum_delete(sound_data,i);
   }
  }
 }
}
static void refresh(int state) { struct sound_source source={.scale=1.f,.gain=1.f};sound_refresh_looping(0,123,&source,state,FALSE,0.f); }
static int live(long i) { return ((struct sound_datum *)datum_get(sound_data,i))->identifier!=0; }
int main(int argc,char **argv) {
 CHECK(argc==2,"case name");char *split=strchr(argv[1],':');CHECK(split,"frame rate");*split='\0';
 const char *case_name=argv[1];int hz=atoi(split+1);reset();
 CASE("fresh-impulse") {
  struct sound_source source={.scale=1.f,.gain=1.f};long i=sound_new_impulse(0,&source,0,NULL,NULL,0);
  CHECK(i!=NONE && sound_calculate_fade(i)==1.f,"new impulse audible");return 0;
 }
 if(!strcmp(case_name,"four-tracks")) test_loop_definition.tracks.count=4;
 refresh(_looping_sound_refresh_start);long primary=loops[0].tracks[0].primary_sound_index,secondary=NONE;
 CASE("fresh-loop") { CHECK(sound_calculate_fade(primary)==1.f,"new loop audible");return 0; }
 CASE("pending") { secondary=update_potentially_audible_looping_sound(2,0,0,_sound_loop_track); }
 CASE("restart") { refresh(_looping_sound_refresh_start); }
 CASE("intro-loop") {
  tracks[0].flags|=FLAG(_fade_in_at_start_bit);tracks[0].fade_in_duration=.2f;
  refresh(_looping_sound_refresh_start);
 }
 CASE("rapid") {
  advance(hz,34);refresh(_looping_sound_refresh_stop);advance(hz,34);
  refresh(_looping_sound_refresh_start);advance(hz,34);
 }
 CASE("other-owner") { loops[1].source=loops[0].source;secondary=update_potentially_audible_looping_sound(2,1,0,_sound_loop_track); }
 CASE("other-track") { secondary=update_potentially_audible_looping_sound(2,0,1,_sound_loop_track); }
 CASE("stop-cue") { tracks[0].stop_sound.index=3; }
 CASE("fake-impulse") { test_loop_definition.flags=FLAG(_looping_sound_fake_impulse_sound_bit);tracks[0].flags=0; }
 refresh(_looping_sound_refresh_stop);
 CASE("repeated-stop") { for(int frame=0;frame<5;frame++) { advance(hz,20);refresh(_looping_sound_refresh_stop); } }
 advance(hz,300);
 CASE("other-owner") { CHECK(!live(primary) && live(secondary),"other owner survives");return 0; }
 CASE("other-track") { CHECK(!live(primary) && live(secondary),"other track survives");return 0; }
 CASE("stop-cue") { CHECK(!live(primary) && loops[0].component_sound_count==1,"authored stop cue survives");return 0; }
 CASE("fake-impulse") { CHECK(live(primary),"ordered impulse preserves authored ending");return 0; }
 CHECK(loops[0].component_sound_count==0,"all owned start/loop voices retire within their fade duration");return 0;
}
