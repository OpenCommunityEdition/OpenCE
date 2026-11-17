/*
DUALSENSE.H

A PS5 DualSense's (and DualSense Edge's) adaptive triggers, lightbar and player lights, driven from the game
(port/linux/game/dualsense_game.c says what each player wants: lightbar colour, player number, R2 and L2 effects).
Settings: input.adaptive_triggers, input.trigger_strength and input.lightbar. Other pads are never touched. Two ways
to the pad:
- SDL's own PS5 driver (the default; also on Windows and macOS): SDL_SendGamepadEffect with the 47-byte effects body,
  SDL_SetGamepadLED, SDL_SetGamepadPlayerIndex.
- Linux with the kernel's hid-playstation driver (input.playstation_kernel_driver): the input stays the kernel's, and
  the output reports are written to the pad's own /dev/hidraw node when this user may write it. Only the trigger,
  lightbar and player light enable bits are set, so the kernel's own reports (its rumble) and these leave each
  other's fields alone.
A pad gets at most one report a frame, only of the fields that changed, and on Bluetooth at most one every 50 ms. A
pad whose hidraw node goes (ENODEV, EIO: the Bluetooth link lost) has it closed, looked for again each second and,
back, everything sent again. A kick is R2's shape for a round, sent once, then the held one again after its time. The
triggers are sent again though unchanged, while both are up, once a second and after the game's rumble changes. The
triggers are let go (and the lightbar given back) at exit, when a pad leaves and when a setting is turned off.
debug.txt says how each DualSense is driven, or why not.
*/

#ifndef __DUALSENSE_H
#define __DUALSENSE_H

#include "dualsense_rules.h"

struct SDL_Gamepad;

/* no adaptive triggers, lightbar or player lights on Android and in a browser: SDL's gamepad calls are not there */
#if defined(HALO_ANDROID) || defined(HALO_ARM64_GUEST) || defined(HALO_WEB)
#define DUALSENSE_NONE
#endif

/* the settings (read again when one was written) */
struct dualsense_settings
{
	int triggers; /* input.adaptive_triggers */
	int percent;  /* input.trigger_strength (dualsense_strength_percent) */
	int lights;   /* input.lightbar not "off": the lightbar and the player lights */
};

/* what one controller port's player wants on its pad (the game side, each frame it has a player) */
struct dualsense_wish
{
	unsigned char red, green, blue;
	/* the player lights' bits (dualsense_player_lights; 0: none) */
	unsigned char player_lights;
	unsigned char right[DUALSENSE_EFFECT_SIZE];
	unsigned char left[DUALSENSE_EFFECT_SIZE];
	/* each trigger's last event kick (ms 0: none) and its events so far (a change = a kick) */
	unsigned char kick_right[DUALSENSE_EFFECT_SIZE];
	int kick_right_ms;
	unsigned long kicks_right;
};

#ifndef DUALSENSE_NONE
struct dualsense_settings dualsense_settings_get(void);
/* whether either feature is on */
int dualsense_on(void);
/* whether controller port `port`'s pad is a DualSense this machine drives (the game works out a wish only then) */
int dualsense_port_active(int port);
/* the game: controller port `port`'s player wants this (copied) */
void dualsense_wish_set(int port, struct dualsense_wish const *wish);
/* xinput_sdl.c XInputSetState: the SDL pad that drives controller port `port` (NULL: none), and whether the game's
rumble is on (the triggers are sent again after it changes) */
void dualsense_port(int port, struct SDL_Gamepad *gamepad, unsigned short left_motor, unsigned short right_motor);
/* sdl_platform.c: once a frame, after the events (sends what changed) */
void dualsense_frame(void);
/* sdl_platform.c: a pad is about to be closed (NULL: nothing) */
void dualsense_removed(struct SDL_Gamepad *gamepad);
#else
#define dualsense_settings_get() (dualsense_no_settings)
#define dualsense_on() 0
#define dualsense_port_active(port) 0
#define dualsense_wish_set(port, wish) ((void)0)
#define dualsense_port(port, gamepad, left_motor, right_motor) ((void)0)
#define dualsense_frame() ((void)0)
#define dualsense_removed(gamepad) ((void)0)
static struct dualsense_settings const dualsense_no_settings = { 0, 100, 0 };
#endif

#endif
