/* Native event-backed implementation of the game's public input API.
   Carbon's physical keyboard and mouse are exposed as virtual control port 0;
   physical gamepads, rumble and text composition are not advertised. */
#define fast_ftol mac_engine_input_unused_fast_ftol
#include "cseries.h"
#undef fast_ftol
#ifndef MAC_INPUT_NO_CARBON
#define MAC_INPUT_NO_CARBON
#endif
#include "input/input.h"
#include "game/players.h"
#include "units/units.h"
#include "interface/ui_widget.h"
#include "../linux/include/halo_keyboard.h"
#include "../linux/src/port_config.h"
#include "mac_input.h"
#include "mac_window.h"
#include "engine_input.h"
#include "halo_ui_pointer.h"
#include "mac_gamepad.h"
#include "../linux/include/input_prompts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAC_INPUT_MOUSE_BUTTONS = 4, MAC_INPUT_KEY_QUEUE_CAPACITY = 32 };

/* This is the consumer's private struct in player_control.c. */
struct mouse_state
{
	long x;
	long y;
	long wheel;
	byte buttons[MAC_INPUT_MOUSE_BUTTONS];
};

struct mac_key_mapping
{
	byte mac_keycode;
	short game_keycode;
};

static const struct mac_key_mapping mac_key_mappings[] =
{
	{0, _key_a}, {1, _key_s}, {2, _key_d}, {3, _key_f}, {4, _key_h}, {5, _key_g},
	{6, _key_z}, {7, _key_x}, {8, _key_c}, {9, _key_v}, {11, _key_b}, {12, _key_q},
	{13, _key_w}, {14, _key_e}, {15, _key_r}, {16, _key_y}, {17, _key_t},
	{18, _key_1}, {19, _key_2}, {20, _key_3}, {21, _key_4}, {22, _key_6},
	{23, _key_5}, {24, _key_equal}, {25, _key_9}, {26, _key_7}, {27, _key_dash},
	{28, _key_8}, {29, _key_0}, {30, _key_right_bracket}, {31, _key_o}, {32, _key_u},
	{33, _key_left_bracket}, {34, _key_i}, {35, _key_p}, {36, _key_return},
	{37, _key_l}, {38, _key_j}, {39, _key_apostrophe}, {40, _key_k}, {41, _key_semicolon},
	{42, _key_backslash}, {43, _key_comma}, {44, _key_forwardslash}, {45, _key_n},
	{46, _key_m}, {47, _key_period}, {48, _key_tab}, {49, _key_space}, {50, _key_backquote},
	{51, _key_backspace}, {53, _key_escape}, {54, _key_right_windows},
	{55, _key_left_windows}, {56, _key_left_shift}, {58, _key_left_alt},
	{59, _key_left_control}, {60, _key_right_shift}, {61, _key_right_alt},
	{62, _key_right_control}, {65, _keypad_decimal}, {67, _keypad_multiply},
	{69, _keypad_add}, {71, _keypad_num_lock}, {75, _keypad_divide},
	{76, _keypad_enter}, {78, _keypad_subtract}, {81, _key_equal},
	{82, _keypad_0}, {83, _keypad_1}, {84, _keypad_2}, {85, _keypad_3},
	{86, _keypad_4}, {87, _keypad_5}, {88, _keypad_6}, {89, _keypad_7},
	{91, _keypad_8}, {92, _keypad_9}, {96, _key_f5}, {97, _key_f6}, {98, _key_f7},
	{99, _key_f3}, {100, _key_f8}, {101, _key_f9}, {103, _key_f11},
	{109, _key_f10}, {111, _key_f12}, {114, _key_insert}, {115, _key_home},
	{116, _key_page_up}, {117, _key_delete}, {118, _key_f4}, {119, _key_end},
	{120, _key_f2}, {121, _key_page_down}, {122, _key_f1}, {123, _key_left_arrow},
	{124, _key_right_arrow}, {125, _key_down_arrow}, {126, _key_up_arrow}
};

static struct mac_input_frame mac_pending_frame;
static struct mouse_state mac_mouse_state;
/* the game pads of the controllers 1 to 4 (port 0 has the keyboard and mouse) */
static struct gamepad_state mac_virtual_gamepad[MAXIMUM_GAMEPADS];
static boolean mac_port_connected[MAXIMUM_GAMEPADS];
static struct gamepad_state mac_suppressed_gamepad;
static byte mac_key_down[NUMBER_OF_KEYS];
static byte mac_key_previous[NUMBER_OF_KEYS];
static struct key_stroke mac_key_queue[MAC_INPUT_KEY_QUEUE_CAPACITY];
static unsigned mac_key_queue_count;
static unsigned mac_key_queue_read;
static boolean mac_input_suppressed;
static boolean mac_input_initialized;
static float mac_mouse_look_yaw;
static float mac_mouse_look_pitch;
static boolean mac_wheel_switch_pending;
static boolean mac_native_text_typing;
static boolean mac_mouse_look_in_frame;
static unsigned int mac_mouse_buttons_previous;
static int mac_wheel_direction;
static int mac_binding_capture_waiting, mac_binding_capture_ready, mac_binding_capture_result, mac_binding_captured_input;
static unsigned long mac_bindings_read_at = (unsigned long)-1;
static int mac_bindings[NUMBER_OF_HALO_KEYBOARD_ACTIONS][2];
/* the device used last (input.prompts "auto"): the prompts name its controls */
static struct input_prompt_tracker mac_prompt_tracker = { 1, 0.0f };
static boolean mac_script_pad_pushed;
static short mac_pad_previous_values[MAXIMUM_GAMEPADS][4];
static unsigned mac_pad_previous_buttons[MAXIMUM_GAMEPADS];
static unsigned char mac_pad_previous_analog[MAXIMUM_GAMEPADS][MAC_GAMEPAD_ANALOG_COUNT];
/* diagnostic: a pad value for one update (mac_engine_input_script_pad) */
static short mac_script_pad_left_x;

#define MAC_BIND_MOUSE_LEFT 0x100
#define MAC_BIND_MOUSE_RIGHT 0x101
#define MAC_BIND_MOUSE_MIDDLE 0x102
#define MAC_BIND_MOUSE_4 0x103
#define MAC_BIND_MOUSE_5 0x104
#define MAC_BIND_WHEEL 0x110
#define MAC_BIND_WHEEL_UP 0x111
#define MAC_BIND_WHEEL_DOWN 0x112

