/*
WORKSHOP_COLLECTIONS.C

The Halo CE map collections of the Steam Workshop (workshop_collections.h). The names are the collections' own;
the lists are the item ids each collection holds.
*/

#include "workshop_collections.h"

#include <string.h>

struct collection
{
	const char *name;
	/* comma-separated Workshop item ids */
	const char *items;
};

static const struct collection collections[] =
{
	{ "PRO EDITION",
		"2899666869,2899343509,2899270824,2899340399,2899341154,2899339889,2899301014,2899352017,2899342285,"
		"2899345854" },
	{ "NHE MAP PACK",
		"2899272390,2899273793,2900197649,2899298743,2899269216,2899300038,2899285963,2899296946,2899282492,"
		"2899286494,2900198928,2899277913,2899299390,2899275294,2899291914,2900080350" },
	{ "SOCIAL MAP PACK",
		"2899509722,2905217149,2899679058,2899688264,2899709584,2899741750,2899747345,2899751222,2899770689,"
		"2899774877,2900050996,2900072240,2900166697,2900177725,2900199550,2900215828,2900242317,2900252105,"
		"2900257741" },
	{ "2V2 MAPS",
		"2899747345,2905217149,3783952417,2899770689,3644749294,3779099103,3504896675,3765551554,3723117625,"
		"3598668955,3795273247,3690378743" },
	{ "CE+ BEYOND", "3007701381" },
};

#define COLLECTION_COUNT ((int)(sizeof(collections) / sizeof(collections[0])))

int workshop_collection_count(void)
{
	return COLLECTION_COUNT;
}

const char *workshop_collection_name(int collection)
{
	return collection >= 0 && collection < COLLECTION_COUNT ? collections[collection].name : "";
}

/* whether the comma-separated `list` holds exactly the id `id` of `length` digits */
static int list_has(const char *list, const char *id, size_t length)
{
	while (*list)
	{
		const char *end = strchr(list, ',');
		size_t size = end ? (size_t)(end - list) : strlen(list);

		if (size == length && !memcmp(list, id, length))
			return 1;
		if (!end)
			break;
		list = end + 1;
	}
	return 0;
}

int workshop_collections_of_item(const char *item_id, int *collections_out, int maximum)
{
	size_t length;
	int collection, count = 0;

	if (!item_id || !item_id[0] || maximum < 1)
		return 0;
	for (length = 0; item_id[length]; length++)
	{
		if (item_id[length] < '0' || item_id[length] > '9')
			return 0;
	}
	for (collection = 0; collection < COLLECTION_COUNT && count < maximum; collection++)
	{
		if (list_has(collections[collection].items, item_id, length))
			collections_out[count++] = collection;
	}
	return count;
}
