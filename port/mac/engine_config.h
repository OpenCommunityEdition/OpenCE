#ifndef HALO_MAC_ENGINE_CONFIG_H
#define HALO_MAC_ENGINE_CONFIG_H
#include <stddef.h>
const char *mac_config_base_path(void);
void *mac_config_load_file(const char *, size_t *);
int mac_config_save_file(const char *, const void *, size_t);
void mac_config_log(const char *, ...);
/* The time of the frame just begun (seconds), for halo_interpolation_enabled */
void mac_interpolation_note_frame(double seconds);
#endif
