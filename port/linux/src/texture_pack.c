/*
TEXTURE_PACK.C

Texture packs; see texture_pack.h and docs/texture_packs.md.
*/

#include "texture_pack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(HALO_LINUX_PLATFORM_LAYER)
/* (the Linux build's <sys/stat.h> is MSVC's, without mkdir) */
#include "posix.h"
#define make_directory(path) posix_make_directory(path)
#elif !defined(_WIN32)
#include <sys/stat.h>
#include <sys/types.h>
#define make_directory(path) mkdir((path), 0755)
#endif

/* what a pack's files may be: the largest file, the edge of an image, its
mip levels */
#define MAX_FILE_BYTES (96UL * 1024UL * 1024UL)
#define MAX_EDGE 4096UL
#define MAX_LEVELS 13UL
#define MAX_PATH_BYTES 1024
#define MAX_TOML_BYTES 65536UL
#define MEMO_SLOTS 8192
#define ACCEPTED_SLOTS 4096
#define MAX_REFUSALS_LOGGED 24

#define DDS_HEADER_BYTES 128UL
#define DDPF_FOURCC 0x4UL
#define DDPF_RGB 0x40UL
#define DDSCAPS2_CUBEMAP 0x200UL
#define DDSCAPS2_CUBE_FACES 0xFC00UL
#define DDSCAPS2_VOLUME 0x200000UL
#define DDSD_MIPMAPCOUNT 0x20000UL
#define FOURCC_DXT1 0x31545844UL
#define FOURCC_DXT3 0x33545844UL
#define FOURCC_DXT5 0x35545844UL

static int pack_open;
static char pack_root[MAX_PATH_BYTES]; /* data_root/texture_packs/pack */
static char pack_name[132], pack_version[132], pack_author[132], pack_licence[132];
static unsigned int limit_dimension;
static int dxt_allowed, rgba_allowed;
static size_t budget_total;

/* files known to be absent or refused, by the hash of their path */
static unsigned long memo[MEMO_SLOTS];
static unsigned int memo_count;
/* the replacements in use, by the hash of their path, with their bytes */
static unsigned long accepted_key[ACCEPTED_SLOTS];
static size_t accepted_size[ACCEPTED_SLOTS];
static unsigned int accepted_count;
static size_t accepted_bytes;
/* the ones left out for the budget since the last reset (not read again) */
static unsigned long skipped_key[ACCEPTED_SLOTS];
static unsigned int skipped_count;
/* the same over all the maps played (the summary's) */
static unsigned long used_total, used_bytes_total, used_bytes_peak;

static unsigned long count_skipped_budget, count_refused, count_too_large, count_unsupported;
static unsigned int refusals_logged;
static int summary_logged, exit_registered;

static unsigned long hash_string(const char *text)
{
	unsigned long hash = 2166136261UL;
	while (*text)
	{
		hash ^= (unsigned char)*text++;
		hash = (hash * 16777619UL) & 0xFFFFFFFFUL;
	}
	return hash ? hash : 1UL;
}

static int memo_has(unsigned long key)
{
	unsigned int slot = (unsigned int)(key % MEMO_SLOTS), probes;
	for (probes = 0; probes < MEMO_SLOTS; ++probes)
	{
		if (!memo[slot]) return 0;
		if (memo[slot] == key) return 1;
		slot = (slot + 1) % MEMO_SLOTS;
	}
	return 0;
}

static void memo_add(unsigned long key)
{
	unsigned int slot = (unsigned int)(key % MEMO_SLOTS), probes;
	if (memo_count >= MEMO_SLOTS / 2) return; /* (a full table asks the file system again) */
	for (probes = 0; probes < MEMO_SLOTS; ++probes)
	{
		if (!memo[slot]) { memo[slot] = key; ++memo_count; return; }
		if (memo[slot] == key) return;
		slot = (slot + 1) % MEMO_SLOTS;
	}
}

static unsigned long read32(const unsigned char *p)
{
	return (unsigned long)p[0] | (unsigned long)p[1] << 8 | (unsigned long)p[2] << 16 | (unsigned long)p[3] << 24;
}

static int power_of_two(unsigned long value)
{
	return value && !(value & (value - 1));
}

