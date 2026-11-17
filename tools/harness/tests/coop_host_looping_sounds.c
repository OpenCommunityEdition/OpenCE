/* A co-op host's script looping sounds (network_coop.c's
host_send_looping_sounds and network_coop_note_sound, with game_sound.c's
scripted looping sounds). A host that draws nothing (a dedicated server)
never renders its sounds, so one that plays once (fake impulse: a
cutscene's) never ends there by itself: it is ended once it has had the time
to play, and is not resent near that end, so that no client starts it again. */
#include "harness.h"

typedef unsigned short word;
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(x,lo,hi) ((x)<(lo)?(lo):(x)>(hi)?(hi):(x))
#define SET_FLAG(f,b,v) ((f)=(v)?(f)|(1U<<(b)):(f)&~(1U<<(b)))
#define TICKS_PER_SECOND 30
#define SOUND_DEFINITION_TAG 0x736e6421UL
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&((type *)(block)->address)[(index)])

struct tag_block_definition;
#include "types.inc"
void network_coop_note_sound(short kind,long definition_index,long object_index,real scale);
void scripted_looping_sound_stop(long sound_index);

/* the map's sounds: how long each plays in milliseconds, its linked permutations together
(coop_sound_sync.c's coop_sound_sync_sound_milliseconds, test_coop_sound_sync.py) */
enum {SHORT_SOUND=0,LONG_SOUND=1,NO_LENGTH=2,HUGE_SOUND=3,SOUNDS=4};
struct sound_definition {long milliseconds;};
static struct sound_definition sounds[SOUNDS];
static long coop_sound_sync_sound_milliseconds(long index)
{
 CHECK(index>=0&&index<SOUNDS,"no such sound %ld",index);
 return sounds[index].milliseconds;
}

/* the map's looping sounds: 0 plays once (a cutscene's: a 10 s start and a
60 s loop), 1 is music that loops, 2 plays once with sounds of no length,
3 plays once with a sound longer than any could be */
enum {ONCE=0,MUSIC=1,ONCE_NO_LENGTH=2,ONCE_HUGE=3,DEFINITIONS=4};
static struct looping_sound_definition definitions[DEFINITIONS];
static struct looping_sound_track tracks[DEFINITIONS];
static struct looping_sound_definition *looping_sound_definition_get(long index)
{
 CHECK(index>=0&&index<DEFINITIONS,"no such looping sound %ld",index);
 return &definitions[index];
}

/* the playing looping sounds (game_sound.c's data) */
struct game_looping_sound_datum {long definition_index;unsigned long flags;real scale;boolean used;};
enum {_game_looping_sound_unattached_bit,_game_looping_sound_scripted_bit,_game_looping_sound_alternate_bit,
 _game_looping_sound_unattached_stop_bit,_game_looping_sound_unattached_stop_fixed_fadeout_bit};
static struct game_looping_sound_datum playing[8];
static long game_looping_sound_new(long object_index,long definition_index,char const *marker,long node)
{
 long index;
 (void)object_index;(void)marker;(void)node;
 for(index=0;index<8;index++)
 {
  if(!playing[index].used)
  {
   memset(&playing[index],0,sizeof(playing[index]));
   playing[index].used=TRUE;playing[index].definition_index=definition_index;
   return index;
  }
 }
 return NONE;
}
static struct game_looping_sound_datum *game_looping_sound_get(long index)
{
 CHECK(index>=0&&index<8&&playing[index].used,"no such playing sound %ld",index);
 return &playing[index];
}
static void scripted_music_stop_all(void){}

/* the host's world: its clock, whether it draws, its resends and the events it sends */
static long now;
static long game_time_get(void){return now;}
boolean debug_no_drawing;
static struct {boolean refresh;} host_resend;
static struct distributed_coop_event sent[1024];
static int sent_count,queue_full;
static struct distributed_coop_event *event_new(byte kind)
{
 struct distributed_coop_event *event;
 if(queue_full||sent_count==1024)return NULL;
 event=&sent[sent_count++];
 memset(event,0,sizeof(*event));
 event->kind=kind;event->object_index=NONE;event->tag_index=NONE;event->target=NONE;
 return event;
}

#include "under_test.inc"

