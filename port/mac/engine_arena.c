/* Keep the real game-state allocator and allocation registration together.
   Only the platform reservation and diagnostic lifecycle live here. Compile
   this instead of game_state.c, never alongside it. */
#include <mach/mach.h>
#define game_state_initialize mac_game_state_initialize_original
#define game_state_dispose mac_game_state_dispose_original
#include "../../source/saved games/game_state.c"
#undef game_state_initialize
#undef game_state_dispose
#include "engine_arena.h"
#include "engine_services.h"
#undef memset
static int mac_arena_owned;
static int mac_arena_borrowed;

/* The normal startup borrows the physical map's game-state reservation. Array
   disposal must recognize that ownership just as the diagnostic bootstrap does. */
void game_state_initialize(void)
{
    assert(!mac_arena_owned && !mac_arena_borrowed);
    mac_game_state_initialize_original();
    mac_arena_borrowed=1;
    mac_engine_services_set_arena_ownership(mac_engine_arena_contains);
}
void game_state_dispose(void)
{
    assert(mac_arena_borrowed && !mac_arena_owned);
    mac_game_state_dispose_original();
    mac_engine_services_set_arena_ownership(NULL);
    mac_arena_borrowed=0;
    if(bss_004d27b0){fclose(bss_004d27b0);bss_004d27b0=NULL;}
    memset(&game_state_globals,0,sizeof(game_state_globals));
    memset(game_state_allocations,0,sizeof(game_state_allocations));game_state_allocation_count=0;
    game_state_last_event="startup";game_state_last_event_time=NONE;
}


/* lruv_delete() clears both the cache and its embedded data_array before
   releasing them. The game-state registration is intentionally retained, so
   recognize that exact disposed representation when checking arena teardown. */
static int mac_engine_arena_lruv_disposed(struct game_state_allocation const *allocation)
{
    struct lruv_cache const *cache=(struct lruv_cache const *)allocation->address;
    struct data_array const *blocks;
    unsigned long address=(unsigned long)allocation->address;
    unsigned long base=(unsigned long)game_state_globals.base_address;
    unsigned long size;
    unsigned long index;
    unsigned char const *bytes;
    if(allocation->maximum_count<=0 || allocation->page_count<=0 || allocation->page_size_bits<=0)
        return 0;
    size=(unsigned long)lruv_allocation_size(allocation->maximum_count);
    if(address<base || address-base>GAME_STATE_SIZE || size>GAME_STATE_SIZE-(address-base))
        return 0;
    if(!cache)return 0;
    bytes=(unsigned char const *)cache;
    for(index=0;index<sizeof(*cache);++index)
        if(bytes[index])return 0;
    blocks=(struct data_array const *)(cache+1);
    bytes=(unsigned char const *)blocks;
    for(index=0;index<sizeof(*blocks);++index)
        if(bytes[index])return 0;
    return 1;
}

int mac_engine_arena_start(void)
{
    vm_address_t address=HALO_PORT_GAME_STATE_BASE_ADDRESS;
    FILE *log;
    if(mac_arena_owned || mac_arena_borrowed || game_state_globals.base_address || game_state_allocation_count || bss_004d27b0)return 0;
    log=tmpfile();
    if(!log)return 0;
    if(vm_allocate(mach_task_self(),&address,GAME_STATE_SIZE,FALSE)!=KERN_SUCCESS){fclose(log);return 0;}
    bss_004d27b0=log;
    game_state_globals.base_address=(void *)address;
    crc_new(&game_state_globals.allocation_size_checksum);
    game_state_globals.header=game_state_malloc("header",NULL,sizeof(*game_state_globals.header));
    game_state_globals.revert_time=NONE;
    mac_arena_owned=1;
    mac_engine_services_set_arena_ownership(mac_engine_arena_contains);
    return 1;
}
unsigned long mac_engine_arena_used(void)
{
    return game_state_globals.cpu_allocation_size+game_state_globals.gpu_allocation_size;
}
int mac_engine_arena_contains(const void *pointer)
{
    unsigned long address=(unsigned long)pointer;
    unsigned long base=(unsigned long)game_state_globals.base_address;
    return (mac_arena_owned || mac_arena_borrowed) && pointer && address>=base && address-base<GAME_STATE_SIZE;
}
unsigned long mac_engine_arena_registrations(void){return game_state_allocation_count;}
int mac_engine_arena_stop(void)
{
    long i;
    if(!mac_arena_owned)return 0;
    for(i=0;i<game_state_allocation_count;++i)
    {
        struct game_state_allocation *a=&game_state_allocations[i];
        if(a->kind==_game_state_allocation_data && ((struct data_array *)a->address)->actual_count) {
            struct data_array *data=a->address;
            fprintf(stderr,"native arena still owns %d records in %.32s\n",data->actual_count,data->name);
            return 0;
        }
        if(a->kind==_game_state_allocation_memory_pool && memory_pool_get_used_size(a->address))return 0;
        if(a->kind==_game_state_allocation_lruv_cache && !mac_engine_arena_lruv_disposed(a))return 0;
    }
    if(vm_deallocate(mach_task_self(),(vm_address_t)game_state_globals.base_address,GAME_STATE_SIZE)!=KERN_SUCCESS)return 0;
    mac_engine_services_set_arena_ownership(NULL);
    fclose(bss_004d27b0);bss_004d27b0=NULL;
    memset(&game_state_globals,0,sizeof(game_state_globals));
    memset(game_state_allocations,0,sizeof(game_state_allocations));game_state_allocation_count=0;
    game_state_last_event="startup";game_state_last_event_time=NONE;
    mac_arena_owned=0;return 1;
}
