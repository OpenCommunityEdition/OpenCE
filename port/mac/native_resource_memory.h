#ifndef HALO_MAC_NATIVE_RESOURCE_MEMORY_H
#define HALO_MAC_NATIVE_RESOURCE_MEMORY_H
#include "xtl.h"
#define PLATFORM_CONTIGUOUS_BASE 0x80000000UL
/* 256 MB (0x80000000..0x90000000, below the system's shared region): campaign
   levels hold more models and textures than the multiplayer maps' 128 MB. */
#define PLATFORM_CONTIGUOUS_SIZE 0x10000000UL
#define PLATFORM_ANY_PHYSICAL_ADDRESS 0xffffffffUL
#define PLATFORM_PHYSICAL_TO_VIRTUAL(p) ((void *)((unsigned long)(p)|PLATFORM_CONTIGUOUS_BASE))
#define PLATFORM_VIRTUAL_TO_PHYSICAL(p) ((unsigned long)(p)&~PLATFORM_CONTIGUOUS_BASE)
void *platform_contiguous_alloc(unsigned long,unsigned long,unsigned long,DWORD);
/* Allocate size bytes at a free fixed address first + k*step <= last,
   searching from just past the previous such allocation and wrapping once.
   Returns 1 with the address in *out. */
int mac_resource_vm_allocate_next(unsigned long first,unsigned long last,
    unsigned long step,unsigned long size,unsigned long *out);
void platform_contiguous_free(void *);
BOOL platform_is_contiguous(const void *);
void memory_watch_prepare_write(void *,unsigned long);
unsigned long mac_resource_write_serial(void);
unsigned long mac_resource_range_generation(const void *,unsigned long);
unsigned long mac_resource_allocation_count(void);
unsigned long mac_resource_allocation_bytes(void);
/* Bytes mapped in the contiguous window, by anything. */
unsigned long mac_resource_window_mapped_bytes(void);
/* True only when the complete byte range belongs to one live native
   contiguous allocation tracked by this owner. */
BOOL mac_resource_range_is_allocated(const void *,unsigned long);
/* Scratch prototype for stable decode-cache identities. Only an exact live
   contiguous allocation or owned range may be certified; owner_handle is its native resource
   object and must be revoked before that object is released. */
BOOL mac_resource_certificate_register(void *owner_handle,const void *base,
    unsigned long bytes,unsigned long long *owner_token);
BOOL mac_resource_certificate_revoke(void *owner_handle);
BOOL mac_resource_certificate_query(const void *address,unsigned long bytes,
    unsigned long long *owner_token,unsigned long *generation,
    unsigned long *base_offset,unsigned long *remaining_bytes);
BOOL mac_resource_register_owned_range(const void *,unsigned long);
void mac_resource_unregister_owned_range(const void *);
void platform_log(const char *,...);
/* Engine-thread resource services. Write tracking records explicit lock/write
   notifications only, not unannounced CPU stores or protection faults. A zero
   write length conservatively covers the remaining requested bytes of a live
   contiguous allocation; unowned pointers and page padding are ignored. */
#endif
