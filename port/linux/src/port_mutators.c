/*
PORT_MUTATORS.C

The mutators (port_mutators.h): the stock mutators, then the folders of the
data root's mutators folder in their names' alphabetical order, each with a
mutator.toml. The stock mutators are embedded in the game (mutator_files.h,
from port/assets/mutators); each scan writes them into the data root's
stock_mutators folder, in place of what is there, so that they can be read
and copied, and plays them as the game has them (as it does when they cannot
be written). A mutator is only data: it names effects the game has
values of tags it sets (port/linux/game/mutator_tags.c), nodes of objects it
poses (port/linux/game/object_pose.c), an add-on and a script. The game checks
each tag edit and pose against the fields and groups it can set
(mutators_tag_edit_check), and each effect against those built in, of which it
has none (mutators_key_check): a mutator with one it does not have, or a value
out of its range, cannot be played, and the Mods menu says why.

mutator.toml (all but effects or tags may be left out):

	name = "Big head"             # what the menu shows (else its folder's name)
	id = "big_head"               # what game.mutators names it by (else its folder's name)
	version = "1.0"
	authors = ["...", "..."]      # or one string
	description = "..."
	format = 1                    # the mutators' format it is made for

	[[pose]]                      # a node of objects it scales and moves
	objects = "biped"             # a group of them, by object_pose.c's name
	tag = '...'                   # one tag of it (else all of them)
	node = "head"                 # a node of their models, or "head", a biped's
	scale = 2.5                   # (else 1)
	offset = [0, 0, -0.1]         # in its parent's frame, world units (else none)

	[[tags]]                      # a tag it edits
	group = "weapon"              # its group, by mutator_tags.c's name
	tag = 'weapons\pistol\pistol' # its name
	zoom_level_count = 1          # a field and its value (a number, or text)
	[tags.triggers.0]             # a block's element (by index, or name)
	rounds_between_tracers = 5
	crosshairs = { from = '...', elements = [1, 2], append = true }
	                              # a block's elements, copied from another tag's

Each edit is kept as the bytes mutator_tags.c reads (its header says what
they are).
The mutators on are game.mutators's (their ids, separated by commas), which
the Mods menu's MUTATORS list turns on and off. A mutator whose id one found
before it has is played by another (mutator_add): the same mutator again
(its files' SHA-256 the same) is played once; another is played by its id
and its folder's name (big_head@my_big_head), or where its folder has that
one's name or that is taken too, its id and the start of its files' SHA-256
(big_head@3fa2c1). Its files are not changed, and it is played by that id
for as long as its folder and files are. Where two mutators that are on set
one parameter, the one found later sets it. On a client the host's are
played instead (port/linux/game/network_mutators.c).
*/

#include "platform.h"
#include "port_config.h"
#include "mutator_files.h"
#include "port_mutators.h"
#include "posix.h"
#include "tomlc17.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

/* the mutators' format this build plays (2: tag edits; 3: an add-on and its
placements; 4: a weapon's; 6: poses; 7: decals, "all" elements, options and
[death]) */
#define MUTATORS_FORMAT 7
#define MAXIMUM_SCRIPT_FILE_SIZE 0x40000UL

#define MAXIMUM_MUTATORS 64
#define MAXIMUM_MUTATOR_FOLDER_LENGTH 64
/* an id, ended: its mutator.toml's, an @ and its folder's name */
#define MAXIMUM_MUTATOR_ID_LENGTH (2 * MAXIMUM_MUTATOR_FOLDER_LENGTH)
/* SHA-256's (p2p_crypto.c's), and the hex digits of it that an id is given */
#define SHA256_SIZE 32
#define ID_DIGEST_DIGITS 6
/* the most effects and parameters one mutator sets */
#define MAXIMUM_KEYS_PER_MUTATOR 16
/* the largest mutator.toml read (a mutator is a few lines) */
#define MAXIMUM_MUTATOR_FILE_SIZE 65536
#define MAXIMUM_PATH_LENGTH 1024
#define MAXIMUM_REFUSAL_LENGTH 160
/* the most bytes of tag edits a game plays, and one mutator has
(mutators.h's MAXIMUM_MUTATOR_TAG_EDIT_BYTES) */
#define MAXIMUM_TAG_EDIT_BYTES 32768
/* a tag edit's group's, tag's, path's and text's most bytes, ended, and its
copy's most elements (mutator_tags.c's) */
#define TAG_EDIT_GROUP_LENGTH 32
#define TAG_EDIT_NAME_LENGTH 256
#define TAG_EDIT_PATH_LENGTH 64
#define TAG_EDIT_TEXT_LENGTH 256
#define MAXIMUM_TAG_EDIT_ELEMENTS 64
/* the most placements one mutator has */
#define MAXIMUM_PLACES_PER_MUTATOR 16
/* the most options one mutator has, the bytes of an option's id and name,
and the most of its tag edits' numbers they scale */
#define MAXIMUM_OPTIONS_PER_MUTATOR 4
#define OPTION_ID_LENGTH 32
#define OPTION_NAME_LENGTH 32
#define MAXIMUM_SCALED_EDITS_PER_MUTATOR 128
/* the largest add-on file played (docs/addons.md) */
#define MAXIMUM_ADDON_FILE_SIZE (64UL * 1024 * 1024)

/* a tag edit's kind (mutators.h's) */
enum
{
	_tag_edit_number,
	_tag_edit_text,
	_tag_edit_copy,
	_tag_edit_pose,
	_tag_edit_death,
	_tag_edit_explosion,
};

/* ---------- structures */

/* an option ([[option]]): a percent the host sets in MUTATOR SETTINGS, which
scales the numbers of its mutator's tag edits that name it */
struct port_mutator_option
{
	char id[OPTION_ID_LENGTH];
	char name[OPTION_NAME_LENGTH];
	long minimum;
	long maximum;
	long step;
	long fallback;
};

/* a tag edit's number scaled by an option: where its number is in the
mutator's tag edits' bytes, the option, and the number at 100% */
struct port_mutator_scaled_edit
{
	long offset;
	short option;
	double number;
};

struct port_mutator
{
	char folder[MAXIMUM_MUTATOR_FOLDER_LENGTH];
	/* what game.mutators names it by, and its mutator.toml's id (else its
	folder's name), which differ where another mutator has that */
	char id[MAXIMUM_MUTATOR_ID_LENGTH];
	char given_id[MAXIMUM_MUTATOR_FOLDER_LENGTH];
	/* the mutator found before it with its given_id, whose id it was not
	played by, else empty */
	char taken_by[MAXIMUM_MUTATOR_FOLDER_LENGTH + 32];
	char name[128];
	char version[32];
	char authors[128];
	char description[256];
	/* whether it is a stock mutator (mutator_files.h's) */
	int stock;
	int on;
	/* why it cannot be played, or empty */
	char refusal[MAXIMUM_REFUSAL_LENGTH];
	long key_count;
	char keys[MAXIMUM_KEYS_PER_MUTATOR][PORT_MUTATOR_KEY_LENGTH];
	double values[MAXIMUM_KEYS_PER_MUTATOR];
	/* its tag edits' bytes (allocated), how many bytes and edits, and how
	many tags they are of */
	unsigned char *tag_edits;
	long tag_edit_size;
	long tag_edit_count;
	long tag_count;
	/* of its edits, how many are poses */
	long pose_count;
	/* its add-on's file (docs/addons.md), else empty, and where it places
	the add-on's vehicles */
	char addon_path[MAXIMUM_PATH_LENGTH];
	int addon_kind;
	long place_count;
	struct port_mutator_place places[MAXIMUM_PLACES_PER_MUTATOR];
	/* its options, and its tag edits' numbers they scale */
	long option_count;
	struct port_mutator_option options[MAXIMUM_OPTIONS_PER_MUTATOR];
	long scaled_edit_count;
	struct port_mutator_scaled_edit scaled_edits[MAXIMUM_SCALED_EDITS_PER_MUTATOR];
	/* its Lua script's file (docs/mutators.md, "Scripts"), else empty, and on a client,
	whether its player refused to run a host's */
	char script_path[MAXIMUM_PATH_LENGTH];
	int script_refused;
	/* its mutator.toml's SHA-256, and its files' (mutator_digest): the
	SHA-256 of its mutator.toml's and its add-on's file's, one after the
	other; digest_state is 0 until that is made, 1 once it is, -1 if a file
	cannot be read */
	unsigned char toml_digest[SHA256_SIZE];
	int toml_digest_made;
	unsigned char digest[SHA256_SIZE];
	int digest_state;
};

/* ---------- globals */

/* what the stock_mutators folder's README.txt says */
static const char stock_mutators_readme[] =
	"The stock mutators, as this build of the game has them.\n"
	"\n"
	"The game writes these folders again each time it starts, in place of what\n"
	"is here, and plays them as it has them: a change made here is lost, and a\n"
	"folder added here is not read. To make a mutator of your own, copy one of\n"
	"these folders into the mutators folder beside this one, and change it. It\n"
	"is played by an id of its own: the id in its mutator.toml, else, while a\n"
	"stock mutator has that, the id and its folder's name (big_head@my_head).\n";

static struct
{
	struct port_mutator mutators[MAXIMUM_MUTATORS];
	long count;
	unsigned long scans;
	/* whether the stock mutators' writing failing has been logged */
	int stock_unwritten_logged;
} port_mutators;

#define MAXIMUM_SHARED_MUTATORS 8

/* on a client: the host's mutators with files, played in place of its own */
static struct port_mutator host_mutators[MAXIMUM_SHARED_MUTATORS];
static long host_mutator_count;
static int host_mutators_on;

/* ---------- prototypes/MUTATORS.C (port/linux/game/mutators.h's) */

int mutators_key_check(const char *key, double value, char *reason, unsigned long size);
int mutators_tag_edit_check(const unsigned char *edit, long size, char *reason, unsigned long reason_size);

/* ---------- prototypes/P2P_CRYPTO.C (p2p_internal.h's) */

void p2p_sha256(const void *data, int size, unsigned char *digest);

/* ---------- private code */

static char lower(char character)
{
	return character >= 'A' && character <= 'Z' ? (char)(character - 'A' + 'a') : character;
}

static int names_compare(const char *first, const char *second)
{
	for (; *first && lower(*first) == lower(*second); first++, second++)
	{
	}
	return (unsigned char)lower(*first) - (unsigned char)lower(*second);
}

/* text as mutator.toml has it, its control characters (but its line breaks,
if `lines`) spaces, its last character whole: what the menus show of it */
/* a row of the Controls Setup screen a mutator's script asks for */
struct port_mutator_control
{
	long mutator;
	char name[32];
	char label[32];
	char value[64];
	char fallback[64];
};

typedef void (*port_mutators_setting_proc)(void *context, const char *name, const char *text, double number,
	int boolean_value, int kind);

static void text_copy(char *destination, size_t size, const char *source, int lines)
{
	size_t length;
	char *text;

	snprintf(destination, size, "%s", source ? source : "");
	length = strlen(destination);
	/* (a UTF-8 character cut short at the end dropped) */
	while (length > 0 && ((unsigned char)destination[length - 1] & 0xC0) == 0x80)
		length--;
	if (length > 0 && (unsigned char)destination[length - 1] >= 0xC0)
	{
		unsigned char first = (unsigned char)destination[length - 1];
		size_t need = first >= 0xF0 ? 4 : first >= 0xE0 ? 3 : 2;

		if (strlen(destination) - (length - 1) < need)
			destination[length - 1] = 0;
	}
	for (text = destination; *text; text++)
	{
		if (((unsigned char)*text < 0x20 && !(lines && *text == '\n')) || *text == 0x7F)
			*text = ' ';
	}
}

/* whether an id can be named in game.mutators: printable, and no comma or
quote */
static int id_valid(const char *id)
{
	if (!id[0])
		return 0;
	for (; *id; id++)
	{
		if ((unsigned char)*id < 0x20 || *id == 0x7F || *id == ',' || *id == '"' || *id == '\\' || *id == '@')
			return 0;
	}
	return 1;
}

static void refuse(struct port_mutator *mutator, const char *path, const char *reason)
{
	if (!mutator->refusal[0])
		snprintf(mutator->refusal, sizeof(mutator->refusal), "%s", reason);
	platform_log("mutators: %s: %s: it cannot be played", path, reason);
}

/* a string key of mutator.toml, or NULL (and refused when it is not one) */
static const char *toml_text(toml_datum_t table, const char *key, struct port_mutator *mutator, const char *path)
{
	toml_datum_t value = toml_get(table, key);
	char reason[MAXIMUM_REFUSAL_LENGTH];

	if (value.type == TOML_UNKNOWN)
		return NULL;
	if (value.type != TOML_STRING)
	{
		snprintf(reason, sizeof(reason), "line %d: %s is not text", value.lineno, key);
		refuse(mutator, path, reason);
		return NULL;
	}
	return value.u.s;
}

