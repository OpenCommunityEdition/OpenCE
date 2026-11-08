/* A co-op client that joins during a looping sound that plays once (a
cutscene's) starts it where the host has reached (coop_sound_sync.c): the
host's time walked through the parts the client's sound manager is about to
play, to a permutation of a linked chain and a block's start into it. */
#include "harness.h"
#include <math.h>

typedef unsigned short word;
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(x,lo,hi) ((x)<(lo)?(lo):(x)>(hi)?(hi):(x))
#define VALID_INDEX(i,c) ((i)>=0&&(i)<(c))
#define SET_FLAG(f,b,v) ((f)=(v)?(f)|(1U<<(b)):(f)&~(1U<<(b)))
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
#define csmemset memset

#include "types.inc"

/* the client's sound tags: one looping sound, the scripts' instance of it 7,
its start and loop sounds */
enum {LOOPING=3,START_SOUND=10,LOOP_SOUND=11,SCRIPTS_INSTANCE=7,PERMUTATIONS=96};
static struct looping_sound_definition looping;
static struct looping_sound_track track;
static struct sound_definition sounds[2];
static struct sound_pitch_range ranges[2];
static struct sound_permutation permutations[2][PERMUTATIONS];
static struct looping_sound_definition *looping_sound_definition_get(long index)
{
 CHECK(index==LOOPING,"no such looping sound %ld",index);
 return &looping;
}
static struct sound_definition *sound_definition_get(long index)
{
 CHECK(index==START_SOUND||index==LOOP_SOUND,"no such sound %ld",index);
 return &sounds[index-START_SOUND];
}
static unsigned long sound_samples_per_second(short sample_rate)
{
 CHECK(sample_rate==0||sample_rate==1,"no such sample rate %d",sample_rate);
 return sample_rate?44100:22050;
}
static unsigned long now;
static unsigned long system_milliseconds(void){return now;}
#define error(...) ((void)0)
#define tag_get_name(...) "looping sound"

#include "under_test.inc"

enum {MONO=0,STEREO=1,NONE_COMPRESSION=0,XBOX_ADPCM=1,OGG=3,RATE_22=0,RATE_44=1};

/* a block's start, within two blocks of the ideal byte (a map's permutation is whole blocks, so a little
shorter than its nominal time) */
static boolean near(long bytes,real ideal,long block)
{
 return bytes%block==0&&fabsf((real)bytes-ideal)<=2.f*block;
}
/* the ideal byte milliseconds into a sound of channels at rate, an Xbox ADPCM one's or uncompressed */
static real ideal_adpcm(real milliseconds,long rate,long channels){return milliseconds*rate/1000.f/64.f*36.f*channels;}

/* a sound of permutations so long each (milliseconds at pitch 1), each linked to the next when linked */
static void sound(long index,short encoding,short compression,short sample_rate,boolean linked,int count,
 long const *milliseconds)
{
 struct sound_definition *definition=&sounds[index-START_SOUND];
 struct sound_pitch_range *range=&ranges[index-START_SOUND];
 long channels=encoding==STEREO?2:1;
 long rate=sample_rate?44100:22050;
 int permutation;

 memset(definition,0,sizeof(*definition));memset(range,0,sizeof(*range));
 definition->encoding=encoding;definition->compression=compression;definition->sample_rate=sample_rate;
 SET_FLAG(definition->flags,_sound_definition_linked_permutations_bit,linked);
 definition->pitch_ranges.count=1;definition->pitch_ranges.address=range;
 range->permutations.count=count;range->permutations.address=permutations[index-START_SOUND];
 range->actual_permutation_count=1;range->playback_rate=1.f;
 definition->random_pitch_bounds.lower=definition->random_pitch_bounds.upper=1.f;
 definition->zero_pitch_modifier=definition->one_pitch_modifier=1.f;
 for(permutation=0;permutation<count;permutation++)
 {
  struct sound_permutation *p=&permutations[index-START_SOUND][permutation];
  long frames=milliseconds[permutation]*rate/1000;

  memset(p,0,sizeof(*p));
  p->samples.size=compression==XBOX_ADPCM?frames/64*36*channels:frames*2*channels;
  p->next_permutation_index=linked&&permutation+1<count?(short)(permutation+1):NONE;
 }
}

static void setup(void)
{
 static long const ten_seconds[]={10000};

 memset(&looping,0,sizeof(looping));memset(&track,0,sizeof(track));
 looping.runtime_scripting_sound_index=SCRIPTS_INSTANCE;
 looping.tracks.count=1;looping.tracks.address=&track;
 sound(START_SOUND,MONO,XBOX_ADPCM,RATE_22,FALSE,1,ten_seconds);
 sound(LOOP_SOUND,MONO,XBOX_ADPCM,RATE_22,FALSE,1,ten_seconds);
 now=1000;
 coop_sound_sync_reset();
}

