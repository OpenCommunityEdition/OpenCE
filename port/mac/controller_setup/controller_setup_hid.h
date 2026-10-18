/* Maps HID element values to a pad state for OpenCE Controller Setup.
   Pure C89. */
#ifndef CONTROLLER_SETUP_HID_H
#define CONTROLLER_SETUP_HID_H

#ifdef __cplusplus
extern "C" {
#endif

struct mac_gamepad_frame;

#define CS_VENDOR_MICROSOFT 0x045E
#define CS_PRODUCT_XBOX_VIRTUAL 0x028E
/* PlayStation pads (and adapters that present themselves as one, such as
   8BitDo's USB Wireless Adapter 2): sticks X/Y and Z/Rz, triggers Rx/Ry */
#define CS_VENDOR_SONY 0x054C
/* 8BitDo's own ids: the adapter is 2DC8:3105 in Dinput mode (a game pad whose reports
   mac_gamepad_report.h decodes) and 2DC8:3107 "IDLE" while it waits for a controller
   (a vendor-defined page, not a game pad) */
#define CS_VENDOR_8BITDO 0x2DC8
/* the game reads these three from their USB reports (mac_gamepad_report.h);
   this app sees their HID elements, which carry the same state */
#define CS_PRODUCT_DUALSHOCK4 0x05C4
#define CS_PRODUCT_DUALSHOCK4_V2 0x09CC
#define CS_PRODUCT_DUALSENSE 0x0CE6
/* Nintendo pads (the Switch Pro controller, and adapters in Switch mode):
   buttons B, A, Y, X, L, R, ZL, ZR, minus, plus, L3, R3, home, capture;
   ZL and ZR are digital, so they are full trigger pulls */
#define CS_VENDOR_NINTENDO 0x057E

enum { CS_LAYOUT_GENERIC, CS_LAYOUT_XBOX, CS_LAYOUT_PLAYSTATION, CS_LAYOUT_NINTENDO };

#define CS_PAGE_GENERIC_DESKTOP 0x01
#define CS_PAGE_BUTTON 0x09

typedef struct cs_pad_state {
	int lx, ly;            /* -32768..32767, positive y is down */
	int rx, ry;
	int lt, rt;            /* 0..1023, Xbox and PlayStation pads */
	int hat;               /* 0 neutral, 1..8 clockwise from north */
	unsigned long buttons; /* bit n is button n + 1, in the Xbox pad's order
	                          (A B X Y LB RB View Menu LS RS Xbox Share) */
	int xbox;              /* the layout: CS_LAYOUT_* (Xbox: Z and Rz are
	                          triggers; PlayStation: Rx and Ry are) */
	int seen_rstick;       /* Rx or Ry arrived (generic pads) */
} cs_pad_state;

/* "DualShock 4" or "DualSense" for those Sony ids, else "" */
const char *cs_sony_model(int vendor, int product);

void cs_pad_clear(cs_pad_state *pad, int xbox);

/* Applies one element value. Returns 1 if it was a used element. */
int cs_hid_apply(cs_pad_state *pad, int usage_page, int usage, int value,
	int logical_min, int logical_max);

/* Sets the pad from a decoded Sony report (mac_gamepad_report.h), the state the
   game plays with: sticks (positive y down here), triggers, hat and buttons in
   the Xbox pad's order. extra is the report's MAC_PAD_EXTRA_* bits (PS is the
   Xbox button, the touchpad click the Share button). */
void cs_pad_from_frame(cs_pad_state *pad, const struct mac_gamepad_frame *frame, unsigned extra);

/* Scales value in [min, max] to -32768..32767 / 0..1023. */
int cs_hid_scale_stick(int value, int logical_min, int logical_max);
int cs_hid_scale_trigger(int value, int logical_min, int logical_max);

/* Converts a hat value to 0 (neutral) or 1..8 clockwise from north. */
int cs_hid_hat(int value, int logical_min, int logical_max);

/* 1 if the stick lies inside a circular dead zone of the given radius. */
int cs_hid_in_dead_zone(int x, int y, int dead_zone);

#ifdef __cplusplus
}
#endif
#endif
