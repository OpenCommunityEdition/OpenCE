/* Headless MCC geometry checks. Uses the real engine vertex compressor and
 * mock GPU handles; it does not prove rendering or gameplay compatibility. */
#include "cseries.h"
#include "errors.h"
#include "rasterizer/rasterizer_geometry.h"
#include "mcc_runtime.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The game maps allocations and memory copies through cseries. Keep those
 * calls present in the adapter under test, backed by CRT storage here. */
#undef malloc
#undef free
#undef memset
#undef memcpy
#undef fopen
void *debug_malloc(unsigned int bytes,boolean clear,char const *file,long line) {
    void *p=malloc(bytes); (void)file; (void)line;
    if (p && clear) memset(p,0,bytes);
    return p;
}
void debug_free(void *p,char const *file,long line) {
    if (!p) { fprintf(stderr,"invalid NULL debug_free (%s:%ld)\n",file,line); exit(12); }
    free(p);
}
void *csmemset(void *p,long value,unsigned long bytes) { return memset(p,(int)value,bytes); }
void *csmemcpy(void *p,void const *source,unsigned long bytes) { return memcpy(p,source,bytes); }

#ifdef _WIN32
__declspec(dllimport) void *__stdcall VirtualAlloc(void *,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualFree(void *,size_t,unsigned long);
#else
#include <sys/mman.h>
#endif

char temporary[256];
static int active_buffers,created_buffers,fail_at=-1;
static struct vertex_buffer *model_buffers[8192];
static unsigned model_buffer_count;

void error(short priority,char const *format,...) {
    va_list args; (void)priority;
    va_start(args,format); vfprintf(stderr,format,args); va_end(args); fputc('\n',stderr);
}
char *csprintf(char *buffer,char *format,...) {
    va_list args; va_start(args,format); vsnprintf(buffer,256,format,args); va_end(args); return buffer;
}
void display_assert(char *information,char *file,long line,boolean fatal) {
    fprintf(stderr,"assertion: %s (%s:%ld)\n",information,file,line); if (fatal) exit(3);
}
void system_exit(long code) { exit((int)code); }

boolean rasterizer_vertex_buffer_new(struct vertex_buffer *b,long type,long count,
    void const *vertices,long bytes) {
    if (created_buffers==fail_at) return FALSE;
    if (count<=0 || bytes!=count*rasterizer_geometry_get_vertex_size((short)type)) exit(4);
    b->hardware_format=malloc(1); if (!b->hardware_format) exit(5);
    b->type=(short)type; b->count=count; b->offset=0; b->base_address=(void *)vertices;
    if (model_buffer_count==8192) exit(9);
    model_buffers[model_buffer_count++]=b;
    active_buffers++; created_buffers++; return TRUE;
}
void rasterizer_vertex_buffer_delete(struct vertex_buffer *b) {
    if (b->hardware_format) { free(b->hardware_format); b->hardware_format=NULL; active_buffers--; }
}
boolean rasterizer_triangle_buffer_new(struct triangle_buffer *b,short type,long count,void const *data) {
    if (created_buffers==fail_at) return FALSE;
    if (type!=1 || count<=0 || !data) exit(6);
    b->hardware_format=malloc(1); if (!b->hardware_format) exit(5);
    b->type=type; b->count=count; b->base_address=(void *)data;
    active_buffers++; created_buffers++; return TRUE;
}
void rasterizer_triangle_buffer_delete(struct triangle_buffer *b) {
    if (b->hardware_format) { free(b->hardware_format); b->hardware_format=NULL; active_buffers--; }
}

static int read_file(void *context,uint32_t offset,uint32_t size,void *out) {
    return !fseek(context,(long)offset,SEEK_SET) && fread(out,1,size,context)==size;
}
int mcc_runtime_read(struct mcc_runtime *r,uint32_t offset,uint32_t bytes,void *out) {
    return offset<=r->source.size && bytes<=r->source.size-offset && r->source.read(r->source.context,offset,bytes,out);
}
void *mcc_runtime_pointer(struct mcc_runtime *r,uint32_t address,uint32_t bytes) {
    if (address<r->report.tag_base || address-r->report.tag_base>r->used ||
        bytes>r->used-(address-r->report.tag_base)) return NULL;
    return r->tags+(address-r->report.tag_base);
}
void *mcc_runtime_allocate(struct mcc_runtime *r,uint32_t bytes) {
    uint32_t i,start=(r->used+3)&~3u,limit=r->capacity;
    for (i=0;i<r->report.bsp_count;i++) {
        uint32_t b=r->report.bsps[i].address-r->report.tag_base;
        if (b<limit) limit=b;
    }
    if (start>limit || bytes>limit-start) return NULL;
    r->used=start+bytes; memset(r->tags+start,0,bytes); return r->tags+start;
}

int main(int argc,char **argv) {
    struct mcc_runtime r;
    FILE *file;
    long length;
    uint32_t i;
    int success, palette_parts=0, palette_sum=0;
    if (argc<2 || argc>3) return 2;
    if (argc==3) fail_at=atoi(argv[2]);
    memset(&r,0,sizeof(r));
    file=fopen(argv[1],"rb"); if (!file) return 2;
    fseek(file,0,SEEK_END); length=ftell(file); if (length<0) return 2;
    r.source.context=file; r.source.read=read_file; r.source.size=(uint64_t)length;
    if (mcc_cache_inspect(&r.source,&r.report)!=MCC_CACHE_OK) return 2;
    r.capacity=MCC_CACHE_TAG_CAPACITY;
#ifdef _WIN32
    r.tags=VirtualAlloc((void *)(uintptr_t)r.report.tag_base,r.capacity,0x3000,4);
#else
    r.tags=mmap((void *)(uintptr_t)r.report.tag_base,r.capacity,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
#endif
    if ((uintptr_t)r.tags!=r.report.tag_base) return 2;
    if (mcc_cache_load(&r.source,r.tags,r.capacity,&r.report)!=MCC_CACHE_OK) return 2;
    r.used=r.report.identity.tag_data_size;
    r.tag_index=r.tags+0x28;
    success=mcc_geometry_prepare(&r);
    /* The validator must see precisely the buffer's original payload and
     * reject an index/vertex identity mismatch. Palette indices remain
     * inspectable after all models have been converted. */
    for (i=0;success && i<model_buffer_count;i++) {
        struct vertex_buffer *b=model_buffers[i];
        unsigned long bytes=0;
        unsigned char const *nodes=NULL;
        short n,j;
        void *payload=mcc_geometry_buffer_data(&r,b->hardware_format,0,&bytes);
        if (!payload || payload!=b->base_address || bytes!=(unsigned long)b->count*32 ||
            !mcc_geometry_contains(&r,payload,bytes) ||
            mcc_geometry_buffer_data(&r,b->hardware_format,1,NULL)) return 10;
        n=mcc_geometry_part_palette(&r,b,&nodes);
        if (n) palette_parts++;
        for (j=0;j<n;j++) palette_sum+=nodes[j];
    }
    for (i=0;success && i<r.report.bsp_count;i++) {
        void *structure;
        struct mcc_cache_bsp *b=&r.report.bsps[i];
        success=mcc_geometry_bsp_load(&r,b->tag_handle,b->file_offset,b->size,
            (void *)(uintptr_t)b->address,&structure);
        if (success && !structure) return 7;
        mcc_geometry_bsp_unload(&r);
    }
    mcc_geometry_dispose(&r);
    printf("converted=%d\ncreated_buffers=%d\nactive_buffers=%d\npalette_parts=%d\npalette_sum=%d\n",
        success,created_buffers,active_buffers,palette_parts,palette_sum);
    if (active_buffers || r.geometry) return 8;
#ifdef _WIN32
    VirtualFree(r.tags,0,0x8000);
#else
    munmap(r.tags,r.capacity);
#endif
    fclose(file);
    return success ? 0 : 1;
}
