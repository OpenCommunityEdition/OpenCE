#ifndef HALO_MAC_ENGINE_TEXT_H
#define HALO_MAC_ENGINE_TEXT_H
#include <stddef.h>
#include <math.h>
#include "port/include/halo_math.h"
#include "text_hires.h"
/* Explicit native display scale. CPU atlas ownership only; no GL upload. */
int mac_text_set_pixel_scale(float scale);
float mac_text_pixel_scale(void);
const unsigned char *mac_text_atlas(size_t *bytes,unsigned int *width,unsigned int *height);
void mac_text_dispose(void);
void mac_text_log(const char *,...);
#endif
