#ifndef CUSTOM_EDITION_MENU_H
#define CUSTOM_EDITION_MENU_H

#include "cseries.h"

struct widget_instance;
struct network_game_server;
boolean custom_edition_menu_widget(struct widget_instance const *widget);
void custom_edition_menu_cancel_draft(void);
void custom_edition_menu_widget_deleted(struct widget_instance *widget);
void custom_edition_menu_server_dispose(struct network_game_server *server);
boolean pc_menu_new_game_apply(void);

#endif
