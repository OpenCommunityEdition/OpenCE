/* See update_core.h. */
#include "update_core.h"
#include <CoreFoundation/CoreFoundation.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include "monocypher-ed25519.h"

/* ---------- versions */

/* the next number of a dotted version: digits, and whatever follows them up to the next dot is skipped */
static long next_part(const char **p)
{
	long n = 0;
	const char *s = *p;

	while (*s == ' ')
		++s;
	while (*s >= '0' && *s <= '9')
	{
		if (n < 100000000L)
			n = n * 10 + (*s - '0');
		++s;
	}
	while (*s && *s != '.')
		++s;
	if (*s == '.')
		++s;
	*p = s;
	return n;
}

int uc_version_compare(const char *a, const char *b)
{
	while (*a || *b)
	{
		long x = next_part(&a), y = next_part(&b);

		if (x != y)
			return x < y ? -1 : 1;
	}
	return 0;
}

/* ---------- the statement, base64 and Ed25519 */

size_t uc_statement(const struct uc_item *item, char *out, size_t size)
{
	int n = snprintf(out, size,
		"PowerSpark-update-1\nversion=%s\nbuild=%ld\nsha256=%s\nlength=%ld\nurl=%s\nminimumSystemVersion=%s\n",
		item->version, item->build, item->sha256, item->length, item->url, item->min_os);

	return n < 0 || (size_t)n >= size ? 0 : (size_t)n;
}

int uc_base64_decode(const char *text, unsigned char *out, size_t size)
{
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned long bits = 0;
	int have = 0, count = 0, padding = 0;

	for (; *text; ++text)
	{
		const char *at;

		if (*text == '\n' || *text == '\r' || *text == ' ' || *text == '\t')
			continue;
		if (*text == '=')
		{
			++padding;
			continue;
		}
		at = padding ? NULL : strchr(alphabet, *text);
		if (!at || *text == 0)
			return -1;
		bits = ((bits << 6) | (unsigned long)(at - alphabet)) & 0xFFFFFFUL;
		have += 6;
		if (have >= 8)
		{
			have -= 8;
			if ((size_t)count >= size)
				return -1;
			out[count++] = (unsigned char)((bits >> have) & 0xFF);
		}
	}
	return count;
}

int uc_is_sha256_hex(const char *text)
{
	int i;

	for (i = 0; i < 64; ++i)
		if (!((text[i] >= '0' && text[i] <= '9') || (text[i] >= 'a' && text[i] <= 'f')))
			return 0;
	return text[64] == 0;
}

int uc_signature_ok(const struct uc_item *item, const char *public_key_base64)
{
	unsigned char key[40], sig[72];
	char statement[UC_URL + 400];
	size_t n = uc_statement(item, statement, sizeof(statement));

	if (!public_key_base64 || !n || uc_base64_decode(public_key_base64, key, sizeof(key)) != 32 ||
		uc_base64_decode(item->signature, sig, sizeof(sig)) != 64)
		return 0;
	return crypto_ed25519_check(sig, key, (const uint8_t *)statement, n) == 0;
}

/* ---------- the appcast */

static int dict_string(CFDictionaryRef d, const char *key, char *out, size_t size)
{
	CFStringRef k = CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
	CFTypeRef v = k ? CFDictionaryGetValue(d, k) : NULL;
	int ok = 0;

	if (v && CFGetTypeID(v) == CFStringGetTypeID())
		ok = CFStringGetCString((CFStringRef)v, out, (CFIndex)size, kCFStringEncodingUTF8) != 0;
	if (k)
		CFRelease(k);
	if (!ok && size)
		out[0] = 0;
	return ok;
}

