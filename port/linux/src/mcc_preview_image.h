/* Package-owned menu images. No network access or map-family fallback. */
#ifndef MCC_PREVIEW_IMAGE_H
#define MCC_PREVIEW_IMAGE_H
#include <stdint.h>
#define MCC_PREVIEW_FILE_LIMIT (8u * 1024u * 1024u)
#define MCC_PREVIEW_DIMENSION_LIMIT 2048u
struct mcc_preview_image { unsigned width, height; uint32_t *pixels; };
/* 8-bit RGB/RGBA, non-interlaced PNG. Pixels are native ARGB words. */
int mcc_preview_png(const unsigned char *data, unsigned size, struct mcc_preview_image *image);
int mcc_preview_image_load(const char *level, int campaign, struct mcc_preview_image *image);
/* Release through this module: the game-side debug allocator is different. */
void mcc_preview_image_free(struct mcc_preview_image *image);
/* Pure aspect fit, in UI coordinates. Empty/invalid inputs return zero. */
int mcc_preview_fit(unsigned width, unsigned height, int box_width, int box_height, int *width_out, int *height_out);
#endif
