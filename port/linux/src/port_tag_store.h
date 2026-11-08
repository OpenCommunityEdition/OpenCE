/*
PORT_TAG_STORE.H

How a Tag Injector profile's folder keeps its tags' bundles
(port_tag_store.c): each bundle's tags in a tag file of its own, and its
file data's regions (pixels, samples, vertices) in the folder's pack, each
region once however many of its tags have it, compressed. A bundle is read
back as it was written, byte for byte (port_tag_profiles.c's bundles).
*/

#ifndef PORT_TAG_STORE_H
#define PORT_TAG_STORE_H

/* a tag file's extension (a bundle as it was taken out is .fbrpkg) */
#define PORT_TAG_STORE_EXTENSION ".fbrtag"

/* a region of a bundle's file data (tag_bundle.h's tag_bundle_file_region:
the same layout): its offset in the file data, and its size */
struct port_tag_store_region
{
	unsigned long offset;
	unsigned long size;
};

/* Writes the bundle of `size` bytes at `bundle`, whose first `tags_size`
are its tags and the rest its file data, of the regions `regions` (in the
order they lie, the bytes between them zeros), into the folder `folder`:
the regions its pack has not got added to it, and the tag file `file`.
*stored is the bytes the folder grew by. 0 if it cannot be written (a
region added before stays in the pack, for a later bundle to have, or
port_tag_store_collect to let go). */
int port_tag_store_write(const char *folder, const char *file, const unsigned char *bundle, unsigned long size,
	unsigned long tags_size, const struct port_tag_store_region *regions, long region_count, unsigned long *stored);
/* the size of the bundle a tag file holds, read from its header: 0 when it
is not one this build reads */
unsigned long port_tag_store_bundle_size(const char *path);
/* opens a tag file's bundle to read from: a handle, and the bundle's size;
-1 if it cannot be (the pack has not got its regions) */
long port_tag_store_open(const char *folder, const char *file, unsigned long *size);
/* reads `size` bytes at `offset` of the bundle of an open handle into
buffer (the Xbox's memory too); 0 unless all of them were read */
int port_tag_store_read(long handle, unsigned long offset, void *buffer, unsigned long size);
void port_tag_store_close(long handle);
/* Lets go of the regions of the folder's pack that none of the tag files
`files` (`file_count` of them: the folder's) has: the pack is written
again when they are a quarter of it; with no tag files, the pack and its
index go. 0 if it cannot be read or written (it is as it was). */
int port_tag_store_collect(const char *folder, const char *const *files, long file_count);
/* the bytes of the folder's pack and its index */
unsigned long port_tag_store_pack_size(const char *folder);

#endif