static void setup(void)
{
 int index;
 memset(sounds,0,sizeof(sounds));memset(definitions,0,sizeof(definitions));memset(tracks,0,sizeof(tracks));
 memset(playing,0,sizeof(playing));
 sounds[SHORT_SOUND].milliseconds=10000;
 sounds[LONG_SOUND].milliseconds=60000;
 sounds[HUGE_SOUND].milliseconds=0x7fffffffL;
 for(index=0;index<DEFINITIONS;index++)
 {
  definitions[index].runtime_scripting_sound_index=NONE;
  definitions[index].tracks.count=1;definitions[index].tracks.address=&tracks[index];
  tracks[index].start_sound.index=NONE;tracks[index].loop_sound.index=NONE;tracks[index].stop_sound.index=NONE;
  tracks[index].alternate_loop_sound.index=NONE;tracks[index].alternate_stop_sound.index=NONE;
 }
 tracks[ONCE].start_sound.index=SHORT_SOUND;tracks[ONCE].loop_sound.index=LONG_SOUND;
 tracks[MUSIC].loop_sound.index=LONG_SOUND;
 tracks[ONCE_NO_LENGTH].loop_sound.index=NO_LENGTH;
 tracks[ONCE_HUGE].loop_sound.index=HUGE_SOUND;
 SET_FLAG(definitions[ONCE].flags,_looping_sound_fake_impulse_sound_bit,TRUE);
 SET_FLAG(definitions[ONCE_NO_LENGTH].flags,_looping_sound_fake_impulse_sound_bit,TRUE);
 SET_FLAG(definitions[ONCE_HUGE].flags,_looping_sound_fake_impulse_sound_bit,TRUE);
 now=1000;debug_no_drawing=TRUE;host_resend.refresh=FALSE;sent_count=0;queue_full=FALSE;
 host_looping_sound_count=0;
}

static boolean is_playing(long definition_index){return definitions[definition_index].runtime_scripting_sound_index!=NONE;}

/* the events of a type for a sound sent since index from */
static int sent_of(int from,int type,long definition_index)
{
 int index,count=0;
 for(index=from;index<sent_count;index++)
  if(sent[index].type==type&&sent[index].tag_index==definition_index)count++;
 return count;
}

/* the last event of a type for a sound, or NULL */
static struct distributed_coop_event const *last_sent(int type,long definition_index)
{
 int index;
 for(index=sent_count-1;index>=0;index--)
  if(sent[index].type==type&&sent[index].tag_index==definition_index)return &sent[index];
 return NULL;
}

/* the host's ticks from now to the tick at (now is then the tick after),
a resend every two seconds as host_resend_update makes them */
static void run_to(long at)
{
 for(;now<=at;now++)
 {
  host_resend.refresh=now%(2*TICKS_PER_SECOND)==0;
  host_send_looping_sounds();
 }
 host_resend.refresh=FALSE;
}

