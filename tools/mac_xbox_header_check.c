/* Regression check: the cache file header's words are in host order after mac_xbox_cache_header_to_native
   (the engine's cache_file_header_verify compares them as host values). Reads the first 0x800 bytes of each
   Xbox map given (the container's header), converts them, and prints each field and whether the engine's
   verify conditions hold; the output must be identical on little- and big-endian hosts. With no map it
   checks a built header. Exit 1 on a failure. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../port/mac/xbox_swap.h"

/* the engine's struct cache_file_header (source/cache/cache_files.c) */
struct cache_file_header {
    unsigned long header_signature; long version; long file_length; unsigned char reservedC[4];
    long tag_data_offset; long tag_data_size; unsigned char reserved18[8]; char name[0x20]; char build[0x20];
    unsigned char reserved60[4]; unsigned long checksum; unsigned char reserved68[0x794]; unsigned long footer_signature;
};
typedef char size_ok[sizeof(struct cache_file_header) == 0x800 ? 1 : -1];
typedef char sig_ok[sizeof(unsigned long) == 4 ? 1 : -1];

static void put(unsigned char *b, unsigned at, unsigned long v) { b[at] = (unsigned char)v; b[at + 1] = (unsigned char)(v >> 8); b[at + 2] = (unsigned char)(v >> 16); b[at + 3] = (unsigned char)(v >> 24); }

static int show(const char *label, const unsigned char *raw)
{
    struct cache_file_header h;
    int ok;
    memcpy(&h, raw, 0x800);
    mac_xbox_cache_header_to_native(&h);
    ok = h.header_signature == 'head' && h.footer_signature == 'foot' && h.version == 5 && h.file_length >= 0x800 && h.file_length <= 0x11600000 &&
        memchr(h.name, 0, sizeof(h.name)) && h.tag_data_offset >= 0 && h.tag_data_size >= 0 && h.tag_data_size <= 0x1600000 &&
        h.tag_data_offset <= h.file_length - h.tag_data_size;
    printf("%s: version %ld length %ld tag data %ld+%ld checksum %08lx name %.32s build %.32s verify %s\n", label, h.version, h.file_length,
        h.tag_data_offset, h.tag_data_size, h.checksum, h.name, h.build, ok ? "ok" : "FAIL");
    /* converting again changes nothing */
    { struct cache_file_header again = h; mac_xbox_cache_header_to_native(&again); if (memcmp(&again, &h, sizeof(h))) { printf("%s: second conversion changed the header\n", label); return 0; } }
    return ok;
}

int main(int argc, char **argv)
{
    int failed = 0, i;
    unsigned char built[0x800];
    memset(built, 0, sizeof(built));
    put(built, 0, 0x68656164); put(built, 4, 5); put(built, 8, 0x1234567); put(built, 0x10, 0x800); put(built, 0x14, 0x123456);
    strcpy((char *)built + 0x20, "built"); strcpy((char *)built + 0x40, "01.10.12.2276"); put(built, 0x64, 0xdeadbeef); put(built, 0x7fc, 0x666f6f74);
    if (!show("built", built)) ++failed;
    for (i = 1; i < argc; ++i) {
        FILE *f = fopen(argv[i], "rb");
        unsigned char raw[0x800];
        if (!f || fread(raw, 1, sizeof(raw), f) != sizeof(raw)) { printf("%s: cannot read\n", argv[i]); ++failed; if (f) fclose(f); continue; }
        fclose(f);
        { const char *leaf = strrchr(argv[i], '/'); if (!show(leaf ? leaf + 1 : argv[i], raw)) ++failed; }
    }
    printf(failed ? "FAILED\n" : "header conversion ok\n");
    return failed != 0;
}
