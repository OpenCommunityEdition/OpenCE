/*
X10_WARTHOGS.C (test)

The real x10_warthogs_wake_once, x10_warthogs_run_shortened and
x10_warthogs_driver_placed (x10_warthogs.c): a10's hangar scene, Sergeant
Johnson's speech, woke its warthog's script (x10_hog_4) on Legendary twice,
the second wake ending the script's sleep, so the warthog was destroyed
before it was seen; the script slept for the warthog's whole run, which
ends in a crash into the hangar's far wall; and the scene's two warthog
drivers were placed with Johnson's variant. Legendary's own wake is taken
out, the sleep made one of 120 ticks, and the drivers placed with no
variant. The nodes are laid out as a10's are, in a data array of script
nodes: a call (permanent) names its function with its first child (a
primitive of the function name type), which its arguments follow.
test_x10_warthogs.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <stdarg.h>

typedef unsigned short word;
typedef struct { real yaw, pitch, roll; } real_euler_angles3d;
#define NUMBEROF(array) (sizeof(array) / sizeof((array)[0]))
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))

#include "under_test.inc"

#define PRIMITIVE 1
#define PERMANENT 8

static struct data_array *syntax;
static char strings[4096];
static long string_bytes;

static long string_add(char const *text)
{
	long offset = string_bytes;

	strcpy(strings + offset, text);
	string_bytes += (long)strlen(text) + 1;
	return offset;
}

static struct hs_syntax_node *node(long index)
{
	return (struct hs_syntax_node *)datum_get(syntax, index);
}

static long node_new(short type, short flags, char const *text)
{
	long index = datum_new(syntax);
	struct hs_syntax_node *new_node = node(index);

	new_node->type = type;
	new_node->flags = flags;
	new_node->next_node_index = NONE;
	new_node->string_offset = text ? string_add(text) : NONE;
	return index;
}

/* a call of the type given and its arguments, a NONE-ended list of nodes */
static long call_new(short type, char const *name, ...)
{
	long call = node_new(type, PERMANENT, NULL);
	long previous = node_new(_hs_function_name, PRIMITIVE | PERMANENT, name);
	long argument;
	va_list arguments;

	node(call)->data = previous;
	va_start(arguments, name);
	while ((argument = va_arg(arguments, long)) != NONE)
	{
		node(previous)->next_node_index = argument;
		previous = argument;
	}
	va_end(arguments);
	return call;
}

/* a primitive of the text given (a script's name, an object's, a sound's) */
static long name_new(char const *text)
{
	return node_new(_hs_type_string, PRIMITIVE | PERMANENT, text);
}

static long constant_new(short ticks)
{
	long constant = node_new(_hs_type_short_integer, PRIMITIVE | PERMANENT, "ticks");

	node(constant)->constant_type = _hs_type_short_integer;
	node(constant)->short_value = ticks;
	return constant;
}

static long wake_new(char const *script)
{
	return call_new(_hs_type_void, "wake", name_new(script), NONE);
}

static long sleep_new(long ticks)
{
	return call_new(_hs_type_void, "sleep", ticks, NONE);
}

static long recording_time_new(char const *vehicle)
{
	return call_new(_hs_type_short_integer, "recording_time", name_new(vehicle), NONE);
}

/* a begin of the expressions given, a NONE-ended list */
static long begin_new(long first, ...)
{
	long begin = call_new(_hs_type_void, "begin", NONE);
	long previous = node(begin)->data;
	long expression = first;
	va_list expressions;

	va_start(expressions, first);
	while (expression != NONE)
	{
		node(previous)->next_node_index = expression;
		previous = expression;
		expression = va_arg(expressions, long);
	}
	va_end(expressions);
	return begin;
}

/* (if (= <difficulty> (game_difficulty_get_real)) <begin>) */
static long if_new(char const *difficulty, long branch)
{
	long condition = call_new(5, "=", name_new(difficulty), call_new(32, "game_difficulty_get_real", NONE), NONE);

	return call_new(_hs_type_void, "if", condition, branch, NONE);
}

