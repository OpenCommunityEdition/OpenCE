#include "controller_setup_hid.h"

#include <string.h>

#include "../mac_gamepad_report.h"

const char *cs_sony_model(int vendor, int product)
{
	if (vendor != CS_VENDOR_SONY)
		return "";
	if (product == CS_PRODUCT_DUALSHOCK4 || product == CS_PRODUCT_DUALSHOCK4_V2)
		return "DualShock 4";
	if (product == CS_PRODUCT_DUALSENSE)
		return "DualSense";
	return "";
}

void cs_pad_clear(cs_pad_state *pad, int xbox)
{
	memset(pad, 0, sizeof(*pad));
	pad->xbox = xbox;
}

int cs_hid_scale_stick(int value, int logical_min, int logical_max)
{
	double t;
	int result;

	if (logical_max <= logical_min)
		return 0;
	if (logical_min == -32768 && logical_max == 32767) {
		result = value;
	} else {
		if (value < logical_min)
			value = logical_min;
		if (value > logical_max)
			value = logical_max;
		t = (double)(value - logical_min) / (double)(logical_max - logical_min);
		result = (int)(t * 65535.0 + 0.5) - 32768;
	}
	if (result < -32768)
		result = -32768;
	if (result > 32767)
		result = 32767;
	return result;
}

int cs_hid_scale_trigger(int value, int logical_min, int logical_max)
{
	double t;

	if (logical_max <= logical_min)
		return 0;
	if (value < logical_min)
		value = logical_min;
	if (value > logical_max)
		value = logical_max;
	t = (double)(value - logical_min) / (double)(logical_max - logical_min);
	return (int)(t * 1023.0 + 0.5);
}

int cs_hid_hat(int value, int logical_min, int logical_max)
{
	if (value < logical_min || value > logical_max)
		return 0;
	if (logical_max - logical_min >= 8)
		return value - logical_min; /* 0 is the neutral value in the range */
	return value - logical_min + 1;
}

int cs_hid_in_dead_zone(int x, int y, int dead_zone)
{
	double dx = (double)x, dy = (double)y, r = (double)dead_zone;

	return dx * dx + dy * dy <= r * r;
}

int cs_hid_apply(cs_pad_state *pad, int usage_page, int usage, int value,
	int logical_min, int logical_max)
{
	/* a PlayStation pad's buttons (square, cross, circle, triangle, L1, R1,
	L2, R2, share, options, L3, R3, PS, touchpad) in the Xbox order; its L2
	and R2 are also the analog triggers */
	static const int playstation_buttons[14] = { 3, 1, 2, 4, 5, 6, 0, 0, 7, 8, 9, 10, 11, 12 };

	if (usage_page == CS_PAGE_BUTTON && pad->xbox == CS_LAYOUT_PLAYSTATION) {
		if (usage < 1 || usage > 14 || !playstation_buttons[usage - 1])
			return 0;
		usage = playstation_buttons[usage - 1];
	}
	/* a Nintendo pad's buttons by position (its B is the Xbox pad's A), with
	ZL and ZR as the triggers */
	if (usage_page == CS_PAGE_BUTTON && pad->xbox == CS_LAYOUT_NINTENDO) {
		static const int nintendo_buttons[14] = { 1, 2, 3, 4, 5, 6, 0, 0, 7, 8, 9, 10, 11, 12 };
		if (usage == 7 || usage == 8) {
			if (usage == 7)
				pad->lt = value ? 1023 : 0;
			else
				pad->rt = value ? 1023 : 0;
			return 1;
		}
		if (usage < 1 || usage > 14 || !nintendo_buttons[usage - 1])
			return 0;
		usage = nintendo_buttons[usage - 1];
	}
	if (usage_page == CS_PAGE_BUTTON) {
		if (usage < 1 || usage > 32)
			return 0;
		if (value)
			pad->buttons |= 1UL << (usage - 1);
		else
			pad->buttons &= ~(1UL << (usage - 1));
		return 1;
	}
	if (usage_page != CS_PAGE_GENERIC_DESKTOP)
		return 0;
	switch (usage) {
	case 0x30: /* X */
		pad->lx = cs_hid_scale_stick(value, logical_min, logical_max);
		return 1;
	case 0x31: /* Y */
		pad->ly = cs_hid_scale_stick(value, logical_min, logical_max);
		return 1;
	case 0x33: /* Rx */
		if (pad->xbox == CS_LAYOUT_PLAYSTATION) {
			pad->lt = cs_hid_scale_trigger(value, logical_min, logical_max);
			return 1;
		}
		pad->seen_rstick = 1;
		pad->rx = cs_hid_scale_stick(value, logical_min, logical_max);
		return 1;
	case 0x34: /* Ry */
		if (pad->xbox == CS_LAYOUT_PLAYSTATION) {
			pad->rt = cs_hid_scale_trigger(value, logical_min, logical_max);
			return 1;
		}
		pad->seen_rstick = 1;
		pad->ry = cs_hid_scale_stick(value, logical_min, logical_max);
		return 1;
	case 0x32: /* Z */
		if (pad->xbox == CS_LAYOUT_XBOX)
			pad->lt = cs_hid_scale_trigger(value, logical_min, logical_max);
		else if (pad->xbox == CS_LAYOUT_PLAYSTATION || !pad->seen_rstick)
			pad->rx = cs_hid_scale_stick(value, logical_min, logical_max);
		else
			return 0;
		return 1;
	case 0x35: /* Rz */
		if (pad->xbox == CS_LAYOUT_XBOX)
			pad->rt = cs_hid_scale_trigger(value, logical_min, logical_max);
		else if (pad->xbox == CS_LAYOUT_PLAYSTATION || !pad->seen_rstick)
			pad->ry = cs_hid_scale_stick(value, logical_min, logical_max);
		else
			return 0;
		return 1;
	case 0x39: /* hat switch */
		pad->hat = cs_hid_hat(value, logical_min, logical_max);
		return 1;
	}
	return 0;
}

