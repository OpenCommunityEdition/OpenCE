#ifndef HALO_MAC_NATIVE_MODEL_GPU_PROBE_H
#define HALO_MAC_NATIVE_MODEL_GPU_PROBE_H
#include <stddef.h>
struct mac_native_model_gpu_result { unsigned parts,vertices,pixels; };
/* Hidden diagnostic draw of one live object through its owned compressed
   model buffers and the native indexed D3D subset. */
int mac_native_model_gpu_probe_object(long,struct mac_native_model_gpu_result *,const char **);
#endif
