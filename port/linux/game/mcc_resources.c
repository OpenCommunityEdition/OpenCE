/* MCC resource cache reader. The on-disk header and twelve-byte entries are
 * decoded explicitly; no CE resource state or readers are shared. */
#include "mcc_runtime.h"
#include "mcc_resources.h"
#include "mcc_package.h"
#include "cseries.h"
#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Logging includes the game's allocation macros. This reader owns libc
 * allocations (including calloc), so keep their original matching frees. */
#undef malloc
#undef free

#define MCC_RESOURCE_METADATA_LIMIT 0x1000000u
struct mcc_resource_item { uint32_t name, size, offset; };
struct mcc_resource_file {
    FILE *stream;
    char *names;
    struct mcc_resource_item *items;
    uint32_t count,data_bytes;
};
struct mcc_resources { struct mcc_resource_file files[2]; };

static uint32_t mcc_resource_word(unsigned char const *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}

static void mcc_resource_close(struct mcc_resource_file *file)
{
    if (file->stream) fclose(file->stream);
    free(file->names);free(file->items);memset(file,0,sizeof(*file));
}

static int mcc_resource_compare(void const *a,void const *b)
{
    uint32_t x=((struct mcc_resource_item const *)a)->offset;
    uint32_t y=((struct mcc_resource_item const *)b)->offset;
    return x<y ? -1 : x>y;
}

static int mcc_resource_open(struct mcc_resource_file *file,unsigned type,char const *level)
{
    unsigned char header[16],entry[12];
    uint32_t names,index,count,names_bytes,i;
    long length;
    char path[128];
    if (!mcc_package_resource_path(level,type,path)) return 0;
    file->stream=fopen(path,"rb");
    if (!file->stream) {
        error(_error_silent,"mcc resources: cannot open %s (required by %s)",path,level);
        return 0;
    }
    if (fseek(file->stream,0,SEEK_END) || (length=ftell(file->stream))<16 ||
        fseek(file->stream,0,SEEK_SET) || fread(header,1,16,file->stream)!=16) goto failed;
    names=mcc_resource_word(header+4);index=mcc_resource_word(header+8);count=mcc_resource_word(header+12);
    if (mcc_resource_word(header)!=type || names<16 || index<names || index>(uint32_t)length ||
        !count || count>MCC_RESOURCE_METADATA_LIMIT/sizeof(*file->items) ||
        count>((uint32_t)length-index)/12) goto failed;
    names_bytes=index-names;
    if (!names_bytes || names_bytes>MCC_RESOURCE_METADATA_LIMIT) goto failed;
    file->names=malloc(names_bytes);file->items=malloc(count*sizeof(*file->items));
    if (!file->names || !file->items || fseek(file->stream,(long)names,SEEK_SET) ||
        fread(file->names,1,names_bytes,file->stream)!=names_bytes) goto failed;
    for (i=0;i<count;i++) {
        struct mcc_resource_item *item=&file->items[i];
        if (fread(entry,1,12,file->stream)!=12) goto failed;
        item->name=mcc_resource_word(entry);item->size=mcc_resource_word(entry+4);item->offset=mcc_resource_word(entry+8);
        if (item->name>=names_bytes || !memchr(file->names+item->name,0,
            names_bytes-item->name<320 ? names_bytes-item->name : 320) ||
            item->offset<16 || item->offset>names || item->size>names-item->offset) goto failed;
    }
    file->count=count;
    file->data_bytes=names;
    qsort(file->items,count,sizeof(*file->items),mcc_resource_compare);
    return 1;
failed:
    error(_error_silent,"mcc resources: invalid resource file %s (required by %s)",path,level);
    mcc_resource_close(file);return 0;
}

int mcc_resources_read(struct mcc_runtime *runtime,unsigned type,char const *name,
    uint32_t offset,uint32_t bytes,void *out)
{
    struct mcc_resources *resources;
    struct mcc_resource_file *file;
    uint32_t lo=0,hi,i;
    struct mcc_resource_item const *relocated=NULL;
    if (!runtime || type<1 || type>2 || !name || !out || !bytes) return 0;
    resources=runtime->resources;
    if (!resources) {
        resources=calloc(1,sizeof(*resources));
        if (!resources) return 0;
        runtime->resources=resources;
    }
    file=&resources->files[type-1];
    if (!file->stream && !mcc_resource_open(file,type,runtime->level_name)) return 0;
    hi=file->count;
    while (lo<hi) {
        uint32_t middle=lo+(hi-lo)/2;
        if (file->items[middle].offset<offset) lo=middle+1;
        else hi=middle;
    }
    for (;lo<file->count && file->items[lo].offset==offset;lo++) {
        struct mcc_resource_item const *item=&file->items[lo];
        if (bytes<=item->size && !strcmp(file->names+item->name,name))
            return !fseek(file->stream,(long)offset,SEEK_SET) && fread(out,1,bytes,file->stream)==bytes;
    }
    /* Shared caches can be repacked independently of a scenario. A stale
     * offset must never select unrelated bytes: accept a unique full name
     * and exact size from the supplied cache's validated directory instead.
     * Prefix reads still require their original offset. */
    if (offset<16 || offset>file->data_bytes || bytes>file->data_bytes-offset) return 0;
    for (i=0;i<file->count;i++) {
        struct mcc_resource_item const *item=&file->items[i];
        if (!strcmp(file->names+item->name,name)) {
            if (relocated || bytes!=item->size) return 0;
            relocated=item;
        }
    }
    return relocated && !fseek(file->stream,(long)relocated->offset,SEEK_SET) &&
        fread(out,1,bytes,file->stream)==bytes;
}

void mcc_resources_dispose(struct mcc_runtime *runtime)
{
    struct mcc_resources *resources=runtime->resources;
    if (!resources) return;
    mcc_resource_close(&resources->files[0]);mcc_resource_close(&resources->files[1]);
    free(resources);runtime->resources=NULL;
}
