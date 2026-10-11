/* Fixed-width guest/host protocol. UI is rendered by Halo using map fonts. */
#ifndef HALO_PORTING_UI_H
#define HALO_PORTING_UI_H
#define HALO_PORTING_ROWS 66 /* all 65 controls plus Back, without pagination */
struct halo_porting_row { char text[96]; int action, value; };
struct halo_porting_menu {
    int revision, page, count, editing;
    char title[96];
    struct halo_porting_row rows[HALO_PORTING_ROWS];
};
void host_porting_menu_read(struct halo_porting_menu *menu);
void host_porting_action(int revision, int action, int value);
void host_touch_ui_context(int menus, int context);
void host_touch_pointer_read(float *point);
void host_touch_frame(void);
/* the installed app's version ("v4.0 (4192)"), or "" before the app told it:
the main menu's build label shows it */
#define HALO_APP_VERSION_SIZE 48
void host_touch_app_version(char *buffer, unsigned int size);
#endif
