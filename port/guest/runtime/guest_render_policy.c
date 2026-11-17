#include "guest_host.h"
#include "halo_render_policy.h"

int halo_geometry_fixes = 1;
int halo_texture_fixes = 1;
int halo_render_chip = _halo_chip_unknown;

void halo_render_policy_initialize(void)
{
	halo_render_chip = host_chip_family();
	if (halo_render_chip < _halo_chip_unknown || halo_render_chip > _halo_chip_other)
		halo_render_chip = _halo_chip_unknown;
	halo_geometry_fixes = halo_render_chip == _halo_chip_unknown ||
		halo_render_chip == _halo_chip_snapdragon;
	halo_texture_fixes = halo_render_chip != _halo_chip_other;
}
