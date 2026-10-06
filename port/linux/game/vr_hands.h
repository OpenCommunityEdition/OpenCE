/*
VR_HANDS.H

The VR mode's first-person hands and arms (HALO_VR; vr_hands.c), and what
they ask of the view (vr_render.c).
*/

#ifndef __VR_HANDS_H
#define __VR_HANDS_H

#ifdef HALO_VR

/* the frame the hands are posed in (vr_render_hands) */
struct vr_hands_frame
{
	/* vr.hands = "arms": the arms reach from the shoulders to the hands */
	boolean arms;
	/* the camera the first-person weapon was posed from */
	real_matrix4x3 camera;
	/* each controller's pose in the world, when it is known: its grip's
	position, turned as it points (the right one as the weapon points);
	right_valid, the weapon posed in the right controller */
	boolean right_valid;
	real_matrix4x3 right;
	boolean left_valid;
	real_matrix4x3 left;
	/* how far the left hand holds the long gun's foregrip (0 to 1) */
	real two_handed;
	/* the right trigger's and the left grip's pull (0 to 1) */
	real right_trigger;
	real left_grip;
	/* the eyes' middle, in the world, and the head's heading: from the
	heading the view is turned by (base_yaw, the turns of the stick) */
	real_point3d head;
	real base_yaw;
	real head_yaw;
	/* vr.shoulders: each shoulder from the eyes' middle, in metres (out to
	its side, up, forward), and the world's units a metre */
	real shoulder[3];
	real units;
	/* the time from the last frame's display to this one's (seconds) */
	real seconds;
};

/* vr_render.c's: the frame, FALSE when the hands are the game's */
boolean vr_render_hands(struct vr_hands_frame *frame);

/* vr_hands.c's */
boolean vr_hands_right_centre(real_point3d *centre);

#endif

#endif
