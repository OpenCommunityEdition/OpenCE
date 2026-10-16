/* MCC transitions use the native menu lifecycle, without selecting Xbox
 * campaign progression or its credits movie for a standalone MCC scenario. */
#include "cseries.h"
#include "errors.h"
#include "main/main.h"
#include "interface/ui_widget.h"
#include "networking/network_game_globals.h"
#include "mcc_cache.h"
#include "mcc_main.h"

boolean mcc_main_map_completed(void)
{
    if (!mcc_cache_tags_loaded()) return FALSE;
    /* Unknown campaign levels reach main_roll_credits. MCC packages have
     * no native next-level mapping: return normally instead of starting the
     * Xbox outro immediately after menu precaching. That movie borrows and
     * protects texture-cache memory while menu reads may still be pending. */
    error(_error_silent, "mcc: scenario completed; returning to the menu (select the next campaign map manually)");
    main_goto_main_menu();
    return TRUE;
}

boolean mcc_main_load_failed(char const *name)
{
    if (!mcc_level_name(name)) return FALSE;
    error(_error_silent, "mcc: returning to the menu after failed load of %s", name);
    display_error_text_when_main_menu_loaded(
        L"The MCC map could not be loaded. See debug.txt for the failing stage.");
    errors_clear();
    main_goto_main_menu();
    main_menu_load();
    return TRUE;
}

boolean mcc_main_network_load_failed(char const *name)
{
    if (!mcc_level_name(name)) return FALSE;
    error(_error_silent, "mcc: ending the network session after failed load of %s", name);
    display_error_text_when_main_menu_loaded(
        L"The MCC map could not be loaded. See debug.txt for the failing stage.");
    errors_clear();
    /* This runs inside a network receive callback. Queue both operations:
     * the next frame needs the menu's globals before input_abstraction_update,
     * which runs before the normal network abort is processed. Do not free
     * the connection or load the menu on this receive stack. */
    network_game_abort();
    main_goto_main_menu();
    return TRUE;
}
