#include <mach/mach.h>
#include "cseries.h"
#include "game_state.h"
#include "data.h"
#include "memory_pool.h"
#include "engine_arena.h"
static int checks,failures;
#define CHECK(x) do { ++checks; if(!(x)){++failures;fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);} } while(0)
int main(void)
{
    int cycle;
    vm_address_t occupied=HALO_PORT_GAME_STATE_BASE_ADDRESS;
    CHECK(vm_allocate(mach_task_self(),&occupied,vm_page_size,FALSE)==KERN_SUCCESS);
    *(volatile unsigned long *)occupied=0x51ac73e2UL;
    CHECK(!mac_engine_arena_start());
    CHECK(*(volatile unsigned long *)occupied==0x51ac73e2UL);
    CHECK(mac_engine_arena_used()==0);
    CHECK(vm_deallocate(mach_task_self(),occupied,vm_page_size)==KERN_SUCCESS);
    for(cycle=0;cycle<2;++cycle)
    {
        struct data_array *data;
        struct memory_pool *pool;
        long index;
        unsigned char *block=NULL,*gpu;
        CHECK(mac_engine_arena_start());
        CHECK(!mac_engine_arena_start());
        CHECK(mac_engine_arena_used()>0);
        CHECK(mac_engine_arena_registrations()==0);
        data=game_state_data_new("arena probe",4,8);
        pool=game_state_memory_pool_new("arena pool",4096);
        CHECK(mac_engine_arena_registrations()==2);
        CHECK((unsigned long)data>=HALO_PORT_GAME_STATE_BASE_ADDRESS);
        CHECK(!data->valid);
        data_make_valid(data);
        index=datum_new(data);CHECK(index!=NONE);
        CHECK(!mac_engine_arena_stop());
        datum_delete(data,index);
        CHECK(memory_pool_block_allocate(pool,(void **)&block,64));
        block[0]=0x51;block[63]=0xa7;
        CHECK(!mac_engine_arena_stop());
        memory_pool_compact(pool);CHECK(block[0]==0x51 && block[63]==0xa7);
        memory_pool_block_free(pool,(void **)&block);
        gpu=game_state_gpu_malloc("GPU record",NULL,64);
        CHECK((unsigned long)gpu==HALO_PORT_GAME_STATE_BASE_ADDRESS+HALO_PORT_GAME_STATE_SIZE-64);
        CHECK(gpu[0]==0 && gpu[63]==0);
        gpu[0]=0x34;
        CHECK(mac_engine_arena_stop());CHECK(!mac_engine_arena_stop());
        CHECK(mac_engine_arena_used()==0 && mac_engine_arena_registrations()==0);
    }
    printf("engine arena: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
