#define MAC_LOAD_TIMING_OWNER
#include "xbox_map.h"
#include "mac_load_timing.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <zlib.h>
#include <sys/time.h>

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

static uint32_t u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static int fits(uint32_t offset, uint32_t bytes, uint32_t limit)
{ return offset <= limit && bytes <= limit - offset; }

int mac_xbox_map_path_version(const char *path)
{
    unsigned char head[8];
    int fd, ok = 0;
    ssize_t n;
    if (!path) return 0;
    fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    n = read(fd, head, 8);
    close(fd);
    if (n == 8 && u32(head) == UINT32_C(0x68656164)) ok = (int)u32(head + 4);
    return ok == MAC_XBOX_MAP_VERSION ? ok : 0;
}

/* a zlib stream begins with a byte whose low nibble is 8 and a 16-bit
   header that is a multiple of 31 */
static int looks_zlib(const unsigned char *p)
{
    return (p[0] & 15) == 8 && ((p[0] << 8) | p[1]) % 31 == 0;
}

/* port: where the load time goes (mac_load_timing.h): the time of the first map open, and the milliseconds each part took */
double mac_load_clock(void)
{
    struct timeval now;
    gettimeofday(&now, 0);
    return (double)now.tv_sec * 1000.0 + (double)now.tv_usec / 1000.0;
}

static double load_first_open;
static double load_slot_ms[MAC_LOAD_SLOTS];
static double load_open_in_tags, load_open_at_tags_begin;
static int load_reported;

void mac_load_timing_add(int slot, double milliseconds)
{
    if (slot == MAC_LOAD_TAGS_BEGIN) load_open_at_tags_begin = load_slot_ms[MAC_LOAD_SLOT_CACHE];
    else if (slot == MAC_LOAD_SLOT_TAGS) {
        load_slot_ms[slot] += milliseconds;
        load_open_in_tags += load_slot_ms[MAC_LOAD_SLOT_CACHE] - load_open_at_tags_begin;
    } else if (slot >= 0 && slot < MAC_LOAD_SLOTS) load_slot_ms[slot] += milliseconds;
}

/* once, when the first frame of the game has been drawn */
void mac_load_timing_report(void)
{
    double total, tags;
    if (load_reported || load_first_open == 0.0) return;
    load_reported = 1;
    total = mac_load_clock() - load_first_open;
    /* game_load opens the cache file inside itself when the map is not open yet: that open is not counted twice */
    tags = load_slot_ms[MAC_LOAD_SLOT_TAGS] - load_open_in_tags;
    fprintf(stderr, "load timing: map open to first frame %.0f ms: cache file open (inflate or read) %.0f, tag load %.0f, "
        "texture decode and upload %.0f, program compile %.0f, other %.0f\n",
        total, load_slot_ms[MAC_LOAD_SLOT_CACHE], tags, load_slot_ms[MAC_LOAD_SLOT_TEXTURES],
        load_slot_ms[MAC_LOAD_SLOT_PROGRAMS],
        total - load_slot_ms[MAC_LOAD_SLOT_CACHE] - tags - load_slot_ms[MAC_LOAD_SLOT_TEXTURES] -
        load_slot_ms[MAC_LOAD_SLOT_PROGRAMS]);
}

/* port: the loading screen's pump, called between the steps that inflate a map (set by the session program) */
void (*mac_xbox_map_pump)(void) = NULL;
static const unsigned int mac_xbox_map_step_bytes = 2u << 20;

/* ---------- the inflated map, kept on disk
   Inflating a disc map into memory costs its whole size each run (b30's is 225
   MB). The first run writes the inflated map to a cache file, streaming it out
   of zlib in 1 MB pieces, and every later run maps that file read-only: only the
   pages the game reads become resident. The cache is the folder
   HALO_MAC_MAP_CACHE, else ".opence-cache" beside the map. A cache file is a
   0x4000-byte header (a multiple of any page size, so the map starts a page) (little endian: signature, version, the disc file's size and
   modification time, the inflated length, the CRC-32 of the inflated map)
   and the inflated map. It is used only if all of those agree with the disc
   file and the map's own header bytes match the disc file's; HALO_MAC_MAP_CACHE=off
   turns it off and HALO_MAC_MAP_CACHE_VERIFY=1 checks the CRC-32 too. Anything
   wrong with it (or no room to write it) falls back to inflating into memory. */
