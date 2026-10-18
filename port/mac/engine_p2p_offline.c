/* Native builds have no internet-play tunnel. These entry points preserve the
   shared P2P API's unavailable/empty results; local system-link transport is
   implemented separately and is not routed through this provider. */
#include "p2p.h"
#include "monocypher.h"

#include <string.h>
#include <stdint.h>

static void clear_text(char *buffer,int size)
{
    if(buffer && size>0)buffer[0]=0;
}

void p2p_set_hosting_password(const char *password){(void)password;}
void p2p_set_hosting_public(int is_public){(void)is_public;}
void p2p_set_hosting_allowed(int allowed){(void)allowed;}
void p2p_set_game_player_counts(int count,int maximum){(void)count;(void)maximum;}
void p2p_set_game_listing(const char *name,const char *map,const char *gametype,
    int engine_type,int open,int in_progress,int has_teams)
{
    (void)name;(void)map;(void)gametype;(void)engine_type;
    (void)open;(void)in_progress;(void)has_teams;
}
void p2p_lobby_browse(int on){(void)on;}
void p2p_lobby_refresh(void){}
void p2p_lobby_mark_failed(const unsigned char *identifier){(void)identifier;}
int p2p_join_invite(const char *text){(void)text;return 0;}
int p2p_invite_link(char *link,int size){clear_text(link,size);return 0;}
int p2p_peer_address(const unsigned char *identifier,unsigned long *address)
{
    (void)identifier;if(address)*address=0;return 0;
}
unsigned long p2p_peer_endpoint_address(unsigned long address)
{(void)address;return 0;}
int p2p_peer_key_hash(unsigned long virtual_address,unsigned char *identifier,
    unsigned char *key_hash)
{
    (void)virtual_address;(void)identifier;(void)key_hash;return 0;
}
int p2p_peer_reached(const unsigned char *identifier,unsigned long *address)
{
    (void)identifier;if(address)*address=0;return 0;
}
int p2p_reach_player(const unsigned char *key_hash){(void)key_hash;return 0;}
void p2p_set_answering_players(int on){(void)on;}
int p2p_lobby_games(struct p2p_listing *games,int maximum_count)
{
    (void)games;(void)maximum_count;
    return 0;
}
int p2p_listing_unlock(struct p2p_listing *listing,const char *password)
{
    (void)password;
    return listing && !listing->locked;
}
void p2p_discord_sanitize(char *destination,int size,const char *source,int name)
{
    int length=0;
    if(!destination || size<=0)return;
    while(source && *source && length<size-1) {
        const char c=*source++;
        if((c>='0' && c<='9') ||
           (name && ((c>='a' && c<='z') || (c>='A' && c<='Z') ||
                     c=='_' || c=='.' || c=='-')))
            destination[length++]=c;
    }
    destination[length]=0;
}
void p2p_discord_identity(char *id,int id_size,char *name,int name_size)
{
    clear_text(id,id_size);clear_text(name,name_size);
}
void p2p_hardware_id(char *hex,int size){clear_text(hex,size);}
void p2p_hardware_id_sanitize(char *destination,int size,const char *source)
{
    int length=0;
    if(!destination || size<=0)return;
    while(source && *source && length<size-1 && length<2*P2P_HARDWARE_ID_BYTES) {
        char c=*source++;
        if(c>='A' && c<='F')c=(char)(c-'A'+'a');
        if((c>='0' && c<='9') || (c>='a' && c<='f'))
            destination[length++]=c;
    }
    destination[length]=0;
}
void p2p_blake2b_256(const void *data,unsigned long size,unsigned char *digest)
{
    if(!digest || (size && !data))return;
    crypto_blake2b(digest,32,(const uint8_t *)data,(size_t)size);
}
