/* Included by mcc_cache.c for its bounds-checked file opener. Cached facts
 * about local files are separate from session admission and are re-stamped
 * on every use. Only successful checks are retained; actual loads still
 * fingerprint the opened content independently. */
#include <xtl.h>
#define MCC_VALIDATION_RESULTS 4
static struct mcc_validation_result {
    char name[64];
    struct mcc_precache_snapshot files;
    uint32_t checksum;
    unsigned char digest[32];
} mcc_validation_results[MCC_VALIDATION_RESULTS];
static unsigned mcc_validation_next;
static struct {
    struct mcc_validation_result result;
    struct mcc_cache_source source;
    FILE *stream;
    struct mcc_resource_check *check;
    void *thread, *cancel;
    int worker_status;
    char reason[320];
    int state;
} mcc_validation_job;

/* Only private readers and hash state cross this boundary. Parsing, engine
 * diagnostics, snapshots and publication remain on the main thread. */
static unsigned long __stdcall mcc_validation_worker(void *unused)
{
    int status=0;
    (void)unused;
    while (WaitForSingleObject(mcc_validation_job.cancel,0)!=WAIT_OBJECT_0 && !status)
        status=mcc_resources_check_step(mcc_validation_job.check,
            &mcc_validation_job.result.checksum,mcc_validation_job.result.digest);
    mcc_validation_job.worker_status=status;
    return 0;
}

static void mcc_validation_join(void)
{
    if (mcc_validation_job.thread) {
        SetEvent(mcc_validation_job.cancel);
        WaitForSingleObject(mcc_validation_job.thread,INFINITE);
        CloseHandle(mcc_validation_job.thread);
        mcc_validation_job.thread=NULL;
    }
    if (mcc_validation_job.cancel) CloseHandle(mcc_validation_job.cancel);
    mcc_validation_job.cancel=NULL;
}

void mcc_validation_cancel(void)
{
    mcc_validation_join();
    mcc_resources_check_end(mcc_validation_job.check);
    if (mcc_validation_job.stream) fclose(mcc_validation_job.stream);
    memset(&mcc_validation_job,0,sizeof(mcc_validation_job));
}

char const *mcc_validation_error(void)
{
    return mcc_validation_job.reason;
}

int mcc_validation_lookup(char const *name,uint32_t *checksum,unsigned char digest[32])
{
    char canonical[64];
    struct mcc_precache_snapshot files;
    struct mcc_validation_result *result;
    unsigned i;
    if (!mcc_package_name(name,canonical) || !mcc_precache_snapshot(canonical,&files)) return 0;
    for (i=0;i<MCC_VALIDATION_RESULTS;i++) {
        result=&mcc_validation_results[i];
        if (strcmp(result->name,canonical)) continue;
        if (memcmp(&files,&result->files,sizeof(files))) {
            memset(result,0,sizeof(*result));
            return 0;
        }
        if (checksum) *checksum=result->checksum;
        if (digest) memcpy(digest,result->digest,32);
        return 1;
    }
    return 0;
}

int mcc_validation_request(char const *name,uint32_t *checksum,unsigned char digest[32])
{
    char canonical[64];
    struct mcc_precache_snapshot files;
    if (mcc_validation_lookup(name,checksum,digest)) return 1;
    if (!mcc_package_name(name,canonical)) return -1;
    if (!strcmp(canonical,mcc_validation_job.result.name)) {
        if (mcc_validation_job.state>=0) return 0;
        if (mcc_precache_snapshot(canonical,&files) &&
            !memcmp(&files,&mcc_validation_job.result.files,sizeof(files))) return -1;
    }
    mcc_validation_cancel();
    strcpy(mcc_validation_job.result.name,canonical);
    /* Opening, parsing and hashing wait for the pregame update, not a click. */
    return 0;
}

void mcc_validation_update(void)
{
    struct mcc_cache_identity identity;
    struct mcc_precache_snapshot after;
    int status;
    if (!mcc_validation_job.result.name[0] || mcc_validation_job.state<0) return;
    if (!mcc_validation_job.stream) {
        if (!mcc_precache_snapshot(mcc_validation_job.result.name,&mcc_validation_job.result.files))
            goto failed;
        mcc_validation_job.stream=mcc_open_explain(mcc_validation_job.result.name,
            &mcc_validation_job.source,&identity,mcc_validation_job.reason,sizeof(mcc_validation_job.reason));
        if (!mcc_validation_job.stream) goto failed;
        mcc_validation_job.check=mcc_resources_check_begin(mcc_validation_job.result.name,
            &mcc_validation_job.source,mcc_validation_job.reason,sizeof(mcc_validation_job.reason));
        if (!mcc_validation_job.check) goto failed;
        mcc_validation_job.cancel=CreateEventA(NULL,TRUE,FALSE,NULL);
        if (mcc_validation_job.cancel)
            mcc_validation_job.thread=CreateThread(NULL,256*1024,mcc_validation_worker,NULL,0,NULL);
        return;
    }
    if (mcc_validation_job.thread) {
        if (WaitForSingleObject(mcc_validation_job.thread,0)!=WAIT_OBJECT_0) return;
        status=mcc_validation_job.worker_status;
    } else {
        /* A platform without an available worker still keeps navigation live. */
        status=mcc_resources_check_step(mcc_validation_job.check,&mcc_validation_job.result.checksum,
            mcc_validation_job.result.digest);
    }
    if (!status) return;
    if (status<0) goto failed;
    if (!mcc_precache_snapshot(mcc_validation_job.result.name,&after) ||
        memcmp(&after,&mcc_validation_job.result.files,sizeof(after))) {
        snprintf(mcc_validation_job.reason,sizeof(mcc_validation_job.reason),
            "The MCC package changed during validation. Finish installing it and select it again.");
        goto failed;
    }
    mcc_validation_results[mcc_validation_next++ % MCC_VALIDATION_RESULTS]=mcc_validation_job.result;
    mcc_validation_cancel();
    return;
failed:
    mcc_validation_join();
    if (!mcc_validation_job.reason[0]) snprintf(mcc_validation_job.reason,sizeof(mcc_validation_job.reason),
        "The MCC package could not be read completely. Check its map and resources.");
    mcc_resources_check_end(mcc_validation_job.check);
    mcc_validation_job.check=NULL;
    if (mcc_validation_job.stream) fclose(mcc_validation_job.stream);
    mcc_validation_job.stream=NULL;
    mcc_validation_job.state=-1;
}
