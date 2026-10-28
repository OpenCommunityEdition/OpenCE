#include "host.h"

#include <CoreFoundation/CoreFoundation.h>
#include <SDL3/SDL.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

extern char **environ;

char host_data_folder[1024];

void host_logf(int priority, const char *format, ...)
{
	va_list arguments;

	(void)priority;
	va_start(arguments, format);
	fputs("halo: ", stderr);
	vfprintf(stderr, format, arguments);
	fputc('\n', stderr);
	va_end(arguments);
}

void host_fatal(const char *format, ...)
{
	char message[1024];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	host_logf(HOST_LOG_ERROR, "%s", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", message, NULL);
	_exit(1);
}

void host_abort(const char *reason)
{
	host_logf(HOST_LOG_ERROR, "guest abort: %s", reason);
	abort();
}

void host_exit(int code)
{
	exit(code);
}

int host_errno(void)
{
	return host_linux_errno(errno);
}

static uint32_t make_boot(int argc, char **argv)
{
	size_t size = 0x40000;
	char *memory = host_low_map(size, PROT_READ | PROT_WRITE);
	struct halo_guest_boot *boot = (struct halo_guest_boot *)memory;
	uint32_t *pointers = (uint32_t *)(memory + sizeof(*boot)), *list;
	char *strings = memory + size / 4;
	int count, index;

	if (!memory)
		host_fatal("cannot allocate the guest's environment");
	for (count = 0; environ[count]; count++)
		;
	list = pointers;
	for (index = 0; index < argc + count; index++)
	{
		const char *text = index < argc ? argv[index] : environ[index - argc];
		size_t length = strlen(text) + 1;

		if (strings + length > memory + size || (char *)(list + 2) > memory + size / 4)
			break;
		memcpy(strings, text, length);
		*list++ = (uint32_t)(uintptr_t)strings;
		strings += length;
		if (index == argc - 1)
			*list++ = 0;
	}
	*list = 0;
	boot->argc = (uint32_t)argc;
	boot->argv = (uint32_t)(uintptr_t)pointers;
	boot->environment = (uint32_t)(uintptr_t)(pointers + argc + 1);
	boot->page_size = (uint32_t)sysconf(_SC_PAGESIZE);
	return (uint32_t)(uintptr_t)boot;
}

#define MAIN_STACK_SIZE (16 * 1024 * 1024)

int main(int argc, char *argv[])
{
	const char *resources = SDL_GetBasePath();
	char *folder = SDL_GetPrefPath("", "OpenCE");
	char path[1200];
	size_t image_size = 0;
	void *image;
	char *stack;
	uint32_t boot;

	snprintf(host_data_folder, sizeof(host_data_folder), "%.*s", folder ? (int)strlen(folder) - 1 : 0,
		folder ? folder : "");
	SDL_free(folder);
	setenv("XDG_DATA_HOME", host_data_folder, 0);

#ifdef __aarch64__
	snprintf(path, sizeof(path), "%shalo_guest_arm64.elf", resources);
#else
	snprintf(path, sizeof(path), "%shalo_guest_x86_64.elf", resources);
#endif
	image = SDL_LoadFile(path, &image_size);
	if (!image || host_load_image(image, image_size) != 0)
		host_fatal("cannot load the game image %s", path);
	SDL_free(image);
	host_install_signal_handlers();
	signal(SIGPIPE, SIG_IGN);

	CFPreferencesSetAppValue(CFSTR("ApplePersistenceIgnoreState"), kCFBooleanTrue, kCFPreferencesCurrentApplication);

	snprintf(path, sizeof(path), "%s../Frameworks/libGLESv2.dylib", resources);
	SDL_SetHint(SDL_HINT_OPENGL_LIBRARY, path);
	snprintf(path, sizeof(path), "%s../Frameworks/libEGL.dylib", resources);
	SDL_SetHint(SDL_HINT_EGL_LIBRARY, path);

	boot = make_boot(argc, argv);
	stack = host_low_map(MAIN_STACK_SIZE, PROT_READ | PROT_WRITE);
	if (!stack)
		host_fatal("cannot allocate the game's stack");
#ifdef __aarch64__
	__asm__ volatile("mov sp, %0\n\tmov w0, %w2\n\tblr %1\n\tbrk #0"
		:: "r"(stack + MAIN_STACK_SIZE), "r"(host_run_guest_main), "r"(boot) : "x0", "memory");
#else
	__asm__ volatile("mov %0, %%rsp\n\tcall *%1\n\tud2"
		:: "r"(stack + MAIN_STACK_SIZE), "r"(host_run_guest_main), "D"(boot) : "memory");
#endif
	return 0;
}
