/* mac_adaptive_detail.h's governor over made-up frame times, no graphics card needed:
   cc -std=gnu89 -Wall -Iport/mac -o /tmp/gov tools/mac_adaptive_detail_probe.c && /tmp/gov */
#include <stdio.h>
#include "mac_adaptive_detail.h"

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

/* each rung takes this many milliseconds off a frame */
static const double gain[MAC_ADAPT_RUNGS] = { 8.0, 2.0, 1.0, 1.0, 5.0, 4.0, 2.0, 1.0, 6.0 };

static double cost(double base, int level)
{
    int i;
    for (i = 0; i < level && i < MAC_ADAPT_RUNGS; ++i) base -= gain[i];
    return base;
}

/* runs frames of a load (base ms at level 0, plus a slow wobble); returns the steps made, checks they are never
   closer than a second of frames */
static int run(struct mac_governor *g, double base, int frames, unsigned skipped, double *elapsed, int *last_step_frame)
{
    int i, steps = 0, direction = 0;
    for (i = 0; i < frames; ++i) {
        double ms = cost(base, g->level) + ((i * 7) % 5 - 2) * 0.5;
        if (mac_governor_note(g, ms, ms, skipped, &direction)) {
            ++steps;
            CHECK(g->seconds_since_change == 0.0);
        }
        *elapsed += ms / 1000.0;
    }
    (void)last_step_frame;
    return steps;
}

int main(void)
{
    struct mac_governor g;
    double elapsed = 0.0;
    int steps, direction = 0, i, last = 0;

    /* a load that fits: nothing happens */
    mac_governor_reset(&g, 30.0);
    steps = run(&g, 25.0, 2000, 0, &elapsed, &last);
    CHECK(steps == 0 && g.level == 0);

    /* a load of 48 ms: steps down until under 33.3, and holds there (needs 15 ms of gain: rungs 0, 4, 5 and a little) */
    mac_governor_reset(&g, 30.0);
    steps = run(&g, 48.0, 3000, 0, &elapsed, &last);
    CHECK(g.level >= 4 && g.level <= 6);
    CHECK(cost(48.0, g.level) <= 33.3 * 1.04);
    CHECK(g.downs == (unsigned long)g.level || g.ups > 0);
    CHECK(g.downs + g.ups <= 14);       /* it does not oscillate */

    /* the load goes away: back up to level 0, slowly */
    steps = run(&g, 20.0, 4000, 0, &elapsed, &last);
    CHECK(g.level == 0);

    /* a rung given as skipped is passed over, and one step still changes something */
    mac_governor_reset(&g, 30.0);
    g.frames_in_map = 100;
    g.average_ms = 50.0;
    g.since_change = 100;
    g.over_run = 100;
    g.seconds_since_change = 5.0;
    CHECK(mac_governor_note(&g, 50.0, 50.0, 1u << MAC_ADAPT_DETAIL_OBJECTS, &direction) == 1 && g.level == 2 && direction == -1);
    CHECK(mac_governor_level_down(&g, (1u << MAC_ADAPT_DECALS) | (1u << MAC_ADAPT_FLARES)) == 5);
    g.level = 4;
    CHECK(mac_governor_level_up(&g, (1u << 3) | (1u << 2)) == 2);
    g.level = 1;
    CHECK(mac_governor_level_up(&g, 1u << 0) == 0);
    g.level = MAC_ADAPT_RUNGS;
    CHECK(mac_governor_level_down(&g, 0) == MAC_ADAPT_RUNGS);

    /* every rung skipped: nothing to step */
    mac_governor_reset(&g, 30.0);
    steps = run(&g, 60.0, 500, (1u << MAC_ADAPT_RUNGS) - 1u, &elapsed, &last);
    CHECK(steps == 0 && g.level == 0);

    /* hitches among good frames (a spike of 120 ms each 25 frames, as a streaming stall) move nothing */
    mac_governor_reset(&g, 30.0);
    for (i = 0; i < 3000; ++i)
        mac_governor_note(&g, i % 25 == 0 ? 120.0 : 22.0, 22.0, 0, &direction);
    CHECK(g.level == 0);

    /* loads and pauses: a frame of 250 ms or more is no measure */
    mac_governor_reset(&g, 30.0);
    for (i = 0; i < 200; ++i)
        mac_governor_note(&g, i % 10 == 0 ? 900.0 : 20.0, 20.0, 0, &direction);
    CHECK(g.level == 0);

    /* a load that flips each time the ladder goes up (an up step that is undone): the waits lengthen */
    mac_governor_reset(&g, 30.0);
    for (i = 0; i < 20000; ++i) {
        /* at level 0 the frame is 40 ms, at 1 or more 27 ms (under 0.78 of the target): up looks fine, and is not */
        double ms = g.level == 0 ? 40.0 : 25.0;
        mac_governor_note(&g, ms, ms, 0, &direction);
    }
    CHECK(g.downs + g.ups < 40);
    CHECK(g.up_backoff >= 2);

    /* the budget: 45 frames a second */
    mac_governor_reset(&g, 45.0);
    CHECK(g.budget_ms > 22.0 && g.budget_ms < 22.3);
    printf(failures ? "%d failures\n" : "mac_adaptive_detail: all checks passed\n", failures);
    return failures != 0;
}
