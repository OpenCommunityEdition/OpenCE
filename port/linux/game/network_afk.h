/*
NETWORK_AFK.H

A dedicated server's kick of a player away from the controls
(network_afk.c): what the distributed netcode's tick (network_distributed.c)
and the host's server (network_server_manager.c) ask of it.
*/

#ifndef __NETWORK_AFK_H
#define __NETWORK_AFK_H
#pragma once

/* (the host, each tick) the players' input seen, the machines away warned
and kicked */
void network_afk_host_tick(void);
/* a game starts: every machine counted afresh */
void network_afk_new_game(void);
/* (the host's server) a machine joined at the slot */
void network_afk_machine_joined(long machine_index);

#endif // __NETWORK_AFK_H
