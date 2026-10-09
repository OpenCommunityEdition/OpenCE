/*
VR_MENU.C

Quest VR settings are native DeLa widgets built from verified definitions,
using the current map's UI font and the existing setting callbacks. OpenCE's
main menu and Settings hub expose them; campaign pause keeps a direct entry,
and stock multiplayer menus get a fallback entry when OpenCE is unavailable.
The settings, four-row columns, paging, laser hit tests and immediate config
persistence remain shared across all entry paths. No solo-pause templates or
fixed child counts are required to create a functioning screen.
*/

#ifdef HALO_VR

#include "cseries.h"
#include "math/integer_math.h"
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"

#include "halo_vr.h"
#include "../src/vr.h"
#include "../src/port_config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* port/linux/src/platform.h (a variadic call needs its prototype in scope
on the Android guest's ABI) */
void platform_log(const char *format, ...);
/* source/cache/cache_files.c */
long cache_file_add_tag(unsigned long group_tag, unsigned long parent_group_tag, char *name, void *base_address);
char const *ui_widget_event_handler_function_name(long function_index);

/* ---------- the widget definition tag ('DeLa'), as ui_widget.c views it */

#define VR_MENU_WIDGET_TAG 0x44654C61 /* 'DeLa' */

struct vr_menu_event_handler
{
	long flags;
	short event_type;
	short function;
	struct tag_reference widget_tag;
	struct tag_reference sound_effect;
	char script[32];
};

struct vr_menu_child
{
	struct tag_reference widget_tag;
	char name[32];
	long flags;
	short custom_controller_index;
	short vertical_offset;
	short horizontal_offset;
	byte unknown03A[0x50 - 0x3A];
};

struct vr_menu_game_data_input
{
	short function;
	byte unknown002[0x24 - 0x02];
};

struct vr_menu_widget
{
	short type;
	short controller_index;
	char name[32];
	rectangle2d bounds;
	long flags;
	long milliseconds_to_auto_close;
	long auto_close_fade_time;
	struct tag_reference background_bitmap;
	struct tag_block game_data_inputs;
	struct tag_block event_handlers;
	struct tag_block search_and_replace_functions;
	byte unknown06C[0xEC - 0x6C];
	struct tag_reference text_label_string_list;
	struct tag_reference text_font;
	real_argb_color text_color;
	short justification;
	word text_box_flags;
	byte unknown120[0x12E - 0x120];
	short string_list_index;
	short horizontal_offset, vertical_offset;
	byte unknown134[0x150 - 0x134];
	long list_flags;
	struct tag_reference list_header_bitmap, list_footer_bitmap;
	rectangle2d list_header_bounds, list_footer_bounds;
	byte unknown184[0x1A4 - 0x184];
	struct tag_reference extended_description_widget;
	byte unknown1B4[0x2D4 - 0x1B4];
	struct tag_block conditional_widgets;
	byte unknown2E0[0x3E0 - 0x2E0];
	struct tag_block child_widgets;
};

typedef char vr_menu_event_handler_size[sizeof(struct vr_menu_event_handler) == 0x48 ? 1 : -1];
typedef char vr_menu_child_size[sizeof(struct vr_menu_child) == 0x50 ? 1 : -1];
typedef char vr_menu_game_data_input_size[sizeof(struct vr_menu_game_data_input) == 0x24 ? 1 : -1];
typedef char vr_menu_widget_size[sizeof(struct vr_menu_widget) == 0x3EC ? 1 : -1];
typedef char vr_menu_widget_bounds[offsetof(struct vr_menu_widget, bounds) == 0x24 ? 1 : -1];
typedef char vr_menu_widget_game_data_inputs[offsetof(struct vr_menu_widget, game_data_inputs) == 0x48 ? 1 : -1];
typedef char vr_menu_widget_event_handlers[offsetof(struct vr_menu_widget, event_handlers) == 0x54 ? 1 : -1];
typedef char vr_menu_widget_text[offsetof(struct vr_menu_widget, text_label_string_list) == 0xEC ? 1 : -1];
typedef char vr_menu_widget_font[offsetof(struct vr_menu_widget, text_font) == 0xFC ? 1 : -1];
typedef char vr_menu_widget_list_flags[offsetof(struct vr_menu_widget, list_flags) == 0x150 ? 1 : -1];

/* event handler flags and events (ui_widget.c) */
#define VR_MENU_CLOSE_CURRENT 0x1
#define VR_MENU_CLOSE_ALL 0x4
#define VR_MENU_OPEN_WIDGET 0x8
#define VR_MENU_RUN_FUNCTION 0x80
#define VR_MENU_EVENT_A 0
#define VR_MENU_EVENT_B 1
#define VR_MENU_EVENT_DPAD_LEFT 10
#define VR_MENU_EVENT_DPAD_RIGHT 11
#define VR_MENU_EVENT_START 12
#define VR_MENU_EVENT_BACK 13

/* ---------- the settings */

enum
{
	_vr_setting_boolean,
	_vr_setting_real,
	_vr_setting_real_choice,
	_vr_setting_string,
	_vr_setting_degrees,
	_vr_setting_centimetres,
	_vr_setting_scope_centimetres,
	_vr_setting_vehicle_centimetres,
	_vr_setting_reset_alignment,
	_vr_setting_flip_alignment,
	/* test20c: restore a hand's orientation (key "left"/"right") or the gun's
	angle and place (key "weapon") to their built-in defaults */
	_vr_setting_reset_hand,
	_vr_setting_reset_weapon,
	/* test20d: a row that sets several settings at once, each value
	"key=value;key=value" (vr_menu_multi); both hands' angle on one axis
	(key "pitch", "yaw" or "roll"; the left hand mirrored); handedness,
	which also mirrors the hand-specific defaults */
	_vr_setting_multi,
	_vr_setting_hand_degrees,
	_vr_setting_handedness,
	/* test21: the held gun's own aim adjustment (vr.aim_<kind>_up/_right):
	key "gun" names the gun, "up" and "right" step half degrees, "reset"
	clears both for that gun */
	_vr_setting_gun_aim,
	/* test22: the pistol's and sniper rifle's scope places and sizes back
	to their defaults */
	_vr_setting_reset_scopes,
	/* test23: a Quest button for an action (key vr.button_*, values
	VR_BUTTON_SOURCE_*); a button another action has is swapped to it, and
	all of them back to their defaults */
	_vr_setting_button,
	_vr_setting_reset_buttons,
	/* test24b: comfort turning, finer: smooth or snap (vr.snap_turn 0 or
	the snap angle kept in vr.snap_turn_amount), and the snap angle itself
	(vr.snap_turn_amount, and vr.snap_turn while snapping) */
	_vr_setting_turn_mode,
	_vr_setting_snap_angle,
	/* test25: every vehicle seat offset (vr.vehicle_*_up/_forward/_right)
	back to its default; view, horizon and steering kept */
	_vr_setting_reset_vehicle_offsets,
	/* test26: what the left stick's click does: shows or hides the reticle
	(the 1.0.8 default; crouch on the turning stick held down) or crouches,
	as before 1.0.8 (values "reticle", "crouch"; vr_menu_button_set) */
	_vr_setting_crouch_click,
	/* test29: the HUD shown or hidden now (vr_set_hud_hidden: the session's,
	as the head tap's; values "true" shown), and the wrist HUD's place and
	size back to their defaults */
	_vr_setting_hud_shown,
	_vr_setting_reset_wrist,
};

#define VR_MENU_MAXIMUM_VALUES 12
/* test25: a settings row's width: its column's (the right column starts
256 across), less a gap */
#define VR_MENU_BUTTON_WIDTH 250

/* the right stick's turning, on CONTROLS and COMFORT (test24b: snap 22.5
and 60 degrees added) */
#define VR_MENU_TURNING_ROW { "TURNING", "vr.snap_turn", _vr_setting_multi, 12, { { "SMOOTH 60", "vr.snap_turn=0;vr.smooth_turn_speed=60" }, { "SMOOTH 90", "vr.snap_turn=0;vr.smooth_turn_speed=90" }, { "SMOOTH 120", "vr.snap_turn=0;vr.smooth_turn_speed=120" }, { "SMOOTH 150", "vr.snap_turn=0;vr.smooth_turn_speed=150" }, { "SMOOTH 180", "vr.snap_turn=0;vr.smooth_turn_speed=180" }, { "SMOOTH 240", "vr.snap_turn=0;vr.smooth_turn_speed=240" }, { "SMOOTH 300", "vr.snap_turn=0;vr.smooth_turn_speed=300" }, { "SNAP 22.5", "vr.snap_turn=22.5;vr.snap_turn_amount=22.5" }, { "SNAP 30", "vr.snap_turn=30;vr.snap_turn_amount=30" }, { "SNAP 45", "vr.snap_turn=45;vr.snap_turn_amount=45" }, { "SNAP 60", "vr.snap_turn=60;vr.snap_turn_amount=60" }, { "SNAP 90", "vr.snap_turn=90;vr.snap_turn_amount=90" } } }

struct vr_menu_setting
{
	char const *label, *key;
	short type, value_count;
	struct
	{
		char const *label, *value;
	} values[VR_MENU_MAXIMUM_VALUES];
};

/* an effect's setting (vr_graphics.c): as the preset, or on or off */
#define VR_MENU_EFFECT(label, key) \
	{ label, key, _vr_setting_string, 3, { { "AUTO", "auto" }, { "ON", "on" }, { "OFF", "off" } } }

/* test20d: one row per decision. Rows that only mattered with another
(turn speed with smooth turning, holster size with holsters, multiplayer
with physical weapons, run effort with arm run, the arms with the hand
mode) are folded into it; a combination set in config.toml by hand shows
as CUSTOM. */
static struct vr_menu_setting const vr_menu_controls[] =
{
	{ "HANDEDNESS", "vr.left_handed", _vr_setting_handedness, 2, { { "RIGHT", "false" }, { "LEFT", "true" } } },
	{ "MIRROR CONTROLS", "vr.mirror_controls", _vr_setting_string, 2, { { "AUTO", "auto" }, { "OFF", "off" } } },
	VR_MENU_TURNING_ROW,
	{ "MOVE WITH", "vr.move_relative", _vr_setting_string, 3, { { "HEAD", "head" }, { "LEFT HAND", "left" }, { "RIGHT HAND", "right" } } },
	/* test21: AUTO LOCK (default) locks the off hand at the support grip
	without squeezing; SQUEEZE needs the grip held there */
	{ "TWO HANDS", "vr.two_handed", _vr_setting_string, 3, { { "AUTO LOCK", "auto" }, { "SQUEEZE", "grip" }, { "OFF", "off" } } },
	{ "WEAPONS", "vr.weapons", _vr_setting_multi, 3, { { "LOCKED", "vr.weapons=locked" }, { "PHYSICAL", "vr.weapons=physical;vr.physical_multiplayer=false" }, { "PHYSICAL + MP", "vr.weapons=physical;vr.physical_multiplayer=true" } } },
	{ "HOLSTERS", "vr.holsters", _vr_setting_multi, 7, { { "OFF", "vr.holsters=false" }, { "10 CM", "vr.holsters=true;vr.holster_size=0.1" }, { "15 CM", "vr.holsters=true;vr.holster_size=0.15" }, { "20 CM", "vr.holsters=true;vr.holster_size=0.2" }, { "25 CM", "vr.holsters=true;vr.holster_size=0.25" }, { "30 CM", "vr.holsters=true;vr.holster_size=0.3" }, { "40 CM", "vr.holsters=true;vr.holster_size=0.4" } } },
	{ "AIM", "vr.aim", _vr_setting_string, 2, { { "HAND", "hand" }, { "HEAD", "head" } } },
	{ "CONTROLS", "vr.controls", _vr_setting_string, 2, { { "VR", "vr" }, { "XBOX", "pad" } } },
	/* test26: the left stick's click: the reticle's toggle or crouch, as before */
	{ "L STICK CLICK", "buttons", _vr_setting_crouch_click, 2, { { "RETICLE", "reticle" }, { "CROUCH", "crouch" } } },
};

