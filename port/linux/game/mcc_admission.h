/* Bounded MCC file-validation holds, independent of permission to share files. */
#ifndef MCC_ADMISSION_H
#define MCC_ADMISSION_H
#include <stdint.h>
#define MCC_ADMISSION_MACHINES 128
#define MCC_ADMISSION_DEADLINE 120000u
#define MCC_ADMISSION_HEARD 20000u
#define MCC_ADMISSION_PULSE 5000u
#define MCC_ADMISSION_GRACE 15000u

enum mcc_admission_op {
    MCC_ADMISSION_BEGIN=1, MCC_ADMISSION_PING, MCC_ADMISSION_READY,
    MCC_ADMISSION_CANCEL, MCC_ADMISSION_ACK, MCC_ADMISSION_REFUSED
};
struct mcc_admission_identity {
    char name[128];
    unsigned char content[32],metadata[32];
    uint32_t checksum;
};
struct mcc_admission_message {
    uint32_t op,generation;
    struct mcc_admission_identity identity;
};
typedef char mcc_admission_message_size[sizeof(struct mcc_admission_message)==204 ? 1 : -1];
struct mcc_admission_env {
    void *user;
    int (*send)(void *,long,struct mcc_admission_message const *);
    int (*peer_valid)(void *,long);
    int (*peer_has_players)(void *,long);
};
struct mcc_admission_lease {
    uint32_t generation,started,heard,replied;
    int state;
};
struct mcc_admission {
    struct mcc_admission_env env;
    struct mcc_admission_identity host;
    uint32_t revision;
    struct mcc_admission_lease peers[MCC_ADMISSION_MACHINES];
    struct mcc_admission_message client;
    uint32_t next_generation,started,heard,sent;
    int state;
};
void mcc_admission_init(struct mcc_admission *,struct mcc_admission_env const *);
void mcc_admission_tick(struct mcc_admission *,uint32_t,
    struct mcc_admission_identity const *,uint32_t);
void mcc_admission_receive(struct mcc_admission *,long,void const *,unsigned,uint32_t);
void mcc_admission_forget(struct mcc_admission *,long);
int mcc_admission_begin(struct mcc_admission *,struct mcc_admission_identity const *,uint32_t);
void mcc_admission_finish(struct mcc_admission *,int,uint32_t);
int mcc_admission_status(struct mcc_admission const *);
int mcc_admission_active(struct mcc_admission const *,long,uint32_t,int);

/* The game adapter authenticates the reliable sender and owns session lifetime. */
struct network_game;
int mcc_admission_client_begin(struct network_game const *);
int mcc_admission_client_status(void);
int mcc_admission_client_pending(void);
void mcc_admission_client_finish(int);
/* Sharing takes over an existing hold; a stale callback cannot release a later one. */
uint32_t mcc_admission_client_token(void);
void mcc_admission_client_release(uint32_t);
void mcc_admission_client_clear(void);
void mcc_admission_update(void);
int mcc_admission_host_active(long);
int mcc_admission_host_needs_map(long);
void mcc_admission_disconnect(void);
void mcc_admission_peer_left(long);
int mcc_admission_handles_message(void const *,unsigned);
void mcc_admission_handle_message(long,void const *,unsigned,int);
#endif
