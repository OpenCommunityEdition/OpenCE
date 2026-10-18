/* Native equivalents of the small screen-scale queries used by the Xbox
   rasterizer. The current Mac renderer keeps the game's render surface at
   its 640x480 logical size; visible-window scaling is a presentation concern. */
#include <stdint.h>

extern long config_integer(const char *name);

float halo_screen_scale(void)
{
	return 1.0f;
}

long halo_shadow_map_scale(void)
{
	long resolution = config_integer("display.shadow_resolution");
	long scale = 1;

	while (scale < 8 && 128 * scale * 2 <= resolution)
		scale *= 2;
	return scale;
}

/* scenario.c uses Microsoft's intrinsic as a compiler barrier while
   publishing BSP pointers. Keep the same compiler-ordering contract; the
   original intrinsic is not a hardware fence. */
void _ReadWriteBarrier(void)
{
	__asm__ __volatile__("" ::: "memory");
}
