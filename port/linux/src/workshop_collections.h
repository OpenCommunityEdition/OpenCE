/*
WORKSHOP_COLLECTIONS.H

The Steam Workshop collections of Halo CE maps that the MCC map menus group their items by. A collection is a
name and the ids of the Workshop items in it; the menus show one folder for it, holding the maps of the items the
player has. Which items are in a collection is the collection maker's list on the Workshop, written here as data:
Steam's folders on disk say only which items are subscribed. An item in two collections is in both; an item in none
keeps a folder of its own (mcc_maps.c).
*/

#ifndef __HALO_LINUX_WORKSHOP_COLLECTIONS_H
#define __HALO_LINUX_WORKSHOP_COLLECTIONS_H

enum
{
	/* an item is listed under at most this many collections */
	WORKSHOP_COLLECTION_MAXIMUM_PER_ITEM = 4,
};

/* the number of collections */
int workshop_collection_count(void);
/* a collection's name, as the menus show it; "" for a number out of range */
const char *workshop_collection_name(int collection);
/* the collections that hold the Workshop item `item_id` (decimal digits): how many were written to `collections`
(at most `maximum`), 0 for an item in none */
int workshop_collections_of_item(const char *item_id, int *collections, int maximum);

#endif
