/* Live camera adjustments use the same bounded settings as the menu. */
#include "cseries.h"
#include "third_person.h"
#include "halo_keyboard.h"
#include "controller_remap.h"

double config_real(char const *name);
int config_boolean(char const *name);
int config_write(char const *name, char const *value);

void third_person_input_update(void)
{
	static unsigned long previous;
	unsigned long held = halo_keyboard_actions(0);
	unsigned long pressed = held & ~previous;
	unsigned int pad_pressed = 0;
	short port;
	previous = held;
	/* The remapper owns controller context, capture and release guards. */
	for (port = 0; port < 4; port++)
		pad_pressed |= controller_remap_pressed(port, (1u << PAD_TOGGLE_PERSPECTIVE) | (1u << PAD_SWAP_SHOULDER));
	if (pad_pressed & (1u << PAD_TOGGLE_PERSPECTIVE)) pressed |= 1UL << HALO_KEYBOARD_TOGGLE_PERSPECTIVE;
	if (pad_pressed & (1u << PAD_SWAP_SHOULDER)) pressed |= 1UL << HALO_KEYBOARD_SWAP_SHOULDER;
	/* Keyboard and controller share one camera preference and host policy. */
	if ((pressed & (1UL << HALO_KEYBOARD_TOGGLE_PERSPECTIVE)) &&
		third_person_policy() == _third_person_player_choice)
		config_write("display.third_person", config_boolean("display.third_person") ? "false" : "true");
	if ((pressed & (1UL << HALO_KEYBOARD_SWAP_SHOULDER)) && third_person_enabled())
		config_write("display.third_person_left_shoulder", config_boolean("display.third_person_left_shoulder") ? "false" : "true");
}

int third_person_camera_scroll(float notches, unsigned long actions)
{
	char const *setting;
	double position, minimum, maximum, fallback, step;
	int lateral = FALSE;
	int left = FALSE;
	char value[32];

	/* One axis per scroll; preserve distance when modifiers overlap. */
	if (actions & (1UL << HALO_KEYBOARD_CAMERA_DISTANCE))
	{
		setting = "display.third_person_distance";
		minimum = 0.3;
		maximum = 2.0;
		fallback = 0.85;
		step = -0.08;
	}
	else if (actions & (1UL << HALO_KEYBOARD_CAMERA_HEIGHT))
	{
		setting = "display.third_person_height";
		minimum = -0.2;
		maximum = 0.6;
		fallback = 0.12;
		step = 0.04;
	}
	else if (actions & (1UL << HALO_KEYBOARD_CAMERA_OFFSET))
	{
		setting = "display.third_person_offset";
		minimum = 0.0;
		maximum = 0.6;
		fallback = 0.24;
		step = 0.04;
		lateral = TRUE;
	}
	else
		return 0;

	if (!third_person_active(0) || !third_person_available(0) ||
		!(notches >= -100.0f && notches <= 100.0f) || notches == 0.0f)
		return 0;
	position = config_real(setting);
	if (!(position >= minimum && position <= maximum))
		position = fallback;
	if (lateral)
	{
		/* The menu stores magnitude and shoulder separately; scrolling can
		cross the centre without changing that contract or the B binding. */
		left = config_boolean("display.third_person_left_shoulder");
		if (left)
			position = -position;
		minimum = -maximum;
	}
	position = PIN(position + (double)notches * step, minimum, maximum);
	if (lateral)
	{
		/* Keep the shoulder when the saved four-decimal offset rounds to zero. */
		if (position > -0.00005 && position < 0.00005)
			position = 0.0;
		if (position < 0.0 && !left)
			config_write("display.third_person_left_shoulder", "true");
		else if (position > 0.0 && left)
			config_write("display.third_person_left_shoulder", "false");
		if (position < 0.0)
			position = -position;
	}
	snprintf(value, sizeof(value), "%.4f", position);
	config_write(setting, value);
	return 1;
}
