#include <mach/mach.h>
#include "native_resource_memory.h"
#include "engine_physical_memory.h"
#undef malloc
#undef free
#undef calloc
#undef realloc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
struct allocation {vm_address_t address;vm_size_t bytes,requested_bytes;struct allocation *next;};
struct owned_range {unsigned long address,bytes;struct owned_range *next;};
struct resource_certificate {void *owner_handle;unsigned long address,bytes;unsigned long long owner_token;struct resource_certificate *next;};
static struct allocation *allocations;
static struct owned_range *owned_ranges;
static struct resource_certificate *certificates;
static unsigned long count,bytes_used,generation;
static unsigned long long next_certificate_token=1;
static int certificate_tokens_exhausted;
/* Tiger/Leopard on the target Macs have 4096-byte pages. Avoid pretending the
   accounting is correct on another page size: allocation refuses that host. */
static unsigned long page_generation[PLATFORM_CONTIGUOUS_SIZE/4096];
/* The largest generation of each run of 64 pages: generations only grow, so
   it is the last one written there. A range's generation reads whole runs
   from it instead of every page. */
#define GENERATION_BLOCK_PAGES 64UL
static unsigned long block_generation[PLATFORM_CONTIGUOUS_SIZE/4096/GENERATION_BLOCK_PAGES+1];
/* Spans recently found allocated, valid while no allocation or owned range
   has been added or removed (span_serial counts those changes). Draws ask
   about the same few buffers many times a frame. */
#define SPAN_CACHE_SIZE 256
struct span_cache_entry {unsigned long address,bytes,serial;};
static struct span_cache_entry span_cache[SPAN_CACHE_SIZE];
static unsigned long span_serial=1;
/* (a span is kept in the slot of the page a lookup started in) */
#define SPAN_SLOT(address) (((address)>>12)%SPAN_CACHE_SIZE)
static void span_cache_remember(unsigned long start,unsigned long address,unsigned long bytes)
{
    struct span_cache_entry *entry=&span_cache[SPAN_SLOT(start)];
    entry->address=address;entry->bytes=bytes;entry->serial=span_serial;
}
static void spans_changed(void)
{
    if(++span_serial==0){memset(span_cache,0,sizeof(span_cache));span_serial=1;}
}
/* The owned ranges, allocations and certificates sorted by address (none
   overlaps another of its kind), rebuilt after a change: draws ask about
   them many times a frame, and the lists hold hundreds of models' buffers. */
