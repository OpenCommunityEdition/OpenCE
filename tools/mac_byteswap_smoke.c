/* Links the actual engine byte_swapping.c, without the platform layer.
 * Diagnostic stubs fail immediately; they are not game implementations. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
typedef unsigned char boolean;
#include "byte_swapping.h"

char temporary[256];
char *csprintf(char *buffer, char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);
    return buffer;
}
void display_assert(char *information, char *file, long line, boolean fatal)
{
    fprintf(stderr, "%s:%ld: %s (fatal=%d)\n", file, line,
        information ? information : "assertion", fatal);
    exit(2);
}
void system_exit(long status) { exit((int)status); }

int main(void)
{
    union { unsigned long long align; unsigned char bytes[16]; } data;
    unsigned char original[16];
    int width, i, base;
    if (sizeof(void *) != 4 || sizeof(long) != 4 || sizeof(wchar_t) != 2) {
        fprintf(stderr, "FAIL: engine requires 32-bit pointers/long and 16-bit wchar\n");
        return 1;
    }
    for (i = 0; i < 16; i++) original[i] = (unsigned char)(i * 13 + 3);
    for (width = 2; width <= 8; width *= 2) {
        memcpy(data.bytes, original, 16);
        byte_swap_memory(data.bytes, 16 / width, -width);
        for (base = 0; base < 16; base += width)
            for (i = 0; i < width; i++)
                if (data.bytes[base + i] != original[base + width - 1 - i]) {
                    fprintf(stderr, "FAIL: %d-byte swap at %d\n", width, base + i);
                    return 1;
                }
        byte_swap_memory(data.bytes, 16 / width, -width);
        if (memcmp(data.bytes, original, 16)) return 1;
        printf("engine_%d_byte_swap_and_roundtrip=pass\n", width);
    }
    return 0;
}
