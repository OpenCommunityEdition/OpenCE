#ifndef HALO_MAC_ENGINE_FILES_H
#define HALO_MAC_ENGINE_FILES_H
#include <stddef.h>

/* Translate Xbox drive paths below the selected native data/save roots.
   Returns zero on success, one on invalid or overlong paths. */
int mac_engine_files_translate_path(const char *,char *,size_t);

/* The folder the game's data is under: HALO_MAC_DATA_ROOT or paths.data,
else the working directory or the bundle's, whichever has a maps folder.
Returns zero when it has none to give. (Declared here rather than by each
caller: platform.c and mac_gpu_textures.c both want it.) */
int mac_engine_files_data_root(char *,size_t);

#endif
