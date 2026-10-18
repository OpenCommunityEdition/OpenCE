#ifndef HALO_MAC_NATIVE_HUD_MESSAGE_LAYOUT_H
#define HALO_MAC_NATIVE_HUD_MESSAGE_LAYOUT_H
#include "tag_files/tag_groups.h"

/* Private source mirrors: the on-disk HUD message group is declared in
   hud_messaging.c, so these witnesses keep the conversion independent of its
   runtime-only state structures. */
struct mac_native_hud_state_message_element { byte type, data; };
struct mac_native_hud_state_message {
    char name[32];
    word text_start_index, element_start_index;
    byte element_count, pad25[3];
    long unused28[6];
};
struct mac_native_hud_message_text {
    struct tag_data text_data;
    struct tag_block elements, messages;
    long unused2C[21];
};
typedef char mac_hud_state_message_element_size[
    sizeof(struct mac_native_hud_state_message_element)==2?1:-1];
typedef char mac_hud_state_message_size[
    sizeof(struct mac_native_hud_state_message)==0x40?1:-1];
typedef char mac_hud_message_text_size[
    sizeof(struct mac_native_hud_message_text)==0x80?1:-1];
#endif
