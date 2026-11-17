#include "mcc_admission.h"
#include <string.h>

enum { LEASE_PENDING=1, LEASE_READY, LEASE_CLOSED };
enum { CLIENT_WAITING=1, CLIENT_CHECKING, CLIENT_FINISHING };
static int elapsed(uint32_t now,uint32_t then,uint32_t span) { return now-then>=span; }

void mcc_admission_init(struct mcc_admission *state,struct mcc_admission_env const *env)
{
    memset(state,0,sizeof(*state));state->env=*env;
}

int mcc_admission_begin(struct mcc_admission *state,
    struct mcc_admission_identity const *identity,uint32_t now)
{
    if (!identity || !memchr(identity->name,0,64)) return 0;
    if (!++state->next_generation) ++state->next_generation;
    state->client.op=MCC_ADMISSION_BEGIN;
    state->client.generation=state->next_generation;
    state->client.identity=*identity;
    state->started=state->heard=now;state->sent=now-MCC_ADMISSION_PULSE;
    state->state=CLIENT_WAITING;
    if (state->env.send(state->env.user,-1,&state->client)) state->sent=now;
    return 1;
}

void mcc_admission_finish(struct mcc_admission *state,int ready,uint32_t now)
{
    if (state->state<=0) return;
    state->client.op=ready ? MCC_ADMISSION_READY : MCC_ADMISSION_CANCEL;
    state->state=CLIENT_FINISHING;
    if (state->env.send(state->env.user,-1,&state->client)) state->state=0;
    state->sent=now;
}

int mcc_admission_status(struct mcc_admission const *state)
{
    return state->state<0 ? -1 : state->state==CLIENT_CHECKING ? 1 : 0;
}

int mcc_admission_active(struct mcc_admission const *state,long machine,uint32_t now,int needs_map)
{
    struct mcc_admission_lease const *lease;
    if (!state->revision || machine<0 || machine>=MCC_ADMISSION_MACHINES) return 0;
    lease=&state->peers[machine];
    if (lease->state==LEASE_PENDING) return !elapsed(now,lease->started,MCC_ADMISSION_DEADLINE) &&
        !elapsed(now,lease->heard,MCC_ADMISSION_HEARD);
    return !needs_map && lease->state==LEASE_READY && !elapsed(now,lease->heard,MCC_ADMISSION_GRACE);
}

void mcc_admission_forget(struct mcc_admission *state,long machine)
{
    if (machine>=0 && machine<MCC_ADMISSION_MACHINES) memset(&state->peers[machine],0,sizeof(state->peers[machine]));
}

void mcc_admission_tick(struct mcc_admission *state,uint32_t now,
    struct mcc_admission_identity const *host,uint32_t revision)
{
    int i;
    if (!host) revision=0;
    if (revision!=state->revision || (host && memcmp(host,&state->host,sizeof(*host)))) {
        memset(state->peers,0,sizeof(state->peers));
        if (host) state->host=*host;
        else memset(&state->host,0,sizeof(state->host));
        state->revision=revision;
    }
    for (i=0;i<MCC_ADMISSION_MACHINES;i++) {
        struct mcc_admission_lease *lease=&state->peers[i];
        if (!lease->state) continue;
        if (!state->env.peer_valid(state->env.user,i)) {mcc_admission_forget(state,i);continue;}
        if ((lease->state==LEASE_PENDING && !mcc_admission_active(state,i,now,1)) ||
            (lease->state==LEASE_READY && (!mcc_admission_active(state,i,now,0) ||
             state->env.peer_has_players(state->env.user,i)))) lease->state=LEASE_CLOSED;
    }
    if (state->state<=0) return;
    if (elapsed(now,state->started,MCC_ADMISSION_DEADLINE) ||
        (state->state==CLIENT_CHECKING && elapsed(now,state->heard,MCC_ADMISSION_HEARD))) {
        state->state=-1;return;
    }
    if (state->state==CLIENT_FINISHING) {
        if (state->env.send(state->env.user,-1,&state->client)) state->state=0;
    } else if (elapsed(now,state->sent,MCC_ADMISSION_PULSE)) {
        state->client.op=state->state==CLIENT_WAITING ? MCC_ADMISSION_BEGIN : MCC_ADMISSION_PING;
        if (state->env.send(state->env.user,-1,&state->client)) state->sent=now;
    }
}

void mcc_admission_receive(struct mcc_admission *state,long machine,void const *bytes,
    unsigned size,uint32_t now)
{
    struct mcc_admission_message message,reply;
    struct mcc_admission_lease *lease;
    int accepted=0;
    if (!bytes || size!=sizeof(message)) return;
    memcpy(&message,bytes,sizeof(message));
    if (!message.generation || !memchr(message.identity.name,0,64)) return;
    if (machine==-1) {
        if ((state->state!=CLIENT_WAITING && state->state!=CLIENT_CHECKING) ||
            message.generation!=state->client.generation ||
            memcmp(&message.identity,&state->client.identity,sizeof(message.identity))) return;
        if (message.op==MCC_ADMISSION_ACK) {state->state=CLIENT_CHECKING;state->heard=now;}
        else if (message.op==MCC_ADMISSION_REFUSED) state->state=-1;
        return;
    }
    if (machine<0 || machine>=MCC_ADMISSION_MACHINES || !state->revision ||
        !state->env.peer_valid(state->env.user,machine) ||
        memcmp(&message.identity,&state->host,sizeof(state->host)) ||
        message.op<MCC_ADMISSION_BEGIN || message.op>MCC_ADMISSION_CANCEL) return;
    lease=&state->peers[machine];
    if (message.op==MCC_ADMISSION_BEGIN && !lease->state) {
        lease->state=LEASE_PENDING;lease->generation=message.generation;
        lease->started=lease->heard=now;lease->replied=now-1000u;
    }
    if (lease->generation==message.generation && lease->state==LEASE_PENDING &&
        mcc_admission_active(state,machine,now,1)) {
        if (message.op==MCC_ADMISSION_CANCEL) {lease->state=LEASE_CLOSED;return;}
        if (message.op==MCC_ADMISSION_READY) {lease->state=LEASE_READY;lease->heard=now;return;}
        lease->heard=now;accepted=1;
    }
    /* A closed/expired lease remains a tombstone until the peer leaves or the
     * host selects a new identity. Neither retries nor heartbeats buy more time. */
    if (!elapsed(now,lease->replied,1000u)) return;
    reply=message;reply.op=accepted ? MCC_ADMISSION_ACK : MCC_ADMISSION_REFUSED;
    if (state->env.send(state->env.user,machine,&reply)) lease->replied=now;
}
