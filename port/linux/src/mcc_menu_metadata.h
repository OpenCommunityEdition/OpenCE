/* Local MCC metadata used by menu labels and images, independently of Steam. */
#ifndef MCC_MENU_METADATA_H
#define MCC_MENU_METADATA_H
#include "steam_workshop.h"
struct mcc_menu_metadata {
    char title[STEAM_WORKSHOP_TITLE_SIZE];
    int count;
    struct steam_workshop_map_text maps[STEAM_WORKSHOP_MAXIMUM_ITEM_MAPS];
};
int mcc_menu_metadata_load(const char *root, int campaign, struct mcc_menu_metadata *metadata);
/* An ambiguous basename returns NULL, never an arbitrary map's metadata. */
const struct steam_workshop_map_text *mcc_menu_metadata_find(const struct mcc_menu_metadata *metadata, const char *name);
#endif
