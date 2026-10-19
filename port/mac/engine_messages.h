#ifndef HALO_MAC_ENGINE_MESSAGES_H
#define HALO_MAC_ENGINE_MESSAGES_H

void platform_show_message(const char *title,const char *message);
void mac_engine_message_dispatch(int visible_window);
void mac_engine_message_now(const char *title,const char *message);

#endif
