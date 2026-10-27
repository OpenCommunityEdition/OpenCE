#include "guest_host.h"
#include "android_render_policy.h"

int android_geometry_fixes = 1;
int android_texture_fixes = 1;
int android_render_chip = _android_chip_unknown;

void android_render_policy_initialize(void)
{
	android_render_chip = host_android_chip_family();
	if (android_render_chip < _android_chip_unknown || android_render_chip > _android_chip_other)
		android_render_chip = _android_chip_unknown;
	android_geometry_fixes = android_render_chip == _android_chip_unknown ||
		android_render_chip == _android_chip_snapdragon;
	android_texture_fixes = android_render_chip != _android_chip_other;
}
