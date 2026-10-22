/*
PLACEMENT_TRACE.H

The reference traces of the native engine's placements (placement_trace.c).
*/

#ifndef PLACEMENT_TRACE_H
#define PLACEMENT_TRACE_H

struct scenario_object_datum;

/* a scenario object object_new_from_scenario made: logged with
debug.placement_trace */
void placement_trace_placed(long object_index, struct scenario_object_datum const *scenario_object);
/* once a tick: the end of the level's placements, at the first tick */
void placement_trace_update(void);

#endif