/* an integer, written as a number or as a string of digits */
static int dict_integer(CFDictionaryRef d, const char *key, long *out)
{
	CFStringRef k = CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
	CFTypeRef v = k ? CFDictionaryGetValue(d, k) : NULL;
	int ok = 0;

	if (v && CFGetTypeID(v) == CFNumberGetTypeID())
	{
		SInt64 n = 0;

		ok = CFNumberGetValue((CFNumberRef)v, kCFNumberSInt64Type, &n) && n >= 0 && n <= 0x7FFFFFFFL;
		*out = (long)n;
	}
	else if (v && CFGetTypeID(v) == CFStringGetTypeID())
	{
		char text[24];
		char *end;

		if (CFStringGetCString((CFStringRef)v, text, sizeof(text), kCFStringEncodingUTF8) && text[0])
		{
			*out = strtol(text, &end, 10);
			ok = *end == 0 && *out >= 0;
		}
	}
	if (k)
		CFRelease(k);
	if (!ok)
		*out = 0;
	return ok;
}

/* the notes, cut at a whole character if they are too long, and without blank lines at the end */
static void read_notes(CFDictionaryRef d, char *out, size_t size)
{
	CFStringRef k = CFSTR("notes");
	CFTypeRef v = CFDictionaryGetValue(d, k);
	CFIndex used = 0;
	size_t n;

	out[0] = 0;
	if (!v || CFGetTypeID(v) != CFStringGetTypeID())
		return;
	CFStringGetBytes((CFStringRef)v, CFRangeMake(0, CFStringGetLength((CFStringRef)v)), kCFStringEncodingUTF8, '?', false,
		(UInt8 *)out, (CFIndex)size - 1, &used);
	out[used] = 0;
	n = (size_t)used;
	while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == ' ' || out[n - 1] == '\r' || out[n - 1] == '\t'))
		out[--n] = 0;
}

/* an item from a dictionary: 1 if it is described properly (not yet checked for its signature) */
static int read_item(CFDictionaryRef d, struct uc_item *item)
{
	long length;

	memset(item, 0, sizeof(*item));
	if (!dict_string(d, "version", item->version, sizeof(item->version)) || !item->version[0])
		return 0;
	dict_integer(d, "build", &item->build);
	if (!dict_string(d, "url", item->url, sizeof(item->url)) || strncmp(item->url, "https://", 8) != 0)
		return 0;
	if (!dict_string(d, "sha256", item->sha256, sizeof(item->sha256)) || !uc_is_sha256_hex(item->sha256))
		return 0;
	if ((!dict_integer(d, "length", &length) && !dict_integer(d, "size", &length)) || length <= 0 ||
		length > UC_MAX_LENGTH)
		return 0;
	item->length = length;
	dict_string(d, "minimumSystemVersion", item->min_os, sizeof(item->min_os));
	read_notes(d, item->notes, sizeof(item->notes));
	if (!dict_string(d, "signature", item->signature, sizeof(item->signature)) || !item->signature[0])
		return 0;
	return 1;
}

static int newer_than_current(const struct uc_item *item, const struct uc_current *cur)
{
	int c = uc_version_compare(item->version, cur->version);

	return c > 0 || (c == 0 && item->build > cur->build);
}

static int runs_here(const struct uc_item *item, const struct uc_current *cur)
{
	const char *p = item->min_os;
	long major, minor;

	if (!p[0])
		return 1;
	major = next_part(&p);
	minor = next_part(&p);
	return cur->os_major > major || (cur->os_major == major && cur->os_minor >= minor);
}

static void set_err(char *err, size_t size, const char *text)
{
	if (err && size)
		snprintf(err, size, "%s", text);
}

static void consider(CFDictionaryRef d, const struct uc_current *cur, struct uc_item *best, int *have, int *unsigned_newer)
{
	struct uc_item item;

	if (!read_item(d, &item) || !newer_than_current(&item, cur) || !runs_here(&item, cur))
		return;
	if (!uc_signature_ok(&item, cur->public_key))
	{
		*unsigned_newer = 1;
		return;
	}
	if (!*have || uc_version_compare(item.version, best->version) > 0 ||
		(uc_version_compare(item.version, best->version) == 0 && item.build > best->build))
	{
		*best = item;
		*have = 1;
	}
}

