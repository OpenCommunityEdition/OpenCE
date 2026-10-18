/* The Mac session's stand-ins for the platform layer's functions that the
   game's upstream units call: mods (port/linux/src/port_mods.c), the test input (xinput_sdl.c), and the touch chat layout. The Mac has no mods folder
   and no touch chat, so each says "none"; the units that call
   them (mods.c, network_chat.c, menu_functions.c, network_test.c) are built
   as the other platforms build them. */

long port_mods_scan(void) { return 0; }
unsigned long port_mods_scans(void) { return 0; }
long port_mods_played(void) { return 0; }
long port_mods_file_find(const char *tag_file, const char *map_name) { (void)tag_file; (void)map_name; return -1; }
int port_mods_file_stamp(long file, unsigned long *size, unsigned long stamp[2]) { (void)file; (void)size; (void)stamp; return 0; }
int port_mods_file_read(long file, unsigned long offset, void *buffer, unsigned long size) { (void)file; (void)offset; (void)buffer; (void)size; return 0; }
long port_mods_file_order(long file) { (void)file; return -1; }
const char *port_mods_file_mod(long file) { (void)file; return ""; }
const char *port_mods_file_path(long file) { (void)file; return ""; }
long port_mods_count(void) { return 0; }
const char *port_mods_folder(long index) { (void)index; return ""; }
int port_mods_describe(long index, char *text, unsigned long size, const char *map_name) { (void)index; (void)text; (void)size; (void)map_name; return 0; }
long port_mods_at(long position) { (void)position; return -1; }
const char *port_mods_name(long index) { (void)index; return ""; }
int port_mods_on(long index) { (void)index; return 0; }
int port_mods_status(long index, char *text, unsigned long size) { (void)index; (void)text; (void)size; return 0; }
int port_mods_details(long index, char *text, unsigned long size) { (void)index; (void)text; (void)size; return 0; }
long port_mods_move(long position, long places) { (void)places; return position; }
void port_mods_turn(long index, int on) { (void)index; (void)on; }
int port_mods_save(void) { return 1; }
void port_mods_revert(void) { }
int port_mods_conflicts(char *text, unsigned long size) { (void)text; (void)size; return 0; }
int port_mods_conflicting_names(char *text, unsigned long size) { (void)text; (void)size; return 0; }
void port_mods_disable_conflicting(void) { }
void port_mods_startup(void) { }

void test_input_press(unsigned long buttons) { (void)buttons; }
int halo_chat_touch_layout(void) { return 0; }

/* port: internet play's listings (p2p_hosting_listed, p2p_join_listing, p2p_joined_listed_host) were stubs here
   while the session linked engine_p2p_offline.c; p2p.c defines them now. */
