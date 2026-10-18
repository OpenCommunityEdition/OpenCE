/* Whole engine game-time unit with a real scoped globals record. This isolated
   diagnostic does not initialize the unrelated game-state arena or simulation. */
#include "../source/game/game_time.c"
static struct game_time_globals_struct constructor_time_storage;
int mac_constructor_time_start(long ticks)
{
    if(game_time_globals)return 0;
    game_time_globals=&constructor_time_storage;
    game_time_initialize_for_new_map();game_time_set_distributed(ticks);return 1;
}
void mac_constructor_time_stop(void)
{
    game_time_dispose_from_old_map();game_time_globals=NULL;
    csmemset(&constructor_time_storage,0,sizeof(constructor_time_storage));
}
