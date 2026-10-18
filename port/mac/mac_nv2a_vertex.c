/* Compile the unchanged NV2A vertex translator against the native Mac GL
   headers. The shared Linux platform/GL headers are skipped because they
   bind SDL and Winsock; xgpu.h's data/API declarations remain authoritative. */
#include "mac_nv2a_shader_api.h"
#include "../linux/src/nv2a_vsh.c"
