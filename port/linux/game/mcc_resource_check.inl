/* The same CRC and SHA framing as the synchronous load checks, with bounded
 * slices for lobby navigation. Owns its readers and never draws or ticks game
 * state. The source stream belongs to mcc_validation and outlives this check. */
struct mcc_resource_check {
    struct mcc_runtime runtime;
    struct mcc_sha256 hash;
    uint32_t crc, offset, size;
    unsigned type, mask;
    int framed;
};

int mcc_resources_check_paths(struct mcc_resource_check const *check, char paths[2][128])
{
    unsigned type;
    memset(paths,0,2*128);
    if (!check) return 0;
    for (type=1;type<=2;type++) {
        struct mcc_resources const *resources=check->runtime.resources;
        if (!(check->mask&(1u<<(type-1)))) continue;
        if (!resources || !resources->files[type-1].stream) return 0;
        strcpy(paths[type-1],resources->files[type-1].path);
    }
    return 1;
}

void mcc_resources_check_end(struct mcc_resource_check *check)
{
    if (!check) return;
    mcc_resources_dispose(&check->runtime);
    free(check);
}

/* Reject a broken authoritative override before trying any base combinations.
 * Base files that do not satisfy the map are not fingerprint candidates. */
static int mcc_resources_candidates(struct mcc_runtime *runtime,void *raw,
    struct mcc_resource_candidates *choices)
{
    unsigned type,set,i;
    for (type=1;type<=2;type++) {
        unsigned *count=&choices->count[type-1];
        if (!(type==1 ? runtime->report.external_bitmap_count : runtime->report.external_sound_count)) {
            *count=1;
            continue;
        }
        for (set=0;set<MCC_PACKAGE_BASE_SETS;set++) {
            struct mcc_resource_file file;
            struct mcc_resource_probe probe;
            char path[128];
            int authoritative,valid;
            if (!mcc_package_resource_candidate(runtime->level_name,type,set,path)) {
                snprintf(runtime->failure,sizeof(runtime->failure),
                    "Cannot read %s. Check access to this MCC resource.",path);
                return 0;
            }
            authoritative=!mcc_package_shared_resource(path,type);
            for (i=0;i<*count;i++) if (!strcmp(path,choices->paths[type-1][i])) break;
            if (i<*count) continue;
            memset(&file,0,sizeof(file));
            runtime->failure[0]=0;
            valid=mcc_resource_open(&file,type,runtime->level_name,path,runtime->failure,sizeof(runtime->failure));
            if (valid==1) {
                probe.type=type;probe.file=&file;
                valid=mcc_cache_visit_resources(raw,&runtime->report,mcc_resource_check,&probe);
                if (!valid) snprintf(runtime->failure,sizeof(runtime->failure),
                    "%s does not satisfy this MCC map's external references. Install the matching resource.",path);
            } else valid=0;
            mcc_resource_close(&file);
            if (valid) strcpy(choices->paths[type-1][(*count)++],path);
            if (authoritative) {
                if (!valid) return 0;
                break;
            }
        }
        if (!*count) {
            if (!runtime->failure[0]) snprintf(runtime->failure,sizeof(runtime->failure),
                "No readable matching MCC %s. Install the required base resources.",type==1 ? "bitmaps.map" : "sounds.map");
            return 0;
        }
    }
    runtime->failure[0]=0;
    return 1;
}

