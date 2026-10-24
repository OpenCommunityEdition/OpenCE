/* Real mirror decisions with a delayed, renaming GPU. No API failures. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int BOOL;
typedef unsigned GLuint, GLenum;
#define TRUE 1
#define FALSE 0
#ifndef TEST_DESKTOP
#define HALO_GLES 1
#endif
#define STREAM_BUFFER_RING 3
#define GL_COPY_WRITE_BUFFER 1
#define GL_DYNAMIC_DRAW 2
#define SEGMENT_SIZE 0x400000u
#define PLATFORM_CONTIGUOUS_SIZE (2 * SEGMENT_SIZE)
static unsigned char cpu[PLATFORM_CONTIGUOUS_SIZE];
#define PLATFORM_CONTIGUOUS_BASE ((unsigned long)cpu)
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static struct { unsigned long frame, buffer_ring; } device;
static struct { unsigned long mirrored_bytes; } stats;
static unsigned long generations[PLATFORM_CONTIGUOUS_SIZE / 4096], serial;
static unsigned mapped, subdata, overlaps, draws, mismatches, uploads;
static unsigned watch_reads, protections;

/* A subdata update may rename storage immediately and queue preservation
of the old store plus its changed bytes. A later unsynchronized write to
the new store can then be overwritten, even at a disjoint offset. */
static unsigned char *stores[2];
static GLuint bound, next_buffer;
struct command
{
    struct command *next;
    unsigned char *source, *destination, *expected;
    unsigned long offset, size, sequence;
};
static struct command *head, *tail;
static unsigned long submitted, fences[STREAM_BUFFER_RING];

static void enqueue(unsigned char *source, unsigned char *destination,
    unsigned char *expected, unsigned long offset, unsigned long size)
{
    struct command *command = calloc(1, sizeof(*command));
    CHECK(command);
    command->source = source;
    command->destination = destination;
    command->expected = expected;
    command->offset = offset;
    command->size = size;
    command->sequence = ++submitted;
    if (tail) tail->next = command;
    else head = command;
    tail = command;
}

static void complete(unsigned long sequence)
{
    while (head && head->sequence <= sequence)
    {
        struct command *command = head;
        if (command->expected)
        {
            draws++;
            if (memcmp(command->source + command->offset, command->expected, command->size)) mismatches++;
            free(command->expected);
        }
        else
        {
            memcpy(command->destination + command->offset, command->source, command->size);
            free(command->source);
        }
        head = command->next;
        free(command);
    }
    if (!head) tail = NULL;
}

static void host_gl_fence_frame(unsigned slot) { fences[slot] = submitted; }
static void host_gl_wait_frame(unsigned slot) { complete(fences[slot]); }

static void next_frame(void)
{
    /* The actual fence/rotation/wait statements from Present. */
#include "rotation.inc"
    device.frame++;
}

static unsigned long memory_watch_generation(unsigned long address, unsigned long size)
{
    unsigned long page = (address - PLATFORM_CONTIGUOUS_BASE) / 4096;
    unsigned long last = (address - PLATFORM_CONTIGUOUS_BASE + size - 1) / 4096, newest = 0;
    watch_reads++;
    for (; page <= last; page++) if (generations[page] > newest) newest = generations[page];
    return newest;
}
static void memory_watch_protect(unsigned long address, unsigned long size) { (void)address; (void)size; protections++; }
static void write_pages(unsigned first, unsigned count, unsigned char value)
{
    unsigned page;
    for (page = first; page < first + count; page++) generations[page] = ++serial;
    memset(cpu + first * 4096, value, count * 4096);
}
static void glGenBuffers(int count, GLuint *buffer) { CHECK(count == 1 && next_buffer < 2); *buffer = ++next_buffer; }
static void glBindBuffer(GLenum target, GLuint buffer) { CHECK(target == GL_COPY_WRITE_BUFFER && buffer && buffer <= 2); bound = buffer - 1; }
static void glBufferData(GLenum target, unsigned long size, const void *data, GLenum usage)
{
    CHECK(target == GL_COPY_WRITE_BUFFER && size == SEGMENT_SIZE && !data && usage == GL_DYNAMIC_DRAW);
    stores[bound] = calloc(1, size);
    CHECK(stores[bound]);
}
static void buffer_upload(GLenum target, unsigned long offset, unsigned long size, const void *data)
{
    unsigned char *old = stores[bound], *copy = malloc(size);
    CHECK(target == GL_COPY_WRITE_BUFFER && offset + size <= SEGMENT_SIZE && copy);
    stores[bound] = calloc(1, SEGMENT_SIZE);
    CHECK(stores[bound]);
    memcpy(copy, data, size);
    enqueue(old, stores[bound], NULL, 0, SEGMENT_SIZE);
    enqueue(copy, stores[bound], NULL, offset, size);
    subdata++;
    uploads++;
}
static void host_gl_buffer_write(GLenum target, unsigned offset, unsigned size, const void *data)
{
    struct command *command;
    CHECK(target == GL_COPY_WRITE_BUFFER && offset + size <= SEGMENT_SIZE);
    for (command = head; command; command = command->next)
    {
        if (command->destination == stores[bound] && offset < command->offset + command->size &&
            command->offset < offset + size) overlaps++;
        if (command->expected && command->source == stores[bound] && offset < command->offset + command->size &&
            command->offset < offset + size) overlaps++;
    }
    memcpy(stores[bound] + offset, data, size);
    mapped++;
    uploads++;
}

