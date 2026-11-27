/* A co-op client's script looping sounds, from the host's sound events
(network_coop.c's client_apply_sound) and game_sound.c's scripted looping
sounds. A sound that plays once (fake impulse: a cutscene's) and has played
out on the client is not started again by the host's resends. */
#include "harness.h"

typedef unsigned short word;
#define PIN(x,lo,hi) ((x)<(lo)?(lo):(x)>(hi)?(hi):(x))
#define SET_FLAG(f,b,v) ((f)=(v)?(f)|(1U<<(b)):(f)&~(1U<<(b)))
#define SOUND_DEFINITION_TAG 0x736e6421UL
#define LOOPING_SOUND_DEFINITION_TAG 0x6c736e64UL
#define MAXIMUM_LOOPING_SOUNDS 32
#define CLIENT_SCRIPT_SOUND_COUNT 16

#include "types.inc"
struct looping_sound_definition {unsigned long flags;long runtime_scripting_sound_index;};

/* the map's looping sounds: 0 plays once (fake impulse), 1 is music that loops */
enum {ONCE=0,MUSIC=1,DEFINITIONS=2};
static struct looping_sound_definition definitions[DEFINITIONS];
static struct looping_sound_definition *looping_sound_definition_get(long index)
{
 CHECK(index>=0&&index<DEFINITIONS,"no such looping sound %ld",index);
 return &definitions[index];
}
static boolean distributed_tag_of_group(long tag_index,unsigned long group)
{
 return group==LOOPING_SOUND_DEFINITION_TAG&&tag_index>=0&&tag_index<DEFINITIONS;
}
static boolean network_objects_client_has(long object_index){(void)object_index;return FALSE;}

/* the playing looping sounds (game_sound.c's data) */
struct game_looping_sound_datum {long definition_index;unsigned long flags;real scale;boolean used;};
enum {_game_looping_sound_unattached_bit,_game_looping_sound_scripted_bit,_game_looping_sound_alternate_bit,
 _game_looping_sound_unattached_stop_bit,_game_looping_sound_unattached_stop_fixed_fadeout_bit};
