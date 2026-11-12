/* Content-matched local MCC paths; the host's name stays on the wire. */
#include "cseries.h"
#include "errors.h"
#include "game/game.h"
#include "networking/network_game_manager.h"
#include "networking/network_game_globals.h"
#include "interface/ui_widget.h"
#include "mcc_cache.h"
#include "mcc_identity.h"
#include "mcc_maps.h"
#include "mcc_package.h"
#include "mcc_precache.h"
#include "mcc_validation.h"
#include "mcc_workshop.h"
#include "mcc_share.h"
#include "mcc_admission.h"
#include "../src/mcc_manifest.h"
#include "../src/mcc_share_metadata.h"
#include <string.h>

#define MCC_IDENTITY_MAX_CANDIDATES 16
static struct {
    char name[64];
    unsigned char digest[32], metadata[32];
    struct mcc_precache_snapshot files;
    struct mcc_workshop_item item;
    int valid;
} mcc_host_identity;
static struct {
    char session[64], local[64];
    unsigned char digest[32],metadata[32];
    struct mcc_precache_snapshot files;
} mcc_client_identity;
static unsigned long mcc_host_revision;
static struct {
    char wire[128],session[64],candidates[MCC_IDENTITY_MAX_CANDIDATES+1][64],selected[64];
    unsigned char digest[32],metadata[32];
    uint32_t checksum;
    unsigned count,index,matches;
    int active,scanned,finishing;
    struct mcc_validation_check *check;
    struct mcc_validation_proof selected_proof;
} mcc_identity_job;

static void mcc_identity_job_clear(void)
{
    mcc_validation_check_end(mcc_identity_job.check);
    memset(&mcc_identity_job,0,sizeof(mcc_identity_job));
}


/* MCC package sharing (mcc_share.c) may offer to fetch a package that matches nothing
 * here, instead of the join being refused. It registers itself; the identity
 * rules below stay the only judge of what is installed and matches. */
static int (*mcc_identity_missing)(char const name[128],unsigned char const digest[32],unsigned char const metadata[32]);

void mcc_identity_set_missing(int (*handler)(char const name[128],unsigned char const digest[32],unsigned char const metadata[32]))
{
    mcc_identity_missing=handler;
}

/* Required package metadata has its own identity: it does not change the map's
 * gameplay fingerprint, and invalid optional artwork cannot block admission. */
static int mcc_identity_metadata(char const *name, unsigned char digest[32])
{
    char root[1024],reason[128];
    char const *leaf=strrchr(name,'\\');
    memset(digest,0,32);
    if (!leaf || !strchr(name+9,'\\')) return 1;
    return mcc_manifest_package_root(name,root) &&
        mcc_share_metadata_digest(root,-1,leaf+1,digest,reason);
}

static int mcc_identity_origin_matches(char const *name)
{
    struct mcc_workshop_item item;
    mcc_workshop_local_item(name,&item);
    return !memcmp(&item,&mcc_host_identity.item,sizeof(item));
}

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
    mcc_identity_job_clear();
    mcc_admission_disconnect();
    mcc_share_game_disconnect();
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
    struct mcc_workshop_item item;
    char name[64];
    unsigned char digest[32],metadata[32];
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
        if (!++mcc_host_revision) ++mcc_host_revision;
    }
    if (!mcc_identity_metadata(name,metadata)) {
        mcc_host_identity.valid=0;
        return mcc_identity_error("The host package metadata is invalid.");
    }
    /* Advisory origin is outside both content identities, but an offer must
     * not mix a cached wire ID with a newly read package sidecar. The existing
     * revision hook cancels offers prepared under the previous attribution. */
    mcc_workshop_local_item(name,&item);
    if (memcmp(metadata,mcc_host_identity.metadata,32) || memcmp(&item,&mcc_host_identity.item,sizeof(item))) {
        memcpy(mcc_host_identity.metadata,metadata,32);
        mcc_host_identity.item=item;
        if (!++mcc_host_revision) ++mcc_host_revision;
    }
    return 1;
}

unsigned long mcc_identity_host_revision(void)
{
    return mcc_host_identity.valid ? mcc_host_revision : 0;
}

