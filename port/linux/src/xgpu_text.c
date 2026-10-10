/*
XGPU_TEXT.C

A growable string for the shader translators.
*/

#include "xgpu.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void xgpu_text_append(struct xgpu_text *text, const char *format, ...)
{
	va_list arguments;
	int needed;

	for (;;)
	{
		va_start(arguments, format);
		needed = vsnprintf(text->buffer ? text->buffer + text->length : NULL,
			text->buffer ? text->capacity - text->length : 0, format, arguments);
		va_end(arguments);
		if (text->buffer && text->length + (unsigned long)needed < text->capacity)
		{
			text->length += (unsigned long)needed;
			return;
		}
		{
			unsigned long capacity = (text->capacity + (unsigned long)needed + 1) * 2;
			char *buffer = realloc(text->buffer, capacity);

			/* (out of memory: the text stays as it is, short) */
			if (!buffer)
				return;
			text->buffer = buffer;
			text->capacity = capacity;
		}
	}
}
