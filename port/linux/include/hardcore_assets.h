/* The spawn pad, authored respawn meshes, and timer recordings linked into
 * the game, so a checkout has them without the hardcore map files. */
#ifndef HARDCORE_ASSETS_H
#define HARDCORE_ASSETS_H

enum
{
	HARDCORE_ASSET_MARKER = 1,
	HARDCORE_ASSET_AREA = 2,
	HARDCORE_ASSET_SOUND = 3
};

/* bytes stay valid for the rest of the run. 0 when this copy has no match. */
int hardcore_assets_find(unsigned kind, char const *name,
	unsigned char const **bytes, unsigned long *size);

#endif
