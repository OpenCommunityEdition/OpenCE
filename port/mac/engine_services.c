/* Minimal native services for linked engine collision units. Unimplemented
   assertion/tag-error paths fail loudly; this is not a complete game runtime. */
#include "cseries.h"
#include "cseries/errors.h"
#include "tag_files/tag_groups.h"
#include "engine_services.h"
#undef memcpy
#undef memset
#undef malloc
#undef free
#undef realloc
static int (*mac_engine_services_arena_owns_pointer)(const void *pointer);
unsigned long mac_engine_service_errors;
void *csmemcpy(void *out,const void *in,unsigned long bytes) { return memcpy(out,in,bytes); }
void *csmemset(void *out,long value,unsigned long bytes) { return memset(out,(int)value,bytes); }
void error(short priority,const char *format,...)
{
    va_list args;
    (void)priority; ++mac_engine_service_errors;
    va_start(args,format);vfprintf(stderr,format,args);va_end(args);fputc('\n',stderr);
}
void display_assert(char *information,char *file,long line,boolean fatal)
{
    (void)fatal;
    fprintf(stderr,"engine assertion %s:%ld: %s\n",file,line,information);abort();
}
void *tag_block_get_element_with_size(const struct tag_block *block,long index,long size)
{
    (void)block;(void)index;(void)size;
    fprintf(stderr,"invalid engine tag block access\n");abort();return NULL;
}

void *debug_malloc(unsigned int bytes,boolean clear,const char *file,long line)
{
    (void)file;(void)line;return clear?calloc(1,bytes):malloc(bytes);
}
void debug_free(void *pointer,const char *file,long line)
{
    (void)file;(void)line;
    /* Game-state allocations belong to the arena and are reclaimed together. */
    if(!mac_engine_services_arena_owns_pointer || !mac_engine_services_arena_owns_pointer(pointer))free(pointer);
}

void mac_engine_services_set_arena_ownership(int (*owns_pointer)(const void *pointer))
{
    mac_engine_services_arena_owns_pointer=owns_pointer;
}

void *debug_realloc(void *pointer,unsigned int bytes,const char *file,long line)
{
    /* Arena allocations cannot move independently of their registered owners. */
    if(pointer && mac_engine_services_arena_owns_pointer &&
        mac_engine_services_arena_owns_pointer(pointer)) {
        fprintf(stderr,"native realloc refused arena pointer at %s:%ld\n",file?file:"?",line);
        return NULL;
    }
    /* Darwin realloc(p,0) may return a new allocation; the engine's dynamic
       arrays require NULL when resizing to zero. */
    if(!bytes) {free(pointer);return NULL;}
    return realloc(pointer,bytes);
}
