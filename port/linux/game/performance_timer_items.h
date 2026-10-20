#ifndef __PERFORMANCE_TIMER_ITEMS_H
#define __PERFORMANCE_TIMER_ITEMS_H

/* One bit per deterministic map spawn category: rocket, camo, overshield. */
void performance_timer_items_initialize(void);
unsigned performance_timer_items_due(long previous, long current);

#endif
