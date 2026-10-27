#ifndef CONTROLLER_REMAP_H
#define CONTROLLER_REMAP_H

#define CONTROLLER_REMAP_ACTIONS 10
#define CONTROLLER_REMAP_BUTTONS 11
#define CONTROLLER_REMAP_TEXT 192

/* Game control indices: jump, grenade type, use/reload, weapon, melee,
 * flashlight, grenade, fire, crouch, zoom. Start, Back and stick axes stay stock. */
extern unsigned char const controller_remap_controls[CONTROLLER_REMAP_ACTIONS];
char const *controller_remap_setting(short port);
int controller_remap_parse(char const *text, unsigned char *buttons);
void controller_remap_format(unsigned char const *buttons, char *text, unsigned int size);
int controller_remap_read(short port, unsigned char *buttons);
void controller_remap_swap(unsigned char *buttons, int action, int button);
char const *controller_remap_button_name(int button, int family);
int controller_remap_cycle(int button, int direction);

/* Game-side access to the existing profile and menu context. No profile writes. */
void controller_remap_profile(short port, unsigned char *buttons);
int controller_remap_playing(short port);
struct game_input_preferences;
void controller_remap_prompt(short port, struct game_input_preferences *preferences);

/* Raw physical input only, before keyboard/touch merging. Returns true while
 * capture consumes this port. The caller also suppresses its merged menu input. */
struct _XINPUT_GAMEPAD;
int controller_remap_input(short port, struct _XINPUT_GAMEPAD *pad, int connected);
void controller_remap_merge(struct _XINPUT_GAMEPAD const *source, struct _XINPUT_GAMEPAD *target);
void controller_remap_capture_begin(short port);
int controller_remap_capture_poll(int *button);
void controller_remap_capture_end(void);

#endif
