#ifndef MCC_UI_H
#define MCC_UI_H

struct widget_instance;
struct event_record;

/* Positive signed-short callback IDs reserved for generated MCC pause tags. */
enum {
    MCC_PAUSE_ACTION_RESUME = 0x7000,
    MCC_PAUSE_ACTION_REVERT,
    MCC_PAUSE_ACTION_RESTART,
    MCC_PAUSE_ACTION_SAVE,
    MCC_PAUSE_ACTION_QUIT,
    MCC_PAUSE_ACTION_END_GAME,
    MCC_PAUSE_ACTION_RED_TEAM,
    MCC_PAUSE_ACTION_BLUE_TEAM,
    MCC_PAUSE_ACTION_SETTINGS
};

boolean mcc_ui_trusted_action(struct widget_instance *widget, word function);

/* TRUE means this MCC-owned callback supplied result. FALSE leaves the
 * established callback dispatcher entirely responsible for the widget. */
boolean mcc_ui_event_function(struct widget_instance *widget,
    struct event_record *event, word function, boolean *deleted, boolean *result);
boolean mcc_ui_settings_needed(char const *map_name);
/* Native profile-clear and widget-close lifecycle notifications. These
 * affect only the profile editor opened by the MCC action adapter. */
void mcc_ui_settings_profile_released(void);
void mcc_ui_settings_close(short controller);
/* Handles only the MCC-owned multiplayer New Game button. The caller must
 * finish dispatch immediately when handled, including a host-only denial.
 * A successful transition synchronously deletes the active widget tree. */
boolean mcc_ui_new_game(struct widget_instance *widget, boolean *deleted);

#endif
