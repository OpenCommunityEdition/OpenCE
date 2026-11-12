/*
PORT_TAG_PROFILES.H

The Faber Tag Vault's profiles in the data root's OpenCE Forge/tag_vault folder
(port_tag_profiles.c): each a name and the tags taken out of Custom Edition
or Xbox maps into it, as compressed tag bundles (.fbrtag + faber_vault.bin).
*/

#ifndef PORT_TAG_PROFILES_H
#define PORT_TAG_PROFILES_H

struct port_tag_store_region;

/* finds the profiles again: how many there are. Every profile and tag index
from before is gone */
long port_tag_profiles_scan(void);
/* how many profiles the last scan found */
long port_tag_profiles_count(void);
/* the profile of the folder `folder` (any case), or -1 */
long port_tag_profiles_find(const char *folder);
/* makes a profile named `name` (its folder's name made of its letters,
digits, spaces, - and _): its index, or -1 */
long port_tag_profiles_create(const char *name);
/* deletes a profile, its folder and its bundles; 0 if it cannot */
int port_tag_profiles_delete(long profile);

/* a profile's tags, each its group and its path */
long port_tag_profiles_tag_count(long profile);
unsigned long port_tag_profiles_tag_group(long profile, long tag);
const char *port_tag_profiles_tag_name(long profile, long tag);
/* a tag's bundle's bytes, as the last scan found it (0: missing) */
unsigned long port_tag_profiles_tag_size(long profile, long tag);
/* whether a tag is played (on), or kept in the profile and left out (off);
set_ writes the profile: 0 if it cannot. Turning a tag on clears its
"failed" setting */
int port_tag_profiles_tag_enabled(long profile, long tag);
int port_tag_profiles_tag_set_enabled(long profile, long tag, int enabled);
/* a tag's setting, by its key: "faction", "type" (an actor variant's) and
"class" (a weapon's), "custom_type", "hud", "replace", "name" (the name the
menus show it by; its path stays its map's), "pickup", "pickup_text", and
"failed"; "" when it is not set. set_ keeps the value's letters, digits,
spaces, - and _ (lower case but for custom_type's, name's and pickup_text's),
and writes the profile: 0 if it cannot */
const char *port_tag_profiles_tag_option(long profile, long tag, const char *key);
int port_tag_profiles_tag_set_option(long profile, long tag, const char *key, const char *value);
/* the tag of a group and path (any case) of a profile, or -1 */
long port_tag_profiles_tag_find(long profile, unsigned long group, const char *name);
/* adds a tag to a profile, with its bundle (`size` bytes), or replaces the
one of its group and path: with its file data's regions (`region_count` of
them, after its first `tags_size` bytes), it is kept as a tag file and the
profile's pack has its regions, else as it is; 0 if it cannot be written */
int port_tag_profiles_tag_add(long profile, const char *map, unsigned long group, const char *name,
	const void *bundle, unsigned long size, unsigned long tags_size, const struct port_tag_store_region *regions,
	long region_count);
/* takes a tag out of a profile, and deletes its bundle; 0 if it cannot */
int port_tag_profiles_tag_remove(long profile, long tag);
/* opens a tag's bundle for a level to read from, whatever profiles are
found meanwhile: a handle, and its size; -1 if it cannot be */
long port_tag_profiles_bundle_open(long profile, long tag, unsigned long *size);
/* reads `size` bytes at `offset` of an open bundle into buffer (the Xbox's
memory too); 0 unless all of them were read */
int port_tag_profiles_bundle_read_at(long handle, unsigned long offset, void *buffer, unsigned long size);
void port_tag_profiles_bundle_close(long handle);

/* writes a file whole, beside it first (a write cut short leaves it as it
was), and puts a file in another's place; 0 if it cannot */
int port_tag_profiles_file_write(const char *path, const void *bytes, unsigned long size);
int port_tag_profiles_file_replace(const char *from, const char *to);

#endif