struct sorted_span {unsigned long address,bytes;void *item;};
static struct sorted_span *sorted_ranges,*sorted_allocations,*sorted_certificates;
static unsigned long sorted_range_count,sorted_allocation_count,sorted_certificate_count;
static unsigned long sorted_span_serial,certificate_serial=1,sorted_certificate_serial;
static int compare_sorted_spans(const void *a,const void *b)
{
    unsigned long x=((const struct sorted_span *)a)->address,y=((const struct sorted_span *)b)->address;
    return x<y?-1:x>y?1:0;
}
static int sorted_spans_fill(struct sorted_span **out,unsigned long *count,unsigned long n)
{
    free(*out);*out=NULL;*count=0;
    if(!n)return 1;
    *out=(struct sorted_span *)malloc(n*sizeof(**out));
    return *out!=NULL;
}
static int sorted_spans_ready(void);
/* the entry whose address is the greatest not above address, or NULL */
static const struct sorted_span *sorted_spans_find(const struct sorted_span *spans,unsigned long count,unsigned long address)
{
    unsigned long low=0,high=count;
    while(low<high){unsigned long middle=low+(high-low)/2;if(spans[middle].address<=address)low=middle+1;else high=middle;}
    return low?&spans[low-1]:NULL;
}
unsigned long mac_resource_allocation_count(void){return count;}
unsigned long mac_resource_allocation_bytes(void){return bytes_used;}
BOOL platform_is_contiguous(const void *p)
{unsigned long a=(unsigned long)p;return a>=PLATFORM_CONTIGUOUS_BASE&&a-PLATFORM_CONTIGUOUS_BASE<PLATFORM_CONTIGUOUS_SIZE;}
static int sorted_spans_ready(void)
{
    unsigned long n;struct owned_range *range;struct allocation *item;struct resource_certificate *certificate;
    if(sorted_span_serial!=span_serial) {
        sorted_span_serial=0;
        for(n=0,range=owned_ranges;range;range=range->next)++n;
        if(!sorted_spans_fill(&sorted_ranges,&sorted_range_count,n))return 0;
        for(range=owned_ranges;range;range=range->next) {
            sorted_ranges[sorted_range_count].address=range->address;sorted_ranges[sorted_range_count].bytes=range->bytes;
            sorted_ranges[sorted_range_count++].item=range;
        }
        for(n=0,item=allocations;item;item=item->next)++n;
        if(!sorted_spans_fill(&sorted_allocations,&sorted_allocation_count,n))return 0;
        for(item=allocations;item;item=item->next) {
            sorted_allocations[sorted_allocation_count].address=item->address;
            sorted_allocations[sorted_allocation_count].bytes=item->requested_bytes;
            sorted_allocations[sorted_allocation_count++].item=item;
        }
        if(sorted_range_count)qsort(sorted_ranges,sorted_range_count,sizeof(*sorted_ranges),compare_sorted_spans);
        if(sorted_allocation_count)qsort(sorted_allocations,sorted_allocation_count,sizeof(*sorted_allocations),compare_sorted_spans);
        sorted_span_serial=span_serial;
    }
    if(sorted_certificate_serial!=certificate_serial) {
        sorted_certificate_serial=0;
        for(n=0,certificate=certificates;certificate;certificate=certificate->next)++n;
        if(!sorted_spans_fill(&sorted_certificates,&sorted_certificate_count,n))return 0;
        for(certificate=certificates;certificate;certificate=certificate->next) {
            sorted_certificates[sorted_certificate_count].address=certificate->address;
            sorted_certificates[sorted_certificate_count].bytes=certificate->bytes;
            sorted_certificates[sorted_certificate_count++].item=certificate;
        }
        if(sorted_certificate_count)qsort(sorted_certificates,sorted_certificate_count,sizeof(*sorted_certificates),compare_sorted_spans);
        sorted_certificate_serial=certificate_serial;
    }
    return 1;
}
/* port: the console's own allocations (XPhysicalAlloc: the tag cache an Xbox
   map's tags, buffers and bsps are read into, and the texture cache) are
   live resource memory too */
