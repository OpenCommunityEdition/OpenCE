/*
X10_WARTHOGS.H

a10's opening cinematic ends in the hangar with Sergeant Johnson's speech:
its warthog drives behind him on Legendary too, is destroyed out of sight
before it crashes into the hangar's far wall, and neither of the scene's
warthog drivers is another Johnson (x10_warthogs.c).
*/

#ifndef __X10_WARTHOGS_H
#define __X10_WARTHOGS_H

struct data_array;
struct scenario;
struct scenario_object_datum;

/* the begin whose root expression is root_expression_index, given the
script nodes and the strings they name: a wake of x10_hog_4 taken out of
the begin of an if that the begin follows with its own wake of x10_hog_4.
TRUE if it was taken out; FALSE, and nothing changed, if the begin is not
laid out so */
boolean x10_warthogs_wake_once(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index);

/* the begin whose root expression is root_expression_index: the sleep for
the rest of x10_warthog_2's recording, after its recording_play, made a
sleep of 120 ticks. TRUE if it was; FALSE, and nothing changed, if the
begin is not laid out so */
boolean x10_warthogs_run_shortened(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index);

/* the placement of the object of that name: no variant, if it is one of
the scene's warthog drivers placed with Johnson's. TRUE if it changed */
boolean x10_warthogs_driver_placed(
	char const *name,
	struct scenario_object_datum *object);

/* a10's hangar scene, as its scripts are made ready (hs_scenario_postprocess) */
void x10_warthogs_scripts_loaded(
	struct scenario *scenario,
	struct data_array *syntax);

#endif
