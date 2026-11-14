/*
GPU_PROFILE.H

Which GPU and driver this machine draws with (port/linux/src/gpu_profile.c),
and the renderer settings that depend on it. Called once the GL context
exists; display.gpu_profile in config.toml chooses a profile by name instead
of by the driver's text ("auto" asks the driver).
*/

#ifndef GPU_PROFILE_H
#define GPU_PROFILE_H

/* called once the GL context exists, with the driver's own strings */
void gpu_profile_initialize(char const *vendor, char const *renderer, char const *version);
char const *gpu_profile_name(void);
/* draws between pipeline flushes: 0 for none (GPU_PROFILE_INTEL's 3) */
long gpu_profile_flush_draws(void);
/* read the visibility tests' results on the CPU (GPU_PROFILE_AMD's driver
   fills a copied query buffer with wrong counts) */
int gpu_profile_cpu_visibility(void);

#endif
