#include "port_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int checks, failures;
extern int halo_interpolation_enabled(void);
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"line %d failed: %s\n",__LINE__,#x); } } while (0)
int main(int argc, char **argv)
{
    char text[4096], path[4096];
    char *contents;
    size_t size;
    if (argc != 2) return 2;
    if (!strcmp(argv[1], "defaults")) {
        CHECK(config_boolean("game.custom_edition") == 1);
        CHECK(config_boolean("display.vsync") == 1);
        CHECK(config_boolean("debug.hidden_window") == 0);
        CHECK(config_default("display.vsync", text, sizeof(text)) && !strcmp(text,"true"));
        CHECK(config_write_boolean("display.vsync", 0));
        CHECK(config_boolean("display.vsync") == 0);
        CHECK(config_changes() == 1);
    } else if (!strcmp(argv[1], "reload")) {
        CHECK(config_boolean("display.vsync") == 0);
        CHECK(config_boolean("game.custom_edition") == 1);
    } else if (!strcmp(argv[1], "environment")) {
        CHECK(config_boolean("display.vsync") == 0);
        CHECK(config_boolean("game.custom_edition") == 0);
        CHECK(config_boolean("debug.hidden_window") == 1);
    } else if (!strcmp(argv[1], "invalid")) {
        CHECK(config_boolean("display.vsync") == 1);
        CHECK(config_boolean("game.custom_edition") == 0);
    } else if (!strcmp(argv[1], "interpolation")) {
        CHECK(halo_interpolation_enabled() == config_boolean("display.interpolation"));
        CHECK(config_write_boolean("display.interpolation", 0));
        CHECK(!halo_interpolation_enabled());
        CHECK(config_write_boolean("display.interpolation", 1));
        CHECK(halo_interpolation_enabled());
        CHECK(config_write_boolean("display.interpolation", 0));
        CHECK(!halo_interpolation_enabled());
    } else return 2;
    config_folder(path, sizeof(path));
    CHECK(strlen(path) > 1 && path[strlen(path)-1] == '/');
    strcat(path, "config.toml");
    contents = config_file_read(path, &size);
    CHECK(contents && size && contents[size] == 0);
    free(contents);
    printf("native settings %s: %d checks, %d failures\n", argv[1], checks, failures);
    return failures ? 1 : 0;
}
