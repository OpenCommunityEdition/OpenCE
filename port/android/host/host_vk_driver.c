/*
HOST_VK_DRIVER.C

The driver module (host_vk_driver.h): opens the
Vulkan driver and hands out its vkGetInstanceProcAddr.

display.vk_driver empty: the phone's own driver, libvulkan.so through the
system loader. Otherwise the name of an adrenotools archive (a zip with a
meta.json and the driver's library) in the app's external files folder. Android
loads a library only from the app's private storage, so the archive is
unpacked into <internal storage>/vk_driver/<archive name>/ (again only when the
archive's size or modification time differs from what was unpacked), its
meta.json names the library, and libadrenotools opens the system loader with
that library in place of the phone's driver.

Anything that goes wrong with an archive before libadrenotools accepts it is
logged and the phone's driver is opened instead; the description says so. What
can go wrong after that, at vkCreateInstance, cannot be undone here: see
host_vk_driver.h, and host_vk_driver_verify().

Not thread safe. One driver per process.
*/

#include "host_vk_driver.h"
#include "host.h"

#include <SDL3/SDL.h>
#include <dlfcn.h>
#include <errno.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#define ADRENOTOOLS_DRIVER_CUSTOM (1 << 0) /* adrenotools/priv.h */

typedef void *(*open_libvulkan_function)(int dlopen_mode, int feature_flags, const char *tmp_lib_dir,
	const char *hook_lib_dir, const char *custom_driver_dir, const char *custom_driver_name,
	const char *file_redirect_dir, void **user_mapping_handle);

/* an archive and the files in it are bounded, so that a broken or hostile one
cannot take the app's memory or storage */
#define ARCHIVE_MAXIMUM (256u * 1024u * 1024u)
#define ENTRY_MAXIMUM (256u * 1024u * 1024u)
#define ENTRY_COUNT_MAXIMUM 256
#define META_MAXIMUM (64u * 1024u)

static void *opened_library;
static PFN_vkGetInstanceProcAddr opened_function;
static char opened_description[512];
/* the file name of the archive's library while a custom driver is open, else empty */
static char custom_file[256];
/* what the player calls that driver (meta.json's name, else the archive's), for the main menu: host_vk_driver_name */
static char custom_name[160];
/* host_vk_driver_close() was called: no driver may be opened again in this process */
static int closed;

/* ---------- the zip reader: central directory, stored and deflated entries */

struct zip
{
	const uint8_t *data;
	size_t size;
	size_t directory; /* offset of the central directory */
	size_t entries;
};

static uint32_t read16(const uint8_t *p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}

static uint32_t read32(const uint8_t *p)
{
	return read16(p) | read16(p + 2) << 16;
}

static int zip_open(struct zip *zip, const uint8_t *data, size_t size, char *error, size_t error_size)
{
	size_t end, lowest;

	memset(zip, 0, sizeof(*zip));
	if (size < 22)
	{
		snprintf(error, error_size, "not a zip archive (%zu bytes)", size);
		return 0;
	}
	/* the end record is the last 22 bytes unless a comment follows it */
	lowest = size > 22 + 65535 ? size - 22 - 65535 : 0;
	for (end = size - 22;; end--)
	{
		if (read32(data + end) == 0x06054b50u && end + 22 + read16(data + end + 20) == size)
			break;
		if (end == lowest)
		{
			snprintf(error, error_size, "not a zip archive (no end record)");
			return 0;
		}
	}
	if (read16(data + end + 4) != 0 || read16(data + end + 6) != 0 ||
		read16(data + end + 8) != read16(data + end + 10))
	{
		snprintf(error, error_size, "a zip archive in several parts is not supported");
		return 0;
	}
	zip->data = data;
	zip->size = size;
	zip->entries = read16(data + end + 10);
	zip->directory = read32(data + end + 16);
	if (zip->entries == 0xffff || read32(data + end + 12) == 0xffffffffu || zip->directory == 0xffffffffu)
	{
		snprintf(error, error_size, "a zip64 archive is not supported");
		return 0;
	}
	if (zip->directory > end || read32(data + end + 12) > end - zip->directory)
	{
		snprintf(error, error_size, "the zip's central directory is outside the file");
		return 0;
	}
	if (zip->entries > ENTRY_COUNT_MAXIMUM)
	{
		snprintf(error, error_size, "the zip holds %zu entries (at most %d)", zip->entries, ENTRY_COUNT_MAXIMUM);
		return 0;
	}
	return 1;
}