int uc_choose(const void *data, size_t length, const struct uc_current *cur, struct uc_item *out, char *err,
	size_t err_size)
{
	CFDataRef bytes;
	CFPropertyListRef plist;
	CFTypeRef list;
	int have = 0, bad = 0;

	if (!data || length == 0 || length > UC_MAX_APPCAST)
	{
		set_err(err, err_size, "The update feed is empty or too large.");
		return UC_ERROR;
	}
	bytes = CFDataCreate(NULL, (const UInt8 *)data, (CFIndex)length);
	plist = bytes ? CFPropertyListCreateFromXMLData(NULL, bytes, kCFPropertyListImmutable, NULL) : NULL;
	if (bytes)
		CFRelease(bytes);
	if (!plist || CFGetTypeID(plist) != CFDictionaryGetTypeID())
	{
		if (plist)
			CFRelease(plist);
		set_err(err, err_size, "The update feed could not be read.");
		return UC_ERROR;
	}
	list = CFDictionaryGetValue((CFDictionaryRef)plist, CFSTR("versions"));
	if (list && CFGetTypeID(list) == CFArrayGetTypeID())
	{
		CFIndex i;

		for (i = 0; i < CFArrayGetCount((CFArrayRef)list); ++i)
		{
			CFTypeRef one = CFArrayGetValueAtIndex((CFArrayRef)list, i);

			if (one && CFGetTypeID(one) == CFDictionaryGetTypeID())
				consider((CFDictionaryRef)one, cur, out, &have, &bad);
		}
	}
	else
		consider((CFDictionaryRef)plist, cur, out, &have, &bad);
	CFRelease(plist);
	if (have)
		return UC_AVAILABLE;
	if (bad)
	{
		set_err(err, err_size, "The newest release in the update feed is not signed with PowerSpark's release key, "
			"so it was not offered.");
		return UC_ERROR;
	}
	return UC_UP_TO_DATE;
}

/* ---------- SHA-256 */

static const uint32_t K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_block(struct uc_sha256 *s, const unsigned char *p)
{
	uint32_t w[64], a, b, c, d, e, f, g, h, t1, t2;
	int i;

	for (i = 0; i < 16; ++i)
		w[i] = ((uint32_t)p[4 * i] << 24) | ((uint32_t)p[4 * i + 1] << 16) | ((uint32_t)p[4 * i + 2] << 8) |
			(uint32_t)p[4 * i + 3];
	for (i = 16; i < 64; ++i)
	{
		uint32_t s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);

		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}
	a = s->state[0]; b = s->state[1]; c = s->state[2]; d = s->state[3];
	e = s->state[4]; f = s->state[5]; g = s->state[6]; h = s->state[7];
	for (i = 0; i < 64; ++i)
	{
		t1 = h + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + w[i];
		t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
		h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
	}
	s->state[0] += a; s->state[1] += b; s->state[2] += c; s->state[3] += d;
	s->state[4] += e; s->state[5] += f; s->state[6] += g; s->state[7] += h;
}

void uc_sha256_start(struct uc_sha256 *s)
{
	static const uint32_t init[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c,
		0x1f83d9ab, 0x5be0cd19 };

	memcpy(s->state, init, sizeof(init));
	s->bits = 0;
	s->used = 0;
}

void uc_sha256_add(struct uc_sha256 *s, const void *data, size_t length)
{
	const unsigned char *p = (const unsigned char *)data;

	s->bits += (uint64_t)length * 8;
	while (length > 0)
	{
		size_t take = 64 - s->used;

		if (take > length)
			take = length;
		memcpy(s->block + s->used, p, take);
		s->used += (unsigned int)take;
		p += take;
		length -= take;
		if (s->used == 64)
		{
			sha256_block(s, s->block);
			s->used = 0;
		}
	}
}

