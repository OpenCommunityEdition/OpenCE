#define _CRT_SECURE_NO_WARNINGS
#include "harness.h"
#include <stdlib.h>
#include <string.h>
#include "toml.inc"
#include "under_test.inc"

static void check_reset(const char *text)
{
    toml_result_t old = toml_parse(text, (int)strlen(text)), result;
    char *changed;
    size_t i;
    CHECK(old.ok, "valid fixture: %s", old.errmsg);
    CHECK(lighting_settings_legacy(old.toptab), "legacy fixture needs migration");
    changed = lighting_settings_migrate(text, old.toptab);
    CHECK(changed != NULL, "migration produced text");
    result = toml_parse(changed, (int)strlen(changed));
    CHECK(result.ok, "migrated TOML valid: %s", result.errmsg);
    for (i = 0; i < 13; i++)
    {
        toml_datum_t value = toml_seek(result.toptab, lighting_reset_keys[i]);
        CHECK(value.type == TOML_UNKNOWN || (value.type == TOML_BOOLEAN && !value.u.boolean), "lighting off");
    }
    if (toml_seek(result.toptab, "display.shield_glow_intensity").type == TOML_STRING)
        CHECK(!strcmp(toml_seek(result.toptab, "display.shield_glow_intensity").u.s, "default"), "default intensity");
    CHECK(toml_seek(result.toptab, "display.vsync").u.boolean, "unrelated preference preserved");
    CHECK(strstr(changed, "# keep comment"), "comments preserved");
    toml_free(result);toml_free(old);free(changed);
}

int main(int argc, char **argv)
{
    toml_result_t result;
    char text[2048] = "# keep comment\n[display]\nvsync = true\n";
    const char completed[] = "[display]\ndynamic_lighting_reset=true\nweapon_glow=true\n";
    size_t i;
    (void)argc;(void)argv;
    for (i = 0; i < 13; i++)
    { strcat(text, lighting_reset_keys[i] + 8);strcat(text, " = true\n"); }
    strcat(text, "shield_glow_intensity = 'light_show'\n");
    check_reset(text);
    check_reset("# keep comment\r\n[display]\r\nvsync = true\r\n\"weapon_glow\" = true # light\r\nshield_glow_intensity = \"light_show\"\r\n");
    check_reset("# keep comment\ndisplay = {vsync = true, weapon_glow = true, shield_glow_intensity = 'light_show'}\n");
    check_reset("# keep comment\ndisplay.vsync = true\ndisplay.weapon_glow = true\n");
    check_reset("# keep comment\n[display]\nvsync = true\ndynamic_lighting_reset = false\nshield_glow_intensity = \"\"\"light_\nshow\"\"\"\n");
    result=toml_parse(completed, sizeof(completed)-1);
    CHECK(result.ok, "completed migration fixture");
    CHECK(!lighting_settings_legacy(result.toptab), "marker preserves later choices");
    CHECK(lighting_settings_migrate("", result.toptab)==NULL, "no repeated migration");
    CHECK(toml_seek(result.toptab,"display.weapon_glow").u.boolean, "new ON choice retained");
    toml_free(result);
    {
        const char invalid[] = "[display]\nweapon_glow='invalid'\n";
        result=toml_parse(invalid,sizeof(invalid)-1);
        CHECK(result.ok && lighting_settings_legacy(result.toptab), "wrong setting type needs reset");
        CHECK(!lighting_settings_migrate(invalid,result.toptab), "unmigratable file never marked complete");
        toml_free(result);
    }
    CHECK(lighting_settings_reset_key("display.shield_glow"), "glow reset");
    CHECK(!lighting_settings_reset_key("display.shield_look") && !lighting_settings_reset_key("display.per_pixel_lighting"), "unrelated rendering preserved");
    return 0;
}