static void key_add(struct port_mutator *mutator, const char *path, const char *key, double value)
{
	char reason[MAXIMUM_REFUSAL_LENGTH];

	if (strlen(key) >= PORT_MUTATOR_KEY_LENGTH || !mutators_key_check(key, value, reason, sizeof(reason)))
	{
		if (strlen(key) >= PORT_MUTATOR_KEY_LENGTH)
			snprintf(reason, sizeof(reason), "this game has no effect or parameter %.40s...", key);
		refuse(mutator, path, reason);
		return;
	}
	if (mutator->key_count == MAXIMUM_KEYS_PER_MUTATOR)
	{
		snprintf(reason, sizeof(reason), "it sets more than %d effects and parameters", MAXIMUM_KEYS_PER_MUTATOR);
		refuse(mutator, path, reason);
		return;
	}
	snprintf(mutator->keys[mutator->key_count], PORT_MUTATOR_KEY_LENGTH, "%s", key);
	mutator->values[mutator->key_count++] = value;
}

/* [effects.<name>] and its parameters: each effect's key, then its
parameters' ("<name>", "<name>.<parameter>"), checked by the game, which has
no effects built in (mutators_key_check) */
static void effects_read(struct port_mutator *mutator, toml_datum_t effects, const char *path)
{
	char reason[MAXIMUM_REFUSAL_LENGTH];
	int effect;

	if (effects.type != TOML_TABLE)
	{
		snprintf(reason, sizeof(reason), "line %d: effects is not a table of effects", effects.lineno);
		refuse(mutator, path, reason);
		return;
	}
	for (effect = 0; effect < effects.u.tab.size; effect++)
	{
		const char *name = effects.u.tab.key[effect];
		toml_datum_t parameters = effects.u.tab.value[effect];
		int parameter;

		if (parameters.type != TOML_TABLE)
		{
			snprintf(reason, sizeof(reason), "line %d: effects.%.40s is not a table of parameters", parameters.lineno,
				name);
			refuse(mutator, path, reason);
			return;
		}
		key_add(mutator, path, name, 1.0);
		for (parameter = 0; parameter < parameters.u.tab.size; parameter++)
		{
			toml_datum_t value = parameters.u.tab.value[parameter];
			char key[2 * PORT_MUTATOR_KEY_LENGTH];
			double number;

			if (value.type == TOML_FP64)
				number = value.u.fp64;
			else if (value.type == TOML_INT64)
				number = (double)value.u.int64;
			else if (value.type == TOML_BOOLEAN)
				number = value.u.boolean ? 1.0 : 0.0;
			else
			{
				snprintf(reason, sizeof(reason), "line %d: %.24s.%.24s is not a number", value.lineno, name,
					parameters.u.tab.key[parameter]);
				refuse(mutator, path, reason);
				return;
			}
			snprintf(key, sizeof(key), "%s.%s", name, parameters.u.tab.key[parameter]);
			key_add(mutator, path, key, number);
		}
	}
	if (!mutator->key_count)
		refuse(mutator, path, "it turns on no effect");
}

/* a tag edit: its bytes (mutator_tags.c's), checked, after the mutator's */
static void tag_edit_add(struct port_mutator *mutator, const char *path, int line, const char *group, const char *tag,
	const char *field, int kind, double number, const char *text, const long *elements, int element_count, int append)
{
	unsigned char edit[8 + TAG_EDIT_GROUP_LENGTH + TAG_EDIT_NAME_LENGTH + TAG_EDIT_PATH_LENGTH + TAG_EDIT_TEXT_LENGTH +
		2 * MAXIMUM_TAG_EDIT_ELEMENTS];
	char reason[MAXIMUM_REFUSAL_LENGTH];
	char check[MAXIMUM_REFUSAL_LENGTH - 16];
	const char *texts[4];
	float value = (float)number;
	unsigned long bits;
	unsigned char *grown;
	long size = 8;
	int index;

	texts[0] = group;
	texts[1] = tag;
	texts[2] = field;
	texts[3] = text ? text : "";
	if (strlen(group) >= TAG_EDIT_GROUP_LENGTH || strlen(tag) >= TAG_EDIT_NAME_LENGTH ||
		strlen(field) >= TAG_EDIT_PATH_LENGTH || strlen(texts[3]) >= TAG_EDIT_TEXT_LENGTH ||
		element_count > MAXIMUM_TAG_EDIT_ELEMENTS)
	{
		snprintf(reason, sizeof(reason), "line %d: a tag edit's name, field or value is too long", line);
		refuse(mutator, path, reason);
		return;
	}
	memcpy(&bits, &value, sizeof(bits));
	edit[0] = (unsigned char)kind;
	edit[1] = (unsigned char)element_count;
	edit[2] = (unsigned char)(append != 0);
	edit[3] = 0;
	edit[4] = (unsigned char)bits;
	edit[5] = (unsigned char)(bits >> 8);
	edit[6] = (unsigned char)(bits >> 16);
	edit[7] = (unsigned char)(bits >> 24);
	for (index = 0; index < 4; index++)
	{
		size_t length = strlen(texts[index]) + 1;

		memcpy(edit + size, texts[index], length);
		size += (long)length;
	}
	for (index = 0; index < element_count; index++)
	{
		edit[size++] = (unsigned char)elements[index];
		edit[size++] = (unsigned char)(elements[index] >> 8);
	}
	if (!mutators_tag_edit_check(edit, size, check, sizeof(check)))
	{
		snprintf(reason, sizeof(reason), "line %d: %s", line, check);
		refuse(mutator, path, reason);
		return;
	}
	if (mutator->tag_edit_size + size > MAXIMUM_TAG_EDIT_BYTES)
	{
		refuse(mutator, path, "its tag edits are more than 32 KB");
		return;
	}
	grown = realloc(mutator->tag_edits, (size_t)(mutator->tag_edit_size + size));
	if (!grown)
	{
		refuse(mutator, path, "its tag edits do not fit in memory");
		return;
	}
	mutator->tag_edits = grown;
	memcpy(mutator->tag_edits + mutator->tag_edit_size, edit, (size_t)size);
	mutator->tag_edit_size += size;
	mutator->tag_edit_count++;
}

/* a copy's table: { from = '<tag>', elements = [...], append = true } */
static void tag_copy_read(struct port_mutator *mutator, const char *path, const char *group, const char *tag,
	const char *field, toml_datum_t copy)
{
	long elements[MAXIMUM_TAG_EDIT_ELEMENTS];
	char reason[MAXIMUM_REFUSAL_LENGTH];
	toml_datum_t from = toml_get(copy, "from");
	toml_datum_t list = toml_get(copy, "elements");
	toml_datum_t append = toml_get(copy, "append");
	int element_count = 0;
	int index;

	for (index = 0; index < copy.u.tab.size; index++)
	{
		const char *key = copy.u.tab.key[index];

		if (strcmp(key, "from") && strcmp(key, "elements") && strcmp(key, "append"))
		{
			snprintf(reason, sizeof(reason), "line %d: a copy has from, elements and append, not %.40s", copy.lineno,
				key);
			refuse(mutator, path, reason);
			return;
		}
	}
	if (from.type != TOML_STRING || (list.type != TOML_UNKNOWN && list.type != TOML_ARRAY) ||
		(append.type != TOML_UNKNOWN && append.type != TOML_BOOLEAN))
	{
		snprintf(reason, sizeof(reason), "line %d: a copy's from is a tag's name, elements a list of numbers, append "
			"true or false", copy.lineno);
		refuse(mutator, path, reason);
		return;
	}
	if (list.type == TOML_ARRAY)
	{
		for (index = 0; index < list.u.arr.size; index++)
		{
			toml_datum_t element = list.u.arr.elem[index];

			if (element.type != TOML_INT64 || element.u.int64 < 0 || element.u.int64 > 32767 ||
				element_count == MAXIMUM_TAG_EDIT_ELEMENTS)
			{
				snprintf(reason, sizeof(reason), "line %d: a copy's elements are at most %d numbers from 0", copy.lineno,
					MAXIMUM_TAG_EDIT_ELEMENTS);
				refuse(mutator, path, reason);
				return;
			}
			elements[element_count++] = (long)element.u.int64;
		}
	}
	tag_edit_add(mutator, path, copy.lineno, group, tag, field, _tag_edit_copy, 0.0, from.u.s, elements, element_count,
		append.type == TOML_BOOLEAN && append.u.boolean);
}

static int toml_number(toml_datum_t table, const char *key, int required, double *number,
	struct port_mutator *mutator, const char *path);

/* a 4-byte float of the tag edits' bytes, written (tag_edit_add's order) */
static void edit_number_write(unsigned char *bytes, double number)
{
	float value = (float)number;
	unsigned long bits;

	memcpy(&bits, &value, sizeof(bits));
	bytes[0] = (unsigned char)bits;
	bytes[1] = (unsigned char)(bits >> 8);
	bytes[2] = (unsigned char)(bits >> 16);
	bytes[3] = (unsigned char)(bits >> 24);
}

/* a number scaled by an option: { value = <the number at 100%>, option = '<id>' }, checked at the
option's least and most */
static void tag_option_read(struct port_mutator *mutator, const char *path, const char *group, const char *tag,
	const char *field, toml_datum_t table)
{
	char reason[MAXIMUM_REFUSAL_LENGTH];
	char check[MAXIMUM_REFUSAL_LENGTH - 16];
	toml_datum_t option = toml_get(table, "option");
	double number;
	long offset = mutator->tag_edit_size;
	long edits = mutator->tag_edit_count;
	const struct port_mutator_option *found = NULL;
	struct port_mutator_scaled_edit *scaled;
	int index;

	for (index = 0; index < table.u.tab.size; index++)
	{
		if (strcmp(table.u.tab.key[index], "value") && strcmp(table.u.tab.key[index], "option"))
		{
			snprintf(reason, sizeof(reason), "line %d: a number an option scales has value and option, not %.40s",
				table.lineno, table.u.tab.key[index]);
			refuse(mutator, path, reason);
			return;
		}
	}
	if (!toml_number(table, "value", 1, &number, mutator, path))
		return;
	for (index = 0; option.type == TOML_STRING && index < mutator->option_count; index++)
	{
		if (!strcmp(mutator->options[index].id, option.u.s))
			found = &mutator->options[index];
	}
	if (!found)
	{
		snprintf(reason, sizeof(reason), "line %d: %.40s's option is not one of its [[option]]s", table.lineno, field);
		refuse(mutator, path, reason);
		return;
	}
	if (mutator->scaled_edit_count == MAXIMUM_SCALED_EDITS_PER_MUTATOR)
	{
		snprintf(reason, sizeof(reason), "line %d: options scale at most %d numbers", table.lineno,
			MAXIMUM_SCALED_EDITS_PER_MUTATOR);
		refuse(mutator, path, reason);
		return;
	}
	tag_edit_add(mutator, path, table.lineno, group, tag, field, _tag_edit_number, number * found->fallback / 100.0,
		NULL, NULL, 0, 0);
	if (mutator->tag_edit_count == edits)
		return;
	/* (at the option's least and most too) */
	for (index = 0; index < 2; index++)
	{
		edit_number_write(mutator->tag_edits + offset + 4,
			number * (index ? found->maximum : found->minimum) / 100.0);
		if (!mutators_tag_edit_check(mutator->tag_edits + offset, mutator->tag_edit_size - offset, check,
			sizeof(check)))
		{
			snprintf(reason, sizeof(reason), "line %d: at %ld%%, %s", table.lineno,
				index ? found->maximum : found->minimum, check);
			refuse(mutator, path, reason);
			return;
		}
	}
	edit_number_write(mutator->tag_edits + offset + 4, number * found->fallback / 100.0);
	scaled = &mutator->scaled_edits[mutator->scaled_edit_count++];
	scaled->offset = offset + 4;
	scaled->option = (short)(found - mutator->options);
	scaled->number = number;
}

