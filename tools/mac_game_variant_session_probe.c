/* Called only after a real game_initialize inside a bound session scope. */
#include "port/mac/engine_game_variant.h"
#include "game/game_engine.h"

/* 0 means provider/API failure; 1 means genuine map state is not ready; 2
   means one real game_engine_update completed. */
int mac_engine_game_variant_session_probe(const char **blocker)
{
    const char *error = NULL;

    if (blocker)
        *blocker = NULL;
    if (!mac_engine_game_variant_select_slayer(&error)) {
        if (blocker)
            *blocker = error;
        return 0;
    }
    if (!game_engine_get_variant() ||
        game_engine_get_variant()->universal_variant.score_to_win != 15) {
        if (blocker)
            *blocker = "stock Slayer preset did not retain its 15-point target";
        return 0;
    }
    if (!mac_engine_game_variant_initialize_map(&error)) {
        if (blocker)
            *blocker = error;
        return 1;
    }
    if (!mac_engine_game_variant_update(&error)) {
        if (blocker)
            *blocker = error;
        return 0;
    }
    return 2;
}
