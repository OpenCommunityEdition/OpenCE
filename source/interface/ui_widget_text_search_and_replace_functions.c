/* ---------- headers */

#include "cseries.h"
#include "ui_widget_text_search_and_replace_functions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* Fields shared by every widget instance and evidenced by ui_widget.c. */
struct widget_instance_prefix
{
	long definition_tag_index;
	char const *name;
	short local_player_index;
};

typedef char widget_instance_local_player_index_offset[
	offsetof(struct widget_instance_prefix, local_player_index) == 8 ? 1 : -1];

/* ---------- prototypes */

typedef wchar_t *(*ui_widget_text_replacement_function)(void *widget);

static wchar_t *widget_replace_function_null(void *widget);
static wchar_t *widget_controller(void *widget);

/* ---------- globals */

static ui_widget_text_replacement_function replace_function_list[2] =
{
	widget_replace_function_null,
	widget_controller
};

static wchar_t result[2] = { 0 };

/* ---------- public code */

wchar_t *ui_widget_search_and_replace_invoke(void *widget, unsigned short function_index)
{
	match_assert("c:\\halo\\SOURCE\\interface\\ui_widget_text_search_and_replace_functions.c", 45, widget);

	if ((short)function_index >= 0 && function_index < 2)
		return replace_function_list[(short)function_index](widget);

	return L"<invalid>";
}

/* ---------- private code */

static wchar_t *widget_replace_function_null(void *widget)
{
	(void)widget;
	return L"";
}

static wchar_t *widget_controller(void *widget)
{
	struct widget_instance_prefix *instance = widget;

	switch (instance->local_player_index)
	{
	case NONE:
	case 0:
		result[0] = L'1';
		result[1] = L'\0';
		return result;

	case 1:
		result[0] = L'2';
		result[1] = L'\0';
		return result;

	case 2:
		result[0] = L'3';
		result[1] = L'\0';
		return result;

	case 3:
		result[0] = L'4';
		result[1] = L'\0';
		return result;

	default:
		result[0] = L'?';
		result[1] = L'\0';
		return result;
	}
}
