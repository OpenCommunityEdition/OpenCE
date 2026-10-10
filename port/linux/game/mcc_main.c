/* MCC transitions use the native menu lifecycle, without selecting Xbox
 * campaign progression or its credits movie for a standalone MCC scenario. */
#include "cseries.h"
#include "errors.h"
#include "main/main.h"
#include "interface/ui_widget.h"
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
