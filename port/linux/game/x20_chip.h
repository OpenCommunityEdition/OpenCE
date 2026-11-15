#ifndef __X20_CHIP_H
#define __X20_CHIP_H

/* a10's bridge cinematic: Keyes's chip made in his hand at the console, not
in view (x20_chip.c) */

struct data_array;
struct scenario;

/* the pipe's taking, the chip's making and its move to his left hand moved
to right after Cortana's projection is destroyed, in the begin at the root,
if it is as the map has it */
boolean x20_chip_from_console(struct data_array *syntax, char const *strings, long string_bytes,
	long root_expression_index);
/* as a10's scripts are made ready (hs_scenario_postprocess) */
void x20_chip_scripts_loaded(struct scenario *scenario, struct data_array *syntax);

#endif
