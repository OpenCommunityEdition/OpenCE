#ifndef KEYBOARD_PROMPTS_H
#define KEYBOARD_PROMPTS_H

/* Physical activity only; these observers never consume or alter input. */
void keyboard_prompt_pad(short port, unsigned long buttons, int analog, int connected);
void keyboard_prompt_keys(unsigned char const *keys, unsigned int count, int mouse);
int keyboard_prompt_enabled(short port);
int keyboard_prompt_menu_enabled(int device);

#endif