static const struct { short key; const char *name; } mac_key_names[] = {
	{_key_escape,"Escape"},{_key_f1,"F1"},{_key_f2,"F2"},{_key_f3,"F3"},{_key_f4,"F4"},
	{_key_f5,"F5"},{_key_f6,"F6"},{_key_f7,"F7"},{_key_f8,"F8"},{_key_f9,"F9"},
	{_key_f10,"F10"},{_key_f11,"F11"},{_key_f12,"F12"},{_key_print_screen,"Print Screen"},
	{_key_scroll_lock,"Scroll Lock"},{_key_pause,"Pause"},{_key_backquote,"Backquote"},
	{_key_1,"1"},{_key_2,"2"},{_key_3,"3"},{_key_4,"4"},{_key_5,"5"},{_key_6,"6"},
	{_key_7,"7"},{_key_8,"8"},{_key_9,"9"},{_key_0,"0"},{_key_dash,"-"},{_key_equal,"="},
	{_key_backspace,"Backspace"},{_key_tab,"Tab"},{_key_q,"Q"},{_key_w,"W"},{_key_e,"E"},
	{_key_r,"R"},{_key_t,"T"},{_key_y,"Y"},{_key_u,"U"},{_key_i,"I"},{_key_o,"O"},
	{_key_p,"P"},{_key_left_bracket,"["},{_key_right_bracket,"]"},{_key_backslash,"\\"},
	{_key_caps_lock,"Caps Lock"},{_key_a,"A"},{_key_s,"S"},{_key_d,"D"},{_key_f,"F"},
	{_key_g,"G"},{_key_h,"H"},{_key_j,"J"},{_key_k,"K"},{_key_l,"L"},{_key_semicolon,";"},
	{_key_apostrophe,"'"},{_key_return,"Return"},{_key_left_shift,"Left Shift"},{_key_z,"Z"},
	{_key_x,"X"},{_key_c,"C"},{_key_v,"V"},{_key_b,"B"},{_key_n,"N"},{_key_m,"M"},
	{_key_comma,"Comma"},{_key_period,"."},{_key_forwardslash,"/"},{_key_right_shift,"Right Shift"},
	{_key_left_control,"Left Ctrl"},{_key_left_windows,"Left Command"},{_key_left_alt,"Left Alt"},
	{_key_space,"Space"},{_key_right_alt,"Right Alt"},{_key_right_windows,"Right Command"},
	{_key_menu,"Menu"},{_key_right_control,"Right Ctrl"},{_key_up_arrow,"Up"},{_key_down_arrow,"Down"},
	{_key_left_arrow,"Left"},{_key_right_arrow,"Right"},{_key_insert,"Insert"},{_key_home,"Home"},
	{_key_page_up,"Page Up"},{_key_delete,"Delete"},{_key_end,"End"},{_key_page_down,"Page Down"},
	{_keypad_num_lock,"Keypad Num Lock"},{_keypad_divide,"Keypad /"},{_keypad_multiply,"Keypad *"},
	{_keypad_0,"Keypad 0"},{_keypad_1,"Keypad 1"},{_keypad_2,"Keypad 2"},{_keypad_3,"Keypad 3"},
	{_keypad_4,"Keypad 4"},{_keypad_5,"Keypad 5"},{_keypad_6,"Keypad 6"},{_keypad_7,"Keypad 7"},
	{_keypad_8,"Keypad 8"},{_keypad_9,"Keypad 9"},{_keypad_subtract,"Keypad -"},
	{_keypad_add,"Keypad +"},{_keypad_enter,"Keypad Enter"},{_keypad_decimal,"Keypad ."}
};
static const char *const mac_binding_settings[NUMBER_OF_HALO_KEYBOARD_ACTIONS] = {
	"controls.move_forward","controls.move_backward","controls.strafe_left","controls.strafe_right",
	"controls.jump","controls.crouch","controls.fire","controls.throw_grenade","controls.melee",
	"controls.reload","controls.zoom","controls.switch_weapon","controls.switch_grenade","controls.action",
	"controls.flashlight","controls.scoreboard","controls.pause","controls.screenshot","controls.push_to_talk",
	"controls.chat","controls.team_chat","controls.sprint"
};
static const unsigned mac_binding_actions[NUMBER_OF_HALO_KEYBOARD_ACTIONS] = {
	HALO_KEYBOARD_MOVE_FORWARD,HALO_KEYBOARD_MOVE_BACKWARD,HALO_KEYBOARD_STRAFE_LEFT,HALO_KEYBOARD_STRAFE_RIGHT,
	HALO_KEYBOARD_JUMP,HALO_KEYBOARD_CROUCH,HALO_KEYBOARD_FIRE,HALO_KEYBOARD_THROW_GRENADE,HALO_KEYBOARD_MELEE,
	HALO_KEYBOARD_RELOAD,HALO_KEYBOARD_ZOOM,HALO_KEYBOARD_SWITCH_WEAPON,HALO_KEYBOARD_SWITCH_GRENADE,
	HALO_KEYBOARD_ACTION,HALO_KEYBOARD_FLASHLIGHT,HALO_KEYBOARD_SCOREBOARD,HALO_KEYBOARD_PAUSE,
	HALO_KEYBOARD_SCREENSHOT,HALO_KEYBOARD_PUSH_TO_TALK,
	HALO_KEYBOARD_CHAT,HALO_KEYBOARD_TEAM_CHAT,HALO_KEYBOARD_SPRINT
};
static void mac_binding_capture_update(void);
static void mac_bindings_read(void);
static int mac_binding_action_held(unsigned action);
static void mac_screenshot_poll(void);

static int mac_name_equal(const char *a,const char *b)
{
	unsigned char ca,cb;
	if(!a||!b)return 0;
	while(*a&&*b){ca=(unsigned char)*a++;cb=(unsigned char)*b++;if(ca>='a'&&ca<='z')ca-=32;if(cb>='a'&&cb<='z')cb-=32;if(ca!=cb)return 0;}
	return *a==0&&*b==0;
}

int halo_input_from_name(const char *name)
{
	unsigned i;
	for(i=0;i<NUMBEROF(mac_key_names);++i)if(mac_name_equal(name,mac_key_names[i].name))return mac_key_names[i].key;
	if(mac_name_equal(name,"Left Control"))return _key_left_control;
	if(mac_name_equal(name,"Right Control"))return _key_right_control;
	if(mac_name_equal(name,"Mouse Left"))return MAC_BIND_MOUSE_LEFT;
	if(mac_name_equal(name,"Mouse Right"))return MAC_BIND_MOUSE_RIGHT;
	if(mac_name_equal(name,"Mouse Middle"))return MAC_BIND_MOUSE_MIDDLE;
	if(mac_name_equal(name,"Mouse 4"))return MAC_BIND_MOUSE_4;
	if(mac_name_equal(name,"Mouse 5"))return MAC_BIND_MOUSE_5;
	if(mac_name_equal(name,"Wheel"))return MAC_BIND_WHEEL;
	if(mac_name_equal(name,"Wheel Up"))return MAC_BIND_WHEEL_UP;
	if(mac_name_equal(name,"Wheel Down"))return MAC_BIND_WHEEL_DOWN;
	return -1;
}

void halo_input_name(int input,char *name,size_t size)
{
	unsigned i;size_t length;
	const char *text="";
	if(!name||!size)return;
	if(input>=0&&input<NUMBER_OF_KEYS){for(i=0;i<NUMBEROF(mac_key_names);++i)if(mac_key_names[i].key==input){text=mac_key_names[i].name;break;}}
	else if(input==MAC_BIND_MOUSE_LEFT)text="Mouse Left";
	else if(input==MAC_BIND_MOUSE_RIGHT)text="Mouse Right";
	else if(input==MAC_BIND_MOUSE_MIDDLE)text="Mouse Middle";
	else if(input==MAC_BIND_MOUSE_4)text="Mouse 4";
	else if(input==MAC_BIND_MOUSE_5)text="Mouse 5";
	else if(input==MAC_BIND_WHEEL)text="Wheel";
	else if(input==MAC_BIND_WHEEL_UP)text="Wheel Up";
	else if(input==MAC_BIND_WHEEL_DOWN)text="Wheel Down";
	for(length=0;text[length];++length){}
	if(length>=size)length=size-1;
	memcpy(name,text,length);name[length]=0;
}

typedef char mac_engine_mouse_size[sizeof(struct mouse_state) == 0x10 ? 1 : -1];
typedef char mac_engine_mouse_buttons_offset[
	__builtin_offsetof(struct mouse_state, buttons) == 0xC ? 1 : -1];
typedef char mac_engine_keyboard_count[
	NUMBER_OF_KEYS <= 128 ? 1 : -1];

void mac_engine_input_prepare(
	struct mac_input_frame const *frame,
	unsigned int allowed_actions,
	int movement_enabled,
	int look_enabled,
	struct mac_engine_input *prepared)
{
	unsigned int held;
	if (!prepared)
		return;
	memset(prepared, 0, sizeof(*prepared));
	if (!frame)
		return;
	held = frame->held & allowed_actions;
	if (movement_enabled)
	{
		prepared->throttle.i = frame->forward;
		prepared->throttle.j = frame->left;
	}
	if (look_enabled)
	{
		prepared->facing_delta.yaw = frame->yaw;
		prepared->facing_delta.pitch = frame->pitch;
	}
	if (held & MAC_INPUT_JUMP)
		prepared->unit_control_flags |= FLAG(_unit_control_jump_bit);
	if (held & MAC_INPUT_CROUCH)
		prepared->unit_control_flags |= FLAG(_unit_control_crouch_modifier_bit);
	if (held & MAC_INPUT_ACTION)
		prepared->unit_control_flags |= FLAG(_unit_control_action_bit) |
			FLAG(UNIT_CONTROL_PORT_ACTION_ONLY_BIT);
	if (held & MAC_INPUT_FIRE)
	{
		prepared->unit_control_flags |= FLAG(_unit_control_weapon_primary_trigger_bit);
		prepared->primary_trigger = 1.f;
	}
	if (held & MAC_INPUT_GRENADE)
		prepared->unit_control_flags |= FLAG(_unit_control_throw_grenade_bit) |
			FLAG(_unit_control_weapon_secondary_trigger_bit);
	prepared->pause_pressed =
		(frame->pressed & allowed_actions & MAC_INPUT_PAUSE) != 0;
}

