/* The spawn pad, authored respawn meshes, and timer recordings linked into
 * the game, so a checkout has them without the evolved map files. */
#ifndef EVOLVED_ASSETS_H
#define EVOLVED_ASSETS_H

enum
{
	EVOLVED_ASSET_MARKER = 1,
	EVOLVED_ASSET_AREA = 2,
	EVOLVED_ASSET_SOUND = 3
};

/* bytes stay valid for the rest of the run. 0 when this copy has no match. */
int evolved_assets_find(unsigned kind, char const *name,
	unsigned char const **bytes, unsigned long *size);

#endif
