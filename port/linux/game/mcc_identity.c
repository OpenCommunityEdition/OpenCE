/* Content-matched local MCC paths; the host's name stays on the wire. */
#include "cseries.h"
#include "errors.h"
#include "game/game.h"
#include "networking/network_game_manager.h"
#include "interface/ui_widget.h"
#include "mcc_cache.h"
#include "mcc_identity.h"
#include "mcc_maps.h"
#include "mcc_package.h"
#include "mcc_precache.h"
#include "mcc_validation.h"
#include <string.h>

#define MCC_IDENTITY_MAX_CANDIDATES 16
static struct {
    char name[64];
    unsigned char digest[32];
    struct mcc_precache_snapshot files;
    int valid;
} mcc_host_identity;
static struct {
    char session[64], local[64];
    unsigned char digest[32];
    struct mcc_precache_snapshot files;
} mcc_client_identity;

static int mcc_identity_error(char const *message)
{
    wchar_t text[256];
    unsigned i;
    error(_error_silent,"mcc identity: %s",message);
    for (i=0;message[i] && i<255;i++) text[i]=(unsigned char)message[i];
    text[i]=0;
    display_error_text_when_main_menu_loaded(text);
    return 0;
}

void mcc_identity_disconnect(void)
{
    mcc_validation_cancel();
    mcc_cache_validation_clear();
    memset(&mcc_client_identity,0,sizeof(mcc_client_identity));
    memset(&mcc_host_identity,0,sizeof(mcc_host_identity));
    mcc_cache_precache_clear();
}

char const *mcc_identity_local_name(char const *name)
{
    char canonical[64];
    if (mcc_client_identity.session[0] && mcc_package_name(name,canonical) &&
        !csstrcasecmp(canonical,mcc_client_identity.session)) return mcc_client_identity.local;
    return name;
}

static unsigned char const *mcc_identity_expected(char const *name)
{
    char canonical[64];
    if (!mcc_package_name(name,canonical)) return NULL;
    if (mcc_client_identity.session[0] &&
        (!csstrcasecmp(canonical,mcc_client_identity.session) ||
         !csstrcasecmp(canonical,mcc_client_identity.local))) return mcc_client_identity.digest;
    return NULL;
}

/* Prepare alongside the map version, before sending its settings. Initial
 * MCC host setup does this before opening the lobby; mcc_ui_network defers
 * early map changes until the host's loopback client has joined. */
int mcc_identity_prepare_host(char const *map)
{
    struct mcc_precache_snapshot before,after;
    char name[64];
    unsigned char digest[32];
    if (!mcc_package_name(map,name) || !mcc_precache_snapshot(name,&before)) {
        mcc_host_identity.valid=0;
        return 0;
    }
    if (!mcc_host_identity.valid || strcmp(name,mcc_host_identity.name) ||
        memcmp(&before,&mcc_host_identity.files,sizeof(before))) {
        mcc_host_identity.valid=0;
        if (mcc_validation_request(name,NULL,digest)!=1) return 0;
        if (!mcc_precache_snapshot(name,&after) ||
            memcmp(&before,&after,sizeof(before))) {
            mcc_identity_error("The host's MCC map or resources could not be fingerprinted, or changed during validation.");
            return 0;
        }
        strcpy(mcc_host_identity.name,name);
        memcpy(mcc_host_identity.digest,digest,32);
        mcc_host_identity.files=after;
        mcc_host_identity.valid=1;
    }
    return 1;
}

void const *mcc_identity_host_settings(void const *settings,long bytes,struct network_game *copy)
{
    struct network_game const *game=settings;
    struct mcc_precache_snapshot files;
    char canonical[64];
    uint32_t checksum;
    if (!game || bytes!=sizeof(*game)) return NULL;
    memcpy(copy,game,sizeof(*copy));
    memset(copy->mcc_content_sha256,0,32);
    if (!mcc_level_name(game->map.name)) {
        mcc_host_identity.valid=0;
        return copy;
    }
    /* Recheck every resource candidate before reusing the prepared digest.
     * A changed installation still requires fresh validation. */
    /* Sending or tearing down a session must never initiate package work.
     * Only pregame preparation may establish the host's identity. */
    if (!mcc_host_identity.valid || !mcc_package_name(game->map.name,canonical) ||
        strcmp(canonical,mcc_host_identity.name) || !mcc_precache_snapshot(canonical,&files) ||
        memcmp(&files,&mcc_host_identity.files,sizeof(files)) ||
        !mcc_validation_lookup(canonical,&checksum,NULL) ||
        checksum!=(uint32_t)game->map.version) return NULL;
    memcpy(copy->mcc_content_sha256,mcc_host_identity.digest,32);
    return copy;
}

