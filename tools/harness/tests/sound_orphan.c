/* The real refresh_sound and track_loop_track_sound, in frames ordered as
sound_render orders them: a looping sound not refreshed since the last frame
is deleted (process_looping_sounds), each sound is refreshed and stopped when
refresh_sound says so (refresh_sounds), and the sounds left are updated
(update_channels), where a track reads its looping sound. */
#include "harness.h"
#include "types.inc"
static struct sound_manager_globals sound_manager_globals;
static struct data_array *sound_data, *looping_sound_data;
static struct sound_definition test_definition;
static struct sound_class_definition sound_class;
static int tracked;
static void *datum_try_and_get(struct data_array *data, long index) {
 struct datum_header *header;
 if (index==NONE || DATUM_INDEX_TO_ABSOLUTE_INDEX(index)>=data->maximum_count) return NULL;
 header=datum_get(data,index);
 return header->identifier && header->identifier==(short)(index>>16) ? header : NULL;
}
static struct sound_datum *checked_sound(long index) {
 struct sound_datum *sound=datum_try_and_get(sound_data,index);
 CHECK(sound,"access to a stopped sound");
 return sound;
}
#define sound_get(i) checked_sound(i)
#define channel_get(...) ((struct sound_channel_datum *)NULL)
#define sound_definition_get(...) (&test_definition)
#define sound_class_get(...) (&sound_class)
static boolean counted_track_loop_track_sound(long i,void const *d,struct sound_source *s);
#include "under_test.inc"
static boolean counted_track_loop_track_sound(long i,void const *d,struct sound_source *s) {
 tracked++; return track_loop_track_sound(i,d,s);
}
static long loop_new(void) {
 long index=datum_new(looping_sound_data);
 ((struct looping_sound_datum *)datum_get(looping_sound_data,index))->flip_flop=sound_manager_globals.flip_flop;
 return index;
}
/* a looping sound's track, as update_potentially_audible_looping_sound makes one */
static long track_new(long loop_index) {
 long index=datum_new(sound_data);
 struct sound_datum *sound=sound_get(index);
 sound->type=_sound_loop_track; sound->source_identifier=loop_index; sound->playing_channel_index=NONE;
 sound->track_proc=counted_track_loop_track_sound; sound->start_time=sound_manager_globals.render_time;
 return index;
}
static long impulse_new(long start_time) {
 long index=datum_new(sound_data);
 struct sound_datum *sound=sound_get(index);
 sound->type=_sound_impulse; sound->source_identifier=NONE; sound->playing_channel_index=NONE;
 sound->track_proc=counted_track_loop_track_sound; sound->start_time=start_time;
 return index;
}
static long next_index(struct data_array *data,long previous) {
 long i;
 for(i=previous==NONE?0:DATUM_INDEX_TO_ABSOLUTE_INDEX(previous)+1;i<data->maximum_count;i++) {
  struct datum_header *header=(void *)(data->data+i*data->size);
  if(header->identifier) return ((long)(unsigned short)header->identifier<<16)|i;
 }
 return NONE;
}
/* sound_render at a time (milliseconds) */
static void render(long time) {
 long i;
 sound_manager_globals.render_time=time;
 for(i=next_index(looping_sound_data,NONE);i!=NONE;i=next_index(looping_sound_data,i))
  if(((struct looping_sound_datum *)datum_get(looping_sound_data,i))->flip_flop!=sound_manager_globals.flip_flop)
   datum_delete(looping_sound_data,i);
 for(i=next_index(sound_data,NONE);i!=NONE;i=next_index(sound_data,i))
  if(!refresh_sound(i)) datum_delete(sound_data,i);
 for(i=next_index(sound_data,NONE);i!=NONE;i=next_index(sound_data,i)) {
  struct sound_datum *sound=sound_get(i);
  if(sound->type!=_sound_impulse)
   CHECK(datum_try_and_get(looping_sound_data,sound->source_identifier),"a track updated after its looping sound was deleted");
 }
 sound_manager_globals.flip_flop=!sound_manager_globals.flip_flop;
}
static void refresh_loop(long loop_index) {
 ((struct looping_sound_datum *)datum_get(looping_sound_data,loop_index))->flip_flop=sound_manager_globals.flip_flop;
}
int main(int argc,char **argv) {
 const char *case_name;
 long loop_index,track_index;
 CHECK(argc==2,"case name"); case_name=argv[1];
 sound_data=game_state_data_new("sounds",8,sizeof(struct sound_datum));
 looping_sound_data=game_state_data_new("looping sounds",4,sizeof(struct looping_sound_datum));
 render(1000);
 /* a frame refreshes the looping sound and starts its track; the next does
 not refresh it (its object went out of hearing); every frame in the same
 millisecond (frame period 0) or each a millisecond on */
 CASE("same-millisecond") {
  loop_index=loop_new(); track_index=track_new(loop_index);
  render(1000); render(1000);
  CHECK(!datum_try_and_get(looping_sound_data,loop_index),"looping sound deleted");
  CHECK(!datum_try_and_get(sound_data,track_index),"its track stopped");
  return 0;
 }
 CASE("next-millisecond") {
  loop_index=loop_new(); track_index=track_new(loop_index);
  render(1001); render(1002);
  CHECK(!datum_try_and_get(sound_data,track_index),"its track stopped");
  return 0;
 }
 /* a looping sound refreshed every frame keeps its track, in the same
 millisecond or later */
 CASE("refreshed") {
  int frame;
  loop_index=loop_new(); track_index=track_new(loop_index);
  for(frame=0;frame<6;frame++) { render(1000+frame/3); refresh_loop(loop_index); }
  CHECK(datum_try_and_get(sound_data,track_index),"the track plays on");
  return 0;
 }
 /* an impulse sound that starts later (a distant sound's delay) is not
 tracked until it starts, as before */
 CASE("delayed-impulse") {
  long impulse=impulse_new(1050);
  tracked=0; render(1001); render(1002);
  CHECK(datum_try_and_get(sound_data,impulse) && tracked==0,"waits for its start");
  return 0;
 }
 CHECK(FALSE,"unknown case");
 return 1;
}