/* a [[tags]] table's fields, those of its tables' as paths ("triggers.0.x") */
static void tag_fields_read(struct port_mutator *mutator, const char *path, const char *group, const char *tag,
	const char *prefix, toml_datum_t table)
{
	char reason[MAXIMUM_REFUSAL_LENGTH];
	int index;

	for (index = 0; index < table.u.tab.size; index++)
	{
		const char *key = table.u.tab.key[index];
		toml_datum_t value = table.u.tab.value[index];
		char field[TAG_EDIT_PATH_LENGTH + 1];

		if (!prefix[0] && (!strcmp(key, "group") || !strcmp(key, "tag") || !strcmp(key, "where")))
			continue;
		if (strlen(prefix) + strlen(key) + 2 > sizeof(field))
		{
			snprintf(reason, sizeof(reason), "line %d: the field %.40s... is not one a tag has", value.lineno, prefix);
			refuse(mutator, path, reason);
			return;
		}
		snprintf(field, sizeof(field), "%s%s%s", prefix, prefix[0] ? "." : "", key);
		if (value.type == TOML_TABLE)
		{
			if (toml_get(value, "from").type != TOML_UNKNOWN)
				tag_copy_read(mutator, path, group, tag, field, value);
			else if (toml_get(value, "option").type != TOML_UNKNOWN)
				tag_option_read(mutator, path, group, tag, field, value);
			else
				tag_fields_read(mutator, path, group, tag, field, value);
		}
		else if (value.type == TOML_STRING)
		{
			tag_edit_add(mutator, path, value.lineno, group, tag, field, _tag_edit_text, 0.0, value.u.s, NULL, 0, 0);
		}
		else if (value.type == TOML_ARRAY)
		{
			/* (names, a field's flags: separated by commas) */
			char names[TAG_EDIT_TEXT_LENGTH] = "";
			int name;

			for (name = 0; name < value.u.arr.size; name++)
			{
				size_t length = strlen(names);

				if (value.u.arr.elem[name].type != TOML_STRING)
				{
					snprintf(reason, sizeof(reason), "line %d: %.40s is a list of names", value.lineno, field);
					refuse(mutator, path, reason);
					return;
				}
				snprintf(names + length, sizeof(names) - length, "%s%s", length ? "," : "", value.u.arr.elem[name].u.s);
			}
			tag_edit_add(mutator, path, value.lineno, group, tag, field, _tag_edit_text, 0.0, names, NULL, 0, 0);
		}
		else if (value.type == TOML_INT64 || value.type == TOML_FP64 || value.type == TOML_BOOLEAN)
		{
			double number = value.type == TOML_INT64 ? (double)value.u.int64 :
				value.type == TOML_FP64 ? value.u.fp64 : (value.u.boolean ? 1.0 : 0.0);

			tag_edit_add(mutator, path, value.lineno, group, tag, field, _tag_edit_number, number, NULL, NULL, 0, 0);
		}
		else
		{
			snprintf(reason, sizeof(reason), "line %d: %.40s is not a number, text or table", value.lineno, field);
			refuse(mutator, path, reason);
			return;
		}
	}
}

/* [[tags]]: each a tag's group and name, and the fields it sets */
static void tags_read(struct port_mutator *mutator, toml_datum_t tags, const char *path)
{
	char reason[MAXIMUM_REFUSAL_LENGTH];
	int index;

	if (tags.type != TOML_ARRAY)
	{
		snprintf(reason, sizeof(reason), "line %d: tags is not a list of tables ([[tags]])", tags.lineno);
		refuse(mutator, path, reason);
		return;
	}
	for (index = 0; index < tags.u.arr.size; index++)
	{
		toml_datum_t tag = tags.u.arr.elem[index];
		toml_datum_t group;
		toml_datum_t name;

		if (tag.type != TOML_TABLE)
		{
			snprintf(reason, sizeof(reason), "line %d: tags is not a list of tables ([[tags]])", tags.lineno);
			refuse(mutator, path, reason);
			return;
		}
		group = toml_get(tag, "group");
		name = toml_get(tag, "tag");
		if (group.type != TOML_STRING || name.type != TOML_STRING)
		{
			snprintf(reason, sizeof(reason), "line %d: a [[tags]] names its group and its tag", tag.lineno);
			refuse(mutator, path, reason);
			return;
		}
		/* (every tag of the group, with tag = '*', of those whose field has
		one of the values where names: tag_edit_add's "*?type=scratch,burn") */
		if (toml_get(tag, "where").type != TOML_UNKNOWN)
		{
			toml_datum_t where = toml_get(tag, "where");
			char every[TAG_EDIT_NAME_LENGTH];
			int value;

			/* (projectiles a weapon fires: where = { fired = true }) */
			if (!strcmp(name.u.s, "*") && where.type == TOML_TABLE && where.u.tab.size == 1 &&
				!strcmp(where.u.tab.key[0], "fired") && where.u.tab.value[0].type == TOML_BOOLEAN &&
				where.u.tab.value[0].u.boolean)
			{
				tag_fields_read(mutator, path, group.u.s, "*?fired", "", tag);
				mutator->tag_count++;
				continue;
			}
			if (strcmp(name.u.s, "*") || where.type != TOML_TABLE || where.u.tab.size != 1 ||
				where.u.tab.value[0].type != TOML_ARRAY)
			{
				snprintf(reason, sizeof(reason), "line %d: a where is of tag = '*': a field and a list of its values",
					tag.lineno);
				refuse(mutator, path, reason);
				return;
			}
			snprintf(every, sizeof(every), "*?%s=", where.u.tab.key[0]);
			for (value = 0; value < where.u.tab.value[0].u.arr.size; value++)
			{
				toml_datum_t one = where.u.tab.value[0].u.arr.elem[value];
				size_t length = strlen(every);

				if (one.type != TOML_STRING)
				{
					snprintf(reason, sizeof(reason), "line %d: a where's values are names", tag.lineno);
					refuse(mutator, path, reason);
					return;
				}
				snprintf(every + length, sizeof(every) - length, "%s%s", value ? "," : "", one.u.s);
			}
			tag_fields_read(mutator, path, group.u.s, every, "", tag);
		}
		else
			tag_fields_read(mutator, path, group.u.s, name.u.s, "", tag);
		mutator->tag_count++;
	}
}

/* a mutator from its mutator.toml's text */
/* a number of a table, or refused (when it is there and is not one, or
`required` and it is not there) */
static int toml_number(toml_datum_t table, const char *key, int required, double *number,
	struct port_mutator *mutator, const char *path)
{
	toml_datum_t value = toml_get(table, key);
	char reason[MAXIMUM_REFUSAL_LENGTH];

	if (value.type == TOML_INT64)
		*number = (double)value.u.int64;
	else if (value.type == TOML_FP64)
		*number = value.u.fp64;
	else if (value.type != TOML_UNKNOWN || required)
	{
		snprintf(reason, sizeof(reason), "line %d: %s is %s", value.lineno, key,
			value.type == TOML_UNKNOWN ? "missing" : "not a number");
		refuse(mutator, path, reason);
		return 0;
	}
	else
		return 0;
	return 1;
}

/* [[option]]: each a percent the host sets (MUTATOR SETTINGS), from its
least to its most by its step, which scales the numbers that name it */
static void options_read(struct port_mutator *mutator, toml_datum_t options, const char *path)
{
	static const char *const keys[] = { "id", "name", "default", "minimum", "maximum", "step" };
	char reason[MAXIMUM_REFUSAL_LENGTH];
	int index;

	if (options.type != TOML_ARRAY)
	{
		snprintf(reason, sizeof(reason), "line %d: option is not a list of tables ([[option]])", options.lineno);
		refuse(mutator, path, reason);
		return;
	}
	for (index = 0; index < options.u.arr.size; index++)
	{
		toml_datum_t table = options.u.arr.elem[index];
		struct port_mutator_option *option;
		double minimum = 0.0, maximum = 0.0, step = 5.0, fallback = 100.0;
		int key;

		if (table.type != TOML_TABLE || mutator->option_count == MAXIMUM_OPTIONS_PER_MUTATOR)
		{
			snprintf(reason, sizeof(reason), "line %d: option is a list of at most %d tables ([[option]])",
				options.lineno, MAXIMUM_OPTIONS_PER_MUTATOR);
			refuse(mutator, path, reason);
			return;
		}
		for (key = 0; key < table.u.tab.size; key++)
		{
			size_t known;

			for (known = 0; known < sizeof(keys) / sizeof(keys[0]); known++)
			{
				if (!strcmp(table.u.tab.key[key], keys[known]))
					break;
			}
			if (known == sizeof(keys) / sizeof(keys[0]))
			{
				snprintf(reason, sizeof(reason), "line %d: an option has no key %.40s", table.u.tab.value[key].lineno,
					table.u.tab.key[key]);
				refuse(mutator, path, reason);
				return;
			}
		}
		if (toml_get(table, "id").type != TOML_STRING || toml_get(table, "name").type != TOML_STRING ||
			!toml_get(table, "id").u.s[0] || strlen(toml_get(table, "id").u.s) >= OPTION_ID_LENGTH ||
			strchr(toml_get(table, "id").u.s, '.') || strchr(toml_get(table, "id").u.s, '=') ||
			strchr(toml_get(table, "id").u.s, ','))
		{
			snprintf(reason, sizeof(reason), "line %d: an option has an id (no . = or ,) and a name", table.lineno);
			refuse(mutator, path, reason);
			return;
		}
		if (!toml_number(table, "minimum", 1, &minimum, mutator, path) ||
			!toml_number(table, "maximum", 1, &maximum, mutator, path) ||
			(toml_get(table, "step").type != TOML_UNKNOWN && !toml_number(table, "step", 0, &step, mutator, path)) ||
			(toml_get(table, "default").type != TOML_UNKNOWN &&
			!toml_number(table, "default", 0, &fallback, mutator, path)))
		{
			return;
		}
		/* (whole percents, at most 1000%, the default one of the steps) */
		if (!(minimum >= 0.0 && minimum < maximum && maximum <= 1000.0 && step >= 1.0 && fallback >= minimum &&
			fallback <= maximum) || minimum != (long)minimum || maximum != (long)maximum || step != (long)step ||
			fallback != (long)fallback || ((long)fallback - (long)minimum) % (long)step ||
			((long)maximum - (long)minimum) % (long)step)
		{
			snprintf(reason, sizeof(reason), "line %d: an option is whole percents from its minimum to its maximum "
				"(at most 1000) by its step, its default one of them", table.lineno);
			refuse(mutator, path, reason);
			return;
		}
		option = &mutator->options[mutator->option_count++];
		text_copy(option->id, sizeof(option->id), toml_get(table, "id").u.s, 0);
		text_copy(option->name, sizeof(option->name), toml_get(table, "name").u.s, 0);
		option->minimum = (long)minimum;
		option->maximum = (long)maximum;
		option->step = (long)step;
		option->fallback = (long)fallback;
	}
}

/* [death]: what a unit of a group (its objects, as a pose's) does as it dies, played on every machine:
an effect at it, and its body vanishing (kept as a tag edit's bytes, mutator_tags.c's header says how) */
static void death_read(struct port_mutator *mutator, toml_datum_t death, const char *path)
{
	static const char *const keys[] = { "objects", "effect", "body" };
	char reason[MAXIMUM_REFUSAL_LENGTH];
	toml_datum_t objects = toml_get(death, "objects");
	toml_datum_t effect = toml_get(death, "effect");
	toml_datum_t body = toml_get(death, "body");
	int key;

	if (death.type != TOML_TABLE)
	{
		snprintf(reason, sizeof(reason), "line %d: death is a table ([death])", death.lineno);
		refuse(mutator, path, reason);
		return;
	}
	for (key = 0; key < death.u.tab.size; key++)
	{
		size_t known;

		for (known = 0; known < sizeof(keys) / sizeof(keys[0]); known++)
		{
			if (!strcmp(death.u.tab.key[key], keys[known]))
				break;
		}
		if (known == sizeof(keys) / sizeof(keys[0]))
		{
			snprintf(reason, sizeof(reason), "line %d: [death] has no key %.40s", death.u.tab.value[key].lineno,
				death.u.tab.key[key]);
			refuse(mutator, path, reason);
			return;
		}
	}
	if (objects.type != TOML_STRING || (effect.type != TOML_UNKNOWN && effect.type != TOML_STRING) ||
		(body.type != TOML_UNKNOWN && body.type != TOML_STRING))
	{
		snprintf(reason, sizeof(reason), "line %d: [death] names its objects' group, and an effect and a body "
			"(\"vanish\" or \"stays\") if it has them", death.lineno);
		refuse(mutator, path, reason);
		return;
	}
	tag_edit_add(mutator, path, death.lineno, objects.u.s, effect.type == TOML_STRING ? effect.u.s : "", "",
		_tag_edit_death, 0.0, body.type == TOML_STRING ? body.u.s : "stays", NULL, 0, 0);
}

