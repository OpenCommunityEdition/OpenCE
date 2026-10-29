# Tag injection: tag packs and the tag importer

A level can be given tags it does not have, copied from other maps: a game
mode's effects and models (Floodfection's Flood forms and their burst, from
Two Betrayals), another campaign map's actors and weapons. This is done in two parts:

- **Tag packs** (`port/linux/game/tag_pack.c`): `maps/<name>.pack`, each
  holding some maps' tag sections and the bitmap pixels and sound samples of
  theirs a manifest keeps. A pack is written once, at the main menu, from
  the player's own maps, and written again when its manifest or its maps
  change.
- **The tag importer** (`port/linux/game/tag_import.c`): as a level loads,
  the tags asked for (the *roots*) are read from a pack, with every tag they
  name, copied, and appended to the level's tag table. The game then uses
  them as its own: `tag_loaded(group, name)` finds them.

A level never reads a map for its tags. A pack that is not written yet
gives the level nothing: the feature that wanted it does without (and logs
so).

## Files

| File | What it is |
| --- | --- |
| `port/assets/tag_packs/*.toml` | The manifests, one per pack, embedded in the game by `tools/embed_assets.py` |
| `port/linux/src/port_tag_packs.c`, `.h` | Reads the manifests (TOML, `port/third_party/tomlc17`) and hashes what each pack holds |
| `port/linux/game/tag_pack.c`, `.h` | Writes the packs, checks them, renames old ones, asks for them at start-up and on request |
| `port/linux/game/tag_pack_conditions.c` | The `condition:` names a manifest may want its pack by |
| `port/linux/game/tag_import.c`, `.h` | Copies tags from a pack into the loading level |
| `port/linux/game/level_tag_import.c` | A level's import, each user's roots in a fixed order (`cache_files.c` calls it as the level's tags load) |
| `tools/tag_pack.py` | Writes the same packs on a computer, and lists each manifest's content hash |

OpenCE has no user of it yet: a user adds its roots in `level_tag_import.c`.

## Making a pack

A pack is described by a manifest, `port/assets/tag_packs/<name>.toml`.
Rebuilding the game embeds it; nothing else needs to list it.

```toml
name = "opence_floodfection"      # the file's name, and maps/<name>.pack
version = 1                       # raise it by hand only when a keep rule
                                  # means something else than it says
legacy_file_names = []            # files this pack was once called: renamed
                                  # to maps/<name>.pack at start-up
maps = ["c40"]                    # map files (without .map) it is made of:
                                  # at most 32, each at most 31 letters
minimum_bitmaps = 1               # a map with fewer kept bitmaps or sounds
minimum_sounds = 0                # was not read right: it is left out
wanted = ["always"]               # when the main menu writes it at start-up

[keep]                            # whose pixels and samples are kept
include_contains = ["flood"]      # by the bitmap's or sound's tag name,
include_prefixes = ["effects\\"]  # any case: when there are includes, it
exclude_contains = []             # must match one of them; and it must
exclude_bitmap_prefixes = []      # match none of the excludes (the prefix
exclude_sound_prefixes = []       # ones of its own kind)

[text]                            # what the main menu shows (ASCII)
notice = "The game modes need\nsome data from other maps.\nIt takes a moment."
building = "Building game mode data:\n"
ready = "The game mode data is ready."
failed = "The game mode data could not\nbe built. Is the card full?"
```

**Names.** A pack's name is its owner's prefix, an underscore and what it
is: `opence_floodfection`, `watanoge_gamemodes`. Lower-case letters, digits and
`_` only, and the same as the file's name. The prefix keeps two builds'
packs from ever being one file: each developer owns their prefix. A
manifest whose name breaks these rules is logged and left out.

**What it holds.** Every tag of every map listed is in the pack (the whole
tag section, so any tag of those maps can be a root). Only the bitmap pixels
and sound samples the keep rules match are. Keep them to what your roots
draw and play: a pack of the ten campaign maps' actors and weapons is about
500 MB; Floodfection's keeps the Flood's and the effects' of one map (about
25 MB). A copied bitmap whose pixels the pack has not got is read from its
map as the level loads (16 MB of them at most a level), else drawn black; a
sound's samples are silent.

