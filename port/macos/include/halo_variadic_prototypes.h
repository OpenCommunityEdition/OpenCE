#ifndef __HALO_VARIADIC_PROTOTYPES_H
#define __HALO_VARIADIC_PROTOTYPES_H

union real_argb_color;

void error(short priority, const char *format, ...);
void console_printf(unsigned char clear, char const *format, ...);
void terminal_printf(union real_argb_color const *color, char const *format, ...);

#endif