/* [[pose]]: each a node of the objects of a group (or one tag of it) scaled
and moved, kept as a tag edit's bytes (mutator_tags.c's header says how) */
static void poses_read(struct port_mutator *mutator, toml_datum_t poses, const char *path)
{
	static const char *const keys[] = { "objects", "tag", "node", "scale", "offset" };
	char reason[MAXIMUM_REFUSAL_LENGTH];
	int index;

	if (poses.type != TOML_ARRAY)
	{
		snprintf(reason, sizeof(reason), "line %d: pose is not a list of tables ([[pose]])", poses.lineno);
		refuse(mutator, path, reason);
		return;
	}
	for (index = 0; index < poses.u.arr.size; index++)
	{
		toml_datum_t pose = poses.u.arr.elem[index];
		toml_datum_t offset;
		long elements[3];
		double scale = 1.0;
		const char *tag;
		long edits;
		int element_count = 0;
		int key;

		if (pose.type != TOML_TABLE)
		{
			snprintf(reason, sizeof(reason), "line %d: pose is not a list of tables ([[pose]])", poses.lineno);
			refuse(mutator, path, reason);
			return;
		}
		for (key = 0; key < pose.u.tab.size; key++)
		{
			size_t known;

			for (known = 0; known < sizeof(keys) / sizeof(keys[0]); known++)
			{
				if (!strcmp(pose.u.tab.key[key], keys[known]))
					break;
			}
			if (known == sizeof(keys) / sizeof(keys[0]))
			{
				snprintf(reason, sizeof(reason), "line %d: a pose has no key %.40s", pose.u.tab.value[key].lineno,
					pose.u.tab.key[key]);
				refuse(mutator, path, reason);
				return;
			}
		}
		if (toml_get(pose, "objects").type != TOML_STRING || toml_get(pose, "node").type != TOML_STRING)
		{
			snprintf(reason, sizeof(reason), "line %d: a pose names its objects' group and a node", pose.lineno);
			refuse(mutator, path, reason);
			return;
		}
		tag = toml_get(pose, "tag").type == TOML_STRING ? toml_get(pose, "tag").u.s : "";
		if (toml_get(pose, "tag").type != TOML_UNKNOWN && toml_get(pose, "tag").type != TOML_STRING)
		{
			snprintf(reason, sizeof(reason), "line %d: a pose's tag is a tag's name", pose.lineno);
			refuse(mutator, path, reason);
			return;
		}
		if (toml_get(pose, "scale").type != TOML_UNKNOWN && !toml_number(pose, "scale", 0, &scale, mutator, path))
			return;
		offset = toml_get(pose, "offset");
		if (offset.type != TOML_UNKNOWN)
		{
			if (offset.type != TOML_ARRAY || offset.u.arr.size != 3)
			{
				snprintf(reason, sizeof(reason), "line %d: a pose's offset is three numbers (world units)",
					offset.lineno);
				refuse(mutator, path, reason);
				return;
			}
			for (element_count = 0; element_count < 3; element_count++)
			{
				toml_datum_t number = offset.u.arr.elem[element_count];
				double value;

				if (number.type == TOML_INT64)
					value = (double)number.u.int64;
				else if (number.type == TOML_FP64)
					value = number.u.fp64;
				else
				{
					snprintf(reason, sizeof(reason), "line %d: a pose's offset is three numbers (world units)",
						offset.lineno);
					refuse(mutator, path, reason);
					return;
				}
				/* (thousandths of a world unit, rounded; mutator_tags.c checks
				the range) */
				if (!(value > -1000.0 && value < 1000.0))
					value = 1000.0;
				elements[element_count] = (long)(value * 1000.0 + (value < 0.0 ? -0.5 : 0.5));
			}
		}
		edits = mutator->tag_edit_count;
		tag_edit_add(mutator, path, pose.lineno, toml_get(pose, "objects").u.s, tag, toml_get(pose, "node").u.s,
			_tag_edit_pose, scale, NULL, elements, element_count, 0);
		mutator->pose_count += mutator->tag_edit_count - edits;
	}
}

/* [[place]]: where the add-on's vehicles are placed, on which map */
static void places_read(struct port_mutator *mutator, toml_datum_t places, const char *path)
{
	static const char *const keys[] = { "map", "vehicle", "weapon", "position", "facing" };
	char reason[MAXIMUM_REFUSAL_LENGTH];
	int index;

	if (places.type != TOML_ARRAY)
	{
		refuse(mutator, path, "place is not a list of tables ([[place]])");
		return;
	}
	for (index = 0; index < places.u.arr.size; index++)
	{
		toml_datum_t table = places.u.arr.elem[index];
		struct port_mutator_place *place;
		toml_datum_t position;
		const char *tag;
		double facing = 0.;
		int key;
		int field;

		if (table.type != TOML_TABLE)
		{
			refuse(mutator, path, "place is not a list of tables ([[place]])");
			return;
		}
		if (mutator->place_count == MAXIMUM_PLACES_PER_MUTATOR)
		{
			snprintf(reason, sizeof(reason), "it places more than %d vehicles and weapons",
				MAXIMUM_PLACES_PER_MUTATOR);
			refuse(mutator, path, reason);
			return;
		}
		for (key = 0; key < table.u.tab.size; key++)
		{
			size_t known;

			for (known = 0; known < sizeof(keys) / sizeof(keys[0]); known++)
			{
				if (!strcmp(table.u.tab.key[key], keys[known]))
					break;
			}
			if (known == sizeof(keys) / sizeof(keys[0]))
			{
				snprintf(reason, sizeof(reason), "line %d: a place has no key %.40s", table.u.tab.value[key].lineno,
					table.u.tab.key[key]);
				refuse(mutator, path, reason);
			}
		}
		place = &mutator->places[mutator->place_count];
		memset(place, 0, sizeof(*place));
		/* (a vehicle or a weapon, not both) */
		place->weapon = toml_get(table, "weapon").type != TOML_UNKNOWN;
		tag = toml_text(table, place->weapon ? "weapon" : "vehicle", mutator, path);
		if (!toml_text(table, "map", mutator, path) || !tag ||
			(place->weapon && toml_get(table, "vehicle").type != TOML_UNKNOWN) ||
			strlen(toml_text(table, "map", mutator, path)) >= sizeof(place->map) || strlen(tag) >= sizeof(place->tag))
		{
			snprintf(reason, sizeof(reason), "line %d: a place needs a map and a vehicle or a weapon (a tag's name)",
				table.lineno);
			refuse(mutator, path, reason);
			continue;
		}
		snprintf(place->map, sizeof(place->map), "%s", toml_text(table, "map", mutator, path));
		snprintf(place->tag, sizeof(place->tag), "%s", tag);
		position = toml_get(table, "position");
		if (position.type != TOML_ARRAY || position.u.arr.size != 3)
		{
			snprintf(reason, sizeof(reason), "line %d: a place's position is three numbers (world units)", table.lineno);
			refuse(mutator, path, reason);
			continue;
		}
		for (field = 0; field < 3; field++)
		{
			toml_datum_t number = position.u.arr.elem[field];

			if (number.type == TOML_INT64)
				place->position[field] = (float)number.u.int64;
			else if (number.type == TOML_FP64 && number.u.fp64 > -100000. && number.u.fp64 < 100000.)
				place->position[field] = (float)number.u.fp64;
			else
			{
				snprintf(reason, sizeof(reason), "line %d: a place's position is three numbers (world units)",
					table.lineno);
				refuse(mutator, path, reason);
				break;
			}
		}
		if (field < 3)
			continue;
		if (toml_number(table, "facing", 0, &facing, mutator, path) && (facing < -360. || facing > 360.))
		{
			snprintf(reason, sizeof(reason), "line %d: a place's facing is -360 to 360 degrees", table.lineno);
			refuse(mutator, path, reason);
			continue;
		}
		place->facing = (float)facing;
		mutator->place_count++;
	}
}

/* whether the name ends with the suffix (not case-sensitive) */
static int name_ends(const char *name, const char *suffix)
{
	size_t length = strlen(name);

	return length > strlen(suffix) && !names_compare(name + length - strlen(suffix), suffix);
}

/* addon: the add-on file beside mutator.toml (a name alone: no folder) */
static void addon_read(struct port_mutator *mutator, const char *path, const char *name)
{
	struct posix_file_information information;
	size_t folder_length = strlen(path) - strlen("mutator.toml");

	if (!name[0] || strchr(name, '/') || strchr(name, '\\') || !strcmp(name, "..") || name[0] == '.')
	{
		refuse(mutator, path, "its addon is not a file's name beside its mutator.toml");
		return;
	}
	if (folder_length + strlen(name) >= sizeof(mutator->addon_path))
	{
		refuse(mutator, path, "its addon's path is too long");
		return;
	}
	/* (its kind, by its name: the game checks the file's when it adds it) */
	if (name_ends(name, ".cevehicle"))
		mutator->addon_kind = _port_mutator_addon_vehicle;
	else if (name_ends(name, ".ceweapon"))
		mutator->addon_kind = _port_mutator_addon_weapon;
	else
	{
		refuse(mutator, path, "its addon is not a .cevehicle or a .ceweapon");
		return;
	}
	snprintf(mutator->addon_path, sizeof(mutator->addon_path), "%.*s%s", (int)folder_length, path, name);
	if (posix_stat(mutator->addon_path, &information) != 0)
		refuse(mutator, path, "its addon's file is not there");
	else if (information.size_high || information.size_low > MAXIMUM_ADDON_FILE_SIZE)
		refuse(mutator, path, "its addon's file is larger than 64 MB");
}

/* a script's name, beside the mutator.toml at path */
static void script_read(struct port_mutator *mutator, const char *path, const char *name)
{
	struct posix_file_information information;
	size_t folder_length = strlen(path) - strlen("mutator.toml");

	if (!name_ends(name, ".lua") || name[0] == '.' || strchr(name, '/') || strchr(name, '\\') ||
		folder_length + strlen(name) >= sizeof(mutator->script_path))
	{
		refuse(mutator, path, "its script is not a .lua file's name beside its mutator.toml");
		return;
	}
	snprintf(mutator->script_path, sizeof(mutator->script_path), "%.*s%s", (int)folder_length, path, name);
	if (posix_stat(mutator->script_path, &information) != 0)
		refuse(mutator, path, "its script's file is not there");
	else if (information.size_high || information.size_low > MAXIMUM_SCRIPT_FILE_SIZE)
		refuse(mutator, path, "its script's file is larger than 256 KB");
}

