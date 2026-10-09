/* The held gun anchored to its controller (vr_render_first_person_ik).

The first-person model is posed from the weapon camera, so where its gun
hand lands depends on each weapon's animation: the pistol's sat ahead of
and above the real hand and swung around the camera as the controller
turned. Each frame the whole model moves by the gap from its gun hand's
wrist to the tracked wrist. The gap is kept in the camera's own axes, so
turning the controller turns the gun about the wrist.

While the native animation owns the gun arm (ready, reload, melee, throw)
the last gap holds, so the animation plays around the hand; afterwards only
the difference from the tracked gap eases out (~0.05 s), so the gun follows
the controller at once even while it settles (two hands on the gun turn the
camera's axes). Each graph and hand keeps its own last gap: a weapon's grip
is measured once and reused when it is drawn again. Before a
weapon's first measurement the gap is zero (the old placement). Time is the
predicted XR frame time; render-only and allocation-free. */
#ifndef HALO_VR_GUN_ANCHOR_H
#define HALO_VR_GUN_ANCHOR_H
#include <math.h>

#define VR_GUN_ANCHOR_GRAPHS 16

struct vr_gun_anchor
{
	const void *graph[VR_GUN_ANCHOR_GRAPHS];
	int hand[VR_GUN_ANCHOR_GRAPHS];
	float gap[VR_GUN_ANCHOR_GRAPHS][3];
	int count, next, current;
	float local[3], residual[3];
	int valid, settling, residual_set;
	double time;
};

static void vr_gun_anchor_reset(struct vr_gun_anchor *a)
{
	a->current = -1;
	a->valid = 0;
	a->settling = 0;
	a->residual_set = 0;
	a->local[0] = a->local[1] = a->local[2] = 0.0f;
}

/* axes: the weapon camera's forward, left and up (unit vectors); measured:
the tracked wrist minus the model's wrist (world); hold: the gun arm is
natively animated; limit: the largest gap kept (world units). out: the
translation for every node of the model. Returns 1 when out is not zero. */
static int vr_gun_anchor_update(struct vr_gun_anchor *a, const void *graph, int hand,
	const float axes[3][3], const float measured[3], int hold, double now, float limit, float out[3])
{
	float m[3], length;
	int i, entry;

	out[0] = out[1] = out[2] = 0.0f;
	if (!isfinite(now) || !(limit > 0.0f))
	{
		vr_gun_anchor_reset(a);
		return 0;
	}
	if (a->current < 0 || a->graph[a->current] != graph || a->hand[a->current] != hand)
	{
		for (entry = 0; entry < a->count && (a->graph[entry] != graph || a->hand[entry] != hand); entry++)
			;
		if (entry < a->count)
		{
			a->local[0] = a->gap[entry][0];
			a->local[1] = a->gap[entry][1];
			a->local[2] = a->gap[entry][2];
			a->valid = 1;
		}
		else
		{
			entry = a->next;
			a->next = (a->next + 1) % VR_GUN_ANCHOR_GRAPHS;
			if (a->count < VR_GUN_ANCHOR_GRAPHS)
				a->count++;
			a->graph[entry] = graph;
			a->hand[entry] = hand;
			a->local[0] = a->local[1] = a->local[2] = 0.0f;
			a->valid = 0;
		}
		a->current = entry;
		a->settling = 1;
		a->residual_set = 0;
		a->time = now;
	}
	for (i = 0; i < 3; i++)
		m[i] = measured[0] * axes[i][0] + measured[1] * axes[i][1] + measured[2] * axes[i][2];
	length = sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
	if (!isfinite(length))
		hold = 1;
	else if (length > limit)
	{
		for (i = 0; i < 3; i++)
			m[i] *= limit / length;
	}
	if (hold)
	{
		a->settling = 1;
		a->residual_set = 0;
	}
	else if (a->settling)
	{
		double dt = now - a->time;
		float keep, left = 0.0f;

		if (!a->residual_set)
		{
			for (i = 0; i < 3; i++)
				a->residual[i] = a->local[i] - m[i];
			a->residual_set = 1;
		}
		dt = dt < 0.0 ? 0.0 : dt > 0.1 ? 0.1 : dt;
		keep = expf(-(float)dt / 0.05f);
		for (i = 0; i < 3; i++)
		{
			a->residual[i] *= keep;
			a->local[i] = m[i] + a->residual[i];
			left += a->residual[i] * a->residual[i];
		}
		if (sqrtf(left) < limit * 0.003f)
		{
			a->local[0] = m[0]; a->local[1] = m[1]; a->local[2] = m[2];
			a->settling = 0;
			a->residual_set = 0;
		}
		a->valid = 1;
	}
	else
	{
		a->local[0] = m[0]; a->local[1] = m[1]; a->local[2] = m[2];
		a->valid = 1;
	}
	a->time = now;
	if (a->valid)
	{
		a->gap[a->current][0] = a->local[0];
		a->gap[a->current][1] = a->local[1];
		a->gap[a->current][2] = a->local[2];
	}
	for (i = 0; i < 3; i++)
		out[i] = a->local[0] * axes[0][i] + a->local[1] * axes[1][i] + a->local[2] * axes[2][i];
	return a->local[0] != 0.0f || a->local[1] != 0.0f || a->local[2] != 0.0f;
}
#endif
