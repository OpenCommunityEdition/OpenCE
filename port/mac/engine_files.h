#ifndef HALO_MAC_ENGINE_FILES_H
#define HALO_MAC_ENGINE_FILES_H
#include <stddef.h>

/* Translate Xbox drive paths below the selected native data/save roots.
   Returns zero on success, one on invalid or overlong paths. */
int mac_engine_files_translate_path(const char *,char *,size_t);

#endif