#define CACHE_HEADER_BYTES 0x4000
#define CACHE_SIGNATURE_0 UINT32_C(0x434d454f) /* "OEMC" */
#define CACHE_SIGNATURE_1 UINT32_C(0x3143504d) /* "MPC1" */
static void put32(unsigned char *p, uint32_t v)
{ p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); p[2] = (unsigned char)(v >> 16); p[3] = (unsigned char)(v >> 24); }

static int cache_path(const char *path, char *out, size_t capacity)
{
    const char *environment = getenv("HALO_MAC_MAP_CACHE"), *slash = strrchr(path, '/'), *leaf = slash ? slash + 1 : path;
    char directory[1024];
    int n;
    if (environment && !strcmp(environment, "off")) return 0;
    if (environment && *environment) n = snprintf(directory, sizeof(directory), "%s", environment);
    else if (slash) n = snprintf(directory, sizeof(directory), "%.*s/.opence-cache", (int)(slash - path), path);
    else n = snprintf(directory, sizeof(directory), ".opence-cache");
    if (n < 0 || (size_t)n >= sizeof(directory)) return 0;
    mkdir(directory, 0755);
    n = snprintf(out, capacity, "%s/%s.inflated", directory, leaf);
    return n > 0 && (size_t)n < capacity;
}

/* the cache file's map, if it is this disc file's */
static int cache_map(const char *cache, const struct stat *disc_stat, const unsigned char *disc,
    uint32_t file_length, struct mac_mapped_file *out)
{
    unsigned char h[64];
    struct stat st;
    int fd = open(cache, O_RDONLY);
    unsigned char *image;
    if (fd < 0) return 0;
    if (fstat(fd, &st) != 0 || (uint64_t)st.st_size != (uint64_t)CACHE_HEADER_BYTES + file_length ||
        pread(fd, h, sizeof(h), 0) != (ssize_t)sizeof(h) ||
        u32(h) != CACHE_SIGNATURE_0 || u32(h + 4) != CACHE_SIGNATURE_1 ||
        u32(h + 8) != (uint32_t)disc_stat->st_size || u32(h + 12) != (uint32_t)disc_stat->st_mtime ||
        u32(h + 16) != file_length) { close(fd); return 0; }
    image = (unsigned char *)mmap(NULL, file_length, PROT_READ, MAP_PRIVATE, fd, CACHE_HEADER_BYTES);
    close(fd);
    if (image == (unsigned char *)MAP_FAILED) return 0;
    if (memcmp(image, disc, MAC_XBOX_MAP_HEADER_BYTES) != 0) { munmap(image, file_length); return 0; }
    if (getenv("HALO_MAC_MAP_CACHE_VERIFY") && crc32(0L, image, file_length) != u32(h + 20)) {
        munmap(image, file_length); return 0;
    }
    out->data = image; out->size = file_length;
    return 1;
}

