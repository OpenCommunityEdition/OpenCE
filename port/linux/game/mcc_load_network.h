/* MCC conversion progress does not advance simulation or complete a load. */
#ifndef MCC_LOAD_NETWORK_H
#define MCC_LOAD_NETWORK_H
#include "mcc_admission.h"
#define MCC_LOAD_NETWORK_PULSE 5000u
#define MCC_LOAD_NETWORK_IDLE 30000u
/* Measured supported browser conversion takes about 120 seconds. Twice that budget
 * bounds slower progressing peers; repeated pulses never restart it. */
#define MCC_LOAD_NETWORK_LIMIT 240000u
/* Nothing in a pulse proves conversion, so a machine that only pulses could
 * hold every round's start for the whole limit. Each machine has an allowance
 * of holding, the limit at most: holding a start beyond the ordinary wait
 * spends it, and time not holding earns it back at a third of the rate. One
 * pulsing without converting holds at most a quarter of the time; one that
 * converts for two minutes a round has them back in six. Calls closer together
 * than the gap are one hold. */
#define MCC_LOAD_NETWORK_EARN 3u
#define MCC_LOAD_NETWORK_HOLD_GAP 1000u
struct mcc_load_message {
    uint32_t round, sequence;
    struct mcc_admission_identity identity;
};
typedef char mcc_load_message_size[sizeof(struct mcc_load_message) == 204 ? 1 : -1];
struct mcc_load_peer {
    uint32_t sequence, advanced;
    int state;
};
struct mcc_load_allowance {
    uint32_t left, at;
    int known;
};
struct mcc_load_network {
    struct mcc_admission_identity identity;
    uint32_t round, started;
    int active;
    struct mcc_load_peer peers[MCC_ADMISSION_MACHINES];
};
void mcc_load_network_bind(struct mcc_load_network *, struct mcc_admission_identity const *,
                           uint32_t, uint32_t);
int mcc_load_network_receive(struct mcc_load_network *, long, struct mcc_load_message const *,
                             uint32_t);
int mcc_load_network_active(struct mcc_load_network *, long, uint32_t);
void mcc_load_network_forget(struct mcc_load_network *, long);
/* Whether a machine may go on holding the start (spending its allowance), or
 * earn its allowance back after a gap between holds; forget it for a new machine. */
int mcc_load_allowance_hold(struct mcc_load_allowance *, uint32_t);
void mcc_load_allowance_forget(struct mcc_load_allowance *);
struct network_game;
void mcc_load_network_start(struct network_game const *);
void mcc_load_network_begin(char const *);
void mcc_load_network_pulse(void);
void mcc_load_network_work(int, unsigned long, unsigned long);
void mcc_load_network_end(void);
void mcc_load_network_disconnect(void);
void mcc_load_network_peer_left(long);
/* a machine left the game: the next in its slot has a whole allowance */
void mcc_load_network_machine_left(long);
int mcc_load_network_host_waiting(long);
int mcc_load_network_client_expired(void);
int mcc_load_network_handles_message(void const *, unsigned);
void mcc_load_network_handle_message(long, void const *, unsigned, int);
#endif
