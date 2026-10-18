/*
INPUT_PROMPTS.H

Which device the player is using, so the prompts the game draws (the HUD's
"Press X to pick up", "Hold Back for score", the menus' button hints) name
its controls: the keys and mouse buttons of config.toml's [controls] while
the keyboard and mouse were used last, the controller's buttons (the icons
of the map) while a controller was.

input.prompts: "auto" (the default) follows the device used last, "keyboard"
and "gamepad" keep one. The platforms' input code (port/linux/src/xinput_sdl.c,
port/mac/engine_input.c) tells the tracker what it sees and answers
platform_prompts_keyboard; the game (source/interface/ui_widget.c) asks.
*/

#ifndef INPUT_PROMPTS_H
#define INPUT_PROMPTS_H

/* whether the prompts name the keyboard and mouse's controls, not the
controller's buttons (input.prompts) */
int platform_prompts_keyboard(void);

/* whether the controller in use is a PlayStation pad, so that the gameplay
prompts draw its symbols (input.prompt_glyphs "auto"): the SDL gamepad's type
(port/linux/src/xinput_sdl.c), the Mac's HID vendor (port/mac/engine_input.c) */
int platform_prompts_playstation(void);

/* the last device used. Mouse motion counts once it adds up to several
pixels within a few frames, so that a mouse nudged by the table does not
take the prompts from a controller in hand. */
struct input_prompt_tracker
{
	int keyboard;
	float motion;
};

#define INPUT_PROMPT_MOTION_DECAY 0.75f
#define INPUT_PROMPT_MOTION_THRESHOLD 12.0f

static __inline void input_prompt_tracker_start(struct input_prompt_tracker *tracker, int keyboard)
{
	tracker->keyboard = keyboard;
	tracker->motion = 0.0f;
}

/* a key or mouse button went down, or the wheel turned */
static __inline void input_prompt_tracker_keyboard(struct input_prompt_tracker *tracker)
{
	tracker->keyboard = 1;
}

/* a controller button went down, or a stick or trigger passed its dead zone */
static __inline void input_prompt_tracker_gamepad(struct input_prompt_tracker *tracker)
{
	tracker->keyboard = 0;
	tracker->motion = 0.0f;
}

/* the mouse's motion in pixels this frame (call once a frame, 0 if still) */
static __inline void input_prompt_tracker_motion(struct input_prompt_tracker *tracker, float pixels)
{
	if (pixels < 0.0f)
		pixels = -pixels;
	tracker->motion = tracker->motion * INPUT_PROMPT_MOTION_DECAY + pixels;
	if (tracker->motion >= INPUT_PROMPT_MOTION_THRESHOLD)
		tracker->keyboard = 1;
}

/* input.prompts: 0 auto, 1 keyboard, 2 gamepad */
static __inline int input_prompts_mode(char const *setting)
{
	if (setting && (setting[0] == 'k' || setting[0] == 'K'))
		return 1;
	if (setting && (setting[0] == 'g' || setting[0] == 'G'))
		return 2;
	return 0;
}

/* input.prompt_glyphs: 0 auto (the pad's own), 1 xbox, 2 playstation */
static __inline int input_prompt_glyphs_mode(char const *setting)
{
	if (setting && (setting[0] == 'x' || setting[0] == 'X'))
		return 1;
	if (setting && (setting[0] == 'p' || setting[0] == 'P'))
		return 2;
	return 0;
}

/* whether the pad in use shows PlayStation pictures and prompts: input.prompt_glyphs
"playstation", or "auto" with a PlayStation pad. The menus' variant pictures
(menu_art_texture, mac_menu_art_texture) and Gamepad Setup's preview ask this. */
static __inline int input_prompt_glyphs_playstation(char const *setting)
{
	int mode = input_prompt_glyphs_mode(setting);

	return mode == 2 || (mode == 0 && platform_prompts_playstation());
}

#endif
