#ifndef __HALO_MACOS_HOST_H
#define __HALO_MACOS_HOST_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "halo_guest_abi.h"

void host_logf(int priority, const char *format, ...) __attribute__((format(printf, 2, 3)));
#define HOST_LOG_INFO 4
#define HOST_LOG_WARN 5
#define HOST_LOG_ERROR 6
void host_exit(int code) __attribute__((noreturn));
int host_errno(void);
void host_fatal(const char *format, ...) __attribute__((format(printf, 1, 2), noreturn));
int host_linux_errno(int error);
extern char host_data_folder[];

int host_memory_initialize(uint32_t image_base, uint32_t image_size);
void *host_low_map(size_t size, int protection);
void host_low_unmap(void *address, size_t size);
long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset);
long host_guest_munmap(uint64_t address, uint64_t size);
long host_guest_mprotect(uint64_t address, uint64_t size, int protection);
long host_guest_madvise(uint64_t address, uint64_t size, int advice);
void host_install_signal_handlers(void);

struct host_guest_image
{
	const struct halo_guest_header *header;
	uint32_t base, end;
};

extern struct host_guest_image host_image;

int host_load_image(const void *elf, size_t size);

uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size);
void host_run_guest_main(uint32_t boot) __attribute__((noreturn));

void *host_resolve_import(const char *name);

void *host_gl_resolve(const char *name);

#endif
