#ifndef FRONTEND_PROMPTS_H
#define FRONTEND_PROMPTS_H
#include "math/integer_math.h"

enum frontend_prompt_action { FRONTEND_SELECT, FRONTEND_CANCEL, FRONTEND_STYLE };
int frontend_prompt_button(int action);
char const *frontend_prompt_key(int action);
void frontend_prompts_draw(long tag, rectangle2d const *bounds);
boolean frontend_glyph_draw(short port, short button, rectangle2d *cursor, pixel32 color);
#endif