void mac_engine_input_action(
	struct mac_engine_input const *prepared,
	struct player_control const *resolved_control,
	struct player_action *action)
{
	if (!action)
		return;
	memset(action, 0, sizeof(*action));
	if (!prepared || !resolved_control)
		return;
	action->control_flags = prepared->unit_control_flags;
	action->throttle = prepared->throttle;
	action->primary_trigger = prepared->primary_trigger;
	action->desired_facing = resolved_control->desired_angles;
	action->desired_weapon_index = resolved_control->desired_weapon_index;
	action->desired_grenade_index = resolved_control->desired_grenade_index;
	action->desired_zoom_level = resolved_control->zoom_level;
}

static void mac_engine_input_set_key(short key, boolean down)
{
	if (key >= 0 && key < NUMBER_OF_KEYS)
		mac_key_down[key] = down;
}

static long mac_engine_input_mouse_delta(float radians)
{
	float delta;
	if (radians != radians)
		return 0;
	delta = -radians / 0.0022f;
	if (delta > 4096.0f)
		delta = 4096.0f;
	else if (delta < -4096.0f)
		delta = -4096.0f;
	return (long)delta;
}

static long mac_engine_input_wheel(float delta)
{
	if (delta != delta)
		return 0;
	if (delta > 4096.0f)
		delta = 4096.0f;
	else if (delta < -4096.0f)
		delta = -4096.0f;
	return (long)delta;
}

static void mac_engine_input_enqueue(short key, char ascii)
{
	struct key_stroke stroke;
	if (mac_key_queue_count >= NUMBEROF(mac_key_queue))
		return;
	stroke.modifier_flags = 0;
	if (mac_key_down[_key_left_shift] || mac_key_down[_key_right_shift])
		SET_FLAG(stroke.modifier_flags, 0, TRUE);
	if (mac_key_down[_key_left_control] || mac_key_down[_key_right_control])
		SET_FLAG(stroke.modifier_flags, 1, TRUE);
	if (mac_key_down[_key_left_alt] || mac_key_down[_key_right_alt])
		SET_FLAG(stroke.modifier_flags, 2, TRUE);
	stroke.ascii_code = ascii ? ascii : (char)NONE;
	stroke.key_code = key;
	mac_key_queue[mac_key_queue_count++] = stroke;
}

static void mac_engine_input_apply_frame(void)
{
	unsigned i;
	short key;

	memcpy(mac_key_previous, mac_key_down, sizeof(mac_key_down));
	memset(mac_key_down, 0, sizeof(mac_key_down));
	for (i = 0; i < NUMBEROF(mac_key_mappings); i++)
	{
		const struct mac_key_mapping *mapping = &mac_key_mappings[i];
		if (mac_pending_frame.keys[mapping->mac_keycode])
			mac_engine_input_set_key(mapping->game_keycode, TRUE);
	}

	mac_mouse_state.buttons[0] = (mac_pending_frame.mouse_buttons & 1U) != 0;
	mac_mouse_state.buttons[1] = (mac_pending_frame.mouse_buttons & 2U) != 0;
	mac_mouse_state.buttons[2] = (mac_pending_frame.mouse_buttons & 4U) != 0;
	mac_mouse_state.buttons[3] = (mac_pending_frame.mouse_buttons & 8U) != 0;
	mac_mouse_state.x = mac_engine_input_mouse_delta(mac_pending_frame.yaw);
	mac_mouse_state.y = mac_engine_input_mouse_delta(mac_pending_frame.pitch);
	mac_mouse_look_in_frame = mac_pending_frame.yaw != 0.0f || mac_pending_frame.pitch != 0.0f;
	mac_mouse_state.wheel = mac_engine_input_wheel(mac_pending_frame.wheel);
	mac_mouse_look_yaw = mac_pending_frame.yaw;
	mac_mouse_look_pitch = mac_pending_frame.pitch;
	mac_wheel_switch_pending = mac_pending_frame.wheel != 0.0f;
	mac_wheel_direction = mac_pending_frame.wheel > 0.0f ? 1 : mac_pending_frame.wheel < 0.0f ? -1 : 0;

	/* the keyboard and mouse were used: a key or button went down, the wheel
	   turned, or the mouse moved some distance (the tracker's hysteresis) */
	{
		float moved = mac_pending_frame.yaw / 0.0022f;
		float moved_pitch = mac_pending_frame.pitch / 0.0022f;
		if (moved != moved || moved_pitch != moved_pitch)
			moved = moved_pitch = 0.0f;
		for (key = 0; key < NUMBER_OF_KEYS; key++)
			if (mac_key_down[key] && !mac_key_previous[key])
			{
				input_prompt_tracker_keyboard(&mac_prompt_tracker);
				break;
			}
		if ((mac_pending_frame.mouse_buttons & ~mac_mouse_buttons_previous) || mac_pending_frame.wheel != 0.0f)
			input_prompt_tracker_keyboard(&mac_prompt_tracker);
		input_prompt_tracker_motion(&mac_prompt_tracker,
			(moved < 0.0f ? -moved : moved) + (moved_pitch < 0.0f ? -moved_pitch : moved_pitch) +
			(mac_pending_frame.pointer_moved ? 4.0f : 0.0f));
	}
	mac_key_queue_count = 0;
	mac_key_queue_read = 0;
	if (!mac_input_suppressed)
	{
		for (key = 0; key < NUMBER_OF_KEYS; key++)
		{
			if (mac_key_down[key] && !mac_key_previous[key])
			{
				char ascii = 0;
				unsigned text_index, mapping_index;
				for (text_index = 0; text_index < mac_pending_frame.text_count && !ascii; text_index++)
					for (mapping_index = 0; mapping_index < NUMBEROF(mac_key_mappings); mapping_index++)
						if (mac_key_mappings[mapping_index].mac_keycode == mac_pending_frame.text[text_index].key &&
							mac_key_mappings[mapping_index].game_keycode == key)
						{
							ascii = mac_pending_frame.text[text_index].character;
							break;
						}
				mac_engine_input_enqueue(key, ascii);
			}
		}
		/* Carbon key-repeat events carry text even though the key is already
		   down; preserve those characters for the original edit-text consumer. */
		for (i = 0; i < mac_pending_frame.text_count; i++)
		{
			unsigned mapping_index;
			short mapped_key = NONE;
			for (mapping_index = 0; mapping_index < NUMBEROF(mac_key_mappings); mapping_index++)
				if (mac_key_mappings[mapping_index].mac_keycode == mac_pending_frame.text[i].key)
				{
					mapped_key = mac_key_mappings[mapping_index].game_keycode;
					break;
				}
			if (mapped_key != NONE && mac_key_down[mapped_key] && mac_key_previous[mapped_key])
				mac_engine_input_enqueue(mapped_key, mac_pending_frame.text[i].character);
		}
	}
	mac_binding_capture_update();
	mac_mouse_buttons_previous=mac_pending_frame.mouse_buttons;
	/* A caller may update input more than once before publishing another
	   Carbon snapshot. Held keys/buttons persist; deltas and wheel are edges. */
	mac_pending_frame.yaw = 0.0f;
	mac_pending_frame.pitch = 0.0f;
	mac_pending_frame.wheel = 0.0f;
	mac_pending_frame.text_count = 0;
}

void mac_engine_input_publish(const struct mac_input_frame *frame)
{
	if (frame)
		mac_pending_frame = *frame;
	else
		memset(&mac_pending_frame, 0, sizeof(mac_pending_frame));
	if (!frame)
	{
		mac_mouse_look_yaw = mac_mouse_look_pitch = 0.0f;
		mac_wheel_switch_pending = FALSE;
	}
}

/* player_control.c consumes mouse aim directly in radians, independently of
   the gamepad abstraction's normalized stick state. */
