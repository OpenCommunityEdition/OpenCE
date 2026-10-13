/*
MAP_ARCHIVES_ZIP.C

A map taken out of a map archive's zip (map_archives_unzip, map_archives.h):
the zip's file of the map's name and size, in any folder, stored or
deflated (the port's zlib), its CRC checked. Nothing here trusts the zip:
its end record and directory are read with every offset and size checked
against the file (APPNOTE.TXT 4.3; no Zip64, which no map's zip needs), and
the map is written no larger than the size asked for. And a map checked
against the index's hash of it (map_archives_verify). Apart from
map_archives.c, which fetches the zips, so that tools/map_download_check.c
(its "unzip" and "verify") and Android, which fetches none, have them
alone.
*/

#include "platform.h"
#include "posix.h"
#include "map_archives.h"
#include "map_sha1.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zlib_prefixed.h"

/* the zip's end of central directory record, its directory's entries and
local headers (APPNOTE.TXT 4.3) */
#define ZIP_END_SIGNATURE 0x06054B50UL
#define ZIP_END_SIZE 22
#define ZIP_MAXIMUM_COMMENT 0xFFFF
#define ZIP_ENTRY_SIGNATURE 0x02014B50UL
#define ZIP_ENTRY_SIZE 46
#define ZIP_LOCAL_SIGNATURE 0x04034B50UL
#define ZIP_LOCAL_SIZE 30
#define ZIP_MAXIMUM_DIRECTORY (8 * 1024 * 1024)
#define ZIP_STORED 0
#define ZIP_DEFLATED 8

static int same_name(const char *a, const char *b, size_t length)
{
	size_t index;

	for (index = 0; index < length; index++)
	{
		char ca = a[index], cb = b[index];

		if (ca >= 'A' && ca <= 'Z')
			ca = (char)(ca - 'A' + 'a');
		if (cb >= 'A' && cb <= 'Z')
			cb = (char)(cb - 'A' + 'a');
		if (ca != cb)
			return 0;
		if (!ca)
			return 1;
	}
	return 1;
}

static unsigned long zip_u16(const unsigned char *bytes)
{
	return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8;
}

static unsigned long zip_u32(const unsigned char *bytes)
{
	return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8 | (unsigned long)bytes[2] << 16 |
		(unsigned long)bytes[3] << 24;
}

int map_archives_read_at(int file, unsigned long long offset, void *buffer, unsigned long size)
{
	unsigned char *bytes = buffer;
	posix_ulong low, high;

	if (posix_seek(file, (posix_long)(offset & 0xFFFFFFFFUL), (posix_long)(offset >> 32), SEEK_SET, &low, &high) != 0)
		return 0;
	while (size)
	{
		int count = (int)read(file, bytes, size > 65536 ? 65536u : (unsigned)size);

		if (count <= 0)
			return 0;
		bytes += count;
		size -= (unsigned long)count;
	}
	return 1;
}

static int write_all(int file, const unsigned char *bytes, unsigned long size)
{
	while (size)
	{
		int count = (int)write(file, bytes, size > 65536 ? 65536u : (unsigned)size);

		if (count <= 0)
			return 0;
		bytes += count;
		size -= (unsigned long)count;
	}
	return 1;
}

/* the file's data, stored or deflated, from offset in the zip into output */
static int unzip_data(int zip, unsigned long long offset, unsigned long long zip_size, int method,
	unsigned long long packed_size, unsigned long long size, unsigned long crc, int output, char *error,
	int error_size)
{
	static unsigned char in[65536], out[65536];
	unsigned long long read_so_far = 0, written = 0;
	unsigned long computed = crc32(0L, Z_NULL, 0);
	z_stream stream;
	int status = Z_OK;

	if (offset > zip_size || packed_size > zip_size - offset || (method == ZIP_STORED && packed_size != size))
	{
		snprintf(error, (size_t)error_size, "the zip's file runs past its end");
		return 0;
	}
	memset(&stream, 0, sizeof(stream));
	if (method == ZIP_DEFLATED && inflateInit2(&stream, -MAX_WBITS) != Z_OK)
	{
		snprintf(error, (size_t)error_size, "zlib could not start");
		return 0;
	}
	while (read_so_far < packed_size && status != Z_STREAM_END)
	{
		unsigned long chunk = packed_size - read_so_far > sizeof(in) ? (unsigned long)sizeof(in) :
			(unsigned long)(packed_size - read_so_far);

		if (!map_archives_read_at(zip, offset + read_so_far, in, chunk))
		{
			snprintf(error, (size_t)error_size, "the zip cannot be read");
			break;
		}
		read_so_far += chunk;
		if (method == ZIP_STORED)
		{
			if (!write_all(output, in, chunk))
			{
				snprintf(error, (size_t)error_size, "the map cannot be written");
				break;
			}
			computed = crc32(computed, in, (uInt)chunk);
			written += chunk;
			continue;
		}
		stream.next_in = in;
		stream.avail_in = (uInt)chunk;
		while (stream.avail_in && status != Z_STREAM_END)
		{
			unsigned long count;

			stream.next_out = out;
			stream.avail_out = sizeof(out);
			status = inflate(&stream, Z_NO_FLUSH);
			if (status != Z_OK && status != Z_STREAM_END)
			{
				snprintf(error, (size_t)error_size, "the zip's data is damaged (zlib %d)", status);
				break;
			}
			count = (unsigned long)(sizeof(out) - stream.avail_out);
			if (written + count > size || !write_all(output, out, count))
			{
				snprintf(error, (size_t)error_size, written + count > size ? "the map is larger than the zip says" :
					"the map cannot be written");
				status = Z_DATA_ERROR;
				break;
			}
			computed = crc32(computed, out, (uInt)count);
			written += count;
		}
		if (status != Z_OK && status != Z_STREAM_END)
			break;
	}
	if (method == ZIP_DEFLATED)
		inflateEnd(&stream);
	if (written != size || (method == ZIP_DEFLATED && status != Z_STREAM_END))
	{
		if (!error[0])
			snprintf(error, (size_t)error_size, "the map in the zip is cut short");
		return 0;
	}
	if (computed != crc)
	{
		snprintf(error, (size_t)error_size, "the map's CRC is not the zip's");
		return 0;
	}
	return 1;
}

