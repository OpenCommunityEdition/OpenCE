#define fast_ftol mac_engine_input_probe_unused_fast_ftol
#include "cseries.h"
#undef fast_ftol
#include "input/input.h"
#include "mac_input.h"
#include "engine_input.h"
#include "../port/linux/include/halo_keyboard.h"
#include "../port/linux/include/halo_ui_pointer.h"
#undef memset
#undef memcpy
#undef strcmp
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#undef vprintf
#undef vfprintf

struct mouse_state_probe
{
	long x, y, wheel;
	byte buttons[4];
};
static int probe_window_width=1600,probe_window_height=900;
void platform_video_window_size(int *width,int *height)
{*width=probe_window_width;*height=probe_window_height;}

int halo_linux_mouse_look(short gamepad_index, float *yaw, float *pitch);

static int checks, failures;
static unsigned long probe_config_version;
static const char *probe_forward_binding="W";
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr, "input check failed at %d\n", __LINE__); } } while (0)

/* Standalone engine-service boundary used only by this input probe. */
void *csmemset(void *buffer, long value, unsigned long size)
{
	return memset(buffer, (int)value, (size_t)size);
}

void *csmemcpy(void *destination, const void *source, unsigned long size)
{
	return memcpy(destination, source, (size_t)size);
}

/* The probe's only logging goes to the host C library; it does not exercise
   the port's formatted-output service. */
int halo_linux_printf(const char *format, ...)
{
	va_list arguments;
	int result;
	va_start(arguments, format);
	result = vprintf(format, arguments);
	va_end(arguments);
	return result;
}

int halo_linux_fprintf(FILE *stream, const char *format, ...)
{
	va_list arguments;
	int result;
	va_start(arguments, format);
	result = vfprintf(stream, format, arguments);
	va_end(arguments);
	return result;
}

unsigned long config_changes(void) { return probe_config_version; }
int config_boolean(const char *name) { (void)name; return 0; }
const char *config_string(const char *name)
{
	if(!strcmp(name,"controls.move_forward"))return probe_forward_binding;
	if(!strcmp(name,"controls.move_backward"))return "S";
	if(!strcmp(name,"controls.strafe_left"))return "A";
	if(!strcmp(name,"controls.strafe_right"))return "D";
	if(!strcmp(name,"controls.jump"))return "Space";
	if(!strcmp(name,"controls.crouch"))return "Left Ctrl, Right Ctrl";
	if(!strcmp(name,"controls.fire"))return "Mouse Left";
	if(!strcmp(name,"controls.throw_grenade"))return "Mouse Right, G";
	if(!strcmp(name,"controls.melee"))return "F, Mouse 4";
	if(!strcmp(name,"controls.reload"))return "R";
	if(!strcmp(name,"controls.zoom"))return "Z, Mouse Middle";
	if(!strcmp(name,"controls.switch_weapon"))return "Wheel, 1";
	if(!strcmp(name,"controls.switch_grenade"))return "X";
	if(!strcmp(name,"controls.action"))return "E";
	if(!strcmp(name,"controls.flashlight"))return "Q";
	if(!strcmp(name,"controls.scoreboard"))return "Tab";
	if(!strcmp(name,"controls.pause"))return "Escape";
	if(!strcmp(name,"controls.screenshot"))return "F10";
	return "";
}