static int mcc_identity_matches(char const *name,unsigned char const digest[32],
    struct mcc_precache_snapshot *validated)
{
    struct mcc_precache_snapshot before,after;
    unsigned char actual[32];
    int ok;
    if (!mcc_precache_snapshot(name,&before)) return 0;
    /* The host's loopback client admits the very package just fingerprinted
     * for these settings. Rehashing it here stalls remote join requests.
     * Reuse only the exact identity and unchanged complete file snapshot;
     * actual loading still fingerprints the opened content independently. */
    if (mcc_host_identity.valid && !strcmp(name,mcc_host_identity.name) &&
        !memcmp(digest,mcc_host_identity.digest,32) &&
        !memcmp(&before,&mcc_host_identity.files,sizeof(before))) {
        if (validated) *validated=before;
        return 1;
    }
    ok=mcc_content_probe(name,actual) &&
        !memcmp(actual,digest,32) && mcc_precache_snapshot(name,&after) &&
        !memcmp(&before,&after,sizeof(before));
    if (ok && validated) *validated=after;
    return ok;
}

int mcc_identity_accept(struct network_game const *game)
{
    char session[64],selected[64];
    char const *leaf,*candidate,*candidate_leaf;
    unsigned char zero[32]={0};
    short i,count;
    unsigned candidates=0,matches=0;
    struct mcc_precache_snapshot selected_files;
    memset(&mcc_client_identity,0,sizeof(mcc_client_identity));
    mcc_cache_precache_clear();
    if (!mcc_level_name(game->map.name)) return !memcmp(game->mcc_content_sha256,zero,32);
    if (!mcc_package_name(game->map.name,session) || !memcmp(game->mcc_content_sha256,zero,32))
        return mcc_identity_error("The host did not supply a valid MCC content identity.");
    /* A verified exact installation is unambiguous even with duplicate copies. */
    if (mcc_identity_matches(session,game->mcc_content_sha256,&selected_files)) {
        strcpy(selected,session);
        matches=1;
    } else {
        leaf=strrchr(session,'\\')+1;
        mcc_maps_rescan();
        count=mcc_maps_count(FALSE);
        /* Bound work before hashing any fallback candidates. */
        for (i=0;i<count;i++) {
            candidate=mcc_maps_level_name(i);
            candidate_leaf=strrchr(candidate,'\\')+1;
            if (!csstrcasecmp(leaf,candidate_leaf) && ++candidates>MCC_IDENTITY_MAX_CANDIDATES)
                return mcc_identity_error("Too many MCC maps share this filename. Keep one matching package installed and retry.");
        }
        for (i=0;i<count;i++) {
            candidate=mcc_maps_level_name(i);
            candidate_leaf=strrchr(candidate,'\\')+1;
            if (csstrcasecmp(leaf,candidate_leaf) || !csstrcasecmp(candidate,session)) continue;
            if (mcc_identity_matches(candidate,game->mcc_content_sha256,&selected_files)) {
                strcpy(selected,candidate);
                if (++matches>1) return mcc_identity_error("Several local MCC packages match the host. Keep one matching copy, or use the host's package folder name.");
            }
        }
    }
    if (!matches) return mcc_identity_error("No local MCC map and required resources match the host's SHA-256 fingerprint. Install the same complete package; its folder name may differ.");
    mcc_client_identity.files=selected_files;
    strcpy(mcc_client_identity.session,session);
    strcpy(mcc_client_identity.local,selected);
    memcpy(mcc_client_identity.digest,game->mcc_content_sha256,32);
    error(_error_silent,"mcc identity: %s resolved to %s (SHA-256 matched)",session,selected);
    return 1;
}

int mcc_identity_check(char const *name)
{
    unsigned char const *expected=mcc_identity_expected(name);
    struct mcc_precache_snapshot files;
    if (!expected) return 1;
    if (!mcc_precache_snapshot(mcc_client_identity.local,&files)) return 0;
    if (!memcmp(&files,&mcc_client_identity.files,sizeof(files))) return 1;
    return mcc_identity_matches(mcc_client_identity.local,expected,NULL);
}

int mcc_identity_loaded(char const *name,struct mcc_runtime *runtime)
{
    unsigned char actual[32];
    unsigned char const *expected=mcc_identity_expected(name);
    if (!expected) return game_connection()!=_game_connection_network_client &&
        game_connection()!=_game_connection_network_server;
    return mcc_resources_fingerprint(runtime,actual) && !memcmp(actual,expected,32);
}
