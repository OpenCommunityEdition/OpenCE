#include "port/linux/include/halo_menus.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

const struct halo_menus *halo_menus_load(void);
long halo_menus_utf16(const char *, unsigned short *, long);

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); } } while (0)

void platform_log(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	va_end(args);
}

int main(void)
{
	const struct halo_menus *menus = halo_menus_load();
	const struct halo_menus *again = halo_menus_load();
	unsigned short text[16];
	long count = halo_menus_utf16("A\\n\xc3\xa9", text, 16);
	CHECK(menus != NULL);
	if (menus)
	{
		CHECK(menus->widget_count > 0);
		CHECK(menus->string_list_count > 0);
		CHECK(menus->bitmap_count > 0);
		CHECK(menus->root && menus->root[0]);
		CHECK(again == menus);
	}
	CHECK(count == 5);
	CHECK(text[0] == 'A' && text[1] == '\r' && text[2] == '\n');
	CHECK(text[3] == 0x00e9 && text[4] == 0);
	printf("Mac embedded menu parser: %d checks, %d failures; widgets=%ld strings=%ld bitmaps=%ld\n",
		checks, failures, menus ? menus->widget_count : 0, menus ? menus->string_count : 0,
		menus ? menus->bitmap_count : 0);
	return failures ? 1 : 0;
}