void cs_pad_from_frame(cs_pad_state *pad, const struct mac_gamepad_frame *frame, unsigned extra)
{
	unsigned dpad = frame->buttons;
	int up = (dpad & MAC_GAMEPAD_DPAD_UP) != 0, down = (dpad & MAC_GAMEPAD_DPAD_DOWN) != 0;
	int left = (dpad & MAC_GAMEPAD_DPAD_LEFT) != 0, right = (dpad & MAC_GAMEPAD_DPAD_RIGHT) != 0;
	unsigned long b = 0;

	/* (the decoder's up is positive; this view's y grows down) */
	pad->lx = frame->left_x;
	pad->ly = -frame->left_y - 1;
	pad->rx = frame->right_x;
	pad->ry = -frame->right_y - 1;
	pad->lt = frame->analog[MAC_GAMEPAD_LEFT_TRIGGER] * 1023 / 255;
	pad->rt = frame->analog[MAC_GAMEPAD_RIGHT_TRIGGER] * 1023 / 255;
	if (up && right) pad->hat = 2;
	else if (down && right) pad->hat = 4;
	else if (down && left) pad->hat = 6;
	else if (up && left) pad->hat = 8;
	else if (up) pad->hat = 1;
	else if (right) pad->hat = 3;
	else if (down) pad->hat = 5;
	else if (left) pad->hat = 7;
	else pad->hat = 0;
	/* A B X Y LB RB View Menu LS RS Xbox Share */
	if (frame->analog[MAC_GAMEPAD_A]) b |= 1UL << 0;
	if (frame->analog[MAC_GAMEPAD_B]) b |= 1UL << 1;
	if (frame->analog[MAC_GAMEPAD_X]) b |= 1UL << 2;
	if (frame->analog[MAC_GAMEPAD_Y]) b |= 1UL << 3;
	if (frame->analog[MAC_GAMEPAD_WHITE]) b |= 1UL << 4;
	if (frame->analog[MAC_GAMEPAD_BLACK]) b |= 1UL << 5;
	if (dpad & MAC_GAMEPAD_BACK) b |= 1UL << 6;
	if (dpad & MAC_GAMEPAD_START) b |= 1UL << 7;
	if (dpad & MAC_GAMEPAD_LEFT_THUMB) b |= 1UL << 8;
	if (dpad & MAC_GAMEPAD_RIGHT_THUMB) b |= 1UL << 9;
	if (extra & MAC_PAD_EXTRA_PS) b |= 1UL << 10;
	if (extra & MAC_PAD_EXTRA_TOUCHPAD) b |= 1UL << 11;
	pad->buttons = b;
	pad->seen_rstick = 1;
}