void uc_sha256_finish(struct uc_sha256 *s, char hex[65])
{
	uint64_t bits = s->bits;
	unsigned char pad[72];
	size_t padding = (s->used < 56 ? 56 : 120) - s->used;
	int i;

	memset(pad, 0, sizeof(pad));
	pad[0] = 0x80;
	uc_sha256_add(s, pad, padding);
	for (i = 0; i < 8; ++i)
		pad[i] = (unsigned char)(bits >> (56 - 8 * i));
	uc_sha256_add(s, pad, 8);
	for (i = 0; i < 8; ++i)
		snprintf(hex + 8 * i, 9, "%08x", (unsigned int)s->state[i]);
}

int uc_sha256_file(const char *path, long *size, char hex[65])
{
	struct uc_sha256 s;
	unsigned char buffer[16384];
	FILE *f = fopen(path, "rb");
	size_t got;
	long total = 0;

	if (!f)
		return 0;
	uc_sha256_start(&s);
	while ((got = fread(buffer, 1, sizeof(buffer), f)) > 0)
	{
		uc_sha256_add(&s, buffer, got);
		total += (long)got;
	}
	if (ferror(f))
	{
		fclose(f);
		return 0;
	}
	fclose(f);
	uc_sha256_finish(&s, hex);
	if (size)
		*size = total;
	return 1;
}

/* ---------- Mach-O files */

static unsigned long be32(const unsigned char *p)
{
	return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | ((unsigned long)p[2] << 8) | p[3];
}

static unsigned long le32(const unsigned char *p)
{
	return ((unsigned long)p[3] << 24) | ((unsigned long)p[2] << 16) | ((unsigned long)p[1] << 8) | p[0];
}

int uc_macho_has_cpu(const char *path, int cpu)
{
	unsigned char head[8], entry[20];
	FILE *f = fopen(path, "rb");
	int found = 0;

	if (!f)
		return 0;
	if (fread(head, 1, 8, f) == 8)
	{
		unsigned long magic = be32(head);

		if (magic == 0xCAFEBABEUL)
		{
			unsigned long count = be32(head + 4), i;

			for (i = 0; i < count && i < 16 && fread(entry, 1, 20, f) == 20; ++i)
				if ((long)be32(entry) == cpu)
					found = 1;
		}
		else if (magic == 0xFEEDFACEUL)
			found = (long)be32(head + 4) == cpu;	/* a thin big-endian file: ppc */
		else if (le32(head) == 0xFEEDFACEUL)
			found = (long)le32(head + 4) == cpu;	/* a thin little-endian file: i386 */
	}
	fclose(f);
	return found;
}

int uc_this_cpu(void)
{
#if defined(__ppc__) || defined(__POWERPC__)
	return UC_CPU_PPC;
#else
	return UC_CPU_I386;
#endif
}

/* ---------- the unpacked bundle */

static void err_printf(char *err, size_t size, const char *format, const char *a)
{
	if (err && size)
		snprintf(err, size, format, a);
}

int uc_find_app(const char *folder, char *out, size_t size, char *err, size_t err_size)
{
	DIR *d = opendir(folder);
	struct dirent *e;
	int apps = 0, others = 0;

	if (!d)
	{
		set_err(err, err_size, "The unpacked update could not be opened.");
		return -1;
	}
	if (size)
		out[0] = 0;
	while ((e = readdir(d)) != NULL)
	{
		size_t n = strlen(e->d_name);

		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..") || !strcmp(e->d_name, ".DS_Store") ||
			!strcmp(e->d_name, "__MACOSX"))
			continue;
		if (n > 4 && !strcmp(e->d_name + n - 4, ".app") && snprintf(out, size, "%s/%s", folder, e->d_name) < (int)size)
			++apps;
		else
			++others;
	}
	closedir(d);
	if (apps != 1 || others != 0)
	{
		set_err(err, err_size, "The update does not hold a single PowerSpark application.");
		if (size)
			out[0] = 0;
		return -1;
	}
	return 0;
}

