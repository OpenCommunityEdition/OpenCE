/* Explicitly selected multiplayer practice options. Stock variants are off. */
#ifndef __PERFORMANCE_OPTIONS_H
#define __PERFORMANCE_OPTIONS_H
#pragma once

#include "game/performance_variant.h"

unsigned long performance_options_get_flags(void);
/* Network receive / session initialization only; does not rebroadcast. */
void performance_options_apply_host_flags(unsigned long flags);
/* Authority and peer-capability checked by network_server_manager.c. */
boolean performance_options_set_host_flags(unsigned long flags);

void performance_options_initialize_for_new_map(void);
void performance_options_dispose_from_old_map(void);
/* Runs after map scripts, before objects_update, once per simulation tick. */
void performance_options_update(void);
void performance_options_render(void);
/* The spawn pads, with the world's transparent geometry. The line fallback
stays in performance_options_render when the pad model is not loaded. */
void performance_options_render_world(void);

struct performance_timer_statistics
{
    unsigned preferences;
    unsigned long countdown, beeps, minutes, items;
    unsigned long items_by_category[3];
    char const *last_cue;
};
void performance_options_timer_statistics(struct performance_timer_statistics *statistics);

/* Diagnostics use the same clock and marker registry as the visible options. */
long performance_options_timer_ticks(void);
short performance_options_markers_supported_count(void);
short performance_options_markers_visible_count(void);

#endif /* __PERFORMANCE_OPTIONS_H */
