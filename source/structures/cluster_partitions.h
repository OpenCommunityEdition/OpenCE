/*
CLUSTER_PARTITIONS.H

header included in hcex build.
*/

#ifndef __CLUSTER_PARTITIONS_H
#define __CLUSTER_PARTITIONS_H
#pragma once

/* ---------- headers */

#include "math/real_math.h"
#include "memory/data.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct cluster_partition
{
	long *cluster_first_data_references;
	struct data_array *data_reference_data;
	struct data_array *cluster_reference_data;
	/* port: where a datum's references are in the clusters' lists
	(cluster_partitions.c's cluster_partition_port_remove), by absolute
	index: the reference before each in its cluster's list, and each
	cluster reference's reference in its cluster's list. Not in the game
	state. */
	long *port_previous_references;
	long *port_cluster_data_references;
	/* port: counts each change to the clusters' lists (and each forgetting
	of them), for what is kept of them while they don't change
	(object_lights.c's lights_port_cluster) */
	unsigned long port_modification_count;
};

/* ---------- prototypes/CLUSTER_PARTITIONS.C */

void cluster_partition_new(
	struct cluster_partition *partition,
	char const *name);
void cluster_partition_make_valid(
	struct cluster_partition *partition);
void cluster_partition_make_invalid(
	struct cluster_partition *partition);
void cluster_partition_delete(
	struct cluster_partition *partition);
void cluster_partition_copy(
	struct cluster_partition *result,
	struct cluster_partition const *source);
/* port: every partition forgets where its references are (the game state
holding them is about to be loaded: game_state.c's before-load procs) */
void cluster_partitions_port_forget(
	void);
void cluster_partition_reconnect(
	struct cluster_partition *partition,
	long datum_index,
	long *first_cluster_reference,
	real_point3d const *position,
	float radius,
	struct location const *location);
void cluster_partition_disconnect(
	struct cluster_partition *partition,
	long datum_index,
	long *first_cluster_reference);
long cluster_partition_get_first_datum(
	struct cluster_partition const *partition,
	long *reference_index,
	short cluster_index);
long cluster_partition_get_next_datum(
	struct cluster_partition const *partition,
	long *reference_index);
long cluster_partition_get_first_cluster(
	struct cluster_partition const *partition,
	long *reference_index,
	long first_cluster_reference);
long cluster_partition_get_next_cluster(
	struct cluster_partition const *partition,
	long *reference_index);


/* ---------- globals */

/* ---------- public code */

#endif // __CLUSTER_PARTITIONS_H
