#ifndef HALO_MAC_ENGINE_MENU_ART_H
#define HALO_MAC_ENGINE_MENU_ART_H
/* The menus' pictures (the PNGs of port/assets/menus), drawn for the small
   placeholder textures the game made for its menu bitmaps. Plain C types:
   this unit does not see the game's headers. */
void mac_menu_art_register(const void *texture, const char *png);
void mac_menu_art_forget(void);
/* 1 and the GL texture (RGBA, one level) with the scale from the bitmap's
   own units (width by height) to its coordinates; 0 if the texture is not
   one of the menus'; -1 if its picture cannot be drawn. */
int mac_menu_art_texture(const void *texture, long width, long height,
    unsigned int *name, float scale[4]);
#endif
