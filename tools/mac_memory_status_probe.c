#define BUILDING_CSERIES 1
#include "engine_xdk_prefix.h"
#include "mac_host_memory.h"
#include "engine_handles.h"
int main(void)
{
    MEMORYSTATUS status;
    struct mac_host_memory host;
    unsigned checks=0,failures=0;
#define CHECK(x) do { ++checks;if(!(x)){++failures;fprintf(stderr,"memory status FAIL line %d\n",__LINE__);} } while(0)
    CHECK(sizeof(status)==32);
    CHECK(!mac_host_memory_query(NULL));
    CHECK(mac_host_memory_query(&host));
    CHECK(host.physical>=67108864ULL && host.available<=host.physical);
    GlobalMemoryStatus(&status);
    CHECK(status.dwLength==32);
    CHECK(status.dwTotalPhys==67108864UL);
    CHECK(status.dwAvailPhys<=status.dwTotalPhys);
    CHECK(status.dwMemoryLoad<=100);
    CHECK(status.dwMemoryLoad==100-(uint64_t)status.dwAvailPhys*100/status.dwTotalPhys);
    CHECK(!status.dwTotalVirtual && !status.dwAvailVirtual && !status.dwTotalPageFile && !status.dwAvailPageFile);
    GlobalMemoryStatus(NULL);CHECK(GetLastError()==87);
    printf("native memory status: %u checks %u failures; physical=%llu available=%llu engine budget=%lu free=%lu\n",checks,failures,(unsigned long long)host.physical,(unsigned long long)host.available,status.dwTotalPhys,status.dwAvailPhys);
    return failures?1:0;
}
