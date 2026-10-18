/* The Mac session's stand-ins for the platform layer's functions that the
   game's upstream units call: the test input (xinput_sdl.c) and the touch
   chat layout. The Mac has no touch chat, so each says "none"; the units that
   call them (network_chat.c, menu_functions.c, network_test.c) are built as
   the other platforms build them. The mods' are port/linux/src/port_mods.c
   itself, built for the Mac (HALO_MAC). */


void test_input_press(unsigned long buttons) { (void)buttons; }
int halo_chat_touch_layout(void) { return 0; }

/* port: internet play's listings (p2p_hosting_listed, p2p_join_listing, p2p_joined_listed_host) were stubs here
   while the session linked engine_p2p_offline.c; p2p.c defines them now. */