int main(void)
{
	struct mac_input source;
	struct mac_input_frame frame;
	struct mac_input_frame initial_frame;
	struct mac_input modifier_source;
	struct mac_input_frame modifier_frame;
	struct mac_input pointer_source;
	struct mac_input_frame pointer_frame;
	struct halo_ui_pointer pointer;
	struct key_stroke stroke;
	struct mouse_state_probe const *mouse;
	unsigned long actions;
	char description[64];
	char input_name[64];
	int captured;

	mac_input_init(&source);
	mac_input_focus(&source, 1);
	mac_input_modifiers(&source, MAC_INPUT_MOD_SHIFT | MAC_INPUT_MOD_CONTROL |
		MAC_INPUT_MOD_RIGHT_CONTROL | MAC_INPUT_MOD_OPTION | MAC_INPUT_MOD_COMMAND);
	mac_input_key(&source, 54, 1); /* physical right Command */
	mac_input_key(&source, 55, 1); /* physical left Command */
	mac_input_key(&source, 13, 1); /* physical W */
	mac_input_key(&source, 0, 1);  /* physical A */
	mac_input_key(&source, 12, 1); /* physical Q */
	mac_input_key(&source, 14, 1); /* physical E */
	mac_input_character(&source, 12, 'q'); /* Carbon text input for Q */
	mac_input_button(&source, 1, 1);
	mac_input_button(&source, 2, 1);
	mac_input_button(&source, 4, 1);
	mac_input_motion(&source, 10.f, -5.f);
	mac_input_wheel(&source, 3.f);
	mac_input_snapshot(&source, &frame);
	initial_frame=frame;
	CHECK(frame.forward > 0.7f && frame.left > 0.7f);
	CHECK(frame.keys[56] && frame.keys[62] && frame.keys[58]);
	CHECK(frame.keys[54] && frame.keys[55]);
	mac_input_init(&modifier_source);
	mac_input_focus(&modifier_source, 1);
	mac_input_modifiers(&modifier_source, MAC_INPUT_MOD_SHIFT | MAC_INPUT_MOD_CONTROL |
		MAC_INPUT_MOD_RIGHT_CONTROL | MAC_INPUT_MOD_OPTION | MAC_INPUT_MOD_COMMAND);
	mac_input_snapshot(&modifier_source, &modifier_frame);
	CHECK(modifier_frame.keys[56] && modifier_frame.keys[62] && modifier_frame.keys[58]);
	CHECK(modifier_frame.keys[55] && (modifier_frame.held & MAC_INPUT_CROUCH));
	mac_input_modifiers(&modifier_source, 0);
	mac_input_snapshot(&modifier_source, &modifier_frame);
	CHECK(!modifier_frame.keys[54] && !modifier_frame.keys[55]);
	CHECK(!modifier_frame.keys[56] && !modifier_frame.keys[62] && !modifier_frame.keys[58]);
	CHECK(modifier_frame.released & MAC_INPUT_CROUCH);
	CHECK((frame.held & (MAC_INPUT_FIRE|MAC_INPUT_GRENADE)) ==
		(MAC_INPUT_FIRE|MAC_INPUT_GRENADE));
	CHECK(frame.yaw < 0.f && frame.pitch > 0.f);
	CHECK(input_initialize());
	CHECK(halo_input_from_name("w") == _key_w);
	CHECK(halo_input_from_name("Mouse Left") >= 0);
	CHECK(halo_input_from_name("unknown key") == -1);
	halo_input_name(_key_w,input_name,sizeof(input_name));
	CHECK(!strcmp(input_name,"W"));
	mac_input_init(&pointer_source);mac_input_focus(&pointer_source,1);
	mac_input_pointer(&pointer_source,800.0f,450.0f,1,1);
	mac_input_wheel(&pointer_source,2.0f);
	mac_input_snapshot(&pointer_source,&pointer_frame);
	mac_engine_input_publish(&pointer_frame);input_update();
	CHECK(halo_ui_pointer_update(1,&pointer));
	CHECK(pointer.x==320&&pointer.y==240&&pointer.click_x==320&&pointer.click_y==240);
	CHECK(pointer.moved&&pointer.left_clicks==1&&pointer.wheel_steps==2);
	CHECK(!halo_ui_pointer_update(0,&pointer));
	probe_window_width=640;probe_window_height=480;
	mac_engine_input_publish(&frame);
	input_update();
	input_frame_begin();
	CHECK(input_key_is_down(_key_w));
	CHECK(input_key_is_down(_key_a));
	CHECK(input_key_is_down(_key_right_windows));
	CHECK(input_key_is_down(_key_left_windows));
	CHECK(input_key_is_down(_key_left_shift));
	CHECK(input_key_is_down(_key_right_control));
	CHECK(input_key_is_down(_key_left_alt));
	CHECK(input_key_is_down(_key_q) && input_key_is_down(_key_e));
	CHECK(!input_key_is_down(_key_s));
	platform_text_field(TRUE, FALSE);
	CHECK(halo_keyboard_actions(0) == 0);
	platform_text_field(FALSE, FALSE);
	actions=halo_keyboard_actions(0);
	CHECK((actions & (1UL<<HALO_KEYBOARD_MOVE_FORWARD)) != 0);
	CHECK((actions & (1UL<<HALO_KEYBOARD_STRAFE_LEFT)) != 0);
	CHECK((actions & (1UL<<HALO_KEYBOARD_FIRE)) != 0);
	CHECK((actions & (1UL<<HALO_KEYBOARD_THROW_GRENADE)) != 0);
	CHECK((actions & (1UL<<HALO_KEYBOARD_ACTION)) != 0); /* E */
	CHECK((actions & (1UL<<HALO_KEYBOARD_FLASHLIGHT)) != 0); /* Q */
	CHECK((actions & (1UL<<HALO_KEYBOARD_MELEE)) != 0); /* Mouse 4 */
	CHECK((actions & (1UL<<HALO_KEYBOARD_SWITCH_WEAPON)) != 0);
	CHECK(input_get_key(&stroke) && stroke.key_code == _key_q);
	CHECK(stroke.ascii_code == 'q');
	CHECK(input_get_key(&stroke) && stroke.key_code == _key_w);
	CHECK(input_get_key(&stroke) && stroke.key_code == _key_e);
	CHECK(input_get_key(&stroke) && stroke.key_code == _key_a);
	CHECK(input_get_key(&stroke) && stroke.key_code == _key_left_shift);
	CHECK(input_get_key(&stroke) && stroke.key_code == _key_left_windows);
	CHECK(input_get_key(&stroke) && stroke.key_code == _key_left_alt);
	CHECK(input_get_key(&stroke) && stroke.key_code == _key_right_windows);
	CHECK(input_get_key(&stroke) && stroke.key_code == _key_right_control);
	CHECK(!input_get_key(&stroke));
	probe_forward_binding="Up";++probe_config_version;
	CHECK((halo_keyboard_actions(0)&(1UL<<HALO_KEYBOARD_MOVE_FORWARD))==0);
	{
		struct mac_input_frame rebound=frame;
		memset(rebound.keys,0,sizeof(rebound.keys));rebound.keys[126]=1;
		mac_engine_input_publish(&rebound);input_update();
		CHECK((halo_keyboard_actions(0)&(1UL<<HALO_KEYBOARD_MOVE_FORWARD))!=0);
	}
	probe_forward_binding="W";++probe_config_version;
	mac_engine_input_publish(&frame);input_update();
	platform_binding_capture_begin();
	mac_input_key(&source,1,1);mac_input_snapshot(&source,&frame);mac_engine_input_publish(&frame);input_update();
	captured=-1;CHECK(platform_binding_capture_poll(&captured)==1&&captured==_key_s);
	halo_input_name(captured,input_name,sizeof(input_name));CHECK(!strcmp(input_name,"S"));
	mac_input_key(&source,1,0);mac_input_snapshot(&source,&frame);mac_engine_input_publish(&frame);input_update();
	frame=initial_frame;mac_engine_input_publish(&frame);input_update();
	(void)halo_keyboard_actions(0); /* consume the restored wheel edge */
	mouse=(struct mouse_state_probe const *)input_get_mouse_state();
	CHECK(mouse && mouse->x == 10 && mouse->y == -5 && mouse->wheel == 3);
	CHECK(mouse && mouse->buttons[0] && mouse->buttons[1] && mouse->buttons[3]);
	CHECK(input_mouse_button_is_down(0) && input_mouse_button_is_down(1));
	{
		float yaw, pitch;
		CHECK(halo_linux_mouse_look(0, &yaw, &pitch));
		CHECK(yaw == frame.yaw && pitch == frame.pitch);
		CHECK(!halo_linux_mouse_look(0, &yaw, &pitch));
	}
	CHECK(halo_keyboard_actions(0) == (actions & ~(1UL<<HALO_KEYBOARD_SWITCH_WEAPON)));
	CHECK(input_has_gamepad(0) && input_get_gamepad_state(0) != NULL);
	CHECK(!input_has_gamepad(1) && input_get_gamepad_state(1) == NULL);
	input_frame_end();
	mouse=(struct mouse_state_probe const *)input_get_mouse_state();
	CHECK(mouse && mouse->x == 0 && mouse->y == 0 && mouse->wheel == 0);
	mac_input_button(&source,1,0);mac_input_snapshot(&source,&frame);mac_engine_input_publish(&frame);input_update();
	platform_binding_capture_begin();
	mac_input_button(&source,1,1);mac_input_snapshot(&source,&frame);mac_engine_input_publish(&frame);input_update();
	captured=-1;CHECK(platform_binding_capture_poll(&captured)==1);
	halo_input_name(captured,input_name,sizeof(input_name));CHECK(!strcmp(input_name,"Mouse Left"));
	mac_input_button(&source,1,0);mac_input_snapshot(&source,&frame);mac_engine_input_publish(&frame);input_update();
	platform_binding_capture_begin();
	mac_input_key(&source,53,1);mac_input_snapshot(&source,&frame);mac_engine_input_publish(&frame);input_update();
	captured=-1;CHECK(platform_binding_capture_poll(&captured)==3);
	mac_input_key(&source,53,0);mac_input_snapshot(&source,&frame);mac_engine_input_publish(&frame);input_update();
	platform_binding_capture_begin();
	mac_input_key(&source,117,1);mac_input_snapshot(&source,&frame);mac_engine_input_publish(&frame);input_update();
	captured=-1;CHECK(platform_binding_capture_poll(&captured)==2);
	mac_input_key(&source,117,0);mac_input_snapshot(&source,&frame);mac_engine_input_publish(&frame);input_update();

	mac_input_focus(&source, 0);
	mac_input_snapshot(&source, &frame);
	mac_engine_input_publish(&frame);
	input_update();
	CHECK(!input_key_is_down(_key_w) && !input_key_is_down(_key_a));
	CHECK(halo_keyboard_actions(0) == 0);
	mouse=(struct mouse_state_probe const *)input_get_mouse_state();
	CHECK(mouse && !mouse->buttons[0] && !mouse->buttons[1] && !mouse->buttons[3]);
	input_suppress();
	CHECK(!input_key_is_down(_key_w) && input_get_mouse_state() == NULL);
	CHECK(input_get_gamepad_state(0) != NULL);
	input_get_raw_data_string(description, sizeof(description));
	CHECK(strstr(description, "virtual port 0") != NULL &&
		strstr(description, "no physical gamepad") != NULL);
	input_dispose();
	CHECK(input_get_mouse_state() == NULL);
	printf("mac engine input: %d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
