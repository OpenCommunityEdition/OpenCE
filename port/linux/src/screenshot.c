/*
SCREENSHOT.C

Frames saved as BMP files (screenshot.h).
*/

#include "screenshot.h"
#include "port_config.h"

#include <stdio.h>
#include <string.h>

enum
{
	SCREENSHOT_NAME_LENGTH = 64,
	SCREENSHOT_PATH_LENGTH = 512
};

static struct
{
	/* a request not yet saved */
	int pending;
	long frames_left;
	char path[SCREENSHOT_PATH_LENGTH];
	/* a request saved (or not), whose result is not yet taken */
	int finished;
	int saved;
} screenshot;

int screenshot_write_bmp(const char *path, unsigned char *pixels, unsigned long width, unsigned long height)
{
	unsigned char header[54] = { 'B', 'M' };
	unsigned long image_size = width * height * 4;
	unsigned long pixel;
	FILE *file;
	int written;

	for (pixel = 0; pixel < width * height; pixel++)
		pixels[pixel * 4 + 3] = 0xff;
	file = fopen(path, "wb");
	if (!file)
		return 0;
	*(unsigned int *)(header + 2) = (unsigned int)(54 + image_size);
	*(unsigned int *)(header + 10) = 54;
	*(unsigned int *)(header + 14) = 40;
	*(int *)(header + 18) = (int)width;
	*(int *)(header + 22) = -(int)height; /* rows from the top */
	*(unsigned short *)(header + 26) = 1;
	*(unsigned short *)(header + 28) = 32;
	*(unsigned int *)(header + 34) = (unsigned int)image_size;
	written = fwrite(header, 1, sizeof(header), file) == sizeof(header) &&
		fwrite(pixels, 1, image_size, file) == image_size;
	return fclose(file) == 0 && written;
}

static int name_valid(const char *name)
{
	size_t length = strlen(name), index;

	/* (a name that starts with '.' could be "..") */
	if (length == 0 || length >= SCREENSHOT_NAME_LENGTH || name[0] == '.')
		return 0;
	for (index = 0; index < length; index++)
	{
		char character = name[index];

		if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
			(character >= '0' && character <= '9') || character == '-' || character == '_' || character == '.'))
			return 0;
	}
	return 1;
}

int screenshot_request(const char *name, long settle_frames, char *message, unsigned long size)
{
	const char *directory = config_string("debug.screenshot_directory");

	if (!name_valid(name))
	{
		snprintf(message, size, "port_screenshot: the name must be letters, digits, '-', '_' and '.'");
		return 0;
	}
	if (!*directory)
	{
		snprintf(message, size, "port_screenshot: debug.screenshot_directory (HALO_SCREENSHOT_DIR) is not set");
		return 0;
	}
	if (screenshot.pending)
	{
		snprintf(message, size, "port_screenshot: %s is still to be saved", screenshot.path);
		return 0;
	}
	snprintf(screenshot.path, sizeof(screenshot.path), "%s/%s.bmp", directory, name);
	screenshot.pending = 1;
	screenshot.frames_left = settle_frames < 0 ? 0 : settle_frames;
	/* (a result not yet taken is dropped: this request's comes next) */
	screenshot.finished = 0;
	snprintf(message, size, "port_screenshot: saving %s after %ld frames", screenshot.path, screenshot.frames_left);
	return 1;
}

const char *screenshot_due(void)
{
	if (!screenshot.pending)
		return NULL;
	if (screenshot.frames_left > 0)
	{
		screenshot.frames_left--;
		return NULL;
	}
	return screenshot.path;
}

void screenshot_saved(int saved)
{
	screenshot.pending = 0;
	screenshot.finished = 1;
	screenshot.saved = saved;
}

int screenshot_result(char *message, unsigned long size)
{
	if (!screenshot.finished)
		return 0;
	screenshot.finished = 0;
	snprintf(message, size, "%s %s", screenshot.saved ? "captured" : "capture failed", screenshot.path);
	return 1;
}
