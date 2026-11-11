#ifndef KEYBOARD_PROMPTS_H
#define KEYBOARD_PROMPTS_H

/* Physical activity only; these observers never consume or alter input. */
void keyboard_prompt_pad(short port, unsigned long buttons, int analog, int connected);
void keyboard_prompt_keys(unsigned char const *keys, unsigned int count, int mouse);
void keyboard_prompt_touch(void);
/* Physical selection, also used while a menu's automatic choice is pending. */
int keyboard_prompt_active(short port);
int keyboard_prompt_enabled(short port);

#endif