static int plist_of(const char *app, CFDictionaryRef *dict)
{
	char path[PATH_MAX];
	FILE *f;
	long length;
	unsigned char *bytes;
	CFDataRef data;
	CFPropertyListRef plist;

	snprintf(path, sizeof(path), "%s/Contents/Info.plist", app);
	f = fopen(path, "rb");
	if (!f)
		return 0;
	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (length <= 0 || length > 65536 || (bytes = (unsigned char *)malloc((size_t)length)) == NULL)
	{
		fclose(f);
		return 0;
	}
	if (fread(bytes, 1, (size_t)length, f) != (size_t)length)
	{
		fclose(f);
		free(bytes);
		return 0;
	}
	fclose(f);
	data = CFDataCreate(NULL, bytes, length);
	free(bytes);
	plist = data ? CFPropertyListCreateFromXMLData(NULL, data, kCFPropertyListImmutable, NULL) : NULL;
	if (data)
		CFRelease(data);
	if (!plist || CFGetTypeID(plist) != CFDictionaryGetTypeID())
	{
		if (plist)
			CFRelease(plist);
		return 0;
	}
	*dict = (CFDictionaryRef)plist;
	return 1;
}

int uc_validate_bundle(const char *app, const char *bundle_id, const char *version, long build, int cpu, char *err,
	size_t err_size)
{
	CFDictionaryRef info = NULL;
	char id[128], v[64], path[PATH_MAX];
	long b = 0;
	struct stat st;
	static const char *const programs[] = { "PowerSpark", "PowerSpark-game" };
	int i;

	if (!plist_of(app, &info))
	{
		set_err(err, err_size, "The update has no readable Info.plist.");
		return -1;
	}
	if (!dict_string(info, "CFBundleIdentifier", id, sizeof(id)) || strcmp(id, bundle_id) != 0)
	{
		CFRelease(info);
		set_err(err, err_size, "The update is not PowerSpark (its bundle identifier is not PowerSpark's).");
		return -1;
	}
	dict_string(info, "CFBundleShortVersionString", v, sizeof(v));
	dict_integer(info, "CFBundleVersion", &b);
	CFRelease(info);
	if (version && version[0] && strcmp(v, version) != 0)
	{
		err_printf(err, err_size, "The update is version %s, not the one that was offered.", v);
		return -1;
	}
	if (build > 0 && b != build)
	{
		set_err(err, err_size, "The update's build is not the one that was offered.");
		return -1;
	}
	for (i = 0; i < 2; ++i)
	{
		snprintf(path, sizeof(path), "%s/Contents/MacOS/%s", app, programs[i]);
		if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) || (st.st_mode & 0111) == 0)
		{
			err_printf(err, err_size, "The update lacks the program %s.", programs[i]);
			return -1;
		}
		if (!uc_macho_has_cpu(path, cpu))
		{
			err_printf(err, err_size, "The update's %s is not built for this Mac's processor.", programs[i]);
			return -1;
		}
	}
	return 0;
}

/* ---------- putting it in place */

int uc_place_check(const char *app, char *err, size_t err_size)
{
	char parent[PATH_MAX], *slash;
	struct statfs fs;

	snprintf(parent, sizeof(parent), "%s", app);
	slash = strrchr(parent, '/');
	if (slash && slash != parent)
		*slash = 0;
	else if (slash)
		slash[1] = 0;
	if (statfs(parent, &fs) == 0 && (fs.f_flags & MNT_RDONLY))
	{
		set_err(err, err_size, "PowerSpark is on a disk that cannot be changed (most likely the disk image it came "
			"on). Drag PowerSpark to your Applications folder, open it from there, and look for updates again.");
		return UC_PLACE_READ_ONLY;
	}
	if (access(parent, W_OK) != 0)
	{
		char text[PATH_MAX + 300];

		snprintf(text, sizeof(text), "PowerSpark cannot replace itself in %s, because your account may not change "
			"that folder. Move PowerSpark to your Applications folder or your Desktop (an administrator can do it "
			"for you), open it from there, and look for updates again. Nothing was changed.", parent);
		set_err(err, err_size, text);
		return UC_PLACE_NOT_WRITABLE;
	}
	return UC_PLACE_OK;
}

