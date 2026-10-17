"""Native spawn exclusion: stationary stand-ins can occupy every map start."""
import pytest
from tools.harness import read, function
from tools.test_infection import compile_run


@pytest.mark.parametrize('teams', [0, 1])
def test_occupied_start_is_released_by_movement(tmp_path, teams):
    world = r'''
    #include <assert.h>
    typedef int boolean; typedef float real;
    typedef struct { float x,y,z; } real_point3d;
    #define NONE (-1)
    #define FALSE 0
    static int *game_engine, engine, player_data;
    static struct { struct { int teams; } universal_variant; } global_variant;
    struct player_datum { int unit_index,team_index; } players[2];
    struct data_iterator { int next; };
    static real_point3d origins[2];
    static struct player_datum *player_get(long i) { return &players[i]; }
    static void data_iterator_new(struct data_iterator *it,int data) { it->next=0; }
    static void *data_iterator_next(struct data_iterator *it) { return it->next<2 ? &players[it->next++] : 0; }
    static void object_get_origin(long i,real_point3d *p) { *p=origins[i]; }
    static real distance3d(real_point3d const *a,real_point3d const *b) {
        /* All test positions lie on the X axis; no host libm dependency. */
        real x=a->x-b->x; assert(a->y==b->y && a->z==b->z); return x<0 ? -x : x;
    }
    ''' + function(read('source/game/game_engine.c'), 'game_engine_get_distance_rating_for_spawn')
    case = '''int main(void) {
        real_point3d start={0,0,0}; game_engine=&engine;
        global_variant.universal_variant.teams=%d;
        players[0].unit_index=NONE; players[1].unit_index=1;
        /* Same-team occupant at the map start blocks even Team Slayer. */
        assert(game_engine_get_distance_rating_for_spawn(0,&start)==0);
        origins[1].x=6;
        assert(game_engine_get_distance_rating_for_spawn(0,&start)>0);
        /* Opposing players also exclude a two-world-unit radius. */
        players[1].team_index=1; origins[1].x=1.5f;
        assert(game_engine_get_distance_rating_for_spawn(0,&start)==0);
        origins[1].x=6;
        assert(game_engine_get_distance_rating_for_spawn(0,&start)>0);
        return 0;
    }''' % teams
    assert compile_run(tmp_path,world+case)==0
