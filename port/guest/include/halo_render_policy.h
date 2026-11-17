/* Device render policy, selected before the game starts and never changed
while resources exist. Unknown hardware retains both compatibility paths. */
#ifndef HALO_RENDER_POLICY_H
#define HALO_RENDER_POLICY_H

enum halo_chip_family
{
	_halo_chip_unknown,
	_halo_chip_snapdragon,
	_halo_chip_mediatek,
	_halo_chip_other
};

int halo_chip_classify(const char *manufacturer, const char *model,
	const char *hardware, const char *platform);

/* Guest state: written once on its main thread, before game threads start. */
extern int halo_geometry_fixes;
extern int halo_texture_fixes;
extern int halo_render_chip;
void halo_render_policy_initialize(void);

#endif
