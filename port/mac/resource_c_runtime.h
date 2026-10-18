#ifndef HALO_MAC_RESOURCE_C_RUNTIME_H
#define HALO_MAC_RESOURCE_C_RUNTIME_H
/* Shared resource code is platform code, not game code: use Darwin's C runtime
   after the clean XDK declaration prefix has supplied the native call ABI. */
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef memmove
#undef memcmp
#undef strncpy
#undef strlen
#endif