struct zip_entry
{
	char name[256];
	uint32_t method, flags, crc, compressed, size;
	size_t local; /* offset of the local header */
};

/* reads the entry at *offset of the central directory and moves *offset past it */
static int zip_next(const struct zip *zip, size_t *offset, struct zip_entry *entry, char *error, size_t error_size)
{
	const uint8_t *p = zip->data + *offset;
	size_t name_length, extra_length, comment_length;

	if (*offset > zip->size || zip->size - *offset < 46 || read32(p) != 0x02014b50u)
	{
		snprintf(error, error_size, "the zip's central directory is damaged");
		return 0;
	}
	name_length = read16(p + 28);
	extra_length = read16(p + 30);
	comment_length = read16(p + 32);
	if (zip->size - *offset - 46 < name_length + extra_length + comment_length)
	{
		snprintf(error, error_size, "the zip's central directory is damaged");
		return 0;
	}
	if (name_length >= sizeof(entry->name))
	{
		snprintf(error, error_size, "a file name in the zip is too long");
		return 0;
	}
	memcpy(entry->name, p + 46, name_length);
	entry->name[name_length] = 0;
	entry->flags = read16(p + 8);
	entry->method = read16(p + 10);
	entry->crc = read32(p + 16);
	entry->compressed = read32(p + 20);
	entry->size = read32(p + 24);
	entry->local = read32(p + 42);
	*offset += 46 + name_length + extra_length + comment_length;
	return 1;
}

/* the entry's data, in memory made with malloc (one byte more than size, ending in 0), or NULL */
static uint8_t *zip_extract(const struct zip *zip, const struct zip_entry *entry, char *error, size_t error_size)
{
	const uint8_t *local, *packed;
	size_t data_offset;
	uint8_t *out;

	if (entry->flags & 1)
	{
		snprintf(error, error_size, "%s is encrypted", entry->name);
		return NULL;
	}
	if (entry->size == 0xffffffffu || entry->compressed == 0xffffffffu)
	{
		snprintf(error, error_size, "%s needs zip64", entry->name);
		return NULL;
	}
	if (entry->size > ENTRY_MAXIMUM)
	{
		snprintf(error, error_size, "%s is too large (%u bytes)", entry->name, entry->size);
		return NULL;
	}
	if (entry->local > zip->size || zip->size - entry->local < 30 || read32(zip->data + entry->local) != 0x04034b50u)
	{
		snprintf(error, error_size, "%s: the zip's local header is damaged", entry->name);
		return NULL;
	}
	local = zip->data + entry->local;
	data_offset = entry->local + 30 + read16(local + 26) + read16(local + 28);
	if (data_offset > zip->size || zip->size - data_offset < entry->compressed)
	{
		snprintf(error, error_size, "%s: its data is outside the file", entry->name);
		return NULL;
	}
	packed = zip->data + data_offset;
	out = malloc((size_t)entry->size + 1);
	if (!out)
	{
		snprintf(error, error_size, "out of memory for %s", entry->name);
		return NULL;
	}
	if (entry->method == 0)
	{
		if (entry->compressed != entry->size)
		{
			snprintf(error, error_size, "%s: stored with two different sizes", entry->name);
			free(out);
			return NULL;
		}
		memcpy(out, packed, entry->size);
	}
	else if (entry->method == 8)
	{
		z_stream stream;
		int result;

		memset(&stream, 0, sizeof(stream));
		if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
		{
			snprintf(error, error_size, "cannot start inflating %s", entry->name);
			free(out);
			return NULL;
		}
		stream.next_in = (Bytef *)packed;
		stream.avail_in = entry->compressed;
		stream.next_out = out;
		stream.avail_out = entry->size;
		result = entry->size ? inflate(&stream, Z_FINISH) : Z_STREAM_END;
		/* the data must end exactly where the directory says it does */
		if (result != Z_STREAM_END || stream.total_out != entry->size)
		{
			snprintf(error, error_size, "%s does not inflate (zlib %d)", entry->name, result);
			inflateEnd(&stream);
			free(out);
			return NULL;
		}
		inflateEnd(&stream);
	}
	else
	{
		snprintf(error, error_size, "%s uses compression method %u", entry->name, entry->method);
		free(out);
		return NULL;
	}
	if ((uint32_t)crc32(0, out, entry->size) != entry->crc)
	{
		snprintf(error, error_size, "%s: its checksum is wrong", entry->name);
		free(out);
		return NULL;
	}
	out[entry->size] = 0;
	return out;
}

