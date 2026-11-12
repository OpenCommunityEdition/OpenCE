/* Included by mcc_cache.c for its bounds-checked opener. Independent jobs
 * retain private readers; cached facts never establish session admission. */
#include <xtl.h>
#define MCC_VALIDATION_RESULTS 4
static struct mcc_validation_proof mcc_validation_results[MCC_VALIDATION_RESULTS];
static unsigned mcc_validation_next;
struct mcc_validation_check {
    struct mcc_validation_proof result;
    struct mcc_cache_source source;
    FILE *stream;
    struct mcc_resource_check *check;
    void *thread, *cancel;
    int worker_status, state;
    char reason[320];
};
static struct mcc_validation_check *mcc_validation_job;
static char mcc_validation_reason[320];

static unsigned long __stdcall mcc_validation_worker(void *parameter)
{
    struct mcc_validation_check *job=parameter;
    int status=0;
    while (WaitForSingleObject(job->cancel,0)!=WAIT_OBJECT_0 && !status)
        status=mcc_resources_check_step(job->check,&job->result.checksum,job->result.digest);
    job->worker_status=status;
    return 0;
}

static void mcc_validation_close(struct mcc_validation_check *job)
{
    if (job->thread) {
        SetEvent(job->cancel);
        WaitForSingleObject(job->thread,INFINITE);
        CloseHandle(job->thread);
        job->thread=NULL;
    }
    if (job->cancel) CloseHandle(job->cancel);
    job->cancel=NULL;
    mcc_resources_check_end(job->check);
    job->check=NULL;
    if (job->stream) fclose(job->stream);
    job->stream=NULL;
}

void mcc_validation_check_end(struct mcc_validation_check *job)
{
    if (!job) return;
    mcc_validation_close(job);
    free(job);
}

struct mcc_validation_check *mcc_validation_check_begin(char const *name,char const *directory)
{
    char canonical[64];
    struct mcc_validation_check *job;
    if (!mcc_package_name(name,canonical) || (directory && strlen(directory)>=96)) return NULL;
    job=malloc(sizeof(*job));
    if (!job) return NULL;
    memset(job,0,sizeof(*job));
    strcpy(job->result.name,canonical);
    if (directory) strcpy(job->result.directory,directory);
    return job;
}

static int mcc_validation_snapshot(struct mcc_validation_proof const *result,
    struct mcc_precache_snapshot *files)
{
    int valid;
    if (result->directory[0]) mcc_package_stage_redirect(result->name,result->directory);
    valid=mcc_precache_snapshot(result->name,files);
    if (result->directory[0]) mcc_package_stage_redirect(NULL,NULL);
    return valid;
}

int mcc_validation_check_step(struct mcc_validation_check *job,
    struct mcc_validation_proof *proof,char *reason,size_t reason_size)
{
    struct mcc_cache_identity identity;
    struct mcc_precache_snapshot after;
    int status=0;
    if (!job) return -1;
    if (job->state) goto done;
    if (!job->stream) {
        if (!mcc_validation_snapshot(&job->result,&job->result.files)) goto failed;
        /* Redirect only synchronous selection/opening. The worker receives
         * already-open private readers and never observes global routing. */
        if (job->result.directory[0]) mcc_package_stage_redirect(job->result.name,job->result.directory);
        job->stream=mcc_open_explain(job->result.name,&job->source,&identity,job->reason,sizeof(job->reason));
        if (job->stream) job->check=mcc_resources_check_begin(job->result.name,
            &job->source,job->reason,sizeof(job->reason));
        status=job->check && mcc_resources_check_paths(job->check,job->result.resources);
        if (job->result.directory[0]) mcc_package_stage_redirect(NULL,NULL);
        if (!status) goto failed;
        job->cancel=CreateEventA(NULL,TRUE,FALSE,NULL);
        if (job->cancel) job->thread=CreateThread(NULL,256*1024,mcc_validation_worker,job,0,NULL);
        return 0;
    }
    if (job->thread) {
        if (WaitForSingleObject(job->thread,0)!=WAIT_OBJECT_0) return 0;
        status=job->worker_status;
    } else status=mcc_resources_check_step(job->check,&job->result.checksum,job->result.digest);
    if (!status) return 0;
    if (status<0) goto failed;
    if (!mcc_validation_snapshot(&job->result,&after) || memcmp(&after,&job->result.files,sizeof(after))) {
        snprintf(job->reason,sizeof(job->reason),
            "The MCC package changed during validation. Finish installing it and select it again.");
        goto failed;
    }
    job->state=1;
    mcc_validation_close(job);
    goto done;
failed:
    if (!job->reason[0]) snprintf(job->reason,sizeof(job->reason),
        "The MCC package could not be read completely. Check its map and resources.");
    job->state=-1;
    mcc_validation_close(job);
done:
    if (reason && reason_size) snprintf(reason,reason_size,"%s",job->reason);
    if (job->state==1 && proof) *proof=job->result;
    return job->state;
}

static char const *mcc_validation_leaf(char const *name)
{
    char const *a=strrchr(name,'\\'), *b=strrchr(name,'/');
    return a && (!b || a>b) ? a+1 : b ? b+1 : name;
}

