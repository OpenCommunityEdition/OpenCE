#include "cseries.h"
#include "cseries/errors.h"

#include "math/real_math.h"

#include "structures/structures.h"
#include "structures/cluster_partitions.h"
#include "objects/reference_lists.h"
#include "engine_world_partition.h"
#undef malloc
#undef free
#undef memset
#include <stdlib.h>
#include <string.h>
static void release(struct cluster_partition *p)
{
    free(p->cluster_first_data_references);free(p->data_reference_data);free(p->cluster_reference_data);
    free(p->port_previous_references);free(p->port_cluster_data_references);memset(p,0,sizeof(*p));
}
int mac_engine_partition_start(struct cluster_partition *p)
{
    long bytes;
    if(!p || p->cluster_first_data_references || p->data_reference_data || p->cluster_reference_data ||
       p->port_previous_references || p->port_cluster_data_references)return 0;
    bytes=data_allocation_size(HALO_PORT_MAXIMUM_CLUSTER_REFERENCES,sizeof(struct data_reference));
    p->cluster_first_data_references=malloc(MAXIMUM_CLUSTERS_PER_STRUCTURE*sizeof(long));
    p->data_reference_data=malloc(bytes);p->cluster_reference_data=malloc(bytes);
    p->port_previous_references=malloc(HALO_PORT_MAXIMUM_CLUSTER_REFERENCES*sizeof(long));
    p->port_cluster_data_references=malloc(HALO_PORT_MAXIMUM_CLUSTER_REFERENCES*sizeof(long));
    if(!p->cluster_first_data_references || !p->data_reference_data || !p->cluster_reference_data ||
       !p->port_previous_references || !p->port_cluster_data_references){release(p);return 0;}
    data_initialize(p->data_reference_data,"native cluster data",HALO_PORT_MAXIMUM_CLUSTER_REFERENCES,sizeof(struct data_reference));
    data_initialize(p->cluster_reference_data,"native datum clusters",HALO_PORT_MAXIMUM_CLUSTER_REFERENCES,sizeof(struct data_reference));
    cluster_partition_make_valid(p);return 1;
}
int mac_engine_partition_stop(struct cluster_partition *p)
{
    if(!p || !p->data_reference_data || !p->cluster_reference_data ||
       p->data_reference_data->actual_count || p->cluster_reference_data->actual_count)return 0;
    cluster_partition_make_invalid(p);release(p);return 1;
}
