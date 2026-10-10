/* Local MCC load failures recover through the native UI lifecycle. The
 * failed map has already released its tags; no game is initialized for it. */
#include "cseries.h"
#include "errors.h"
#include "main/main.h"
#include "interface/ui_widget.h"
#include "mcc_cache.h"
#include "mcc_main.h"

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
