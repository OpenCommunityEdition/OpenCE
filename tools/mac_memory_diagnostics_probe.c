#define BUILDING_CSERIES 1
#include "engine_xdk_prefix.h"
struct memory_status
{
    unsigned long minimum_available_memory;
    unsigned long maximum_available_memory;
};

static unsigned checks,failures,error_calls;
#define CHECK(x) do { ++checks; if(!(x)){++failures;fprintf(stderr,"memory diagnostics FAIL line %d: %s\n",__LINE__,#x);} } while(0)

void error(short priority,const char *format,...)
{
    (void)priority;(void)format;++error_calls;
}

int main(void)
{
    struct memory_status status;
    MEMORYSTATUS native_status;
    GlobalMemoryStatus(&native_status);
    status.minimum_available_memory=0;
    status.maximum_available_memory=0;
    check_memory_status(&status,"native-memory-probe");
    CHECK(status.minimum_available_memory==0);
    CHECK(status.maximum_available_memory==native_status.dwAvailPhys);
    CHECK(status.maximum_available_memory<=native_status.dwTotalPhys);
    CHECK(error_calls==(native_status.dwAvailPhys>16*1024?1U:0U));
    debug_dump_memory_for_file("probe.map");
    debug_dump_memory_by_file();
    debug_dump_memory();
    CHECK(error_calls==(native_status.dwAvailPhys>16*1024?1U:0U));
    printf("native game memory diagnostic: %u checks %u failures, sampled=%lu, threshold_reports=%lu\n",
        checks,failures,status.maximum_available_memory,(unsigned long)error_calls);
    return failures?1:0;
}
