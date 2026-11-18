/* The real sound_find_best_channel (and what it calls) over the native
builds' channel pools (halo_port_capacity.h, made as sound_initialize makes
them): a heavy fight, stock and with a sound mod's stereo 44 kHz effects,
played into them, counting the channels taken from playing sounds. A heavy
fight: 16 players' and their vehicles' effects (each definition plays at most
4 at once), the music, 4 lines and 8 sounds of the HUD. */
#include "harness.h"
#include "types.inc"
#define MAXIMUM_SOUNDS 512
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
#include "pools.inc"
#include "under_test.inc"
#define HEAVY_FIGHT_EFFECTS 64
/* the type flags of the game's channel types */
#define MONO_2D FLAG(_sound_channel_compressed_bit)
#define MONO_3D (FLAG(_sound_channel_compressed_bit)|FLAG(_sound_channel_3d_bit))
#define STEREO_44K (FLAG(_sound_channel_compressed_bit)|FLAG(_sound_channel_stereo_bit)|FLAG(_sound_channel_44k_bit))
static short taken, music_or_speech_taken;
static void pools(short const *counts) {
 short type, index;
 sound_manager_globals.channel_count=0;
 for(type=0;type<4;type++)
  for(index=0;index<counts[type];index++) {
   CHECK(sound_manager_globals.channel_count<MAXIMUM_SOUNDS,"too many channels");
   channels[sound_manager_globals.channel_count].sound_index=NONE;
   channels[sound_manager_globals.channel_count++].type_flags=sound_channel_type_flags[type];
  }
}
static short class_named(const char *name) {
 short i;
 for(i=0;i<NUMBER_OF_SOUND_CLASSES;i++) if(!strcmp(sound_class_names[i],name)) return i;
 CHECK(FALSE,"no class %s",name);
 return NONE;
}
/* a sound played: the channel it finds, or none; a channel taken from a
playing sound counted */
static void play(short class_index, short type_flags, real distance_squared) {
 long index=sound_count++;
 short channel;
 CHECK(index<MAXIMUM_SOUNDS,"too many sounds");
 definitions[index].sound_class=class_index; definitions[index].compression=1;
 definitions[index].encoding=TEST_FLAG(type_flags,_sound_channel_stereo_bit);
 definitions[index].sample_rate=TEST_FLAG(type_flags,_sound_channel_44k_bit);
 sounds[index].definition_index=index; sounds[index].listener_index=0;
 sounds[index].source.spatialization_mode=TEST_FLAG(type_flags,_sound_channel_3d_bit) ?
  _sound_spatialization_mode_absolute : _sound_spatialization_mode_none;
 sounds[index].source.scale=distance_squared;
 channel=sound_find_best_channel(index);
 if(channel==NONE) return;
 if(channels[channel].sound_index!=NONE) {
  short victim=definitions[channels[channel].sound_index].sound_class;
  taken++;
  music_or_speech_taken+=victim==_sound_class_music || sound_classes[victim].speech;
 }
 channels[channel].sound_index=index;
}
/* the fight: its effects mono 3D (stock) or stereo 44 kHz (modded) */
static void fight_of(short const *counts, boolean modded, short effects) {
 short i, effect_flags=modded ? STEREO_44K : MONO_3D;
 short classes[]={ class_named("weapon_fire"), class_named("projectile_impact"), class_named("vehicle_engine"),
  class_named("weapon_reload") };
 sound_count=0; taken=music_or_speech_taken=0;
 pools(counts);
 play(_sound_class_music,STEREO_44K,0.f);
 for(i=0;i<4;i++) play(_sound_class_scripted_dialog_to_other,MONO_3D,5.f+i);
 for(i=0;i<8;i++) play(class_named("first_person_damage"),MONO_2D,0.f);
 for(i=0;i<effects;i++) play(classes[i%4],effect_flags,1.f+i);
}
#define fight(counts, modded) fight_of(counts, modded, HEAVY_FIGHT_EFFECTS)
/* the sounds playing (each in a channel) */
static short playing(void) {
 short i, count=0;
 for(i=0;i<sound_manager_globals.channel_count;i++) count+=channels[i].sound_index!=NONE;
 return count;
}
int main(int argc,char **argv) {
 const char *case_name;
 static short const desktop[]=DESKTOP_VIRTUAL_CHANNELS, phone[]=PHONE_VIRTUAL_CHANNELS;
 CHECK(argc==2,"case name"); case_name=argv[1];
 /* the desktop's pools hold a heavy fight, stock or modded: no sound
 takes another's channel */
 CASE("desktop") {
  fight(desktop,FALSE); CHECK(!taken,"stock: %d channels taken",taken);
  fight(desktop,TRUE); CHECK(!taken,"modded: %d channels taken",taken);
  return 0;
 }
 /* the phones' hold a stock one; a modded one takes effects' channels past
 their stereo 44 kHz pool, never the music's or a line's */
 CASE("phone") {
  fight(phone,FALSE); CHECK(!taken,"stock: %d channels taken",taken);
  fight(phone,TRUE); CHECK(!music_or_speech_taken,"modded: music or speech lost its channel");
  return 0;
 }
 /* a normal fight (24 effects), which the Xbox's pools held: the same
 sounds play in the larger pools, none more, so the mix and its limiter
 (dsound_sdl.c) are as loud as before */
 CASE("normal-fight-unchanged") {
  static short const xbox[]={ 9, 46, 9, 9 };
  short before;
  fight_of(xbox,FALSE,24); before=playing(); CHECK(!taken,"the Xbox's pools took %d",taken);
  fight_of(desktop,FALSE,24); CHECK(!taken && playing()==before,"%d playing, %d before",playing(),before);
  fight_of(phone,FALSE,24); CHECK(!taken && playing()==before,"phone: %d playing, %d before",playing(),before);
  return 0;
 }
 CHECK(FALSE,"unknown case");
 return 1;
}