/* a name that stays inside the folder it is unpacked into */
static int name_is_safe(const char *name)
{
	return name[0] && name[0] != '/' && !strstr(name, "..");
}

/* ---------- files */

static int make_directories(const char *path)
{
	char copy[1024];
	char *p;

	if (snprintf(copy, sizeof(copy), "%s", path) >= (int)sizeof(copy))
		return 0;
	for (p = copy + 1; *p; p++)
	{
		if (*p == '/')
		{
			*p = 0;
			if (mkdir(copy, 0700) != 0 && errno != EEXIST)
				return 0;
			*p = '/';
		}
	}
	return mkdir(copy, 0700) == 0 || errno == EEXIST;
}

static int write_file(const char *path, const void *data, size_t size)
{
	FILE *file = fopen(path, "wb");
	int ok;

	if (!file)
		return 0;
	ok = fwrite(data, 1, size, file) == size;
	if (fclose(file) != 0)
		ok = 0;
	if (!ok)
		unlink(path);
	return ok;
}

static uint8_t *read_whole_file(const char *path, size_t limit, size_t *size_out, char *error, size_t error_size)
{
	FILE *file = fopen(path, "rb");
	long length;
	uint8_t *data;

	if (!file)
	{
		snprintf(error, error_size, "cannot open %s: %s", path, strerror(errno));
		return NULL;
	}
	if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0)
	{
		snprintf(error, error_size, "cannot read %s", path);
		fclose(file);
		return NULL;
	}
	if ((unsigned long)length > limit)
	{
		snprintf(error, error_size, "%s is too large (%ld bytes)", path, length);
		fclose(file);
		return NULL;
	}
	data = malloc((size_t)length + 1);
	if (!data || fread(data, 1, (size_t)length, file) != (size_t)length)
	{
		snprintf(error, error_size, "cannot read %s", path);
		free(data);
		fclose(file);
		return NULL;
	}
	fclose(file);
	data[length] = 0;
	*size_out = (size_t)length;
	return data;
}

/* ---------- unpacking */

/* what the unpacked folder was made from, written last */
#define STAMP_NAME ".unpacked"

static void stamp_text(const struct stat *archive, char *text, size_t size)
{
	snprintf(text, size, "%lld %lld\n", (long long)archive->st_size, (long long)archive->st_mtim.tv_sec);
}

static int unpacked_is_current(const char *directory, const struct stat *archive)
{
	char path[1024], wanted[64], found[64];
	FILE *file;
	size_t length;

	snprintf(path, sizeof(path), "%s" STAMP_NAME, directory);
	file = fopen(path, "rb");
	if (!file)
		return 0;
	length = fread(found, 1, sizeof(found) - 1, file);
	fclose(file);
	found[length] = 0;
	stamp_text(archive, wanted, sizeof(wanted));
	return !strcmp(found, wanted);
}

