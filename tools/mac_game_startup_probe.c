/* Full initialization link census. The entry point is retained for linking,
   but execution requires an explicit initialized-platform lifecycle driver. */
#include "cseries.h"
#include "game/game.h"
void (*volatile mac_full_game_initialize)(void)=game_initialize;
int main(void)
{
    printf("full game initialization linked; execution not attempted\n");
    return mac_full_game_initialize==NULL;
}
