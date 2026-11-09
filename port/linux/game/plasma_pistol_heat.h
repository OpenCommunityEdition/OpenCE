/*
PLASMA_PISTOL_HEAT.H

The plasma pistol's gauge showing its heat (display.plasma_pistol_heat:
plasma_pistol_heat.c).
*/

#ifndef __PLASMA_PISTOL_HEAT_H
#define __PLASMA_PISTOL_HEAT_H

/* the value a meter shader drawn on an object shows, given its own: with
display.plasma_pistol_heat, the plasma pistol's gauge the larger of its own
(its charge) and the pistol's heat; otherwise its own */
real plasma_pistol_heat_meter_value(long object_index, real value);
/* (the larger of the two, at most 1) */
real plasma_pistol_heat_gauge_value(real value, real heat);

#endif