static void mutator_read(struct port_mutator *mutator, const char *folder, const char *text, size_t size,
	const char *path)
{
	static const char *const keys[] = { "name", "id", "version", "authors", "description", "format", "effects",
		"option", "tags", "pose", "death", "explosion", "addon", "place", "script" };
	toml_result_t result;
	toml_datum_t value;
	char reason[MAXIMUM_REFUSAL_LENGTH];
	int index;

	text_copy(mutator->folder, sizeof(mutator->folder), folder, 0);
	text_copy(mutator->given_id, sizeof(mutator->given_id), folder, 0);
	text_copy(mutator->id, sizeof(mutator->id), folder, 0);
	text_copy(mutator->name, sizeof(mutator->name), folder, 0);
	p2p_sha256(text, (int)size, mutator->toml_digest);
	mutator->toml_digest_made = 1;
	result = toml_parse(text, (int)size);
	if (!result.ok)
	{
		snprintf(reason, sizeof(reason), "it is not TOML (%.100s)", result.errmsg);
		refuse(mutator, path, reason);
		toml_free(result);
		return;
	}
	if (toml_text(result.toptab, "name", mutator, path))
		text_copy(mutator->name, sizeof(mutator->name), toml_text(result.toptab, "name", mutator, path), 0);
	if (toml_text(result.toptab, "id", mutator, path))
	{
		text_copy(mutator->given_id, sizeof(mutator->given_id), toml_text(result.toptab, "id", mutator, path), 0);
		text_copy(mutator->id, sizeof(mutator->id), mutator->given_id, 0);
	}
	text_copy(mutator->version, sizeof(mutator->version), toml_text(result.toptab, "version", mutator, path), 0);
	text_copy(mutator->description, sizeof(mutator->description),
		toml_text(result.toptab, "description", mutator, path), 1);
	value = toml_get(result.toptab, "authors");
	if (value.type == TOML_STRING)
	{
		text_copy(mutator->authors, sizeof(mutator->authors), value.u.s, 0);
	}
	else if (value.type == TOML_ARRAY)
	{
		for (index = 0; index < value.u.arr.size; index++)
		{
			size_t length = strlen(mutator->authors);

			if (value.u.arr.elem[index].type == TOML_STRING)
				snprintf(mutator->authors + length, sizeof(mutator->authors) - length, "%s%s", length ? ", " : "",
					value.u.arr.elem[index].u.s);
		}
		text_copy(mutator->authors, sizeof(mutator->authors), mutator->authors, 0);
	}
	value = toml_get(result.toptab, "format");
	if (value.type == TOML_INT64 && value.u.int64 > MUTATORS_FORMAT)
		refuse(mutator, path, "it is made for a later build");
	else if (value.type != TOML_UNKNOWN && value.type != TOML_INT64)
		refuse(mutator, path, "its format is not a number");
	if (!id_valid(mutator->id))
		refuse(mutator, path, "its id is empty or has a comma, quote, backslash or @");
	for (index = 0; index < result.toptab.u.tab.size; index++)
	{
		size_t key;

		for (key = 0; key < sizeof(keys) / sizeof(keys[0]); key++)
		{
			if (!strcmp(result.toptab.u.tab.key[index], keys[key]))
				break;
		}
		if (key == sizeof(keys) / sizeof(keys[0]))
		{
			/* (a key this build does not know may change what the mutator
			does: not played, as an effect it does not have is not) */
			snprintf(reason, sizeof(reason), "line %d: this game has no key %.40s",
				result.toptab.u.tab.value[index].lineno, result.toptab.u.tab.key[index]);
			refuse(mutator, path, reason);
		}
	}
	value = toml_get(result.toptab, "effects");
	if (value.type != TOML_UNKNOWN)
		effects_read(mutator, value, path);
	/* (its options before the tag edits that name them) */
	value = toml_get(result.toptab, "option");
	if (value.type != TOML_UNKNOWN)
		options_read(mutator, value, path);
	value = toml_get(result.toptab, "tags");
	if (value.type != TOML_UNKNOWN)
		tags_read(mutator, value, path);
	value = toml_get(result.toptab, "pose");
	if (value.type != TOML_UNKNOWN)
		poses_read(mutator, value, path);
	value = toml_get(result.toptab, "death");
	if (value.type != TOML_UNKNOWN)
		death_read(mutator, value, path);
	value = toml_get(result.toptab, "explosion");
	if (value.type != TOML_UNKNOWN)
	{
		/* [explosion]: the decal every explosion leaves, as big as its damage
		reaches (kept as a tag edit's bytes) */
		if (value.type != TOML_TABLE || value.u.tab.size != 1 || strcmp(value.u.tab.key[0], "decal") ||
			value.u.tab.value[0].type != TOML_STRING)
		{
			snprintf(reason, sizeof(reason), "line %d: [explosion] has a decal (a decal's name), and nothing else",
				value.lineno);
			refuse(mutator, path, reason);
		}
		else
			tag_edit_add(mutator, path, value.lineno, "", value.u.tab.value[0].u.s, "", _tag_edit_explosion, 0.0,
				NULL, NULL, 0, 0);
	}
	if (toml_text(result.toptab, "addon", mutator, path))
	{
		/* (the stock mutators have none: their folders are written from
		mutator.toml alone) */
		if (mutator->stock)
			refuse(mutator, path, "a stock mutator has no addon");
		else
			addon_read(mutator, path, toml_text(result.toptab, "addon", mutator, path));
	}
	value = toml_get(result.toptab, "place");
	if (value.type != TOML_UNKNOWN)
	{
		places_read(mutator, value, path);
		if (!mutator->addon_path[0])
			refuse(mutator, path, "it places vehicles or weapons but has no addon");
	}
	value = toml_get(result.toptab, "script");
	if (value.type != TOML_UNKNOWN)
	{
		const char *script = value.type == TOML_TABLE ? toml_text(value, "file", mutator, path) : NULL;

		if (script)
			script_read(mutator, path, script);
		else
			refuse(mutator, path, "its [script] has no file");
	}
	if (!mutator->key_count && !mutator->tag_edit_count && !mutator->addon_path[0] && !mutator->script_path[0])
		refuse(mutator, path, "it edits no tag, poses nothing and adds nothing");
	toml_free(result);
}

/* where the mutator is, as the log and the Mods menu say it */
/* a file written whole, of any bytes */
static int share_file_write(const char *path, const void *bytes, unsigned long size)
{
	FILE *stream = fopen(path, "wb");
	int written;

	if (!stream)
		return 0;
	written = fwrite(bytes, 1, size, stream) == size;
	if (fclose(stream))
		written = 0;
	return written;
}

static void mutator_place_text(const struct port_mutator *mutator, char *text, size_t size)
{
	snprintf(text, size, "%s/%s", mutator->stock ? "stock_mutators" : "mutators", mutator->folder);
}

/* the mutator's files' SHA-256 (its digest), made the first time it is
asked for: 0 if a file cannot be read */
static int mutator_digest(struct port_mutator *mutator)
{
	unsigned char digests[3 * SHA256_SIZE];
	int size = SHA256_SIZE;

	if (mutator->digest_state)
		return mutator->digest_state > 0;
	mutator->digest_state = -1;
	if (!mutator->toml_digest_made)
		return 0;
	memcpy(digests, mutator->toml_digest, SHA256_SIZE);
	if (mutator->addon_path[0])
	{
		size_t addon_size = 0;
		char *addon = config_file_read(mutator->addon_path, &addon_size);

		if (!addon || addon_size > MAXIMUM_ADDON_FILE_SIZE)
		{
			free(addon);
			return 0;
		}
		p2p_sha256(addon, (int)addon_size, digests + SHA256_SIZE);
		free(addon);
		size += SHA256_SIZE;
	}
	if (mutator->script_path[0])
	{
		size_t script_size = 0;
		char *script = config_file_read(mutator->script_path, &script_size);

		if (!script || script_size > MAXIMUM_SCRIPT_FILE_SIZE)
		{
			free(script);
			return 0;
		}
		p2p_sha256(script, (int)script_size, digests + size);
		free(script);
		size += SHA256_SIZE;
	}
	p2p_sha256(digests, size, mutator->digest);
	mutator->digest_state = 1;
	return 1;
}

/* whether a mutator found before the list's `count`th is played by `id` */
static int mutators_id_taken(long count, const char *id)
{
	long index;

	for (index = 0; index < count; index++)
	{
		if (!names_compare(port_mutators.mutators[index].id, id))
			return 1;
	}
	return 0;
}

/* (the mutator's tag edits are the list's after it) */
static void mutator_add(const struct port_mutator *mutator)
{
	struct port_mutator *added;
	char place[MAXIMUM_PATH_LENGTH];
	long first = -1;
	int same_folder = 0;
	long index;

	if (port_mutators.count == MAXIMUM_MUTATORS)
	{
		platform_log("mutators: more than %d: %s and those after it are left out", MAXIMUM_MUTATORS, mutator->folder);
		free(mutator->tag_edits);
		return;
	}
	added = &port_mutators.mutators[port_mutators.count];
	*added = *mutator;
	mutator_place_text(added, place, sizeof(place));
	for (index = 0; index < port_mutators.count; index++)
	{
		if (!names_compare(port_mutators.mutators[index].given_id, added->given_id))
		{
			if (first < 0)
				first = index;
			if (!names_compare(port_mutators.mutators[index].folder, added->folder))
				same_folder = 1;
			/* (the same files again: played once) */
			if (mutator_digest(added) && mutator_digest(&port_mutators.mutators[index]) &&
				!memcmp(added->digest, port_mutators.mutators[index].digest, SHA256_SIZE))
			{
				char other[MAXIMUM_PATH_LENGTH];

				mutator_place_text(&port_mutators.mutators[index], other, sizeof(other));
				platform_log("mutators: %s is the same as %s: it is played once, as %s", place, other,
					port_mutators.mutators[index].id);
				free(added->tag_edits);
				memset(added, 0, sizeof(*added));
				return;
			}
		}
	}
	if (first >= 0)
	{
		char other[MAXIMUM_PATH_LENGTH];

		mutator_place_text(&port_mutators.mutators[first], other, sizeof(other));
		snprintf(added->taken_by, sizeof(added->taken_by), "%s", other);
		snprintf(added->id, sizeof(added->id), "%s@%s", added->given_id, added->folder);
		/* (where its folder's name is that of a mutator with its id, in the
		other folder, its files' tell them apart) */
		if (same_folder || mutators_id_taken(port_mutators.count, added->id))
		{
			if (mutator_digest(added))
			{
				int digit;

				snprintf(added->id, sizeof(added->id), "%s@", added->given_id);
				for (digit = 0; digit < ID_DIGEST_DIGITS; digit++)
				{
					size_t length = strlen(added->id);

					snprintf(added->id + length, sizeof(added->id) - length, "%x",
						(added->digest[digit / 2] >> (digit % 2 ? 0 : 4)) & 0xF);
				}
			}
			if (!mutator_digest(added) || mutators_id_taken(port_mutators.count, added->id))
			{
				snprintf(added->id, sizeof(added->id), "%s", added->given_id);
				refuse(added, place, "another mutator has its id, and its folder's name and files too");
				port_mutators.count++;
				return;
			}
		}
		platform_log("mutators: %s has the id %s, as %s has: it is played as %s", place, added->given_id, other,
			added->id);
	}
	port_mutators.count++;
}

/* the folders of the mutators folder with a mutator.toml, by name */
static void mutators_find(const char *root)
{
	char names[MAXIMUM_MUTATORS][MAXIMUM_MUTATOR_FOLDER_LENGTH];
	long name_count = 0;
	void *directory = posix_directory_open(root);
	char name[256];
	long index;

	if (!directory)
		return;
	while (posix_directory_next(directory, name, sizeof(name)))
	{
		char path[MAXIMUM_PATH_LENGTH];
		struct posix_file_information information;
		long place;

		snprintf(path, sizeof(path), "%s/%s", root, name);
		if (name[0] == '.' || posix_stat(path, &information) != 0 || !(information.flags & _posix_file_is_directory))
			continue;
		if (strlen(name) >= MAXIMUM_MUTATOR_FOLDER_LENGTH)
		{
			platform_log("mutators: the folder name %s is longer than %d characters: it is not a mutator", name,
				MAXIMUM_MUTATOR_FOLDER_LENGTH - 1);
			continue;
		}
		if (name_count == MAXIMUM_MUTATORS)
		{
			platform_log("mutators: more than %d folders in %s: %s and those after it are left out", MAXIMUM_MUTATORS,
				root, name);
			break;
		}
		for (place = name_count; place > 0 && names_compare(names[place - 1], name) > 0; place--)
			memcpy(names[place], names[place - 1], sizeof(names[place]));
		snprintf(names[place], sizeof(names[place]), "%s", name);
		name_count++;
	}
	posix_directory_close(directory);
	for (index = 0; index < name_count; index++)
	{
		char path[MAXIMUM_PATH_LENGTH];
		struct posix_file_information information;
		struct port_mutator *mutator = calloc(1, sizeof(*mutator));
		char *text;
		size_t size = 0;

		if (!mutator)
			break;
		snprintf(path, sizeof(path), "%s/%s/mutator.toml", root, names[index]);
		if (posix_stat(path, &information) != 0)
		{
			platform_log("mutators: %s/%s has no mutator.toml: it is not a mutator", root, names[index]);
			free(mutator);
			continue;
		}
		if (information.size_high || information.size_low > MAXIMUM_MUTATOR_FILE_SIZE)
		{
			text_copy(mutator->folder, sizeof(mutator->folder), names[index], 0);
			text_copy(mutator->given_id, sizeof(mutator->given_id), names[index], 0);
			text_copy(mutator->id, sizeof(mutator->id), names[index], 0);
			text_copy(mutator->name, sizeof(mutator->name), names[index], 0);
			refuse(mutator, path, "its mutator.toml is larger than 64 KB");
		}
		else if (!(text = config_file_read(path, &size)))
		{
			text_copy(mutator->folder, sizeof(mutator->folder), names[index], 0);
			text_copy(mutator->given_id, sizeof(mutator->given_id), names[index], 0);
			text_copy(mutator->id, sizeof(mutator->id), names[index], 0);
			text_copy(mutator->name, sizeof(mutator->name), names[index], 0);
			refuse(mutator, path, "its mutator.toml cannot be read");
		}
		else
		{
			mutator_read(mutator, names[index], text, size, path);
			free(text);
		}
		mutator_add(mutator);
		free(mutator);
	}
}

/* whether the file `path` holds `text` */
static int file_holds(const char *path, const char *text)
{
	size_t size = 0;
	char *held = config_file_read(path, &size);
	int holds = held && size == strlen(text) && !memcmp(held, text, size);

	free(held);
	return holds;
}

/* writes the README and the stock mutators into the stock_mutators folder
`root`, each file that does not hold what the game has; 0 if one cannot be
written */
static int stock_mutators_write(const char *root)
{
	char path[MAXIMUM_PATH_LENGTH];
	unsigned int index;
	int written = 1;

	posix_make_directory(root);
	snprintf(path, sizeof(path), "%s/README.txt", root);
	if (!file_holds(path, stock_mutators_readme))
		written = config_file_write(path, stock_mutators_readme);
	for (index = 0; index < mutator_files_embedded_count; index++)
	{
		const char *text = (const char *)mutator_files_embedded[index].data;

		snprintf(path, sizeof(path), "%s/%s", root, mutator_files_embedded[index].folder);
		posix_make_directory(path);
		snprintf(path, sizeof(path), "%s/%s/mutator.toml", root, mutator_files_embedded[index].folder);
		if (!file_holds(path, text) && !config_file_write(path, text))
			written = 0;
	}
	return written;
}

