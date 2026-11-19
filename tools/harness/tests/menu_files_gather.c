/*
MENU_FILES_GATHER.C (test)

The real files_gather, file_find and file_add (port/linux/src/menu_files.c):
a file of the menus folder with an embedded file's name replaces it, and the
folder's other .xml files are added, also where the folder's listing spells
a subfolder with a backslash, as Windows does (ce\main_menu.xml), which must
not add a replaced file a second time. test_menu_files_gather.py takes the
code from the sources.
*/

#include "harness.h"

#ifdef _WIN32
#define strdup _strdup
#endif

struct menu_file_embedded { const char *path; const unsigned int *data; unsigned int size; };

static const unsigned int embedded_data[2] = { 1, 2 };
static const struct menu_file_embedded menu_files_embedded[] = {
	{ "ce/main_menu.xml", embedded_data, 4 },
	{ "ce/bitmaps.xml", embedded_data, 4 },
};
static const unsigned int menu_files_embedded_count = 2;

static int backslashes;

static void config_folder(char *folder, size_t size) { snprintf(folder, size, "F/"); }
static void platform_log(const char *format, ...) { (void)format; }
#define SDL_free free

/* the menus folder: ce/main_menu.xml (a replacement) and mine.xml */
static unsigned char *file_read(const char *path, unsigned long *size)
{
	const char *known[] = { "F/menus/ce/main_menu.xml", "F/menus/ce\\main_menu.xml", "F/menus/mine.xml" };
	int index;

	for (index = 0; index < 3; index++)
	{
		if (!strcmp(path, known[index]))
		{
			*size = 3;
			return (unsigned char *)strdup("xml");
		}
	}
	return NULL;
}

static char **SDL_GlobDirectory(const char *folder, const char *pattern, int flags, int *count)
{
	char **names = calloc(2, sizeof(char *));

	(void)folder;
	(void)flags;
	if (!strcmp(pattern, "*.xml"))
	{
		names[0] = strdup("mine.xml");
	}
	else
	{
		names[0] = strdup(backslashes ? "ce\\main_menu.xml" : "ce/main_menu.xml");
	}
	*count = 1;
	return names;
}

#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	long index;

	backslashes = !strcmp(case_name, "backslashes");
	if (strcmp(case_name, "slashes") && strcmp(case_name, "backslashes"))
	{
		fprintf(stderr, "no case named %s\n", case_name);
		return 2;
	}
	files_gather();
	CHECK(file_count == 3, "%ld files, not the 2 embedded and mine.xml", file_count);
	index = file_find("ce/main_menu.xml");
	CHECK(index >= 0 && files[index].loaded, "ce/main_menu.xml not replaced by the folder's");
	CHECK(file_find("mine.xml") >= 0, "mine.xml not added");
	CHECK(file_find("ce/bitmaps.xml") >= 0 && !files[file_find("ce/bitmaps.xml")].loaded, "ce/bitmaps.xml not kept");
	return 0;
}