static unsigned int log2_of(unsigned long value)
{
	unsigned int bits = 0;
	while ((1UL << bits) < value) ++bits;
	return bits;
}

size_t texture_pack_level_bytes(unsigned int format, unsigned int width, unsigned int height)
{
	if (format == TEXTURE_PACK_FORMAT_RGBA) return (size_t)width * height * 4;
	return (size_t)((width + 3) / 4) * ((height + 3) / 4) * (format == TEXTURE_PACK_FORMAT_DXT1 ? 8 : 16);
}

static unsigned int level_edge(unsigned int edge, unsigned int level)
{
	edge >>= level;
	return edge ? edge : 1;
}

/* ---- names ---- */

/* A tag path as a relative file path (slashes, no dot components, only
characters a Halo tag name has), or 0 */
static int tag_path(const char *tag, char *path, size_t capacity)
{
	size_t length = 0, component = 0;
	unsigned char c;
	if (!tag || !tag[0]) return 0;
	for (; *tag; ++tag)
	{
		c = (unsigned char)*tag;
		if (c == '\\') c = '/';
		if (c == '/')
		{
			if (!component) return 0; /* empty component, or a leading slash */
			if (component == 1 && path[length - 1] == '.') return 0;
			if (component == 2 && path[length - 1] == '.' && path[length - 2] == '.') return 0;
			component = 0;
		}
		else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
			c == '_' || c == '-' || c == '.' || c == ' ')
			++component;
		else
			return 0;
		if (length + 2 >= capacity) return 0;
		path[length++] = (char)c;
	}
	path[length] = 0;
	if (!component) return 0; /* a trailing slash */
	if (component == 1 && path[length - 1] == '.') return 0;
	if (component == 2 && path[length - 1] == '.' && path[length - 2] == '.') return 0;
	return 1;
}

static int pack_name_valid(const char *pack)
{
	size_t length = 0;
	if (!pack || pack[0] == '.') return 0;
	for (; pack[length]; ++length)
	{
		char c = pack[length];
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
			c == '_' || c == '-' || c == '.' || c == ' ')) return 0;
	}
	return length > 0 && length < 100;
}

/* ---- pack.toml: the top-level  key = "string"  lines ---- */

static void toml_string(const char *line, const char *key, char *out, size_t capacity)
{
	size_t key_length = strlen(key), used = 0;
	if (strncmp(line, key, key_length) != 0) return;
	line += key_length;
	while (*line == ' ' || *line == '\t') ++line;
	if (*line != '=') return;
	++line;
	while (*line == ' ' || *line == '\t') ++line;
	if (*line != '"') return;
	++line;
	while (*line && *line != '"' && used + 1 < capacity)
	{
		unsigned char c = (unsigned char)*line++;
		if (c == '\\' && *line) c = (unsigned char)*line++;
		out[used++] = (c < 32 || c > 126) ? '?' : (char)c;
	}
	out[used] = 0;
}

static int read_pack_toml(char *message, size_t message_size)
{
	char path[MAX_PATH_BYTES + 16];
	char *text, *line, *end;
	FILE *file;
	long size;
	size_t got;
	pack_name[0] = pack_version[0] = pack_author[0] = pack_licence[0] = 0;
	sprintf(path, "%s/pack.toml", pack_root);
	file = fopen(path, "rb");
	if (!file) { sprintf(message, "no pack.toml"); return 0; }
	if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 || (unsigned long)size > MAX_TOML_BYTES ||
		fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		sprintf(message, "pack.toml is too large or unreadable");
		return 0;
	}
	text = (char *)malloc((size_t)size + 1);
	if (!text) { fclose(file); sprintf(message, "out of memory"); return 0; }
	got = fread(text, 1, (size_t)size, file);
	fclose(file);
	text[got] = 0;
	for (line = text; *line; line = end)
	{
		char *next;
		end = strchr(line, '\n');
		if (end) { *end = 0; next = end + 1; } else next = line + strlen(line);
		end = next;
		while (*line == ' ' || *line == '\t' || *line == '\r') ++line;
		if (*line == '[') break; /* the tables after it (the files' checksums) are the tool's */
		if (*line == '#' || !*line) continue;
		toml_string(line, "name", pack_name, sizeof(pack_name));
		toml_string(line, "version", pack_version, sizeof(pack_version));
		toml_string(line, "author", pack_author, sizeof(pack_author));
		toml_string(line, "licence", pack_licence, sizeof(pack_licence));
		toml_string(line, "license", pack_licence, sizeof(pack_licence));
	}
	free(text);
	(void)message_size;
	if (!pack_name[0]) { sprintf(message, "pack.toml has no name"); return 0; }
	if (!pack_licence[0]) { sprintf(message, "pack.toml has no licence"); return 0; }
	return 1;
}