/* inflate the disc map into a new cache file (written beside, renamed when whole) */
static int cache_make(const char *cache, const struct stat *disc_stat, const unsigned char *disc, size_t disc_size,
    uint32_t file_length)
{
    char temporary[1100];
    unsigned char h[CACHE_HEADER_BYTES], *buffer;
    z_stream z;
    uLong crc;
    int fd, status, ok = 0;
    if (snprintf(temporary, sizeof(temporary), "%s.%ld.tmp", cache, (long)getpid()) >= (int)sizeof(temporary)) return 0;
    fd = open(temporary, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0) return 0;
    buffer = (unsigned char *)malloc(1 << 20);
    memset(h, 0, sizeof(h));
    if (!buffer || write(fd, h, sizeof(h)) != (ssize_t)sizeof(h) ||
        write(fd, disc, MAC_XBOX_MAP_HEADER_BYTES) != MAC_XBOX_MAP_HEADER_BYTES) goto done;
    crc = crc32(0L, disc, MAC_XBOX_MAP_HEADER_BYTES);
    memset(&z, 0, sizeof(z));
    if (inflateInit(&z) != Z_OK) goto done;
    z.next_in = (Bytef *)(disc + MAC_XBOX_MAP_HEADER_BYTES);
    z.avail_in = (uInt)(disc_size - MAC_XBOX_MAP_HEADER_BYTES);
    do {
        size_t produced;
        z.next_out = buffer; z.avail_out = 1 << 20;
        status = inflate(&z, Z_NO_FLUSH);
        if (status != Z_OK && status != Z_STREAM_END) break;
        produced = (1 << 20) - z.avail_out;
        if (z.total_out > file_length - MAC_XBOX_MAP_HEADER_BYTES) { status = Z_DATA_ERROR; break; }
        if (produced) {
            crc = crc32(crc, buffer, (uInt)produced);
            if (write(fd, buffer, produced) != (ssize_t)produced) { status = Z_ERRNO; break; }
        }
    } while (status == Z_OK);
    ok = status == Z_STREAM_END && z.total_out == file_length - MAC_XBOX_MAP_HEADER_BYTES;
    inflateEnd(&z);
    if (ok) {
        put32(h, CACHE_SIGNATURE_0); put32(h + 4, CACHE_SIGNATURE_1);
        put32(h + 8, (uint32_t)disc_stat->st_size); put32(h + 12, (uint32_t)disc_stat->st_mtime);
        put32(h + 16, file_length); put32(h + 20, (uint32_t)crc);
        ok = pwrite(fd, h, 64, 0) == 64;
    }
done:
    free(buffer);
    if (close(fd) != 0) ok = 0;
    if (ok && rename(temporary, cache) != 0) ok = 0;
    if (!ok) unlink(temporary);
    return ok;
}

static int xbox_map_open(const char *path, struct mac_mapped_file *out, const char **error);

int mac_xbox_map_open(const char *path, struct mac_mapped_file *out, const char **error)
{
    double begin = mac_load_clock(); /* port: mac_load_timing.h */
    int opened;
    if (load_first_open == 0.0) load_first_open = begin;
    opened = xbox_map_open(path, out, error);
    load_slot_ms[MAC_LOAD_SLOT_CACHE] += mac_load_clock() - begin;
    return opened;
}

