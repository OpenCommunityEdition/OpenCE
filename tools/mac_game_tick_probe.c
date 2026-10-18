/* Link census only: the genuine tick is retained, never called before its
   complete platform, map and gameplay lifecycle has been initialized. */
#include "cseries.h"
#include "game/game.h"
void (*volatile mac_full_game_tick)(void)=game_tick;
int main(void)
{
    printf("full game tick linked; execution not attempted\n");
    return mac_full_game_tick==NULL;
}
