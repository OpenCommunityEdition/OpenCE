#ifndef LIGHTING_MENU_H
#define LIGHTING_MENU_H

/* Group controls stage normally in the menu; CUSTOM preserves saved members.
   Return -1 for an unrelated setting, otherwise the operation's success. */
int lighting_menu_text(const char *name, char *text, unsigned int size, int defaults);
int lighting_menu_write(const char *name, const char *value);

#endif