A tag section is 7 to 17 MB a map, so a pack of five maps for a few HUDs is
still some 70 MB. Use as few maps as the roots need.

**Staleness.** What a pack holds is hashed from its manifest's version,
maps, minimums and keep rules (`port_tag_packs_content`), and the hash is
in the pack's header. A pack of another hash, or of another format, or one
of whose maps was replaced since, is stale: it is not read, and the main
menu writes it again when it is next wanted. So changing a manifest's maps
or keep rules is enough to have every player's pack rebuilt; `version` is
only for a change the rules' text does not show. The `[text]` and `wanted`
parts are not hashed. `python3 tools/tag_pack.py --list` prints each
manifest's hash; the game logs the same (`tag packs: <name>, <n> maps,
content <hash>`) when it reads the manifests.

**Legacy names.** A pack once written under another name (a build's
`gamemodes.pack` before its owner's prefix) lists it in
`legacy_file_names`. At start-up a legacy file is renamed to the pack's
name, or deleted when the pack is there already. The renamed file is then
checked like any pack (an old format is stale and written again).

### Writing a pack on a computer

```sh
python3 tools/tag_pack.py                         # every manifest's pack
python3 tools/tag_pack.py opence_floodfection        # some of them
python3 tools/tag_pack.py --maps-dir ~/halo/maps --output-dir /tmp/packs opence_floodfection
python3 tools/tag_pack.py --list                  # maps and content hashes
```

It writes what the game writes, so a pack it makes is one the game reads
(and one it makes from other maps than the player's is stale for them: a
pack records each map's length, tag offset and tag size).

## Asking for a pack

**At start-up.** The first frame of the main menu renames legacy files, then
asks for every pack whose manifest's `wanted` says so and is not ready:

- `"always"`: whatever is set (a pack any multiplayer game may need: a
  client cannot know which mode it will join).
- `"setting:<key>"`: a boolean setting of `config.toml` is on
  (`"setting:game.<key>"`).
- `"condition:<name>"`: a function of `tag_pack_conditions.c` says so
  (`"condition:<name>"`, `tag_pack_conditions[]`). Add a line there for a
  question only code can answer.

A pack none of whose maps is there is never asked for (Floodfection's, on a
machine without `c40.map`).

**From code**, when a feature is turned on or a game is joined
(`tag_pack.h`):

```c
#include "tag_pack.h"

tag_pack_request("opence_floodfection"); /* nothing when it is ready */
if (!tag_pack_ready("opence_floodfection"))
	/* ... the level will not have its tags */;
```

`tag_pack_request` queues the pack. The main menu shows its notice, writes
it behind a dialog that shows its progress, then says it is ready or could
not be written. Packs are written one at a time, in the order asked.
`tag_pack_busy()` is true while one is queued or being written (a question
the main menu asks may wait for it). `tag_pack_file_name(name)` is its path, for
messages.

A network game that needs a pack should check it before joining (in
`network_game_client_advertised_game_compatible`,
`source/networking/network_client_manager.c`): without it the player is
told, the pack is asked for, and nothing is joined. A game a machine without
the pack still plays alike (below) needs no check: its pack is written at
start-up.

## Copying tags into a level

A level's import is one `tag_import_begin`, any number of root lists, one
`tag_import_run` and one `tag_import_end`. `level_tag_import_loaded`
(`level_tag_import.c`, called by `cache_files.c` as the level's tags load)
makes it for everyone, in a fixed order. A mode adds its roots there:

```c
#include "tag_import.h"

void opence_mode_tag_import(void)
{
	static struct tag_import_root const roots[] =
	{
		{ 'effe', "characters\\floodcarrier\\effects\\body destroyed" },
		{ 'mode', "characters\\floodcombat elite\\floodcombat elite" },
		{ 'antr', "characters\\floodcombat elite\\floodcombat elite" },
	};

	tag_import_roots("opence_floodfection", "c40", roots, (int)NUMBEROF(roots));
}

