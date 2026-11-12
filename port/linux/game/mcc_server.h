/* MCC-only integration with the dedicated server's rotation and catalog. */
#ifndef MCC_SERVER_H
#define MCC_SERVER_H
struct server_entry;
void mcc_server_resolve(struct server_entry *entry);
int mcc_server_maps_each(void (*each)(void *, const char *, const char *, const char *), void *context);
#endif
