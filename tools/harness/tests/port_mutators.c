/*
The mutators' files (port/linux/src/port_mutators.c), whole, over a fake
mutators folder: the stock mutators written into stock_mutators (again when
changed, not when unchanged) and played as the game has them (also when they
cannot be written), mutator.toml read and refused, the order, the ids
game.mutators turns on, and what the Mods menu keeps.
*/

#include "harness.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
/* (the library is C11: the harness builds C99) */
#define static_assert _Static_assert
#include "tomlc17.inc"
#pragma clang diagnostic pop

/* the folder: each entry a folder (with its mutator.toml's text, or NULL:
none) */
struct fake_folder { char const *name; char const *text; unsigned long size; };
static struct fake_folder folders[8];
/* files beside the mutators' mutator.toml (their add-ons), by path */
struct fake_file { char const *path; char const *bytes; unsigned long size; };
static struct fake_file files[4];
static int file_count;
static int folder_count, listed, reads, logged;
static char setting[300], written[300];

#include "port_mutators_source.inc"

static void folder(char const *name, char const *text)
{
	folders[folder_count].name = name;
	folders[folder_count].text = text;
	folders[folder_count++].size = text ? (unsigned long)strlen(text) : 0;
}

static void file(char const *path, char const *bytes)
{
	files[file_count].path = path;
	files[file_count].bytes = bytes;
	files[file_count++].size = (unsigned long)strlen(bytes);
}

/* whether the file written at `path` has `text` in it */
static int written_holds(char const *path, char const *text)
{
	int index;

	for (index = 0; index < 16; index++)
	{
		if (files_written[index].text && !strcmp(files_written[index].path, path))
			return strstr(files_written[index].text, text) != NULL;
	}
	return 0;
}

static long find(char const *id)
{
	long index;

	for (index = 0; index < port_mutators_count(); index++)
	{
		if (!strcmp(port_mutators.mutators[index].id, id))
			return index;
	}
	return NONE;
}

