/* Keep the private globals type and all engine functions in the same
   translation unit, so the original layout witnesses cover native storage. */
#include "../../source/game/game_engine.c"

struct game_engine_globals game_engine_globals;