static struct vr_menu_setting const vr_menu_body[] =
{
	{ "BODY", "vr.body", _vr_setting_string, 4, { { "ARMS + HANDS", "arms" }, { "FULL", "full" }, { "LEGS + ARMS", "legs" }, { "HANDS ONLY", "hands" } } },
	/* how the hands follow: body IK, floating (arms as BODY shows them),
	Halo's own arm animation, or the gun alone */
	/* test21: FLOATING is hands with no arms again (test20c); FLOAT + ARMS
	hangs arms from a floating shoulder */
	{ "HANDS", "vr.hand_tracking", _vr_setting_multi, 5, { { "BODY IK", "vr.arms=ik;vr.hand_tracking=ik" }, { "FLOATING", "vr.arms=ik;vr.hand_tracking=floating" }, { "FLOAT + ARMS", "vr.arms=ik;vr.hand_tracking=floating_arms" }, { "ANIMATED", "vr.arms=animated;vr.hand_tracking=ik" }, { "GUN ONLY", "vr.arms=hidden;vr.hand_tracking=ik" } } },
	{ "FINGERS", "vr.fingers", _vr_setting_boolean, 2, { { "OFF", "false" }, { "TRACKED", "true" } } },
	{ "ROOM-SCALE", "vr.roomscale", _vr_setting_boolean, 2, { { "OFF", "false" }, { "ON", "true" } } },
	{ "CROUCH DEPTH", "vr.crouch_height", _vr_setting_real, 9, { { "OFF", "0" }, { "5 CM", "0.05" }, { "10 CM", "0.1" }, { "15 CM", "0.15" }, { "20 CM", "0.2" }, { "25 CM", "0.25" }, { "30 CM", "0.3" }, { "35 CM", "0.35" }, { "40 CM", "0.4" } } },
	{ "ARM RUN", "vr.arm_run", _vr_setting_multi, 7, { { "OFF", "vr.arm_run=false" }, { "EASY 0.3", "vr.arm_run=true;vr.arm_run_speed=0.3" }, { "0.45", "vr.arm_run=true;vr.arm_run_speed=0.45" }, { "0.6", "vr.arm_run=true;vr.arm_run_speed=0.6" }, { "0.8", "vr.arm_run=true;vr.arm_run_speed=0.8" }, { "1.0", "vr.arm_run=true;vr.arm_run_speed=1.0" }, { "HARD 1.2", "vr.arm_run=true;vr.arm_run_speed=1.2" } } },
	/* test21: physical melee in network games only with "+ ONLINE"
	(vr.melee_multiplayer, off by default); the melee button always works */
	{ "MELEE", "vr.melee", _vr_setting_multi, 4, { { "IMPACT", "vr.melee=impact;vr.melee_multiplayer=false" }, { "SWING", "vr.melee=swing;vr.melee_multiplayer=false" }, { "IMPACT + ONLINE", "vr.melee=impact;vr.melee_multiplayer=true" }, { "SWING + ONLINE", "vr.melee=swing;vr.melee_multiplayer=true" } } },
	{ "MELEE SPEED", "vr.melee_speed", _vr_setting_real, 12, { { "OFF", "0" }, { "1.0", "1.0" }, { "1.2", "1.2" }, { "1.4", "1.4" }, { "1.6", "1.6" }, { "1.8", "1.8" }, { "2.0", "2.0" }, { "2.3", "2.3" }, { "2.6", "2.6" }, { "2.9", "2.9" }, { "3.2", "3.2" }, { "3.6", "3.6" } } },
};

/* the feel of play, with the crosshair (formerly its own page) */
static struct vr_menu_setting const vr_menu_vr[] =
{
	{ "HAPTICS", "vr.haptics", _vr_setting_real, 11, { { "0%", "0.0" }, { "10%", "0.1" }, { "20%", "0.2" }, { "30%", "0.3" }, { "40%", "0.4" }, { "50%", "0.5" }, { "60%", "0.6" }, { "70%", "0.7" }, { "80%", "0.8" }, { "90%", "0.9" }, { "100%", "1.0" } } },
	{ "SCOPE", "vr.scope", _vr_setting_boolean, 2, { { "OFF", "false" }, { "ON", "true" } } },
	{ "CUTSCENES", "vr.cutscenes", _vr_setting_string, 3, { { "IMMERSIVE", "immersive" }, { "3D SCREEN", "screen" }, { "FLAT", "flat" } } },
	{ "CLOSE CONTACT", "vr.close_contact", _vr_setting_boolean, 2, { { "OFF", "false" }, { "ON", "true" } } },
};

/* test26: what the HUD shows and where: the crosshair (formerly on
GAMEPLAY; its button is on BUTTONS) and the wrist HUD. test29: the HUD
itself shown or hidden (as the head tap does) and the head tap's reach or
off (as on HEAD GESTURES), first; the wrist HUD's place and size */
static struct vr_menu_setting const vr_menu_hud[] =
{
	{ "HUD", "hud", _vr_setting_hud_shown, 2, { { "SHOWN", "true" }, { "HIDDEN", "false" } } },
	{ "HEAD TAP", "vr.hud_tap_distance", _vr_setting_real, 6, { { "OFF", "0" }, { "6 CM", "0.06" }, { "8 CM", "0.08" }, { "10 CM", "0.1" }, { "12 CM", "0.12" }, { "15 CM", "0.15" } } },
	{ "CROSSHAIR", "vr.crosshair", _vr_setting_string, 2, { { "NATIVE", "native" }, { "OFF", "off" } } },
	{ "CROSSHAIR SIZE", "vr.crosshair_size", _vr_setting_real, 8, { { "25%", "0.25" }, { "50%", "0.5" }, { "75%", "0.75" }, { "100%", "1" }, { "125%", "1.25" }, { "150%", "1.5" }, { "200%", "2" }, { "300%", "3" } } },
	{ "OPACITY", "vr.crosshair_opacity", _vr_setting_real, 11, { { "0%", "0" }, { "10%", "0.1" }, { "20%", "0.2" }, { "30%", "0.3" }, { "40%", "0.4" }, { "50%", "0.5" }, { "60%", "0.6" }, { "70%", "0.7" }, { "80%", "0.8" }, { "90%", "0.9" }, { "100%", "1" } } },
	{ "WRIST HUD", "vr.wrist_hud", _vr_setting_boolean, 2, { { "OFF", "false" }, { "ON", "true" } } },
	{ "WRIST ALONG", "vr.wrist_hud_along", _vr_setting_centimetres, 0, { { NULL, NULL } } },
	{ "WRIST ACROSS", "vr.wrist_hud_across", _vr_setting_centimetres, 0, { { NULL, NULL } } },
	{ "WRIST HEIGHT", "vr.wrist_hud_out", _vr_setting_centimetres, 0, { { NULL, NULL } } },
	{ "WRIST SIZE", "vr.wrist_hud_size", _vr_setting_real, 7, { { "50%", "0.5" }, { "75%", "0.75" }, { "100%", "1" }, { "125%", "1.25" }, { "150%", "1.5" }, { "175%", "1.75" }, { "200%", "2" } } },
	{ "RESET WRIST", "wrist", _vr_setting_reset_wrist, 0, { { NULL, NULL } } },
};

/* test26: the head taps, each with its reach or off (the flashlight's
button stays): the off hand to the head toggles the flashlight, the weapon
hand to its own temple the HUD */
static struct vr_menu_setting const vr_menu_gestures[] =
{
	{ "FLASHLIGHT", "vr.flashlight_distance", _vr_setting_real, 6, { { "OFF", "0" }, { "10 CM", "0.1" }, { "15 CM", "0.15" }, { "20 CM", "0.2" }, { "25 CM", "0.25" }, { "30 CM", "0.3" } } },
	{ "HUD TAP", "vr.hud_tap_distance", _vr_setting_real, 6, { { "OFF", "0" }, { "6 CM", "0.06" }, { "8 CM", "0.08" }, { "10 CM", "0.1" }, { "12 CM", "0.12" }, { "15 CM", "0.15" } } },
};

