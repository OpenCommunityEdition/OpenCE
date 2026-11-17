/* Browser-owned persistence for MCC packages; native publication is unchanged. */
#ifndef WEB_MCC_STORAGE_H
#define WEB_MCC_STORAGE_H
#include <stdint.h>
struct mcc_share_io;
void web_mcc_share_io(struct mcc_share_io *io);
uint64_t web_mcc_free_bytes(void);
int web_mcc_package_visible(char const *level);
int web_mcc_directory_visible(char const *directory);
int web_mcc_remove_begin(char const *folder);
void web_mcc_remove_end(char const *folder);
#endif