int halo_linux_mouse_look(short gamepad_index, float *yaw, float *pitch)
{
	if (!yaw || !pitch)
		return FALSE;
	*yaw = *pitch = 0.0f;
	if (!mac_input_initialized || mac_input_suppressed || gamepad_index != 0)
		return FALSE;
	/* port: the profile's mouse settings, as xinput_sdl.c applies them
	   (input.mouse_sensitivity, input.mouse_vertical_sensitivity,
	   input.invert_mouse; read again when the settings change) */
	{
		static float sensitivity = 1.0f, vertical_sensitivity = 1.0f;
		static int invert;
		static unsigned long read_at = (unsigned long)-1;

		if (read_at != config_changes())
		{
			read_at = config_changes();
			sensitivity = (float)config_real("input.mouse_sensitivity");
			if (sensitivity <= 0.0f)
				sensitivity = 1.0f;
			vertical_sensitivity = (float)config_real("input.mouse_vertical_sensitivity");
			if (vertical_sensitivity <= 0.0f)
				vertical_sensitivity = sensitivity;
			invert = config_boolean("input.invert_mouse");
		}
		*yaw = mac_mouse_look_yaw * sensitivity;
		*pitch = mac_mouse_look_pitch * vertical_sensitivity * (invert ? -1.0f : 1.0f);
	}
	mac_mouse_look_yaw = mac_mouse_look_pitch = 0.0f;
	return *yaw != 0.0f || *pitch != 0.0f;
}

boolean input_initialize(void)
{
	memset(&mac_pending_frame, 0, sizeof(mac_pending_frame));
	memset(&mac_mouse_state, 0, sizeof(mac_mouse_state));
	memset(mac_virtual_gamepad, 0, sizeof(mac_virtual_gamepad));
	memset(mac_port_connected, 0, sizeof(mac_port_connected));
	memset(&mac_suppressed_gamepad, 0, sizeof(mac_suppressed_gamepad));
	memset(mac_key_down, 0, sizeof(mac_key_down));
	memset(mac_key_previous, 0, sizeof(mac_key_previous));
	mac_key_queue_count = mac_key_queue_read = 0;
	mac_input_suppressed = FALSE;
	mac_native_text_typing = FALSE;
	mac_mouse_look_in_frame = FALSE;
	mac_mouse_look_yaw = mac_mouse_look_pitch = 0.0f;
	mac_wheel_switch_pending = FALSE;
	mac_mouse_buttons_previous=0;mac_wheel_direction=0;
	mac_binding_capture_waiting=mac_binding_capture_ready=mac_binding_capture_result=0;
	mac_binding_captured_input=-1;mac_bindings_read_at=(unsigned long)-1;
	mac_input_initialized = TRUE;
	return TRUE;
}

void input_dispose(void)
{
	mac_engine_input_publish(NULL);
	memset(&mac_mouse_state, 0, sizeof(mac_mouse_state));
	memset(mac_virtual_gamepad, 0, sizeof(mac_virtual_gamepad));
	memset(mac_port_connected, 0, sizeof(mac_port_connected));
	memset(&mac_suppressed_gamepad, 0, sizeof(mac_suppressed_gamepad));
	memset(mac_key_down, 0, sizeof(mac_key_down));
	memset(mac_key_previous, 0, sizeof(mac_key_previous));
	mac_key_queue_count = mac_key_queue_read = 0;
	mac_input_suppressed = FALSE;
	mac_native_text_typing = FALSE;
	mac_mouse_look_in_frame = FALSE;
	mac_mouse_look_yaw = mac_mouse_look_pitch = 0.0f;
	mac_wheel_switch_pending = FALSE;
	mac_mouse_buttons_previous=0;mac_wheel_direction=0;
	mac_binding_capture_waiting=mac_binding_capture_ready=mac_binding_capture_result=0;
	mac_binding_captured_input=-1;mac_bindings_read_at=(unsigned long)-1;
	mac_input_initialized = FALSE;
}

void input_activate(void) { }

void input_deactivate(void)
{
	mac_engine_input_publish(NULL);
	memset(mac_key_down, 0, sizeof(mac_key_down));
	memset(&mac_mouse_state, 0, sizeof(mac_mouse_state));
}

void input_flush(void)
{
	mac_engine_input_publish(NULL);
	memset(mac_key_down, 0, sizeof(mac_key_down));
	memset(mac_key_previous, 0, sizeof(mac_key_previous));
	memset(&mac_mouse_state, 0, sizeof(mac_mouse_state));
	memset(mac_virtual_gamepad, 0, sizeof(mac_virtual_gamepad));
	memset(mac_port_connected, 0, sizeof(mac_port_connected));
	mac_key_queue_count = mac_key_queue_read = 0;
}

boolean input_key_is_down(short key_code)
{
	if (!mac_input_initialized || mac_input_suppressed)
		return FALSE;
	if (key_code == _key_shift)
		return mac_key_down[_key_left_shift] || mac_key_down[_key_right_shift];
	if (key_code == _key_control)
		return mac_key_down[_key_left_control] || mac_key_down[_key_right_control];
	if (key_code == _key_alt)
		return mac_key_down[_key_left_alt] || mac_key_down[_key_right_alt];
	if (key_code == _key_windows)
		return mac_key_down[_key_left_windows] || mac_key_down[_key_right_windows];
	if (key_code < 0 || key_code >= NUMBER_OF_KEYS)
		return FALSE;
	return mac_key_down[key_code] != 0;
}

boolean input_get_key(struct key_stroke *key)
{
	if (!mac_input_initialized || !key || mac_key_queue_read >= mac_key_queue_count)
		return FALSE;
	*key = mac_key_queue[mac_key_queue_read++];
	return TRUE;
}

const struct mouse_state *input_get_mouse_state(void)
{
	return !mac_input_initialized || mac_input_suppressed ? NULL : &mac_mouse_state;
}

void input_suppress(void) { mac_input_suppressed = TRUE; }

boolean input_mouse_button_is_down(short button_index)
{
	if (!mac_input_initialized || mac_input_suppressed || button_index < 0 || button_index >= MAC_INPUT_MOUSE_BUTTONS)
		return FALSE;
	return mac_mouse_state.buttons[button_index] != 0;
}

boolean input_has_gamepad(short gamepad_index)
{
	/* Port 0 is the real keyboard/mouse virtual control source, like the
	   shared SDL backend. Ports 1-3 are the game pads that operate the
	   controllers 2 to 4 (mac_pad_assign). */
	if (!mac_input_initialized || gamepad_index < 0 || gamepad_index >= MAXIMUM_GAMEPADS)
		return FALSE;
	return gamepad_index == 0 || mac_port_connected[gamepad_index];
}

const struct gamepad_state *input_get_gamepad_state(short gamepad_index)
{
	if (!input_has_gamepad(gamepad_index))
		return NULL;
	return mac_input_suppressed ? &mac_suppressed_gamepad : &mac_virtual_gamepad[gamepad_index];
}

void input_vertical_blank_interrupt(void) { }

void input_get_raw_data_string(char *buffer, short size)
{
	static const char description[] = "native keyboard/mouse virtual port 0; HID game pads on ports 0 to 3";
	short index;
	if (!buffer || size <= 0)
		return;
	for (index = 0; index < size - 1 && description[index]; index++)
		buffer[index] = description[index];
	buffer[index] = 0;
}

/* The game pad's analog button thresholds and hold counts, as the Xbox's
   input_xbox.c keeps them: a button counts the ticks it has been held. */
enum
{
	MAC_PAD_PRESS_THRESHOLD_OFFSET = 32,
	MAC_PAD_RELEASE_THRESHOLD_OFFSET = 64,
	MAC_PAD_RELEASE_SATURATION = 191,
	MAC_PAD_DEFAULT_DEAD_ZONE = 9000
};
static long mac_pad_down_times[MAXIMUM_GAMEPADS][NUMBER_OF_GAMEPAD_BUTTONS];
static unsigned long mac_pad_settings_read_at = (unsigned long)-1;
static short mac_pad_left_dead_zone = MAC_PAD_DEFAULT_DEAD_ZONE;
static short mac_pad_right_dead_zone = MAC_PAD_DEFAULT_DEAD_ZONE;
static boolean mac_pad_invert_look = FALSE;

static void mac_pad_hold(byte *ticks, long *down_time, boolean down)
{
	if (!down)
		*ticks = 0;
	else if (*ticks == 0)
	{
		*ticks = 1;
		*down_time = (long)system_milliseconds();
	}
	else
	{
		long held = (long)system_milliseconds() - *down_time;
		held = held < 0 ? 0 : MIN(held, (long)UNSIGNED_CHAR_MAX * 1000 / TICKS_PER_SECOND);
		*ticks = (byte)PIN(1 + held * TICKS_PER_SECOND / 1000, 2, UNSIGNED_CHAR_MAX);
	}
}