int main(int argc,char **argv)
{
 char const *name=argc>1?argv[1]:"";
 setup();
 if(!strcmp(name,"length"))
 {
  /* the longest a sound that plays once can take: start and loop, 70 s */
  CHECK(looping_sound_play_once_ticks(ONCE)==70*TICKS_PER_SECOND,"%ld ticks",looping_sound_play_once_ticks(ONCE));
  CHECK(looping_sound_play_once_ticks(MUSIC)==0,"music that loops was given a length");
  CHECK(looping_sound_play_once_ticks(ONCE_NO_LENGTH)==0,"sounds of no length were given one");
  tracks[ONCE].alternate_loop_sound.index=LONG_SOUND;tracks[ONCE].loop_sound.index=SHORT_SOUND;
  CHECK(looping_sound_play_once_ticks(ONCE)==70*TICKS_PER_SECOND,"the longer alternate loop was not taken");
  CHECK(looping_sound_play_once_ticks(ONCE_HUGE)==60*60*TICKS_PER_SECOND,"a map's huge length was %ld ticks",
   looping_sound_play_once_ticks(ONCE_HUGE));
 }
 else if(!strcmp(name,"ended"))
 {
  /* a dedicated server: the cutscene's sound ends once it has had the time to play, its stop sent to the clients */
  long start=now;
  scripted_looping_sound_start(ONCE,NONE,1.0f);
  CHECK(is_playing(ONCE)&&host_looping_sound_count==1,"fixture: the scripts' start did not play");
  run_to(start+70*TICKS_PER_SECOND);
  CHECK(is_playing(ONCE),"ended before it could have played out");
  run_to(start+72*TICKS_PER_SECOND+1);
  CHECK(!is_playing(ONCE)&&host_looping_sound_count==0,"never ended: the clients would hear it again and again");
  CHECK(sent_of(0,_coop_sound_looping_stop,ONCE)>=1,"its end was not sent to the clients");
 }
 else if(!strcmp(name,"not-resent-near-end"))
 {
  /* resent while it plays (for a joiner), but not in the margin before its end, when a client's copy may have ended */
  long start=now;
  int from;
  scripted_looping_sound_start(ONCE,NONE,1.0f);
  run_to(start+68*TICKS_PER_SECOND-1);
  CHECK(sent_of(0,_coop_sound_looping_start,ONCE)>=30,"resent %d times while it played",
   sent_of(0,_coop_sound_looping_start,ONCE));
  /* (none in the 2 s before its 70 s, nor after) */
  from=sent_count;
  run_to(start+72*TICKS_PER_SECOND+1);
  CHECK(sent_of(from,_coop_sound_looping_start,ONCE)==0,"resent %d times in the 2 s before its end",
   sent_of(from,_coop_sound_looping_start,ONCE));
 }
 else if(!strcmp(name,"drawing-host"))
 {
  /* a host that draws renders its sounds, which end there as they always have: never ended early here */
  long start=now;
  debug_no_drawing=FALSE;
  scripted_looping_sound_start(ONCE,NONE,1.0f);
  run_to(start+600*TICKS_PER_SECOND);
  CHECK(is_playing(ONCE),"a drawing host ended a sound its sound system plays");
 }
 else if(!strcmp(name,"music"))
 {
  /* music that loops plays until the scripts stop it, resent all along */
  long start=now;
  int from;
  scripted_looping_sound_start(MUSIC,NONE,1.0f);
  run_to(start+600*TICKS_PER_SECOND-1);
  CHECK(is_playing(MUSIC),"looping music was ended");
  from=sent_count;
  run_to(start+602*TICKS_PER_SECOND);
  CHECK(sent_of(from,_coop_sound_looping_start,MUSIC)>=1,"looping music stopped being resent");
 }
 else if(!strcmp(name,"no-length"))
 {
  /* a sound that plays once whose sounds give no length is left as before */
  long start=now;
  scripted_looping_sound_start(ONCE_NO_LENGTH,NONE,1.0f);
  run_to(start+600*TICKS_PER_SECOND);
  CHECK(is_playing(ONCE_NO_LENGTH),"a sound of no length was ended at once");
 }
 else if(!strcmp(name,"started-again"))
 {
  /* the scripts start it again while it plays: its time counts from the new start */
  long start=now;
  scripted_looping_sound_start(ONCE,NONE,1.0f);
  run_to(start+50*TICKS_PER_SECOND);
  scripted_looping_sound_start(ONCE,NONE,1.0f);
  start=now;
  run_to(start+60*TICKS_PER_SECOND);
  CHECK(is_playing(ONCE),"the new start was ended on the old one's time");
  run_to(start+73*TICKS_PER_SECOND);
  CHECK(!is_playing(ONCE),"the new start never ended");
 }
 else if(!strcmp(name,"queue-full"))
 {
  /* its end comes when no event can be sent: ended and taken out all the same, with no endless loop */
  long start=now;
  scripted_looping_sound_start(ONCE,NONE,1.0f);
  scripted_looping_sound_start(MUSIC,NONE,1.0f);
  run_to(start+72*TICKS_PER_SECOND-1);
  CHECK(is_playing(ONCE),"fixture: ended before the queue filled");
  queue_full=TRUE;
  run_to(start+73*TICKS_PER_SECOND);
  CHECK(!is_playing(ONCE)&&host_looping_sound_count==1&&is_playing(MUSIC),
   "with the queue full: %d listed, once playing %d",host_looping_sound_count,is_playing(ONCE));
 }
 else if(!strcmp(name,"resent-time"))
 {
  /* the resends of one that plays once say how long ago it started, for a machine that joins during it */
  long start=now;
  struct distributed_coop_event const *event;
  scripted_looping_sound_start(ONCE,NONE,1.0f);
  event=last_sent(_coop_sound_looping_start,ONCE);
  CHECK(event&&event->target==NONE,"the scripts' start said it began %ld ms ago",event?event->target:0);
  run_to(start+30*TICKS_PER_SECOND);
  event=last_sent(_coop_sound_looping_start,ONCE);
  CHECK(event&&event->target>=28000&&event->target<=30000,"a resend 30 s in said %ld ms",event?event->target:0);
  /* (resends fall on even seconds of the game's time: the last before now) */
  CHECK(event->target==((now-1)/(2*TICKS_PER_SECOND)*(2*TICKS_PER_SECOND)-start)*1000/TICKS_PER_SECOND,
   "a resend said %ld ms, not its tick's",event->target);
  /* music that loops starts from its beginning everywhere, as it always has */
  scripted_looping_sound_start(MUSIC,NONE,1.0f);
  start=now;
  run_to(start+10*TICKS_PER_SECOND);
  event=last_sent(_coop_sound_looping_start,MUSIC);
  CHECK(event&&event->target==NONE,"music that loops was resent with %ld ms",event?event->target:0);
 }
 else
 {
  fprintf(stderr,"unknown case %s\n",name);
  return 2;
 }
 return 0;
}
