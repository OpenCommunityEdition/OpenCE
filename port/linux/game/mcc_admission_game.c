/* MCC admission's transport and session hooks; file validation lives in identity. */
#include "cseries.h"
#include "bungie_net/common/message_header.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "network_distributed.h"
#include "mcc_admission.h"
#include "mcc_identity.h"
#include "mcc_cache.h"
#include <string.h>

boolean network_distributed_client_send_reliably(void *,word);
boolean network_distributed_server_send_to_machine_reliably(long,void *,word);
unsigned long system_milliseconds(void);
struct network_game_server_client_machine *network_game_server_get_client_machine_at_index(struct network_game_server *,long);
boolean network_game_server_client_machine_is_joined_to_game(struct network_game_server *,struct network_game_server_client_machine *);

static struct mcc_admission admission;
static int initialized;

static int send_message(void *unused,long machine,struct mcc_admission_message const *body)
{
    struct { struct distributed_message_header header;struct mcc_admission_message body; } message;
    (void)unused;
    memset(&message,0,sizeof(message));
    build_message_header(&message.header.header,sizeof(message),2,0);
    message.header.type=_distributed_message_mcc_admission;
    message.header.count=1;message.body=*body;
    return machine<0 ? network_distributed_client_send_reliably(&message,sizeof(message)) :
        network_distributed_server_send_to_machine_reliably(machine,&message,sizeof(message));
}

static int peer_valid(void *unused,long index)
{
    struct network_game_server *server=global_network_game_server_get();
    struct network_game_server_client_machine *machine;
    struct network_game *game=network_game_get_game();
    (void)unused;
    if (!server || !game || index<0 || index>=MCC_ADMISSION_MACHINES || !mcc_level_name(game->map.name)) return 0;
    machine=network_game_server_get_client_machine_at_index(server,index);
    return network_game_server_client_machine_is_joined_to_game(server,machine);
}

static int peer_has_players(void *unused,long index)
{
    struct network_game *game=network_game_get_game();
    unsigned i;
    (void)unused;
    if (!game) return 0;
    for (i=0;i<NUMBEROF(game->players);i++)
        if (network_player_is_valid(&game->players[i]) && game->players[i].machine_index==index) return 1;
    return 0;
}

static void initialize(void)
{
    struct mcc_admission_env env;
    if (initialized) return;
    memset(&env,0,sizeof(env));env.send=send_message;env.peer_valid=peer_valid;env.peer_has_players=peer_has_players;
    mcc_admission_init(&admission,&env);initialized=1;
}

static void identity(struct network_game const *game,struct mcc_admission_identity *out)
{
    memcpy(out->name,game->map.name,128);
    memcpy(out->content,game->mcc_content_sha256,32);
    memcpy(out->metadata,game->mcc_metadata_sha256,32);
    out->checksum=(uint32_t)game->map.version;
}

int mcc_admission_client_begin(struct network_game const *game)
{
    struct mcc_admission_identity expected;
    initialize();identity(game,&expected);
    return mcc_admission_begin(&admission,&expected,(uint32_t)system_milliseconds());
}
int mcc_admission_client_status(void) { return initialized ? mcc_admission_status(&admission) : -1; }
int mcc_admission_client_pending(void) { return initialized && admission.state>0; }
void mcc_admission_client_finish(int ready)
{
    if (initialized) mcc_admission_finish(&admission,ready,(uint32_t)system_milliseconds());
}
uint32_t mcc_admission_client_token(void)
{
    return mcc_admission_client_pending() ? admission.client.generation : 0;
}
void mcc_admission_client_release(uint32_t token)
{
    if (token && token==mcc_admission_client_token()) mcc_admission_client_finish(0);
}
void mcc_admission_client_clear(void)
{
    if (!initialized) return;
    mcc_admission_finish(&admission,0,(uint32_t)system_milliseconds());
    admission.state=0;
}

void mcc_admission_update(void)
{
    struct network_game *game=network_game_get_game(),copy;
    struct mcc_admission_identity current,*host=NULL;
    uint32_t revision=0;
    initialize();
    if (global_network_game_server_get() && game && mcc_level_name(game->map.name)) {
        revision=(uint32_t)mcc_identity_host_revision();
        if (revision && revision==admission.revision &&
            !strcmp(game->map.name,admission.host.name) && (uint32_t)game->map.version==admission.host.checksum)
            host=&admission.host;
        else if (revision && mcc_identity_host_settings(game,sizeof(*game),&copy)) {
            identity(&copy,&current);host=&current;
        }
    }
    mcc_admission_tick(&admission,(uint32_t)system_milliseconds(),host,revision);
    if (admission.state>0) {
        struct network_game_client *client=global_network_game_client_get();
        game=client ? network_game_client_get_game(client) : NULL;
        if (!game) admission.state=-1;
        else {
            identity(game,&current);
            if (memcmp(&current,&admission.client.identity,sizeof(current))) admission.state=-1;
        }
    }
}

int mcc_admission_host_active(long machine)
{
    return initialized && mcc_admission_active(&admission,machine,(uint32_t)system_milliseconds(),0);
}
int mcc_admission_host_needs_map(long machine)
{
    return initialized && mcc_admission_active(&admission,machine,(uint32_t)system_milliseconds(),1);
}
void mcc_admission_disconnect(void) { memset(&admission,0,sizeof(admission));initialized=0; }
void mcc_admission_peer_left(long machine) { if (initialized) mcc_admission_forget(&admission,machine); }

int mcc_admission_handles_message(void const *message,unsigned size)
{
    struct distributed_message_header header;
    if (!message || size<sizeof(header)) return 0;
    memcpy(&header,message,sizeof(header));return header.type==_distributed_message_mcc_admission;
}

void mcc_admission_handle_message(long machine,void const *message,unsigned size,int reliable)
{
    struct distributed_message_header header;
    if (!reliable || !mcc_admission_handles_message(message,size) ||
        size!=sizeof(header)+sizeof(struct mcc_admission_message)) return;
    memcpy(&header,message,sizeof(header));
    if (header.count!=1 || GET_MESSAGE_FLAGS(header.header) ||
        GET_MESSAGE_TYPE(header.header)!=2 || GET_MESSAGE_SIZE(header.header)!=size) return;
    if (machine==NONE ? global_network_game_server_get()!=NULL : !peer_valid(NULL,machine)) return;
    /* Refresh host revision before accepting a message, even between frames. */
    mcc_admission_update();
    mcc_admission_receive(&admission,machine,(unsigned char const *)message+sizeof(header),
        size-(unsigned)sizeof(header),(uint32_t)system_milliseconds());
}