static struct game_looping_sound_datum playing[8];
static int starts[DEFINITIONS],full;
static long game_looping_sound_new(long object_index,long definition_index,char const *marker,long node)
{
 long index;
 (void)object_index;(void)marker;(void)node;
 if(full)return NONE;
 for(index=0;index<8;index++)
 {
  if(!playing[index].used)
  {
   memset(&playing[index],0,sizeof(playing[index]));
   playing[index].used=TRUE;playing[index].definition_index=definition_index;
   starts[definition_index]++;
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
static void network_coop_note_sound(short kind,long definition_index,long object_index,real scale)
{(void)kind;(void)definition_index;(void)object_index;(void)scale;}

/* coop_sound_sync.c's: the host's time for each sound, as the client was told it */
static long synced[DEFINITIONS];
static int sync_begins;
static void coop_sound_sync_begin(long definition_index,long elapsed_milliseconds)
{
 CHECK(definition_index>=0&&definition_index<DEFINITIONS,"synced no such sound %ld",definition_index);
 synced[definition_index]=elapsed_milliseconds;sync_begins++;
}
static void coop_sound_sync_end(long definition_index)
{
 CHECK(definition_index>=0&&definition_index<DEFINITIONS,"ended no such sound %ld",definition_index);
 synced[definition_index]=NONE;
}
static void scripted_music_stop_all(void){}

/* as game_sound.c's update when a fake impulse sound's permutations have all played */
static void play_out(long definition_index)
{
 struct looping_sound_definition *definition=&definitions[definition_index];
 long index=definition->runtime_scripting_sound_index;
 CHECK(index!=NONE,"sound %ld is not playing to play out",definition_index);
 definition->runtime_scripting_sound_index=NONE;
 playing[index].used=FALSE;
}
static boolean is_playing(long definition_index){return definitions[definition_index].runtime_scripting_sound_index!=NONE;}

/* the client's impulse sounds, which these cases don't send */
static struct {long definition_indices[CLIENT_SCRIPT_SOUND_COUNT];short next;} client_script_sounds;
static void scripted_sound_new(long tag,long object,real scale){(void)tag;(void)object;(void)scale;}

#include "under_test.inc"

/* (target: the host's milliseconds since it started it, NONE for just now) */
static void send_at(int type,long definition_index,long target)
{
 struct distributed_coop_event event;
 memset(&event,0,sizeof(event));
 event.kind=0;event.type=(byte)type;event.tag_index=definition_index;event.object_index=NONE;event.reals[0]=1.0f;
 event.target=target;
 client_apply_sound(&event);
}
static void send(int type,long definition_index){send_at(type,definition_index,NONE);}
/* the host's two-second resend, or the one when a machine joins */
static void resend(long definition_index){send(_coop_sound_looping_start,definition_index);}

static void setup(void)
{
 int index;
 memset(definitions,0,sizeof(definitions));
 for(index=0;index<DEFINITIONS;index++)definitions[index].runtime_scripting_sound_index=NONE;
 SET_FLAG(definitions[ONCE].flags,_looping_sound_fake_impulse_sound_bit,TRUE);
 memset(playing,0,sizeof(playing));memset(starts,0,sizeof(starts));full=FALSE;
 client_looping_sounds.count=0;
 for(index=0;index<DEFINITIONS;index++)synced[index]=NONE;
 sync_begins=0;
}

int main(int argc,char **argv)
{
 char const *name=argc>1?argv[1]:"";
 setup();
 if(!strcmp(name,"played-out"))
 {
  /* the cutscene's sound starts, plays to its end here, and the host, whose copy never ends, resends it */
  send(_coop_sound_looping_start,ONCE);
  CHECK(is_playing(ONCE)&&starts[ONCE]==1,"the host's start did not start it");
  play_out(ONCE);
  resend(ONCE);resend(ONCE);resend(ONCE);
  CHECK(!is_playing(ONCE)&&starts[ONCE]==1,"played out, then started again %d times by the resends",starts[ONCE]-1);
 }
 else if(!strcmp(name,"playing"))
 {
  send(_coop_sound_looping_start,ONCE);
  resend(ONCE);resend(ONCE);
  CHECK(is_playing(ONCE)&&starts[ONCE]==1,"a resend restarted a sound still playing");
 }
 else if(!strcmp(name,"started-again"))
 {
  /* the scripts start it again: the host sends a stop, then a start */
  send(_coop_sound_looping_start,ONCE);play_out(ONCE);
  send(_coop_sound_looping_stop,ONCE);send(_coop_sound_looping_start,ONCE);
  CHECK(is_playing(ONCE)&&starts[ONCE]==2,"the scripts' new start was refused");
  play_out(ONCE);resend(ONCE);
  CHECK(!is_playing(ONCE)&&starts[ONCE]==2,"the second playing restarted after it ended");
 }
 else if(!strcmp(name,"late-joiner"))
 {
  /* a client that joins after the start hears it from the resend that comes with its join */
  resend(ONCE);
  CHECK(is_playing(ONCE)&&starts[ONCE]==1,"the join's resend did not start it");
  play_out(ONCE);resend(ONCE);
  CHECK(!is_playing(ONCE)&&starts[ONCE]==1,"the joiner's copy restarted after it ended");
 }
 else if(!strcmp(name,"music"))
 {
  /* music that loops, lost here some other way, is started again as before */
  send(_coop_sound_looping_start,MUSIC);play_out(MUSIC);resend(MUSIC);
  CHECK(is_playing(MUSIC)&&starts[MUSIC]==2,"music lost here was not started again");
  CHECK(client_looping_sounds.count==0,"music was remembered as played out");
 }
 else if(!strcmp(name,"no-room"))
 {
  /* a start that found no room is tried again by the next resend */
  full=TRUE;send(_coop_sound_looping_start,ONCE);
  CHECK(!is_playing(ONCE),"fixture: a start with no room played");
  full=FALSE;resend(ONCE);
  CHECK(is_playing(ONCE)&&starts[ONCE]==1,"a start that found no room was never tried again");
 }
 else if(!strcmp(name,"stopped"))
 {
  /* the host stops it while it plays: stopped here and forgotten */
  send(_coop_sound_looping_start,ONCE);send(_coop_sound_looping_stop,ONCE);
  CHECK(!is_playing(ONCE)&&client_looping_sounds.count==0,"the host's stop left it playing or remembered");
 }
 else if(!strcmp(name,"joined-during"))
 {
  /* a client that joins 40 s into the cutscene's sound starts it from there */
  send_at(_coop_sound_looping_start,ONCE,40000);
  CHECK(is_playing(ONCE)&&synced[ONCE]==40000,"the host's 40 s became %ld",synced[ONCE]);
  CHECK(sync_begins==1,"the time was given %d times for one start",sync_begins);
  /* the resends while it plays change nothing */
  send_at(_coop_sound_looping_start,ONCE,42000);
  CHECK(synced[ONCE]==40000&&sync_begins==1,"a resend while it played moved it to %ld",synced[ONCE]);
  /* and the host's stop forgets it */
  send(_coop_sound_looping_stop,ONCE);
  CHECK(synced[ONCE]==NONE,"the host's stop kept its time");
 }
 else if(!strcmp(name,"music-time"))
 {
  /* music that loops starts at its beginning, whatever time a host sends */
  send_at(_coop_sound_looping_start,MUSIC,40000);
  CHECK(is_playing(MUSIC)&&synced[MUSIC]==NONE,"music that loops was given the time %ld",synced[MUSIC]);
 }
 else if(!strcmp(name,"just-started"))
 {
  /* the scripts' own start, or an older host's resend: from its beginning */
  synced[ONCE]=12345;
  send(_coop_sound_looping_start,ONCE);
  CHECK(is_playing(ONCE)&&synced[ONCE]==NONE,"a start with no time kept the time %ld",synced[ONCE]);
 }
 else
 {
  fprintf(stderr,"unknown case %s\n",name);
  return 2;
 }
 return 0;
}
