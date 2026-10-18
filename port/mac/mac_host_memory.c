#include "mac_host_memory.h"
#include <mach/mach.h>
#include <sys/types.h>
#include <sys/sysctl.h>
int mac_host_memory_query(struct mac_host_memory *out)
{
    int mib[2]={CTL_HW,HW_MEMSIZE};
    size_t size;
    uint64_t physical=0,available;
    vm_statistics_data_t statistics;
    mach_msg_type_number_t count=HOST_VM_INFO_COUNT;
    vm_size_t page_size;
    mach_port_t host;
    kern_return_t result;
    if(!out)return 0;
    out->physical=out->available=0;
    size=sizeof(physical);
    if(sysctl(mib,2,&physical,&size,NULL,0)!=0 || size!=sizeof(physical) || !physical)return 0;
    host=mach_host_self();
    result=host_page_size(host,&page_size);
    if(result==KERN_SUCCESS)
        result=host_statistics(host,HOST_VM_INFO,(host_info_t)&statistics,&count);
    mach_port_deallocate(mach_task_self(),host);
    if(result!=KERN_SUCCESS || count<HOST_VM_INFO_COUNT)return 0;
    available=((uint64_t)statistics.free_count+statistics.inactive_count)*page_size;
    out->physical=physical;out->available=available<physical?available:physical;
    return 1;
}