/* whether game.mutators names `id` */
static int mutators_setting_names(const char *setting, const char *id)
{
	size_t length = strlen(id);

	while (*setting)
	{
		const char *end = strchr(setting, ',');
		const char *last;
		size_t item_length;

		if (!end)
			end = setting + strlen(setting);
		while (setting < end && (*setting == ' ' || *setting == '\t'))
			setting++;
		for (last = end; last > setting && (last[-1] == ' ' || last[-1] == '\t'); last--)
		{
		}
		item_length = (size_t)(last - setting);
		if (item_length == length)
		{
			size_t index;

			for (index = 0; index < length && lower(setting[index]) == lower(id[index]); index++)
			{
			}
			if (index == length)
				return 1;
		}
		setting = *end ? end + 1 : end;
	}
	return 0;
}

/* ---------- public code */

long port_mutators_scan(void)
{
	char root[MAXIMUM_PATH_LENGTH];
	long index;
	long on = 0;

	for (index = 0; index < port_mutators.count; index++)
		free(port_mutators.mutators[index].tag_edits);
	port_mutators.count = 0;
	port_mutators.scans++;
	snprintf(root, sizeof(root), "%s/stock_mutators", platform_data_root());
	if (!stock_mutators_write(root) && !port_mutators.stock_unwritten_logged)
	{
		platform_log("mutators: %s cannot be written: the stock mutators are played as the game has them", root);
		port_mutators.stock_unwritten_logged = 1;
	}
	for (index = 0; index < (long)mutator_files_embedded_count; index++)
	{
		const struct mutator_file_embedded *file = &mutator_files_embedded[index];
		struct port_mutator *mutator = calloc(1, sizeof(*mutator));
		char path[MAXIMUM_PATH_LENGTH];

		if (!mutator)
			break;
		snprintf(path, sizeof(path), "%s/%s/mutator.toml", root, file->folder);
		mutator->stock = 1;
		mutator_read(mutator, file->folder, (const char *)file->data, file->size, path);
		mutator_add(mutator);
		free(mutator);
	}
	snprintf(root, sizeof(root), "%s/mutators", platform_data_root());
	mutators_find(root);
	port_mutators_revert();
	for (index = 0; index < port_mutators.count; index++)
	{
		if (port_mutators.mutators[index].on && !port_mutators.mutators[index].refusal[0])
		{
			platform_log("mutators: %s (%s) is on", port_mutators.mutators[index].id, port_mutators.mutators[index].name);
			on++;
		}
	}
	return on;
}

/* a script's key: "script." and its mutator's digest's first digits */
static int script_key_make(struct port_mutator *mutator, char *key)
{
	static const char digits[] = "0123456789abcdef";
	int index;

	if (!mutator_digest(mutator))
		return 0;
	strcpy(key, "script.");
	for (index = 0; index < 12; index++)
	{
		key[7 + index * 2] = digits[mutator->digest[index] >> 4];
		key[7 + index * 2 + 1] = digits[mutator->digest[index] & 15];
	}
	key[7 + 24] = 0;
	return 1;
}

long port_mutators_keys(char keys[][PORT_MUTATOR_KEY_LENGTH], double *values, long maximum)
{
	long count = 0;
	long index;

	if (!port_mutators.scans)
		port_mutators_scan();
	for (index = 0; index < port_mutators.count; index++)
	{
		const struct port_mutator *mutator = &port_mutators.mutators[index];
		long key;

		if (!mutator->on || mutator->refusal[0])
			continue;
		for (key = 0; key < mutator->key_count && count < maximum; key++)
		{
			memcpy(keys[count], mutator->keys[key], PORT_MUTATOR_KEY_LENGTH);
			values[count++] = mutator->values[key];
		}
		if (mutator->script_path[0] && count < maximum && script_key_make(&port_mutators.mutators[index], keys[count]))
			values[count++] = 1.;
	}
	return count;
}

/* the mutator (on or not: a client plays the host's) whose script the key names */
static struct port_mutator *script_mutator_find(const char *key)
{
	char own[PORT_MUTATOR_KEY_LENGTH];
	long index;

	if (!port_mutators.scans)
		port_mutators_scan();
	/* (a host's, unless this player refused it) */
	for (index = 0; index < host_mutator_count; index++)
	{
		struct port_mutator *mutator = &host_mutators[index];

		if (mutator->script_path[0] && script_key_make(mutator, own) && !strcmp(own, key))
		{
			if (mutator->script_refused)
			{
				platform_log("mutators: the host's script %s is not run: this player refused it", mutator->id);
				return NULL;
			}
			return mutator;
		}
	}
	for (index = 0; index < port_mutators.count; index++)
	{
		struct port_mutator *mutator = &port_mutators.mutators[index];

		if (mutator->script_path[0] && !mutator->refusal[0] && script_key_make(mutator, own) && !strcmp(own, key))
			return mutator;
	}
	return NULL;
}

char *port_mutators_script_read(const char *key, unsigned long *size)
{
	struct port_mutator *mutator = script_mutator_find(key);
	size_t read_size = 0;
	char *text = mutator ? config_file_read(mutator->script_path, &read_size) : NULL;

	*size = (unsigned long)read_size;
	return text;
}

void port_mutators_script_free(char *text)
{
	free(text);
}

/* a table's values of a kind a script's settings can hold */
static void script_table_values(toml_datum_t table, port_mutators_setting_proc proc, void *context)
{
	int index;

	for (index = 0; index < table.u.tab.size; index++)
	{
		const char *name = table.u.tab.key[index];
		toml_datum_t value = table.u.tab.value[index];

		if (value.type == TOML_STRING)
			proc(context, name, value.u.s, 0., 0, 0);
		else if (value.type == TOML_INT64)
			proc(context, name, NULL, (double)value.u.int64, 0, 1);
		else if (value.type == TOML_FP64)
			proc(context, name, NULL, value.u.fp64, 0, 1);
		else if (value.type == TOML_BOOLEAN)
			proc(context, name, NULL, 0., value.u.boolean, 2);
	}
}

/* a file beside the script, parsed: its top table, or its [script] table */
static void script_file_values(const struct port_mutator *mutator, const char *file, int script_table,
	port_mutators_setting_proc proc, void *context)
{
	char path[MAXIMUM_PATH_LENGTH];
	const char *slash = strrchr(mutator->script_path, '/');
	size_t size = 0;
	char *text;
	toml_result_t result;

	if (!slash || (size_t)(slash - mutator->script_path) + strlen(file) + 2 > sizeof(path))
		return;
	snprintf(path, sizeof(path), "%.*s/%s", (int)(slash - mutator->script_path), mutator->script_path, file);
	text = config_file_read(path, &size);
	if (!text)
		return;
	result = toml_parse(text, (int)size);
	if (result.ok)
	{
		toml_datum_t table = script_table ? toml_get(result.toptab, "script") : result.toptab;

		if (table.type == TOML_TABLE)
			script_table_values(table, proc, context);
	}
	else
	{
		platform_log("mutators: %s is not TOML (%.100s)", path, result.errmsg);
	}
	toml_free(result);
	free(text);
}

/* one value of a file's top table, or of its [script] table, if it is text */
struct script_value_find
{
	const char *name;
	char *value;
	size_t size;
	int found;
};

static void script_value_found(void *context, const char *name, const char *text, double number, int boolean_value,
	int kind)
{
	struct script_value_find *find = context;

	(void)number;
	(void)boolean_value;
	if (kind == 0 && !strcmp(name, find->name))
	{
		text_copy(find->value, find->size, text, 0);
		find->found = 1;
	}
}

/* the controls that the mutators on have the Controls Setup screen show: for each
key of its [script.menu], a row of that label for that value of its [script],
the player's own from settings.toml */
long port_mutators_script_controls(struct port_mutator_control *controls, long maximum)
{
	long count = 0;
	long index;

	if (!port_mutators.scans)
		port_mutators_scan();
	for (index = 0; index < port_mutators.count; index++)
	{
		const struct port_mutator *mutator = &port_mutators.mutators[index];
		char path[MAXIMUM_PATH_LENGTH];
		const char *slash = strrchr(mutator->script_path, '/');
		size_t size = 0;
		char *text;
		toml_result_t result;

		if (!mutator->on || mutator->refusal[0] || !mutator->script_path[0] || !slash ||
			(size_t)(slash - mutator->script_path) + sizeof("/mutator.toml") > sizeof(path))
		{
			continue;
		}
		snprintf(path, sizeof(path), "%.*s/mutator.toml", (int)(slash - mutator->script_path), mutator->script_path);
		text = config_file_read(path, &size);
		if (!text)
			continue;
		result = toml_parse(text, (int)size);
		if (result.ok)
		{
			toml_datum_t menu = toml_seek(result.toptab, "script.menu");
			int row;

			for (row = 0; menu.type == TOML_TABLE && row < menu.u.tab.size && count < maximum; row++)
			{
				struct port_mutator_control *control = &controls[count];
				struct script_value_find find;

				if (menu.u.tab.value[row].type != TOML_STRING)
					continue;
				memset(control, 0, sizeof(*control));
				control->mutator = index;
				text_copy(control->name, sizeof(control->name), menu.u.tab.key[row], 0);
				text_copy(control->label, sizeof(control->label), menu.u.tab.value[row].u.s, 0);
				find.name = control->name;
				find.value = control->fallback;
				find.size = sizeof(control->fallback);
				find.found = 0;
				script_file_values(mutator, "mutator.toml", 1, script_value_found, &find);
				find.value = control->value;
				find.size = sizeof(control->value);
				find.found = 0;
				script_file_values(mutator, "settings.toml", 0, script_value_found, &find);
				if (!find.found)
					text_copy(control->value, sizeof(control->value), control->fallback, 0);
				count++;
			}
		}
		toml_free(result);
		free(text);
	}
	return count;
}

/* settings.toml being written again, each value as it was */
struct script_settings_write
{
	const char *name;
	const char *value;
	int written;
	char text[0x2000];
	size_t used;
};

static void script_settings_text(struct script_settings_write *write, const char *name, const char *value)
{
	const char *cursor;

	write->used += (size_t)snprintf(write->text + write->used, sizeof(write->text) - write->used, "\"%s\" = \"", name);
	for (cursor = value; *cursor && write->used + 4 < sizeof(write->text); cursor++)
	{
		if (*cursor == '"' || *cursor == '\\')
			write->text[write->used++] = '\\';
		if ((unsigned char)*cursor >= 0x20)
			write->text[write->used++] = *cursor;
	}
	write->used += (size_t)snprintf(write->text + write->used, sizeof(write->text) - write->used, "\"\n");
}

static void script_settings_kept(void *context, const char *name, const char *text, double number, int boolean_value,
	int kind)
{
	struct script_settings_write *write = context;

	if (write->used + 0x200 > sizeof(write->text))
		return;
	if (!strcmp(name, write->name))
	{
		script_settings_text(write, name, write->value);
		write->written = 1;
	}
	else if (kind == 1)
		write->used += (size_t)snprintf(write->text + write->used, sizeof(write->text) - write->used,
			"\"%s\" = %.17g\n", name, number);
	else if (kind == 2)
		write->used += (size_t)snprintf(write->text + write->used, sizeof(write->text) - write->used,
			"\"%s\" = %s\n", name, boolean_value ? "true" : "false");
	else
		script_settings_text(write, name, text);
}

/* a value in the mutator's settings.toml: 0 if it cannot be written */
int port_mutators_script_control_write(long index, const char *name, const char *value)
{
	static struct script_settings_write write;
	const struct port_mutator *mutator;
	char path[MAXIMUM_PATH_LENGTH];
	const char *slash;

	if (index < 0 || index >= port_mutators.count)
		return 0;
	mutator = &port_mutators.mutators[index];
	slash = strrchr(mutator->script_path, '/');
	if (!slash || (size_t)(slash - mutator->script_path) + sizeof("/settings.toml") > sizeof(path))
		return 0;
	snprintf(path, sizeof(path), "%.*s/settings.toml", (int)(slash - mutator->script_path), mutator->script_path);
	memset(&write, 0, sizeof(write));
	write.name = name;
	write.value = value;
	script_file_values(mutator, "settings.toml", 0, script_settings_kept, &write);
	if (!write.written)
		script_settings_text(&write, name, value);
	return config_file_write(path, write.text);
}

