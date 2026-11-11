#include "mcc_load_network.h"
#include <string.h>
void mcc_load_network_bind(struct mcc_load_network *state,
                           struct mcc_admission_identity const *identity, uint32_t round,
                           uint32_t now)
{
    if (state->active && state->round == round &&
        !memcmp(&state->identity, identity, sizeof(*identity)))
        return;
    memset(state, 0, sizeof(*state));
    state->identity = *identity;
    state->round = round;
    state->started = now;
    state->active = 1;
}
int mcc_load_network_active(struct mcc_load_network *state, long index, uint32_t now)
{
    struct mcc_load_peer *peer;
    if (!state->active || index < 0 || index >= MCC_ADMISSION_MACHINES)
        return 0;
    peer = &state->peers[index];
    if (peer->state == 1 && ((uint32_t)(now - state->started) >= MCC_LOAD_NETWORK_LIMIT ||
                             (uint32_t)(now - peer->advanced) >= MCC_LOAD_NETWORK_IDLE))
        peer->state = 2;
    return peer->state == 1;
}
int mcc_load_network_receive(struct mcc_load_network *state, long index,
                             struct mcc_load_message const *message, uint32_t now)
{
    struct mcc_load_peer *peer;
    if (!state->active || index < 0 || index >= MCC_ADMISSION_MACHINES || !message->sequence ||
        message->round != state->round ||
        memcmp(&message->identity, &state->identity, sizeof(state->identity)) ||
        (uint32_t)(now - state->started) >= MCC_LOAD_NETWORK_LIMIT)
        return 0;
    peer = &state->peers[index];
    mcc_load_network_active(state, index, now);
    if (peer->state == 2 || message->sequence <= peer->sequence ||
        (peer->state == 1 && (uint32_t)(now - peer->advanced) < MCC_LOAD_NETWORK_PULSE))
        return 0;
    peer->state = 1;
    peer->sequence = message->sequence;
    peer->advanced = now;
    return 1;
}
void mcc_load_network_forget(struct mcc_load_network *state, long index)
{
    if (index >= 0 && index < MCC_ADMISSION_MACHINES)
        state->peers[index].state = 2;
}
int mcc_load_allowance_hold(struct mcc_load_allowance *allowance, uint32_t now)
{
    uint32_t elapsed;
    if (!allowance->known) {
        allowance->known = 1;
        allowance->left = MCC_LOAD_NETWORK_LIMIT;
        allowance->at = now;
    }
    elapsed = (uint32_t)(now - allowance->at);
    allowance->at = now;
    if (elapsed > MCC_LOAD_NETWORK_HOLD_GAP) {
        elapsed /= MCC_LOAD_NETWORK_EARN;
        allowance->left = elapsed >= MCC_LOAD_NETWORK_LIMIT - allowance->left ?
            MCC_LOAD_NETWORK_LIMIT : allowance->left + elapsed;
    } else
        allowance->left = elapsed >= allowance->left ? 0 : allowance->left - elapsed;
    return allowance->left != 0;
}
void mcc_load_allowance_forget(struct mcc_load_allowance *allowance)
{
    memset(allowance, 0, sizeof(*allowance));
}
