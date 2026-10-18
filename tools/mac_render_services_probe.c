#define BUILDING_CSERIES 1
#define fast_ftol mac_render_services_probe_unused_fast_ftol
#include "engine_xdk_prefix.h"
#undef fast_ftol
#include "input/input.h"
#include "halo_ui_pointer.h"
#include "engine_device_state.h"
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#undef vprintf
#undef vfprintf

struct system_memory_information { long free; long total; };
struct mac_native_mouse_state { long x, y, wheel; byte buttons[4]; };
struct native_texture_cache_debug_options { boolean graph; boolean list; };
struct native_texture_cache_debug_options texture_cache_debug_options = { FALSE, FALSE };
long fast_ftol_C(float value);
void system_memory_information_get(struct system_memory_information *information);
void platform_scoreboard_scroll(int open, long *notches, long *pages);
int halo_scoreboard_pointer_update(int offered, struct halo_ui_pointer *pointer);
void halo_screen_anti_alias(short x0, short y0, short x1, short y1);
void texture_cache_debug_render(void);

static struct mac_native_mouse_state mouse_state;
static struct halo_ui_pointer pointer_state;
static struct mac_device_state device_state;
static boolean page_up_down, page_down_down;
static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"render service check failed at %d\n",__LINE__); } } while(0)

int halo_linux_printf(const char *format, ...)
{
	va_list arguments;
	int result;
	va_start(arguments, format); result = vprintf(format, arguments); va_end(arguments);
	return result;
}
int halo_linux_fprintf(FILE *stream, const char *format, ...)
{
	va_list arguments;
	int result;
	va_start(arguments, format); result = vfprintf(stream, format, arguments); va_end(arguments);
	return result;
}

const struct mouse_state *input_get_mouse_state(void)
{ return (const struct mouse_state *)&mouse_state; }
boolean input_key_is_down(short key)
{
	return key == _key_page_up ? page_up_down : key == _key_page_down ? page_down_down : FALSE;
}
int halo_ui_pointer_update(int active, struct halo_ui_pointer *pointer)
{
	if (!active || !pointer_state.moved) return 0;
	*pointer = pointer_state;
	return 1;
}
const struct mac_device_state *mac_device_state_get(void) { return &device_state; }
const char *config_string(const char *name) { (void)name; return "off"; }
void GlobalMemoryStatus(MEMORYSTATUS *status)
{
	if (!status) return;
	status->dwLength = sizeof(*status);
	status->dwTotalPhys = 67108864UL;
	status->dwAvailPhys = 40000000UL;
}

int main(void)
{
	struct system_memory_information memory;
	struct halo_ui_pointer pointer;
	long notches, pages;
	CHECK(fast_ftol_C(1.5f) == 2);
	CHECK(fast_ftol_C(2.5f) == 2);
	CHECK(fast_ftol_C(-1.5f) == -2);
	system_memory_information_get(&memory);
	CHECK(memory.total == 67108864L && memory.free == 40000000L);
	system_memory_information_get(NULL);
	memset(&mouse_state, 0, sizeof(mouse_state));
	mouse_state.wheel = -3;
	platform_scoreboard_scroll(1, &notches, &pages);
	CHECK(notches == -3 && pages == 0);
	page_up_down = TRUE;
	platform_scoreboard_scroll(1, &notches, &pages);
	CHECK(pages == -1);
	platform_scoreboard_scroll(1, &notches, &pages);
	CHECK(pages == 0); /* A held key pages once, not once per raster pass. */
	page_up_down = FALSE;
	platform_scoreboard_scroll(1, &notches, &pages);
	page_down_down = TRUE;
	platform_scoreboard_scroll(1, &notches, &pages);
	CHECK(pages == 1);
	page_down_down = FALSE;
	platform_scoreboard_scroll(0, &notches, &pages);
	CHECK(notches == 0 && pages == 0);
	memset(&pointer_state, 0, sizeof(pointer_state));
	pointer_state.x = 17; pointer_state.y = 23; pointer_state.moved = TRUE;
	CHECK(halo_scoreboard_pointer_update(1, &pointer) == 1);
	CHECK(pointer.x == 17 && pointer.y == 23 && pointer.moved);
	CHECK(halo_scoreboard_pointer_update(0, &pointer) == 0);
	CHECK(!pointer.moved);
	halo_screen_anti_alias(0, 0, 640, 480); /* CPU-only state has no AA pass. */
	texture_cache_debug_render(); /* disabled debug visualization is silent. */
	printf("native render services: %u checks, %u failures\n", checks, failures);
	return failures ? 1 : 0;
}
