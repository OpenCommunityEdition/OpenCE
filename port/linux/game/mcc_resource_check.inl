/* The same CRC and SHA framing as the synchronous load checks, with bounded
 * slices for lobby navigation. Owns its readers and never draws or ticks game
 * state. The source stream belongs to mcc_validation and outlives this check. */
struct mcc_resource_check {
    struct mcc_runtime runtime;
    struct mcc_sha256 hash;
    uint32_t table[256], crc, offset, size;
    unsigned type, mask;
    int framed;
};

void mcc_resources_check_end(struct mcc_resource_check *check)
{
    if (!check) return;
    mcc_resources_dispose(&check->runtime);
    free(check);
}

struct mcc_resource_check *mcc_resources_check_begin(char const *name,
    struct mcc_cache_source const *source, char *reason, size_t size)
{
    struct mcc_resource_check *check;
    struct mcc_cache_identity identity;
    unsigned char header[MCC_CACHE_HEADER_BYTES];
    void *raw=NULL;
    enum mcc_cache_status status=MCC_CACHE_READ_FAILED;
    uint32_t i,j,value;
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
    if (status!=MCC_CACHE_OK || !mcc_resources_select(&check->runtime,raw)) goto failed;
    free(raw);
    check->mask=(check->runtime.report.external_bitmap_count ? 1u : 0u) |
        (check->runtime.report.external_sound_count ? 2u : 0u);
    check->crc=~check->runtime.report.identity.checksum;
    for (i=0;i<256;i++) {
        value=i;
        for (j=0;j<8;j++) value=(value>>1)^(0xEDB88320u & (0u-(value&1u)));
        check->table[i]=value;
    }
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
    unsigned char buffer[16384];
    struct mcc_resource_file *resource;
    uint32_t bytes,k,spent=0;
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
                check->crc=(check->crc>>8)^check->table[(check->crc^check->type)&255];
            } else check->size=(uint32_t)check->runtime.source.size;
            mcc_fingerprint_file_header(&check->hash,check->type,check->size);
            check->framed=1;
        }
        bytes=check->size-check->offset;
        if (bytes>sizeof(buffer)) bytes=sizeof(buffer);
        if (bytes) {
            if (check->type) {
                if (fread(buffer,1,bytes,resource->stream)!=bytes) return -1;
                for (k=0;k<bytes;k++)
                    check->crc=(check->crc>>8)^check->table[(check->crc^buffer[k])&255];
            } else if (!check->runtime.source.read(check->runtime.source.context,
                check->offset,bytes,buffer)) return -1;
            mcc_sha256_add(&check->hash,buffer,(int)bytes);
            check->offset+=bytes;
            spent+=bytes;
        }
        if (check->offset==check->size) {
            if (check->type) check->crc=(check->crc>>8)^check->table[(check->crc^0xff)&255];
            do {check->type++;} while (check->type<=2 && !(check->mask&(1u<<(check->type-1))));
            if (check->type>2) {
                *checksum=~check->crc;
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
