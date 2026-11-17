/* The real sound_find_best_channel, sound_preempts_sound and
sound_valid_for_channel, with the game's own sound class table: which sound
loses its channel when every channel of a type plays (#1125). A sound's
distance squared to its listener is its source's scale here. */
#include "harness.h"
#include "types.inc"
#define MAXIMUM_SOUNDS 16
static struct sound_manager_globals sound_manager_globals;
static struct sound_channel_datum channels[MAXIMUM_SOUNDS];
static struct sound_datum sounds[MAXIMUM_SOUNDS];
static struct sound_definition definitions[MAXIMUM_SOUNDS];
static long sound_count;
#define sound_get(i) (&sounds[i])
#define channel_get(i) (&channels[i])
#define sound_definition_get(i) (&definitions[i])
#define sound_class_get(i) (&sound_classes[i])
#define source_distance_squared(listener, source) ((source)->scale)
#include "classes.inc"
#include "under_test.inc"
/* the type flags of the game's stereo 44 kHz and mono 3D channels */
#define STEREO_44K (FLAG(_sound_channel_compressed_bit)|FLAG(_sound_channel_stereo_bit)|FLAG(_sound_channel_44k_bit))
#define MONO_3D (FLAG(_sound_channel_compressed_bit)|FLAG(_sound_channel_3d_bit))
static long sound_new(short class_index, short type_flags, real distance_squared) {
 long index=sound_count++;
 definitions[index].sound_class=class_index; definitions[index].compression=1;
 definitions[index].encoding=TEST_FLAG(type_flags,_sound_channel_stereo_bit);
 definitions[index].sample_rate=TEST_FLAG(type_flags,_sound_channel_44k_bit);
 sounds[index].definition_index=index; sounds[index].listener_index=0;
 sounds[index].source.spatialization_mode=TEST_FLAG(type_flags,_sound_channel_3d_bit) ?
  _sound_spatialization_mode_absolute : _sound_spatialization_mode_none;
 sounds[index].source.scale=distance_squared;
 return index;
}
/* every channel (count of them, of one type) plays a sound: the first of
first_class, the rest of other_class, each farther than the one before */
static void fill(short count, short type_flags, short first_class, short other_class) {
 short i;
 sound_manager_globals.channel_count=count;
 for(i=0;i<count;i++) {
  channels[i].type_flags=type_flags;
  channels[i].sound_index=sound_new(i ? other_class : first_class, type_flags, 10.f+i);
 }
}
static short class_named(const char *name) {
 short i;
 for(i=0;i<NUMBER_OF_SOUND_CLASSES;i++) if(!strcmp(sound_class_names[i],name)) return i;
 CHECK(FALSE,"no class %s",name);
 return NONE;
}
int main(int argc,char **argv) {
 const char *case_name;
 short weapon_fire, vehicle_engine, channel;
 CHECK(argc==2,"case name"); case_name=argv[1];
 weapon_fire=class_named("weapon_fire"); vehicle_engine=class_named("vehicle_engine");
 /* the issue's: nine stereo 44 kHz channels, music in one and a mod's
 stereo weapons in the rest; a 10th weapon sound, the nearest, takes a
 weapon's channel, never the music's */
 CASE("music-kept") {
  fill(9,STEREO_44K,_sound_class_music,weapon_fire);
  channel=sound_find_best_channel(sound_new(weapon_fire,STEREO_44K,1.f));
  CHECK(channel!=NONE && channel!=0,"took channel %d (the music's is 0)",channel);
  CHECK(channel==8,"took channel %d, not the farthest weapon's (8)",channel);
  return 0;
 }
 /* the same for a unit's line and an engine in mono 3D channels */
 CASE("speech-kept") {
  fill(4,MONO_3D,_sound_class_unit_dialog,vehicle_engine);
  channel=sound_find_best_channel(sound_new(vehicle_engine,MONO_3D,1.f));
  CHECK(channel!=NONE && channel!=0,"took channel %d (the line's is 0)",channel);
  return 0;
 }
 /* music finds a channel when effects fill its type */
 CASE("music-takes-effects-channel") {
  fill(9,STEREO_44K,weapon_fire,weapon_fire);
  channel=sound_find_best_channel(sound_new(_sound_class_music,STEREO_44K,100.f));
  CHECK(channel!=NONE,"music found no channel");
  return 0;
 }
 /* between effects, the priorities decide as before: an engine (3) takes
 no weapon's (4) channel, however near */
 CASE("effects-as-before") {
  fill(9,STEREO_44K,weapon_fire,weapon_fire);
  channel=sound_find_best_channel(sound_new(vehicle_engine,STEREO_44K,0.f));
  CHECK(channel==NONE,"an engine took weapon channel %d",channel);
  return 0;
 }
 CHECK(FALSE,"unknown case");
 return 1;
}
