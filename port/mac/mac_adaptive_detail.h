#ifndef HALO_MAC_ADAPTIVE_DETAIL_H
#define HALO_MAC_ADAPTIVE_DETAIL_H
/* The adaptive detail governor (mac.adaptive_detail): measures the smoothed time of the
   frames and steps the picture's cost down a fixed ladder of rungs while it is over the
   target, and back up while it is comfortably under. Pure arithmetic over the frame
   times, so that a host test runs it (tools/mac_adaptive_detail_probe.c); mac_gpu_info.c
   owns the one instance and render_detail.c applies the rungs. Only what is drawn is
   ever a rung: nothing here is read by the simulation.

   Level L means rungs 0 to L-1 are in force. A rung the caller cannot apply (the
   baseline already has it, or the build cannot) is given in the skipped mask and is
   passed over, so a step always changes something. */
#include <string.h>

/* the ladder, in the order it is stepped down (what the profiles show costs time first and
   what is seen least first): */
enum
{
    MAC_ADAPT_DETAIL_OBJECTS,   /* ground grass sprites off (c10: 16% of the main thread) */
    MAC_ADAPT_PARTICLES_WIDE,   /* particles no wider than 96 pixels, a share of the wider left out */
    MAC_ADAPT_DECALS,           /* the world's decals off */
    MAC_ADAPT_FLARES,           /* lens flares off */
    MAC_ADAPT_SIZES_NEAR,       /* scenery and objects under twice the preset's pixels left out, models one detail level down */
    MAC_ADAPT_SIZES_FAR,        /* four times the preset's pixels (at most 192 and 40), models two levels down */
    MAC_ADAPT_FOG,              /* the planar fog and the fog screen off */
    MAC_ADAPT_PARTICLES_TINY,   /* particles no wider than 48 pixels */
    MAC_ADAPT_MODELS_LOWEST,    /* every model drawn at a tenth of its size on screen for its level of detail: the lowest but for the nearest */
    MAC_ADAPT_UNITS_SMALL,      /* bipeds and vehicles under 12 pixels (and what they carry) left out: a draw is 5 a biped */
    MAC_ADAPT_UNITS_MEDIUM,     /* and under 24 pixels */
    MAC_ADAPT_RESOLUTION,       /* the frame drawn with the Xbox's 480 lines, the last step */
    MAC_ADAPT_RUNGS
};

#define MAC_GOV_WARMUP_FRAMES 45        /* no step in the first frames of a map */
#define MAC_GOV_DOWN_DWELL 40           /* frames to wait after a step before another down */
#define MAC_GOV_SEVERE_DWELL 20         /* the same when the average is 1.5 times the target */
#define MAC_GOV_UP_DWELL 240            /* frames to wait after a step before an up (times the back-off) */
#define MAC_GOV_MIN_SECONDS 1.0         /* and no two steps closer than this */
#define MAC_GOV_DOWN_OVER 1.00          /* down while the average is above the target times this (1.04 left iBook b30 hovering at a level of 33.9 ms, 0.6 over the target, when one step down holds 32.3) */
#define MAC_GOV_UP_UNDER 0.78           /* up while it is below the target times this */
#define MAC_GOV_IGNORE_MS 250.0         /* a frame this long is a load or a pause, not a measure */
#define MAC_GOV_OVER_RUN 15             /* frames the average must have been over the target before a step down (8 when it is 1.5 times) */
#define MAC_GOV_CLAMP 2.0               /* one frame counts as at most this many targets, so that a hitch does not move the average far */

struct mac_governor
{
    double budget_ms;                   /* 1000 / the target frame rate */
    double average_ms;                  /* the smoothed frame time */
    double seconds_since_change;
    double total_ms, level_ms[MAC_ADAPT_RUNGS + 1];
    unsigned long frames, counted;      /* frames offered; frames measured */
    unsigned long level_frames[MAC_ADAPT_RUNGS + 1];
    unsigned frames_in_map;
    unsigned since_change, since_up, over_run;
    unsigned up_backoff;                /* 1 to 8: doubles when an up step is soon undone */
    int level, maximum_level;
    unsigned long downs, ups;
    int stepped_up;                     /* an up step happened and has not been undone or outlived */
    double window_ms, last_window_ms;   /* the sum of the current block of 300 frames, the mean of the last whole block */
    unsigned window_frames;
};

static __inline__ void mac_governor_reset(struct mac_governor *g, double target_fps)
{
    memset(g, 0, sizeof(*g));
    if (!(target_fps >= 10.0)) target_fps = 10.0;
    if (target_fps > 120.0) target_fps = 120.0;
    g->budget_ms = 1000.0 / target_fps;
    g->up_backoff = 1;
}