static void mac_pad_threshold(byte *threshold, byte value, boolean pressed)
{
	byte next;
	if (pressed)
	{
		next = value < MAC_PAD_PRESS_THRESHOLD_OFFSET ? 0 : (byte)(value - MAC_PAD_PRESS_THRESHOLD_OFFSET);
		if (next > *threshold)
			*threshold = next;
	}
	else
	{
		next = value > MAC_PAD_RELEASE_SATURATION ? UNSIGNED_CHAR_MAX :
			(byte)(value + MAC_PAD_RELEASE_THRESHOLD_OFFSET);
		if (next < *threshold)
			*threshold = next;
	}
}

static short mac_pad_dead_zone(short value, short dead_range)
{
	if (value > dead_range)
		return (short)((long)(value - dead_range) * SHORT_MAX / (SHORT_MAX - dead_range));
	if (value < -dead_range)
		return (short)((long)(value + dead_range) * SHORT_MIN / (SHORT_MIN + dead_range));
	return 0;
}

static short mac_pad_dead_zone_setting(const char *name)
{
	long value = config_integer(name);
	return (short)(value < 0 ? 0 : value > 30000 ? 30000 : value);
}

/* The first HID game pad (mac_gamepad.c) as port 0's game pad, beside the
   keyboard and mouse, as the shared SDL backend makes a controller. */
/* Buttons of the first game pad that the keyboard presses (menu mode):
   masks of the analog buttons, and of the binary ones (up, down, left,
   right, start, back, ...) */
static unsigned mac_keyboard_pad_analog, mac_keyboard_pad_binary;
void mac_engine_input_keyboard_pad(unsigned analog_mask, unsigned binary_mask)
{
	mac_keyboard_pad_analog = analog_mask;
	mac_keyboard_pad_binary = binary_mask;
}

/* the pad was used (not the keyboard standing in for it): a button newly
   down, or a stick or trigger moved while it is past its dead zone, so a
   stick held still does not take the prompts back from the mouse */
static void mac_pad_note_activity(int port, const struct mac_gamepad_frame *frame)
{
	short values[4];
	unsigned index;
	int active = FALSE;

	values[0] = frame->left_x;
	values[1] = frame->left_y;
	values[2] = frame->right_x;
	values[3] = frame->right_y;
	if (frame->buttons & ~mac_pad_previous_buttons[port])
		active = TRUE;
	for (index = 0; index < MAC_GAMEPAD_ANALOG_COUNT; index++)
		if (frame->analog[index] > 64 && mac_pad_previous_analog[port][index] <= 64)
			active = TRUE;
	for (index = 0; index < 4; index++)
	{
		long value = values[index], was = mac_pad_previous_values[port][index];
		long size = value < 0 ? -value : value, change = value - was;
		if (change < 0)
			change = -change;
		if (size > 12000 && change > 1500)
			active = TRUE;
	}
	mac_pad_previous_buttons[port] = frame->buttons;
	for (index = 0; index < MAC_GAMEPAD_ANALOG_COUNT; index++)
		mac_pad_previous_analog[port][index] = frame->analog[index];
	for (index = 0; index < 4; index++)
		mac_pad_previous_values[port][index] = values[index];
	if (active)
		input_prompt_tracker_gamepad(&mac_prompt_tracker);
}

/* the slot of the pad at port 0 (the first pad, or the lone pad even while
   it plays as port 1), for input.prompt_glyphs */
static int mac_port0_slot = -1;

int platform_prompts_playstation(void)
{
	return mac_gamepad_is_playstation_at(mac_port0_slot);
}

/* input.prompts: whether the prompts name the keyboard and mouse's controls */
int platform_prompts_keyboard(void)
{
	static unsigned long read_at = (unsigned long)-1;
	static int mode;

	if (read_at != config_changes())
	{
		read_at = config_changes();
		mode = input_prompts_mode(config_string("input.prompts"));
	}
	return mode == 1 ? 1 : mode == 2 ? 0 : mac_prompt_tracker.keyboard;
}

/* diagnostic (tools/mac_session_interactive.c, HALO_MAC_INPUT_DEVICE): the
   left stick pushed for the next update, as a pad in hand */
void mac_engine_input_script_pad(short left_x)
{
	mac_script_pad_left_x = left_x;
}

/* ---------- the game pads of the controllers (local multiplayer)

   As the Linux port's xinput_sdl.c assigns them: the first game pad adds to
   controller 1, beside the keyboard and mouse; the others operate the
   controllers 2 to 4 (a pad keeps its slot while it is connected, so a pad
   that goes does not move the others). With two or more players on this
   computer and one pad, that pad is controller 2 and the keyboard and mouse
   stay controller 1; the pad changes controller only when none of its
   buttons is held (a button held across the change would be pressed again
   on the other controller). */

/* port: whether this machine has, or is adding, a second player (co-op,
   split screen, the lobby's: port/linux/game/menu_functions.c); tools that
   build the input without the menus have no second player */
__attribute__((weak)) unsigned char pc_menu_split_players(void)
{
	return 0;
}

static boolean mac_lone_pad_split;

static boolean mac_pad_frame_idle(const struct mac_gamepad_frame *frame)
{
	unsigned index;
	if (frame->buttons)
		return FALSE;
	for (index = 0; index < MAC_GAMEPAD_ANALOG_COUNT; index++)
		if (frame->analog[index] > 64)
			return FALSE;
	return abs(frame->left_x) <= 8000 && abs(frame->left_y) <= 8000 &&
		abs(frame->right_x) <= 8000 && abs(frame->right_y) <= 8000;
}

/* the slot of the pad at each port, or -1 (for the rumble) */
static int mac_port_slot[MAXIMUM_GAMEPADS] = { -1, -1, -1, -1 };

/* the pads by slot to the ports; ports[0] keeps the keyboard's controller */
static void mac_pad_assign(const struct mac_gamepad_frame slots[MAC_GAMEPAD_MAXIMUM_DEVICES],
	struct mac_gamepad_frame ports[MAXIMUM_GAMEPADS])
{
	int slot, count = 0, only = 0;

	memset(ports, 0, sizeof(ports[0]) * MAXIMUM_GAMEPADS);
	mac_port0_slot = -1;
	for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES; slot++)
		if (slots[slot].connected)
		{
			count++;
			only = slot;
		}
	mac_port0_slot = count == 1 ? only : 0;
	for (slot = 0; slot < MAXIMUM_GAMEPADS; slot++)
		mac_port_slot[slot] = -1;
	if (count == 1)
	{
		boolean split = pc_menu_split_players() != 0;

		if (split != mac_lone_pad_split && mac_pad_frame_idle(&slots[only]))
			mac_lone_pad_split = split;
		ports[mac_lone_pad_split ? 1 : 0] = slots[only];
		mac_port_slot[mac_lone_pad_split ? 1 : 0] = only;
	}
	else
		for (slot = 0; slot < MAC_GAMEPAD_MAXIMUM_DEVICES && slot < MAXIMUM_GAMEPADS; slot++)
		{
			ports[slot] = slots[slot];
			mac_port_slot[slot] = slot;
		}
}

/* port: the game's rumble (input_set_gamepad_rumbler_state): the motors of the pad at a port, which only a Sony pad on
   USB has (mac_gamepad_rumble); the large motor is the left, the small one the right. Returns 1 when a pad took it. */
int mac_engine_input_rumble(unsigned port, unsigned short left, unsigned short right)
{
	int slot = port < MAXIMUM_GAMEPADS ? mac_port_slot[port] : -1;

	return mac_gamepad_rumble(slot, (unsigned char)(left >> 8), (unsigned char)(right >> 8));
}

/* ---------- diagnostic: virtual game pads
   HALO_MAC_VIRTUAL_PADS=N (1 to 4) replaces the real pads with N pads that
   HALO_MAC_PAD_KEYS drives, a token for each HALO_MAC_PAD_KEY_FRAMES
   updates (10): items joined by +, each pN.button (N is the pad, 1 to 4;
   a b x y black white lt rt up down left right start back l3 r3) or
   pN.lx=, ly=, rx=, ry= with a value (-32768 to 32767), held for the token;
   "wait" is a token with nothing held; *K after a token repeats it K times.
   e.g. HALO_MAC_PAD_KEYS="wait*3,p2.start,wait,p2.a,p2.ly=30000*20". The
   count is of input updates, one for each frame of a hidden run, so the
   same interval as HALO_MAC_MENU_KEY_FRAMES lines the two scripts up. */
