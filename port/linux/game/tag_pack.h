/*
TAG_PACK.H

The tag packs (tag_pack.c): maps/<name>.pack, each a few maps' tags and the
pixels and samples a manifest keeps of them (port/assets/tag_packs:
port/linux/src/port_tag_packs.c), written once by the main menu from the
maps in maps/, behind a notice, and read by the tag importer
(tag_import.c) as a level loads. port/linux/TAG_INJECTION.md tells how to
make one.
*/

#ifndef __TAG_PACK_H
#define __TAG_PACK_H

/* the packs' file format: 'rpak', this, the pack's content hash
(port_tag_packs_content), how many maps; then a 56-byte record each: the
map's name (32 bytes), its file length, tag offset and tag size, where its
tags are in the pack, how many pixel and sample ranges and where their
table is (twelve bytes each: the offset in the map, the size, where the
bytes are in the pack). tools/tag_pack.py writes the same */
#define TAG_PACK_FORMAT_VERSION 2
#define TAG_PACK_HEADER_SIZE 16
#define TAG_PACK_RECORD_SIZE 56
#define TAG_PACK_MAP_NAME_SIZE 32
#define TAG_PACK_MAPS 32

/* a pack's file (maps/<name>.pack, which the map directory is put before),
or NULL when no manifest has that name */
char const *tag_pack_file_name(char const *name);
/* what it holds: its manifest's hash, which its header must carry */
unsigned long tag_pack_content(char const *name);

/* the pack is there, written by this build's manifest, and each map of it
in maps/ is the one it was written from */
int tag_pack_ready(char const *name);
/* the pack is wanted: a notice, then, at the main menu, the pack written.
Nothing when it is ready, or none of its maps is there */
void tag_pack_request(char const *name);
/* a pack's notice or dialog up, or a pack waiting for its turn */
int tag_pack_busy(void);
/* the main menu, each frame (main.c): at the first, the packs once called
otherwise renamed, and those their manifests want at start-up asked for;
then the one asked for first written */
void tag_pack_update(boolean main_menu);

/* a map's header, as a pack records it. 0 when the map is not there or is
not an Xbox map */
int tag_pack_map_header(char const *leaf, long *file_length, long *tag_offset, long *tag_size, int *compressed);

/* a manifest's "condition:<name>" (tag_pack_conditions.c): whether the pack
is wanted at start-up */
struct tag_pack_condition
{
	char const *name;
	int (*wanted)(void);
};

extern struct tag_pack_condition const tag_pack_conditions[];

#endif /* __TAG_PACK_H */