int main(int argc, char **argv)
{
	char const *case_name = argv[1];
	char keys[16][PORT_MUTATOR_KEY_LENGTH];
	double values[16];
	char text[1024];

	CHECK(argc == 2, "case");
	if (!strcmp(case_name, "stock"))
	{
		static unsigned char pose[4096];
		size_t size = 0;
		char *readme;

		CHECK(port_mutators_scan() == 0, "a mutator on with game.mutators empty");
		CHECK(port_mutators_count() == 4 && find("accurized_ma5b") == 0 && find("big_head") == 1 &&
			find("paintball") == 2 && find("revolved_plasma_rifle") == 3, "the stock mutators, by folder");
		CHECK(file_writes == 5, "the README and the four mutator.toml written (%d)", file_writes);
		CHECK(written_holds("/data/stock_mutators/big_head/mutator.toml", "node = \"head\"\nscale = 2.5\n") &&
			written_holds("/data/stock_mutators/accurized_ma5b/mutator.toml", "id = \"accurized_ma5b\"") &&
			written_holds("/data/stock_mutators/revolved_plasma_rifle/mutator.toml", "group = \"damage_effect\""),
			"the stock mutators' files");
		readme = config_file_read("/data/stock_mutators/README.txt", &size);
		CHECK(readme && strstr(readme, "copy one of"), "the README");
		free(readme);
		port_mutators_scan();
		CHECK(file_writes == 5, "files that hold the game's written again (%d)", file_writes);
		/* (written after the README and accurized_ma5b's) */
		CHECK(!strcmp(files_written[2].path, "/data/stock_mutators/big_head/mutator.toml"), "the order written");
		strcpy(files_written[2].text, "id = \"changed\"\n[effects.big_head]\nscale = 4\n");
		port_mutators_scan();
		CHECK(file_writes == 6 && written_holds(files_written[2].path, "format = 6"), "a changed file not put back");
		CHECK(find("changed") == NONE, "a changed file played");
		CHECK(port_mutators_status(0, text, sizeof(text)) == _port_mutator_fine, "%s", text);
		CHECK(port_mutators_status(1, text, sizeof(text)) == _port_mutator_fine, "%s", text);
		CHECK(port_mutators_status(2, text, sizeof(text)) == _port_mutator_fine, "%s", text);
		CHECK(port_mutators_status(3, text, sizeof(text)) == _port_mutator_fine, "%s", text);
		CHECK(port_mutators_keys(keys, values, 16) == 0, "keys of a mutator off");
		strcpy(setting, " Big_Head ");
		CHECK(port_mutators_scan() == 1 && port_mutators_on(1), "game.mutators's id (any case, spaced)");
		/* (a pose: an edit, and no key) */
		CHECK(port_mutators_keys(keys, values, 16) == 0 && port_mutators_tag_edits(pose, sizeof(pose)) > 0,
			"the big head's pose");
		strcpy(setting, "big_head_x,xbig_head");
		CHECK(port_mutators_scan() == 0, "an id taken for another it begins or ends");
		CHECK(port_mutators_details(1, text, sizeof(text)) && strstr(text, "stock_mutators/big_head") &&
			strstr(text, "Poses: 1"), "the details: %s", text);
	}
	else if (!strcmp(case_name, "stock_unwritable"))
	{
		files_unwritable = 1;
		CHECK(port_mutators_scan() == 0 && port_mutators_count() == 4, "the stock mutators unwritten not played");
		CHECK(port_mutators_status(0, text, sizeof(text)) == _port_mutator_fine &&
			port_mutators_status(2, text, sizeof(text)) == _port_mutator_fine, "%s", text);
		CHECK(logged == 1, "the folder unwritten logged (%d)", logged);
		port_mutators_scan();
		CHECK(logged == 1, "the folder unwritten logged again (%d)", logged);
		strcpy(setting, "big_head");
		CHECK(port_mutators_scan() == 1 && port_mutators_on(1), "a stock mutator unwritten turned on");
	}
	else if (!strcmp(case_name, "folders"))
	{
		static char large[70000];

		memset(large, '#', sizeof(large) - 1);
		folder("zeta", "name = \"Huge heads\"\n[effects.big_head]\nscale = 4\n");
		folder("alpha", "id = \"moon\"\n[effects.moon_gravity]\nscale = 0.2\n");
		folder("broken", "this is [not toml");
		folder("copy", "id = \"BIG_HEAD\"\n[effects.big_head]\n");
		folder("none", NULL);
		folder("range", "[effects.big_head]\nscale = 40\n");
		folder("large", large);
		folder("unknown", "run = \"run()\"\n[effects.big_head]\n");
		strcpy(setting, "zeta,moon,big_head,copy,range,large,unknown");
		port_mutators_scan();
		CHECK(port_mutators_count() == 11, "the folders with a mutator.toml, and the stock mutators (%ld)",
			port_mutators_count());
		CHECK(find("accurized_ma5b") == 0 && find("big_head") == 1 && find("paintball") == 2 &&
			find("revolved_plasma_rifle") == 3 && find("moon") == 4 && find("broken") == 5 &&
			find("BIG_HEAD@copy") == 6 && find("large") == 7 && find("range") == 8 &&
			find("unknown") == 9 && find("zeta") == 10, "the stock mutators first, then the folders by name");
		CHECK(reads == 6, "the large file read, or one without mutator.toml (%d)", reads);
		CHECK(port_mutators_status(find("moon"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, "moon_gravity"), "an effect this build lacks: %s", text);
		CHECK(port_mutators_status(find("broken"), text, sizeof(text)) == _port_mutator_unplayable, "not TOML");
		CHECK(logged >= 7, "the refusals and the folder without mutator.toml not logged (%d)", logged);
		CHECK(port_mutators_status(find("BIG_HEAD@copy"), text, sizeof(text)) == _port_mutator_fine,
			"another's id (any case), played by its folder's: %s", text);
		CHECK(port_mutators_details(find("zeta"), text, sizeof(text)) && strstr(text, "Folder: mutators/zeta"),
			"a folder's details: %s", text);
		CHECK(port_mutators_status(find("range"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, "from 1 to 4"), "out of range: %s", text);
		CHECK(port_mutators_status(find("large"), text, sizeof(text)) == _port_mutator_unplayable, "64 KB");
		CHECK(port_mutators_status(find("unknown"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, "run"), "a key this build does not know: %s", text);
		CHECK(!port_mutators_on(find("moon")) && !port_mutators_on(find("range")), "a refused mutator on");
		/* (the stock big head's is a pose: zeta's alone) */
		CHECK(port_mutators_keys(keys, values, 16) == 2 && !strcmp(keys[0], "big_head") && values[0] == 1.0 &&
			!strcmp(keys[1], "big_head.scale") && values[1] == 4.0, "the keys of those that can be played, in order");
		CHECK(!strcmp(port_mutators_name(find("zeta")), "Huge heads"), "the name");
		CHECK(port_mutators_keys(keys, values, 1) == 1, "the most keys asked for");
	}
	else if (!strcmp(case_name, "scripts"))
	{
		extern char *port_mutators_script_read(const char *key, unsigned long *size);
		extern void port_mutators_script_free(char *text);
		extern int port_mutators_share_valid(const struct port_mutator_share *entry);
		extern int port_mutators_share_store(const struct port_mutator_share *entry, long file, const void *bytes,
			unsigned long size);
		extern long port_mutators_script_controls(struct port_mutator_control *controls, long maximum);
		extern int port_mutators_script_control_write(long index, const char *name, const char *value);
		struct port_mutator_control rows[4];
		char keys[4][PORT_MUTATOR_KEY_LENGTH];
		double values[4];
		char text[128];
		char *script;
		unsigned long size = 0;

		folder("view", "format = 5\n[script]\nfile = \"main.lua\"\ndistance = 2.5\nkeyboard = \"F5\"\n"
			"[script.menu]\nkeyboard = \"THIRD PERSON\"\n");
		file("/data/mutators/view/main.lua", "function frame() end");
		folder("missing", "[script]\nfile = \"gone.lua\"\n");
		folder("path", "[script]\nfile = \"../main.lua\"\n");
		folder("other", "[script]\nfile = \"main.txt\"\n");
		folder("nofile", "[script]\ndistance = 2.5\n");
		folder("notable", "script = \"main.lua\"\n");
		folder("later", "format = 6\n[script]\nfile = \"main.lua\"\n");
		port_mutators_scan();
		CHECK(port_mutators_status(find("view"), text, sizeof(text)) == _port_mutator_fine, "refused: %s", text);
		CHECK(port_mutators_status(find("missing"), text, sizeof(text)) == _port_mutator_unplayable, "a script not there");
		CHECK(port_mutators_status(find("path"), text, sizeof(text)) == _port_mutator_unplayable, "a script in another folder");
		CHECK(port_mutators_status(find("other"), text, sizeof(text)) == _port_mutator_unplayable, "a script not .lua");
		CHECK(port_mutators_status(find("nofile"), text, sizeof(text)) == _port_mutator_unplayable, "no file named");
		CHECK(port_mutators_status(find("notable"), text, sizeof(text)) == _port_mutator_unplayable, "not a table");
		CHECK(port_mutators_status(find("later"), text, sizeof(text)) == _port_mutator_unplayable, "a later format");
		port_mutators_turn(find("view"), 1);
		CHECK(port_mutators_keys(keys, values, 4) == 1 && !strncmp(keys[0], "script.", 7) && strlen(keys[0]) == 31,
			"its key: %s", keys[0]);
		script = port_mutators_script_read(keys[0], &size);
		CHECK(script && size == 20 && !memcmp(script, "function", 8), "its file");
		port_mutators_script_free(script);
		CHECK(!port_mutators_script_read("script.000000000000000000000000", &size), "another's key found");
		/* a control's row, only while its mutator is on, and its value written */
		CHECK(port_mutators_script_controls(rows, 4) == 1 && !strcmp(rows[0].name, "keyboard") &&
			!strcmp(rows[0].label, "THIRD PERSON") && !strcmp(rows[0].value, "F5") && !strcmp(rows[0].fallback, "F5"),
			"its row");
		CHECK(port_mutators_script_control_write(rows[0].mutator, "keyboard", "F6, G"), "not written");
		CHECK(written_holds("/data/mutators/view/settings.toml", "\"keyboard\" = \"F6, G\""), "what it wrote");
		port_mutators_turn(find("view"), 0);
		CHECK(port_mutators_script_controls(rows, 4) == 0, "a row with its mutator off");
		/* what a host lists: only a mutator.toml first, then one add-on and one script,
		of plain names, no larger than the game reads; nothing else is asked for */
		{
			struct port_mutator_share entry;

			memset(&entry, 0, sizeof(entry));
			entry.file_count = 2;
			strcpy(entry.files[0].name, "mutator.toml");
			entry.files[0].size = 100;
			strcpy(entry.files[1].name, "main.lua");
			entry.files[1].size = 100;
			CHECK(port_mutators_share_valid(&entry), "a mutator and its script");
			strcpy(entry.files[1].name, "../../evil.lua");
			CHECK(!port_mutators_share_valid(&entry), "a name out of its folder");
			strcpy(entry.files[1].name, "c:evil.lua");
			CHECK(!port_mutators_share_valid(&entry), "a name of a drive");
			strcpy(entry.files[1].name, "evil.exe");
			CHECK(!port_mutators_share_valid(&entry), "a file of another kind");
			strcpy(entry.files[1].name, "main.lua");
			entry.files[1].size = 0x40001;
			CHECK(!port_mutators_share_valid(&entry), "a script too large");
			entry.files[1].size = 100;
			strcpy(entry.files[0].name, "other.toml");
			CHECK(!port_mutators_share_valid(&entry), "no mutator.toml first");
			strcpy(entry.files[0].name, "mutator.toml");
			memset(entry.files[1].name, 'a', sizeof(entry.files[1].name));
			CHECK(!port_mutators_share_valid(&entry), "a name not ended");
			strcpy(entry.files[1].name, "main.lua");
			entry.file_count = 4;
			CHECK(!port_mutators_share_valid(&entry), "too many files");
			entry.file_count = 2;
			/* (bytes that are not what the list says are not kept) */
			CHECK(!port_mutators_share_store(&entry, 1, "not those bytes", 15), "another file's size kept");
			entry.files[1].size = 15;
			CHECK(!port_mutators_share_store(&entry, 1, "not those bytes", 15), "another file's bytes kept");
		}
	}
	else if (!strcmp(case_name, "addons"))
	{
		struct port_mutator_place places[8];
		char bytes[64];
		char name[64];

		folder("falcon", "format = 3\naddon = \"falcon.cevehicle\"\n"
			"[[place]]\nmap = \"bloodgulch\"\nvehicle = 'vehicles\\falcon\\falcon'\n"
			"position = [35.75, -98.335, 1]\nfacing = -48.6\n"
			"[[place]]\nmap = \"sidewinder\"\nvehicle = 'vehicles\\falcon\\falcon'\nposition = [1, 2, 3]\n");
		file("/data/mutators/falcon/falcon.cevehicle", "cevh: the add-on's bytes");
		folder("missing", "addon = \"gone.cevehicle\"\n");
		folder("path", "addon = \"sub/falcon.cevehicle\"\n");
		folder("unplaced", "[effects.big_head]\n[[place]]\nmap = \"bloodgulch\"\nvehicle = 'v'\nposition = [0, 0, 0]\n");
		folder("position", "addon = \"p.cevehicle\"\n[[place]]\nmap = \"m\"\nvehicle = 'v'\nposition = [0, 0]\n");
		file("/data/mutators/position/p.cevehicle", "cevh");
		folder("unknown", "addon = \"u.cevehicle\"\n[[place]]\nmap = \"m\"\nvehicle = 'v'\nposition = [0, 0, 0]\n"
			"team = \"red\"\n");
		file("/data/mutators/unknown/u.cevehicle", "cevh");
		strcpy(setting, "falcon,missing,path,unplaced,position,unknown");
		port_mutators_scan();
		CHECK(port_mutators_status(find("falcon"), text, sizeof(text)) == _port_mutator_fine, "an add-on: %s", text);
		CHECK(port_mutators_status(find("missing"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, "not there"), "an add-on's file that is not there: %s", text);
		CHECK(port_mutators_status(find("path"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, "beside"), "an add-on in another folder: %s", text);
		CHECK(port_mutators_status(find("unplaced"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, "no addon"), "placements without an add-on: %s", text);
		CHECK(port_mutators_status(find("position"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, "three numbers"), "a position of two numbers: %s", text);
		CHECK(port_mutators_status(find("unknown"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, "team"), "a placement's key this build does not know: %s", text);
		CHECK(port_mutators_addon_count() == 1, "the add-ons of the mutators on that can be played (%ld)",
			port_mutators_addon_count());
		CHECK(port_mutators_addon_size(0, name, sizeof(name)) == strlen("cevh: the add-on's bytes") &&
			!strcmp(name, "falcon"), "the add-on's size and its mutator's id");
		memset(bytes, 0, sizeof(bytes));
		CHECK(port_mutators_addon_read(0, bytes, strlen("cevh: the add-on's bytes")) &&
			!strcmp(bytes, "cevh: the add-on's bytes"), "the add-on's bytes");
		CHECK(!port_mutators_addon_read(0, bytes, 4), "a read of another size");
		CHECK(port_mutators_addon_size(1, name, sizeof(name)) == 0, "past the add-ons");
		CHECK(port_mutators_places(places, 8) == 2 && !strcmp(places[0].map, "bloodgulch") &&
			!strcmp(places[0].tag, "vehicles\\falcon\\falcon") && places[0].position[1] == -98.335f &&
			places[0].facing == -48.6f && places[1].facing == 0.f && places[1].position[2] == 3.f,
			"the placements, in order");
		CHECK(port_mutators_places(places, 1) == 1, "the most placements asked for");
		CHECK(port_mutators_details(find("falcon"), text, sizeof(text)) &&
			strstr(text, "Adds: falcon.cevehicle, placed 2 times"), "an add-on's details: %s", text);
	}
	else if (!strcmp(case_name, "ids"))
	{
		long hashed;
		char hashed_id[64];
		int digit;

		/* (stock_mutators/big_head's text: the same mutator again) */
		folder("big_head", (char const *)mutator_files_embedded[1].data);
		folder("mine", "id = \"big_head\"\n[effects.big_head]\nscale = 4\n");
		folder("accurized_ma5b", "[effects.big_head]\nscale = 3\n");
		folder("at", "id = \"a@b\"\n[effects.big_head]\n");
		strcpy(setting, "big_head@mine");
		CHECK(port_mutators_scan() == 1 && port_mutators_count() == 7, "the same mutator again played once (%ld)",
			port_mutators_count());
		CHECK(find("big_head") == 1 && !strcmp(port_mutators.mutators[1].folder, "big_head") &&
			port_mutators.mutators[1].stock, "the stock mutator kept");
		text[0] = 0;
		CHECK(find("big_head@mine") == 6 && port_mutators_on(6) &&
			port_mutators_status(6, text, sizeof(text)) == _port_mutator_fine, "its folder's name: %s", text);
		CHECK(port_mutators_keys(keys, values, 16) == 2 && values[1] == 4.0, "big_head@mine's keys");
		CHECK(port_mutators_details(6, text, sizeof(text)) &&
			strstr(text, "Played as big_head@mine: stock_mutators/big_head has its id, big_head"), "%s", text);
		/* (its folder's name is the stock one's: its files' SHA-256's first
		hex digits) */
		hashed = 4;
		CHECK(!strcmp(port_mutators.mutators[hashed].folder, "accurized_ma5b") &&
			!port_mutators.mutators[hashed].stock, "the order: %s", port_mutators.mutators[hashed].id);
		snprintf(hashed_id, sizeof(hashed_id), "accurized_ma5b@");
		for (digit = 0; digit < 6; digit++)
		{
			unsigned char byte = port_mutators.mutators[hashed].digest[digit / 2];

			snprintf(hashed_id + strlen(hashed_id), sizeof(hashed_id) - strlen(hashed_id), "%x",
				digit % 2 ? byte & 0xF : byte >> 4);
		}
		CHECK(!strcmp(port_mutators.mutators[hashed].id, hashed_id) &&
			port_mutators_status(hashed, text, sizeof(text)) == _port_mutator_fine, "%s, not %s",
			port_mutators.mutators[hashed].id, hashed_id);
		CHECK(port_mutators_status(find("a@b"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, "@"), "an id with an @: %s", text);
		port_mutators_scan();
		CHECK(find(hashed_id) == hashed && find("big_head@mine") == 6, "the same ids found again");
	}
	else if (!strcmp(case_name, "weapons"))
	{
		struct port_mutator_place places[8];

		folder("dmr", "format = 4\naddon = \"dmr.ceweapon\"\n"
			"[[place]]\nmap = \"bloodgulch\"\nweapon = 'bourrin\\weapons\\dmr\\dmr'\nposition = [31, -88, 0.5]\n");
		file("/data/mutators/dmr/dmr.ceweapon", "cevh: a weapon");
		folder("falcon", "format = 3\naddon = \"falcon.CEVEHICLE\"\n");
		file("/data/mutators/falcon/falcon.CEVEHICLE", "cevh: a vehicle");
		folder("kind", "addon = \"thing.bin\"\n");
		file("/data/mutators/kind/thing.bin", "cevh");
		folder("both", "addon = \"b.ceweapon\"\n[[place]]\nmap = \"m\"\nweapon = 'w'\nvehicle = 'v'\n"
			"position = [0, 0, 0]\n");
		file("/data/mutators/both/b.ceweapon", "cevh");
		strcpy(setting, "dmr,falcon,kind,both");
		port_mutators_scan();
		CHECK(port_mutators_status(find("dmr"), text, sizeof(text)) == _port_mutator_fine, "a weapon: %s", text);
		CHECK(port_mutators_status(find("kind"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, ".ceweapon"), "an add-on of neither kind: %s", text);
		CHECK(port_mutators_status(find("both"), text, sizeof(text)) == _port_mutator_unplayable &&
			strstr(text, "a vehicle or a weapon"), "a place of both: %s", text);
		CHECK(port_mutators_addon_count() == 2 && port_mutators_addon_kind(0) == _port_mutator_addon_weapon &&
			port_mutators_addon_kind(1) == _port_mutator_addon_vehicle && port_mutators_addon_kind(2) == 0,
			"the add-ons' kinds, by their names (any case), in order");
		CHECK(!strcmp(port_mutators_addon_name(0), "dmr") && port_mutators_addon_name(2) == NULL, "their names");
		CHECK(port_mutators_places(places, 8) == 1 && places[0].weapon &&
			!strcmp(places[0].tag, "bourrin\\weapons\\dmr\\dmr") && places[0].position[2] == 0.5f,
			"a weapon's place");
	}
	else if (!strcmp(case_name, "menu"))
	{
		folder("zeta", "[effects.big_head]\nscale = 3\n");
		folder("bad", "[effects.nothing]\n");
		strcpy(setting, "zeta");
		port_mutators_scan();
		port_mutators_turn(find("big_head"), 1);
		port_mutators_turn(find("zeta"), 0);
		port_mutators_turn(find("bad"), 1);
		CHECK(port_mutators_save() && !strcmp(written, "big_head"), "kept: %s", written);
		port_mutators_turn(find("zeta"), 1);
		port_mutators_revert();
		CHECK(!port_mutators_on(find("zeta")) && port_mutators_on(find("big_head")), "reverted to what was kept");
		port_mutators_turn(find("zeta"), 1);
		CHECK(port_mutators_save() && !strcmp(written, "big_head,zeta"), "kept: %s", written);
		port_mutators_turn(-1, 1);
		port_mutators_turn(99, 1);
		CHECK(!port_mutators_on(-1) && !port_mutators_details(99, text, sizeof(text)), "an index past the list");
	}
	else
	{
		CHECK(0, "no case %s", case_name);
	}
	return 0;
}
