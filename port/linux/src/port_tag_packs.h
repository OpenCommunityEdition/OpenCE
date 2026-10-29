/*
PORT_TAG_PACKS.H

The tag packs' manifests (port_tag_packs.c): one TOML file each in
port/assets/tag_packs, embedded by tools/embed_assets.py, naming a pack
maps/<name>.pack, the maps it is made of, which of their pixels and samples
it keeps, and when it is wanted. The game builds and reads the packs
(port/linux/game/tag_pack.c, tag_import.c, which declare these for
themselves: keep them the same). port/linux/TAG_INJECTION.md has the format.
*/

#ifndef PORT_TAG_PACKS_H
#define PORT_TAG_PACKS_H

/* an embedded manifest, by its file's name in port/assets/tag_packs */
struct tag_pack_manifest_embedded
{
	const char *path;
	const unsigned int *data;
	unsigned int size;
};

extern const struct tag_pack_manifest_embedded tag_pack_manifests_embedded[];
extern const unsigned int tag_pack_manifests_embedded_count;

/* the most maps a pack is made of, and the longest map name (a pack's
record holds it: format 2's 32 bytes, the terminator included) */
#define PORT_TAG_PACK_MAPS 32
#define PORT_TAG_PACK_MAP_LENGTH 32

/* the manifests read (the first call reads them): how many. One that does
not parse, or whose name is not its file's, is logged and left out */
long port_tag_packs_count(void);
/* a pack's name (<owner>_<what>, its file maps/<name>.pack), or -1 */
const char *port_tag_packs_name(long pack);
long port_tag_packs_find(const char *name);
/* a hash of what the pack holds: its version, maps, minimums and keep
rules (what tools/tag_pack.py hashes alike). A pack file of another is
stale, and written again */
unsigned long port_tag_packs_content(long pack);
/* its maps (a map file's name, without .map) */
long port_tag_packs_map_count(long pack);
const char *port_tag_packs_map(long pack, long index);
/* the files it was once called (each renamed to the pack's at start-up) */
long port_tag_packs_legacy_count(long pack);
const char *port_tag_packs_legacy(long pack, long index);
/* the fewest bitmaps and sounds a map of it has whose pixels and samples it
keeps (fewer, and the map's tags were not read right) */
long port_tag_packs_minimum_bitmaps(long pack);
long port_tag_packs_minimum_sounds(long pack);
/* when it is wanted at start-up: "always", "setting:<a boolean setting>",
"condition:<a name tag_pack_conditions.c gives>" */
long port_tag_packs_wanted_count(long pack);
const char *port_tag_packs_wanted(long pack, long index);
/* whether it keeps a bitmap's pixels ('bitm') or a sound's samples
('snd!') of this tag name */
int port_tag_packs_keep(long pack, unsigned long group, const char *name);
/* the words shown while it is written: "notice", "building", "ready",
"failed" (UTF-8, lines split by \n); "" when it has none */
const char *port_tag_packs_text(long pack, const char *which);

#endif
