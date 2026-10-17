"""Exercise production callbacks, preserving native Slayer labels and numeric scores."""
from tools.harness import read, function, structure, enum_with
from tools.test_infection import compile_run, production


def presentation():
    rules = read('port/linux/game/infection.c')
    return production() + '\n' + '\n'.join([
        '#define _CRT_SECURE_NO_WARNINGS 1\n#include <wchar.h>\n#include <stdarg.h>',
        enum_with(read('source/game/game_engine.h'), '_get_score_individual'),
        'struct weapon_datum; struct player_starting_location;',
        structure(read('source/game/game_engine.h'), 'game_engine'),
        '''static struct game_engine infection_engine, slayer_engine;
        static struct distributed_infection_state infection_state;
        static struct distributed_infection_player *infection_player(long i) { return &infection_state.players[i]; }
        static void usprintf(wchar_t *out, wchar_t const *fmt, ...) {
            char ascii[32]; int i; va_list args;
            assert(fmt[0]=='%' && fmt[1]=='l' && fmt[2]=='d' && !fmt[3]);
            va_start(args,fmt); vsnprintf(ascii,sizeof(ascii),"%ld",args); va_end(args);
            for(i=0;ascii[i];i++) out[i]=(wchar_t)ascii[i]; out[i]=0;
        }
        /* Halo wchar_t is 16-bit even on Linux: do not call the host CRT's
           32-bit wide string functions in this asset-free world. */
        static void ustrncpy_terminated(wchar_t *out,wchar_t const *in,long n) {
            while(n>1 && *in) { *out++=*in++; n--; } *out=0;
        }
        static int wide_compare(wchar_t const *a,wchar_t const *b) {
            while(*a && *a==*b) { a++; b++; } return *a-*b;
        }
        #define wcscmp wide_compare
        #define GET_MULTIPLAYER_GAME_TEXT(x) L"Score"
        static boolean infection_initialize(void) { return 1; }
        static void infection_added(long p) {}
        static void infection_tick_player(long p) {}
        static void infection_reconcile_player(long p) {}
        static void infection_spawn(long p) {}
        static void infection_update(void) {}
        static boolean infection_pickup(long u,long w) { return 0; }
        static boolean infection_objective_pickup(long w,long p) { return 0; }
        static void infection_drop(long w) {}
        static void infection_killed(long a,long b,long c,boolean d) {}
        static long game_engine_get_team_score(long team) {
            long i; for(i=0;i<128;i++) if(infection_state.players[i].role==team+1)
                return infection_engine.get_player_score(i,_get_score_team);
            return 0;
        }''',
        function(read('source/game/game_engine_slayer.c'), 'slayer_get_score_header_string'),
        *(function(rules, n) for n in ('infection_score', 'infection_score_string',
                                     'infection_team_string', 'infection_won')),
        function(rules, 'infection_score_header') if 'static wchar_t *infection_score_header(' in rules else '',
        function(rules, 'infection_engine_get'),
        '''static void setup(void) {
            slayer_engine.name="slayer"; slayer_engine.type=2;
            slayer_engine.format_score_name=slayer_get_score_header_string;
            infection_engine_get();
        }''',
    ])


def test_native_header_in_every_phase(tmp_path):
    source = presentation() + '''int main(void) {
        wchar_t text[256]; int phase, winner; setup();
        assert(infection_engine.type==slayer_engine.type);
        assert(infection_engine.name==slayer_engine.name);
        assert(infection_engine.format_score_name==slayer_engine.format_score_name);
        for(phase=0;phase<=3;phase++) for(winner=-1;winner<=1;winner++) {
            infection_state.phase=phase; infection_state.winner=winner;
            infection_engine.format_score_name(text); assert(!wcscmp(text,L"Score"));
        }
        return 0;
    }'''
    assert compile_run(tmp_path, '#define _CRT_SECURE_NO_WARNINGS 1\n' + source) == 0


def test_team_and_player_strings_match_numeric_ranking(tmp_path):
    source = presentation() + '''int main(void) {
        wchar_t text[256]; int i; setup();
        infection_state.players[0].role=1; infection_state.players[0].score=7;
        infection_state.players[1].role=1; infection_state.players[1].score=3;
        infection_state.players[2].role=2; infection_state.players[2].score=12;
        infection_engine.format_team_name(0,text); assert(!wcscmp(text,L"10"));
        infection_engine.format_team_name(1,text); assert(!wcscmp(text,L"12"));
        infection_engine.format_player_score(0,text); assert(!wcscmp(text,L"7"));
        assert(infection_engine.get_player_score(0,_get_score_team)==10);
        /* A conversion changes membership, without rewriting any player's points. */
        infection_state.players[0].role=2;
        infection_engine.format_team_name(0,text); assert(!wcscmp(text,L"3"));
        infection_engine.format_team_name(1,text); assert(!wcscmp(text,L"19"));
        assert(infection_state.players[0].score==7);
        for(i=0;i<128;i++) { infection_state.players[i].role=2; infection_state.players[i].score=1000000; }
        infection_engine.format_team_name(0,text); assert(!wcscmp(text,L"0"));
        infection_engine.format_team_name(1,text); assert(!wcscmp(text,L"128000000"));
        return 0;
    }'''
    assert compile_run(tmp_path, '#define _CRT_SECURE_NO_WARNINGS 1\n' + source) == 0