static struct vr_menu_setting const vr_menu_vehicles[] =
{
	{ "VIEW", "vr.vehicle_view", _vr_setting_string, 2, { { "THIRD PERSON", "chase" }, { "FIRST PERSON", "first_person" } } },
	/* test25: first person, a driver's view: the horizon level (the
	cockpit tilts against the view), or tilting with the vehicle, half or
	whole (the cockpit stays put) */
	{ "HORIZON", "vr.vehicle_tilt", _vr_setting_real, 3, { { "LEVEL", "0" }, { "HALF", "0.5" }, { "VEHICLE", "1" } } },
	{ "STEERING", "vr.vehicle_steering", _vr_setting_string, 4, { { "RIGHT HAND", "right" }, { "LEFT HAND", "left" }, { "HEAD", "head" }, { "STICK", "stick" } } },
	{ "TURRET AIM", "vr.turret_aim", _vr_setting_string, 4, { { "RIGHT HAND", "right" }, { "LEFT HAND", "left" }, { "HEAD", "head" }, { "STICK", "stick" } } },
	{ "ALL UP", "vr.vehicle_all_up", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "ALL FWD", "vr.vehicle_all_forward", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "ALL RIGHT", "vr.vehicle_all_right", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "RESET OFFSETS", "vehicle offsets", _vr_setting_reset_vehicle_offsets, 0, { { NULL, NULL } } },
	{ "HOG GLASS", "vr.vehicle_warthog_hide_glass", _vr_setting_boolean, 2, { { "VISIBLE", "false" }, { "HIDDEN", "true" } } },
	{ "HOG UP", "vr.vehicle_warthog_up", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "HOG FWD", "vr.vehicle_warthog_forward", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "HOG RIGHT", "vr.vehicle_warthog_right", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "GHOST UP", "vr.vehicle_ghost_up", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "GHOST FWD", "vr.vehicle_ghost_forward", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "GHOST RIGHT", "vr.vehicle_ghost_right", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "BANSHEE UP", "vr.vehicle_banshee_up", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "BANSHEE FWD", "vr.vehicle_banshee_forward", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "BANSHEE RIGHT", "vr.vehicle_banshee_right", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "TANK UP", "vr.vehicle_scorpion_up", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "TANK FWD", "vr.vehicle_scorpion_forward", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "TANK RIGHT", "vr.vehicle_scorpion_right", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "PELICAN UP", "vr.vehicle_pelican_up", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "PELICAN FWD", "vr.vehicle_pelican_forward", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
	{ "PELICAN RIGHT", "vr.vehicle_pelican_right", _vr_setting_vehicle_centimetres, 0, { { NULL, NULL } } },
};

static struct vr_menu_setting const vr_menu_graphics[] =
{
	{ "PRESET", "graphics.preset", _vr_setting_string, 5, { { "AUTO", "auto" }, { "LOW", "low" }, { "MEDIUM", "medium" }, { "HIGH", "high" }, { "MAX", "max" } } },
	/* a slider of the eye images' size against the headset's
	recommendation (Quest 3: 1680x1760), the Quest 3's own panels (2064x2208)
	marked ("125% Q3": test29, the row's width), supersampling above; FOV's glasses window draws about 40%
	of the pixels, which pays for the higher notches */
	{ "RESOLUTION", "vr.resolution_scale", _vr_setting_real, 12, { { "AUTO", "0" }, { "60%", "0.6" }, { "70%", "0.7" }, { "80%", "0.8" }, { "90%", "0.9" }, { "100%", "1" }, { "110%", "1.1" }, { "125% Q3", "1.25" }, { "140%", "1.4" }, { "160%", "1.6" }, { "180%", "1.8" }, { "200%", "2" } } },
	{ "FOV", "vr.fov_mode", _vr_setting_string, 2, { { "FULL", "full" }, { "GLASSES 70X66", "glasses" } } },
	VR_MENU_EFFECT("SHADOWS", "graphics.shadows"),
	VR_MENU_EFFECT("LIGHTS", "graphics.dynamic_lights"),
	VR_MENU_EFFECT("SPECULAR", "graphics.specular"),
	VR_MENU_EFFECT("REFLECTIONS", "graphics.reflections"),
	VR_MENU_EFFECT("BUMP MAPS", "graphics.bump_mapping"),
	VR_MENU_EFFECT("GRASS", "graphics.detail_objects"),
	VR_MENU_EFFECT("FOG LAYERS", "graphics.fog_screen"),
};

static struct vr_menu_setting const vr_menu_effects[] =
{
	VR_MENU_EFFECT("DECALS", "graphics.decals"),
	VR_MENU_EFFECT("PARTICLES", "graphics.particles"),
	VR_MENU_EFFECT("CONTRAILS", "graphics.contrails"),
	VR_MENU_EFFECT("WEATHER", "graphics.weather"),
	VR_MENU_EFFECT("LENS FLARES", "graphics.lens_flares"),
	VR_MENU_EFFECT("CAMO", "graphics.camouflage_multipass"),
	{ "REFRESH", "vr.refresh_rate", _vr_setting_real, 4, { { "72 HZ", "72" }, { "80 HZ", "80" }, { "90 HZ", "90" }, { "120 HZ", "120" } } },
};

/* test20c/d: the visible hands (never the gun: one row turns both, the
left mirrored; per-hand values stay in config.toml as vr.hand_left_* and
vr.hand_right_*), then the held gun: its angle (shots and reticle follow)
and its place in the hand (vr.gun_*, with vr.gun_anchor) */
static struct vr_menu_setting const vr_menu_hands[] =
{
    { "HAND PITCH", "pitch", _vr_setting_hand_degrees, 0, { { NULL,NULL } } },
    { "HAND YAW", "yaw", _vr_setting_hand_degrees, 0, { { NULL,NULL } } },
    { "HAND ROLL", "roll", _vr_setting_hand_degrees, 0, { { NULL,NULL } } },
    { "RESET HANDS", "both", _vr_setting_reset_hand, 0, { { NULL,NULL } } },
    { "GUN PITCH", "vr.weapon_pitch", _vr_setting_degrees, 0, { { NULL,NULL } } },
    { "GUN YAW", "vr.weapon_yaw", _vr_setting_degrees, 0, { { NULL,NULL } } },
    { "GUN ROLL", "vr.weapon_roll", _vr_setting_degrees, 0, { { NULL,NULL } } },
    { "RESET GUN", "weapon", _vr_setting_reset_weapon, 0, { { NULL,NULL } } },
    { "GUN FORWARD", "vr.gun_forward", _vr_setting_centimetres, 0, { { NULL,NULL } } },
    { "GUN UP", "vr.gun_up", _vr_setting_centimetres, 0, { { NULL,NULL } } },
    { "GUN OUT", "vr.gun_out", _vr_setting_centimetres, 0, { { NULL,NULL } } },
    { "GUN GRIP", "vr.gun_anchor", _vr_setting_boolean, 2, { { "ANCHORED", "true" }, { "CLASSIC", "false" } } },
    { "AIM FOR", "gun", _vr_setting_gun_aim, 0, { { NULL,NULL } } },
    { "AIM UP", "up", _vr_setting_gun_aim, 0, { { NULL,NULL } } },
    { "AIM RIGHT", "right", _vr_setting_gun_aim, 0, { { NULL,NULL } } },
    { "RESET AIM", "reset", _vr_setting_gun_aim, 0, { { NULL,NULL } } },
};

/* test24b: comfort: turning (smooth or snap, each finely set: smooth
speed, snap angle) and the vignette, which darkens the view's edges while
you move or turn by stick */
static struct vr_menu_setting const vr_menu_comfort[] =
{
	{ "TURNING", "turn", _vr_setting_turn_mode, 2, { { "SMOOTH", "smooth" }, { "SNAP", "snap" } } },
	{ "SMOOTH SPEED", "vr.smooth_turn_speed", _vr_setting_real, 12, { { "30", "30" }, { "45", "45" }, { "60", "60" }, { "75", "75" }, { "90", "90" }, { "105", "105" }, { "120", "120" }, { "150", "150" }, { "180", "180" }, { "210", "210" }, { "240", "240" }, { "300", "300" } } },
	{ "SNAP ANGLE", "vr.snap_turn_amount", _vr_setting_snap_angle, 9, { { "10 DEG", "10" }, { "15 DEG", "15" }, { "20 DEG", "20" }, { "22.5 DEG", "22.5" }, { "30 DEG", "30" }, { "40 DEG", "40" }, { "45 DEG", "45" }, { "60 DEG", "60" }, { "90 DEG", "90" } } },
	{ "VIGNETTE", "vr.vignette", _vr_setting_real, 4, { { "OFF", "0" }, { "LOW", "0.35" }, { "MEDIUM", "0.65" }, { "HIGH", "1" } } },
	{ "VIGNETTE ON", "vr.vignette_when", _vr_setting_string, 3, { { "MOVING", "move_turn" }, { "TURNING", "turn" }, { "ALWAYS", "always" } } },
};

/* test22: the scopes' places and sizes, the pistol's and the sniper
rifle's apart (vr.scope_pistol_*, vr.scope_sniper_*: 0 and 100%, the usual) */
static struct vr_menu_setting const vr_menu_scopes[] =
{
    { "PISTOL FWD", "vr.scope_pistol_forward", _vr_setting_scope_centimetres, 0, { { NULL,NULL } } },
    { "PISTOL UP", "vr.scope_pistol_up", _vr_setting_scope_centimetres, 0, { { NULL,NULL } } },
    { "PISTOL RIGHT", "vr.scope_pistol_right", _vr_setting_scope_centimetres, 0, { { NULL,NULL } } },
    { "PISTOL SIZE", "vr.scope_pistol_scale", _vr_setting_real, 7, { { "50%", "0.5" }, { "75%", "0.75" }, { "100%", "1" }, { "125%", "1.25" }, { "150%", "1.5" }, { "175%", "1.75" }, { "200%", "2" } } },
    { "SNIPER FWD", "vr.scope_sniper_forward", _vr_setting_scope_centimetres, 0, { { NULL,NULL } } },
    { "SNIPER UP", "vr.scope_sniper_up", _vr_setting_scope_centimetres, 0, { { NULL,NULL } } },
    { "SNIPER RIGHT", "vr.scope_sniper_right", _vr_setting_scope_centimetres, 0, { { NULL,NULL } } },
    { "SNIPER SIZE", "vr.scope_sniper_scale", _vr_setting_real, 7, { { "50%", "0.5" }, { "75%", "0.75" }, { "100%", "1" }, { "125%", "1.25" }, { "150%", "1.5" }, { "175%", "1.75" }, { "200%", "2" } } },
    { "RESET SCOPES", "scopes", _vr_setting_reset_scopes, 0, { { NULL,NULL } } },
};

/* test23: the Quest's buttons (vr.button_*), remappable. "A"/"B" are the
gun hand's lower/upper buttons and "X"/"Y" the other hand's (mirrored with
Mirror Controls); the grip is the gun hand's, with locked weapons only */
static struct vr_menu_setting const vr_menu_buttons[] =
{
    { "JUMP", "vr.button_jump", _vr_setting_button, 9, { { "A", "a" }, { "B", "b" }, { "X", "x" }, { "Y", "y" }, { "R STICK", "right_stick" }, { "L STICK", "left_stick" }, { "R DOWN", "right_stick_down" }, { "GRIP", "grip" }, { "NONE", "none" } } },
    { "USE / RELOAD", "vr.button_action", _vr_setting_button, 9, { { "A", "a" }, { "B", "b" }, { "X", "x" }, { "Y", "y" }, { "R STICK", "right_stick" }, { "L STICK", "left_stick" }, { "R DOWN", "right_stick_down" }, { "GRIP", "grip" }, { "NONE", "none" } } },
    { "MELEE", "vr.button_melee", _vr_setting_button, 9, { { "A", "a" }, { "B", "b" }, { "X", "x" }, { "Y", "y" }, { "R STICK", "right_stick" }, { "L STICK", "left_stick" }, { "R DOWN", "right_stick_down" }, { "GRIP", "grip" }, { "NONE", "none" } } },
    { "CROUCH", "vr.button_crouch", _vr_setting_button, 9, { { "A", "a" }, { "B", "b" }, { "X", "x" }, { "Y", "y" }, { "R STICK", "right_stick" }, { "L STICK", "left_stick" }, { "R DOWN", "right_stick_down" }, { "GRIP", "grip" }, { "NONE", "none" } } },
    { "NEXT WEAPON", "vr.button_switch_weapon", _vr_setting_button, 9, { { "A", "a" }, { "B", "b" }, { "X", "x" }, { "Y", "y" }, { "R STICK", "right_stick" }, { "L STICK", "left_stick" }, { "R DOWN", "right_stick_down" }, { "GRIP", "grip" }, { "NONE", "none" } } },
    { "GRENADE", "vr.button_grenade", _vr_setting_button, 9, { { "A", "a" }, { "B", "b" }, { "X", "x" }, { "Y", "y" }, { "R STICK", "right_stick" }, { "L STICK", "left_stick" }, { "R DOWN", "right_stick_down" }, { "GRIP", "grip" }, { "NONE", "none" } } },
    { "NEXT GRENADE", "vr.button_switch_grenade", _vr_setting_button, 10, { { "HOLD", "hold" }, { "A", "a" }, { "B", "b" }, { "X", "x" }, { "Y", "y" }, { "R STICK", "right_stick" }, { "L STICK", "left_stick" }, { "R DOWN", "right_stick_down" }, { "GRIP", "grip" }, { "NONE", "none" } } },
    /* test26: shows or hides the reticle (starts shown) */
    { "RETICLE", "vr.button_reticle", _vr_setting_button, 9, { { "A", "a" }, { "B", "b" }, { "X", "x" }, { "Y", "y" }, { "R STICK", "right_stick" }, { "L STICK", "left_stick" }, { "R DOWN", "right_stick_down" }, { "GRIP", "grip" }, { "NONE", "none" } } },
    { "RESET BUTTONS", "buttons", _vr_setting_reset_buttons, 0, { { NULL,NULL } } },
};

static struct vr_menu_setting const vr_menu_align_left[] =
{
    { "PITCH", "vr.align_left_pitch", _vr_setting_degrees, 0, { { NULL,NULL } } },
    { "YAW", "vr.align_left_yaw", _vr_setting_degrees, 0, { { NULL,NULL } } },
    { "ROLL", "vr.align_left_roll", _vr_setting_degrees, 0, { { NULL,NULL } } },
    { "RIGHT", "vr.align_left_right", _vr_setting_centimetres, 0, { { NULL,NULL } } },
    { "UP", "vr.align_left_up", _vr_setting_centimetres, 0, { { NULL,NULL } } },
    { "BACK", "vr.align_left_back", _vr_setting_centimetres, 0, { { NULL,NULL } } },
    { "AIM SOURCE", "vr.align_left_grip_aim", _vr_setting_boolean, 2, { { "NATIVE", "false" }, { "GRIP", "true" } } },
    { "FLIP ROLL 180", "vr.align_left_roll", _vr_setting_flip_alignment, 0, { { NULL,NULL } } },
    { "RESET LEFT", "left", _vr_setting_reset_alignment, 0, { { NULL,NULL } } },
};

static struct vr_menu_setting const vr_menu_align_right[] =
{
    { "PITCH", "vr.align_right_pitch", _vr_setting_degrees, 0, { { NULL,NULL } } },
    { "YAW", "vr.align_right_yaw", _vr_setting_degrees, 0, { { NULL,NULL } } },
    { "ROLL", "vr.align_right_roll", _vr_setting_degrees, 0, { { NULL,NULL } } },
    { "RIGHT", "vr.align_right_right", _vr_setting_centimetres, 0, { { NULL,NULL } } },
    { "UP", "vr.align_right_up", _vr_setting_centimetres, 0, { { NULL,NULL } } },
    { "BACK", "vr.align_right_back", _vr_setting_centimetres, 0, { { NULL,NULL } } },
    { "AIM SOURCE", "vr.align_right_grip_aim", _vr_setting_boolean, 2, { { "NATIVE", "false" }, { "GRIP", "true" } } },
    { "FLIP ROLL 180", "vr.align_right_roll", _vr_setting_flip_alignment, 0, { { NULL,NULL } } },
    { "RESET RIGHT", "right", _vr_setting_reset_alignment, 0, { { NULL,NULL } } },
};

/* a page of settings, opened from the categories: its first `left_count`
settings in the left column, the rest in the right */
static struct vr_menu_page
{
	char const *label;
	struct vr_menu_setting const *settings;
	long count;
} const vr_menu_pages[] =
{
	{ "CONTROLS", vr_menu_controls, NUMBEROF(vr_menu_controls) },
	{ "BUTTONS", vr_menu_buttons, NUMBEROF(vr_menu_buttons) },
	{ "COMFORT", vr_menu_comfort, NUMBEROF(vr_menu_comfort) },
	{ "BODY", vr_menu_body, NUMBEROF(vr_menu_body) },
	{ "HANDS + GUN", vr_menu_hands, NUMBEROF(vr_menu_hands) },
	{ "SCOPES", vr_menu_scopes, NUMBEROF(vr_menu_scopes) },
	{ "GAMEPLAY", vr_menu_vr, NUMBEROF(vr_menu_vr) },
	{ "HUD + RETICLE", vr_menu_hud, NUMBEROF(vr_menu_hud) },
	{ "HEAD GESTURES", vr_menu_gestures, NUMBEROF(vr_menu_gestures) },
	{ "VEHICLES", vr_menu_vehicles, NUMBEROF(vr_menu_vehicles) },
	{ "GRAPHICS", vr_menu_graphics, NUMBEROF(vr_menu_graphics) },
	{ "DISPLAY", vr_menu_effects, NUMBEROF(vr_menu_effects) },
	/* advanced: tracking corrections that move hand and gun together */
	{ "CONTROLLER LEFT", vr_menu_align_left, NUMBEROF(vr_menu_align_left) },
	{ "CONTROLLER RIGHT", vr_menu_align_right, NUMBEROF(vr_menu_align_right) },
};

#define VR_MENU_PAGE_COUNT ((long)NUMBEROF(vr_menu_pages))
/* Four rows per column. The fifth left row is reserved for navigation;
 * the fifth right row belongs to the stock button hints. Capacity derives
 * from the definitions, so new settings/categories create further screens. */
#define VR_MENU_SETTINGS_PER_SCREEN 8
#define VR_MENU_PAGES(n) (((n) + VR_MENU_SETTINGS_PER_SCREEN - 1) / VR_MENU_SETTINGS_PER_SCREEN)


/* a combined row's value, "key=value;key=value": whether every setting
holds its value, or (write) each written; FALSE for a malformed value */
static boolean vr_menu_multi(char const *values, boolean write, boolean *written)
{
	char pair[96];
	char const *at = values;
	boolean all = TRUE;

	while (*at)
	{
		size_t length = strcspn(at, ";");
		char *equals;

		if (length >= sizeof(pair))
			return FALSE;
		memcpy(pair, at, length);
		pair[length] = 0;
		at += length + (at[length] == ';');
		equals = strchr(pair, '=');
		if (!equals)
			return FALSE;
		*equals = 0;
		if (write)
			*written = config_write_text(pair, equals + 1) && *written;
		else if (!config_matches(pair, equals + 1))
			all = FALSE;
	}
	return all;
}

/* both hands' angle on one axis, as the right hand's (the left's is its
mirror image: the same pitch, yaw and roll the other way). Left-handed, the
left hand's is shown, mirrored */
static double vr_menu_hand_angle(char const *axis)
{
	char key[64];
	double value, sign = strcmp(axis, "pitch") ? -1.0 : 1.0;
	boolean left = config_boolean("vr.left_handed");

	snprintf(key, sizeof(key), "vr.hand_%s_%s", left ? "left" : "right", axis);
	value = config_real(key);
	if (!isfinite(value))
		value = 0.0;
	return (left ? value * sign : value) + 0.0; /* never "-0" */
}

/* test23: a remappable button's action (VR_BUTTON_ACTION_*, NONE: not
one), and the button it has, as the game reads it */
/* test24b: the snap angle: the one snapping now, or the one kept for it
while turning smoothly (vr.snap_turn_amount) */
static double vr_menu_snap_angle(void)
{
	double angle = config_real("vr.snap_turn");

	if (!(angle > 0.0))
		angle = config_real("vr.snap_turn_amount");
	return angle > 0.0 && angle <= 180.0 ? angle : 45.0;
}

static int vr_menu_button_action(char const *key)
{
	int action;

	for (action = 0; action < VR_BUTTON_ACTIONS; action++)
	{
		if (!strcmp(key, vr_button_action_key(action)))
			return action;
	}
	return NONE;
}

static int vr_menu_button_source(int action)
{
	return vr_button_source_of(config_string(vr_button_action_key(action)), vr_button_default(action));
}

/* test23: `action` takes `source`; another action on that button takes
this one's old button (the switch grenade's "hold" is no button: the other
is left with none), so no button ever does two actions. The grenade put on
another action's button leaves "switch grenade" to holding it */
static boolean vr_menu_button_set(int action, int source)
{
	int other, old = vr_menu_button_source(action), displaced = FALSE;
	boolean written = config_write_string(vr_button_action_key(action), vr_button_source_value(source));

	if (source == VR_BUTTON_SOURCE_NONE || source == VR_BUTTON_SOURCE_HOLD)
		return written;
	for (other = 0; other < VR_BUTTON_ACTIONS; other++)
	{
		int moved;

		if (other == action || vr_menu_button_source(other) != source)
			continue;
		moved = old == VR_BUTTON_SOURCE_HOLD ? VR_BUTTON_SOURCE_NONE : old;
		if (other == VR_BUTTON_ACTION_SWITCH_GRENADE && action == VR_BUTTON_ACTION_GRENADE)
			moved = VR_BUTTON_SOURCE_HOLD;
		displaced |= moved == old;
		written = config_write_string(vr_button_action_key(other), vr_button_source_value(moved)) && written;
		platform_log("vr: %s was on %s, moved to %s", vr_button_action_key(other),
			vr_button_source_value(source), vr_button_source_value(moved));
	}
	/* the grip throws while held and cannot be held to switch grenades:
	switching takes the grenade's old button (the layout locked weapons had
	before 1.0.5), unless the grip's old action just took that button */
	if (action == VR_BUTTON_ACTION_GRENADE && source == VR_BUTTON_SOURCE_GRIP && !displaced &&
		vr_menu_button_source(VR_BUTTON_ACTION_SWITCH_GRENADE) == VR_BUTTON_SOURCE_HOLD &&
		old != VR_BUTTON_SOURCE_GRIP)
	{
		written = config_write_string(vr_button_action_key(VR_BUTTON_ACTION_SWITCH_GRENADE), vr_button_source_value(old)) && written;
		platform_log("vr: vr.button_switch_grenade moved to %s (the grip cannot be held to switch)", vr_button_source_value(old));
	}
	return written;
}

/* the setting's value now, as an index into its values (NONE: none of them) */
static long vr_menu_value_index(
	struct vr_menu_setting const *setting)
{
	long index;