#ifdef HALO_ANDROID
#ifdef TEST_ANDROID_NORMAL
static int android_geometry_fixes = 0;
#else
static int android_geometry_fixes = 1;
#endif
#endif
#include "mirror.inc"

static int draw_pages(unsigned first, unsigned count)
{
    GLuint buffer;
    unsigned long offset;
    unsigned char *expected;
    if (!mirror_range(PLATFORM_CONTIGUOUS_BASE + first * 4096, count * 4096, &buffer, &offset, NULL)) return 0;
    expected = malloc(count * 4096);
    CHECK(expected);
    memcpy(expected, cpu + first * 4096, count * 4096);
    enqueue(stores[buffer - 1], NULL, expected, offset, count * 4096);
    return 1;
}

int main(int argc, char **argv)
{
    unsigned index, old_uploads;
    int rollover = argc > 1 && !strcmp(argv[1], "rollover");
    device.frame = rollover ? ~0UL - 1 : 100;
#if defined(HALO_ANDROID) && !defined(TEST_ANDROID_NORMAL)
    /* First-use and reused ranges must always go to the caller's existing
    streaming path, without creating GPU storage or consulting tracking. */
    for (index = 0; index < 1200; index++)
    {
        GLuint buffer = 99;
        unsigned long offset = 77, generation = 55;
        write_pages(0, 300, (unsigned char)index);
        CHECK(!mirror_range(PLATFORM_CONTIGUOUS_BASE, 300 * 4096, &buffer, &offset, &generation));
        CHECK(buffer == 99 && offset == 77 && generation == 55);
        CHECK(!draw_pages(0, 1));
        CHECK(!draw_pages(1023, 2));
        CHECK(!draw_pages(1024, 1));
        next_frame();
    }
    CHECK(!next_buffer && !uploads && !watch_reads && !protections && !stats.mirrored_bytes);
    fprintf(stderr, "bypassed=4800 allocations=0 uploads=0 watch_reads=0 protections=0\n");
    return 0;
#endif
    write_pages(0, 1, 1);
    CHECK(draw_pages(0, 1));
    CHECK(watch_reads && protections);
    next_frame();
    /* An ordinary reused allocation: some previously mirrored pages and
    some first-time pages. 300 pages also crosses the 256-page refresh chunk. */
    write_pages(0, 300, 2);
    CHECK(draw_pages(0, 300));
    CHECK(draw_pages(0, 1));
    next_frame();
    write_pages(300, 1, 3);
    CHECK(draw_pages(300, 1));
    next_frame();
    write_pages(301, 1, 4);
    CHECK(draw_pages(301, 1));
    /* A different segment has no pending preservation copy. */
    write_pages(1024, 1, 5);
    CHECK(draw_pages(1024, 1));
    CHECK(!draw_pages(1023, 2)); /* segment boundary streams instead */
    for (index = 0; index < STREAM_BUFFER_RING; index++) next_frame();
    index = mapped;
    write_pages(302, 1, 6);
    CHECK(draw_pages(302, 1));
#if defined(TEST_DESKTOP)
    CHECK(mapped == 0); /* every persistent-cache upload is ordered */
#else
    CHECK(mapped == index + 1); /* fast path returns after the fence */
#endif
    complete(submitted);
    old_uploads = uploads;
    /* CPU generations are unchanged: any GPU damage is retained. */
    for (index = 0; index < 1200; index++)
    {
        CHECK(draw_pages(256, 1));
        next_frame();
    }
    complete(submitted);
    CHECK(uploads == old_uploads);
    fprintf(stderr, "mapped=%u subdata=%u overlaps=%u bad_draws=%u draws=%u repair_uploads=%u\n",
        mapped, subdata, overlaps, mismatches, draws, uploads - old_uploads);
    for (index = 0; index < 2; index++) free(stores[index]);
    CHECK(overlaps == 0 && mismatches == 0);
    return 0;
}