static int xbox_map_open(const char *path, struct mac_mapped_file *out, const char **error)
{
    struct stat st;
    int fd;
    const unsigned char *disc;
    unsigned char *image;
    uint32_t file_length;
    size_t disc_size;
    if (error) *error = "xbox map output";
    if (!out) return 0;
    out->data = NULL; out->size = 0;
    if (!path) { if (error) *error = "xbox map path"; return 0; }
    fd = open(path, O_RDONLY);
    if (fd < 0) { if (error) *error = "xbox map open"; return 0; }
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < MAC_XBOX_MAP_HEADER_BYTES ||
        (uint64_t)st.st_size > (uint64_t)0x7fffffff) {
        close(fd); if (error) *error = "xbox map size or type"; return 0;
    }
    disc_size = (size_t)st.st_size;
    disc = (const unsigned char *)mmap(NULL, disc_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (disc == (const unsigned char *)MAP_FAILED) { if (error) *error = "xbox map address space"; return 0; }
    if (u32(disc) != UINT32_C(0x68656164) || u32(disc + 0x7fc) != UINT32_C(0x666f6f74) ||
        u32(disc + 4) != MAC_XBOX_MAP_VERSION) {
        munmap((void *)disc, disc_size); if (error) *error = "not a version 5 cache file"; return 0;
    }
    file_length = u32(disc + 8);
    if (file_length < MAC_XBOX_MAP_HEADER_BYTES || file_length > MAC_XBOX_MAP_MAX_BYTES) {
        munmap((void *)disc, disc_size); if (error) *error = "xbox map declared length"; return 0;
    }
    if (disc_size >= (size_t)file_length || !looks_zlib(disc + MAC_XBOX_MAP_HEADER_BYTES)) {
        /* already the whole map (a disk copy): used as it is */
        if (disc_size < (size_t)file_length) {
            munmap((void *)disc, disc_size); if (error) *error = "xbox map shorter than its header says"; return 0;
        }
        out->data = disc; out->size = disc_size; if (error) *error = NULL;
        return 1;
    }
    {
        char cache[1200];
        if (cache_path(path, cache, sizeof(cache))) {
            int used = cache_map(cache, &st, disc, file_length, out), made = 0;
            if (!used && (made = cache_make(cache, &st, disc, disc_size, file_length)) != 0)
                used = cache_map(cache, &st, disc, file_length, out);
            if (getenv("HALO_MAC_MAP_CACHE_LOG"))
                fprintf(stderr, "xbox map cache %s: %s\n", cache, used ? (made ? "made" : "used") : "not usable, inflated in memory");
            if (used) {
                munmap((void *)disc, disc_size);
                if (error) *error = NULL;
                return 1;
            }
        }
    }
    image = (unsigned char *)mmap(NULL, file_length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (image == (unsigned char *)MAP_FAILED) {
        munmap((void *)disc, disc_size); if (error) *error = "xbox map memory"; return 0;
    }
    memcpy(image, disc, MAC_XBOX_MAP_HEADER_BYTES);
    {
        z_stream z;
        int status;
        memset(&z, 0, sizeof(z));
        if (inflateInit(&z) != Z_OK) {
            munmap(image, file_length); munmap((void *)disc, disc_size);
            if (error) *error = "xbox map inflate"; return 0;
        }
        z.next_in = (Bytef *)(disc + MAC_XBOX_MAP_HEADER_BYTES);
        z.avail_in = (uInt)(disc_size - MAC_XBOX_MAP_HEADER_BYTES);
        z.next_out = image + MAC_XBOX_MAP_HEADER_BYTES;
        z.avail_out = file_length - MAC_XBOX_MAP_HEADER_BYTES;
        /* port: in steps of mac_xbox_map_step_bytes, between which the loading screen is drawn
           (mac_xbox_map_pump); the last step finishes the stream, as a single call would */
        {
            uInt remaining = z.avail_out;
            for (;;) {
                uInt step = remaining > mac_xbox_map_step_bytes ? mac_xbox_map_step_bytes : remaining;
                z.avail_out = step;
                status = inflate(&z, step == remaining ? Z_FINISH : Z_NO_FLUSH);
                remaining -= step - z.avail_out;
                if (status != Z_OK || remaining == 0)
                    break;
                if (mac_xbox_map_pump)
                    mac_xbox_map_pump();
            }
            z.avail_out = remaining;
        }
        /* the stream must end exactly where the header says the map does */
        if (status != Z_STREAM_END || z.avail_out != 0) {
            inflateEnd(&z); munmap(image, file_length); munmap((void *)disc, disc_size);
            if (error) *error = "xbox map compressed data is damaged or its length is not the header's";
            return 0;
        }
        inflateEnd(&z);
    }
    munmap((void *)disc, disc_size);
    mprotect(image, file_length, PROT_READ);
    out->data = image; out->size = file_length;
    if (error) *error = NULL;
    return 1;
}

/* a virtual address in the tag data -> offset in the map, or 0 with failure */
static int tag_span(const unsigned char *map, const struct mac_xbox_map_info *i,
    uint32_t address, uint32_t bytes, const unsigned char **out)
{
    uint32_t offset;
    if (address < MAC_XBOX_TAG_BASE) return 0;
    offset = address - MAC_XBOX_TAG_BASE;
    if (!fits(offset, bytes, i->tag_data_size)) return 0;
    *out = map + i->tag_data_offset + offset;
    return 1;
}

/* buffer tables: count 12-byte descriptors in region [base,base+bytes), each
   with a data address inside [lo,hi) of the tag cache */
static int buffers_ok(uint32_t count, uint32_t address, uint32_t base, uint32_t bytes)
{
    uint32_t offset;
    if (!count) return 1;
    if (address < base) return 0;
    offset = address - base;
    if (offset > bytes || count > (bytes - offset) / 12) return 0;
    return 1;
}

int mac_xbox_map_inspect(const unsigned char *map, size_t size,
    struct mac_xbox_map_info *out, const char **error)
{
    struct mac_xbox_map_info i;
    const unsigned char *th, *inst, *scenario, *refs, *p;
    uint32_t k, n, address;
#define REJECT(message) do { if (error) *error = (message); return 0; } while (0)
    if (error) *error = NULL;
    if (!out) { if (error) *error = "xbox map info output"; return 0; }
    memset(out, 0, sizeof(*out)); memset(&i, 0, sizeof(i));
    if (!map || size < MAC_XBOX_MAP_HEADER_BYTES) REJECT("file size");
    if (u32(map) != UINT32_C(0x68656164) || u32(map + 0x7fc) != UINT32_C(0x666f6f74)) REJECT("header signatures");
    if (u32(map + 4) != MAC_XBOX_MAP_VERSION) REJECT("not version 5");
    i.file_length = u32(map + 8);
    if (i.file_length < MAC_XBOX_MAP_HEADER_BYTES || i.file_length > MAC_XBOX_MAP_MAX_BYTES ||
        i.file_length > size) REJECT("declared length");
    if (!memchr(map + 0x20, 0, 32) || !memchr(map + 0x40, 0, 32)) REJECT("header string");
    i.tag_data_offset = u32(map + 0x10); i.tag_data_size = u32(map + 0x14);
    if (i.tag_data_offset < MAC_XBOX_MAP_HEADER_BYTES || i.tag_data_size < 0x24 ||
        i.tag_data_size > MAC_XBOX_TAG_CACHE_BYTES ||
        !fits(i.tag_data_offset, i.tag_data_size, i.file_length)) REJECT("tag data range");
    i.scenario_type = (uint32_t)map[0x60] | (uint32_t)map[0x61] << 8;
    i.checksum = u32(map + 0x64);
    memcpy(i.name, map + 0x20, 32); memcpy(i.build, map + 0x40, 32);
    th = map + i.tag_data_offset;
    if (u32(th + 0x20) != UINT32_C(0x74616773)) REJECT("tag header signature");
    i.tag_count = u32(th + 0xc);
    if (!i.tag_count || i.tag_count > 65535) REJECT("tag count");
    i.scenario_handle = u32(th + 4); i.checksum = u32(th + 8);
    address = u32(th);
    if (address < MAC_XBOX_TAG_BASE || (address & 3) ||
        !fits(address - MAC_XBOX_TAG_BASE, i.tag_count * 32, i.tag_data_size)) REJECT("tag table");
    inst = th + (address - MAC_XBOX_TAG_BASE);
    i.vertex_buffers = u32(th + 0x10); i.index_buffers = u32(th + 0x18);
    if (!buffers_ok(i.vertex_buffers, u32(th + 0x14), MAC_XBOX_TAG_BASE, i.tag_data_size)) REJECT("vertex buffer table");
    if (!buffers_ok(i.index_buffers, u32(th + 0x1c), MAC_XBOX_TAG_BASE, i.tag_data_size)) REJECT("index buffer table");
    for (k = 0; k < i.vertex_buffers + i.index_buffers; ++k) {
        uint32_t table = k < i.vertex_buffers ? u32(th + 0x14) : u32(th + 0x1c);
        uint32_t entry = k < i.vertex_buffers ? k : k - i.vertex_buffers;
        p = th + (table - MAC_XBOX_TAG_BASE) + entry * 12;
        address = u32(p + 4);
        /* a buffer's data lies in the tag cache (the bsps' and tags' alike) */
        if (address < MAC_XBOX_TAG_BASE || address - MAC_XBOX_TAG_BASE >= MAC_XBOX_TAG_CACHE_BYTES) REJECT("buffer data address");
    }
    for (k = 0; k < i.tag_count; ++k) {
        const unsigned char *t = inst + k * 32, *name;
        uint32_t base, avail;
        if ((u32(t + 12) & 0xffff) != k) REJECT("tag handle");
        if (!tag_span(map, &i, u32(t + 16), 1, &name)) REJECT("tag name address");
        avail = i.tag_data_size - (u32(t + 16) - MAC_XBOX_TAG_BASE);
        if (avail > 256) avail = 256;
        if (!memchr(name, 0, avail)) REJECT("tag name termination");
        base = u32(t + 20);
        if (u32(t) == UINT32_C(0x73627370)) {
            if (base) REJECT("unexpected bsp tag address");
        } else if (base && !tag_span(map, &i, base, 1, &name)) REJECT("tag definition address");
    }
    if ((i.scenario_handle & 0xffff) >= i.tag_count) REJECT("scenario handle");
    {
        const unsigned char *t = inst + (i.scenario_handle & 0xffff) * 32;
        if (u32(t + 12) != i.scenario_handle || u32(t) != UINT32_C(0x73636e72)) REJECT("scenario tag");
        i.scenario_address = u32(t + 20);
        if (!tag_span(map, &i, i.scenario_address, 0x5b0, &scenario)) REJECT("scenario range");
    }
    i.bsp_count = u32(scenario + 0x5a4); address = u32(scenario + 0x5a8);
    if (i.bsp_count > MAC_XBOX_MAX_BSPS) REJECT("bsp count");
    if (i.bsp_count && (!tag_span(map, &i, address, i.bsp_count * 32, &refs) || (address & 3))) REJECT("bsp references");
    for (k = 0; k < i.bsp_count; ++k) {
        struct mac_xbox_bsp *b = &i.bsps[k];
        const unsigned char *r = refs + k * 32, *h;
        uint32_t j, root;
        b->file_offset = u32(r); b->size = u32(r + 4); b->base = u32(r + 8); b->handle = u32(r + 28);
        n = b->handle & 0xffff;
        if (n >= i.tag_count || u32(inst + n * 32 + 12) != b->handle || u32(inst + n * 32) != UINT32_C(0x73627370)) REJECT("bsp tag handle");
        if (b->file_offset < MAC_XBOX_MAP_HEADER_BYTES || b->size < 0x18 || b->size > MAC_XBOX_TAG_CACHE_BYTES ||
            !fits(b->file_offset, b->size, i.file_length)) REJECT("bsp file range");
        /* read to the top of the tag cache after the tag data, a whole number of sectors */
        if (b->base < MAC_XBOX_TAG_BASE + i.tag_data_size ||
            !fits(b->base - MAC_XBOX_TAG_BASE, b->size, MAC_XBOX_TAG_CACHE_BYTES)) REJECT("bsp virtual range");
        h = map + b->file_offset;
        if (u32(h + 0x14) != UINT32_C(0x73627370)) REJECT("bsp header signature");
        root = u32(h);
        if (root < b->base || !fits(root - b->base, 0x288, b->size)) REJECT("bsp root");
        b->vertex_buffers = u32(h + 4); b->index_buffers = u32(h + 0xc);
        if (!buffers_ok(b->vertex_buffers, u32(h + 8), b->base, b->size)) REJECT("bsp vertex buffer table");
        if (!buffers_ok(b->index_buffers, u32(h + 0x10), b->base, b->size)) REJECT("bsp lightmap buffer table");
        for (j = 0; j < b->vertex_buffers + b->index_buffers; ++j) {
            uint32_t table = j < b->vertex_buffers ? u32(h + 8) : u32(h + 0x10);
            uint32_t entry = j < b->vertex_buffers ? j : j - b->vertex_buffers;
            address = u32(h + (table - b->base) + entry * 12 + 4);
            if (address < b->base || address - b->base >= b->size) REJECT("bsp buffer data address");
        }
    }
    i.compressed_on_disc = 0;
    *out = i;
    return 1;
#undef REJECT
}