/* directory ends in '/' */
static int unpack_archive(const char *archive_path, const struct stat *archive_stat, const char *directory,
	char *error, size_t error_size)
{
	size_t size = 0, offset, index;
	uint8_t *data;
	struct zip zip;
	char path[1024], stamp[64];
	int ok = 0;

	data = read_whole_file(archive_path, ARCHIVE_MAXIMUM, &size, error, error_size);
	if (!data)
		return 0;
	if (!zip_open(&zip, data, size, error, error_size))
	{
		free(data);
		return 0;
	}
	if (!make_directories(directory))
	{
		snprintf(error, error_size, "cannot make %s: %s", directory, strerror(errno));
		free(data);
		return 0;
	}
	/* an unpacking that dies halfway is redone: no stamp, no trust */
	snprintf(path, sizeof(path), "%s" STAMP_NAME, directory);
	unlink(path);
	offset = zip.directory;
	for (index = 0; index < zip.entries; index++)
	{
		struct zip_entry entry;
		uint8_t *contents;
		size_t length;

		if (!zip_next(&zip, &offset, &entry, error, error_size))
			goto done;
		length = strlen(entry.name);
		if (!name_is_safe(entry.name))
		{
			snprintf(error, error_size, "the zip holds a file named \"%s\", which leaves its folder", entry.name);
			goto done;
		}
		if (entry.name[length - 1] == '/')
			continue; /* a directory; the files below it make it */
		contents = zip_extract(&zip, &entry, error, error_size);
		if (!contents)
			goto done;
		if (snprintf(path, sizeof(path), "%s%s", directory, entry.name) >= (int)sizeof(path))
		{
			snprintf(error, error_size, "the path of %s is too long", entry.name);
			free(contents);
			goto done;
		}
		{
			char *slash = strrchr(path, '/');

			*slash = 0;
			if (!make_directories(path))
			{
				snprintf(error, error_size, "cannot make %s: %s", path, strerror(errno));
				free(contents);
				goto done;
			}
			*slash = '/';
		}
		if (!write_file(path, contents, entry.size))
		{
			snprintf(error, error_size, "cannot write %s: %s", path, strerror(errno));
			free(contents);
			goto done;
		}
		free(contents);
	}
	stamp_text(archive_stat, stamp, sizeof(stamp));
	snprintf(path, sizeof(path), "%s" STAMP_NAME, directory);
	if (!write_file(path, stamp, strlen(stamp)))
	{
		snprintf(error, error_size, "cannot write %s: %s", path, strerror(errno));
		goto done;
	}
	ok = 1;
done:
	free(data);
	return ok;
}

/* ---------- meta.json */

/* the string value of "key" at the top level of a flat JSON object, copied
into out; 0 when it is not there. Only what the adrenotools archives' meta.json
uses is understood: strings with the usual escapes; a \uXXXX escape outside
ASCII is replaced by '?'. */
static int json_string(const char *json, const char *key, char *out, size_t size)
{
	char pattern[64];
	const char *p = json;
	size_t length = 0;

	snprintf(pattern, sizeof(pattern), "\"%s\"", key);
	while ((p = strstr(p, pattern)) != NULL)
	{
		const char *q = p + strlen(pattern);

		p = q;
		while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')
			q++;
		if (*q != ':')
			continue; /* the text of some other string's value */
		q++;
		while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')
			q++;
		if (*q != '"')
			return 0;
		for (q++; *q && *q != '"'; q++)
		{
			char c = *q;

			if (c == '\\' && q[1])
			{
				q++;
				switch (*q)
				{
					case 'n': c = '\n'; break;
					case 't': c = '\t'; break;
					case 'r': c = '\r'; break;
					case 'u':
					{
						unsigned value = 0;
						int digit;

						for (digit = 1; digit <= 4 && q[digit]; digit++)
						{
							char h = q[digit];

							value = value * 16 + (h >= 'a' ? h - 'a' + 10 : h >= 'A' ? h - 'A' + 10 : h - '0');
						}
						q += digit - 1;
						c = value < 0x80 && value >= 0x20 ? (char)value : '?';
						break;
					}
					default: c = *q; break;
				}
			}
			if (length + 1 < size)
				out[length++] = c;
		}
		if (!*q)
			return 0;
		out[length] = 0;
		return 1;
	}
	return 0;
}

/* ---------- opening */

