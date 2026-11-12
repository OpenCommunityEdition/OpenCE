#ifndef MCC_UI_TEAMS_H
#define MCC_UI_TEAMS_H
struct network_game_server;
struct network_game_server_client_machine;
boolean mcc_ui_team_balance_allows(short machine, short controller, short team);
void mcc_ui_teams_reset(void);
/* a living player's unit killed for a team change: a death no one is credited
   with (no suicide scored), after which the player respawns on the new team */
void mcc_ui_team_change_kill(long player_index, long unit_index);
void mcc_ui_team_respawn_damage(long player_index, long unit_index, boolean active);
void mcc_ui_team_respawn_death(long *killer, long *object, long dead, boolean *friendly);
boolean mcc_ui_team_request(struct network_game_server *server,
    struct network_game_server_client_machine *machine, word *message, short size);
#endif