/* level_tag_import_loaded */
if (opence_mode_tag_import_wanted() && tag_import_begin())
{
	opence_mode_tag_import();
	tag_import_run();
	tag_import_end();
	tag_import_log("opence mode");
}
```

Afterwards (`level_loaded`, a tick, the renderer) the copies are the
level's tags: `tag_loaded('effe', "characters\\floodcarrier\\effects\\body
destroyed")` is their index, NONE when the import did not happen (no pack,
no room). Always handle NONE.

What the importer does with a root:

- A root the level already has is left: the level's own is used.
- Everything a root names (shaders, bitmaps, sounds, effects, models,
  animations, through every tag reference and tag index the tag schema
  knows) is copied too, or the root is not taken at all.
- Scenarios, structures, globals and vehicles are never copied: a reference
  to one is pointed at the level's tag of that name, or NONE.
- A tag asked for from two maps is copied once, from the map asked first.
- A model's parts get vertex and index buffers made for them; a bitmap's
  pixels and a sound's samples are read from the pack when the game draws
  or plays them.
- A level copies at most 2048 tags (32 MB of them) from at most 16 maps of
  8 packs. A root whose tags do not fit is not taken, nor any root after
  it; running out of memory while copying keeps no copy at all.
- `tag_import_level_unloaded` (from `level_tag_import_unloaded`) lets them
  all go with the level.

To choose roots by looking at a map (every weapon of it, say), open it and
want tags by their index:

```c
struct tag_import_donor *donor = tag_import_open("opence_campaign", "b30");
long tag;

for (tag = 0; donor && tag < tag_import_tag_count(donor); tag++)
{
	if (tag_import_tag_group(donor, tag) == WEAPON_DEFINITION_TAG)
		tag_import_want(donor, tag);
}
tag_import_release(donor);
```

`tag_import_tag_data`, `tag_import_address` and `tag_import_string` read
a donor's tags, bounds-checked. `tag_import_peek` opens a map the same way
outside a level (the menus: is this a campaign tag?), from its pack, or the
map itself when the pack is not written; close it with
`tag_import_peek_close`.

## Network games

Copies are appended to the tag table, so every tag after them moves. In a
network game, every machine must make the same import in the same order,
or the tags after it (anything sent over the network by its index) are
other tags on each machine:

- Decide from what every machine of the game has alike: the game's variant
  and options (the game's mode, not a local setting), the host's
  advertisement.
- Ask for the same roots in the same order. The importer sorts its copies
  by map, then name, so the same roots give the same table.
- Make sure every machine has the pack: write it at start-up (`always`) or
  refuse a join without it.
- Every level logs its table's fingerprint (`tag_import_tag_table_fingerprint`:
  `... tag table: <n> tags, fingerprint <hash>`); two machines' lines must
  match.

A game a machine without the importer can join (a take on a mode that a
build without it plays as the mode itself) may use the copies only for what each
machine draws and plays for itself (effects, a model drawn in place of
another), never for anything it sends.

## The pack file

Little-endian. Written as `<name>.pack.partial` and renamed when whole.

| Offset | Size | |
| --- | --- | --- |
| 0 | 4 | `'rpak'` |
| 4 | 4 | format: 2 (`TAG_PACK_FORMAT_VERSION`) |
| 8 | 4 | the manifest's content hash |
| 12 | 4 | how many maps it has |
| 16 | 56 each | a record per map of the manifest (those it has first) |

A record: the map's name (32 bytes, NUL-padded), its file length, tag
offset and tag size (from its header, to tell a replaced map), where its
tag section is in the pack, how many pixel and sample ranges it has, and
where their table is. The table has 12 bytes a range, sorted by offset: the
offset in the map, the size, where the bytes are in the pack. Identical
ranges of bytes are stored once. Format 1 (a build's before) had 8-byte
names and its content in the version word.

## OpenCE's packs

None yet: each is listed here with its maps, when it is wanted, its size
and what uses it.
