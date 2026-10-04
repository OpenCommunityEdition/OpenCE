"""Exercise real definition transitions and channel updates with instance limits."""
import argparse
import subprocess
import sys
import tempfile
from pathlib import Path
from test_sound_lifecycle import function

ROOT = Path(__file__).resolve().parent.parent
PRELUDE = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
typedef float real;
typedef int boolean;
#define NONE (-1)
#define TRUE 1
#define FALSE 0
#define TEST_FLAG(x,b) (((x)&(1u<<(b)))!=0)
#define SET_FLAG(x,b,v) ((x)=(v)?((x)|(1u<<(b))):((x)&~(1u<<(b))))
#define match_assert(f,l,c) do {if(!(c)) abort();} while(0)
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
enum { _sound_impulse, _sound_start_track, _sound_loop_track, _sound_stopping_track, _sound_stop_track };
enum { _sound_waiting_for_cache_bit, _fade_in_at_start_bit, _sound_definition_linked_permutations_bit, _looping_sound_fake_impulse_sound_bit };
enum { _sound_channel_idle, _sound_channel_playing, _sound_channel_queued, _sound_fade_mode_crossfade };
struct block {int count; void *address;};
struct sound_permutation {real gain; int next_permutation_index;};
struct sound_pitch_range {struct block permutations; real playback_rate,natural_pitch;};
struct sound_definition {
    struct block pitch_ranges;
    real zero_pitch_modifier,one_pitch_modifier,inner_cone_angle,outer_cone_angle,outer_cone_gain;
    real gain_modifier,zero_gain_modifier,one_gain_modifier,maximum_bend_per_second;
    int sound_class,flags;
};
struct sound_source {real scale,gain;};
struct sound_datum {
    int live,flags,type,playing_channel_index,loop_track_index,pitch_range_index,permutation_index;
    long definition_index,next_definition_index,source_identifier,fade_start_time,fade_stop_time;
    real pitch,fade_interpolation_end;
    struct sound_source source;
};
struct sound_channel_datum {long sound_index;real pitch;struct sound_permutation *playing_permutation;};
struct looping_sound_track {int flags;real gain;};
struct looping_sound_definition {struct block tracks;int flags;};
struct looping_sound_datum {long definition_index;int ordered_sounds_finished;struct {long primary_sound_index;} tracks[1];};
struct platform_sound_channel_properties {
    real minimum_distance,maximum_distance,cone_inside_angle,cone_outside_angle,cone_outside_gain;
    real reverb_attenuation,gain,pitch;
};
struct sound_channel_summary {
    int like_source_count,maximum_source_instance_count,like_definition_count,maximum_instance_count;
    short like_source_channels[4],like_definition_channels[4];
};
struct sound_class {real wet_gain;};
static struct sound_datum sounds[2];
static struct sound_channel_datum channels[2];
static struct sound_definition definition;
static struct sound_pitch_range range;
static struct sound_permutation permutation;
static struct looping_sound_track track;
static struct looping_sound_definition loop_definition;
static struct looping_sound_datum loop;
static struct sound_class sound_class;
static int limited,victim,queues,property_writes,updates;
static void channel_update(short i) {(void)i;updates++;}
static struct {void (*channel_update)(short);} platform={channel_update};
static struct {int idling;void *unused;typeof(platform) *platform_definition;} sound_manager_globals;
static real sound_pitch_range_fade_time=.1f;
static struct sound_datum *sound_get(long i) {if(i<0||i>1||!sounds[i].live) abort();return &sounds[i];}
static struct sound_channel_datum *channel_get(short i) {return &channels[i];}
static struct sound_definition *sound_definition_get(long i) {(void)i;return &definition;}
static struct looping_sound_datum *looping_sound_get(long i) {(void)i;return &loop;}
static struct looping_sound_definition *looping_sound_definition_get(long i) {(void)i;return &loop_definition;}
static struct sound_class *sound_class_get(int i) {(void)i;return &sound_class;}
static int sound_definition_find_pitch_range_by_pitch(struct sound_definition *d,real p,int i) {(void)d;(void)p;(void)i;return 0;}
static int sound_definition_next_permutation(struct sound_definition *d,int p,int i) {(void)d;(void)p;(void)i;return 0;}
static void sound_channel_summary_build(struct sound_channel_summary *s,long i) {
    (void)i;memset(s,0,sizeof(*s));s->maximum_source_instance_count=1;s->maximum_instance_count=4;
    s->like_source_count=limited;
}
static short sound_find_like_channel(long i,short *c,int n) {(void)i;(void)c;(void)n;return victim;}
static void sound_stop(long i) {
    sounds[i].live=0;channels[i].sound_index=NONE;sounds[i].playing_channel_index=NONE;
}
static real sound_scale_value(real a,real b,real c,real s) {(void)b;(void)c;(void)s;return a;}
static real sound_definition_get_minimum_distance(long i) {(void)i;return 0;}
static real sound_manager_master_gain(int i) {(void)i;return 1;}
static real limit_pitch(real a,real b,real c) {(void)b;(void)c;return a;}
static int sound_cache_sound_loaded(void *p) {(void)p;return 1;}
static int _sound_cache_sound_request(void *p,int a,int b,int c) {(void)p;(void)a;(void)b;(void)c;return 1;}
static short channel_get_state(short i) {(void)i;return _sound_channel_playing;}
static void channel_set_properties_hardware(short i,void *p,int g) {(void)i;(void)p;(void)g;property_writes++;}
static void channel_queue_sound(short i,void *p) {(void)i;(void)p;queues++;}
static long update_potentially_audible_looping_sound(long d,long o,short t,short s) {(void)d;(void)o;(void)t;(void)s;abort();}
static void sound_start_fade(short m,real s,long a,long b) {(void)m;(void)s;(void)a;(void)b;abort();}
static void reset(int limit,int candidate) {
    memset(sounds,0,sizeof(sounds));memset(channels,0,sizeof(channels));memset(&definition,0,sizeof(definition));
    sounds[0].live=sounds[1].live=1;sounds[0].playing_channel_index=0;sounds[1].playing_channel_index=1;
    sounds[0].type=_sound_start_track;sounds[0].next_definition_index=1;sounds[0].pitch=1;
    sounds[0].source.gain=1;channels[0].sound_index=0;channels[1].sound_index=1;
    channels[0].playing_permutation=&permutation;permutation.next_permutation_index=NONE;permutation.gain=1;
    range.permutations.address=&permutation;range.permutations.count=1;range.playback_rate=range.natural_pitch=1;
    definition.pitch_ranges.address=&range;definition.pitch_ranges.count=1;definition.gain_modifier=1;
    loop_definition.tracks.address=&track;loop_definition.tracks.count=1;track.gain=1;
    loop.tracks[0].primary_sound_index=0;sound_manager_globals.platform_definition=&platform;
    limited=limit;victim=candidate;queues=property_writes=updates=0;
}
'''
TESTS = r'''
#define CHECK(c) do {if(!(c)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);return 1;}} while(0)
int main(void) {
    reset(1,NONE);update_channel_for_looping_sound(0,1);
    CHECK(!sounds[0].live);CHECK(channels[0].sound_index==NONE);
    CHECK(queues==0);CHECK(property_writes==0);CHECK(updates==0);
    reset(1,1);update_channel_for_looping_sound(0,1);
    CHECK(sounds[0].live);CHECK(!sounds[1].live);CHECK(queues==1);CHECK(property_writes==1);CHECK(updates==1);
    reset(0,NONE);update_channel_for_looping_sound(0,1);
    CHECK(sounds[0].live && sounds[1].live);CHECK(queues==1);CHECK(property_writes==1);CHECK(updates==1);
    reset(1,0);update_channel_for_looping_sound(0,1);
    CHECK(!sounds[0].live);CHECK(queues==0);CHECK(property_writes==0);CHECK(updates==0);
    puts("PASS: self-retired definition transition cannot queue ownerless audio; surviving transitions still play");
    return 0;
}
'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc',default='clang')
    parser.add_argument('--source',type=Path,default=ROOT/'source/sound/sound_manager.c')
    args=parser.parse_args()
    source=args.source.read_text()
    compiler=[args.cc,'-std=gnu11','-O2','-fuse-ld=lld']
    if sys.platform=='win32': compiler.append('--target=i686-pc-windows-msvc')
    with tempfile.TemporaryDirectory(prefix='halo-transition-test-') as directory:
        unit=Path(directory)/'transition.c'; exe=Path(directory)/'transition.exe'
        unit.write_text(PRELUDE+function(source,'sound_set_definition_end')+function(source,'update_channel_for_looping_sound')+TESTS)
        subprocess.run([*compiler,str(unit),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)

if __name__=='__main__':main()