enum { MAC_VPAD_TOKENS = 512, MAC_VPAD_ITEMS = 8 };
struct mac_vpad_item
{
	unsigned char pad, kind, index;
	short value;
};
static struct
{
	int parsed, pads, interval, count;
	unsigned long updates;
	struct { unsigned short repeat; unsigned char count; struct mac_vpad_item item[MAC_VPAD_ITEMS]; } token[MAC_VPAD_TOKENS];
} mac_vpad;

static const char *const mac_vpad_button_names[] =
{
	"a", "b", "x", "y", "black", "white", "lt", "rt",	/* analog 0-7 */
	"up", "down", "left", "right", "start", "back", "l3", "r3"	/* binary bits 0-7 */
};
static const char *const mac_vpad_axis_names[] = { "lx", "ly", "rx", "ry" };

static void mac_vpad_parse_token(const char *text, size_t length)
{
	char word[160];
	char *cursor, *star;
	int repeat = 1;
	if (length >= sizeof(word) || mac_vpad.count >= MAC_VPAD_TOKENS)
		return;
	memcpy(word, text, length);
	word[length] = 0;
	star = strchr(word, '*');
	if (star)
	{
		repeat = atoi(star + 1);
		*star = 0;
	}
	mac_vpad.token[mac_vpad.count].repeat = (unsigned short)(repeat < 1 ? 1 : repeat);
	mac_vpad.token[mac_vpad.count].count = 0;
	cursor = word;
	while (*cursor && strcmp(word, "wait"))
	{
		char *end = strchr(cursor, '+');
		char *dot, *equals;
		struct mac_vpad_item item;
		unsigned name;
		int found = FALSE;
		if (end)
			*end = 0;
		memset(&item, 0, sizeof(item));
		dot = strchr(cursor, '.');
		if (cursor[0] == 'p' && cursor[1] >= '1' && cursor[1] <= '4' && dot == cursor + 2)
		{
			item.pad = (unsigned char)(cursor[1] - '1');
			equals = strchr(dot + 1, '=');
			if (equals)
			{
				*equals = 0;
				for (name = 0; name < 4; name++)
					if (!strcmp(dot + 1, mac_vpad_axis_names[name]))
					{
						item.kind = 2;
						item.index = (unsigned char)name;
						item.value = (short)atoi(equals + 1);
						found = TRUE;
					}
			}
			else
				for (name = 0; name < 16; name++)
					if (!strcmp(dot + 1, mac_vpad_button_names[name]))
					{
						item.kind = name < 8 ? 0 : 1;
						item.index = (unsigned char)(name < 8 ? name : name - 8);
						found = TRUE;
					}
		}
		if (found && mac_vpad.token[mac_vpad.count].count < MAC_VPAD_ITEMS)
			mac_vpad.token[mac_vpad.count].item[mac_vpad.token[mac_vpad.count].count++] = item;
		else if (!found)
			fprintf(stderr, "pad keys: unknown item %s\n", cursor);
		if (!end)
			break;
		cursor = end + 1;
	}
	mac_vpad.count++;
}

static void mac_vpad_parse(void)
{
	const char *text = getenv("HALO_MAC_PAD_KEYS"), *every = getenv("HALO_MAC_PAD_KEY_FRAMES");
	const char *pads = getenv("HALO_MAC_VIRTUAL_PADS");
	mac_vpad.parsed = 1;
	mac_vpad.pads = pads ? atoi(pads) : 0;
	if (mac_vpad.pads < 0)
		mac_vpad.pads = 0;
	if (mac_vpad.pads > MAC_GAMEPAD_MAXIMUM_DEVICES)
		mac_vpad.pads = MAC_GAMEPAD_MAXIMUM_DEVICES;
	mac_vpad.interval = every && atoi(every) > 0 ? atoi(every) : 10;
	while (mac_vpad.pads && text && *text)
	{
		size_t length = strcspn(text, ", ");
		if (length)
			mac_vpad_parse_token(text, length);
		text += length;
		while (*text == ',' || *text == ' ')
			text++;
	}
	if (mac_vpad.pads)
		fprintf(stderr, "virtual pads: %d, %d tokens, one every %d updates\n", mac_vpad.pads,
			mac_vpad.count, mac_vpad.interval);
}

/* the virtual pads' states for this update (the real pads' place) */
static int mac_vpad_poll(struct mac_gamepad_frame slots[MAC_GAMEPAD_MAXIMUM_DEVICES])
{
	unsigned long step;
	int pad, index, token = 0;
	if (!mac_vpad.parsed)
		mac_vpad_parse();
	if (!mac_vpad.pads)
		return -1;
	memset(slots, 0, sizeof(slots[0]) * MAC_GAMEPAD_MAXIMUM_DEVICES);
	for (pad = 0; pad < mac_vpad.pads; pad++)
		slots[pad].connected = 1;
	step = mac_vpad.updates++ / (unsigned long)mac_vpad.interval;
	/* the token holding this step (a token lasts its repeats) */
	for (index = 0; index < mac_vpad.count; index++)
	{
		if (step < mac_vpad.token[index].repeat)
		{
			token = index + 1;
			break;
		}
		step -= mac_vpad.token[index].repeat;
	}
	if (token)
		for (index = 0; index < mac_vpad.token[token - 1].count; index++)
		{
			const struct mac_vpad_item *item = &mac_vpad.token[token - 1].item[index];
			struct mac_gamepad_frame *frame = &slots[item->pad];
			if (item->kind == 0)
				frame->analog[item->index] = 255;
			else if (item->kind == 1)
				frame->buttons |= 1u << item->index;
			else if (item->index == 0)
				frame->left_x = item->value;
			else if (item->index == 1)
				frame->left_y = item->value;
			else if (item->index == 2)
				frame->right_x = item->value;
			else
				frame->right_y = item->value;
		}
	return mac_vpad.pads;
}

/* one port's game pad state from a frame (with the Xbox's holds and thresholds) */
static void mac_pad_apply(int port, const struct mac_gamepad_frame *frame)
{
	struct gamepad_state *pad = &mac_virtual_gamepad[port];
	short index;

	for (index = 0; index < NUMBER_OF_GAMEPAD_ANALOG_BUTTONS; index++)
	{
		pad->analog_buttons[index] = frame->analog[index];
		mac_pad_hold(&pad->buttons[FIRST_GAMEPAD_ANALOG_BUTTON + index],
			&mac_pad_down_times[port][FIRST_GAMEPAD_ANALOG_BUTTON + index],
			pad->analog_buttons[index] > pad->analog_button_thresholds[index]);
		mac_pad_threshold(&pad->analog_button_thresholds[index], pad->analog_buttons[index],
			pad->buttons[FIRST_GAMEPAD_ANALOG_BUTTON + index] != 0);
	}
	for (index = 0; index < NUMBER_OF_GAMEPAD_BINARY_BUTTONS; index++)
		mac_pad_hold(&pad->buttons[FIRST_GAMEPAD_BINARY_BUTTON + index],
			&mac_pad_down_times[port][FIRST_GAMEPAD_BINARY_BUTTON + index],
			(frame->buttons & (1u << index)) != 0);
	/* HALO_MAC_INPUT_LOG (diagnostic): the pad's Back going down or up (port 0's) */
	if (port == 0)
	{
		static int back_was_down = -1, log_input = -1;
		int back_down = pad->buttons[FIRST_GAMEPAD_BINARY_BUTTON + _gamepad_binary_button_back - FIRST_GAMEPAD_BINARY_BUTTON] != 0;

		if (log_input < 0)
			log_input = getenv("HALO_MAC_INPUT_LOG") != NULL;
		if (log_input && back_down != back_was_down)
			fprintf(stderr, "input: pad Back %s\n", back_down ? "down" : "up");
		back_was_down = back_down;
	}
	pad->sticks[_gamepad_stick_left].x = mac_pad_dead_zone(frame->left_x, mac_pad_left_dead_zone);
	pad->sticks[_gamepad_stick_left].y = mac_pad_dead_zone(frame->left_y, mac_pad_left_dead_zone);
	pad->sticks[_gamepad_stick_right].x = mac_pad_dead_zone(frame->right_x, mac_pad_right_dead_zone);
	pad->sticks[_gamepad_stick_right].y = mac_pad_dead_zone(
		mac_pad_invert_look ? (short)(-1 - frame->right_y) : frame->right_y, mac_pad_right_dead_zone);
}

