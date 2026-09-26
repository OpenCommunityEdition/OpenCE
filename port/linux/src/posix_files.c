/*
POSIX_FILES.C

glibc file system helpers for the platform layer (see posix.h). Built with
the host ABI and _FILE_OFFSET_BITS=64.
*/

#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

#include "posix.h"

static void split64(unsigned long long value, unsigned long *low, unsigned long *high)
{
	*low = (unsigned long)(value & 0xffffffffULL);
	*high = (unsigned long)(value >> 32);
}

static void fill_information(const struct stat *st, struct posix_file_information *information)
{
	memset(information, 0, sizeof(*information));
	if (S_ISDIR(st->st_mode))
		information->flags |= _posix_file_is_directory;
	if (!(st->st_mode & S_IWUSR))
		information->flags |= _posix_file_is_read_only;
	split64((unsigned long long)st->st_size, &information->size_low, &information->size_high);
	information->modification_seconds = (unsigned long)st->st_mtim.tv_sec;
	information->modification_nanoseconds = (unsigned long)st->st_mtim.tv_nsec;
	information->access_seconds = (unsigned long)st->st_atim.tv_sec;
	information->access_nanoseconds = (unsigned long)st->st_atim.tv_nsec;
	/* Linux has no portable creation time; the change time is the closest */
	information->creation_seconds = (unsigned long)st->st_ctim.tv_sec;
	information->creation_nanoseconds = (unsigned long)st->st_ctim.tv_nsec;
}

int posix_stat(const char *path, struct posix_file_information *information)
{
	struct stat st;

	if (stat(path, &st) != 0)
		return -1;
	fill_information(&st, information);
	return 0;
}

int posix_fstat(int descriptor, struct posix_file_information *information)
{
	struct stat st;

	if (fstat(descriptor, &st) != 0)
		return -1;
	fill_information(&st, information);
	return 0;
}

int posix_set_file_times(const char *path,
	unsigned long access_seconds, unsigned long access_nanoseconds,
	unsigned long modification_seconds, unsigned long modification_nanoseconds)
{
	struct timespec times[2];

	times[0].tv_sec = (time_t)access_seconds;
	times[0].tv_nsec = access_seconds ? (long)access_nanoseconds : UTIME_OMIT;
	times[1].tv_sec = (time_t)modification_seconds;
	times[1].tv_nsec = modification_seconds ? (long)modification_nanoseconds : UTIME_OMIT;
	return utimensat(AT_FDCWD, path, times, 0);
}

int posix_seek(int descriptor, long offset_low, long offset_high, int whence,
	unsigned long *position_low, unsigned long *position_high)
{
	off_t offset = (off_t)(((unsigned long long)(unsigned long)offset_high << 32) | (unsigned long)offset_low);
	off_t result = lseek(descriptor, offset, whence);

	if (result == (off_t)-1)
		return -1;
	split64((unsigned long long)result, position_low, position_high);
	return 0;
}

int posix_truncate(int descriptor, unsigned long size_low, unsigned long size_high)
{
	return ftruncate(descriptor, (off_t)(((unsigned long long)size_high << 32) | size_low));
}

int posix_disk_space(const char *path,
	unsigned long *free_low, unsigned long *free_high,
	unsigned long *total_low, unsigned long *total_high)
{
	struct statvfs st;

	if (statvfs(path, &st) != 0)
		return -1;
	split64((unsigned long long)st.f_bavail * st.f_frsize, free_low, free_high);
	split64((unsigned long long)st.f_blocks * st.f_frsize, total_low, total_high);
	return 0;
}

int posix_set_read_only(const char *path, int read_only)
{
	struct stat st;
	mode_t mode;

	if (stat(path, &st) != 0)
		return -1;
	mode = st.st_mode & 07777;
	mode = read_only ? (mode & ~(mode_t)0222) : (mode | S_IWUSR);
	return chmod(path, mode);
}

int posix_make_directory(const char *path)
{
	return mkdir(path, 0755);
}

void *posix_directory_open(const char *path)
{
	return opendir(path);
}

int posix_directory_next(void *directory, char *name, unsigned long name_size)
{
	struct dirent *entry;

	while ((entry = readdir((DIR *)directory)) != NULL)
	{
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		if (strlen(entry->d_name) + 1 > name_size)
			continue;
		strcpy(name, entry->d_name);
		return 1;
	}
	return 0;
}

void posix_directory_close(void *directory)
{
	if (directory)
		closedir((DIR *)directory);
}

int posix_find_entry_case_insensitive(const char *directory, const char *name,
	char *result, unsigned long result_size)
{
	DIR *handle = opendir(*directory ? directory : ".");
	struct dirent *entry;
	int found = 0;

	if (!handle)
		return 0;
	while ((entry = readdir(handle)) != NULL)
	{
		if (!strcasecmp(entry->d_name, name) && strlen(entry->d_name) + 1 <= result_size)
		{
			strcpy(result, entry->d_name);
			found = 1;
			break;
		}
	}
	closedir(handle);
	return found;
}
