#include "harness.h"
#include <wchar.h>
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))

struct widget_instance {
	char const *name; int visible;
	struct { struct { struct widget_instance *extended_description; } list; } parameters;
};
static int host, calls, last_add, last_team;
static int saves, applies, apply_ok = TRUE, errors;
static wchar_t displayed[180];
static struct widget_instance description, options;
static void *global_network_game_server_get(void) { return host ? (void *)1 : NULL; }
static struct widget_instance *screen_of(struct widget_instance *widget) { return widget; }
static struct widget_instance *named(struct widget_instance *widget, char const *name, int index)
{ (void)widget; (void)index; CHECK(!strcmp(name, "options_menu"), "wrong list"); return &options; }
static void text_set(struct widget_instance *widget, wchar_t const *text)
{ CHECK(widget == &description, "status did not target list description"); wcsncpy(displayed, text, 179); displayed[179] = 0; }
static int bots_manage(int add, int team, char *status, long size)
{
	calls++; last_add = add; last_team = team;
	snprintf(status, size, "Host result");
	return host;
}
static void console_printf(int color, char const *format, ...) { (void)color; (void)format; }
struct pc_menu_setting;
static boolean setting_changed_save(struct widget_instance *widget, struct pc_menu_setting *setting)
{ (void)widget; (void)setting; return TRUE; }
static void settings_each(struct widget_instance *widget,
	boolean (*visit)(struct widget_instance *, struct pc_menu_setting *))
{ (void)widget; CHECK(visit == setting_changed_save, "wrong save callback"); saves++; }
static int bots_lobby_settings_apply(char *status, long size)
{ CHECK(saves == applies + 1, "policy applied before saving choices"); applies++; snprintf(status, size, "Slot conflict"); return apply_ok; }
static void display_error_text_deferred(wchar_t const *message, short controller)
{ (void)controller; CHECK(!wcscmp(message, L"Slot conflict"), "missing rejection text"); errors++; }
#include "under_test.inc"

int main(void)
{
	static char const *const rows[] = { "op_add_any", "op_add_red", "op_add_blue",
		"op_drop_any", "op_drop_red", "op_drop_blue" };
	static char const *const commands[] = { "add_bot", "add_bot_red", "add_bot_blue",
		"drop_bot", "drop_bot_red", "drop_bot_blue" };
	struct widget_instance widget = { "button_bots", TRUE, { { NULL } } };
	int index;
	options.parameters.list.extended_description = &description;
	CHECK(bots_menu_event(&description, "port bots init") && displayed[0], "initial help missing");
	CHECK(bots_menu_event(&widget, "port bots host entry") && !widget.visible, "client host button visible");
	host = TRUE;
	CHECK(bots_menu_event(&widget, "port bots host entry") && widget.visible, "host button hidden");
	for (index = 0; index < 6; index++) {
		widget.name = rows[index];
		CHECK(bots_menu_event(&widget, "port bots command"), "unhandled menu action");
		CHECK(last_add == (index < 3) && last_team == index % 3 - 1, "wrong menu team or operation");
		CHECK(!wcscmp(displayed, L"Host result"), "host result not shown");
		CHECK(bots_console_command(commands[index]) == TRUE, "console command unhandled");
		CHECK(last_add == (index < 3) && last_team == index % 3 - 1, "wrong console team or operation");
	}
	CHECK(calls == 12, "duplicated action");
	CHECK(bots_console_command("add_bot unexpected") == -1 && bots_console_command("unknown") == -1,
		"unrelated console command intercepted");
	host = FALSE;
	CHECK(bots_console_command("add_bot") == FALSE, "host rejection hidden");
	widget.name = "button_ok";
	CHECK(!bots_menu_event(&widget, "unrelated") && saves == 0, "unrelated event applied settings");
	CHECK(bots_menu_event(&widget, "port bots settings save") && saves == 1 && applies == 1 && !errors,
		"OK did not save then apply");
	apply_ok = FALSE;
	CHECK(bots_menu_event(&widget, "port bots settings save") && errors == 1, "slot conflict was silent");
	return 0;
}