int map_archives_unzip(const char *zip_path, const char *file_name, unsigned long long size, const char *path,
	char *error, int error_size)
{
	struct posix_file_information information;
	unsigned char tail[ZIP_END_SIZE + ZIP_MAXIMUM_COMMENT];
	unsigned char *directory = NULL;
	unsigned long long zip_size, tail_size, directory_offset;
	unsigned long directory_size, entries, entry;
	unsigned long at;
	size_t name_length = strlen(file_name);
	int zip, output = -1, found = 0, done = 0;
	long end;

	error[0] = 0;
	zip = open(zip_path, O_RDONLY | O_CLOEXEC);
	if (zip < 0 || posix_fstat(zip, &information) != 0)
	{
		snprintf(error, (size_t)error_size, "the zip cannot be opened");
		if (zip >= 0)
			close(zip);
		return 0;
	}
	zip_size = (unsigned long long)information.size_high << 32 | information.size_low;
	/* the end of central directory record: the last of its signature in
	the file's last 64 KB, which it and the zip's comment fill */
	tail_size = zip_size < sizeof(tail) ? zip_size : sizeof(tail);
	if (tail_size < ZIP_END_SIZE || !map_archives_read_at(zip, zip_size - tail_size, tail, (unsigned long)tail_size))
	{
		snprintf(error, (size_t)error_size, "it is not a zip");
		goto finish;
	}
	for (end = (long)tail_size - ZIP_END_SIZE; end >= 0; end--)
	{
		if (zip_u32(tail + end) == ZIP_END_SIGNATURE &&
			(unsigned long long)end + ZIP_END_SIZE + zip_u16(tail + end + 20) <= tail_size)
		{
			break;
		}
	}
	if (end < 0)
	{
		snprintf(error, (size_t)error_size, "it is not a zip");
		goto finish;
	}
	entries = zip_u16(tail + end + 10);
	directory_size = zip_u32(tail + end + 12);
	directory_offset = zip_u32(tail + end + 16);
	/* (a Zip64 archive, over 4 GB, is no map's) */
	if (directory_size > ZIP_MAXIMUM_DIRECTORY || directory_offset > zip_size ||
		directory_size > zip_size - directory_offset)
	{
		snprintf(error, (size_t)error_size, "the zip's directory is damaged");
		goto finish;
	}
	directory = malloc(directory_size ? directory_size : 1);
	if (!directory || !map_archives_read_at(zip, directory_offset, directory, directory_size))
	{
		snprintf(error, (size_t)error_size, "the zip's directory cannot be read");
		goto finish;
	}
	for (entry = 0, at = 0; entry < entries && !found; entry++)
	{
		unsigned long entry_name_length, extra_length, comment_length;
		const char *name;
		const char *base;
		unsigned long index;

		if (at + ZIP_ENTRY_SIZE > directory_size || zip_u32(directory + at) != ZIP_ENTRY_SIGNATURE)
		{
			snprintf(error, (size_t)error_size, "the zip's directory is damaged");
			goto finish;
		}
		entry_name_length = zip_u16(directory + at + 28);
		extra_length = zip_u16(directory + at + 30);
		comment_length = zip_u16(directory + at + 32);
		if (at + ZIP_ENTRY_SIZE + entry_name_length + extra_length + comment_length > directory_size)
		{
			snprintf(error, (size_t)error_size, "the zip's directory is damaged");
			goto finish;
		}
		/* (the name's last part, after any folders) */
		name = (const char *)directory + at + ZIP_ENTRY_SIZE;
		base = name;
		for (index = 0; index < entry_name_length; index++)
		{
			if (name[index] == '/' || name[index] == '\\')
				base = name + index + 1;
		}
		if ((size_t)(name + entry_name_length - base) == name_length && same_name(base, file_name, name_length) &&
			zip_u32(directory + at + 24) == size)
		{
			unsigned char local[ZIP_LOCAL_SIZE];
			unsigned long long header = zip_u32(directory + at + 42);
			int method = (int)zip_u16(directory + at + 10);

			found = 1;
			if ((zip_u16(directory + at + 8) & 1) || (method != ZIP_STORED && method != ZIP_DEFLATED))
			{
				snprintf(error, (size_t)error_size, "the map is %s in the zip",
					(zip_u16(directory + at + 8) & 1) ? "encrypted" : "compressed in a way not read here");
				goto finish;
			}
			if (header + ZIP_LOCAL_SIZE > zip_size || !map_archives_read_at(zip, header, local, ZIP_LOCAL_SIZE) ||
				zip_u32(local) != ZIP_LOCAL_SIGNATURE)
			{
				snprintf(error, (size_t)error_size, "the zip's file header is damaged");
				goto finish;
			}
			output = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
			if (output < 0)
			{
				snprintf(error, (size_t)error_size, "the map cannot be written");
				goto finish;
			}
			done = unzip_data(zip, header + ZIP_LOCAL_SIZE + zip_u16(local + 26) + zip_u16(local + 28), zip_size, method,
				zip_u32(directory + at + 20), size, zip_u32(directory + at + 16), output, error, error_size);
		}
		at += ZIP_ENTRY_SIZE + entry_name_length + extra_length + comment_length;
	}
	if (!found)
		snprintf(error, (size_t)error_size, "the zip has no %s of %llu bytes", file_name, size);

finish:
	if (output >= 0)
	{
		if (close(output) != 0)
			done = 0;
		if (!done)
			unlink(path);
	}
	free(directory);
	close(zip);
	return done;
}

