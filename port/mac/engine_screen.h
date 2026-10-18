#ifndef HALO_MAC_ENGINE_SCREEN_H
#define HALO_MAC_ENGINE_SCREEN_H
long mac_engine_ui_offset(void);
long halo_screen_commit(void);
void halo_screen_ui_offset(unsigned char centered);
void mac_engine_vblank_presented(void);
void mac_engine_vblank_stop(void);
volatile unsigned int *d3d_find_flipcount(void);
#endif