int mcc_validation_publish(char const *name,struct mcc_validation_proof const *proof)
{
    struct mcc_validation_proof result;
    struct mcc_precache_snapshot after;
    char canonical[64],folder[128],*leaf;
    size_t length;
    unsigned i;
    if (!proof || !mcc_package_name(name,canonical) ||
        strcmp(mcc_validation_leaf(canonical),mcc_validation_leaf(proof->name)) ||
        (!proof->directory[0] && strcmp(canonical,proof->name)) ||
        !mcc_precache_snapshot(canonical,&after) || memcmp(&after,&proof->files,sizeof(after))) return 0;
    result=*proof;
    strcpy(result.name,canonical);
    result.directory[0]=0;
    if (proof->directory[0]) {
        strcpy(folder,canonical);
        for (i=0;folder[i];i++) if (folder[i]=='\\') folder[i]='/';
        leaf=strrchr(folder,'/');
        if (!leaf) return 0;
        *leaf=0;
        length=strlen(proof->directory);
        for (i=0;i<2;i++) {
            if (!result.resources[i][0]) continue;
            /* Downloaded packages are complete: never promote a proof that
             * borrowed a resource from a root/shared installation. */
            if (strncmp(proof->resources[i],proof->directory,length) || proof->resources[i][length]!='/') return 0;
            if (snprintf(result.resources[i],128,"%s/%s",folder,proof->resources[i]+length+1)>=128) return 0;
        }
    }
    for (i=0;i<MCC_VALIDATION_RESULTS;i++)
        if (!strcmp(mcc_validation_results[i].name,canonical)) break;
    if (i==MCC_VALIDATION_RESULTS) i=mcc_validation_next++ % MCC_VALIDATION_RESULTS;
    mcc_validation_results[i]=result;
    return 1;
}

static struct mcc_validation_proof *mcc_validation_find(char const *name)
{
    char canonical[64];
    struct mcc_precache_snapshot files;
    struct mcc_validation_proof *result;
    unsigned i;
    if (!mcc_package_name(name,canonical) || !mcc_precache_snapshot(canonical,&files)) return NULL;
    for (i=0;i<MCC_VALIDATION_RESULTS;i++) {
        result=&mcc_validation_results[i];
        if (strcmp(result->name,canonical)) continue;
        if (memcmp(&files,&result->files,sizeof(files))) {memset(result,0,sizeof(*result));return NULL;}
        return result;
    }
    return NULL;
}

int mcc_validation_lookup(char const *name,uint32_t *checksum,unsigned char digest[32])
{
    struct mcc_validation_proof const *result=mcc_validation_find(name);
    if (!result) return 0;
    if (checksum) *checksum=result->checksum;
    if (digest) memcpy(digest,result->digest,32);
    return 1;
}

int mcc_validation_resources(char const *name,char paths[2][128])
{
    struct mcc_validation_proof const *result=mcc_validation_find(name);
    memset(paths,0,2*128);
    if (!result) return 0;
    memcpy(paths,result->resources,sizeof(result->resources));
    return 1;
}

void mcc_validation_cancel(void)
{
    mcc_validation_check_end(mcc_validation_job);
    mcc_validation_job=NULL;
    mcc_validation_reason[0]=0;
}

char const *mcc_validation_error(void) {return mcc_validation_reason;}

int mcc_validation_request(char const *name,uint32_t *checksum,unsigned char digest[32])
{
    char canonical[64];
    struct mcc_precache_snapshot files;
    if (mcc_validation_lookup(name,checksum,digest)) return 1;
    if (!mcc_package_name(name,canonical)) {
        snprintf(mcc_validation_reason,sizeof(mcc_validation_reason),"The MCC package name is invalid. Select an installed MCC map.");
        return -1;
    }
    if (mcc_validation_job && !strcmp(canonical,mcc_validation_job->result.name)) {
        if (mcc_validation_job->state>=0) return 0;
        if (mcc_precache_snapshot(canonical,&files) &&
            !memcmp(&files,&mcc_validation_job->result.files,sizeof(files))) return -1;
    }
    mcc_validation_cancel();
    mcc_validation_job=mcc_validation_check_begin(canonical,NULL);
    if (!mcc_validation_job) snprintf(mcc_validation_reason,sizeof(mcc_validation_reason),
        "There is not enough memory to validate the MCC package. Close other games and try again.");
    return mcc_validation_job ? 0 : -1;
}

void mcc_validation_update(void)
{
    struct mcc_validation_proof proof;
    if (!mcc_validation_job) return;
    if (mcc_validation_check_step(mcc_validation_job,&proof,mcc_validation_reason,
        sizeof(mcc_validation_reason))!=1) return;
    if (mcc_validation_publish(proof.name,&proof)) mcc_validation_cancel();
    else {
        mcc_validation_job->state=-1;
        snprintf(mcc_validation_job->reason,sizeof(mcc_validation_job->reason),
            "The MCC package changed before validation completed. Finish installing it and select it again.");
        snprintf(mcc_validation_reason,sizeof(mcc_validation_reason),"%s",mcc_validation_job->reason);
    }
}
