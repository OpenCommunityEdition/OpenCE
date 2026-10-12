/* MCC immutable textures have identities independent of guest physical memory. */
#ifndef MCC_TEXTURE_BRIDGE_H
#define MCC_TEXTURE_BRIDGE_H

void *mcc_texture_bridge_find(void const *bitmap);
void *mcc_texture_bridge_register(void const *bitmap, unsigned long format,
    unsigned long size, void const *pixels, unsigned long bytes);
void *mcc_texture_bridge_register_linear(void const *bitmap, unsigned long format,
    unsigned long width, unsigned long height, unsigned long pitch, void const *pixels, unsigned long bytes);
void mcc_texture_bridge_dispose(void);

#endif