int uc_remove_tree(const char *path)
{
	struct stat st;

	if (lstat(path, &st) != 0)
		return errno == ENOENT ? 0 : -1;
	if (S_ISDIR(st.st_mode))
	{
		DIR *d = opendir(path);
		struct dirent *e;

		if (!d)
			return -1;
		while ((e = readdir(d)) != NULL)
		{
			char child[PATH_MAX];

			if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
				continue;
			if (snprintf(child, sizeof(child), "%s/%s", path, e->d_name) >= (int)sizeof(child))
				continue;
			uc_remove_tree(child);
		}
		closedir(d);
		return rmdir(path);
	}
	return unlink(path);
}

int uc_swap(const char *new_app, const char *app, char *old_out, size_t old_size, char *err, size_t err_size)
{
	char old[PATH_MAX], text[PATH_MAX + 200];

	if (snprintf(old, sizeof(old), "%s.old", app) >= (int)sizeof(old))
	{
		set_err(err, err_size, "The path of PowerSpark is too long.");
		return -1;
	}
	if (old_out && old_size)
		snprintf(old_out, old_size, "%s", old);
	/* (a leftover .old is only ever ours: uc_clean_leftovers refuses others, so this one is cleared first) */
	if (uc_remove_tree(old) != 0)
	{
		snprintf(text, sizeof(text), "%s is in the way and could not be removed.", old);
		set_err(err, err_size, text);
		return -1;
	}
	if (rename(app, old) != 0)
	{
		snprintf(text, sizeof(text), "The running PowerSpark could not be moved aside (%s).", strerror(errno));
		set_err(err, err_size, text);
		return -1;
	}
	if (rename(new_app, app) != 0)
	{
		int why = errno;

		rename(old, app);
		snprintf(text, sizeof(text), "The new PowerSpark could not be put in place (%s). The old one was kept.",
			strerror(why));
		set_err(err, err_size, text);
		return -1;
	}
	return 0;
}

static int is_bundle_of(const char *app, const char *bundle_id)
{
	CFDictionaryRef info = NULL;
	char id[128];
	int ok = 0;

	if (plist_of(app, &info))
	{
		ok = dict_string(info, "CFBundleIdentifier", id, sizeof(id)) && strcmp(id, bundle_id) == 0;
		CFRelease(info);
	}
	return ok;
}

void uc_clean_leftovers(const char *app, const char *bundle_id)
{
	char old[PATH_MAX], parent[PATH_MAX], *slash;
	DIR *d;
	struct dirent *e;
	time_t now = time(NULL);

	if (snprintf(old, sizeof(old), "%s.old", app) < (int)sizeof(old) && is_bundle_of(old, bundle_id))
		uc_remove_tree(old);
	snprintf(parent, sizeof(parent), "%s", app);
	slash = strrchr(parent, '/');
	if (!slash || slash == parent)
		return;
	*slash = 0;
	d = opendir(parent);
	if (!d)
		return;
	while ((e = readdir(d)) != NULL)
	{
		char path[PATH_MAX];
		struct stat st;

		if (strncmp(e->d_name, UC_WORK_PREFIX, strlen(UC_WORK_PREFIX)) != 0 ||
			snprintf(path, sizeof(path), "%s/%s", parent, e->d_name) >= (int)sizeof(path))
			continue;
		if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode) && now - st.st_mtime > 3600)
			uc_remove_tree(path);
	}
	closedir(d);
}

/* ---------- when to look */

int uc_check_due(const char *path, long now, long interval)
{
	FILE *f = fopen(path, "rb");
	long last = 0;
	char text[32];

	if (!f)
		return 1;
	if (fgets(text, sizeof(text), f))
		last = atol(text);
	fclose(f);
	/* (a stamp from the future is a clock that was set wrong: look) */
	return last <= 0 || last > now || now - last >= interval;
}

int uc_check_stamp(const char *path, long now)
{
	FILE *f = fopen(path, "wb");

	if (!f)
		return -1;
	fprintf(f, "%ld\n", now);
	return fclose(f);
}