/* ---- the pack ---- */

void texture_pack_summary(void)
{
	if (!pack_open) return;
	summary_logged = 1;
	fprintf(stderr, "texture pack \"%s\": %lu replacements used (%lu bytes, at most %lu at once of a budget of %lu), %lu left out for the budget, "
		"%lu too large for %u px, %lu refused, %lu unsupported\n",
		pack_name, used_total, used_bytes_total, used_bytes_peak, (unsigned long)budget_total,
		count_skipped_budget, count_too_large, limit_dimension, count_refused, count_unsupported);
}

static void summary_at_exit(void)
{
	if (!summary_logged) texture_pack_summary();
}

int texture_pack_open(const char *data_root, const char *pack, unsigned int max_dimension,
	int allow_dxt, int allow_rgba, size_t budget_bytes, char *message, size_t message_size)
{
	char scratch[256];
	(void)message_size;
	if (pack_open) return 1;
	if (!message) message = scratch;
	message[0] = 0;
	if (!data_root || !data_root[0] || !pack_name_valid(pack))
	{
		sprintf(message, "the pack name is empty or has characters other than letters, digits, space, - _ .");
		return 0;
	}
	if (strlen(data_root) + strlen(pack) + 32 > MAX_PATH_BYTES) { sprintf(message, "the path is too long"); return 0; }
	sprintf(pack_root, "%s/texture_packs/%s", data_root, pack);
	if (!read_pack_toml(message, message_size)) return 0;
	limit_dimension = max_dimension;
	dxt_allowed = allow_dxt;
	rgba_allowed = allow_rgba;
	budget_total = budget_bytes;
	memset(memo, 0, sizeof(memo));
	memo_count = accepted_count = skipped_count = 0;
	accepted_bytes = 0;
	used_total = used_bytes_total = used_bytes_peak = 0;
	count_skipped_budget = count_refused = count_too_large = count_unsupported = 0;
	refusals_logged = 0;
	summary_logged = 0;
	pack_open = 1;
	if (!exit_registered) { exit_registered = 1; atexit(summary_at_exit); }
	fprintf(stderr, "texture pack \"%s\" %s by %s, licence %s: levels up to %u px, %s%s%s, %lu bytes of budget\n",
		pack_name, pack_version[0] ? pack_version : "(no version)", pack_author[0] ? pack_author : "(no author)",
		pack_licence, max_dimension, dxt_allowed ? "DXT" : "", dxt_allowed && rgba_allowed ? " and " : "",
		rgba_allowed ? "RGBA" : "", (unsigned long)budget_bytes);
	return 1;
}

int texture_pack_active(void)
{
	return pack_open;
}

void texture_pack_reset(void)
{
	accepted_count = 0;
	accepted_bytes = 0;
	skipped_count = 0;
}

void texture_pack_image_free(struct texture_pack_image *image)
{
	if (!image) return;
	free(image->data);
	memset(image, 0, sizeof(*image));
}

const unsigned char *texture_pack_image_level(const struct texture_pack_image *image,
	unsigned int face, unsigned int level, unsigned int *width, unsigned int *height, size_t *bytes)
{
	size_t offset = 0;
	unsigned int l;
	if (!image || !image->data || face >= image->faces || level >= image->levels) return NULL;
	for (l = 0; l < level; ++l)
		offset += texture_pack_level_bytes(image->format, level_edge(image->width, l), level_edge(image->height, l));
	if (width) *width = level_edge(image->width, level);
	if (height) *height = level_edge(image->height, level);
	if (bytes) *bytes = texture_pack_level_bytes(image->format, level_edge(image->width, level), level_edge(image->height, level));
	return image->data + face * image->face_bytes + offset;
}

