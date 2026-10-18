/* The few external functions the math units reference, for
   tools/mac_sim_math_probe.c (none of them is called by the probe's
   workload, except the assertion path, which aborts); the Direct3D names
   the weak inline definitions mention are left to the linker
   (-undefined dynamic_lookup). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned long system_milliseconds(void) { return 0; }
void system_exit(int code) { fprintf(stderr, "system_exit(%d)\n", code); abort(); }
void error(short category, char const *format, ...) { fprintf(stderr, "error: %s\n", format); abort(); }
void *csmemset(void *p, int c, unsigned long n) { return memset(p, c, n); }
int game_engine_running(void) { return 0; }
char *csprintf(char *buffer, char const *format, ...) { return buffer; }
unsigned long D3D__RenderState[4096];
char temporary[4096];
unsigned long system_seconds(void) { return 0; }
void *debug_malloc(unsigned long size, int flag, const char *file, int line) { return malloc(size); }
void debug_free(void *p, const char *file, int line) { free(p); }
void display_assert(const char *a, const char *file, long line, int fatal) { fprintf(stderr, "assert %s %s:%ld\n", a, file, line); abort(); }
