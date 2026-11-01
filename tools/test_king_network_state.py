"""Exercise upstream hill framing and stock/multi-hill transitions on real functions."""
import subprocess
from pathlib import Path

from harness import function, structure
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]


def test_hill_state_framing_and_transitions(tmp_path):
    source = (ROOT / "source/game/game_engine_king.c").read_text()
    code = r'''
#include <assert.h>
#include <string.h>
typedef int boolean;
typedef unsigned char byte;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define MAXIMUM_HILLS_AT_ONCE 5
#define VALID_INDEX(i,n) ((i)>=0 && (i)<(n))
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define csmemcpy memcpy
#define csmemset memset
enum { king_hill_uncontrolled, _multiplayer_sound_hill_move };
struct king_globals { long score[2], score_tick[2], hill_timer, hill_id; };
struct king_hill { long hill_id, hill_previous_controller, hill_state; };
static struct king_globals king_globals;
static struct king_hill king_hills[MAXIMUM_HILLS_AT_ONCE];
static short king_hill_count;
static int sounds, finds, score_sounds;
static void find_hill(struct king_hill *hill) { (void)hill; finds++; }
static void king_hills_to_globals(void) { king_globals.hill_id=king_hills[0].hill_id; }
static void game_engine_play_multiplayer_sound(int sound) { (void)sound; sounds++; }
static void king_client_score_sounds(long team,long before,long after)
{ (void)team; (void)before; (void)after; score_sounds++; }
''' + structure(source, "king_extra_hills_network_state") + "\n" + "\n".join(
        function(source, name) for name in (
            "game_engine_king_write_network_state", "game_engine_king_read_network_state")) + r'''
int main(void) {
 byte data[1024]; long stock=sizeof(king_globals), full;
 struct king_extra_hills_network_state extra;
 memset(data,0xA5,sizeof(data));
 king_hill_count=1; king_globals.hill_id=4; king_globals.score[0]=7;
 assert(game_engine_king_write_network_state(data,stock-1)==0);
 assert(game_engine_king_write_network_state(data,sizeof(data))==stock);
 assert(data[stock]==0xA5);
 assert(!game_engine_king_read_network_state(data,stock-1,TRUE));
 assert(game_engine_king_read_network_state(data,stock,TRUE));
 assert(king_hill_count==1 && king_hills[0].hill_id==4 && finds==1);
 assert(sounds==0 && score_sounds==0);
 king_hill_count=3; king_hills[1].hill_id=5; king_hills[2].hill_id=9;
 full=game_engine_king_write_network_state(data,sizeof(data));
 assert(full==stock+(long)sizeof(extra));
 assert(game_engine_king_write_network_state(data,stock)==stock);
 king_hill_count=1;
 assert(game_engine_king_read_network_state(data,full,FALSE));
 assert(king_hill_count==3 && king_hills[1].hill_id==5 && king_hills[2].hill_id==9);
 assert(sounds==1 && finds==3 && score_sounds==2);
 assert(!game_engine_king_read_network_state(data,full-1,FALSE));
 memcpy(&extra,data+stock,sizeof(extra)); extra.hill_count=5;
 memcpy(data+stock,&extra,sizeof(extra));
 assert(!game_engine_king_read_network_state(data,full,FALSE));
 assert(king_hill_count==3 && sounds==1);
 extra.hill_count=-1; memcpy(data+stock,&extra,sizeof(extra));
 assert(!game_engine_king_read_network_state(data,full,FALSE));
 assert(game_engine_king_read_network_state(data,stock,FALSE));
 assert(king_hill_count==1 && sounds==2);
 return 0;
}
'''
    path = tmp_path / "test.c"
    path.write_text(code)
    binary = compile_tool(tmp_path, [path])
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