static void refuse(const char *path, const char *why)
{
	++count_refused;
	if (refusals_logged < MAX_REFUSALS_LOGGED)
	{
		++refusals_logged;
		fprintf(stderr, "texture pack: refused %s: %s\n", path, why);
		if (refusals_logged == MAX_REFUSALS_LOGGED) fprintf(stderr, "texture pack: (no more refusals are logged)\n");
	}
}

/* Is key one of the replacements in use? If not and it fits the budget, it is now. */
static int accept(unsigned long key, size_t bytes)
{
	unsigned int i;
	for (i = 0; i < accepted_count; ++i)
		if (accepted_key[i] == key) return 1;
	if (accepted_count >= ACCEPTED_SLOTS || bytes > budget_total / 4 ||
		accepted_bytes > budget_total / 4 * 3 || bytes > budget_total / 4 * 3 - accepted_bytes)
		return 0;
	accepted_key[accepted_count] = key;
	accepted_size[accepted_count] = bytes;
	++accepted_count;
	accepted_bytes += bytes;
	++used_total;
	used_bytes_total += bytes;
	if (accepted_bytes > used_bytes_peak) used_bytes_peak = accepted_bytes;
	return 1;
}

/* The DDS file at path, read whole; NULL if absent or unreadable */
static unsigned char *read_file(const char *path, size_t *size_out, const char **why)
{
	FILE *file = fopen(path, "rb");
	unsigned char *data;
	long size;
	*why = NULL;
	if (!file) return NULL;
	if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		*why = "cannot read it";
		return NULL;
	}
	if ((unsigned long)size < DDS_HEADER_BYTES) { fclose(file); *why = "shorter than a DDS header"; return NULL; }
	if ((unsigned long)size > MAX_FILE_BYTES) { fclose(file); *why = "larger than the 96 MB limit"; return NULL; }
	data = (unsigned char *)malloc((size_t)size);
	if (!data) { fclose(file); *why = "out of memory"; return NULL; }
	if (fread(data, 1, (size_t)size, file) != (size_t)size)
	{
		fclose(file);
		free(data);
		*why = "truncated while reading";
		return NULL;
	}
	fclose(file);
	*size_out = (size_t)size;
	return data;
}

