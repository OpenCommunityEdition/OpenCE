/*
GPU_PROFILE.C

Which GPU and driver this machine draws with, and the few settings that
differ because of it (port/linux/src/d3d8_gl.c asks).
One place for the driver checks that were spread through the renderer as
string compares, and a name in the log, so a bug report says "Adreno 740"
instead of nothing.
*/

#include "gpu_profile.h"
#include "platform.h"
#include "port_config.h"

#include <stdio.h>
#include <string.h>

enum
{
	_gpu_profile_generic = 0,
	_gpu_profile_intel,
	_gpu_profile_amd,
	_gpu_profile_nvidia,
	_gpu_profile_adreno,
	_gpu_profile_mali,
	_gpu_profile_powervr,
	_gpu_profile_angle,
	_gpu_profile_software,
	_gpu_profile_count
};

static char const *const profile_names[_gpu_profile_count] =
{
	"generic", "intel", "amd", "nvidia", "adreno", "mali", "powervr", "angle", "software"
};

static struct
{
	int initialized;
	long profile;
	char renderer[160];
	char vendor[96];
	char version[96];
	long flush_draws;
	int cpu_visibility;
} gpu_profile;

static int text_has(char const *text, char const *needle)
{
	return text && needle && strstr(text, needle) != NULL;
}

static long profile_from_name(char const *name)
{
	long index;

	if (!name || !name[0] || !strcmp(name, "auto"))
		return -1;
	for (index = 0; index < _gpu_profile_count; index++)
		if (!strcmp(name, profile_names[index]))
			return index;
	return -1;
}

static long profile_classify(char const *vendor, char const *renderer)
{
	/* (ANGLE reports the driver it wraps in the renderer's text; name it
	first so the wrapper is what a driver bug is filed against) */
	if (text_has(renderer, "ANGLE"))
		return _gpu_profile_angle;
	if (text_has(renderer, "llvmpipe") || text_has(renderer, "softpipe") ||
		text_has(renderer, "SwiftShader"))
		return _gpu_profile_software;
	if (text_has(renderer, "Adreno") || text_has(vendor, "Qualcomm"))
		return _gpu_profile_adreno;
	if (text_has(renderer, "Mali") || text_has(vendor, "ARM"))
		return _gpu_profile_mali;
	if (text_has(renderer, "PowerVR") || text_has(vendor, "Imagination"))
		return _gpu_profile_powervr;
	if (text_has(renderer, "Intel") || text_has(vendor, "Intel"))
		return _gpu_profile_intel;
	if (text_has(vendor, "ATI") || text_has(vendor, "AMD") || text_has(renderer, "AMD"))
		return _gpu_profile_amd;
	if (text_has(vendor, "NVIDIA") || text_has(renderer, "NVIDIA"))
		return _gpu_profile_nvidia;
	return _gpu_profile_generic;
}

void gpu_profile_initialize(char const *vendor, char const *renderer, char const *version)
{
	char const *chosen;
	long profile;

	if (gpu_profile.initialized)
		return;
	gpu_profile.initialized = 1;
	snprintf(gpu_profile.vendor, sizeof(gpu_profile.vendor), "%s", vendor ? vendor : "?");
	snprintf(gpu_profile.renderer, sizeof(gpu_profile.renderer), "%s", renderer ? renderer : "?");
	snprintf(gpu_profile.version, sizeof(gpu_profile.version), "%s", version ? version : "?");
	chosen = config_string("display.gpu_profile");
	profile = profile_from_name(chosen);
	if (profile < 0)
		profile = profile_classify(gpu_profile.vendor, gpu_profile.renderer);
	gpu_profile.profile = profile;
	/* the ones that have needed one so far (d3d8_gl.c) */
	gpu_profile.flush_draws = profile == _gpu_profile_intel ? 3 : 0;
	gpu_profile.cpu_visibility = profile == _gpu_profile_amd;
	platform_log("GPU: %s (%s) %s; profile %s%s", gpu_profile.renderer, gpu_profile.vendor,
		gpu_profile.version, gpu_profile_name(),
		(chosen && chosen[0] && strcmp(chosen, "auto")) ? " (chosen by display.gpu_profile)" : "");
}

char const *gpu_profile_name(void)
{
	return gpu_profile.initialized ? profile_names[gpu_profile.profile] : "unknown";
}

long gpu_profile_flush_draws(void)
{
	return gpu_profile.initialized ? gpu_profile.flush_draws : 0;
}

int gpu_profile_cpu_visibility(void)
{
	return gpu_profile.initialized ? gpu_profile.cpu_visibility : 0;
}
