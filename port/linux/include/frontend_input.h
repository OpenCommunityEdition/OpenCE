#ifndef FRONTEND_INPUT_H
#define FRONTEND_INPUT_H

/* Presentation state only. Never remaps controls or writes glyph preferences.
 * -1 is keyboard/mouse, -2 touch, 0..3 the physical controller port. */
void frontend_input_enable(int enabled);
int frontend_input_device(void);
void frontend_input_pad(short port, unsigned long buttons, int connected);
void frontend_input_keys(unsigned char const *keys, unsigned int count);
void frontend_input_pointer(int touch, int activity);

#endif