static BOOL physical_span_allocated(unsigned long start,unsigned long size)
{
    unsigned long base,bytes;
    if(!mac_engine_physical_memory_span((const void *)start,size,&base,&bytes))return FALSE;
    (void)base;(void)bytes;return TRUE;
}
BOOL mac_resource_range_is_allocated(const void *p,unsigned long size)
{
    unsigned long start=(unsigned long)p;struct allocation *item;struct owned_range *range;
    const struct span_cache_entry *entry;
    if(!p||!size||size>(unsigned long)-1-start)return FALSE;
    entry=&span_cache[SPAN_SLOT(start)];
    if(entry->serial==span_serial&&start>=entry->address&&start-entry->address<=entry->bytes&&
       size<=entry->bytes-(start-entry->address))return TRUE;
    if(sorted_spans_ready()) {
        const struct sorted_span *found=sorted_spans_find(sorted_ranges,sorted_range_count,start);
        if(found&&start-found->address<=found->bytes&&size<=found->bytes-(start-found->address)) {
            span_cache_remember(start,found->address,found->bytes);return TRUE;
        }
        if(!platform_is_contiguous(p))return FALSE;
        found=sorted_spans_find(sorted_allocations,sorted_allocation_count,start);
        if(found&&start-found->address<=found->bytes&&size<=found->bytes-(start-found->address)) {
            span_cache_remember(start,found->address,found->bytes);return TRUE;
        }
        return physical_span_allocated(start,size);
    }
    for(range=owned_ranges;range;range=range->next)
        if(start>=range->address&&start-range->address<=range->bytes&&size<=range->bytes-(start-range->address)) {
            span_cache_remember(start,range->address,range->bytes);return TRUE;
        }
    if(platform_is_contiguous(p)) {
        for(item=allocations;item;item=item->next)
            if(start>=item->address&&start-item->address<=item->requested_bytes&&size<=item->requested_bytes-(start-item->address)) {
                span_cache_remember(start,item->address,item->requested_bytes);return TRUE;
            }
        return physical_span_allocated(start,size);
    }
    return FALSE;
}
static struct allocation *find_exact_allocation(const void *p,unsigned long size)
{
    struct allocation *item;unsigned long address=(unsigned long)p;
    for(item=allocations;item;item=item->next)
        if(item->address==address&&item->requested_bytes==size)return item;
    return NULL;
}
static struct owned_range *find_exact_owned_range(const void *p,unsigned long size)
{
    struct owned_range *range;unsigned long address=(unsigned long)p;
    for(range=owned_ranges;range;range=range->next)
        if(range->address==address&&range->bytes==size)return range;
    return NULL;
}
BOOL mac_resource_certificate_register(void *owner_handle,const void *base,
    unsigned long size,unsigned long long *owner_token)
{
    struct allocation *allocation;struct resource_certificate *item;
    if(owner_token)*owner_token=0;
    if(!owner_handle||!base||!size||!owner_token||certificate_tokens_exhausted)return FALSE;
    if(next_certificate_token==ULLONG_MAX){certificate_tokens_exhausted=1;return FALSE;}
    allocation=find_exact_allocation(base,size);
    /* (or an owned range, such as a model's vertices, in the tracked span) */
    if(!allocation&&!find_exact_owned_range(base,size))return FALSE;
    for(item=certificates;item;item=item->next)
        if(item->owner_handle==owner_handle||item->address==(unsigned long)base)return FALSE;
    item=(struct resource_certificate *)malloc(sizeof(*item));if(!item)return FALSE;
    item->owner_handle=owner_handle;item->address=(unsigned long)base;item->bytes=size;
    item->owner_token=next_certificate_token++;
    if(next_certificate_token==0)certificate_tokens_exhausted=1;
    item->next=certificates;certificates=item;++certificate_serial;*owner_token=item->owner_token;return TRUE;
}
BOOL mac_resource_certificate_revoke(void *owner_handle)
{
    struct resource_certificate **at=&certificates,*item;
    if(!owner_handle)return FALSE;
    while(*at&&(*at)->owner_handle!=owner_handle)at=&(*at)->next;
    if(!*at)return FALSE;
    item=*at;*at=item->next;free(item);++certificate_serial;return TRUE;
}
BOOL mac_resource_certificate_query(const void *p,unsigned long size,
    unsigned long long *owner_token,unsigned long *range_generation,
    unsigned long *base_offset,unsigned long *remaining_bytes)
{
    struct resource_certificate *item;unsigned long address=(unsigned long)p,offset,observed;
    if(owner_token)*owner_token=0;
    if(range_generation)*range_generation=0;
    if(base_offset)*base_offset=0;
    if(remaining_bytes)*remaining_bytes=0;
    if(!p||!size||!owner_token||!range_generation||!base_offset||!remaining_bytes)return FALSE;
    if(sorted_spans_ready()) {
        /* (certificates do not overlap: only the nearest below can hold it) */
        const struct sorted_span *found=sorted_spans_find(sorted_certificates,sorted_certificate_count,address);
        item=found?(struct resource_certificate *)found->item:NULL;
        if(!item)return FALSE;
        offset=address-item->address;
        if(offset>item->bytes||size>item->bytes-offset)return FALSE;
        if(!mac_resource_range_is_allocated(p,size) ||
           !mac_resource_range_is_allocated((const void *)item->address,item->bytes))return FALSE;
        observed=mac_resource_range_generation((const void *)item->address,item->bytes);
        if(!observed)return FALSE;
        *owner_token=item->owner_token;*range_generation=observed;
        *base_offset=offset;*remaining_bytes=item->bytes-offset;return TRUE;
    }
    for(item=certificates;item;item=item->next) {
        if(address<item->address)continue;
        offset=address-item->address;
        if(offset>item->bytes||size>item->bytes-offset)continue;
        if(!mac_resource_range_is_allocated(p,size) ||
           !mac_resource_range_is_allocated((const void *)item->address,item->bytes))return FALSE;
        observed=mac_resource_range_generation((const void *)item->address,item->bytes);
        if(!observed)return FALSE;
        *owner_token=item->owner_token;*range_generation=observed;
        *base_offset=offset;*remaining_bytes=item->bytes-offset;return TRUE;
    }
    return FALSE;
}
BOOL mac_resource_register_owned_range(const void *p,unsigned long size)
{
    struct owned_range *range;struct allocation *allocation;unsigned long start=(unsigned long)p,end;
    if(!p||!size||size>(unsigned long)-1-start)return FALSE;
    end=start+size;
    for(range=owned_ranges;range;range=range->next)
        if(start<range->address+range->bytes&&range->address<end)return FALSE;
    for(allocation=allocations;allocation;allocation=allocation->next)
        if(start<allocation->address+allocation->requested_bytes&&allocation->address<end)return FALSE;
    range=(struct owned_range *)malloc(sizeof(*range));if(!range)return FALSE;
    range->address=start;range->bytes=size;range->next=owned_ranges;owned_ranges=range;
    spans_changed();
    /* Its contents were just written: give its pages a write generation. */
    memory_watch_prepare_write((void *)start,size);
    return TRUE;
}
void mac_resource_unregister_owned_range(const void *p)
{
    struct owned_range **at=&owned_ranges,*range;unsigned long start=(unsigned long)p;
    while(*at&&(*at)->address!=start)at=&(*at)->next;
    if(!*at){fprintf(stderr,"native resource range unregister does not own %p\n",p);abort();}
    range=*at;*at=range->next;
    spans_changed();
    {
        /* No certificate outlives its range, and a reused address is a
           new generation. */
        struct resource_certificate **certificate_at=&certificates,*certificate;
        while(*certificate_at) {
            certificate=*certificate_at;
            if(certificate->address>=range->address&&
               certificate->address-range->address<=range->bytes&&
               certificate->bytes<=range->bytes-(certificate->address-range->address)) {
                *certificate_at=certificate->next;free(certificate);++certificate_serial;
            } else certificate_at=&certificate->next;
        }
    }
    memory_watch_prepare_write((void *)range->address,range->bytes);
    free(range);
}
void *platform_contiguous_alloc(unsigned long size,unsigned long alignment,unsigned long physical,DWORD protect)
{
    struct allocation *item;vm_size_t bytes;vm_address_t address;unsigned long step;
    if(vm_page_size!=4096||!size||size>PLATFORM_CONTIGUOUS_SIZE-4096||!alignment||(alignment&(alignment-1))||alignment>PLATFORM_CONTIGUOUS_SIZE||protect!=PAGE_READWRITE)return NULL;
    bytes=(size+4095UL)&~4095UL;step=alignment>4096?alignment:4096;
    if(physical!=PLATFORM_ANY_PHYSICAL_ADDRESS && (!physical||(physical&(step-1))||physical>PLATFORM_CONTIGUOUS_SIZE-bytes))return NULL;
    item=malloc(sizeof(*item));if(!item)return NULL;
    if(physical!=PLATFORM_ANY_PHYSICAL_ADDRESS) {
        vm_address_t candidate=PLATFORM_CONTIGUOUS_BASE+physical;
        address=vm_allocate(mach_task_self(),&candidate,bytes,FALSE)==KERN_SUCCESS?candidate:0;
    } else {
        unsigned long found=0;
        address=mac_resource_vm_allocate_next(PLATFORM_CONTIGUOUS_BASE+step,
            PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE-bytes,step,bytes,&found)?(vm_address_t)found:0;
    }
    if(address) {
        item->address=address;item->bytes=bytes;item->requested_bytes=size;item->next=allocations;
        allocations=item;spans_changed();++count;bytes_used+=bytes;
        memory_watch_prepare_write((void*)address,bytes);return (void*)address;
    }
    free(item);return NULL;
}
/* Asking the kernel about every address from the base, as the allocators
   did, made thousands of mach calls per allocation while loading. */
