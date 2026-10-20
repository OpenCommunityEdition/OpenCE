/*
VM_POST.C

The Metal renderer's post passes (port/macos/guest/d3d8_metal_post.c), as
the guest build compiles them, run in the macOS host's VM
(tools/test_macos_post.py): each prints the library it writes, or a lookup
texture's size and a checksum of its texels.

	vm_post source fxaa|edges|weights|blend <width> <height>
	vm_post lookup area|search
*/

#include "xgpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	static const char *const names[NUMBER_OF_METAL_POST_PASSES] = { "fxaa", "edges", "weights", "blend" };
	int which;

	if (argc == 5 && strcmp(argv[1], "source") == 0)
	{
		char *source;

		for (which = 0; which < NUMBER_OF_METAL_POST_PASSES; which++)
		{
			if (strcmp(argv[2], names[which]) == 0)
			{
				break;
			}
		}
		if (which == NUMBER_OF_METAL_POST_PASSES)
		{
			return 2;
		}
		source = xgpu_metal_post_source(which, strtoul(argv[3], NULL, 10), strtoul(argv[4], NULL, 10));
		if (!source)
		{
			return 1;
		}
		fputs(source, stdout);
		free(source);
		return 0;
	}
	if (argc == 3 && strcmp(argv[1], "lookup") == 0)
	{
		unsigned long width, height, texel_size, sum = 0, index;
		unsigned char *texels = xgpu_metal_post_lookup(strcmp(argv[2], "search") == 0, &width, &height, &texel_size);

		if (!texels)
		{
			return 1;
		}
		for (index = 0; index < width * height * texel_size; index++)
		{
			sum = (sum * 31 + texels[index]) & 0xffffffffUL;
		}
		printf("%lu %lu %lu %08lx\n", width, height, texel_size, sum);
		free(texels);
		return 0;
	}
	return 2;
}