/* a difficulty's lines, as a10's are: Johnson's last line on Legendary has
x10_hog_4 woken first */
static long lines_new(char const *wake)
{
	return wake ?
		begin_new(
			call_new(_hs_type_void, "camera_set", name_new("johnson_right_zoom"), NONE),
			call_new(_hs_type_void, "custom_animation", name_new("johnson"), NONE),
			wake_new(wake),
			call_new(_hs_type_void, "sound_impulse_start", name_new("sound\\dialog\\x10\\sgt05f"), NONE),
			sleep_new(constant_new(57)),
			NONE) :
		begin_new(
			call_new(_hs_type_void, "camera_set", name_new("johnson_right_zoom"), NONE),
			call_new(_hs_type_void, "custom_animation", name_new("johnson"), NONE),
			call_new(_hs_type_void, "sound_impulse_start", name_new("sound\\dialog\\x10\\sgt05b"), NONE),
			sleep_new(constant_new(43)),
			NONE);
}

/* a10's hangar_2, from the four difficulties' lines to the marines'
answer: Legendary's lines wake the script given, and after the ifs the
script given is woken */
static long hangar_new(char const *legendary_wake, char const *wake)
{
	return begin_new(
		if_new("easy", lines_new(NULL)),
		if_new("normal", lines_new(NULL)),
		if_new("hard", lines_new(NULL)),
		if_new("impossible", lines_new(legendary_wake)),
		wake ? wake_new(wake) : call_new(_hs_type_void, "camera_set", name_new("johnson_right_1b"), NONE),
		call_new(_hs_type_void, "camera_set", name_new("johnson_right_1b"), NONE),
		call_new(_hs_type_void, "sound_impulse_start", name_new("sound\\dialog\\x10\\mar01"), NONE),
		NONE);
}

/* a10's x10_hog_4, with the vehicle the run plays on and the one whose
recording the sleep waits for */
static long hog_new(char const *played, char const *waited)
{
	return begin_new(
		call_new(_hs_type_void, "object_create_anew", name_new("x10_warthog_2"), NONE),
		call_new(_hs_type_void, "object_create_anew", name_new("hog_2_driver"), NONE),
		call_new(_hs_type_void, "vehicle_load_magic", name_new("x10_warthog_2"), NONE),
		call_new(_hs_type_void, "object_teleport", name_new("x10_warthog_2"), NONE),
		call_new(_hs_type_void, "recording_play", name_new(played), name_new("x10_hog_4_run"), NONE),
		sleep_new(recording_time_new(waited)),
		call_new(_hs_type_void, "object_destroy", name_new("x10_warthog_2"), NONE),
		call_new(_hs_type_void, "object_destroy", name_new("hog_2_driver"), NONE),
		NONE);
}

/* the begin's expressions, in order: a call by its function's name (a wake
or a sleep with its argument), anything else as '?' */
static char const *begin_order(long begin)
{
	static char order[2048];
	long index = node(node(begin)->data)->next_node_index;
	long walked = 0;

	order[0] = 0;
	while (index != NONE && walked++ < 200)
	{
		struct hs_syntax_node *expression = node(index);

		if (order[0])
			strcat(order, " ");
		if (TEST_FLAG(expression->flags, 0))
		{
			strcat(order, "?");
		}
		else
		{
			struct hs_syntax_node *name = node(expression->data);
			struct hs_syntax_node *argument = name->next_node_index != NONE ? node(name->next_node_index) : NULL;

			strcat(order, strings + name->string_offset);
			if (!strcmp(strings + name->string_offset, "wake"))
			{
				sprintf(order + strlen(order), ":%s", strings + argument->string_offset);
			}
			else if (!strcmp(strings + name->string_offset, "sleep"))
			{
				if (TEST_FLAG(argument->flags, 0))
					sprintf(order + strlen(order), ":%d", argument->short_value);
				else
					sprintf(order + strlen(order), ":(%s)", strings + node(argument->data)->string_offset);
			}
		}
		index = expression->next_node_index;
	}
	return order;
}

/* the begin of the hangar's nth if */
static long hangar_lines(long hangar, int n)
{
	long index = node(node(hangar)->data)->next_node_index;
	long condition;

	while (n-- > 0)
		index = node(index)->next_node_index;
	condition = node(node(index)->data)->next_node_index;
	return node(condition)->next_node_index;
}

/* the sleep's argument in the hog's begin */
static struct hs_syntax_node *hog_ticks(long hog)
{
	long index = node(node(hog)->data)->next_node_index;