static void mac_pad_update(void)
{
	struct mac_gamepad_frame slots[MAC_GAMEPAD_MAXIMUM_DEVICES], ports[MAXIMUM_GAMEPADS];
	unsigned long version = config_changes();
	short index;
	int port;

	if (version != mac_pad_settings_read_at)
	{
		mac_pad_left_dead_zone = mac_pad_dead_zone_setting("input.gamepad_left_dead_zone");
		mac_pad_right_dead_zone = mac_pad_dead_zone_setting("input.gamepad_right_dead_zone");
		mac_pad_invert_look = config_boolean("input.gamepad_invert_look") ? TRUE : FALSE;
		mac_pad_settings_read_at = version;
	}
	if (mac_vpad_poll(slots) < 0)
		mac_gamepad_poll_all(slots);
	mac_pad_assign(slots, ports);
	{
		/* port: before any input, the prompts name the keyboard unless a pad is there (input.prompts "auto") */
		static boolean prompt_device_chosen;

		if (!prompt_device_chosen)
		{
			int pad;

			prompt_device_chosen = TRUE;
			for (pad = 0; pad < MAXIMUM_GAMEPADS; pad++)
				if (ports[pad].connected)
					input_prompt_tracker_start(&mac_prompt_tracker, 0);
		}
	}
	mac_script_pad_pushed = mac_script_pad_left_x != 0;
	if (mac_script_pad_left_x)
		ports[0].left_x = mac_script_pad_left_x;
	mac_script_pad_left_x = 0;
	for (port = 0; port < MAXIMUM_GAMEPADS; port++)
	{
		boolean connected = port == 0 || ports[port].connected;

		if (!connected)
		{
			/* (a pad gone: its controller is not there) */
			if (mac_port_connected[port])
			{
				memset(&mac_virtual_gamepad[port], 0, sizeof(mac_virtual_gamepad[port]));
				memset(mac_pad_down_times[port], 0, sizeof(mac_pad_down_times[port]));
			}
			mac_port_connected[port] = FALSE;
			continue;
		}
		mac_port_connected[port] = TRUE;
		/* port: only a pad that is there is a device in use: port 0 is always processed (the keyboard's
		   controller), and what it reads with no pad is not a pad's activity (a diagnostic's stick push is) */
		if (ports[port].connected || (port == 0 && mac_script_pad_pushed))
			mac_pad_note_activity(port, &ports[port]);
		if (port == 0)
		{
			/* the keyboard standing in for the pad in the menus (engine_menu.c) */
			for (index = 0; index < NUMBER_OF_GAMEPAD_ANALOG_BUTTONS; index++)
				if (mac_keyboard_pad_analog & (1u << index))
					ports[0].analog[index] = 255;
			ports[0].buttons |= mac_keyboard_pad_binary;
			/* port: the scoreboard key presses the pad's Back in a game, as the shared
			   backend's (xinput_sdl.c keyboard_controls), not while a menu is up or a
			   binding chosen */
			if (!mac_input_suppressed && !mac_native_text_typing && !mac_binding_capture_waiting && !ui_widgets_active())
			{
				mac_bindings_read();
				if (mac_binding_action_held(HALO_KEYBOARD_SCOREBOARD))
					ports[0].buttons |= 0x20u;
			}
		}
		mac_pad_apply(port, &ports[port]);
	}
}

void input_update(void)
{
	if (!mac_input_initialized)
		input_initialize();
	mac_engine_input_apply_frame();
	mac_screenshot_poll();
	mac_pad_update();
	mac_input_suppressed = FALSE;
}

void platform_binding_capture_begin(void)
{
	mac_binding_capture_waiting=1;
	mac_binding_capture_ready=0;
	mac_binding_capture_result=0;
}

int platform_binding_capture_poll(int *input)
{
	int result;
	if(!input||!mac_binding_capture_ready)return 0;
	result=mac_binding_capture_result;
	*input=mac_binding_captured_input;
	mac_binding_capture_ready=0;
	return result;
}

static int mac_binding_is_held(int input)
{
	if(input>=0&&input<NUMBER_OF_KEYS)return mac_key_down[input]!=0;
	switch(input) {
	case MAC_BIND_MOUSE_LEFT:return mac_mouse_state.buttons[0]!=0;
	case MAC_BIND_MOUSE_RIGHT:return mac_mouse_state.buttons[1]!=0;
	case MAC_BIND_MOUSE_MIDDLE:return mac_mouse_state.buttons[2]!=0;
	case MAC_BIND_MOUSE_4:return mac_mouse_state.buttons[3]!=0;
	case MAC_BIND_MOUSE_5:return (mac_pending_frame.mouse_buttons&16U)!=0;
	case MAC_BIND_WHEEL:return mac_wheel_switch_pending!=FALSE;
	case MAC_BIND_WHEEL_UP:return mac_wheel_switch_pending&&mac_wheel_direction>0;
	case MAC_BIND_WHEEL_DOWN:return mac_wheel_switch_pending&&mac_wheel_direction<0;
	default:return 0;
	}
}

static void mac_bindings_read(void)
{
	unsigned action,slot;
	unsigned long version=config_changes();
	if(mac_bindings_read_at==version)return;
	mac_bindings_read_at=version;
	for(action=0;action<NUMBER_OF_HALO_KEYBOARD_ACTIONS;++action) {
		const char *text=config_string(mac_binding_settings[action]);
		for(slot=0;slot<2;++slot) {
			char name[64];size_t length;
			while(*text==' '||*text=='\t'||*text==',')++text;
			length=strcspn(text,",");
			if(length>=sizeof(name))length=sizeof(name)-1;
			memcpy(name,text,length);name[length]=0;
			while(length&& (name[length-1]==' '||name[length-1]=='\t'))name[--length]=0;
			mac_bindings[action][slot]=length?halo_input_from_name(name):-1;
			text+=strcspn(text,",");
		}
	}
}

static int mac_binding_action_held(unsigned action)
{
	return mac_binding_is_held(mac_bindings[action][0])||mac_binding_is_held(mac_bindings[action][1]);
}

/* port: an action's binding in a native snapshot (the menus' pause key) */
int mac_engine_input_bound_held(unsigned action,const struct mac_input_frame *frame)
{
	unsigned slot,index;
	if(!frame||action>=NUMBER_OF_HALO_KEYBOARD_ACTIONS)return 0;
	mac_bindings_read();
	for(slot=0;slot<2;++slot) {
		int input=mac_bindings[action][slot];
		if(input>=0&&input<NUMBER_OF_KEYS) {
			for(index=0;index<NUMBEROF(mac_key_mappings);++index)
				if(mac_key_mappings[index].game_keycode==input&&frame->keys[mac_key_mappings[index].mac_keycode])return 1;
		} else switch(input) {
		case MAC_BIND_MOUSE_LEFT:if(frame->mouse_buttons&1U)return 1;break;
		case MAC_BIND_MOUSE_RIGHT:if(frame->mouse_buttons&2U)return 1;break;
		case MAC_BIND_MOUSE_MIDDLE:if(frame->mouse_buttons&4U)return 1;break;
		case MAC_BIND_MOUSE_4:if(frame->mouse_buttons&8U)return 1;break;
		case MAC_BIND_MOUSE_5:if(frame->mouse_buttons&16U)return 1;break;
		default:break;
		}
	}
	return 0;
}

/* port: the screenshot key, once for each press (taken by the frame loop,
   tools/mac_session_interactive.c, which writes the PNG); not while a
   binding is being chosen or text typed */
