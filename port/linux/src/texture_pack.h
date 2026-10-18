/*
TEXTURE_PACK.H

Texture packs: replacement pictures for the maps' bitmaps, read from loose
DDS files in <data root>/texture_packs/<pack>/ (display.texture_pack names
the pack; empty is the stock game). docs/texture_packs.md has the format.

A replacement is keyed by the bitmap's tag path and its index in the tag:
<pack>/<tag path with / for \>/<index>.dds, for example
texture_packs/mypack/levels/test/bloodgulch/bitmaps/rock/0.dds. It has the
bitmap's aspect and is a power-of-two multiple of its size, with a full mip
chain, as DXT1, DXT3 or DXT5, or (cards with more than 32 MB) as 32-bit
BGRA. A cube map is one DDS cube map. Volumes, HUD meters and bitmaps with
no file keep the maps' own.

The pack's files are untrusted input: the loader checks every size and
index, and a file that is wrong in any way is refused (the stock bitmap is
used, and the refusal logged once). No platform code lives here: the
renderer decides when to ask, and uploads what it is given.
*/

#ifndef TEXTURE_PACK_H
#define TEXTURE_PACK_H

#include <stddef.h>

#define TEXTURE_PACK_FORMAT_RGBA 0
#define TEXTURE_PACK_FORMAT_DXT1 14
#define TEXTURE_PACK_FORMAT_DXT3 15
#define TEXTURE_PACK_FORMAT_DXT5 16

/* a replacement, ready to upload: the levels to upload, face by face
   (faces in OpenGL's order: +X, -X, +Y, -Y, +Z, -Z), each face's levels
   from the largest */
struct texture_pack_image
{
	unsigned int format; /* TEXTURE_PACK_FORMAT_*; RGBA is 8 bits a channel, in that order */
	unsigned int width, height; /* the largest level uploaded */
	unsigned int levels, faces; /* faces: 1, or 6 for a cube map */
	size_t bytes; /* of all the data */
	size_t face_bytes; /* of one face's levels */
	unsigned char *data; /* from malloc; texture_pack_image_free */
};

/* Open the pack named pack in data_root/texture_packs: reads its pack.toml
(a name and a licence are required) and logs a line about it. max_dimension:
the largest edge a level may have (a replacement is uploaded without its
largest levels to fit, and refused if it still does not); allow_dxt, and
allow_rgba (uncompressed): what the card takes and the budget allows;
budget_bytes: the memory the pack's textures may use at once (the renderer's
texture cache). Returns 1 if the pack is open; otherwise 0 and message says
why. The summary is logged when the program exits. */
int texture_pack_open(const char *data_root, const char *pack, unsigned int max_dimension,
	int allow_dxt, int allow_rgba, size_t budget_bytes, char *message, size_t message_size);

/* 1 if a pack is open */
int texture_pack_active(void);

/* Load the replacement for bitmap index of the tag named tag (backslash or
slash separated), whose stock level 0 is stock_width by stock_height with
stock_levels levels (1: no mip maps), cube or not. Returns 1 and fills out
(free it with texture_pack_image_free); 0: none, refused or over budget (the
stock bitmap is used). */
int texture_pack_load(const char *tag, unsigned int index, unsigned int stock_width,
	unsigned int stock_height, unsigned int stock_levels, int cube, struct texture_pack_image *out);

void texture_pack_image_free(struct texture_pack_image *image);

/* one level of one face of a loaded image; NULL if there is none */
const unsigned char *texture_pack_image_level(const struct texture_pack_image *image,
	unsigned int face, unsigned int level, unsigned int *width, unsigned int *height, size_t *bytes);

/* the bytes of one level of a width by height image of a format */
size_t texture_pack_level_bytes(unsigned int format, unsigned int width, unsigned int height);

/* Forget which replacements are in use (the map changed): the budget starts again */
void texture_pack_reset(void);

/* log the one-line summary now (it is also logged at exit, if not yet) */
void texture_pack_summary(void);

/* For pack authors (HALO_MAC_TEXTURE_PACK_DUMP): write the 128 bytes of a DDS
header for a DXT image of a format, width, height, levels, and cube or not;
the caller writes the levels after it, face by face in OpenGL's order. */
void texture_pack_dds_header(unsigned char header[128], unsigned int format, unsigned int width,
	unsigned int height, unsigned int levels, int cube);

/* open the file dir/<tag>/<index>.dds for writing, creating its folders; NULL
if it exists already, or cannot be made (the caller closes it) */
void *texture_pack_dump_open(const char *dir, const char *tag, unsigned int index);

#endif