/* the directory libmain.so itself was loaded from: with the APK packaged
with legacy packaging that is nativeLibraryDir, where libadrenotools must find
its hooks */
static int native_library_directory(char *out, size_t size)
{
	Dl_info info;
	const char *slash;

	memset(&info, 0, sizeof(info));
	if (!dladdr((void *)host_vk_driver_open, &info) || !info.dli_fname)
		return 0;
	slash = strrchr(info.dli_fname, '/');
	if (!slash || (size_t)(slash - info.dli_fname) + 1 >= size)
		return 0;
	snprintf(out, size, "%.*s", (int)(slash - info.dli_fname), info.dli_fname);
	return 1;
}

/* opens a driver archive; on success *library is the loader's handle and
description says what was opened, on failure error says why */
static int open_archive(const char *setting, void **library, char *description, size_t description_size,
	char *error, size_t error_size)
{
	const char *external = SDL_GetAndroidExternalStoragePath();
	const char *internal = SDL_GetAndroidInternalStoragePath();
	char archive_path[1024], directory[1024], native[1024], path[1024];
	char library_name[256], name[160], version[160];
	struct stat archive_stat;
	uint8_t *meta = NULL;
	size_t meta_size = 0;
	void *adrenotools, *handle;
	open_libvulkan_function open_libvulkan;

	if (!name_is_safe(setting) || strchr(setting, '/'))
	{
		snprintf(error, error_size, "\"%s\" is not the name of a file in the data folder", setting);
		return 0;
	}
	if (!external || !internal)
	{
		snprintf(error, error_size, "Android's storage is unavailable: %s", SDL_GetError());
		return 0;
	}
	snprintf(archive_path, sizeof(archive_path), "%s/%s", external, setting);
	if (stat(archive_path, &archive_stat) != 0)
	{
		snprintf(error, error_size, "%s: %s", archive_path, strerror(errno));
		return 0;
	}
	if (!S_ISREG(archive_stat.st_mode))
	{
		snprintf(error, error_size, "%s is not a file", archive_path);
		return 0;
	}
	snprintf(directory, sizeof(directory), "%s/vk_driver/%s/", internal, setting);
	if (!unpacked_is_current(directory, &archive_stat))
	{
		uint64_t start = SDL_GetTicks();

		if (!unpack_archive(archive_path, &archive_stat, directory, error, error_size))
			return 0;
		host_logf(HOST_LOG_INFO, "vk driver: unpacked %s into %s in %llu ms", setting, directory,
			(unsigned long long)(SDL_GetTicks() - start));
	}
	else
		host_logf(HOST_LOG_INFO, "vk driver: %s is already unpacked in %s", setting, directory);

	snprintf(path, sizeof(path), "%smeta.json", directory);
	meta = read_whole_file(path, META_MAXIMUM, &meta_size, error, error_size);
	if (!meta)
		return 0;
	if (!json_string((const char *)meta, "libraryName", library_name, sizeof(library_name)))
	{
		snprintf(error, error_size, "meta.json has no libraryName");
		free(meta);
		return 0;
	}
	name[0] = version[0] = 0;
	json_string((const char *)meta, "name", name, sizeof(name));
	json_string((const char *)meta, "driverVersion", version, sizeof(version));
	free(meta);
	if (!name_is_safe(library_name) || strchr(library_name, '/'))
	{
		snprintf(error, error_size, "meta.json names the library \"%s\"", library_name);
		return 0;
	}
	snprintf(path, sizeof(path), "%s%s", directory, library_name);
	{
		/* libadrenotools hands back the system loader whatever the driver is, and its hook quietly falls back to the
		phone's driver when the library will not load, so a file that cannot be one is refused here */
		FILE *file = fopen(path, "rb");
		unsigned char magic[4] = { 0, 0, 0, 0 };

		if (!file)
		{
			snprintf(error, error_size, "the archive has no %s: %s", library_name, strerror(errno));
			return 0;
		}
		if (fread(magic, 1, 4, file) != 4 || memcmp(magic, "\x7f" "ELF", 4) != 0)
		{
			fclose(file);
			snprintf(error, error_size, "%s is not an ELF library", library_name);
			return 0;
		}
		fclose(file);
	}

	if (!native_library_directory(native, sizeof(native)))
	{
		snprintf(error, error_size, "cannot find the native library folder");
		return 0;
	}
	snprintf(path, sizeof(path), "%s/libadrenotools.so", native);
	adrenotools = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (!adrenotools)
	{
		snprintf(error, error_size, "cannot load libadrenotools.so: %s", dlerror());
		return 0;
	}
	open_libvulkan = (open_libvulkan_function)dlsym(adrenotools, "adrenotools_open_libvulkan");
	if (!open_libvulkan)
	{
		snprintf(error, error_size, "libadrenotools.so has no adrenotools_open_libvulkan");
		dlclose(adrenotools);
		return 0;
	}
	/* the driver's folder ends in '/' because the library joins it to the name with nothing between;
	the hooks are in the native library folder or every driver is refused, the phone's included */
	host_logf(HOST_LOG_INFO, "vk driver: opening %s from %s (hooks in %s)", library_name, directory, native);
	handle = open_libvulkan(RTLD_NOW | RTLD_LOCAL, ADRENOTOOLS_DRIVER_CUSTOM, NULL, native, directory,
		library_name, NULL, NULL);
	if (!handle)
	{
		snprintf(error, error_size, "libadrenotools refused %s (logcat tag adrenotools says why)", library_name);
		dlclose(adrenotools);
		return 0;
	}
	/* libadrenotools stays loaded: its hooks are installed in the process */
	*library = handle;
	snprintf(custom_file, sizeof(custom_file), "%s", library_name);
	if (name[0])
		snprintf(custom_name, sizeof(custom_name), "%s", name);
	else
	{
		size_t length = strlen(setting);

		if (length > 4 && !strcmp(setting + length - 4, ".zip"))
			length -= 4;
		snprintf(custom_name, sizeof(custom_name), "%.*s", (int)length, setting);
	}
	/* the driver itself is loaded later, by the system loader at vkCreateInstance, through the hook:
	host_vk_driver_verify says whether that happened */
	snprintf(description, description_size, "%s%s%s%s%s (asked for %s through libadrenotools)", setting,
		name[0] ? ", " : "", name, version[0] ? ", " : "", version, library_name);
	return 1;
}