/* ---------- checking */

int map_archives_verify(const char *path, const char *file_name, unsigned long long size, unsigned long piece_length,
	const char *info_hash, char *error, int error_size)
{
	static const char hex[] = "0123456789abcdef";
	unsigned long long pieces = piece_length ? (size + piece_length - 1) / piece_length : 0;
	unsigned char *hashes = NULL;
	unsigned char *piece = NULL;
	unsigned char digest[SHA1_DIGEST_SIZE];
	struct sha1_state state;
	char prefix[512];
	char found[41];
	unsigned long long index;
	int file;
	int ok = 0;

	/* (a piece's hash is 20 bytes: a few MB of them for any map there is) */
	if (!size || !pieces || pieces > (16ULL << 20) / SHA1_DIGEST_SIZE || strlen(info_hash) != 40)
	{
		snprintf(error, (size_t)error_size, "the index's size or piece length is bad");
		return 0;
	}
	file = open(path, O_RDONLY | O_CLOEXEC);
	if (file < 0)
	{
		snprintf(error, (size_t)error_size, "it cannot be read");
		return 0;
	}
	hashes = malloc((size_t)pieces * SHA1_DIGEST_SIZE);
	piece = malloc(piece_length);
	if (!hashes || !piece)
	{
		snprintf(error, (size_t)error_size, "no memory to check it");
		goto done;
	}
	for (index = 0; index < pieces; index++)
	{
		unsigned long length = index + 1 < pieces ? piece_length : (unsigned long)(size - index * piece_length);

		if (!map_archives_read_at(file, index * piece_length, piece, length))
		{
			snprintf(error, (size_t)error_size, "it is shorter than the index's");
			goto done;
		}
		sha1_bytes(piece, length, hashes + index * SHA1_DIGEST_SIZE);
	}
	/* (the info dictionary as BitTorrent encodes it, its keys in order:
	the index's maker made it so, tools/map_index.py) */
	snprintf(prefix, sizeof(prefix), "d6:lengthi%llue4:name%lu:%s12:piece lengthi%lue6:pieces%llu:", size,
		(unsigned long)strlen(file_name), file_name, piece_length, pieces * SHA1_DIGEST_SIZE);
	sha1_start(&state);
	sha1_update(&state, prefix, (unsigned long)strlen(prefix));
	sha1_update(&state, hashes, (unsigned long)(pieces * SHA1_DIGEST_SIZE));
	sha1_update(&state, "e", 1);
	sha1_finish(&state, digest);
	for (index = 0; index < SHA1_DIGEST_SIZE; index++)
	{
		found[index * 2] = hex[digest[index] >> 4];
		found[index * 2 + 1] = hex[digest[index] & 15];
	}
	found[40] = 0;
	/* (the index's digits in either case) */
	ok = 1;
	for (index = 0; index < 40; index++)
	{
		char wanted = info_hash[index];

		if (wanted >= 'A' && wanted <= 'F')
			wanted = (char)(wanted - 'A' + 'a');
		if (found[index] != wanted)
			ok = 0;
	}
	if (!ok)
		snprintf(error, (size_t)error_size, "its hash is %s, the index's %s", found, info_hash);

done:
	free(piece);
	free(hashes);
	close(file);
	return ok;
}