static int mac_screenshot_down,mac_screenshot_pending;
static void mac_screenshot_poll(void)
{
	int down;
	if(mac_native_text_typing||mac_binding_capture_waiting) { mac_screenshot_down=1;return; }
	mac_bindings_read();
	down=mac_binding_action_held(HALO_KEYBOARD_SCREENSHOT);
	if(down&&!mac_screenshot_down)mac_screenshot_pending=1;
	mac_screenshot_down=down;
}
int mac_engine_input_screenshot_pressed(void)
{
	int pressed=mac_screenshot_pending;
	mac_screenshot_pending=0;
	return pressed;
}

/* the automated tests (port/linux/game/network_test.c): the scripted player
stands still, holding the action button, to pick a weapon up or get into a
vehicle. xinput_sdl.c holds the pad's X; the keyboard's action is this
build's way to the same control flag. */
static int mac_test_holding_action;

void test_input_hold_action(int hold)
{
	mac_test_holding_action = hold != 0;
}

/* voice chat's key, which the game itself does not read (halo_keyboard.h).
(mac_binding_settings and mac_binding_actions are in halo_keyboard_action's
order, so an action is its own index.) */
int halo_push_to_talk_held(void)
{
	if (!mac_input_initialized || mac_input_suppressed || mac_native_text_typing ||
		mac_binding_capture_waiting)
	{
		return 0;
	}
	mac_bindings_read();
	return mac_binding_action_held(HALO_KEYBOARD_PUSH_TO_TALK) ? 1 : 0;
}

static void mac_binding_capture_update(void)
{
	unsigned i;
	if(!mac_binding_capture_waiting)return;
	for(i=0;i<mac_key_queue_count;++i) {
		short key=mac_key_queue[i].key_code;
		if(key==_key_f11||key==_key_f12)continue;
		mac_binding_captured_input=key;
		mac_binding_capture_result=key==_key_escape?3:key==_key_delete?2:1;
		mac_binding_capture_waiting=0;mac_binding_capture_ready=1;return;
	}
	{
		unsigned pressed=mac_pending_frame.mouse_buttons&~mac_mouse_buttons_previous;
		if(pressed&1U)mac_binding_captured_input=MAC_BIND_MOUSE_LEFT;
		else if(pressed&2U)mac_binding_captured_input=MAC_BIND_MOUSE_RIGHT;
		else if(pressed&4U)mac_binding_captured_input=MAC_BIND_MOUSE_MIDDLE;
		else if(pressed&8U)mac_binding_captured_input=MAC_BIND_MOUSE_4;
		else if(pressed&16U)mac_binding_captured_input=MAC_BIND_MOUSE_5;
		else if(mac_pending_frame.wheel>0.0f)mac_binding_captured_input=MAC_BIND_WHEEL_UP;
		else if(mac_pending_frame.wheel<0.0f)mac_binding_captured_input=MAC_BIND_WHEEL_DOWN;
		else return;
	}
	mac_binding_capture_result=1;mac_binding_capture_waiting=0;mac_binding_capture_ready=1;
}

void input_frame_begin(void) { }

void input_frame_end(void)
{
	mac_mouse_state.x = 0;
	mac_mouse_state.y = 0;
	mac_mouse_state.wheel = 0;
}

unsigned long halo_keyboard_actions(short controller_index)
{
	unsigned long actions = 0;
	unsigned action;
	if (!mac_input_initialized || controller_index != 0 || mac_input_suppressed || mac_native_text_typing || mac_binding_capture_waiting)
		return 0;
	mac_bindings_read();
	for(action=0;action<NUMBER_OF_HALO_KEYBOARD_ACTIONS;++action)
		if(mac_binding_action_held(action))actions|=1UL<<mac_binding_actions[action];
	if (mac_test_holding_action)
		actions |= 1UL << HALO_KEYBOARD_ACTION;
	mac_wheel_switch_pending = FALSE;
	return actions;
}

/* Native keyboard/mouse has no touch surface or gyro. Keep those optional
   port hooks truthful while the real keyboard/mouse path remains available. */
int halo_linux_touch_move(short controller_index, float *forward, float *strafe)
{
	if (forward) *forward = 0.0f;
	if (strafe) *strafe = 0.0f;
	return FALSE;
}

int halo_linux_touch_look(short gamepad_index, float *yaw, float *pitch,
	float *gyro_yaw, float *gyro_pitch)
{
	if (yaw) *yaw = 0.0f;
	if (pitch) *pitch = 0.0f;
	if (gyro_yaw) *gyro_yaw = 0.0f;
	if (gyro_pitch) *gyro_pitch = 0.0f;
	return FALSE;
}

int halo_linux_touch_aiming(short gamepad_index)
{
	return FALSE;
}

int halo_linux_mouse_aiming(short gamepad_index)
{
	return mac_input_initialized && !mac_input_suppressed && gamepad_index == 0 &&
		mac_mouse_look_in_frame && !config_boolean("input.mouse_aim_assist");
}

int mac_engine_input_text_typing(void)
{
	return mac_native_text_typing;
}

void platform_text_typing(int typing)
{
	mac_native_text_typing = typing != 0;
}

/* Menus use the same lifecycle with or without a platform on-screen
   keyboard. Native Carbon text arrives through mac_input_character(); the
   password flag is consumed by the engine edit control, not the host. */
void platform_text_field(int typing, int password)
{
	(void)password;
	platform_text_typing(typing);
}

extern void mac_window_mouse_menus(int menus_active) __attribute__((weak));

static short mac_ui_pointer_coordinate(float value)
{
	if(value>32767.0f)return 32767;
	if(value< -32768.0f)return -32768;
	return (short)value;
}

int halo_ui_pointer_update(int menus_active,struct halo_ui_pointer *pointer)
{
	int window_width,window_height;
	float scale,left,top,x,y,click_x,click_y,picture_width,centring;
	/* port: the window gives the cursor back while a menu is up
	   (mac_window.c; absent from the probes that stub the window) */
	if(mac_window_mouse_menus)mac_window_mouse_menus(menus_active);
	if(!menus_active||!pointer||!mac_input_initialized||!mac_pending_frame.pointer_valid)return 0;
	platform_video_window_size(&window_width,&window_height);
	/* port: HALO_MAC_MENU_WINDOW=WIDTHxHEIGHT (diagnostic): the window size the
	   pointer is mapped through, for a hidden run's scripted pointer (a hidden
	   window is always the picture's size) */
	if(getenv("HALO_MAC_MENU_WINDOW"))
	{
		int test_width=0,test_height=0;
		if(sscanf(getenv("HALO_MAC_MENU_WINDOW"),"%dx%d",&test_width,&test_height)==2&&test_width>0&&test_height>0)
		{
			window_width=test_width;
			window_height=test_height;
		}
	}
	if(window_width<=0||window_height<=0)return 0;
	/* The game's surface is 480 lines by halo_screen_width() columns (more than
	   640 where the window is wider), drawn as large as the window's shape
	   allows, centred; invert that letterbox, and the menus' centring in the
	   wider picture (halo_screen_ui_offset), to deliver the widget system's
	   own coordinates. */
	picture_width=(float)halo_screen_width();
	if(picture_width<640.0f)picture_width=640.0f;
	scale=(float)window_width/picture_width;
	if((float)window_height/480.0f<scale)scale=(float)window_height/480.0f;
	if(scale<=0.0f)return 0;
	left=((float)window_width-picture_width*scale)*0.5f;
	top=((float)window_height-480.0f*scale)*0.5f;
	centring=(picture_width-640.0f)*0.5f;
	x=(mac_pending_frame.pointer_x-left)/scale-centring;
	y=(mac_pending_frame.pointer_y-top)/scale;
	click_x=(mac_pending_frame.click_x-left)/scale-centring;
	click_y=(mac_pending_frame.click_y-top)/scale;
	memset(pointer,0,sizeof(*pointer));
	pointer->x=mac_ui_pointer_coordinate(x);
	pointer->y=mac_ui_pointer_coordinate(y);
	pointer->click_x=mac_ui_pointer_coordinate(click_x);
	pointer->click_y=mac_ui_pointer_coordinate(click_y);
	pointer->moved=mac_pending_frame.pointer_moved;
	pointer->left_clicks=mac_pending_frame.left_clicks;
	pointer->right_clicks=mac_pending_frame.right_clicks;
	if(mac_mouse_state.wheel>8)pointer->wheel_steps=8;
	else if(mac_mouse_state.wheel< -8)pointer->wheel_steps=-8;
	else pointer->wheel_steps=(signed char)mac_mouse_state.wheel;
	return 1;
}