/* display.vk_driver = "auto": the archive vk_driver_auto.txt names, if it is in the external files folder; else empty */
static void auto_resolve(char *out, size_t size)
{
	const char *external = SDL_GetAndroidExternalStoragePath();
	char path[1024], line[256] = "";
	struct stat archive;
	FILE *file;

	out[0] = 0;
	if (!external)
		return;
	snprintf(path, sizeof(path), "%s/vk_driver_auto.txt", external);
	file = fopen(path, "r");
	if (file)
	{
		if (!fgets(line, sizeof(line), file))
			line[0] = 0;
		fclose(file);
	}
	line[strcspn(line, "\r\n")] = 0;
	if (line[0])
	{
		snprintf(path, sizeof(path), "%s/%s", external, line);
		if (name_is_safe(line) && !strchr(line, '/') && !stat(path, &archive))
			snprintf(out, size, "%s", line);
	}
	if (out[0])
		host_logf(HOST_LOG_INFO, "vk driver: auto: %s", out);
	else if (!file)
		host_logf(HOST_LOG_INFO, "vk driver: auto: the phone's own (the launcher has not chosen one yet)");
	else if (line[0])
		host_logf(HOST_LOG_INFO, "vk driver: auto: the phone's own (%s is not in the data folder)", line);
	else
		host_logf(HOST_LOG_INFO, "vk driver: auto: the phone's own (no Turnip build for this GPU)");
}

