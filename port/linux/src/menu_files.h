/*
MENU_FILES.H

The menus' files (port/assets/menus: their XML, and the PNGs of their
bitmaps), embedded by tools/embed_assets.py; menu_files.c reads them, and
those in a menus folder beside config.toml, for the game (halo_menus.h).
*/

#ifndef MENU_FILES_H
#define MENU_FILES_H

/* an embedded file, by its path in port/assets/menus */
struct menu_file_embedded
{
	const char *path;
	const unsigned int *data;
	unsigned int size;
};

extern const struct menu_file_embedded menu_files_embedded[];
extern const unsigned int menu_files_embedded_count;

/* the GL texture standing for a menu bitmap's D3D texture (data: its Data),
decoded from its PNG on first use, and its mip levels; 0 if it is none */
unsigned int menu_art_texture(unsigned long data, unsigned long *levels);

#ifdef HALO_ANDROID
/* the Vulkan renderer's: the file of the art registered for the bitmap whose
pixels are at data (NULL if none), and that file's contents (NULL if it
cannot be read) */
const char *menu_art_name(unsigned long data);
/* a number that changes whenever the art registered does, so an answer of
menu_art_name's can be kept until it changes */
unsigned long menu_art_serial(void);
const unsigned char *menu_art_png(const char *name, unsigned long *size);
#endif

#endif