	while (strcmp(strings + node(node(index)->data)->string_offset, "sleep"))
		index = node(index)->next_node_index;
	return node(node(node(index)->data)->next_node_index);
}

/* the nodes as they are, to see that nothing changed */
static unsigned char before[1024 * sizeof(struct hs_syntax_node)];
static long before_count;

static void nodes_keep(void)
{
	memcpy(before, syntax->data, (size_t)(syntax->maximum_count * syntax->size));
	before_count = syntax->count;
}

static boolean nodes_unchanged(void)
{
	return before_count == syntax->count &&
		!memcmp(before, syntax->data, (size_t)(syntax->maximum_count * syntax->size));
}

static boolean wake_once(long root)
{
	return x10_warthogs_wake_once(syntax, strings, string_bytes, root);
}

static boolean run_shortened(long root)
{
	return x10_warthogs_run_shortened(syntax, strings, string_bytes, root);
}

static struct scenario_object_datum placed(short variant)
{
	struct scenario_object_datum object;

	memset(&object, 0, sizeof(object));
	object.variant_number = variant;
	return object;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	syntax = game_state_data_new("script node", 1024, sizeof(struct hs_syntax_node));

	/* Legendary's lines no longer wake x10_hog_4: it is woken once, after
	the ifs, as on every other difficulty; nothing else moves */
	CASE("legendary")
	{
		long hangar = hangar_new("x10_hog_4", "x10_hog_4");
		char normal[2048];

		strcpy(normal, begin_order(hangar_lines(hangar, 1)));
		CHECK(wake_once(hangar), "not changed");
		CHECK(!strcmp(begin_order(hangar_lines(hangar, 3)), "camera_set custom_animation sound_impulse_start sleep:57"),
			"Legendary's lines %s", begin_order(hangar_lines(hangar, 3)));
		CHECK(!strcmp(begin_order(hangar_lines(hangar, 1)), normal), "Normal's lines %s", begin_order(hangar_lines(hangar, 1)));
		CHECK(!strcmp(begin_order(hangar), "if if if if wake:x10_hog_4 camera_set sound_impulse_start"),
			"the scene %s", begin_order(hangar));
		return 0;
	}
	/* once: a second time (hs_scenario_postprocess runs again on a revert)
	changes nothing */
	CASE("woken once")
	{
		long hangar = hangar_new("x10_hog_4", "x10_hog_4");

		CHECK(wake_once(hangar), "not changed");
		nodes_keep();
		CHECK(!wake_once(hangar), "changed again");
		CHECK(nodes_unchanged(), "changed again");
		return 0;
	}
	/* an if's wake with no wake after the if, one of another script, or one
	after the if of another: left as they are */
	CASE("other wakes")
	{
		long alone = hangar_new("x10_hog_4", NULL);
		long other_inside = hangar_new("x10_hog_5", "x10_hog_4");
		long other_after = hangar_new("x10_hog_4", "x10_hog_5");
		long call = wake_new("x10_hog_4");

		nodes_keep();
		CHECK(!wake_once(alone), "a wake not woken again taken out");
		CHECK(!wake_once(other_inside), "another script's wake taken out");
		CHECK(!wake_once(other_after), "taken out for another script's wake");
		CHECK(!wake_once(call), "no begin changed");
		CHECK(!wake_once(NONE), "no root changed");
		CHECK(!x10_warthogs_wake_once(syntax, NULL, string_bytes, alone), "no strings changed");
		CHECK(!x10_warthogs_wake_once(NULL, strings, string_bytes, alone), "no nodes changed");
		CHECK(nodes_unchanged(), "changed");
		return 0;
	}
	/* the sleep for the rest of the run is one of 120 ticks, a constant of
	its type; nothing else moves */
	CASE("run")
	{
		long hog = hog_new("x10_warthog_2", "x10_warthog_2");
		struct hs_syntax_node *ticks;
		long count = syntax->count;

		CHECK(!strcmp(begin_order(hog), "object_create_anew object_create_anew vehicle_load_magic object_teleport "
			"recording_play sleep:(recording_time) object_destroy object_destroy"), "the run %s", begin_order(hog));
		CHECK(run_shortened(hog), "not changed");
		CHECK(!strcmp(begin_order(hog), "object_create_anew object_create_anew vehicle_load_magic object_teleport "
			"recording_play sleep:120 object_destroy object_destroy"), "the run %s", begin_order(hog));
		ticks = hog_ticks(hog);
		CHECK(TEST_FLAG(ticks->flags, 0) && !TEST_FLAG(ticks->flags, 2) && TEST_FLAG(ticks->flags, 3),
			"flags %#x", ticks->flags);
		CHECK(ticks->type == _hs_type_short_integer && ticks->constant_type == _hs_type_short_integer,
			"type %d, constant type %d", ticks->type, ticks->constant_type);
		CHECK(ticks->next_node_index == NONE, "an argument after it");
		CHECK(syntax->count == count, "%ld nodes made", syntax->count - count);
		return 0;
	}
	/* once: a second time changes nothing */
	CASE("shortened once")
	{
		long hog = hog_new("x10_warthog_2", "x10_warthog_2");

		CHECK(run_shortened(hog), "not changed");
		nodes_keep();
		CHECK(!run_shortened(hog), "changed again");
		CHECK(nodes_unchanged(), "changed again");
		return 0;
	}
	/* another vehicle's run, a wait for another's, a wait before the run, a
	sleep of a constant: left as they are */
	CASE("other runs")
	{
		long other_run = hog_new("x10_warthog_3", "x10_warthog_3");
		long other_wait = hog_new("x10_warthog_2", "x10_warthog_3");
		long wait_first = begin_new(
			sleep_new(recording_time_new("x10_warthog_2")),
			call_new(_hs_type_void, "recording_play", name_new("x10_warthog_2"), name_new("x10_hog_4_run"), NONE),
			NONE);
		long constant = begin_new(
			call_new(_hs_type_void, "recording_play", name_new("x10_warthog_2"), name_new("x10_hog_4_run"), NONE),
			sleep_new(constant_new(307)),
			NONE);

		nodes_keep();
		CHECK(!run_shortened(other_run), "another vehicle's run changed");
		CHECK(!run_shortened(other_wait), "a wait for another vehicle changed");
		CHECK(!run_shortened(wait_first), "a wait before the run changed");
		CHECK(!run_shortened(constant), "a constant sleep changed");
		CHECK(!run_shortened(NONE), "no root changed");
		CHECK(!x10_warthogs_run_shortened(syntax, NULL, string_bytes, other_run), "no strings changed");
		CHECK(nodes_unchanged(), "changed");
		return 0;
	}
	/* the two drivers placed with Johnson's variant are placed with none;
	Johnson himself, another marine, and a driver of another variant are
	left as they are */
	CASE("drivers")
	{
		struct scenario_object_datum hog_2_driver = placed(100);
		struct scenario_object_datum hog_3_driver = placed(100);
		struct scenario_object_datum johnson = placed(100);
		struct scenario_object_datum hog_1_driver = placed(0);
		struct scenario_object_datum helmeted = placed(12);

		CHECK(x10_warthogs_driver_placed("hog_2_driver", &hog_2_driver) && hog_2_driver.variant_number == 0,
			"hog_2_driver's variant %d", hog_2_driver.variant_number);
		CHECK(x10_warthogs_driver_placed("hog_3_driver", &hog_3_driver) && hog_3_driver.variant_number == 0,
			"hog_3_driver's variant %d", hog_3_driver.variant_number);
		CHECK(!x10_warthogs_driver_placed("hog_2_driver", &hog_2_driver), "placed again");
		CHECK(!x10_warthogs_driver_placed("johnson", &johnson) && johnson.variant_number == 100,
			"Johnson's variant %d", johnson.variant_number);
		CHECK(!x10_warthogs_driver_placed("hog_1_driver", &hog_1_driver) && hog_1_driver.variant_number == 0,
			"hog_1_driver's variant %d", hog_1_driver.variant_number);
		CHECK(!x10_warthogs_driver_placed("hog_2_driver", &helmeted) && helmeted.variant_number == 12,
			"a helmeted driver's variant %d", helmeted.variant_number);
		CHECK(!x10_warthogs_driver_placed(NULL, &johnson) && !x10_warthogs_driver_placed("hog_2_driver", NULL),
			"nothing placed");
		return 0;
	}

	return 2;
}