PFN_vkGetInstanceProcAddr host_vk_driver_open(const char *setting, char *description, size_t size)
{
	char error[512] = "", custom_description[384] = "", auto_setting[256];
	void *library = NULL;
	PFN_vkGetInstanceProcAddr function;

	if (opened_function)
	{
		snprintf(description, size, "%s", opened_description);
		return opened_function;
	}
	if (closed)
	{
		snprintf(description, size, "the Vulkan driver was closed; it cannot be opened again in this process");
		host_logf(HOST_LOG_ERROR, "vk driver: %s", description);
		return NULL;
	}
	/* "auto": the archive the launcher chose for this phone's GPU (LauncherActivity, Updater.java), named in
	vk_driver_auto.txt; the phone's own driver when it names none or the archive is not there */
	if (setting && !strcmp(setting, "auto"))
	{
		auto_resolve(auto_setting, sizeof(auto_setting));
		setting = auto_setting;
	}
	if (setting && setting[0])
	{
		if (open_archive(setting, &library, custom_description, sizeof(custom_description), error, sizeof(error)))
		{
			function = (PFN_vkGetInstanceProcAddr)dlsym(library, "vkGetInstanceProcAddr");
			if (function)
			{
				opened_library = library;
				opened_function = function;
				snprintf(opened_description, sizeof(opened_description), "%s", custom_description);
				snprintf(description, size, "%s", opened_description);
				host_logf(HOST_LOG_INFO, "vk driver: %s", opened_description);
				return function;
			}
			snprintf(error, sizeof(error), "the loader has no vkGetInstanceProcAddr: %s", dlerror());
			/* the handle is the loader adrenotools made; closing it leaves the hooks installed
			for nothing, which is harmless, and the phone's own driver is opened below */
			dlclose(library);
			custom_file[0] = 0;
			custom_name[0] = 0;
		}
		host_logf(HOST_LOG_ERROR, "vk driver: %s failed: %s; using the phone's driver", setting, error);
	}

	library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
	if (!library)
	{
		snprintf(description, size, "%s%sno Vulkan driver: %s", error[0] ? setting : "", error[0] ? " failed; " : "",
			dlerror());
		host_logf(HOST_LOG_ERROR, "vk driver: %s", description);
		return NULL;
	}
	function = (PFN_vkGetInstanceProcAddr)dlsym(library, "vkGetInstanceProcAddr");
	if (!function)
	{
		snprintf(description, size, "libvulkan.so has no vkGetInstanceProcAddr: %s", dlerror());
		host_logf(HOST_LOG_ERROR, "vk driver: %s", description);
		dlclose(library);
		return NULL;
	}
	opened_library = library;
	opened_function = function;
	if (error[0])
		snprintf(opened_description, sizeof(opened_description), "%s failed: %s; using the phone's driver", setting,
			error);
	else
		snprintf(opened_description, sizeof(opened_description), "the phone's driver (libvulkan.so)");
	snprintf(description, size, "%s", opened_description);
	host_logf(HOST_LOG_INFO, "vk driver: %s", opened_description);
	return function;
}

static int find_custom_library(struct dl_phdr_info *info, size_t size, void *data)
{
	const char *slash;

	(void)size;
	(void)data;
	if (!info->dlpi_name)
		return 0;
	slash = strrchr(info->dlpi_name, '/');
	return slash && !strcmp(slash + 1, custom_file);
}

int host_vk_driver_verify(char *text, size_t size)
{
	if (!opened_function)
	{
		snprintf(text, size, "no driver is open");
		return 0;
	}
	if (!custom_file[0])
	{
		snprintf(text, size, "the phone's driver was asked for");
		return 1;
	}
	if (dl_iterate_phdr(find_custom_library, NULL))
	{
		snprintf(text, size, "%s is loaded in the process", custom_file);
		return 1;
	}
	snprintf(text, size, "%s is not loaded: the hook fell back to the phone's own driver (logcat tag hook_impl says why)",
		custom_file);
	return 0;
}

void host_vk_driver_close(void)
{
	if (opened_library)
		dlclose(opened_library);
	opened_library = NULL;
	opened_function = NULL;
	opened_description[0] = 0;
	custom_file[0] = 0;
	closed = 1;
}

/* the guest asks (the main menu, under the version number): the name of the driver archive in use into its buffer at out, of size
bytes, or an empty string when the phone's own driver is (or no driver is open) */
uint32_t host_vk_driver_name(uint32_t out, uint32_t size)
{
	char *text = (char *)(uintptr_t)out;

	if (!text || !size)
		return 0;
	snprintf(text, size, "%s", opened_function && custom_file[0] ? custom_name : "");
	return (uint32_t)strlen(text);
}
