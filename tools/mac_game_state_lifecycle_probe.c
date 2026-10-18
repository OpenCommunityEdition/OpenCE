/* Real game-state startup against physical memory and a private save directory.
   This does not initialize the game/map or treat a blank image as a valid save. */
#include "cseries.h"
#include "engine_physical_memory.h"
#include "engine_arena.h"
#include "memory/data.h"
#include "cache/physical_memory_map.h"
#include "saved games/game_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef memset
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__);}}while(0)
int main(void)
{
    unsigned long *allocation;void *base;
    struct data_array *data;long datum;unsigned cycle;
    CHECK(getenv("HALO_MAC_SAVE_ROOT")!=NULL);if(failures)return 2;
    for(cycle=0;cycle<2;++cycle)
    {
    CHECK(mac_engine_physical_memory_start());if(failures)return 2;
    physical_memory_allocate();physical_memory_verify();
    CHECK(mac_engine_physical_memory_allocation_count()==4);
    base=physical_memory_get_game_state_base_address();CHECK(base==(void *)HALO_PORT_GAME_STATE_BASE_ADDRESS);
    game_state_initialize();
    allocation=game_state_malloc("native startup witness",NULL,16);
    CHECK((unsigned long)allocation>(unsigned long)base);
    CHECK((unsigned long)allocation+16<=(unsigned long)base+HALO_PORT_GAME_STATE_CPU_SIZE);
    if(allocation){allocation[0]=0x12345678;allocation[3]=0x87654321;CHECK(allocation[0]==0x12345678&&allocation[3]==0x87654321);}
    data=game_state_data_new("native array witness",4,8);
    CHECK(mac_engine_arena_contains(data));
    data_make_valid(data);datum=datum_new(data);CHECK(datum!=NONE);
    CHECK(data->actual_count==1);datum_delete(data,datum);CHECK(data->actual_count==0);
    data_dispose(data);CHECK(data->signature==0);
    CHECK(mac_engine_arena_contains(allocation));
    /* This confirms actual file I/O, not a playable-map save/revert. */
    CHECK(game_state_write_to_file());
    game_state_dispose();
    CHECK(!mac_engine_arena_contains(allocation));
    CHECK(mac_engine_arena_registrations()==0);
    CHECK(mac_engine_physical_memory_allocation_count()==4);
    physical_memory_free();CHECK(mac_engine_physical_memory_allocation_count()==0);
    mac_engine_physical_memory_stop();
    }
    printf("native game-state startup: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