struct mcc_resource_check *mcc_resources_check_begin_candidate(char const *name,
    struct mcc_cache_source const *source, struct mcc_resource_candidates *candidates, char *reason, size_t size)
{
    struct mcc_resource_check *check;
    struct mcc_cache_identity identity;
    unsigned char header[MCC_CACHE_HEADER_BYTES];
    void *raw=NULL;
    char paths[2][128];
    enum mcc_cache_status status=MCC_CACHE_READ_FAILED;
    check=calloc(1,sizeof(*check));
    if (!check) return NULL;
    if (!source || !source->read || source->size>0x7fffffffu ||
        !mcc_package_name(name,check->runtime.level_name) ||
        !source->read(source->context,0,sizeof(header),header)) goto failed;
    status=mcc_cache_identify(header,sizeof(header),source->size,&identity);
    if (status!=MCC_CACHE_OK) goto failed;
    raw=malloc(identity.tag_data_size);
    status=MCC_CACHE_NO_MEMORY;
    if (!raw) goto failed;
    check->runtime.source=*source;
    status=mcc_cache_load(source,raw,identity.tag_data_size,&check->runtime.report);
    if (status!=MCC_CACHE_OK) goto failed;
    memset(paths,0,sizeof(paths));
    if (candidates) {
        if (!candidates->count[0] && !mcc_resources_candidates(&check->runtime,raw,candidates)) goto failed;
        if (!candidates->count[1] || candidates->index>=candidates->count[0]*candidates->count[1]) goto failed;
        strcpy(paths[0],candidates->paths[0][candidates->index%candidates->count[0]]);
        strcpy(paths[1],candidates->paths[1][candidates->index/candidates->count[0]]);
    }
    if (!mcc_resources_select_paths(&check->runtime,raw,candidates ? paths : NULL)) goto failed;
    free(raw);
    check->mask=(check->runtime.report.external_bitmap_count ? 1u : 0u) |
        (check->runtime.report.external_sound_count ? 2u : 0u);
    check->crc=check->runtime.report.identity.checksum;
    mcc_sha256_begin(&check->hash);
    mcc_sha256_add(&check->hash,"GulchCE MCC content v1",sizeof("GulchCE MCC content v1")-1);
    return check;
failed:
    if (reason && size) snprintf(reason,size,"%s",check->runtime.failure[0] ?
        check->runtime.failure : mcc_status_explanation(status));
    free(raw);
    mcc_resources_check_end(check);
    return NULL;
}

int mcc_resources_check_step(struct mcc_resource_check *check, uint32_t *checksum,
    unsigned char digest[32])
{
#ifdef HALO_WEB
    /* OPFS proxies each read to a worker. Larger bounded reads reduce that
     * overhead without changing the identity bytes or the slice limits. */
    unsigned char buffer[65536],marker;
#else
    unsigned char buffer[16384],marker;
#endif
    struct mcc_resource_file *resource;
    uint32_t bytes,spent=0;
    unsigned long began=system_milliseconds();
    long size;
    /* Bound both CPU work and bytes. No validation slices run in gameplay. */
    do {
        resource=check->type ? mcc_resource_get(&check->runtime,check->type) : NULL;
        if (!check->framed) {
            if (check->type) {
                if (!resource || fseek(resource->stream,0,SEEK_END) ||
                    (size=ftell(resource->stream))<0 || fseek(resource->stream,0,SEEK_SET)) return -1;
                check->size=(uint32_t)size;
                marker=(unsigned char)check->type;
                check->crc=(uint32_t)crc32(check->crc,&marker,1);
            } else check->size=(uint32_t)check->runtime.source.size;
            mcc_fingerprint_file_header(&check->hash,check->type,check->size);
            check->framed=1;
        }
        bytes=check->size-check->offset;
        if (bytes>sizeof(buffer)) bytes=sizeof(buffer);
        if (bytes) {
            if (check->type) {
                if (fread(buffer,1,bytes,resource->stream)!=bytes) return -1;
                check->crc=(uint32_t)crc32(check->crc,buffer,(uInt)bytes);
            } else if (!check->runtime.source.read(check->runtime.source.context,
                check->offset,bytes,buffer)) return -1;
            mcc_sha256_add(&check->hash,buffer,(int)bytes);
            check->offset+=bytes;
            spent+=bytes;
        }
        if (check->offset==check->size) {
            if (check->type) {
                marker=0xff;
                check->crc=(uint32_t)crc32(check->crc,&marker,1);
            }
            do {check->type++;} while (check->type<=2 && !(check->mask&(1u<<(check->type-1))));
            if (check->type>2) {
                *checksum=check->crc;
                if (!*checksum) *checksum=1;
                mcc_sha256_end(&check->hash,digest);
                return 1;
            }
            check->offset=0;
            check->framed=0;
        }
    } while (spent<4194304 && (unsigned long)(system_milliseconds()-began)<2);
    return 0;
}

struct mcc_resource_check *mcc_resources_check_begin(char const *name,
    struct mcc_cache_source const *source,char *reason,size_t size)
{
    return mcc_resources_check_begin_candidate(name,source,NULL,reason,size);
}