/* a script's settings: the values of its mutator.toml's [script] table, then this
player's own, from a settings.toml beside it, which is in no digest */
void port_mutators_script_settings(const char *key, port_mutators_setting_proc proc, void *context)
{
	struct port_mutator *mutator = script_mutator_find(key);

	if (mutator)
	{
		script_file_values(mutator, "mutator.toml", 1, proc, context);
		script_file_values(mutator, "settings.toml", 0, proc, context);
	}
}

/* an option's percent: game.mutator_options' "<mutator's id>.<option's id>=<percent>", else its default,
kept to its least, its most and its steps */
static long option_value(const struct port_mutator *mutator, const struct port_mutator_option *option)
{
	const char *setting = config_string("game.mutator_options");
	char key[MAXIMUM_MUTATOR_ID_LENGTH + OPTION_ID_LENGTH + 2];
	size_t length;
	long value = option->fallback;

	snprintf(key, sizeof(key), "%s.%s=", mutator->id, option->id);
	length = strlen(key);
	while (setting && *setting)
	{
		while (*setting == ' ' || *setting == ',')
			setting++;
		if (!strncmp(setting, key, length))
		{
			value = strtol(setting + length, NULL, 10);
			break;
		}
		setting = strchr(setting, ',');
	}
	if (value < option->minimum || value > option->maximum)
		value = option->fallback;
	return value - (value - option->minimum) % option->step;
}

long port_mutators_tag_edits(unsigned char *edits, long maximum)
{
	long size = 0;
	long index;

	if (!port_mutators.scans)
		port_mutators_scan();
	for (index = 0; index < port_mutators.count; index++)
	{
		const struct port_mutator *mutator = &port_mutators.mutators[index];

		if (!mutator->on || mutator->refusal[0] || !mutator->tag_edit_size)
			continue;
		if (size + mutator->tag_edit_size > maximum)
		{
			platform_log("mutators: the tag edits of those on are more than %ld bytes: %s's and those after are not "
				"played", maximum, mutator->id);
			break;
		}
		memcpy(edits + size, mutator->tag_edits, (size_t)mutator->tag_edit_size);
		/* (its options' numbers, at the percents the host has set) */
		{
			long scaled;

			for (scaled = 0; scaled < mutator->scaled_edit_count; scaled++)
			{
				const struct port_mutator_scaled_edit *edit = &mutator->scaled_edits[scaled];

				edit_number_write(edits + size + edit->offset,
					edit->number * option_value(mutator, &mutator->options[edit->option]) / 100.0);
			}
		}
		size += mutator->tag_edit_size;
	}
	return size;
}

/* the option at `index` of those of the mutators on, in their order, else NULL */
static const struct port_mutator_option *option_of_on(long index, const struct port_mutator **of)
{
	long mutator_index;

	if (!port_mutators.scans)
		port_mutators_scan();
	for (mutator_index = 0; index >= 0 && mutator_index < port_mutators.count; mutator_index++)
	{
		const struct port_mutator *mutator = &port_mutators.mutators[mutator_index];

		if (!mutator->on || mutator->refusal[0])
			continue;
		if (index < mutator->option_count)
		{
			*of = mutator;
			return &mutator->options[index];
		}
		index -= mutator->option_count;
	}
	return NULL;
}

long port_mutators_option_count(void)
{
	const struct port_mutator *mutator;
	long count = 0;

	while (option_of_on(count, &mutator))
		count++;
	return count;
}

int port_mutators_option(long index, char *name, unsigned long size, long *value, long *minimum, long *maximum,
	long *step)
{
	const struct port_mutator *mutator;
	const struct port_mutator_option *option = option_of_on(index, &mutator);

	if (!option)
		return 0;
	if (name && size)
		snprintf(name, size, "%s", option->name);
	*value = option_value(mutator, option);
	*minimum = option->minimum;
	*maximum = option->maximum;
	*step = option->step;
	return 1;
}

int port_mutators_option_set(long index, long value)
{
	const struct port_mutator *mutator;
	const struct port_mutator_option *option = option_of_on(index, &mutator);
	const char *setting;
	char kept[1024] = "";
	char key[MAXIMUM_MUTATOR_ID_LENGTH + OPTION_ID_LENGTH + 2];
	size_t length;

	if (!option || value < option->minimum || value > option->maximum || (value - option->minimum) % option->step)
		return 0;
	snprintf(key, sizeof(key), "%s.%s=", mutator->id, option->id);
	length = strlen(key);
	/* (the setting's other options kept, this one's in place of its own; none at its default) */
	for (setting = config_string("game.mutator_options"); setting && *setting;)
	{
		const char *end;
		size_t used = strlen(kept);

		while (*setting == ' ' || *setting == ',')
			setting++;
		end = strchr(setting, ',');
		if (!end)
			end = setting + strlen(setting);
		if (end > setting && strncmp(setting, key, length))
			snprintf(kept + used, sizeof(kept) - used, "%s%.*s", used ? "," : "", (int)(end - setting), setting);
		setting = end;
	}
	if (value != option->fallback)
	{
		size_t used = strlen(kept);

		snprintf(kept + used, sizeof(kept) - used, "%s%s%ld", used ? "," : "", key, value);
	}
	return config_write("game.mutator_options", kept);
}

/* the mutators on that can be played that have an add-on, in their order */
static const struct port_mutator *addon_mutator(long index)
{
	long mutator_index;

	/* (a client's are its host's, in the host's order) */
	if (host_mutators_on)
	{
		for (mutator_index = 0; mutator_index < host_mutator_count; mutator_index++)
		{
			if (host_mutators[mutator_index].addon_path[0] && !index--)
				return &host_mutators[mutator_index];
		}
		return NULL;
	}
	for (mutator_index = 0; mutator_index < port_mutators.count; mutator_index++)
	{
		const struct port_mutator *mutator = &port_mutators.mutators[mutator_index];

		if (!mutator->on || mutator->refusal[0] || !mutator->addon_path[0])
			continue;
		if (!index--)
			return mutator;
	}
	return NULL;
}

/* ---------- the host's mutators' files (port/linux/game/mutator_share.c) */

#define SHARED_NAME_BYTES 64
#define SHARED_ID_BYTES 64

struct port_mutator_share_file
{
	char name[SHARED_NAME_BYTES];
	unsigned long size;
	unsigned char sha256[SHA256_SIZE];
};

struct port_mutator_share
{
	char id[SHARED_ID_BYTES];
	unsigned char digest[SHA256_SIZE];
	long file_count;
	struct port_mutator_share_file files[3];
};

/* the host's mutators with files, by the order they are played */
static struct port_mutator *share_mutator(long wanted)
{
	long index;

	if (!port_mutators.scans)
		port_mutators_scan();
	for (index = 0; index < port_mutators.count; index++)
	{
		struct port_mutator *mutator = &port_mutators.mutators[index];

		if (mutator->on && !mutator->refusal[0] && (mutator->addon_path[0] || mutator->script_path[0]) && wanted-- == 0)
			return mutator;
	}
	return NULL;
}

int port_mutators_share_any(void)
{
	return share_mutator(0) != NULL;
}

long port_mutators_share_count(void)
{
	long count = 0;

	while (share_mutator(count))
		count++;
	return count;
}

static const char *path_file_name(const char *path)
{
	const char *slash = strrchr(path, '/');

	return slash ? slash + 1 : path;
}

/* a mutator's files: its mutator.toml, its add-on's, its script's */
static int share_file_paths(const struct port_mutator *mutator, char paths[3][MAXIMUM_PATH_LENGTH])
{
	const char *one = mutator->addon_path[0] ? mutator->addon_path : mutator->script_path;
	const char *slash = strrchr(one, '/');
	int count = 0;

	if (!slash || (size_t)(slash - one) + sizeof("/mutator.toml") > MAXIMUM_PATH_LENGTH)
		return 0;
	snprintf(paths[count++], MAXIMUM_PATH_LENGTH, "%.*s/mutator.toml", (int)(slash - one), one);
	if (mutator->addon_path[0])
		snprintf(paths[count++], MAXIMUM_PATH_LENGTH, "%s", mutator->addon_path);
	if (mutator->script_path[0])
		snprintf(paths[count++], MAXIMUM_PATH_LENGTH, "%s", mutator->script_path);
	return count;
}

int port_mutators_share_entry(long index, struct port_mutator_share *entry)
{
	struct port_mutator *mutator = share_mutator(index);
	char paths[3][MAXIMUM_PATH_LENGTH];
	int count;
	int file;

	if (!mutator || !mutator_digest(mutator) || !(count = share_file_paths(mutator, paths)))
		return 0;
	memset(entry, 0, sizeof(*entry));
	text_copy(entry->id, sizeof(entry->id), mutator->id, 0);
	memcpy(entry->digest, mutator->digest, SHA256_SIZE);
	entry->file_count = count;
	for (file = 0; file < count; file++)
	{
		size_t size = 0;
		char *bytes = config_file_read(paths[file], &size);

		if (!bytes)
			return 0;
		text_copy(entry->files[file].name, sizeof(entry->files[file].name), path_file_name(paths[file]), 0);
		entry->files[file].size = (unsigned long)size;
		p2p_sha256(bytes, (int)size, entry->files[file].sha256);
		free(bytes);
	}
	return 1;
}

/* the last file read, kept: a piece of it is asked for at a time */
static struct
{
	char path[MAXIMUM_PATH_LENGTH];
	char *bytes;
	size_t size;
} share_read_file;

int port_mutators_share_file_size(long index, long file, unsigned long *size)
{
	struct port_mutator *mutator = share_mutator(index);
	char paths[3][MAXIMUM_PATH_LENGTH];
	struct posix_file_information information;

	if (!mutator || file < 0 || file >= share_file_paths(mutator, paths) || posix_stat(paths[file], &information) != 0 ||
		information.size_high)
	{
		return 0;
	}
	*size = (unsigned long)information.size_low;
	return 1;
}

int port_mutators_share_read(long index, long file, unsigned long offset, void *buffer, long size)
{
	struct port_mutator *mutator = share_mutator(index);
	char paths[3][MAXIMUM_PATH_LENGTH];

	if (!mutator || file < 0 || file >= share_file_paths(mutator, paths))
		return 0;
	if (strcmp(share_read_file.path, paths[file]) || !share_read_file.bytes)
	{
		free(share_read_file.bytes);
		share_read_file.bytes = config_file_read(paths[file], &share_read_file.size);
		snprintf(share_read_file.path, sizeof(share_read_file.path), "%s", paths[file]);
		if (!share_read_file.bytes)
			return 0;
	}
	if (size < 0 || offset > share_read_file.size || (size_t)size > share_read_file.size - offset)
		return 0;
	memcpy(buffer, share_read_file.bytes + offset, (size_t)size);
	return 1;
}

/* a file's name and size as a host's list may have them: mutator.toml first, then
at most one add-on and one script, each no larger than the game reads */
int port_mutators_share_valid(const struct port_mutator_share *entry)
{
	long file;
	int addon = 0;
	int script = 0;

	if (entry->file_count < 2 || entry->file_count > 3)
		return 0;
	for (file = 0; file < entry->file_count; file++)
	{
		const char *name = entry->files[file].name;
		unsigned long maximum;

		if (!memchr(name, 0, SHARED_NAME_BYTES) || !name[0] || name[0] == '.' || strchr(name, '/') || strchr(name, '\\') ||
			strchr(name, ':'))
		{
			return 0;
		}
		if (file == 0)
		{
			if (strcmp(name, "mutator.toml"))
				return 0;
			maximum = MAXIMUM_MUTATOR_FILE_SIZE;
		}
		else if (name_ends(name, ".lua") && !script)
		{
			script = 1;
			maximum = MAXIMUM_SCRIPT_FILE_SIZE;
		}
		else if ((name_ends(name, ".cevehicle") || name_ends(name, ".ceweapon")) && !addon)
		{
			addon = 1;
			maximum = MAXIMUM_ADDON_FILE_SIZE;
		}
		else
		{
			return 0;
		}
		if (!entry->files[file].size || entry->files[file].size > maximum)
			return 0;
	}
	return 1;
}

static void share_folder(const struct port_mutator_share *entry, char *path, size_t size)
{
	static const char digits[] = "0123456789abcdef";
	char text[17];
	int index;

	for (index = 0; index < 8; index++)
	{
		text[index * 2] = digits[entry->digest[index] >> 4];
		text[index * 2 + 1] = digits[entry->digest[index] & 15];
	}
	text[16] = 0;
	snprintf(path, size, "%s/mutator_downloads/%s", platform_data_root(), text);
}

