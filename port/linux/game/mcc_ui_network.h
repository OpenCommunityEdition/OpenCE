#ifndef MCC_UI_NETWORK_H
#define MCC_UI_NETWORK_H

struct network_game_server;
boolean mcc_ui_restart_network_game(void);
boolean mcc_ui_network_restart_settings(struct network_game_server *server);
boolean mcc_ui_network_restart_pregame(void);
void mcc_ui_network_server_dispose(struct network_game_server *server);

#endif
