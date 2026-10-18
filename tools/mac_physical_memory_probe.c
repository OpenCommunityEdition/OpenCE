#include <mach/mach.h>
#include <stdio.h>
#include <string.h>
#include "engine_physical_memory.h"
#include "halo_port_capacity.h"

#undef fprintf
#undef printf

static unsigned int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); } } while (0)

int main(void)
{
    unsigned char *a, *fixed;
    unsigned long i;
    vm_address_t blocker;
    CHECK(mac_engine_physical_memory_start());
    CHECK(!mac_engine_physical_memory_start());
    CHECK(XPhysicalAlloc(0,0,0,PAGE_READWRITE)==NULL);
    CHECK(XPhysicalAlloc(4096,0,3,PAGE_READWRITE)==NULL);
    CHECK(XPhysicalAlloc(4096,0,0,0x1000)==NULL);
    CHECK(XPhysicalAlloc(0x08000000UL,0,0,PAGE_READWRITE)==NULL);

    a=(unsigned char *)XPhysicalAlloc(8192,0xffffffffUL,0,PAGE_READWRITE|PAGE_WRITECOMBINE);
    CHECK(a!=NULL);
    CHECK(a && (unsigned long)a>=0x80000000UL && (unsigned long)a<0x88000000UL);
    CHECK(a && !((unsigned long)a&4095UL));
    CHECK(a && XQueryMemoryProtect(a)==PAGE_READWRITE);
    if(a){
        for(i=0;i<8192;++i)if(a[i])break;
        CHECK(i==8192);
        a[0]=0x5a;a[8191]=0xa5;CHECK(a[0]==0x5a&&a[8191]==0xa5);
        CHECK(XQueryMemoryProtect(a+4096)==PAGE_READWRITE);
        XPhysicalProtect(a,4096,PAGE_READONLY);
        CHECK(XQueryMemoryProtect(a)==PAGE_READONLY);
        CHECK(XQueryMemoryProtect(a+4096)==PAGE_READWRITE);
        XPhysicalProtect(a,4096,PAGE_READWRITE);
        CHECK(XQueryMemoryProtect(a)==PAGE_READWRITE);
        XPhysicalProtect(a+4096,4096,PAGE_NOACCESS);
        CHECK(XQueryMemoryProtect(a+4096)==PAGE_NOACCESS);
        XPhysicalProtect(a+4096,4096,PAGE_READWRITE);
        CHECK(XQueryMemoryProtect(a+4096)==PAGE_READWRITE);
        XPhysicalProtect(a+8190,8,PAGE_READONLY);
        CHECK(XQueryMemoryProtect(a+4096)==PAGE_READWRITE);
        XPhysicalProtect(a,0,PAGE_READONLY);
        CHECK(XQueryMemoryProtect(a)==PAGE_READWRITE);
        XPhysicalFree(a+1);
        CHECK(mac_engine_physical_memory_allocation_count()==1);
        CHECK(XQueryMemoryProtect(a)==PAGE_READWRITE);
    }

    fixed=(unsigned char *)XPhysicalAlloc(4096,0x00600000UL,0,PAGE_READWRITE);
    CHECK(fixed==(unsigned char *)0x80600000UL);
    CHECK(XPhysicalAlloc(4096,0x00600000UL,0,PAGE_READWRITE)==NULL);
    CHECK(XPhysicalAlloc(4096,0x08000000UL,0,PAGE_READWRITE)==NULL);
    CHECK(XPhysicalAlloc(4096,0x00600001UL,0,PAGE_READWRITE)==NULL);
    blocker=0x80700000UL;
    CHECK(vm_allocate(mach_task_self(),&blocker,4096,FALSE)==KERN_SUCCESS);
    CHECK(XPhysicalAlloc(4096,0x00700000UL,0,PAGE_READWRITE)==NULL);
    CHECK(vm_deallocate(mach_task_self(),blocker,4096)==KERN_SUCCESS);
    CHECK(mac_engine_physical_memory_allocation_count()==2);
    XPhysicalFree((void *)0x80600000UL);
    CHECK(XQueryMemoryProtect((void *)0x80600000UL)==PAGE_NOACCESS);
    CHECK(mac_engine_physical_memory_allocation_count()==1);
    if(a){ XPhysicalFree(a); CHECK(XQueryMemoryProtect(a)==PAGE_NOACCESS); }
    CHECK(mac_engine_physical_memory_allocation_count()==0);
    fixed=(unsigned char *)XPhysicalAlloc(4096,
        HALO_PORT_GAME_STATE_BASE_ADDRESS & 0x7fffffffUL,0,PAGE_READWRITE);
    CHECK(fixed==(unsigned char *)HALO_PORT_GAME_STATE_BASE_ADDRESS);
    XPhysicalFree(fixed);
    fixed=(unsigned char *)XPhysicalAlloc(4096,0x003a6000UL,0,PAGE_READWRITE);
    CHECK(fixed==(unsigned char *)0x803a6000UL);
    XPhysicalFree(fixed);
    CHECK(mac_engine_physical_memory_allocation_count()==0);
    mac_engine_physical_memory_stop();
    CHECK(XPhysicalAlloc(4096,0xffffffffUL,0,PAGE_READWRITE)==NULL);
    printf("physical-memory checks: %u, failures: %u\n",checks,failures);
    return failures?1:0;
}