/* a new map: the measure starts again, and so does the ladder (the statistics stay) */
static __inline__ void mac_governor_new_map(struct mac_governor *g)
{
    g->average_ms = 0.0;
    g->frames_in_map = 0;
    g->since_change = g->since_up = g->over_run = 0;
    g->seconds_since_change = 0.0;
    g->level = 0;
    g->up_backoff = 1;
    g->stepped_up = 0;
}

static __inline__ int mac_governor_level_down(const struct mac_governor *g, unsigned skipped)
{
    int level = g->level + 1;
    while (level <= MAC_ADAPT_RUNGS && (skipped & (1u << (level - 1)))) ++level;
    return level <= MAC_ADAPT_RUNGS ? level : g->level;
}

static __inline__ int mac_governor_level_up(const struct mac_governor *g, unsigned skipped)
{
    int level = g->level - 1;
    while (level > 0 && (skipped & (1u << (level - 1)))) --level;
    return level >= 0 ? level : 0;
}

/* One frame of ms milliseconds (the work of the frame, not the wait of a limiter; wall_ms
   is the time it took with the wait, which the minimum time between steps counts). Returns
   1 if the level changed (mac_governor_level says to what, and *direction -1 for a step
   down in cost, +1 for back up). */
static __inline__ int mac_governor_note(struct mac_governor *g, double ms, double wall_ms, unsigned skipped, int *direction)
{
    unsigned dwell;
    int level;
    double sample;
    ++g->frames;
    if (ms < 0.0 || ms >= MAC_GOV_IGNORE_MS) return 0;
    g->level_frames[g->level]++;
    g->level_ms[g->level] += ms;
    g->total_ms += ms;
    ++g->counted;
    g->window_ms += ms;
    if (++g->window_frames == 300u) {
        g->last_window_ms = g->window_ms / 300.0;
        g->window_ms = 0.0;
        g->window_frames = 0;
    }
    ++g->frames_in_map;
    sample = ms > g->budget_ms * MAC_GOV_CLAMP ? g->budget_ms * MAC_GOV_CLAMP : ms;
    if (g->frames_in_map == 1) g->average_ms = sample;
    else if (g->frames_in_map <= 10) g->average_ms += (sample - g->average_ms) / (double)g->frames_in_map;
    else g->average_ms += (sample - g->average_ms) * 0.1;
    ++g->since_change;
    if (g->average_ms > g->budget_ms * MAC_GOV_DOWN_OVER) ++g->over_run;
    else g->over_run = 0;
    if (g->since_up < 100000u) ++g->since_up;
    g->seconds_since_change += (wall_ms > ms ? wall_ms : ms) / 1000.0;
    if (g->stepped_up && g->since_up >= 900u) {
        /* the last up step has held: trust the ladder more */
        g->stepped_up = 0;
        if (g->up_backoff > 1) g->up_backoff /= 2;
    }
    if (g->frames_in_map < MAC_GOV_WARMUP_FRAMES || g->seconds_since_change < MAC_GOV_MIN_SECONDS) return 0;
    dwell = g->average_ms > g->budget_ms * 1.5 ? MAC_GOV_SEVERE_DWELL : MAC_GOV_DOWN_DWELL;
    if (g->average_ms > g->budget_ms * MAC_GOV_DOWN_OVER && g->since_change >= dwell &&
        g->over_run >= (g->average_ms > g->budget_ms * 1.5 ? MAC_GOV_OVER_RUN / 2 : MAC_GOV_OVER_RUN)) {
        level = mac_governor_level_down(g, skipped);
        if (level == g->level) return 0;
        if (g->stepped_up && g->since_up < 450u && g->up_backoff < 8) g->up_backoff *= 2;
        g->stepped_up = 0;
        g->level = level;
        if (level > g->maximum_level) g->maximum_level = level;
        ++g->downs;
        g->since_change = 0;
        g->seconds_since_change = 0.0;
        if (direction) *direction = -1;
        return 1;
    }
    if (g->level > 0 && g->average_ms < g->budget_ms * MAC_GOV_UP_UNDER &&
        g->since_change >= MAC_GOV_UP_DWELL * g->up_backoff) {
        level = mac_governor_level_up(g, skipped);
        if (level == g->level) return 0;
        g->level = level;
        ++g->ups;
        g->stepped_up = 1;
        g->since_up = 0;
        g->since_change = 0;
        g->seconds_since_change = 0.0;
        if (direction) *direction = 1;
        return 1;
    }
    return 0;
}
#endif