int texture_pack_load(const char *tag, unsigned int index, unsigned int stock_width,
	unsigned int stock_height, unsigned int stock_levels, int cube, struct texture_pack_image *out)
{
	char relative[MAX_PATH_BYTES / 2], path[MAX_PATH_BYTES + 64];
	unsigned char *file = NULL, *copy;
	const unsigned char *header, *body;
	size_t file_size = 0, level_offset[MAX_LEVELS + 1], face_stride, total, copied;
	unsigned long key, width, height, levels, flags, caps2, pf_flags, fourcc, bit_count, k;
	unsigned int format, faces, scale_log, drop, used, face, level, last_level, i;
	unsigned long edge_limit;
	const char *why = NULL;
	if (out) memset(out, 0, sizeof(*out));
	if (!pack_open || !out || index > 65535 || !stock_width || !stock_height || !stock_levels ||
		!tag_path(tag, relative, sizeof(relative)))
		return 0;
	sprintf(path, "%s/%s/%u.dds", pack_root, relative, index);
	key = hash_string(path);
	if (memo_has(key)) return 0;
	for (i = 0; i < skipped_count; ++i)
		if (skipped_key[i] == key) return 0;

	file = read_file(path, &file_size, &why);
	if (!file)
	{
		memo_add(key);
		if (why) refuse(path, why);
		return 0;
	}
	header = file;
	body = file + DDS_HEADER_BYTES;

	/* the header */
	if (memcmp(header, "DDS ", 4) != 0) { why = "not a DDS file (bad magic)"; goto bad; }
	if (read32(header + 4) != 124 || read32(header + 76) != 32) { why = "bad DDS header sizes"; goto bad; }
	flags = read32(header + 8);
	height = read32(header + 12);
	width = read32(header + 16);
	levels = (flags & DDSD_MIPMAPCOUNT) ? read32(header + 28) : 1;
	pf_flags = read32(header + 80);
	fourcc = read32(header + 84);
	bit_count = read32(header + 88);
	caps2 = read32(header + 112);
	if (!levels) levels = 1;
	if (!width || !height || width > MAX_EDGE || height > MAX_EDGE || !power_of_two(width) || !power_of_two(height))
	{
		why = "width and height must be powers of two up to 4096";
		goto bad;
	}
	if (levels > MAX_LEVELS || levels > log2_of(width > height ? width : height) + 1)
	{
		why = "bad mip level count";
		goto bad;
	}
	if (caps2 & DDSCAPS2_VOLUME) { why = "volume textures are not supported"; goto unsupported; }
	if (caps2 & DDSCAPS2_CUBEMAP)
	{
		if ((caps2 & DDSCAPS2_CUBE_FACES) != DDSCAPS2_CUBE_FACES) { why = "a cube map needs all six faces"; goto bad; }
		if (width != height) { why = "cube map faces must be square"; goto bad; }
		faces = 6;
	}
	else
		faces = 1;
	if ((faces == 6) != (cube != 0)) { why = cube ? "the bitmap is a cube map, the file is not" : "the file is a cube map, the bitmap is not"; goto bad; }
	if (pf_flags & DDPF_FOURCC)
	{
		format = fourcc == FOURCC_DXT1 ? TEXTURE_PACK_FORMAT_DXT1 : fourcc == FOURCC_DXT3 ? TEXTURE_PACK_FORMAT_DXT3 :
			fourcc == FOURCC_DXT5 ? TEXTURE_PACK_FORMAT_DXT5 : 0;
		if (!format) { why = "only DXT1, DXT3 and DXT5 are supported"; goto unsupported; }
		if (!dxt_allowed) { why = "this card takes no DXT"; goto unsupported; }
	}
	else if ((pf_flags & DDPF_RGB) && bit_count == 32 && read32(header + 92) == 0x00FF0000UL &&
		read32(header + 96) == 0x0000FF00UL && read32(header + 100) == 0x000000FFUL &&
		(read32(header + 104) == 0xFF000000UL || read32(header + 104) == 0))
	{
		format = TEXTURE_PACK_FORMAT_RGBA;
		if (!rgba_allowed) { why = "uncompressed pictures do not fit this card (use DXT)"; goto unsupported; }
	}
	else
	{
		why = "only DXT1, DXT3, DXT5 and 32-bit BGRA are supported";
		goto unsupported;
	}

	/* the data must be exactly the levels the header declares */
	total = 0;
	for (level = 0; level < levels; ++level)
	{
		size_t bytes = texture_pack_level_bytes(format, level_edge((unsigned int)width, level), level_edge((unsigned int)height, level));
		level_offset[level] = total;
		total += bytes;
	}
	level_offset[levels] = total;
	face_stride = total;
	if (face_stride * faces / faces != face_stride || face_stride * faces != file_size - DDS_HEADER_BYTES)
	{
		why = "the size does not match the header's size, levels and faces";
		goto bad;
	}

	/* the bitmap it replaces: the same aspect, a power-of-two multiple of its size */
	if (width % stock_width || height % stock_height || width / stock_width != height / stock_height ||
		!power_of_two(width / stock_width))
	{
		why = "not a power-of-two multiple of the bitmap's size, or another aspect";
		goto bad;
	}
	k = width / stock_width;
	scale_log = log2_of(k);
	if (stock_levels > 1 && levels < stock_levels + scale_log)
	{
		why = "too few mip levels: the chain must reach the bitmap's smallest level";
		goto bad;
	}

	/* the card's limit: leave out the largest levels, to the bitmap's own size at least */
	edge_limit = limit_dimension;
	drop = 0;
	while (drop < scale_log && ((width >> drop) > edge_limit || (height >> drop) > edge_limit)) ++drop;
	if ((width >> drop) > edge_limit || (height >> drop) > edge_limit)
	{
		++count_too_large;
		memo_add(key);
		free(file);
		return 0;
	}
	used = stock_levels > 1 ? stock_levels + (scale_log - drop) : 1;
	last_level = drop + used;
	if (last_level > levels) { why = "no mip chain to leave the card's largest levels out of"; goto bad; }

	copied = 0;
	for (level = drop; level < last_level; ++level) copied += level_offset[level + 1] - level_offset[level];
	if (copied > (size_t)-1 / faces) { why = "too large"; goto bad; }
	if (!accept(key, copied * faces))
	{
		++count_skipped_budget;
		if (skipped_count < ACCEPTED_SLOTS) skipped_key[skipped_count++] = key;
		free(file);
		return 0; /* (not memoized: a later map may have room) */
	}
	copy = (unsigned char *)malloc(copied * faces);
	if (!copy) { accepted_bytes -= accepted_size[--accepted_count]; free(file); return 0; }
	for (face = 0; face < faces; ++face)
	{
		unsigned char *to = copy + face * copied;
		for (level = drop; level < last_level; ++level)
		{
			size_t bytes = level_offset[level + 1] - level_offset[level];
			memcpy(to, body + face * face_stride + level_offset[level], bytes);
			if (format == TEXTURE_PACK_FORMAT_RGBA)
			{
				/* BGRA to RGBA; a file with no alpha is opaque */
				int opaque = read32(header + 104) == 0;
				for (i = 0; i + 3 < bytes; i += 4)
				{
					unsigned char swap = to[i];
					to[i] = to[i + 2];
					to[i + 2] = swap;
					if (opaque) to[i + 3] = 255;
				}
			}
			to += bytes;
		}
	}
	free(file);
	out->format = format;
	out->width = (unsigned int)(width >> drop);
	out->height = (unsigned int)(height >> drop);
	out->levels = used;
	out->faces = faces;
	out->face_bytes = copied;
	out->bytes = copied * faces;
	out->data = copy;
	return 1;

unsupported:
	++count_unsupported;
	if (refusals_logged < MAX_REFUSALS_LOGGED)
	{
		++refusals_logged;
		fprintf(stderr, "texture pack: skipped %s: %s\n", path, why);
	}
	memo_add(key);
	free(file);
	return 0;
bad:
	refuse(path, why);
	memo_add(key);
	free(file);
	return 0;
}

