#include <mach/mach.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine_physical_memory.h"

/* This Mach boundary owns its bookkeeping outside the game's watched heap. */
#undef malloc
#undef free
#undef memset
#undef memcpy
#undef fprintf
#undef printf

#define PHYSICAL_BASE 0x80000000UL
#define PHYSICAL_SIZE 0x08000000UL
#define PHYSICAL_PAGE 4096UL
#define PHYSICAL_ANY 0xffffffffUL

struct physical_allocation {
    vm_address_t address;
    vm_size_t bytes;
    unsigned long *page_protect;
    unsigned long page_count;
    struct physical_allocation *next;
};

static pthread_mutex_t physical_lock = PTHREAD_MUTEX_INITIALIZER;
static struct physical_allocation *physical_allocations;
static unsigned long physical_count;
static int physical_active;

static int physical_protection(unsigned long value, vm_prot_t *mach,
    unsigned long *effective)
{
    unsigned long base = value & ~(PAGE_NOCACHE | PAGE_WRITECOMBINE);
    *mach = 0;
    if (base == PAGE_NOACCESS) *mach = VM_PROT_NONE;
    else if (base == PAGE_READONLY) *mach = VM_PROT_READ;
    else if (base == PAGE_READWRITE) *mach = VM_PROT_READ | VM_PROT_WRITE;
    else if (base == PAGE_EXECUTE) *mach = VM_PROT_EXECUTE;
    else if (base == PAGE_EXECUTE_READ) *mach = VM_PROT_READ | VM_PROT_EXECUTE;
    else if (base == PAGE_EXECUTE_READWRITE) *mach = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
    else return 0;
    if (value & ~(PAGE_NOACCESS | PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE |
            PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_NOCACHE | PAGE_WRITECOMBINE))
        return 0;
    *effective = base;
    return 1;
}

static int physical_range(unsigned long address, unsigned long bytes,
    struct physical_allocation **owner, unsigned long *first, unsigned long *length)
{
    struct physical_allocation *item;
    unsigned long end, rounded_end;
    if (!bytes || address > 0xffffffffUL - bytes) return 0;
    end = address + bytes;
    for (item = physical_allocations; item; item = item->next) {
        if (address >= item->address && end <= item->address + item->bytes) {
            *owner = item;
            *first = address & ~(PHYSICAL_PAGE - 1);
            rounded_end = (end + PHYSICAL_PAGE - 1) & ~(PHYSICAL_PAGE - 1);
            if (rounded_end < end || *first < item->address || rounded_end > item->address + item->bytes)
                return 0;
            *length = rounded_end - *first;
            return 1;
        }
    }
    return 0;
}

int mac_engine_physical_memory_start(void)
{
    int ok;
    pthread_mutex_lock(&physical_lock);
    ok = vm_page_size == PHYSICAL_PAGE && !physical_active && !physical_allocations;
    if (ok) physical_active = 1;
    pthread_mutex_unlock(&physical_lock);
    return ok;
}

void mac_engine_physical_memory_stop(void)
{
    pthread_mutex_lock(&physical_lock);
    if (physical_allocations) {
        fprintf(stderr, "physical memory stop refused with live allocations\n");
        pthread_mutex_unlock(&physical_lock);
        return;
    }
    physical_active = 0;
    pthread_mutex_unlock(&physical_lock);
}

unsigned long mac_engine_physical_memory_allocation_count(void)
{
    unsigned long count;
    pthread_mutex_lock(&physical_lock); count = physical_count; pthread_mutex_unlock(&physical_lock);
    return count;
}

int mac_engine_physical_memory_contains(const void *address, unsigned long bytes)
{
    struct physical_allocation *owner;
    unsigned long first, length;
    int contains;
    pthread_mutex_lock(&physical_lock);
    contains = physical_active && physical_range((unsigned long)address, bytes,
        &owner, &first, &length);
    pthread_mutex_unlock(&physical_lock);
    return contains;
}

int mac_engine_physical_memory_span(const void *address, unsigned long bytes,
    unsigned long *start, unsigned long *size)
{
    struct physical_allocation *owner;
    unsigned long first, length;
    int contains;
    pthread_mutex_lock(&physical_lock);
    contains = physical_active && physical_range((unsigned long)address, bytes,
        &owner, &first, &length);
    if (contains) { *start = owner->address; *size = owner->bytes; }
    pthread_mutex_unlock(&physical_lock);
    return contains;
}

