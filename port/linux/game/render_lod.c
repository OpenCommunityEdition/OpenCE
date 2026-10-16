#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "models/model_definitions.h"
#include "render_lod.h"

int config_boolean(char const *name);
unsigned long config_changes(void);

boolean render_lod_disabled(void)
{
	static boolean initialized, disabled;
	static unsigned long read_at;
	unsigned long changes = config_changes();

	if (!initialized || read_at != changes)
	{
		disabled = config_boolean("display.disable_lod") != 0;
		initialized = TRUE;
		read_at = changes;
	}
	return disabled;
}

boolean render_lod_model_disabled(long model_index)
{
	/* The cryotube's highest LOD includes a headless cinematic Chief.
	Keep its authored selection, including across map changes/tag reuse. */
	return render_lod_disabled() && csstrcasecmp(tag_get_name(model_index),
		"levels\\a10\\devices\\cryotube\\cryotube") != 0;
}

boolean render_lod_particle_visible(real diameter, real cutoff)
{
	/* A zero/negative projection must not reach minimum-pixel scaling's
	division by diameter, even with the size cutoff disabled. */
	return render_lod_disabled() ? diameter > 0.0f : diameter > cutoff;
}

short render_lod_geometry_level(struct model const *model, real pixels,
	short debug_level, boolean full_detail)
{
	short level = NUMBEROF(model->detail_cutoff_pixels) - 1;

	if (debug_level != NONE)
		return PIN(debug_level, 0, NUMBEROF(model->detail_cutoff_pixels) - 1);
	if (full_detail)
		return level;
	while (level > 0 && pixels < model->detail_cutoff_pixels[level])
		level--;
	return level;
}

short render_lod_permutation_level(struct model_region_permutation const *permutation,
	short level, boolean full_detail)
{
	/* Only the full-detail override fills missing LODs. Authored selection
	   and explicit debug levels must still be able to hide a permutation. */
	while (full_detail && level > 0 && permutation->geometry_indices[level] == NONE)
		level--;
	return level;
}
