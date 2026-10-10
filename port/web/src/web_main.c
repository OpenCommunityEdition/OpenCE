/*
WEB_MAIN.C

The web build's entry point, and the WebAssembly memory's layout.

main runs on a thread of its own (Emscripten's PROXY_TO_PTHREAD), where it
may block as the game does. It mounts the site's Origin Private File System,
where the page put the game's maps folder (port/web/site/xiso-worker.js) and
where the saved games and config.toml are kept, at /data; then it starts the
game (shell_xbox.c's main, renamed halo_game_main by tools/web_build.py).
*/

#include <emscripten.h>
#include <emscripten/wasmfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "web_shared.h"

int halo_game_main(void);
/* web_library.js: a line for the page (kind 0 a status, 3 a fatal error) */
void web_js_post(int kind, const char *text);

/* ---------- memory

The Xbox window (port/linux/src/platform.h) is the top 128 MB of the
WebAssembly memory, 0x80000000 to 0x88000000 (tools/web_build.py makes the
memory that size). sbrk grows the C heap up to the heap size this reports,
so reporting the window's start keeps the heap below it. */

#define WEB_HEAP_LIMIT 0x80000000UL

size_t emscripten_get_heap_size(void)
{
	size_t size = (size_t)__builtin_wasm_memory_size(0) << 16;

	return size > WEB_HEAP_LIMIT ? WEB_HEAP_LIMIT : size;
}

/* ---------- what the page reads and writes (web_shared.h) */

volatile int32_t web_shared[WEB_SHARED_WORDS];

EMSCRIPTEN_KEEPALIVE volatile int32_t *web_state(void)
{
	return web_shared;
}

void web_frame_shown(void)
{
	__atomic_add_fetch(&web_shared[_web_frames_shown], 1, __ATOMIC_RELAXED);
}

/* ---------- start */

/* a setting from the browser, unless the page's arguments gave it */
static void set_default(const char *name, const char *value)
{
	setenv(name, value, 0);
}

int main(int argc, char **argv)
{
	backend_t opfs;
	int index;

	/* --NAME=value arguments from the page (port/web/site/app.js) become
	environment variables, which come before config.toml
	(port/linux/src/port_config.c) */
	for (index = 1; index < argc; index++)
	{
		const char *equals = strchr(argv[index], '=');

		if (!strncmp(argv[index], "--", 2) && equals && equals - argv[index] - 2 < 128)
		{
			char name[128];
			size_t length = (size_t)(equals - argv[index] - 2);

			memcpy(name, argv[index] + 2, length);
			name[length] = '\0';
			setenv(name, equals + 1, 1);
		}
	}

	opfs = wasmfs_create_opfs_backend();
	if (!opfs || wasmfs_create_directory("/data", 0777, opfs) != 0)
	{
		web_js_post(3, "The browser's private storage (OPFS) cannot be opened.");
		return EXIT_FAILURE;
	}
	mkdir("/data/save", 0777);

	set_default("HALO_DATA_ROOT", "/data");
	set_default("HALO_SAVE_ROOT", "/data/save");
	/* no UDP and no TCP in a browser: system link and internet play wait
	for the browsers' own transport (port/web/README.md, "Limits") */
	set_default("HALO_NET_ONLINE", "false");
	/* the canvas fills the page, which the page's own button takes
	fullscreen (a page may only do so from a click) */
	set_default("HALO_DISPLAY_MODE", "windowed");
	/* (Custom Edition maps' window would be the C heap's here:
	port/linux/src/xbox_memory.c) */
	set_default("HALO_CUSTOM_EDITION", "false");
	web_js_post(0, "starting");
	return halo_game_main();
}
