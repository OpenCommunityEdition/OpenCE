#include "p2p.h"
#include <string.h>
#include <stdio.h>

static int checks,failures;
#define CHECK(x) do { ++checks; if(!(x)) { ++failures; fprintf(stderr,"check %d failed: %s\n",checks,#x); } } while(0)

int main(void)
{
    static const unsigned char empty_blake2b_256[32]={
        0x0e,0x57,0x51,0xc0,0x26,0xe5,0x43,0xb2,0xe8,0xab,0x2e,0xb0,0x60,0x99,0xda,0xa1,
        0xd1,0xe5,0xdf,0x47,0x77,0x8f,0x77,0x87,0xfa,0xab,0x45,0xcd,0xf1,0x2f,0xe3,0xa8};
    unsigned char digest[32],identifier[6]={1,2,3,4,5,6};
    unsigned long address=0x01020304UL;
    struct p2p_listing listing;
    char text[80];
    memset(&listing,0,sizeof(listing));memset(digest,0,sizeof(digest));
    strcpy(text,"not empty");
    CHECK(!p2p_join_invite("halo://join/example"));
    CHECK(!p2p_invite_link(text,sizeof(text))&&text[0]==0);
    CHECK(!p2p_peer_address(identifier,&address)&&address==0);
    CHECK(!p2p_peer_endpoint_address(0x01020304UL));
    CHECK(!p2p_lobby_games(&listing,1));
    listing.locked=1;CHECK(!p2p_listing_unlock(&listing,"wrong"));
    listing.locked=0;CHECK(p2p_listing_unlock(&listing,""));
    p2p_hardware_id_sanitize(text,sizeof(text),"ABcd-0123456789abcdef0123456789abcdef");
    CHECK(!strcmp(text,"abcd0123456789abcdef0123456789ab"));
    p2p_discord_sanitize(text,sizeof(text),"A user-42!",1);
    CHECK(!strcmp(text,"Auser-42"));
    p2p_discord_identity(text,sizeof(text),text+40,sizeof(text)-40);
    CHECK(text[0]==0&&text[40]==0);
    p2p_hardware_id(text,sizeof(text));CHECK(text[0]==0);
    p2p_blake2b_256(NULL,0,digest);
    CHECK(!memcmp(digest,empty_blake2b_256,sizeof(digest)));
    printf("native offline P2P boundary: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
