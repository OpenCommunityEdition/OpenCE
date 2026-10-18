#ifndef HALO_MAC_NATIVE_HUD_MESSAGES_H
#define HALO_MAC_NATIVE_HUD_MESSAGES_H
#include "native_object_tags.h"
int mac_native_hud_message_text_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
void mac_native_hud_message_text_owner_free(void *owner);
void mac_native_hud_message_text_tag_free(struct mac_native_object_tag *);
#endif