int mac_resource_vm_allocate_next(unsigned long first,unsigned long last,
    unsigned long step,unsigned long size,unsigned long *out)
{
    static unsigned long next_hint;
    unsigned long start,address,end;int pass;
    if(!out||!step||!size||last<first)return 0;
    *out=0;
    start=next_hint>first?first+((next_hint-first+step-1)/step)*step:first;
    if(start>last||start<first)start=first;
    for(pass=0;pass<2;pass++) {
        if(pass&&start==first)break;
        address=pass?first:start;end=pass?start:last;
        for(;address<=end&&address<=last;address+=step) {
            vm_address_t candidate=(vm_address_t)address;
            if(pass&&address==start)break;
            if(vm_allocate(mach_task_self(),&candidate,(vm_size_t)size,FALSE)==KERN_SUCCESS) {
                *out=(unsigned long)candidate;next_hint=(unsigned long)candidate+size;return 1;
            }
            if(address>last-step)break;
        }
    }
    return 0;
}
void platform_contiguous_free(void *p)
{
    struct allocation **at=&allocations,*item;struct resource_certificate **certificate_at,*certificate;if(!p)return;
    while(*at&&(*at)->address!=(vm_address_t)p)at=&(*at)->next;
    if(!*at){fprintf(stderr,"native resource free does not own %p\n",p);abort();}
    item=*at;
    if(vm_deallocate(mach_task_self(),item->address,item->bytes)!=KERN_SUCCESS){fprintf(stderr,"native resource deallocation failed\n");abort();}
    certificate_at=&certificates;
    while(*certificate_at) {
        certificate=*certificate_at;
        if(certificate->address>=item->address&&
           certificate->address-item->address<=item->requested_bytes&&
           certificate->bytes<=item->requested_bytes-(certificate->address-item->address)) {
            *certificate_at=certificate->next;free(certificate);++certificate_serial;
        } else certificate_at=&certificate->next;
    }
    memory_watch_prepare_write(p,item->bytes);*at=item->next;spans_changed();--count;bytes_used-=item->bytes;free(item);
}
void memory_watch_prepare_write(void *p,unsigned long size)
{
    unsigned long start=(unsigned long)p,end,first,last,i;
    if(start<PLATFORM_CONTIGUOUS_BASE||start>=PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE)return;
    if(!size) {
        struct allocation *item;
        /* A zero-size vertex lock exposes the buffer without an explicit
           length. Conservatively track its exact owned remainder, never VM
           page padding or an unowned pointer. Read-only locks may write too. */
        struct owned_range *range;
        for(item=allocations;item;item=item->next)
            if(start>=item->address&&start-item->address<item->requested_bytes) {
                size=item->requested_bytes-(start-item->address);break;
            }
        for(range=owned_ranges;!size&&range;range=range->next)
            if(start>=range->address&&start-range->address<range->bytes)
                size=range->bytes-(start-range->address);
    }
    if(!size||size>PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE-start)return;
    end=start+size-1;first=(start-PLATFORM_CONTIGUOUS_BASE)/4096;last=(end-PLATFORM_CONTIGUOUS_BASE)/4096;
    if(generation==0xffffffffUL){fprintf(stderr,"native resource write generation exhausted\n");abort();}++generation;
    for(i=first;i<=last;++i)page_generation[i]=generation;
    for(i=first/GENERATION_BLOCK_PAGES;i<=last/GENERATION_BLOCK_PAGES;++i)block_generation[i]=generation;
}
unsigned long mac_resource_write_serial(void){return generation;}
unsigned long mac_resource_range_generation(const void *p,unsigned long size)
{
    unsigned long start=(unsigned long)p,first,last,i,result=0;
    if(!size||start<PLATFORM_CONTIGUOUS_BASE||start>=PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE||size>PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE-start)return 0;
    first=(start-PLATFORM_CONTIGUOUS_BASE)/4096;last=(start+size-1-PLATFORM_CONTIGUOUS_BASE)/4096;
    for(i=first;i<=last;) {
        if(!(i%GENERATION_BLOCK_PAGES)&&last-i+1>=GENERATION_BLOCK_PAGES) {
            if(block_generation[i/GENERATION_BLOCK_PAGES]>result)result=block_generation[i/GENERATION_BLOCK_PAGES];
            i+=GENERATION_BLOCK_PAGES;
        } else {
            if(page_generation[i]>result)result=page_generation[i];
            ++i;
        }
    }
    return result;
}

/* Bytes mapped inside the contiguous window by anyone (the Xbox resources,
   textures, model buffers, and whatever else the process placed there). */
unsigned long mac_resource_window_mapped_bytes(void)
{
    vm_address_t address=PLATFORM_CONTIGUOUS_BASE;
    unsigned long total=0;
    while(address<PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE) {
        vm_size_t size=0;vm_region_basic_info_data_t info;
        mach_msg_type_number_t infoCount=VM_REGION_BASIC_INFO_COUNT;
        mach_port_t object;
        vm_address_t start=address;
        if(vm_region(mach_task_self(),&start,&size,VM_REGION_BASIC_INFO,(vm_region_info_t)&info,
            &infoCount,&object)!=KERN_SUCCESS || !size)break;
        if(start>=PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE)break;
        {
            vm_address_t end=start+size;
            if(start<PLATFORM_CONTIGUOUS_BASE)start=PLATFORM_CONTIGUOUS_BASE;
            if(end>PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE)end=PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE;
            total+=end-start;address=end;
        }
    }
    return total;
}
