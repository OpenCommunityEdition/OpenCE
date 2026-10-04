"""Compile and exercise the real sound-manager functions with a fake backend.

No game assets or profile are needed. Run with Python and clang on PATH.
The harness models multiple voices owned by a loop, including cache-delayed
voices and the outgoing voice of a crossfade; it does not model SDL mixing.
"""

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def function(source, name):
    match = re.search(r"^(?:static )?[\w *]+\b" + name + r"\(\s*[^;{]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError(f"Function not found: {name}")
    start = source.index("{", match.start())
    depth = 1
    end = start + 1
    # These functions have no braces in strings or comments.
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


PRELUDE = r'''
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef float real;
typedef int boolean;
#define NONE (-1)
#define TRUE 1
#define FALSE 0
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define TEST_FLAG(x,b) (((x)&(1u<<(b)))!=0)
#define SET_FLAG(x,b,v) ((x)=(v)?((x)|(1u<<(b))):((x)&~(1u<<(b))))
#define match_assert(file,line,condition) do { if (!(condition)) abort(); } while (0)
#define match_vassert(file,line,condition,message) match_assert(file,line,condition)
#define TAG_BLOCK_GET_ELEMENT(block,index,type) (&((type *)(block)->address)[index])
enum { _sound_fade_mode_linear, _sound_fade_mode_crossfade };
enum { _looping_sound_refresh_start, _looping_sound_refresh_loop, _looping_sound_refresh_stop };
enum { _sound_impulse, _sound_start_track, _sound_loop_track, _sound_stopping_track, _sound_stop_track };
enum { _fade_in_at_start_bit, _fade_out_at_stop_bit, _fade_in_alternate_bit };
enum { _looping_sound_fake_impulse_sound_bit };
struct tag_reference { long index; };
struct tag_block { int count; void *address; };
struct sound_source {
    int spatialization_mode;
    struct { int position, forward; } location;
};
enum { _sound_spatialization_mode_none };
struct sound_datum {
    int live, type, loop_track_index, playing_channel_index;
    long source_identifier, definition_index, next_definition_index;
    real fade_interpolation_start, fade_interpolation_end;
    long fade_start_time, fade_stop_time;
    short fade_mode;
};
struct looping_sound_track {
    unsigned flags;
    real fade_in_duration, fade_out_duration;
    struct tag_reference start_sound, loop_sound, stop_sound;
    struct tag_reference alternate_loop_sound, alternate_stop_sound;
};
struct looping_sound_definition {
    unsigned flags;
    struct tag_reference continuous_damage_effect;
    struct tag_block tracks;
    real runtime_maximum_distance;
};
struct looping_sound_datum {
    int live, component_sound_count, ordered_sounds_finished, flip_flop, alternate, state;
    long definition_index;
    struct sound_source source;
    struct { long primary_sound_index; } tracks[4];
};
static struct { long render_time; int flip_flop; } sound_manager_globals;
static struct sound_datum sounds[64];
static struct looping_sound_datum loop;
static struct looping_sound_track tracks[4];
static struct looping_sound_definition definition;
static int sound_data, looping_sound_data;
static real sound_fade_exponent = 1.5f, sound_inaudible_fade_out_time = 2.f;
static struct sound_datum *sound_get(long i) { if(i<0 || i>=64 || !sounds[i].live) abort(); return &sounds[i]; }
static struct sound_datum *datum_get(int data,long i) { (void)data; return sound_get(i); }
static long data_next_index(int data,long i) { (void)data; for(i++;i<64;i++) if(sounds[i].live) return i; return NONE; }
static int sound_is_active(void) { return TRUE; }
static int valid_real_normal3d(void *v) { (void)v; return TRUE; }
static void render_debug_looping_sound(long i,void *s) { (void)i;(void)s; }
static long looping_sound_find(long i) { (void)i; return loop.live?0:NONE; }
static long looping_sound_new(long d,long i,struct sound_source *s) {
    (void)i; memset(&loop,0,sizeof(loop)); loop.live=1; loop.definition_index=d;loop.source=*s; return 0;
}
static struct looping_sound_datum *looping_sound_get(long i) { (void)i;return &loop; }
static struct looping_sound_definition *looping_sound_definition_get(long i) { (void)i;return &definition; }
static void datum_delete(int data,long i) { (void)data;(void)i;loop.live=0; }
static void player_effect_continuous_refresh(long i,void *p) { (void)i;(void)p; }
static int source_audible(void *s,real distance) { (void)s;(void)distance;return 0; }
static void sound_set_definition_begin(long i,long d) { sounds[i].next_definition_index=d; }
static long update_potentially_audible_looping_sound(long d,long owner,short track,short type) {
    long i;
    for(i=0;i<64;i++) if(!sounds[i].live) {
        memset(&sounds[i],0,sizeof(sounds[i])); sounds[i].live=1;sounds[i].type=type;
        sounds[i].definition_index=d;sounds[i].source_identifier=owner;sounds[i].loop_track_index=track;
        sounds[i].playing_channel_index=NONE;sounds[i].fade_interpolation_end=1.f;
        loop.component_sound_count++;return i;
    }
    abort();
}
static void reset(void) {
    memset(sounds,0,sizeof(sounds));memset(&loop,0,sizeof(loop));memset(tracks,0,sizeof(tracks));
    memset(&definition,0,sizeof(definition));definition.tracks.count=1;definition.tracks.address=tracks;
    definition.continuous_damage_effect.index=NONE;
    tracks[0].start_sound.index=1;tracks[0].loop_sound.index=2;tracks[0].stop_sound.index=NONE;
    tracks[0].alternate_loop_sound.index=tracks[0].alternate_stop_sound.index=NONE;
    tracks[0].flags=1u<<_fade_out_at_stop_bit;tracks[0].fade_out_duration=.1f;
    sound_manager_globals.render_time=1000;
}
static int failures;
#define CHECK(c,description) do { if(!(c)) {fprintf(stderr,"FAIL: %s\n",description);failures++;} } while(0)
'''

TESTS = r'''
static void retire_faded_voices(void) {
    long i;
    for(i=0;i<64;i++) if(sounds[i].live && sound_calculate_fade(i)==0.f) {
        sounds[i].live=0;loop.component_sound_count--;
        if(loop.tracks[0].primary_sound_index==i) loop.tracks[0].primary_sound_index=NONE;
    }
}
static void advance(int hz,int milliseconds) {
    int start=(int)sound_manager_globals.render_time,frame;
    for(frame=1;frame<=hz*milliseconds/1000+1;frame++) {
        sound_manager_globals.render_time=start+frame*1000/hz;retire_faded_voices();
    }
}
static void refresh(int state) {
    struct sound_source source={0};sound_refresh_looping(0,123,&source,state,FALSE,0.f);
}
int main(void) {
    long primary,secondary;int hz,frame;
    reset();refresh(_looping_sound_refresh_start);primary=loop.tracks[0].primary_sound_index;
    CHECK(sound_calculate_fade(primary)==1.f,"fresh voice starts audible");
    sound_start_fade(_sound_fade_mode_linear,.1f,NONE,primary);
    sound_manager_globals.render_time=1100;
    CHECK(sound_calculate_fade(primary)==0.f,"fade reaches zero");
    CHECK(sound_calculate_fade(primary)==0.f,"second fade query stays zero");
    sound_manager_globals.render_time=5000;
    CHECK(sound_calculate_fade(primary)==0.f,"completed fade stays silent later");
    for(hz=30;hz<=240;hz*=2) {
        reset();refresh(_looping_sound_refresh_start);primary=loop.tracks[0].primary_sound_index;
        /* A second voice represents a pending cache request or an outgoing crossfade. */
        secondary=update_potentially_audible_looping_sound(2,0,0,_sound_loop_track);
        (void)secondary;refresh(_looping_sound_refresh_stop);advance(hz,200);
        CHECK(loop.component_sound_count==0,"stop cancels every owned start/loop voice");
        reset();refresh(_looping_sound_refresh_start);refresh(_looping_sound_refresh_start);
        refresh(_looping_sound_refresh_stop);advance(hz,200);
        CHECK(loop.component_sound_count==0,"restart cannot orphan previous intro voice");
        reset();tracks[0].flags|=1u<<_fade_in_at_start_bit;tracks[0].fade_in_duration=.2f;
        refresh(_looping_sound_refresh_start);refresh(_looping_sound_refresh_stop);advance(hz,300);
        CHECK(loop.component_sound_count==0,"stop cancels intro alongside fading-in loop");
        reset();refresh(_looping_sound_refresh_start);advance(hz,34);refresh(_looping_sound_refresh_stop);
        advance(hz,34);refresh(_looping_sound_refresh_start);advance(hz,34);refresh(_looping_sound_refresh_stop);
        advance(hz,200);CHECK(loop.component_sound_count==0,"rapid release/repress finishes within fade duration");
        reset();refresh(_looping_sound_refresh_start);primary=loop.tracks[0].primary_sound_index;
        secondary=update_potentially_audible_looping_sound(3,999,0,_sound_loop_track);
        refresh(_looping_sound_refresh_stop);advance(hz,200);
        CHECK(!sounds[primary].live && sounds[secondary].live,"stop preserves another loop owner");
        reset();refresh(_looping_sound_refresh_start);primary=loop.tracks[0].primary_sound_index;
        secondary=update_potentially_audible_looping_sound(3,0,1,_sound_loop_track);
        refresh(_looping_sound_refresh_stop);advance(hz,200);
        CHECK(!sounds[primary].live && sounds[secondary].live,"stop preserves a different track");
        reset();tracks[0].stop_sound.index=3;refresh(_looping_sound_refresh_start);
        primary=loop.tracks[0].primary_sound_index;refresh(_looping_sound_refresh_stop);advance(hz,200);
        CHECK(!sounds[primary].live && loop.component_sound_count==1,"authored stop cue survives cancellation");
        reset();definition.flags=1u<<_looping_sound_fake_impulse_sound_bit;tracks[0].flags=0;
        refresh(_looping_sound_refresh_start);primary=loop.tracks[0].primary_sound_index;
        refresh(_looping_sound_refresh_stop);advance(hz,200);
        CHECK(sounds[primary].live,"ordered fake impulse without fade flag keeps its authored ending");
        reset();refresh(_looping_sound_refresh_start);refresh(_looping_sound_refresh_stop);
        for(frame=0;frame<5;frame++) { advance(hz,20);refresh(_looping_sound_refresh_stop); }
        advance(hz,100);CHECK(loop.component_sound_count==0,"repeated stop does not prolong the fade");
    }
    reset();refresh(_looping_sound_refresh_start);primary=loop.tracks[0].primary_sound_index;
    sound_start_fade(_sound_fade_mode_crossfade,.1f,NONE,primary);
    for(frame=0;frame<=100;frame++) {
        sound_manager_globals.render_time=1000+frame;
        real a=sound_calculate_fade(primary),b=sound_calculate_fade(primary);
        CHECK(a==b,"crossfade queries are idempotent");
    }
    CHECK(sound_calculate_fade(primary)==0.f,"crossfade remains zero at completion");
    if(failures) return 1;
    puts("PASS: sound lifecycle at 30/60/120/240 FPS, repeated fade queries, restart, crossfade, cache delay and owner isolation");
    return 0;
}
'''

BACKEND_PRELUDE = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define CALLBACK
enum { _sound_channel_idle, _sound_channel_playing, _sound_channel_queued };
enum { XMEDIAPACKET_STATUS_SUCCESS, XMEDIAPACKET_STATUS_FLUSHED,
       XMEDIAPACKET_STATUS_FAILURE, XMEDIAPACKET_STATUS_PENDING };
struct sound_channel {
    int state, packet_count, stopping;
    void *stream, *playing_permutation, *queued_permutation;
};
static struct sound_channel channel;
static struct { int actual_channel_count, paused; } dsound_globals = { 1, 0 };
static int refills, unlocks, stops;
static struct sound_channel *channel_get(short i) { if(i) abort(); return &channel; }
static void sound_cache_sound_hardware_unlock(void *p) { (void)p;unlocks++; }
static void interrupt_time_error(void *r,const char *m) { (void)r;fprintf(stderr,"%s\n",m);abort(); }
static void dsound_channel_fill(short i) {
    (void)i;if(channel.playing_permutation) { refills++;channel.packet_count++; }
}
static void dsound_channel_callback(void *,void *,unsigned long);
static void DirectSoundStopStream(void *stream) {
    int first=1;(void)stream;stops++;
    /* SDL flush reports an already mixed head as SUCCESS, synchronously. */
    while(channel.packet_count) {
        dsound_channel_callback(NULL,NULL,first?XMEDIAPACKET_STATUS_SUCCESS:XMEDIAPACKET_STATUS_FLUSHED);
        first=0;
    }
}
static void reset(int state) {
    memset(&channel,0,sizeof(channel));channel.state=state;channel.packet_count=3;
    channel.playing_permutation=channel.queued_permutation=&channel;
    dsound_globals.paused=0;refills=unlocks=stops=0;
}
static int failures;
#define CHECK(c,d) do { if(!(c)) {fprintf(stderr,"FAIL: %s\n",d);failures++;} } while(0)
'''

BACKEND_TESTS = r'''
int main(void) {
    reset(_sound_channel_playing);channel_stop(0);
    CHECK(refills==0,"stop callbacks cannot queue more audio");
    CHECK(unlocks==3 && channel.packet_count==0,"stop releases exactly the outstanding packets");
    CHECK(!channel.playing_permutation && !channel.queued_permutation,"stop clears both permutations");
    CHECK(channel.state==_sound_channel_idle,"stopped channel becomes idle");
    reset(_sound_channel_idle);channel_stop(0);
    CHECK(channel.packet_count==0 && stops==1,"idle state still drains outstanding packets");
    channel_stop(0);CHECK(stops==1,"repeated stop is harmless");
    reset(_sound_channel_playing);dsound_globals.paused=1;channel_stop(0);
    CHECK(refills==0 && channel.packet_count==0,"paused stop drains without refill");
    reset(_sound_channel_playing);dsound_channel_callback(NULL,NULL,XMEDIAPACKET_STATUS_SUCCESS);
    CHECK(refills==1 && channel.packet_count==3,"normal playback completion still refills");
    if(failures) return 1;
    puts("PASS: synchronous flush, idle backlog, paused cancellation, repeated stop and normal refill");return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--source", type=Path, default=ROOT / "source/sound/sound_manager.c")
    parser.add_argument("--backend-source", type=Path, default=ROOT / "source/sound/sound_dsound_xbox.c")
    args = parser.parse_args()
    source = args.source.read_text(encoding="utf-8")
    compiler = [args.cc, "-std=c11", "-O2", "-fuse-ld=lld", "-Wno-void-pointer-to-int-cast"]
    if sys.platform == "win32":
        compiler.append("--target=i686-pc-windows-msvc")
    names = ["sound_calculate_fade", "sound_start_fade"]
    if re.search(r"static void sound_fade_looping_track_components\(", source):
        names.append("sound_fade_looping_track_components")
    names.append("sound_refresh_looping")
    with tempfile.TemporaryDirectory(prefix="halo-sound-test-") as directory:
        path = Path(directory)
        unit = path / "sound_lifecycle.c"
        executable = path / "sound_lifecycle.exe"
        unit.write_text(PRELUDE + "\n".join(function(source, name) for name in names) + TESTS, encoding="utf-8")
        subprocess.run([*compiler, str(unit), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
        backend = args.backend_source.read_text(encoding="utf-8")
        unit.write_text(BACKEND_PRELUDE + function(backend, "dsound_channel_callback") +
                        function(backend, "channel_stop") + BACKEND_TESTS, encoding="utf-8")
        subprocess.run([*compiler, str(unit), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