int mcc_identity_host_package(char const *level,unsigned char digest[32])
{
    char canonical[64];
    unsigned char metadata[32];
    struct mcc_precache_snapshot files;
    if (!mcc_host_identity.valid || !mcc_package_name(level,canonical) ||
        csstrcasecmp(canonical,mcc_host_identity.name) ||
        !mcc_precache_snapshot(canonical,&files) || memcmp(&files,&mcc_host_identity.files,sizeof(files)) ||
        !mcc_identity_origin_matches(canonical) ||
        !mcc_identity_metadata(canonical,metadata) || memcmp(metadata,mcc_host_identity.metadata,32)) return 0;
    memcpy(digest,mcc_host_identity.digest,32);
    return 1;
}

void const *mcc_identity_host_settings(void const *settings,long bytes,struct network_game *copy)
{
    struct network_game const *game=settings;
    struct mcc_precache_snapshot files;
    char canonical[64];
    unsigned char metadata[32];
    uint32_t checksum;
    if (!game || bytes!=sizeof(*game)) return NULL;
    memcpy(copy,game,sizeof(*copy));
    memset(copy->mcc_content_sha256,0,32);
    memset(copy->mcc_metadata_sha256,0,32);
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
        !mcc_identity_origin_matches(canonical) ||
        !mcc_identity_metadata(canonical,metadata) || memcmp(metadata,mcc_host_identity.metadata,32) ||
        !mcc_validation_lookup(canonical,&checksum,NULL) ||
        checksum!=(uint32_t)game->map.version) return NULL;
    memcpy(copy->mcc_content_sha256,mcc_host_identity.digest,32);
    memcpy(copy->mcc_metadata_sha256,mcc_host_identity.metadata,32);
    /* MCC protocol 64 reserves the name tail for advisory Workshop metadata. */
    mcc_workshop_wire_pack(copy->map.name,&mcc_host_identity.item);
    return copy;
}

static int mcc_identity_matches(char const *name,unsigned char const digest[32],
    unsigned char const metadata[32],struct mcc_precache_snapshot *validated)
{
    struct mcc_precache_snapshot before,after;
    unsigned char actual[32],actual_metadata[32];
    int ok;
    if (!mcc_precache_snapshot(name,&before) ||
        !mcc_identity_metadata(name,actual_metadata) || memcmp(actual_metadata,metadata,32)) return 0;
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
    ok=mcc_validation_lookup(name,NULL,actual) &&
        !memcmp(actual,digest,32) && mcc_precache_snapshot(name,&after) &&
        !memcmp(&before,&after,sizeof(before));
    if (ok && validated) *validated=after;
    return ok;
}

/* Only a known item ID establishes presence. Its older package may not yet
 * contain the requested level, so search independently of the filename. */
static int mcc_identity_refusal(char const name[128],char const *session)
{
    struct mcc_workshop_item host,local;
    enum mcc_workshop_refusal kind=MCC_WORKSHOP_MISSING;
    char const *leaf=strrchr(session,'\\')+1,*candidate;
    char text[256];
    short i,count=mcc_maps_count(FALSE);
    if (!mcc_workshop_wire_unpack(name,&host)) kind=MCC_WORKSHOP_NOT_WORKSHOP;
    else for (i=0;i<count;i++) {
        candidate=mcc_maps_level_name(i);
        if (mcc_workshop_local_item(candidate,&local) && !strcmp(local.id,host.id)) {
            kind=MCC_WORKSHOP_DIFFERS;
            break;
        }
    }
    mcc_workshop_refusal_text(kind,&host,leaf,text,sizeof(text));
    return mcc_identity_error(text);
}

/* Admission reads only completed facts in packet handlers. A separate owned
 * check advances candidate fingerprints from the ordinary frame update. */
static void mcc_identity_select(char const *session,char const *local,
    unsigned char const digest[32],unsigned char const metadata[32],
    struct mcc_precache_snapshot const *files)
{
    mcc_client_identity.files=*files;
    strcpy(mcc_client_identity.session,session);
    strcpy(mcc_client_identity.local,local);
    memcpy(mcc_client_identity.digest,digest,32);
    memcpy(mcc_client_identity.metadata,metadata,32);
    error(_error_silent,"mcc identity: %s resolved to %s (SHA-256 matched)",session,local);
}

int mcc_identity_pending(char const *name)
{
    char canonical[64];
    if (!mcc_identity_job.active && !mcc_admission_client_pending()) return 0;
    return !name || (mcc_package_name(name,canonical) && !strcmp(canonical,mcc_identity_job.session));
}