/* the mutator of those digits read from the downloads folder: 0 if it is not whole,
or its files are not its list's, or the game would not play it */
static int share_read_mutator(const struct port_mutator_share *entry, struct port_mutator *mutator)
{
	char folder[MAXIMUM_PATH_LENGTH];
	char path[MAXIMUM_PATH_LENGTH];
	size_t size = 0;
	char *text;
	int result = 0;

	share_folder(entry, folder, sizeof(folder));
	if (strlen(folder) + sizeof("/mutator.toml") > sizeof(path))
		return 0;
	snprintf(path, sizeof(path), "%s/mutator.toml", folder);
	text = config_file_read(path, &size);
	if (!text)
		return 0;
	memset(mutator, 0, sizeof(*mutator));
	mutator_read(mutator, path_file_name(folder), text, size, path);
	free(text);
	if (!mutator->refusal[0] && mutator_digest(mutator) && !memcmp(mutator->digest, entry->digest, SHA256_SIZE))
		result = 1;
	else
		platform_log("mutators: %s is not played: %s", folder, mutator->refusal[0] ? mutator->refusal : "its files are not the host's");
	mutator->on = result;
	return result;
}

/* this machine's own mutator of the digest, or NULL */
static struct port_mutator *share_own_mutator(const struct port_mutator_share *entry)
{
	long index;

	if (!port_mutators.scans)
		port_mutators_scan();
	for (index = 0; index < port_mutators.count; index++)
	{
		struct port_mutator *mutator = &port_mutators.mutators[index];

		if (!mutator->refusal[0] && mutator_digest(mutator) && !memcmp(mutator->digest, entry->digest, SHA256_SIZE))
			return mutator;
	}
	return NULL;
}

int port_mutators_share_has(const struct port_mutator_share *entry)
{
	static struct port_mutator read;

	return share_own_mutator(entry) || share_read_mutator(entry, &read);
}

int port_mutators_share_file_has(const struct port_mutator_share *entry, long file)
{
	char folder[MAXIMUM_PATH_LENGTH];
	char path[MAXIMUM_PATH_LENGTH + SHARED_NAME_BYTES];
	unsigned char sha256[SHA256_SIZE];
	size_t size = 0;
	char *bytes;
	int has;

	share_folder(entry, folder, sizeof(folder));
	snprintf(path, sizeof(path), "%s/%s", folder, entry->files[file].name);
	bytes = config_file_read(path, &size);
	if (!bytes)
		return 0;
	p2p_sha256(bytes, (int)size, sha256);
	has = size == entry->files[file].size && !memcmp(sha256, entry->files[file].sha256, SHA256_SIZE);
	free(bytes);
	return has;
}

/* a file fetched kept in the downloads folder, if it is what the list says */
int port_mutators_share_store(const struct port_mutator_share *entry, long file, const void *bytes, unsigned long size)
{
	char root[MAXIMUM_PATH_LENGTH];
	char folder[MAXIMUM_PATH_LENGTH];
	char path[MAXIMUM_PATH_LENGTH + SHARED_NAME_BYTES];
	unsigned char sha256[SHA256_SIZE];

	if (size != entry->files[file].size)
		return 0;
	p2p_sha256(bytes, (int)size, sha256);
	if (memcmp(sha256, entry->files[file].sha256, SHA256_SIZE))
		return 0;
	snprintf(root, sizeof(root), "%s/mutator_downloads", platform_data_root());
	share_folder(entry, folder, sizeof(folder));
	snprintf(path, sizeof(path), "%s/%s", folder, entry->files[file].name);
	posix_make_directory(root);
	posix_make_directory(folder);
	return share_file_write(path, bytes, size);
}

void port_mutators_host_begin(void)
{
	host_mutator_count = 0;
	host_mutators_on = 1;
}

/* whether a host's mutator has a script */
int port_mutators_share_has_script(const struct port_mutator_share *entry)
{
	long file;

	for (file = 1; file < entry->file_count && file < 3; file++)
	{
		if (name_ends(entry->files[file].name, ".lua"))
			return 1;
	}
	return 0;
}

static void share_allowed_path(const struct port_mutator_share *entry, char *path, size_t size)
{
	char folder[MAXIMUM_PATH_LENGTH];

	share_folder(entry, folder, sizeof(folder));
	snprintf(path, size, "%.*s/script_allowed", (int)(MAXIMUM_PATH_LENGTH - sizeof("/script_allowed")), folder);
}

/* whether a host's script needs no question: the player's own mutator has it, or they
said yes to it */
int port_mutators_share_script_known(const struct port_mutator_share *entry)
{
	char path[MAXIMUM_PATH_LENGTH];
	struct posix_file_information information;

	if (share_own_mutator(entry))
		return 1;
	share_allowed_path(entry, path, sizeof(path));
	return posix_stat(path, &information) == 0;
}

void port_mutators_share_script_allow(const struct port_mutator_share *entry)
{
	char root[MAXIMUM_PATH_LENGTH];
	char folder[MAXIMUM_PATH_LENGTH];
	char path[MAXIMUM_PATH_LENGTH];

	snprintf(root, sizeof(root), "%s/mutator_downloads", platform_data_root());
	share_folder(entry, folder, sizeof(folder));
	share_allowed_path(entry, path, sizeof(path));
	posix_make_directory(root);
	posix_make_directory(folder);
	config_file_write(path, "The player allowed this mutator's Lua script.\n");
}

int port_mutators_host_add(const struct port_mutator_share *entry, int script_allowed)
{
	struct port_mutator *own;
	struct port_mutator *mutator;

	if (host_mutator_count >= MAXIMUM_SHARED_MUTATORS)
		return 0;
	mutator = &host_mutators[host_mutator_count];
	own = share_own_mutator(entry);
	if (own)
	{
		*mutator = *own;
		mutator->on = 1;
	}
	else if (!port_mutators_share_valid(entry) || !share_read_mutator(entry, mutator))
	{
		return 0;
	}
	mutator->script_refused = !script_allowed;
	host_mutator_count++;
	return 1;
}

void port_mutators_host_clear(void)
{
	host_mutators_on = 0;
	host_mutator_count = 0;
}

long port_mutators_addon_count(void)
{
	long count = 0;

	if (!port_mutators.scans)
		port_mutators_scan();
	while (addon_mutator(count))
		count++;
	return count;
}

unsigned long port_mutators_addon_size(long index, char *name, unsigned long name_size)
{
	const struct port_mutator *mutator = addon_mutator(index);
	struct posix_file_information information;

	if (!mutator || posix_stat(mutator->addon_path, &information) != 0 || information.size_high ||
		information.size_low > MAXIMUM_ADDON_FILE_SIZE)
	{
		return 0;
	}
	if (name && name_size)
		snprintf(name, name_size, "%s", mutator->id);
	return (unsigned long)information.size_low;
}

int port_mutators_addon_kind(long index)
{
	const struct port_mutator *mutator = addon_mutator(index);

	return mutator ? mutator->addon_kind : 0;
}

const char *port_mutators_addon_name(long index)
{
	const struct port_mutator *mutator = addon_mutator(index);

	return mutator ? mutator->name : NULL;
}

int port_mutators_addon_read(long index, void *buffer, unsigned long size)
{
	const struct port_mutator *mutator = addon_mutator(index);
	size_t read_size = 0;
	char *bytes;
	int read;

	if (!mutator || !(bytes = config_file_read(mutator->addon_path, &read_size)))
		return 0;
	read = read_size == size;
	if (read)
		memcpy(buffer, bytes, size);
	else
		platform_log("mutators: %s changed as it was read", mutator->addon_path);
	free(bytes);
	return read;
}

long port_mutators_places(struct port_mutator_place *places, long maximum)
{
	long count = 0;
	long index;

	if (!port_mutators.scans)
		port_mutators_scan();
	/* (a client's are its host's) */
	if (host_mutators_on)
	{
		for (index = 0; index < host_mutator_count; index++)
		{
			long place;

			for (place = 0; place < host_mutators[index].place_count && count < maximum; place++)
				places[count++] = host_mutators[index].places[place];
		}
		return count;
	}
	for (index = 0; index < port_mutators.count; index++)
	{
		const struct port_mutator *mutator = &port_mutators.mutators[index];
		long place;

		if (!mutator->on || mutator->refusal[0])
			continue;
		for (place = 0; place < mutator->place_count && count < maximum; place++)
			places[count++] = mutator->places[place];
	}
	return count;
}

long port_mutators_count(void)
{
	return port_mutators.count;
}

const char *port_mutators_name(long index)
{
	return index >= 0 && index < port_mutators.count ? port_mutators.mutators[index].name : "";
}

int port_mutators_on(long index)
{
	return index >= 0 && index < port_mutators.count && port_mutators.mutators[index].on &&
		!port_mutators.mutators[index].refusal[0];
}

int port_mutators_status(long index, char *text, unsigned long size)
{
	if (text && size)
		text[0] = 0;
	if (index < 0 || index >= port_mutators.count || !port_mutators.mutators[index].refusal[0])
		return _port_mutator_fine;
	if (text)
		snprintf(text, size, "It cannot be played: %s.", port_mutators.mutators[index].refusal);
	return _port_mutator_unplayable;
}

int port_mutators_details(long index, char *text, unsigned long size)
{
	const struct port_mutator *mutator;
	size_t length;
	long key;

	if (!text || !size)
		return 0;
	text[0] = 0;
	if (index < 0 || index >= port_mutators.count)
		return 0;
	mutator = &port_mutators.mutators[index];
	if (mutator->version[0])
		snprintf(text, size, "Version %s\n", mutator->version);
	length = strlen(text);
	if (mutator->authors[0])
		snprintf(text + length, size - length, "By %s\n", mutator->authors);
	length = strlen(text);
	if (mutator->stock)
		snprintf(text + length, size - length, "Stock: stock_mutators/%s (written again at each start)\n",
			mutator->folder);
	else
		snprintf(text + length, size - length, "Folder: mutators/%s\n", mutator->folder);
	length = strlen(text);
	if (mutator->taken_by[0] && !mutator->refusal[0])
		snprintf(text + length, size - length, "Played as %s: %s has its id, %s\n", mutator->id, mutator->taken_by,
			mutator->given_id);
	for (key = 0; key < mutator->key_count; key++)
	{
		const char *dot = strchr(mutator->keys[key], '.');

		length = strlen(text);
		if (dot)
			snprintf(text + length, size - length, " %s %g", dot + 1, mutator->values[key]);
		else
			snprintf(text + length, size - length, "%s%s", key ? "\n" : "Effects: ", mutator->keys[key]);
	}
	length = strlen(text);
	if (mutator->tag_edit_count > mutator->pose_count)
		snprintf(text + length, size - length, "%sTag edits: %ld, of %ld tags", mutator->key_count ? "\n" : "",
			mutator->tag_edit_count - mutator->pose_count, mutator->tag_count);
	length = strlen(text);
	if (mutator->pose_count)
		snprintf(text + length, size - length, "%sPoses: %ld", mutator->key_count ||
			mutator->tag_edit_count > mutator->pose_count ? "\n" : "", mutator->pose_count);
	length = strlen(text);
	if (mutator->addon_path[0])
		snprintf(text + length, size - length, "%sAdds: %s, placed %ld times", mutator->key_count ||
			mutator->tag_edit_count ? "\n" : "", strrchr(mutator->addon_path, '/') ?
			strrchr(mutator->addon_path, '/') + 1 : mutator->addon_path, mutator->place_count);
	length = strlen(text);
	snprintf(text + length, size - length, "%sPlayed in the next game you host or play alone; a game you join "
		"plays its host's.\n", mutator->key_count || mutator->tag_edit_count || mutator->addon_path[0] ? "\n" : "");
	length = strlen(text);
	if (mutator->description[0])
		snprintf(text + length, size - length, "\n%s", mutator->description);
	return 1;
}

void port_mutators_turn(long index, int on)
{
	if (index >= 0 && index < port_mutators.count)
		port_mutators.mutators[index].on = on != 0;
}

int port_mutators_save(void)
{
	char setting[257] = "";
	long index;

	for (index = 0; index < port_mutators.count; index++)
	{
		const struct port_mutator *mutator = &port_mutators.mutators[index];
		size_t length = strlen(setting);

		if (!mutator->on || mutator->refusal[0])
			continue;
		if (length + strlen(mutator->id) + 2 >= sizeof(setting))
		{
			platform_log("mutators: too many on to keep in game.mutators: %s and those after it are not kept",
				mutator->id);
			break;
		}
		snprintf(setting + length, sizeof(setting) - length, "%s%s", length ? "," : "", mutator->id);
	}
	if (!config_write("game.mutators", setting))
	{
		platform_log("mutators: game.mutators cannot be written: the mutators on are kept until the game quits");
		return 0;
	}
	return 1;
}

void port_mutators_revert(void)
{
	const char *setting = config_string("game.mutators");
	long index;

	for (index = 0; index < port_mutators.count; index++)
		port_mutators.mutators[index].on = mutators_setting_names(setting, port_mutators.mutators[index].id);
}