/* the sound manager about to play a part, its first permutation chosen as 0 at pitch */
static short place_at(long instance,short part,long sound_index,short *permutation,real pitch,long datum)
{
 *permutation=0;
 return coop_sound_sync_place(LOOPING,instance,0,part,sound_index,0,permutation,pitch,datum);
}
static short place(short part,long sound_index,short *permutation,long datum)
{
 return place_at(SCRIPTS_INSTANCE,part,sound_index,permutation,1.0f,datum);
}
static long offset(long datum,long sound_index,short permutation)
{
 return coop_sound_sync_take_offset(datum,&permutations[sound_index-START_SOUND][permutation]);
}

int main(int argc,char **argv)
{
 char const *name=argc>1?argv[1]:"";
 short permutation;
 short result;
 long bytes;

 setup();
 if(!strcmp(name,"pcm"))
 {
  /* uncompressed stereo at 44 kHz, 4 s into a 10 s loop: 4 bytes a sample */
  static long const ten_seconds[]={10000};
  sound(LOOP_SOUND,STEREO,NONE_COMPRESSION,RATE_44,FALSE,1,ten_seconds);
  coop_sound_sync_begin(LOOPING,4000);
  result=place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  bytes=offset(100,LOOP_SOUND,0);
  CHECK(result==_coop_sound_sync_play&&permutation==0,"result %d, permutation %d",result,permutation);
  CHECK(bytes==4000L*44100/1000*4,"4 s in is byte %ld",bytes);
 }
 else if(!strcmp(name,"chain"))
 {
  /* Xbox ADPCM, mono at 22 kHz, three linked 2 s permutations: 5 s is 1 s into the third,
  at the start of its block (36 bytes a 64 samples) */
  static long const two_seconds[]={2000,2000,2000};
  sound(LOOP_SOUND,MONO,XBOX_ADPCM,RATE_22,TRUE,3,two_seconds);
  coop_sound_sync_begin(LOOPING,5000);
  result=place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  bytes=offset(100,LOOP_SOUND,2);
  CHECK(result==_coop_sound_sync_play&&permutation==2,"result %d, landed on permutation %d",result,permutation);
  CHECK(near(bytes,ideal_adpcm(1000,22050,1),36),"1 s into it is byte %ld",bytes);
 }
 else if(!strcmp(name,"stereo-blocks"))
 {
  /* a stereo ADPCM block is twice a mono one's */
  static long const ten_seconds[]={10000};
  sound(LOOP_SOUND,STEREO,XBOX_ADPCM,RATE_44,FALSE,1,ten_seconds);
  coop_sound_sync_begin(LOOPING,3000);
  place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  bytes=offset(100,LOOP_SOUND,0);
  CHECK(near(bytes,ideal_adpcm(3000,44100,2),72),"3 s into stereo is byte %ld",bytes);
 }
 else if(!strcmp(name,"pitch"))
 {
  /* at twice the pitch, 2 s in is 4 s of samples */
  static long const ten_seconds[]={10000};
  sound(LOOP_SOUND,STEREO,NONE_COMPRESSION,RATE_44,FALSE,1,ten_seconds);
  coop_sound_sync_begin(LOOPING,2000);
  place_at(SCRIPTS_INSTANCE,_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,2.0f,100);
  bytes=offset(100,LOOP_SOUND,0);
  CHECK(bytes==4L*44100*4,"2 s in at pitch 2 is byte %ld",bytes);
 }
 else if(!strcmp(name,"start-skipped"))
 {
  /* 13 s after a 10 s start: the start does not play, and a 20 s loop starts 3 s in */
  static long const twenty_seconds[]={20000};
  sound(LOOP_SOUND,MONO,XBOX_ADPCM,RATE_22,FALSE,1,twenty_seconds);
  coop_sound_sync_begin(LOOPING,13000);
  result=place(_coop_sound_sync_start_part,START_SOUND,&permutation,100);
  CHECK(result==_coop_sound_sync_skip,"a start that has played out gave %d",result);
  CHECK(offset(100,START_SOUND,0)==0,"the skipped start kept an offset");
  result=place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,101);
  bytes=offset(101,LOOP_SOUND,0);
  CHECK(result==_coop_sound_sync_play&&near(bytes,ideal_adpcm(3000,22050,1),36),"the loop gave %d from byte %ld",
   result,bytes);
 }
 else if(!strcmp(name,"start-playing"))
 {
  /* 4 s into a 10 s start: the start plays from there (the loop follows it from its beginning) */
  coop_sound_sync_begin(LOOPING,4000);
  result=place(_coop_sound_sync_start_part,START_SOUND,&permutation,100);
  bytes=offset(100,START_SOUND,0);
  CHECK(result==_coop_sound_sync_play&&near(bytes,ideal_adpcm(4000,22050,1),36),"the start gave %d from byte %ld",
   result,bytes);
 }
 else if(!strcmp(name,"fade-in-at-start"))
 {
  /* a loop that fades in with its start plays alongside it: 13 s is 13 s into it, a 20 s one */
  static long const twenty_seconds[]={20000};
  SET_FLAG(track.flags,COOP_SOUND_TRACK_FADE_IN_AT_START_BIT,TRUE);
  sound(LOOP_SOUND,MONO,XBOX_ADPCM,RATE_22,FALSE,1,twenty_seconds);
  coop_sound_sync_begin(LOOPING,13000);
  place(_coop_sound_sync_start_part,START_SOUND,&permutation,100);
  place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,101);
  bytes=offset(101,LOOP_SOUND,0);
  CHECK(near(bytes,ideal_adpcm(13000,22050,1),36),"a loop alongside its start started from byte %ld",bytes);
 }
 else if(!strcmp(name,"finished"))
 {
  /* past the end of a linked loop: the sound has played out, and its stop does not play */
  static long const two_seconds[]={2000,2000,2000};
  sound(LOOP_SOUND,MONO,XBOX_ADPCM,RATE_22,TRUE,3,two_seconds);
  coop_sound_sync_begin(LOOPING,7000);
  result=place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  CHECK(result==_coop_sound_sync_finished,"a loop that has played out gave %d",result);
  coop_sound_sync_refreshed(LOOPING,SCRIPTS_INSTANCE,TRUE);
  result=place(_coop_sound_sync_stop_part,LOOP_SOUND,&permutation,101);
  CHECK(result==_coop_sound_sync_skip,"its stop gave %d",result);
 }
 else if(!strcmp(name,"repeat"))
 {
  /* a loop whose permutations are not linked repeats: 5.5 s round a 2 s one is 1.5 s in */
  static long const two_seconds[]={2000};
  sound(LOOP_SOUND,MONO,XBOX_ADPCM,RATE_22,FALSE,1,two_seconds);
  coop_sound_sync_begin(LOOPING,5500);
  result=place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  bytes=offset(100,LOOP_SOUND,0);
  CHECK(result==_coop_sound_sync_play,"a repeating loop gave %d",result);
  CHECK(near(bytes,ideal_adpcm(1500,22050,1),36),"5.5 s round it is byte %ld",bytes);
 }
 else if(!strcmp(name,"late"))
 {
  /* the time since the host's came counts too */
  coop_sound_sync_begin(LOOPING,3000);
  now+=1000;
  place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  bytes=offset(100,LOOP_SOUND,0);
  CHECK(near(bytes,ideal_adpcm(4000,22050,1),36),"3 s and 1 s waiting is byte %ld",bytes);
 }
 else if(!strcmp(name,"just-started"))
 {
  /* no time, or the scripts' own start: from the beginning */
  coop_sound_sync_begin(LOOPING,0);
  result=place(_coop_sound_sync_start_part,START_SOUND,&permutation,100);
  CHECK(result==_coop_sound_sync_play&&offset(100,START_SOUND,0)==0,"a start with no time was placed");
  coop_sound_sync_begin(LOOPING,NONE);
  result=place(_coop_sound_sync_start_part,START_SOUND,&permutation,100);
  CHECK(result==_coop_sound_sync_play&&offset(100,START_SOUND,0)==0,"a start with time NONE was placed");
 }
 else if(!strcmp(name,"ended"))
 {
  /* the host's stop, or a new game, forgets it */
  coop_sound_sync_begin(LOOPING,4000);
  coop_sound_sync_end(LOOPING);
  place(_coop_sound_sync_start_part,START_SOUND,&permutation,100);
  CHECK(offset(100,START_SOUND,0)==0,"a stopped sound was placed");
  coop_sound_sync_begin(LOOPING,4000);
  coop_sound_sync_reset();
  place(_coop_sound_sync_start_part,START_SOUND,&permutation,100);
  CHECK(offset(100,START_SOUND,0)==0,"a sound of the last game was placed");
 }
 else if(!strcmp(name,"placed"))
 {
  /* a refresh that started nothing (no listener heard it yet) leaves it to be placed */
  coop_sound_sync_begin(LOOPING,4000);
  coop_sound_sync_refreshed(LOOPING,SCRIPTS_INSTANCE,FALSE);
  result=place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  CHECK(result==_coop_sound_sync_play&&offset(100,LOOP_SOUND,0)>0,"a refresh that started nothing ended the placing");
  /* after the refresh that started it nothing more is placed (a pitch range's crossfade starts at the beginning) */
  coop_sound_sync_refreshed(LOOPING,SCRIPTS_INSTANCE,TRUE);
  result=place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,101);
  CHECK(result==_coop_sound_sync_play&&offset(101,LOOP_SOUND,0)==0,"a part after the first refresh was placed");
 }
 else if(!strcmp(name,"other-instance"))
 {
  /* another instance of the same looping sound (an object's) starts as it always has */
  coop_sound_sync_begin(LOOPING,4000);
  result=place_at(SCRIPTS_INSTANCE+1,_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,1.0f,100);
  CHECK(result==_coop_sound_sync_play&&offset(100,LOOP_SOUND,0)==0,"an instance not the scripts' was placed");
  coop_sound_sync_refreshed(LOOPING,SCRIPTS_INSTANCE+1,TRUE);
  place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,101);
  CHECK(offset(101,LOOP_SOUND,0)>0,"another instance's refresh ended the scripts' placing");
 }
 else if(!strcmp(name,"unknown-compression"))
 {
  /* a compression whose time is not known here starts at the beginning */
  static long const ten_seconds[]={10000};
  sound(LOOP_SOUND,MONO,OGG,RATE_22,FALSE,1,ten_seconds);
  coop_sound_sync_begin(LOOPING,4000);
  result=place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  CHECK(result==_coop_sound_sync_play&&offset(100,LOOP_SOUND,0)==0,"an Ogg sound was placed");
 }
 else if(!strcmp(name,"taken-once"))
 {
  /* the offset is the sound's first permutation's, taken once */
  coop_sound_sync_begin(LOOPING,4000);
  place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  CHECK(coop_sound_sync_take_offset(101,&permutations[1][0])==0,"another sound took the offset");
  CHECK(coop_sound_sync_take_offset(100,&permutations[0][0])==0,"another permutation took the offset");
  CHECK(coop_sound_sync_take_offset(100,&permutations[1][0])==0,"the offset outlived a wrong permutation");
  place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,102);
  CHECK(offset(102,LOOP_SOUND,0)>0&&offset(102,LOOP_SOUND,0)==0,"the offset was not taken exactly once");
 }
 else if(!strcmp(name,"hostile"))
 {
  /* what a host sends, and what a map holds, cannot run past a sound's samples or loop for ever */
  static long const two_seconds[]={2000,2000,2000};
  int pitch;
  real const pitches[]={0.0f,-1.0f,0.0001f,1e30f,(real)NAN,(real)INFINITY};

  coop_sound_sync_begin(LOOPING,0x7fffffffL);
  place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  bytes=offset(100,LOOP_SOUND,0);
  CHECK(bytes>=0&&bytes<permutations[1][0].samples.size&&bytes%36==0,"a huge time gave byte %ld",bytes);
  for(pitch=0;pitch<(int)(sizeof(pitches)/sizeof(pitches[0]));pitch++)
  {
   coop_sound_sync_begin(LOOPING,4000);
   result=place_at(SCRIPTS_INSTANCE,_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,pitches[pitch],100);
   bytes=offset(100,LOOP_SOUND,0);
   CHECK(result!=_coop_sound_sync_skip&&bytes>=0&&bytes<permutations[1][0].samples.size,
    "pitch %g gave %d from byte %ld",pitches[pitch],result,bytes);
  }
  /* a linked chain that comes back on itself */
  sound(LOOP_SOUND,MONO,XBOX_ADPCM,RATE_22,TRUE,3,two_seconds);
  permutations[1][2].next_permutation_index=0;
  coop_sound_sync_begin(LOOPING,1000000);
  result=place(_coop_sound_sync_loop_part,LOOP_SOUND,&permutation,100);
  CHECK(result==_coop_sound_sync_play,"a chain round on itself gave %d",result);
  /* a permutation past the range, and no such range */
  coop_sound_sync_begin(LOOPING,4000);
  permutation=PERMUTATIONS+5;
  result=coop_sound_sync_place(LOOPING,SCRIPTS_INSTANCE,0,_coop_sound_sync_loop_part,LOOP_SOUND,0,&permutation,1.f,100);
  CHECK(result==_coop_sound_sync_play&&permutation==PERMUTATIONS+5,"a bad permutation gave %d, %d",result,permutation);
  permutation=0;
  result=coop_sound_sync_place(LOOPING,SCRIPTS_INSTANCE,0,_coop_sound_sync_loop_part,LOOP_SOUND,NONE,&permutation,1.f,100);
  CHECK(result==_coop_sound_sync_play&&offset(100,LOOP_SOUND,0)==0,"no range was placed");
  result=coop_sound_sync_place(LOOPING,SCRIPTS_INSTANCE,9,_coop_sound_sync_loop_part,LOOP_SOUND,0,&permutation,1.f,100);
  CHECK(result==_coop_sound_sync_play&&offset(100,LOOP_SOUND,0)==0,"a track past the sound's was placed");
 }
 else if(!strcmp(name,"length"))
 {
  /* a cutscene's music: 86 linked permutations of 1.3 s and a last of 0.7 s, nearly two minutes (its
  tag's longest_permutation_length, 1485, is one permutation's) */
  static long parts[87];
  int part;
  long length;
  for(part=0;part<86;part++)parts[part]=1321;
  parts[86]=724;
  sound(LOOP_SOUND,STEREO,XBOX_ADPCM,RATE_44,TRUE,87,parts);
  sounds[1].longest_permutation_length=1485;
  length=coop_sound_sync_sound_milliseconds(LOOP_SOUND);
  CHECK(length>=113000&&length<=114400,"the music's time was %ld ms",length);
  /* a loop not linked plays its longest permutation, picked at random */
  {
   static long const three[]={2000,5000,3000};
   sound(LOOP_SOUND,MONO,XBOX_ADPCM,RATE_22,FALSE,3,three);
   ranges[1].actual_permutation_count=3;
   length=coop_sound_sync_sound_milliseconds(LOOP_SOUND);
   CHECK(length>=4900&&length<=5000,"the longest of three not linked was %ld ms",length);
  }
  /* at its slowest pitch: the random pitch's lower bound, half, makes it twice as long */
  sounds[1].random_pitch_bounds.lower=0.5f;
  length=coop_sound_sync_sound_milliseconds(LOOP_SOUND);
  CHECK(length>=9800&&length<=10000,"at half pitch it was %ld ms",length);
  /* a compression whose time is not known here: the tag's longest permutation, for each linked one */
  {
   static long const two_seconds[]={2000,2000,2000};
   sound(LOOP_SOUND,MONO,OGG,RATE_22,TRUE,3,two_seconds);
   sounds[1].longest_permutation_length=2500;
   length=coop_sound_sync_sound_milliseconds(LOOP_SOUND);
   CHECK(length==7500,"three Ogg permutations were %ld ms",length);
  }
 }
 else if(!strcmp(name,"length-hostile"))
 {
  /* a chain round on itself, a map's huge length and no sound end, an hour at most */
  static long const two_seconds[]={2000,2000,2000};
  long length;
  sound(LOOP_SOUND,MONO,XBOX_ADPCM,RATE_22,TRUE,3,two_seconds);
  permutations[1][2].next_permutation_index=0;
  length=coop_sound_sync_sound_milliseconds(LOOP_SOUND);
  CHECK(length>0&&length<=7000,"a chain round on itself was %ld ms",length);
  sound(LOOP_SOUND,MONO,OGG,RATE_22,TRUE,3,two_seconds);
  sounds[1].longest_permutation_length=0x7fffffffL;
  length=coop_sound_sync_sound_milliseconds(LOOP_SOUND);
  CHECK(length==60*60*1000,"a huge length was %ld ms",length);
  sounds[1].random_pitch_bounds.lower=-3.f;sounds[1].zero_pitch_modifier=0.f;ranges[1].playback_rate=(real)NAN;
  sound(LOOP_SOUND,MONO,XBOX_ADPCM,RATE_22,TRUE,3,two_seconds);
  sounds[1].random_pitch_bounds.lower=-3.f;sounds[1].zero_pitch_modifier=0.f;ranges[1].playback_rate=(real)NAN;
  length=coop_sound_sync_sound_milliseconds(LOOP_SOUND);
  CHECK(length>0&&length<=60*60*1000,"pitches a map breaks gave %ld ms",length);
  CHECK(coop_sound_sync_sound_milliseconds(NONE)==0,"no sound has a time");
 }
 else
 {
  fprintf(stderr,"unknown case %s\n",name);
  return 2;
 }
 return 0;
}
