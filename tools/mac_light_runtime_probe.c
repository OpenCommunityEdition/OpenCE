#include "cseries.h"
#include "real_math.h"
#include "data.h"
#include "structures/cluster_partitions.h"
#include "objects/object_lights.h"
#include "engine_arena.h"
extern struct data_array *light_data;
extern struct cluster_partition light_cluster_partition;
static int checks,failures;
#define CHECK(x) do { ++checks; if(!(x)){++failures;fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);} } while(0)
int main(void)
{
    int map;
    CHECK(mac_engine_arena_start());
    lights_initialize();
    CHECK(light_data!=NULL);
    CHECK(!light_data->valid);
    CHECK(mac_engine_arena_registrations()==3);
    for(map=0;map<2;++map)
    {
        long index;
        lights_initialize_for_new_map();
        CHECK(light_data->valid && light_data->actual_count==0);
        CHECK(light_cluster_partition.data_reference_data->valid);
        CHECK(light_cluster_partition.cluster_reference_data->valid);
        CHECK(light_cluster_partition.cluster_first_data_references[0]==NONE);
        CHECK(!lights_enable(FALSE));CHECK(lights_enable(TRUE));
        /* Verify actual pool allocation, not light_new or an attached light. */
        index=datum_new(light_data);CHECK(index!=NONE);
        CHECK(!mac_engine_arena_stop());
        datum_delete(light_data,index);
        lights_dispose_from_old_map();
        CHECK(!light_data->valid);
        CHECK(!light_cluster_partition.data_reference_data->valid);
    }
    lights_dispose();
    CHECK(mac_engine_arena_stop());
    printf("light runtime: %d checks, %d failures; actual init/map lifecycle, no light_new\n",checks,failures);
    return failures?1:0;
}