/* ---- for pack authors ---- */

static void put32(unsigned char *p, unsigned long value)
{
	p[0] = (unsigned char)value;
	p[1] = (unsigned char)(value >> 8);
	p[2] = (unsigned char)(value >> 16);
	p[3] = (unsigned char)(value >> 24);
}

void texture_pack_dds_header(unsigned char header[128], unsigned int format, unsigned int width,
	unsigned int height, unsigned int levels, int cube)
{
	memset(header, 0, DDS_HEADER_BYTES);
	memcpy(header, "DDS ", 4);
	put32(header + 4, 124);
	/* caps, height, width, pixel format, linear size, and mip count when there are some */
	put32(header + 8, 0x1007UL | 0x80000UL | (levels > 1 ? DDSD_MIPMAPCOUNT : 0));
	put32(header + 12, height);
	put32(header + 16, width);
	put32(header + 20, (unsigned long)texture_pack_level_bytes(format, width, height));
	put32(header + 28, levels);
	put32(header + 76, 32);
	put32(header + 80, DDPF_FOURCC);
	put32(header + 84, format == TEXTURE_PACK_FORMAT_DXT1 ? FOURCC_DXT1 : format == TEXTURE_PACK_FORMAT_DXT3 ? FOURCC_DXT3 : FOURCC_DXT5);
	put32(header + 108, 0x1000UL | (levels > 1 ? 0x400008UL : 0) | (cube ? 0x8UL : 0));
	put32(header + 112, cube ? DDSCAPS2_CUBEMAP | DDSCAPS2_CUBE_FACES : 0);
}

void *texture_pack_dump_open(const char *dir, const char *tag, unsigned int index)
{
#ifdef _WIN32
	(void)dir; (void)tag; (void)index;
	return NULL;
#else
	char relative[MAX_PATH_BYTES / 2], path[MAX_PATH_BYTES + 64];
	size_t i;
	FILE *file;
	if (!dir || !dir[0] || !tag_path(tag, relative, sizeof(relative)) || strlen(dir) + strlen(relative) + 32 > sizeof(path))
		return NULL;
	sprintf(path, "%s/%s/%u.dds", dir, relative, index);
	file = fopen(path, "rb");
	if (file) { fclose(file); return NULL; }
	make_directory(dir);
	for (i = strlen(dir) + 1; path[i]; ++i)
		if (path[i] == '/')
		{
			path[i] = 0;
			make_directory(path);
			path[i] = '/';
		}
	return fopen(path, "wb");
#endif
}
