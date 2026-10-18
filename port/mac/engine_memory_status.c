/* Match the shared port's 64 MiB engine-visible physical-memory budget, using
   actual Darwin free/reclaimable pages. This is not a hardware inventory API.
   Virtual/commit limits are left unknown (zero); engine callers use the physical
   fields only. Never report all memory free when the host query fails. */
#define BUILDING_CSERIES 1
#include "engine_xdk_prefix.h"
#include "mac_host_memory.h"
#include "engine_handles.h"
void GlobalMemoryStatus(struct _MEMORYSTATUS *status)
{
    struct mac_host_memory memory;
    uint64_t total,available;
    if(!status){SetLastError(87);return;}
    memset(status,0,sizeof(*status));status->dwLength=sizeof(*status);
    if(!mac_host_memory_query(&memory)){SetLastError(31);return;}
    total=memory.physical<67108864ULL?memory.physical:67108864ULL;
    available=memory.available<total?memory.available:total;
    status->dwTotalPhys=(unsigned long)total;
    status->dwAvailPhys=(unsigned long)available;
    status->dwMemoryLoad=(unsigned long)(100-available*100/total);
}