	for (index = 0; index < setting->value_count; index++)
	{
		char const *value = setting->values[index].value;

		switch (setting->type)
		{
		case _vr_setting_boolean:
		case _vr_setting_handedness:
			if (config_boolean(setting->key) == !strcmp(value, "true"))
				return index;
			break;
		case _vr_setting_multi:
			if (vr_menu_multi(value, FALSE, NULL))
				return index;
			break;
		case _vr_setting_real:
		case _vr_setting_real_choice:
			if (fabs(config_real(setting->key) - atof(value)) < 0.001)
				return index;
			break;
		case _vr_setting_string:
			if (!strcmp(config_string(setting->key), value))
				return index;
			break;
		case _vr_setting_turn_mode:
			if ((config_real("vr.snap_turn") > 0.0) == !strcmp(value, "snap"))
				return index;
			break;
		case _vr_setting_snap_angle:
			if (fabs(vr_menu_snap_angle() - atof(value)) < 0.001)
				return index;
			break;
		case _vr_setting_button:
			if (!strcmp(vr_button_source_value(vr_menu_button_source(vr_menu_button_action(setting->key))), value))
				return index;
			break;
		case _vr_setting_crouch_click:
			if (vr_menu_button_source(!strcmp(value, "crouch") ? VR_BUTTON_ACTION_CROUCH : VR_BUTTON_ACTION_RETICLE) ==
				VR_BUTTON_SOURCE_LEFT_STICK)
				return index;
			break;
		case _vr_setting_hud_shown:
			if (!vr_hud_hidden() == !strcmp(value, "true"))
				return index;
			break;
		}
	}
	return NONE;
}

/* ---------- the tags */

struct vr_menu_navigation { long tag, next, total; };

static struct
{
	/* the tags this made for the map loaded, and what they point at: the
	pause menu's item, the categories' screen and its buttons, and each
	page's screen and buttons */
	long button_tag_index, categories_tag_index;
	long title_tag_index, hints_tag_index;
	boolean loaded;
	long category_tag_indices[NUMBEROF(vr_menu_pages)];
	long page_tag_indices[NUMBEROF(vr_menu_pages)];
	long *setting_tag_indices[NUMBEROF(vr_menu_pages)];
 struct vr_menu_navigation *navigation;
 long navigation_count, navigation_capacity;
 void **allocations;
 long allocation_count, allocation_capacity;
} vr_menu;

/* Track lifetime without a fixed allocation/name ceiling. All allocations
 * are made while map tags load, never in the render loop. */
static void *vr_menu_allocate(long size)
{
    void *memory;
    if (size <= 0) return NULL;
    if (vr_menu.allocation_count == vr_menu.allocation_capacity) {
        long capacity = vr_menu.allocation_capacity ? vr_menu.allocation_capacity * 2 : 128;
        void **grown;
        if (capacity <= vr_menu.allocation_capacity || capacity > 32768) return NULL;
        grown = malloc(capacity * sizeof(*grown));
        if (!grown) return NULL;
        if (vr_menu.allocations) {
            memcpy(grown, vr_menu.allocations, vr_menu.allocation_count * sizeof(*grown));
            free(vr_menu.allocations);
        }
        vr_menu.allocations = grown; vr_menu.allocation_capacity = capacity;
    }
    memory = malloc((size_t)size);
    if (memory) {
        memset(memory, 0, (size_t)size);
        vr_menu.allocations[vr_menu.allocation_count++] = memory;
    }
    return memory;
}

/* Each tag retains its own name until map teardown. */
static char *vr_menu_name(char const *format, long a, long b)
{
    char *name = vr_menu_allocate(64);
    if (name) snprintf(name, 64, format, a, b);
    return name;
}

static struct vr_menu_widget *vr_menu_widget_get(
	long tag_index)
{
	return tag_index != NONE ? tag_get(VR_MENU_WIDGET_TAG, tag_index) : NULL;
}

static void vr_menu_reference(
	struct tag_reference *reference,
	long tag_index)
{
	reference->group_tag = VR_MENU_WIDGET_TAG;
	reference->name = tag_index != NONE ? tag_get_name(tag_index) : "";
	reference->name_length = (long)strlen(reference->name);
	reference->index = tag_index;
}

/* a widget cloned from `template_index`, named, added as a tag of its own */
static long vr_menu_clone(
	long template_index,
	char *tag_name,
	char const *widget_name,
	struct vr_menu_widget **out_widget)
{
	struct vr_menu_widget *source = vr_menu_widget_get(template_index);
	struct vr_menu_widget *widget = tag_name && source ? vr_menu_allocate(sizeof(*widget)) : NULL;
	long tag_index;

