#ifndef HALO_MAC_RESOURCE_RELEASE_PROBE_H
#define HALO_MAC_RESOURCE_RELEASE_PROBE_H
/* Diagnostic guards: actual shared Release must not reach either deallocator
   for tag-owned headers. These are not production platform implementations. */
static void mac_resource_unexpected_free(void *p)
{(void)p;fprintf(stderr,"shared D3D Release tried to free a tag-owned resource\n");abort();}
#define PLATFORM_PHYSICAL_TO_VIRTUAL(p) ((void *)((unsigned long)(p)|0x80000000UL))
#define platform_contiguous_free mac_resource_unexpected_free
#define free mac_resource_unexpected_free
#include "build/mac-audit/mac_resource_release_body.inc"
#undef free
#undef platform_contiguous_free
#undef PLATFORM_PHYSICAL_TO_VIRTUAL
#endif