int mcc_identity_accept(struct network_game const *game)
{
    char session[64];
    unsigned char zero[32]={0};
    struct mcc_precache_snapshot files;
    if (mcc_identity_job.active && !memcmp(game->map.name,mcc_identity_job.wire,128) &&
        (uint32_t)game->map.version==mcc_identity_job.checksum &&
        !memcmp(game->mcc_content_sha256,mcc_identity_job.digest,32) &&
        !memcmp(game->mcc_metadata_sha256,mcc_identity_job.metadata,32)) return 1;
    mcc_identity_job_clear();
    mcc_admission_client_clear();
    memset(&mcc_client_identity,0,sizeof(mcc_client_identity));
    mcc_cache_precache_clear();
    if (!mcc_level_name(game->map.name)) return !memcmp(game->mcc_content_sha256,zero,32) &&
        !memcmp(game->mcc_metadata_sha256,zero,32);
    if (!mcc_package_name(game->map.name,session) || !memcmp(game->mcc_content_sha256,zero,32))
        return mcc_identity_error("The host did not supply a valid MCC content identity.");
    /* The prepared host's loopback and unchanged previously validated exact
     * files need no asynchronous work or hold. Actual loading still rehashes. */
    if (mcc_identity_matches(session,game->mcc_content_sha256,game->mcc_metadata_sha256,&files)) {
        mcc_identity_select(session,session,game->mcc_content_sha256,game->mcc_metadata_sha256,&files);
        return 1;
    }
    memcpy(mcc_identity_job.wire,game->map.name,128);
    strcpy(mcc_identity_job.session,session);
    memcpy(mcc_identity_job.digest,game->mcc_content_sha256,32);
    memcpy(mcc_identity_job.metadata,game->mcc_metadata_sha256,32);
    mcc_identity_job.checksum=(uint32_t)game->map.version;
    strcpy(mcc_identity_job.candidates[0],session);mcc_identity_job.count=1;
    if (!mcc_admission_client_begin(game)) return mcc_identity_error("The MCC validation request could not be started.");
    mcc_identity_job.active=1;
    return 1;
}

static void mcc_identity_failed(char const *message)
{
    mcc_identity_error(message);
    mcc_identity_job_clear();mcc_admission_client_finish(0);
    memset(&mcc_client_identity,0,sizeof(mcc_client_identity));
    network_game_abort();
}

/* Refresh discovery once, then count all same-leaf candidates before opening
 * any fallback. A preferred exact path is retried after the rescan. */
static int mcc_identity_candidates(void)
{
    char const *leaf=strrchr(mcc_identity_job.session,'\\')+1,*candidate;
    short i,count;
    unsigned matches=0;
    mcc_maps_rescan();count=mcc_maps_count(FALSE);
    for (i=0;i<count;i++) {
        candidate=mcc_maps_level_name(i);
        if (!csstrcasecmp(leaf,strrchr(candidate,'\\')+1) && ++matches>MCC_IDENTITY_MAX_CANDIDATES) return 0;
    }
    mcc_identity_job.count=1;mcc_identity_job.index=0;mcc_identity_job.scanned=1;
    strcpy(mcc_identity_job.candidates[0],mcc_identity_job.session);
    for (i=0;i<count;i++) {
        candidate=mcc_maps_level_name(i);
        if (csstrcasecmp(leaf,strrchr(candidate,'\\')+1) || !csstrcasecmp(candidate,mcc_identity_job.session)) continue;
        /* The exact path is kept separately even when absent from discovery. */
        if (mcc_identity_job.count>=MCC_IDENTITY_MAX_CANDIDATES+1u) return 0;
        strcpy(mcc_identity_job.candidates[mcc_identity_job.count++],candidate);
    }
    return 1;
}

