/* Narrow platform services used by the original render and interface paths.
   Mouse services consume the native input backend's real event snapshot.
   GPU-only diagnostics fail visibly when their requested feature is absent. */
#define BUILDING_CSERIES 1
#define fast_ftol mac_render_services_unused_fast_ftol
#include "engine_xdk_prefix.h"
#undef fast_ftol
#include "input/input.h"
#include "halo_ui_pointer.h"
#include "engine_device_state.h"
#include "../linux/src/port_config.h" /* port: config_real, config_changes (display.gamma) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct system_memory_information
{
	long free;
	long total;
};

struct native_texture_cache_debug_options
{
	boolean graph;
	boolean list;
};

extern struct native_texture_cache_debug_options texture_cache_debug_options;
extern int halo_ui_pointer_update(int menus_active, struct halo_ui_pointer *pointer);
extern const char *config_string(const char *name);

struct mac_native_mouse_state
{
	long x, y, wheel;
	byte buttons[4];
};
typedef char mac_native_mouse_state_witness[
	sizeof(struct mac_native_mouse_state) == 0x10 ? 1 : -1];
static boolean previous_page_up;
static boolean previous_page_down;

/* Match cseries.h fast_ftol's current-control-word conversion without calling
   a host math library. */
long fast_ftol_C(float value)
{
#if defined(__i386__)
	long result;
	__asm__ __volatile__("flds %1\n\tfistpl %0" : "=m"(result) : "m"(value) : "st");
	return result;
#elif defined(__ppc__)
	union { double value; unsigned long words[2]; } result;
	double source = (double)value;
	__asm__ __volatile__("fctiw %0,%1\n\tstfd %0,%2"
		: "=&f"(result.value) : "f"(source), "m"(result.value) : "memory");
	/* PPC32 Darwin is big-endian: the converted word is the second word. */
	return (long)result.words[1];
#else
#error fast_ftol_C requires the supported i386 or PPC target
#endif
}

void system_memory_information_get(struct system_memory_information *information)
{
	MEMORYSTATUS status;
	if (!information)
		return;
	information->free = 0;
	information->total = 0;
	memset(&status, 0, sizeof(status));
	status.dwLength = sizeof(status);
	GlobalMemoryStatus(&status);
	information->free = (long)status.dwAvailPhys;
	information->total = (long)status.dwTotalPhys;
}

void platform_scoreboard_scroll(int open, long *notches, long *pages)
{
	const struct mac_native_mouse_state *mouse =
		(const struct mac_native_mouse_state *)input_get_mouse_state();
	long page_delta = 0;
	boolean page_up = open && input_key_is_down(_key_page_up);
	boolean page_down = open && input_key_is_down(_key_page_down);
	if (!open)
	{
		previous_page_up = previous_page_down = FALSE;
	}
	else
	{
		if (page_up && !previous_page_up)
			--page_delta;
		if (page_down && !previous_page_down)
			++page_delta;
		previous_page_up = page_up;
		previous_page_down = page_down;
	}
	if (notches)
		*notches = open && mouse ? mouse->wheel : 0;
	if (pages)
		*pages = page_delta;
}

int halo_scoreboard_pointer_update(int offered, struct halo_ui_pointer *pointer)
{
	if (!pointer)
		return 0;
	memset(pointer, 0, sizeof(*pointer));
	/* This uses the same scaled, window-owned Carbon pointer snapshot as menus. */
	return halo_ui_pointer_update(offered != 0, pointer);
}

/* port: FXAA lives with the GPU device (engine_device_state.c), which sets
   this when it is linked; the render graph's own program has no device */
int (*mac_device_post_fxaa_hook)(int x0, int y0, int x1, int y1);

/* port: display.gamma for the device's present (mac_post_gamma,
   engine_device_state.c), read again when a setting is written; the hook is
   a common symbol, as the FXAA one is */
double (*mac_device_gamma_hook)(void);

static double mac_render_gamma(void)
{
	static unsigned long read_at = (unsigned long)-1;
	static double gamma = 1.0;

	if (read_at != config_changes())
	{
		read_at = config_changes();
		gamma = config_real("display.gamma");
		if (!(gamma > 0.0))
			gamma = 1.0;
		else if (gamma < 0.5)
			gamma = 0.5;
		else if (gamma > 3.0)
			gamma = 3.0;
	}
	return gamma;
}

static void mac_render_gamma_install(void) __attribute__((constructor));
static void mac_render_gamma_install(void)
{
	mac_device_gamma_hook = mac_render_gamma;
}

/* port: mac.render_resolution for the device's frame (mac_render_scale_update,
   engine_device_state.c): the setting's words, and the card's video memory
   and CPU, which "auto" and the limits go by (mac_gpu_info.c, when linked) */
const char *(*mac_device_render_setting_hook)(int *vram_mb, int *is_ppc);
extern int mac_gpu_info_vram_mb(void) __attribute__((weak));
extern int mac_gpu_info_is_ppc(void) __attribute__((weak));

static const char *mac_render_setting(int *vram_mb, int *is_ppc)
{
	static unsigned long read_at = (unsigned long)-1;
	static const char *setting = "auto";

	if (read_at != config_changes())
	{
		read_at = config_changes();
		setting = config_string("mac.render_resolution");
	}
	*vram_mb = mac_gpu_info_vram_mb ? mac_gpu_info_vram_mb() : 32;
	*is_ppc = mac_gpu_info_is_ppc ? mac_gpu_info_is_ppc() : 0;
	return setting;
}

static void mac_render_setting_install(void) __attribute__((constructor));
static void mac_render_setting_install(void)
{
	mac_device_render_setting_hook = mac_render_setting;
}

void halo_screen_anti_alias(short x0, short y0, short x1, short y1)
{
	const struct mac_device_state *device = mac_device_state_get();
	const char *mode;
	/* Headless CPU sessions have no display pass to filter. */
	if (!device || !device->gpu)
		return;
	mode = config_string("display.anti_aliasing");
	if (!mode || strcmp(mode, "off") == 0)
		return;
	/* port: FXAA over the 3D view, before the HUD and menus are drawn over it
	   (mac_device_post_fxaa, engine_device_state.c) */
	if (strcmp(mode, "fxaa") == 0)
	{
		static int reported;

		if ((!mac_device_post_fxaa_hook || !mac_device_post_fxaa_hook(x0, y0, x1, y1)) && !reported)
		{
			reported = 1;
			fprintf(stderr, "native renderer: display.anti_aliasing=fxaa could not be applied\n");
		}
		return;
	}
	/* (the Mac's Video Setup offers off and fxaa; a config.toml of another
	   machine can name the others: the picture is not filtered, and the game
	   goes on) */
	{
		static int reported;

		if (!reported)
		{
			reported = 1;
			fprintf(stderr, "native renderer: display.anti_aliasing=%s is unsupported (off and fxaa are); not applied\n", mode);
		}
	}
}

void texture_cache_debug_render(void)
{
	if (!texture_cache_debug_options.graph && !texture_cache_debug_options.list)
		return;
	fprintf(stderr, "native texture cache: Xbox page-usage and bitmap-list visualization is unavailable\n");
	abort();
}
