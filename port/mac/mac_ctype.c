/* GCC 4 expands the system isspace macro in hs.c's legacy function
   declaration. Give that source an ordinary typed C-library entry point. */
#include <ctype.h>

int mac_game_tick_isspace(int character)
{
    return isspace(character);
}