void *__stdcall XPhysicalAlloc(unsigned long size, unsigned long physical,
    unsigned long alignment, unsigned long protect)
{
    struct physical_allocation *item;
    unsigned long rounded, step, candidate, limit, effective;
    vm_prot_t mach_protect;
    vm_address_t address;
    int result = 0;
    if (!size || vm_page_size != PHYSICAL_PAGE ||
        size > PHYSICAL_SIZE - PHYSICAL_PAGE ||
        !physical_protection(protect, &mach_protect, &effective)) return NULL;
    step = alignment ? alignment : PHYSICAL_PAGE;
    if ((step & (step - 1)) || step > PHYSICAL_SIZE) return NULL;
    if (step < PHYSICAL_PAGE) step = PHYSICAL_PAGE;
    rounded = (size + PHYSICAL_PAGE - 1) & ~(PHYSICAL_PAGE - 1);
    if (rounded < size || rounded > PHYSICAL_SIZE - PHYSICAL_PAGE) return NULL;
    if (physical != PHYSICAL_ANY) {
        if (physical > PHYSICAL_SIZE - rounded || (physical & (step - 1))) return NULL;
        candidate = PHYSICAL_BASE + physical;
        if ((candidate & (step - 1)) != 0) return NULL;
    } else candidate = PHYSICAL_BASE + step;
    item = malloc(sizeof(*item));
    if (!item) return NULL;
    pthread_mutex_lock(&physical_lock);
    if (!physical_active) goto done;
    limit = PHYSICAL_BASE + PHYSICAL_SIZE - rounded;
    for (; candidate <= limit; candidate += step) {
        address = candidate;
        if (vm_allocate(mach_task_self(), &address, rounded, FALSE) == KERN_SUCCESS) {
            if (address != candidate || vm_protect(mach_task_self(), address, rounded, FALSE, mach_protect) != KERN_SUCCESS) {
                vm_deallocate(mach_task_self(), address, rounded);
                break;
            }
            item->address = address; item->bytes = rounded;
            item->page_count = rounded / PHYSICAL_PAGE;
            item->page_protect = malloc(item->page_count * sizeof(*item->page_protect));
            if (!item->page_protect) { vm_deallocate(mach_task_self(), address, rounded); break; }
            { unsigned long page; for (page = 0; page < item->page_count; ++page) item->page_protect[page] = effective; }
            item->next = physical_allocations; physical_allocations = item;
            ++physical_count; result = 1; break;
        }
        if (physical != PHYSICAL_ANY) break;
        if (candidate > limit - step) break;
    }
done:
    pthread_mutex_unlock(&physical_lock);
    if (!result) { free(item); return NULL; }
    return (void *)item->address;
}

void __stdcall XPhysicalFree(void *address)
{
    struct physical_allocation **at, *item;
    if (!address) return;
    pthread_mutex_lock(&physical_lock);
    at = &physical_allocations;
    while (*at && (*at)->address != (vm_address_t)address) at = &(*at)->next;
    if (!*at) { fprintf(stderr, "XPhysicalFree refused unowned address %p\n", address); pthread_mutex_unlock(&physical_lock); return; }
    item = *at;
    if (vm_deallocate(mach_task_self(), item->address, item->bytes) != KERN_SUCCESS) {
        fprintf(stderr, "XPhysicalFree failed to deallocate %p\n", address);
        pthread_mutex_unlock(&physical_lock); return;
    }
    *at = item->next; --physical_count; free(item->page_protect); free(item);
    pthread_mutex_unlock(&physical_lock);
}

void __stdcall XPhysicalProtect(void *address, unsigned long size, unsigned long protect)
{
    struct physical_allocation *owner;
    unsigned long first, length, effective, page;
    vm_prot_t mach_protect;
    if (!address || !physical_protection(protect, &mach_protect, &effective)) return;
    pthread_mutex_lock(&physical_lock);
    if (physical_range((unsigned long)address, size, &owner, &first, &length) &&
        vm_protect(mach_task_self(), first, length, FALSE, mach_protect) == KERN_SUCCESS) {
        for (page = (first - owner->address) / PHYSICAL_PAGE;
            page < (first - owner->address + length) / PHYSICAL_PAGE; ++page)
            owner->page_protect[page] = effective;
    } else fprintf(stderr, "XPhysicalProtect refused invalid range %p + %lu\n", address, size);
    pthread_mutex_unlock(&physical_lock);
}

unsigned long __stdcall XQueryMemoryProtect(void *address)
{
    struct physical_allocation *item;
    unsigned long result = PAGE_NOACCESS, value = (unsigned long)address;
    pthread_mutex_lock(&physical_lock);
    for (item = physical_allocations; item; item = item->next)
        if (value >= item->address && value - item->address < item->bytes) {
            result = item->page_protect[(value - item->address) / PHYSICAL_PAGE]; break;
        }
    pthread_mutex_unlock(&physical_lock);
    return result;
}