	*out_widget = NULL;
	if (!widget)
		return NONE;
	memcpy(widget, source, sizeof(*widget));
	memset(widget->name, 0, sizeof(widget->name));
	strncpy(widget->name, widget_name, sizeof(widget->name) - 1);
	tag_index = cache_file_add_tag(VR_MENU_WIDGET_TAG, (unsigned long)NONE, tag_name, widget);
	if (tag_index != NONE)
		*out_widget = widget;
	return tag_index;
}

static boolean vr_menu_handlers(
	struct vr_menu_widget *widget,
	struct vr_menu_event_handler const *handlers,
	long count)
{
	struct vr_menu_event_handler *copy;
    if (!count) {
        widget->event_handlers.count = 0;
        widget->event_handlers.address = NULL;
        return TRUE;
    }
    if (count < 0 || !handlers) return FALSE;
    copy = vr_menu_allocate(count * (long)sizeof(*copy));

	if (!copy)
		return FALSE;
	memcpy(copy, handlers, (size_t)count * sizeof(*copy));
	widget->event_handlers.count = count;
	widget->event_handlers.address = copy;
	return TRUE;
}

/* a button cloned from the pause menu's "RESUME GAME", its text from code */
static long vr_menu_button(
	long template_index,
	char *tag_name,
	char const *widget_name,
	struct vr_menu_event_handler const *handlers,
	long handler_count)
{
	struct vr_menu_widget *widget;
	struct vr_menu_game_data_input *input;
	long tag_index = vr_menu_clone(template_index, tag_name, widget_name, &widget);

	if (tag_index == NONE || !(input = vr_menu_allocate(sizeof(*input))) ||
		!vr_menu_handlers(widget, handlers, handler_count))
	{
		return NONE;
	}
	/* test25: as wide as a column (the text is laid out and clipped in the
	button's own bounds: the pause menu's are narrower, and longer rows lost
	their ends, "ALL FORWARD: < 0 C") */
	if (widget->bounds.x1 - widget->bounds.x0 < VR_MENU_BUTTON_WIDTH)
	{
		static boolean logged;

		if (!logged)
			platform_log("vr: VR settings rows widened from %d to %d", widget->bounds.x1 - widget->bounds.x0,
				VR_MENU_BUTTON_WIDTH);
		logged = TRUE;
		widget->bounds.x1 = (short)(widget->bounds.x0 + VR_MENU_BUTTON_WIDTH);
	}
	input->function = VR_MENU_GAME_DATA_FUNCTION;
	widget->game_data_inputs.count = 1;
	widget->game_data_inputs.address = input;
	widget->text_label_string_list.index = NONE;
	widget->text_label_string_list.name = "";
	widget->text_label_string_list.name_length = 0;
	return tag_index;
}

/* one handler, opening `widget_index` on A (NONE: the handler set later) */
static void vr_menu_open_handler(
	struct vr_menu_event_handler *handler,
	long widget_index)
{
	memset(handler, 0, sizeof(*handler));
	handler->flags = VR_MENU_OPEN_WIDGET;
	handler->event_type = VR_MENU_EVENT_A;
	vr_menu_reference(&handler->widget_tag, widget_index);
	vr_menu_reference(&handler->sound_effect, NONE);
}

/* Native DeLa definitions, independent of a map's optional solo pause tags.
 * All references are initialized to NONE, including fields a container does
 * not currently use. A zero tag index would name an unrelated map asset. */
static long vr_menu_native(char *name, char const *label, short type,
    short width, short height, struct vr_menu_widget **out)
{
    struct vr_menu_widget *widget = name ? vr_menu_allocate(sizeof(*widget)) : NULL;
    long index;
    *out = NULL;
    if (!widget) return NONE;
    widget->type = type;
    widget->controller_index = 4; /* inherit the invoking controller */
    strncpy(widget->name, label, sizeof(widget->name) - 1);
    widget->bounds.x1 = width;
    widget->bounds.y1 = height;
    widget->flags = type == 3 ? (1 | (1 << 5)) : type == 0 ? (1 | (1 << 1)) : 0;
    vr_menu_reference(&widget->background_bitmap, NONE);
    widget->background_bitmap.group_tag = 'bitm';
    vr_menu_reference(&widget->text_label_string_list, NONE);
    widget->text_label_string_list.group_tag = 'ustr';
    vr_menu_reference(&widget->text_font, NONE);
    widget->text_font.group_tag = 'font';
    vr_menu_reference(&widget->list_header_bitmap, NONE);
    widget->list_header_bitmap.group_tag = 'bitm';
    vr_menu_reference(&widget->list_footer_bitmap, NONE);
    widget->list_footer_bitmap.group_tag = 'bitm';
    vr_menu_reference(&widget->extended_description_widget, NONE);
    index = cache_file_add_tag(VR_MENU_WIDGET_TAG, (unsigned long)NONE, name, widget);
    if (index != NONE) *out = widget;
    return index;
}

static void vr_menu_child_set(struct vr_menu_child *child, long tag, short x, short y)
{
    memset(child, 0, sizeof(*child));
    vr_menu_reference(&child->widget_tag, tag);
    child->horizontal_offset = x;
    child->vertical_offset = y;
    child->custom_controller_index = 4;
    strcpy(child->name, "vr_settings");
}

/* Four settings per column, plus a separate NEXT row. Fixed 640x480 menu
 * coordinates are also the native pointer/Quest laser hit-test space. */
static long vr_menu_screen(
    long pause_index, long list_index, char *screen_name, char *list_name,
    char const *widget_name, long const *buttons, long button_count,
    long left_count, short column_x, short hints_x)
{
    struct vr_menu_widget *screen, *list;
    struct vr_menu_child *children;
    struct vr_menu_event_handler handlers[3];
    long list_tag_index, screen_tag_index, index;
    (void)hints_x;
    if (button_count <= 0 || button_count > VR_MENU_SETTINGS_PER_SCREEN + 1 ||
        left_count != 4 || column_x < VR_MENU_BUTTON_WIDTH || column_x + VR_MENU_BUTTON_WIDTH > 512)
        return NONE;
    list_tag_index = vr_menu_clone(list_index, list_name, "vr settings list", &list);
    if (list_tag_index == NONE || !(children = vr_menu_allocate(button_count * sizeof(*children))))
        return NONE;
    for (index = 0; index < button_count; index++) {
        boolean navigation = index == VR_MENU_SETTINGS_PER_SCREEN;
        long row = navigation ? 4 : (index < left_count ? index : index - left_count);
        if (buttons[index] == NONE) return NONE;
        vr_menu_child_set(&children[index], buttons[index],
            navigation || index < left_count ? 0 : column_x, (short)(row * 28));
    }
    list->bounds.x1 = (short)(column_x + VR_MENU_BUTTON_WIDTH);
    list->bounds.y1 = 5 * 28;
    list->child_widgets.count = button_count;
    list->child_widgets.address = children;
    screen_tag_index = vr_menu_clone(pause_index, screen_name, widget_name, &screen);
    if (screen_tag_index == NONE || !(children = vr_menu_allocate(3 * sizeof(*children))))
        return NONE;
    vr_menu_child_set(&children[0], vr_menu.title_tag_index, 64, 90);
    vr_menu_child_set(&children[1], list_tag_index, 64, 142);
    vr_menu_child_set(&children[2], vr_menu.hints_tag_index, 64, 340);
    screen->child_widgets.count = 3;
    screen->child_widgets.address = children;
    memset(handlers, 0, sizeof(handlers));
    handlers[0].flags = VR_MENU_CLOSE_CURRENT;
    handlers[0].event_type = VR_MENU_EVENT_B;
    handlers[1] = handlers[0];
    handlers[1].event_type = VR_MENU_EVENT_BACK;
    handlers[2].flags = VR_MENU_CLOSE_ALL;
    handlers[2].event_type = VR_MENU_EVENT_START;
    for (index = 0; index < 3; index++) {
        vr_menu_reference(&handlers[index].widget_tag, NONE);
        vr_menu_reference(&handlers[index].sound_effect, NONE);
    }
    return vr_menu_handlers(screen, handlers, 3) ? screen_tag_index : NONE;
}

/* Build back-to-front so each NEXT button references an existing screen.
 * The native widget stack makes B return to the preceding page, then the
 * categories. No looping links or unbounded history growth. */
static long vr_menu_paged(long pause, long list, long resume, long group,
    long const *buttons, long count, short column_x, short hints_x)
{
    long pages = VR_MENU_PAGES(count), next = NONE, sub;
    for (sub = pages - 1; sub >= 0; --sub) {
        long visible[VR_MENU_SETTINGS_PER_SCREEN + 1];
        long length = MIN(VR_MENU_SETTINGS_PER_SCREEN, count - sub * VR_MENU_SETTINGS_PER_SCREEN);
        struct vr_menu_event_handler handler;
        memcpy(visible, buttons + sub * VR_MENU_SETTINGS_PER_SCREEN, length * sizeof(long));
        if (next != NONE) {
            long slot = vr_menu.navigation_count;
            if (slot >= vr_menu.navigation_capacity) return NONE;
            vr_menu_open_handler(&handler, next);
            visible[length] = vr_menu_button(resume,
                vr_menu_name("ui\\shell\\solo_game\\pause_game\\vr_next_%ld_%ld", group, sub),
                "vr_next_page", &handler, 1);
            if (visible[length] == NONE) return NONE;
            vr_menu.navigation[slot].tag = visible[length++];
            vr_menu.navigation[slot].next = sub + 2;
            vr_menu.navigation[slot].total = pages;
            vr_menu.navigation_count++;
        }
        next = vr_menu_screen(pause, list,
            vr_menu_name("ui\\shell\\solo_game\\pause_game\\vr_page_%ld_%ld", group, sub),
            vr_menu_name("ui\\shell\\solo_game\\pause_game\\vr_list_%ld_%ld", group, sub),
            "vr_settings_page", visible, length, 4, column_x, hints_x);
        if (next == NONE) return NONE;
    }
    return next;
}

/* Add a row without relying on a stock list's exact child count. The
 * optional before index preserves native profile option/description indices. */
static boolean vr_menu_insert(long list_index, long tag, short x, short y, long before)
{
    struct vr_menu_widget *list = vr_menu_widget_get(list_index);
    struct vr_menu_child *old, *grown;
    long count, index;
    if (!list || list->type != 3 || tag == NONE) return FALSE;
    count = list->child_widgets.count;
    old = list->child_widgets.address;
    if (count < 0 || count > 128 || (count && !old)) return FALSE;
    for (index = 0; index < count; index++)
        if (old[index].widget_tag.index == tag) return TRUE;
    if (before < 0 || before > count) before = count;
    grown = vr_menu_allocate((count + 1) * sizeof(*grown));
    if (!grown) return FALSE;
    if (before) memcpy(grown, old, before * sizeof(*grown));
    vr_menu_child_set(&grown[before], tag, x, y);
    if (before < count) memcpy(grown + before + 1, old + before, (count - before) * sizeof(*grown));
    list->child_widgets.address = grown;
    list->child_widgets.count = count + 1;
    list->bounds.y1 = MAX(list->bounds.y1, y + 28);
    return TRUE;
}

/* Fit the extra main-menu entry above the profile footer (y=446). The
 * imported 33px artwork keeps its size; only its 3px gaps become 1px. */
static boolean vr_menu_main(long list_index)
{
    struct vr_menu_widget *list = vr_menu_widget_get(list_index);
    struct vr_menu_child *children;
    long index, y = 247, count;
    if (!list || list->type != 3 || list->child_widgets.count < 0 ||
        list->child_widgets.count > 16 || !list->child_widgets.address) return FALSE;
    count = list->child_widgets.count;
    children = list->child_widgets.address;
    for (index = 0; index < count; index++) {
        struct vr_menu_widget *item = vr_menu_widget_get(children[index].widget_tag.index);
        if (children[index].widget_tag.index == vr_menu.button_tag_index) return TRUE;
        if (!item || item->bounds.y1 < item->bounds.y0) return FALSE;
        y += item->bounds.y1 - item->bounds.y0 + 1;
    }
    if (y + 28 > 446 || !vr_menu_insert(list_index, vr_menu.button_tag_index, 192, (short)y, NONE))
        return FALSE;
    children = list->child_widgets.address;
    y = 247;
    for (index = 0; index < count; index++) {
        struct vr_menu_widget *item = vr_menu_widget_get(children[index].widget_tag.index);
        children[index].vertical_offset = (short)(y - item->bounds.y0);
        y += item->bounds.y1 - item->bounds.y0 + 1;
    }
    return TRUE;
}

/* A stock/CE pause list is identified by its real native quit callback,
 * not its number/order of children. Shared lists are only extended once. */
static boolean vr_menu_pause_list(long screen_index, long list_index)
{
    struct vr_menu_widget *screen = vr_menu_widget_get(screen_index);
    struct vr_menu_widget *list = vr_menu_widget_get(list_index);
    struct vr_menu_child *children;
    long index, bottom = 0;
    short list_x = 0, list_y = 0;
    if (!screen || !list || list->type != 3 || list->child_widgets.count <= 0 ||
        list->child_widgets.count > 128 || !list->child_widgets.address) return FALSE;
    children = list->child_widgets.address;
    for (index = 0; index < list->child_widgets.count; index++) {
        struct vr_menu_widget *button;
        if (children[index].widget_tag.index == vr_menu.button_tag_index) return TRUE;
        button = vr_menu_widget_get(children[index].widget_tag.index);
        if (button) bottom = MAX(bottom, children[index].vertical_offset + button->bounds.y1);
    }
    children = screen->child_widgets.address;
    for (index = 0; index < screen->child_widgets.count; index++)
        if (children[index].widget_tag.index == list_index) {
            list_x = children[index].horizontal_offset + list->bounds.x0;
            list_y = children[index].vertical_offset + list->bounds.y0;
        }
    /* Do not write beyond a custom CE screen. The existing Settings route
     * remains usable if that map's direct pause list has no spare room. */
    if (list_x + VR_MENU_BUTTON_WIDTH > 640 || list_y + bottom + 28 > 440) {
        platform_log("vr: pause layout has no room for a VR row: %s", tag_get_name(list_index));
        return FALSE;
    }
    if (!vr_menu_insert(list_index, vr_menu.button_tag_index, 0, (short)bottom, NONE)) return FALSE;
    /* The original solo hints shared this row. Move only an overlapping
     * non-list sibling (typically hints); no hard-coded child[4] access. */
    for (index = 0; index < screen->child_widgets.count; index++) {
        struct vr_menu_widget *other;
        short x, y;
        if (children[index].widget_tag.index == list_index) continue;
        other = vr_menu_widget_get(children[index].widget_tag.index);
        if (!other) continue;
        x = children[index].horizontal_offset + other->bounds.x0;
        y = children[index].vertical_offset + other->bounds.y0;
        if (other->bounds.y1 - other->bounds.y0 <= 40 &&
            x < list_x + VR_MENU_BUTTON_WIDTH && x + other->bounds.x1 - other->bounds.x0 > list_x &&
            y < list_y + bottom + 28 && y + other->bounds.y1 - other->bounds.y0 > list_y + bottom) {
            if (328 + other->bounds.x1 - other->bounds.x0 <= 640)
                children[index].horizontal_offset = (short)(328 - other->bounds.x0);
            else if (y + 28 + other->bounds.y1 - other->bounds.y0 <= 480)
                children[index].vertical_offset += 28;
        }
    }
    return TRUE;
}

static void vr_menu_multiplayer_fallback(void)
{
    long collection = tag_loaded('Soul', "ui\\shell\\multiplayer");
    struct tag_block *screens;
    long screen, child, button, handler;
    if (collection == NONE) return;
    screens = tag_get('Soul', collection);
    if (!screens || screens->count < 0 || screens->count > 128 || !screens->address) return;
    for (screen = 0; screen < screens->count; screen++) {
        long screen_index = ((struct tag_reference *)screens->address)[screen].index;
        struct vr_menu_widget *root = vr_menu_widget_get(screen_index);
        struct vr_menu_child *lists;
        if (!root || root->child_widgets.count > 128 || !root->child_widgets.address) continue;
        lists = root->child_widgets.address;
        for (child = 0; child < root->child_widgets.count; child++) {
            struct vr_menu_widget *list = vr_menu_widget_get(lists[child].widget_tag.index);
            struct vr_menu_child *buttons;
            boolean quit = FALSE;
            if (!list || list->type != 3 || list->child_widgets.count > 128 || !list->child_widgets.address) continue;
            buttons = list->child_widgets.address;
            for (button = 0; button < list->child_widgets.count && !quit; button++) {
                struct vr_menu_widget *item = vr_menu_widget_get(buttons[button].widget_tag.index);
                struct vr_menu_event_handler *events;
                if (!item || item->event_handlers.count > 128 || !item->event_handlers.address) continue;
                events = item->event_handlers.address;
                for (handler = 0; handler < item->event_handlers.count; handler++) {
                    char const *name;
                    if (!(events[handler].flags & VR_MENU_RUN_FUNCTION)) continue;
                    name = ui_widget_event_handler_function_name(events[handler].function);
                    if (name && !strcmp(name, "mp game player quit")) { quit = TRUE; break; }
                }
            }
            if (quit) vr_menu_pause_list(screen_index, lists[child].widget_tag.index);
        }
    }
}

/* Called after widgets/textures close, before native menu tags and the map
 * table are released. Idempotent; nothing is freed from a live UI callback. */
void vr_menu_tags_unloaded(void)
{
    long index;
    for (index = 0; index < vr_menu.allocation_count; index++) free(vr_menu.allocations[index]);
    /* cseries maps free to debug_free, which rejects NULL. First load and
     * repeated/failed-load teardown can legitimately have no registry. */
    if (vr_menu.allocations) free(vr_menu.allocations);
    memset(&vr_menu, 0, sizeof(vr_menu));
    vr_menu.button_tag_index = vr_menu.categories_tag_index = NONE;
    vr_menu.title_tag_index = vr_menu.hints_tag_index = NONE;
    for (index = 0; index < VR_MENU_PAGE_COUNT; index++)
        vr_menu.category_tag_indices[index] = vr_menu.page_tag_indices[index] = NONE;
}

void vr_menu_tags_loaded(void)
{
    static short const hints_x = 328, right_column_x = 328 - 72;
    long pause_index, list_index, resume_index, font, page, index;
    struct vr_menu_widget *widget;
    struct vr_menu_event_handler handlers[3];
    if (vr_menu.loaded) return; /* menu hooks may be requested twice */
    vr_menu_tags_unloaded();
    vr_menu.loaded = TRUE;
    font = tag_loaded('font', "ui\\small_ui");
    if (font == NONE) font = tag_loaded('font', "ui\\large_ui");
    if (font == NONE) font = tag_loaded('font', "ui\\interstate");
    if (font == NONE) {
        platform_log("vr: map has no UI font; preserving its menus, VR runtime remains active");
        return;
    }
    pause_index = vr_menu_native(vr_menu_name("vr\\menu\\screen_template", 0, 0),
        "vr_screen", 0, 640, 480, &widget);
    if (pause_index == NONE) return;
    /* A new root replaces and deletes its caller; every VR screen must own
     * its solo pause. ui_widget.c suppresses this flag for network sessions
     * and the main menu when the screen is instantiated, not at map load. */
    index = tag_loaded('bitm', "pc\\bitmaps/gradient");
    if (index != NONE) {
        vr_menu_reference(&widget->background_bitmap, index);
        widget->background_bitmap.group_tag = 'bitm';
    }
    list_index = vr_menu_native(vr_menu_name("vr\\menu\\list_template", 0, 0),
        "vr_list", 3, 506, 140, &widget);
    if (list_index == NONE) return;
    resume_index = vr_menu_native(vr_menu_name("vr\\menu\\text_template", 0, 0),
        "vr_text", 1, VR_MENU_BUTTON_WIDTH, 28, &widget);
    if (resume_index == NONE) return;
    vr_menu_reference(&widget->text_font, font);
    widget->text_font.group_tag = 'font';
    widget->text_color.alpha = 1.0f;
    widget->text_color.red = 0.25f;
    widget->text_color.green = 0.75f;
    widget->text_color.blue = 1.0f;
    widget->horizontal_offset = 2;
    widget->vertical_offset = 4;
    vr_menu.title_tag_index = vr_menu_button(resume_index,
        vr_menu_name("vr\\menu\\title", 0, 0), "vr_title", NULL, 0);
    vr_menu.hints_tag_index = vr_menu_button(resume_index,
        vr_menu_name("vr\\menu\\hints", 0, 0), "vr_hints", NULL, 0);
    if (vr_menu.title_tag_index == NONE || vr_menu.hints_tag_index == NONE) return;
    vr_menu_widget_get(vr_menu.hints_tag_index)->bounds.x1 = 512;
    vr_menu.navigation_capacity = VR_MENU_PAGES(VR_MENU_PAGE_COUNT);
    for (page = 0; page < VR_MENU_PAGE_COUNT; page++)
        vr_menu.navigation_capacity += VR_MENU_PAGES(vr_menu_pages[page].count);
    vr_menu.navigation = vr_menu_allocate(vr_menu.navigation_capacity * sizeof(*vr_menu.navigation));
    if (!vr_menu.navigation) return;
    for (page = 0; page < VR_MENU_PAGE_COUNT; page++) {
        vr_menu.setting_tag_indices[page] = vr_menu_allocate(vr_menu_pages[page].count * sizeof(long));
        if (!vr_menu.setting_tag_indices[page]) return;
        for (index = 0; index < vr_menu_pages[page].count; index++) vr_menu.setting_tag_indices[page][index] = NONE;
    }

	/* each setting: A or right steps on, left back */
	memset(handlers, 0, sizeof(handlers));
	handlers[0].flags = VR_MENU_RUN_FUNCTION;
	handlers[0].event_type = VR_MENU_EVENT_A;
	handlers[0].function = VR_MENU_NEXT_FUNCTION;
	handlers[1] = handlers[0];
	handlers[1].event_type = VR_MENU_EVENT_DPAD_RIGHT;
	handlers[2] = handlers[0];
	handlers[2].event_type = VR_MENU_EVENT_DPAD_LEFT;
	handlers[2].function = VR_MENU_PREVIOUS_FUNCTION;
	for (index = 0; index < 3; index++)
	{
		vr_menu_reference(&handlers[index].widget_tag, NONE);
		vr_menu_reference(&handlers[index].sound_effect, NONE);
	}

	/* each page: its settings' buttons and its screen */
	for (page = 0; page < VR_MENU_PAGE_COUNT; page++)
	{
		struct vr_menu_page const *definition = &vr_menu_pages[page];

		for (index = 0; index < definition->count; index++)
		{
			char widget_name[32];

			snprintf(widget_name, sizeof(widget_name), "vr_setting_%ld_%ld", page, index);
			vr_menu.setting_tag_indices[page][index] = vr_menu_button(resume_index,
				vr_menu_name("ui\\shell\\solo_game\\pause_game\\vr_setting_%ld_%ld", page, index),
				widget_name, handlers, 3);
			if (vr_menu.setting_tag_indices[page][index] == NONE)
				return;
		}
		vr_menu.page_tag_indices[page] = vr_menu_paged(pause_index, list_index, resume_index,
            page, vr_menu.setting_tag_indices[page], definition->count, right_column_x, hints_x);
		if (vr_menu.page_tag_indices[page] == NONE)
			return;
	}

	/* the categories, each opening its page, in one column */
	for (page = 0; page < VR_MENU_PAGE_COUNT; page++)
	{
		vr_menu_open_handler(&handlers[0], vr_menu.page_tag_indices[page]);
		vr_menu.category_tag_indices[page] = vr_menu_button(resume_index,
			vr_menu_name("ui\\shell\\solo_game\\pause_game\\vr_category_%ld", page, 0),
			"vr_category_button", handlers, 1);
		if (vr_menu.category_tag_indices[page] == NONE)
			return;
	}
	vr_menu.categories_tag_index = vr_menu_paged(pause_index, list_index, resume_index,
        VR_MENU_PAGE_COUNT, vr_menu.category_tag_indices, VR_MENU_PAGE_COUNT, right_column_x, hints_x);
	if (vr_menu.categories_tag_index == NONE)
		return;

	/* the pause menu's new item, opening the categories */
	vr_menu_open_handler(&handlers[0], vr_menu.categories_tag_index);
	vr_menu.button_tag_index = vr_menu_button(resume_index,
		vr_menu_name("ui\\shell\\solo_game\\pause_game\\vr_settings_button", 0, 0),
		"vr_settings_button", handlers, 1);
    if (vr_menu.button_tag_index == NONE) return;
    {
        long main = tag_loaded(VR_MENU_WIDGET_TAG, "pc\\main_menu/main_menu_select_list");
        long settings = tag_loaded(VR_MENU_WIDGET_TAG,
            "pc\\main_menu/settings_select/player_setup/player_profile_edit/profile_edit_select_list");
        boolean settings_added = FALSE, main_added = FALSE;
        if (main != NONE) main_added = vr_menu_main(main);
        if (settings != NONE) {
            struct vr_menu_child *child;
            long row = vr_menu_native(vr_menu_name("vr\\menu\\settings_row", 0, 0),
                "vr_settings_row", 3, VR_MENU_BUTTON_WIDTH, 28, &widget);
            if (row != NONE && (child = vr_menu_allocate(sizeof(*child)))) {
                vr_menu_child_set(child, vr_menu.button_tag_index, 0, 0);
                /* The outer Settings list owns up/down. A one-item inner
                 * list must not consume those events and trap gamepad focus. */
                widget->flags = 1;
                widget->child_widgets.count = 1;
                widget->child_widgets.address = child;
                /* The native profile description callback recognizes nested
                 * button bars; no nonexistent tenth picture/string is read. */
                settings_added = vr_menu_insert(settings, row, 51, 377,
                    vr_menu_widget_get(settings)->child_widgets.count - 1);
            }
        }
        pause_index = tag_loaded(VR_MENU_WIDGET_TAG, "ui\\shell\\solo_game\\pause_game\\pause_game");
        list_index = tag_loaded(VR_MENU_WIDGET_TAG, "ui\\shell\\solo_game\\pause_game\\pause_list");
        if (pause_index != NONE && list_index != NONE) vr_menu_pause_list(pause_index, list_index);
        /* OpenCE already puts SETTINGS on every multiplayer pause screen.
         * The nested VR row makes that route work without growing its art.
         * Stock-menu fallback gets a direct VR entry when that route is absent. */
        if (!settings_added) vr_menu_multiplayer_fallback();
        platform_log("vr: native VR settings ready: %ld categories, four rows per column; main=%d settings=%d",
            VR_MENU_PAGE_COUNT, main_added, settings_added);
    }
}

/* ---------- the widgets' callbacks */

enum
{
	_vr_menu_none,
	_vr_menu_pause_item,
	_vr_menu_category,
    _vr_menu_navigation,
    _vr_menu_title,
    _vr_menu_hints,
	_vr_menu_setting,
};

/* what the widget's definition is: the pause menu's item, a category
(its page in *page) or a setting (its page and index) */
static int vr_menu_widget_kind(
	long definition_tag_index,
	long *page,
	long *setting)
{
	long index;

	if (definition_tag_index == NONE || !vr_menu.loaded)
		return _vr_menu_none;
    if (definition_tag_index == vr_menu.title_tag_index) return _vr_menu_title;
    if (definition_tag_index == vr_menu.hints_tag_index) return _vr_menu_hints;
	if (definition_tag_index == vr_menu.button_tag_index)
        return _vr_menu_pause_item;
    for (index = 0; index < vr_menu.navigation_count; index++)
        if (vr_menu.navigation[index].tag == definition_tag_index) {
            *setting = index;
            return _vr_menu_navigation;
        }
	for (*page = 0; *page < VR_MENU_PAGE_COUNT; (*page)++)
	{
		if (vr_menu.category_tag_indices[*page] == definition_tag_index)
			return _vr_menu_category;
		for (index = 0; vr_menu.setting_tag_indices[*page] && index < vr_menu_pages[*page].count; index++)
		{
			if (vr_menu.setting_tag_indices[*page][index] == definition_tag_index)
			{
				*setting = index;
				return _vr_menu_setting;
			}
		}
	}
	return _vr_menu_none;
}

/* Only generated root screens own the solo pause, including NEXT targets.
 * Do not infer ownership from a tag name supplied by a map. */
boolean vr_menu_is_screen(long definition_tag_index)
{
    long index;
    if (definition_tag_index == NONE || !vr_menu.loaded) return FALSE;
    if (definition_tag_index == vr_menu.categories_tag_index) return TRUE;
    for (index = 0; index < VR_MENU_PAGE_COUNT; ++index)
        if (definition_tag_index == vr_menu.page_tag_indices[index]) return TRUE;
    for (index = 0; index < vr_menu.navigation_count; ++index) {
        struct vr_menu_widget *button = vr_menu_widget_get(vr_menu.navigation[index].tag);
        struct vr_menu_event_handler *handler = button ? button->event_handlers.address : NULL;
        if (handler && button->event_handlers.count == 1 &&
            definition_tag_index == handler->widget_tag.index) return TRUE;
    }
    return FALSE;
}

boolean vr_menu_is_setting(long definition_tag_index)
{
	long page = NONE, setting = NONE;
	return vr_menu_widget_kind(definition_tag_index, &page, &setting) == _vr_menu_setting;
}

boolean vr_menu_setting_text(
	long definition_tag_index,
	wchar_t *text,
	long size)
{
	long page = NONE, setting_index = NONE;
	int kind = vr_menu_widget_kind(definition_tag_index, &page, &setting_index);
	char line[64];
	long index;

	if (kind == _vr_menu_none || size <= 0)
		return FALSE;
	if (kind == _vr_menu_pause_item || kind == _vr_menu_title)
	{
		snprintf(line, sizeof(line), "VR SETTINGS");
	}
	else if (kind == _vr_menu_hints)
    {
        snprintf(line, sizeof(line), "A/RIGHT: NEXT   LEFT: PREVIOUS   B: BACK");
    }
	else if (kind == _vr_menu_navigation)
    {
        snprintf(line, sizeof(line), "NEXT PAGE (%ld/%ld)",
            vr_menu.navigation[setting_index].next, vr_menu.navigation[setting_index].total);
    }
    else if (kind == _vr_menu_category)
	{
		snprintf(line, sizeof(line), "%s", vr_menu_pages[page].label);
	}
	else
	{
		struct vr_menu_setting const *setting = &vr_menu_pages[page].settings[setting_index];
		long value_index = vr_menu_value_index(setting);

        if(setting->type == _vr_setting_gun_aim) {
            int kind=vr_gun_class(); char key[64];
            if(!strcmp(setting->key,"gun")) snprintf(line,sizeof(line),"%s: %s",setting->label,vr_gun_class_label(kind));
            else if(!strcmp(setting->key,"reset")) snprintf(line,sizeof(line),"%s: APPLY",setting->label);
            else if(kind<0) snprintf(line,sizeof(line),"%s: HOLD A GUN",setting->label);
            else { double value; snprintf(key,sizeof(key),"vr.aim_%s_%s",vr_gun_class_key(kind),setting->key);
                value=config_real(key); if(!isfinite(value)) value=0;
                snprintf(line,sizeof(line),"%s: < %.1f DEG >",setting->label,value+0.0); }
        }
        else if(setting->type == _vr_setting_hand_degrees)
            snprintf(line,sizeof(line),"%s: < %.0f DEG >",setting->label,vr_menu_hand_angle(setting->key));
		else if(setting->type == _vr_setting_degrees || setting->type == _vr_setting_centimetres ||
			setting->type == _vr_setting_scope_centimetres || setting->type == _vr_setting_vehicle_centimetres) {
            double value=config_real(setting->key); if(!isfinite(value)) value=0;
            snprintf(line,sizeof(line),"%s: < %.0f %s >",setting->label,
                setting->type!=_vr_setting_degrees?value*100:value,
                setting->type!=_vr_setting_degrees?"CM":"DEG");
        } else if(setting->type == _vr_setting_reset_alignment || setting->type == _vr_setting_flip_alignment ||
            setting->type == _vr_setting_reset_hand || setting->type == _vr_setting_reset_weapon ||
            setting->type == _vr_setting_reset_scopes || setting->type == _vr_setting_reset_buttons ||
            setting->type == _vr_setting_reset_vehicle_offsets || setting->type == _vr_setting_reset_wrist)
            snprintf(line,sizeof(line),"%s: APPLY",setting->label);
        else
		snprintf(line, sizeof(line), setting->type == _vr_setting_real || setting->type == _vr_setting_snap_angle ?
			"%s: < %s >" : "%s: %s", setting->label,
			value_index != NONE ? setting->values[value_index].label : "CUSTOM");
	}
	for (index = 0; line[index] && index < size - 1; index++)
		text[index] = (wchar_t)(unsigned char)line[index];
	text[index] = 0;
	return TRUE;
}

boolean vr_menu_setting_change(
	long definition_tag_index,
	long step)
{
	long page = NONE, setting_index = NONE;
	struct vr_menu_setting const *setting;
	long value_index;
	char const *value;
	boolean written = FALSE;

	if (vr_menu_widget_kind(definition_tag_index, &page, &setting_index) != _vr_menu_setting)
		return FALSE;
	setting = &vr_menu_pages[page].settings[setting_index];
    if(setting->type == _vr_setting_degrees || setting->type == _vr_setting_centimetres ||
		setting->type == _vr_setting_scope_centimetres || setting->type == _vr_setting_vehicle_centimetres) {
        double value=config_real(setting->key), unit=setting->type==_vr_setting_degrees?5.0:0.01;
        double limit=setting->type==_vr_setting_degrees?180.0:
			setting->type==_vr_setting_vehicle_centimetres?0.50:
			setting->type==_vr_setting_scope_centimetres?VR_SCOPE_ADJUST_LIMIT_METRES:0.20;
        if(!isfinite(value)) value=0.0;
        value=step>0?(floor(value/unit+0.00001)+1)*unit:(ceil(value/unit-0.00001)-1)*unit;
        value=fmax(-limit,fmin(limit,value)); written=config_write_real(setting->key,value);
        vr_reload_settings(); platform_log("vr: %s %.3f%s",setting->key,value,written?"":" (save failed)");
        return TRUE;
    }
    if(setting->type == _vr_setting_reset_scopes) {
        static const char *const kinds[]={"pistol","sniper"};
        static const char *const parts[]={"forward","up","right","scale"};
        char key[64]; int kind, part;
        written=TRUE;
        for(kind=0;kind<2;kind++) for(part=0;part<4;part++) {
            snprintf(key,sizeof(key),"vr.scope_%s_%s",kinds[kind],parts[part]);
            written=config_write_real(key,config_default_real(key))&&written; }
        vr_reload_settings(); platform_log("vr: reset scope places and sizes%s",written?"":" (save failed)");
        return TRUE;
    }
    if(setting->type == _vr_setting_hud_shown) {
        /* (the session's, as the head tap's: nothing written) */
        vr_set_hud_hidden(!vr_hud_hidden());
        return TRUE;
    }
    if(setting->type == _vr_setting_reset_wrist) {
        static const char *const keys[]={"vr.wrist_hud_along","vr.wrist_hud_across","vr.wrist_hud_out","vr.wrist_hud_size"};
        int key;
        written=TRUE;
        for(key=0;key<(int)NUMBEROF(keys);key++) written=config_write_real(keys[key],config_default_real(keys[key]))&&written;
        vr_reload_settings(); platform_log("vr: reset the wrist HUD's place and size%s",written?"":" (save failed)");
        return TRUE;
    }
    if(setting->type == _vr_setting_reset_vehicle_offsets) {
        static const char *const vehicles[]={"all","warthog","ghost","banshee","scorpion","pelican"};
        static const char *const axes[]={"up","forward","right"};
        char key[64]; int vehicle, axis;
        written=TRUE;
        for(vehicle=0;vehicle<(int)NUMBEROF(vehicles);vehicle++) for(axis=0;axis<3;axis++) {
            snprintf(key,sizeof(key),"vr.vehicle_%s_%s",vehicles[vehicle],axes[axis]);
            written=config_write_real(key,config_default_real(key))&&written; }
        vr_reload_settings(); platform_log("vr: reset vehicle seat offsets%s",written?"":" (save failed)");
        return TRUE;
    }
    if(setting->type == _vr_setting_turn_mode) {
        /* smooth keeps the snap angle for later; snap takes it up again */
        boolean snapping=config_real("vr.snap_turn")>0.0;
        double angle=vr_menu_snap_angle();
        written=config_write_real("vr.snap_turn_amount",angle);
        written=config_write_real("vr.snap_turn",snapping?0.0:angle)&&written;
        vr_reload_settings(); platform_log("vr: turning %s (snap angle %.1f)%s",snapping?"smooth":"snap",angle,written?"":" (save failed)");
        return TRUE;
    }
    if(setting->type == _vr_setting_snap_angle) {
        /* a step from the angle now, as the numeric rows step */
        double current=vr_menu_snap_angle(), angle;
        long i;
        value_index=step>0?setting->value_count-1:0;
        if(step>0) { for(i=0;i<setting->value_count;i++) if(atof(setting->values[i].value)>current+0.00001) { value_index=i; break; } }
        else { for(i=setting->value_count-1;i>=0;i--) if(atof(setting->values[i].value)<current-0.00001) { value_index=i; break; } }
        angle=atof(setting->values[value_index].value);
        written=config_write_real("vr.snap_turn_amount",angle);
        if(config_real("vr.snap_turn")>0.0) written=config_write_real("vr.snap_turn",angle)&&written;
        vr_reload_settings(); platform_log("vr: snap angle %.1f%s",angle,written?"":" (save failed)");
        return TRUE;
    }
    if(setting->type == _vr_setting_reset_buttons) {
        int action;
        written=TRUE;
        for(action=0;action<VR_BUTTON_ACTIONS;action++)
            written=config_write_string(vr_button_action_key(action),vr_button_source_value(vr_button_default(action)))&&written;
        vr_reload_settings(); platform_log("vr: reset buttons to their defaults%s",written?"":" (save failed)");
        return TRUE;
    }
    if(setting->type == _vr_setting_button) {
        int action=vr_menu_button_action(setting->key);
        if(action==NONE) return TRUE;
        value_index=vr_menu_value_index(setting);
        value_index=value_index==NONE?0:(value_index+step+setting->value_count)%setting->value_count;
        written=vr_menu_button_set(action,vr_button_source_of(setting->values[value_index].value,VR_BUTTON_SOURCE_NONE));
        vr_reload_settings(); platform_log("vr: %s set to %s%s",setting->key,setting->values[value_index].value,written?"":" (save failed)");
        return TRUE;
    }
    if(setting->type == _vr_setting_gun_aim) {
        /* the held gun's kind only; nothing without a gun */
        int kind=vr_gun_class(); char key[64];
        if(kind<0||!strcmp(setting->key,"gun")) return TRUE;
        if(!strcmp(setting->key,"reset")) {
            snprintf(key,sizeof(key),"vr.aim_%s_up",vr_gun_class_key(kind)); written=config_write_real(key,0.0);
            snprintf(key,sizeof(key),"vr.aim_%s_right",vr_gun_class_key(kind)); written=config_write_real(key,0.0)&&written;
            vr_reload_settings(); platform_log("vr: reset %s aim%s",vr_gun_class_label(kind),written?"":" (save failed)");
            return TRUE;
        }
        {
            double value;
            snprintf(key,sizeof(key),"vr.aim_%s_%s",vr_gun_class_key(kind),setting->key);
            value=config_real(key); if(!isfinite(value)) value=0.0;
            value=step>0?(floor(value/0.5+0.00001)+1)*0.5:(ceil(value/0.5-0.00001)-1)*0.5;
            value=fmax(-VR_GUN_AIM_LIMIT,fmin(VR_GUN_AIM_LIMIT,value));
            written=config_write_real(key,value+0.0); vr_reload_settings();
            platform_log("vr: %s %.1f%s",key,value,written?"":" (save failed)");
        }
        return TRUE;
    }
    if(setting->type == _vr_setting_hand_degrees) {
        /* both hands at once, the left mirrored */
        double value=vr_menu_hand_angle(setting->key), sign=strcmp(setting->key,"pitch")?-1.0:1.0;
        char key[64];
        value=step>0?(floor(value/5.0+0.00001)+1)*5.0:(ceil(value/5.0-0.00001)-1)*5.0;
        value=fmax(-180.0,fmin(180.0,value));
        snprintf(key,sizeof(key),"vr.hand_right_%s",setting->key); written=config_write_real(key,value);
        snprintf(key,sizeof(key),"vr.hand_left_%s",setting->key); written=config_write_real(key,value*sign+0.0)&&written;
        vr_reload_settings(); platform_log("vr: both hands %s %.1f (left %.1f)%s",setting->key,value,value*sign,written?"":" (save failed)");
        return TRUE;
    }
    if(setting->type == _vr_setting_flip_alignment) {
        double roll=config_real(setting->key);
        if(!isfinite(roll)) roll=0.0;
        roll=roll>0.0?roll-180.0:roll+180.0;
        roll=fmax(-180.0,fmin(180.0,roll));
        written=config_write_real(setting->key,roll); vr_reload_settings();
        platform_log("vr: %s flipped to %.1f%s",setting->key,roll,written?"":" (save failed)"); return TRUE;
    }
    if(setting->type == _vr_setting_reset_hand || setting->type == _vr_setting_reset_weapon) {
        static const char *const hand_axes[]={"pitch","yaw","roll"};
        static const char *const hand_sides[]={"left","right"};
        static const char *const weapon_keys[]={"vr.weapon_pitch","vr.weapon_yaw","vr.weapon_roll",
            "vr.gun_forward","vr.gun_up","vr.gun_out",
            "vr.weapon_offset_right","vr.weapon_offset_up","vr.weapon_offset_back"};
        char key[64]; int a, h;
        written=TRUE;
        if(setting->type == _vr_setting_reset_hand) {
            /* key "left", "right" or "both" */
            for(h=0;h<2;h++) if(!strcmp(setting->key,"both")||!strcmp(setting->key,hand_sides[h]))
                for(a=0;a<3;a++) { snprintf(key,sizeof(key),"vr.hand_%s_%s",hand_sides[h],hand_axes[a]);
                    written=config_write_real(key,config_default_real(key))&&written; }
        } else for(a=0;a<(int)NUMBEROF(weapon_keys);a++) written=config_write_real(weapon_keys[a],config_default_real(weapon_keys[a]))&&written;
        vr_reload_settings();
        platform_log("vr: reset %s %s%s",setting->key,setting->type == _vr_setting_reset_hand ? "hand orientation" : "gun angle and place",
            written?"":" (save failed)"); return TRUE;
    }
    if(setting->type == _vr_setting_reset_alignment) {
        const char *axes[]={"pitch","yaw","roll","right","up","back"}; char key[64]; int a;
        written=TRUE;
        for(a=0;a<6;a++) { snprintf(key,sizeof(key),"vr.align_%s_%s",setting->key,axes[a]);
            written=config_write_real(key,0.0)&&written; }
        snprintf(key,sizeof(key),"vr.align_%s_grip_aim",setting->key);
        written=config_write_boolean(key,FALSE)&&written; vr_reload_settings();
        platform_log("vr: reset %s controller calibration%s",setting->key,written?"":" (save failed)"); return TRUE;
    }
	value_index = vr_menu_value_index(setting);
	if (setting->type == _vr_setting_real)
	{
		/* Move in the requested direction even from a custom numeric value;
         * nearest-then-step skipped valid thresholds (e.g. 0.34 -> 0.40). */
        double current = config_real(setting->key);
        long i;
        if (step > 0) {
            value_index = setting->value_count - 1;
            for (i = 0; i < setting->value_count; i++)
                if (atof(setting->values[i].value) > current + 0.00001) { value_index = i; break; }
        } else {
            value_index = 0;
            for (i = setting->value_count - 1; i >= 0; i--)
                if (atof(setting->values[i].value) < current - 0.00001) { value_index = i; break; }
        }
	}
	else
		value_index = value_index == NONE ? 0 : (value_index + step + setting->value_count) % setting->value_count;
	value = setting->values[value_index].value;
	switch (setting->type)
	{
	case _vr_setting_boolean:
		written = config_write_boolean(setting->key, !strcmp(value, "true"));
		break;
	case _vr_setting_multi:
		written = TRUE;
		if (!vr_menu_multi(value, TRUE, &written))
			written = FALSE;
		break;
	case _vr_setting_handedness:
	{
		/* the gun's hand, and what was chosen for the old hand follows it:
		vehicle steering and Move With (the sticks and face buttons mirror
		through vr.mirror_controls) */
		boolean left = !strcmp(value, "true");
		char const *from = left ? "right" : "left", *to = left ? "left" : "right";

		written = config_write_boolean(setting->key, left);
		if (!strcmp(config_string("vr.vehicle_steering"), from))
			written = config_write_string("vr.vehicle_steering", to) && written;
		if (!strcmp(config_string("vr.move_relative"), from))
			written = config_write_string("vr.move_relative", to) && written;
		break;
	}
	case _vr_setting_real:
	case _vr_setting_real_choice:
		written = config_write_real(setting->key, atof(value));
		break;
	case _vr_setting_string:
		written = config_write_string(setting->key, value);
		/* Older configs may have disabled the old 3D screen toggle. Selecting
		the new 3D SCREEN mode explicitly enables that screen. */
		if (!strcmp(setting->key, "vr.cutscenes") && !strcmp(value, "screen"))
			written = config_write_boolean("vr.cinema_3d", TRUE) && written;
		break;
	case _vr_setting_crouch_click:
		/* crouch: on the left stick's click with the reticle's toggle off it
		(no button; BUTTONS can give it one) and the turning stick held down
		free, the layout before 1.0.8; the reticle: crouch back on the
		turning stick held down. Each through the BUTTONS page's swap, so
		no button does two things */
		if (!strcmp(value, "crouch"))
		{
			written = vr_menu_button_set(VR_BUTTON_ACTION_RETICLE, VR_BUTTON_SOURCE_NONE);
			written = vr_menu_button_set(VR_BUTTON_ACTION_CROUCH, VR_BUTTON_SOURCE_LEFT_STICK) && written;
		}
		else
		{
			written = vr_menu_button_set(VR_BUTTON_ACTION_CROUCH, VR_BUTTON_SOURCE_RIGHT_STICK_DOWN);
			written = vr_menu_button_set(VR_BUTTON_ACTION_RETICLE, VR_BUTTON_SOURCE_LEFT_STICK) && written;
		}
		break;
	}
	vr_reload_settings();
	platform_log("vr: %s set to %s%s", setting->key, value, written ? "" : " (not saved to config.toml)");
	return TRUE;
}

#endif