void mcc_identity_update(void)
{
    struct mcc_validation_proof proof;
    struct mcc_precache_snapshot files;
    unsigned char metadata[32];
    char reason[320];
    char const *candidate;
    int status;
    if (!mcc_identity_job.active && !mcc_identity_job.finishing) return;
    status=mcc_admission_client_status();
    if (status<0) {mcc_identity_failed("The host's MCC validation window expired or changed. Keep one matching package installed and rejoin.");return;}
    if (mcc_identity_job.finishing) {
        if (!mcc_admission_client_pending()) mcc_identity_job.finishing=0;
        return;
    }
    if (!status) return;
    if (mcc_identity_job.index<mcc_identity_job.count) {
        candidate=mcc_identity_job.candidates[mcc_identity_job.index];
        if (!mcc_identity_job.check) {
            if (!mcc_identity_metadata(candidate,metadata) || memcmp(metadata,mcc_identity_job.metadata,32)) {
                mcc_identity_job.index++;return;
            }
            mcc_identity_job.check=mcc_validation_check_begin(candidate,NULL);
            if (!mcc_identity_job.check) {mcc_identity_failed("There is not enough memory to check the MCC package.");return;}
        }
        status=mcc_validation_check_step(mcc_identity_job.check,&proof,reason,sizeof(reason));
        if (!status) return;
        mcc_validation_check_end(mcc_identity_job.check);mcc_identity_job.check=NULL;
        if (status>0 && !memcmp(proof.digest,mcc_identity_job.digest,32) &&
            (!mcc_identity_job.checksum || proof.checksum==mcc_identity_job.checksum) &&
            mcc_identity_metadata(candidate,metadata) && !memcmp(metadata,mcc_identity_job.metadata,32)) {
            strcpy(mcc_identity_job.selected,candidate);mcc_identity_job.selected_proof=proof;
            if (++mcc_identity_job.matches>1) {mcc_identity_failed("Several local MCC packages match the host. Keep one matching copy, or use the host's package folder name.");return;}
            if (!strcmp(candidate,mcc_identity_job.session)) mcc_identity_job.index=mcc_identity_job.count;
            else mcc_identity_job.index++;
        } else mcc_identity_job.index++;
        return;
    }
    if (!mcc_identity_job.matches && !mcc_identity_job.scanned) {
        if (!mcc_identity_candidates()) mcc_identity_failed("Too many MCC maps share this filename. Keep one matching package installed and retry.");
        return;
    }
    if (!mcc_identity_job.matches) {
        /* The sharing adapter retains this lease until the host confirms its
         * transfer hold. Queued intent alone is not proof a request was sent. */
        if (!mcc_identity_missing || !mcc_identity_missing(mcc_identity_job.wire,mcc_identity_job.digest,mcc_identity_job.metadata)) {
            mcc_identity_refusal(mcc_identity_job.wire,mcc_identity_job.session);
            mcc_identity_job_clear();mcc_admission_client_finish(0);network_game_abort();return;
        }
        mcc_identity_job.active=0;return;
    }
    if (!mcc_validation_publish(mcc_identity_job.selected,&mcc_identity_job.selected_proof) ||
        !mcc_identity_matches(mcc_identity_job.selected,mcc_identity_job.digest,mcc_identity_job.metadata,&files)) {
        mcc_identity_failed("The MCC package changed during admission. Finish installing it and rejoin.");return;
    }
    mcc_identity_select(mcc_identity_job.session,mcc_identity_job.selected,mcc_identity_job.digest,mcc_identity_job.metadata,&files);
    mcc_identity_job.active=0;mcc_identity_job.finishing=1;mcc_admission_client_finish(1);
}

int mcc_identity_accept_installed(char const name[128],unsigned char const digest[32],
    unsigned char const metadata[32],char const *installed)
{
    char session[64],selected[64];
    struct mcc_precache_snapshot files;
    if (!installed || !mcc_package_name(name,session) || !mcc_package_name(installed,selected) ||
        csstrcasecmp(strrchr(selected,'\\'),strrchr(session,'\\')) ||
        !mcc_identity_matches(selected,digest,metadata,&files))
        return mcc_identity_error("The downloaded MCC snapshot could not be verified after installation.");
    mcc_identity_select(session,selected,digest,metadata,&files);
    return 1;
}

int mcc_identity_check(char const *name)
{
    unsigned char const *expected=mcc_identity_expected(name);
    unsigned char metadata[32];
    struct mcc_precache_snapshot files;
    if (mcc_identity_pending(name)) return 0;
    if (!expected) return 1;
    if (!mcc_precache_snapshot(mcc_client_identity.local,&files) ||
        !mcc_identity_metadata(mcc_client_identity.local,metadata) ||
        memcmp(metadata,mcc_client_identity.metadata,32)) return 0;
    if (!memcmp(&files,&mcc_client_identity.files,sizeof(files))) return 1;
    return mcc_identity_matches(mcc_client_identity.local,expected,mcc_client_identity.metadata,NULL);
}

int mcc_identity_loaded(char const *name,struct mcc_runtime *runtime)
{
    unsigned char actual[32];
    unsigned char const *expected=mcc_identity_expected(name);
    if (!expected) return game_connection()!=_game_connection_network_client &&
        game_connection()!=_game_connection_network_server;
    return mcc_identity_metadata(mcc_client_identity.local,actual) &&
        !memcmp(actual,mcc_client_identity.metadata,32) &&
        mcc_resources_fingerprint(runtime,actual) && !memcmp(actual,expected,32);
}
