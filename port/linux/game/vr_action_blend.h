/* Render-only native action ownership. Time is the predicted XR frame time. */
#ifndef HALO_VR_ACTION_BLEND_H
#define HALO_VR_ACTION_BLEND_H
#include <math.h>
struct vr_action_blend { double time; float weight[2]; int valid; };
static void vr_action_blend_update(struct vr_action_blend *s, double now, unsigned mask, int reset)
{
    double dt = now - s->time;
    if (!isfinite(now)) { s->valid = 0; s->weight[0] = s->weight[1] = 0; return; }
    if (reset || !s->valid || dt < 0 || dt > 0.25) {
        s->weight[0] = (mask & 1) ? 1 : 0;
        s->weight[1] = (mask & 2) ? 1 : 0;
    } else if (dt > 0) {
        for (int side = 0; side < 2; side++) {
            float target = (mask & (1u << side)) ? 1 : 0;
            float step = (float)dt / (target ? 0.08f : 0.16f);
            float w = s->weight[side];
            s->weight[side] = target ? fminf(1, w + step) : fmaxf(0, w - step);
        }
    }
    s->time = now; s->valid = 1;
}
#endif
