#ifndef NETWORK_TEST_H
#define NETWORK_TEST_H

/* Opt-in diagnostics; inactive unless debug.network_test is configured. */
void network_test_verdict_log(char const *reason, long killer, long victim, boolean friendly);
void network_test_spawn_inventory_log(long player_index, char const *stage, long flags);

#endif
