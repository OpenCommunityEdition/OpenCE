/* Native resident texture boundary. Loading/decoding has completed before
   binding. This returns real owned CPU texture resources, never a dummy GPU
   handle. No rendering entry point is supplied by this module. */
#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "native_bitmap_tags.h"
#include "native_resource_memory.h"
#include "engine_cache_files.h"
void *xbox_texture_cache_bitmap_get_hardware_format(struct bitmap_data *bitmap,boolean block,boolean load);
#include <stdlib.h>

/* port/linux/game/menu_tags.c */
boolean pc_menu_tag(long tag_index);

/* Mirror the source-private bitmap enums in bitmaps.c and
   xbox_texture_cache.c. The non-cached direct-resource path below currently
   admits the exact dynamic A4R4G4B4 2D atlas used by rasterizer_text.c. */
enum {
    native_bitmap_type_2d = 0,
    native_bitmap_format_a4r4g4b4 = 9,
    native_bitmap_allocated_bit = 6,
    native_bitmap_cached_bit = 7
};

static int allocated_bitmap_hardware_format(struct bitmap_data *bitmap,
    void **hardware_out)
{
    D3DBaseTexture *texture;
    unsigned long width,height,levels,bytes,level;
    unsigned long format_word,format,width_log,height_log;
    if(hardware_out)*hardware_out=NULL;
    if(!bitmap||!hardware_out||
       (bitmap->flags&(1U<<native_bitmap_cached_bit))||
       !(bitmap->flags&(1U<<native_bitmap_allocated_bit))||
       bitmap->type!=native_bitmap_type_2d||
       bitmap->format!=native_bitmap_format_a4r4g4b4||
       bitmap->depth!=1||bitmap->width<=0||bitmap->height<=0||
       !bitmap->hardware_format)
        return 0;

    texture=(D3DBaseTexture *)bitmap->hardware_format;
    if((texture->Common&(D3DCOMMON_TYPE_MASK|D3DCOMMON_D3DCREATED))!=
       (D3DCOMMON_TYPE_TEXTURE|D3DCOMMON_D3DCREATED)||!texture->Data)
        return 0;
    format_word=texture->Format;
    format=(format_word&D3DFORMAT_FORMAT_MASK)>>D3DFORMAT_FORMAT_SHIFT;
    width_log=(format_word&D3DFORMAT_USIZE_MASK)>>D3DFORMAT_USIZE_SHIFT;
    height_log=(format_word&D3DFORMAT_VSIZE_MASK)>>D3DFORMAT_VSIZE_SHIFT;
    width=1UL<<width_log;height=1UL<<height_log;
    levels=(format_word&D3DFORMAT_MIPMAP_MASK)>>D3DFORMAT_MIPMAP_SHIFT;
    if(format!=D3DFMT_A4R4G4B4||
       ((format_word&D3DFORMAT_DIMENSION_MASK)>>D3DFORMAT_DIMENSION_SHIFT)!=2||
       (format_word&D3DFORMAT_CUBEMAP)||texture->Size!=0||
       width_log>12||height_log>12||width!=(unsigned long)bitmap->width||
       height!=(unsigned long)bitmap->height||!levels||levels>13||
       levels!=(unsigned long)bitmap->mipmap_count+1)
        return 0;

    /* D3DFMT_A4R4G4B4 is the source's swizzled, two-byte texel format. Its
       allocation is the exact sum of the declared 2D mip texels. Bound every
       multiplication and sum before asking the live allocator registry. */
    bytes=0;
    for(level=0;level<levels;++level) {
        unsigned long mip_width=width>>level,mip_height=height>>level;
        unsigned long mip_bytes;
        if(!mip_width)mip_width=1;
        if(!mip_height)mip_height=1;
        if(mip_width>(unsigned long)-1/mip_height/2)return 0;
        mip_bytes=mip_width*mip_height*2;
        if(bytes>(unsigned long)-1-mip_bytes)return 0;
        bytes+=mip_bytes;
    }
    if(!bytes||!mac_resource_range_is_allocated(
       PLATFORM_PHYSICAL_TO_VIRTUAL(texture->Data),bytes))
        return 0;
    *hardware_out=texture;
    return 1;
}
/* A cache contains at most 65536 salted tag slots. The full map exceeds
   the original focused probe's 64 bitmap owners. */
struct native_bitmap_cache_owner {
    void *owner;
    struct bitmap_group *root;
};
static struct native_bitmap_cache_owner *owners;
static size_t owner_count;
/* the owners' bitmap arrays by address, to find a bitmap's owner by binary
   search (the game asks once per texture set) */
struct native_bitmap_cache_span {unsigned long start,end;size_t owner;};
static struct native_bitmap_cache_span *owner_spans;
static size_t owner_span_count;
/* every bound bitmap's hardware pointer, sorted, for contains(): most of its
   callers ask about a texture the game made itself, which is in none */
static unsigned long *hardware_index;
static size_t hardware_index_count;
static int compare_hardware(const void *a,const void *b)
{
    unsigned long x=*(const unsigned long *)a,y=*(const unsigned long *)b;
    return x<y?-1:x>y?1:0;
}
static int compare_owner_spans(const void *a,const void *b)
{
    const struct native_bitmap_cache_span *x=(const struct native_bitmap_cache_span *)a;
    const struct native_bitmap_cache_span *y=(const struct native_bitmap_cache_span *)b;
    return x->start<y->start?-1:x->start>y->start?1:0;
}
static void build_owner_spans(void)
{
    size_t i;
    free(owner_spans);owner_spans=NULL;owner_span_count=0;
    if(!owner_count)return;
    owner_spans=(struct native_bitmap_cache_span *)malloc(owner_count*sizeof(*owner_spans));
    if(!owner_spans)return; /* (the linear search remains) */
    for(i=0;i<owner_count;++i) {
        const struct bitmap_group *root=owners[i].root;
        if(!root||root->bitmaps.count<=0||!root->bitmaps.address)continue;
        owner_spans[owner_span_count].start=(unsigned long)root->bitmaps.address;
        owner_spans[owner_span_count].end=owner_spans[owner_span_count].start+
            (unsigned long)root->bitmaps.count*sizeof(struct bitmap_data);
        owner_spans[owner_span_count].owner=i;
        ++owner_span_count;
    }
    qsort(owner_spans,owner_span_count,sizeof(*owner_spans),compare_owner_spans);
    free(hardware_index);hardware_index=NULL;hardware_index_count=0;
    {
        size_t total=0,j;
        for(i=0;i<owner_count;++i)
            if(owners[i].owner&&owners[i].root&&owners[i].root->bitmaps.count>=1&&
               owners[i].root->bitmaps.count<=2048&&owners[i].root->bitmaps.address)
                total+=(size_t)owners[i].root->bitmaps.count;
        if(!total)return;
        hardware_index=(unsigned long *)malloc(total*sizeof(*hardware_index));
        if(!hardware_index)return; /* (the linear search remains) */
        for(i=0;i<owner_count;++i) {
            struct bitmap_data *bitmaps;
            if(!owners[i].owner||!owners[i].root||owners[i].root->bitmaps.count<1||
               owners[i].root->bitmaps.count>2048||!owners[i].root->bitmaps.address)continue;
            bitmaps=(struct bitmap_data *)owners[i].root->bitmaps.address;
            for(j=0;j<(size_t)owners[i].root->bitmaps.count;++j)
                hardware_index[hardware_index_count++]=(unsigned long)bitmaps[j].hardware_format;
        }
        qsort(hardware_index,hardware_index_count,sizeof(*hardware_index),compare_hardware);
    }
}
static unsigned long successful_requests;
static unsigned long owner_generation;
unsigned long mac_native_bitmap_cache_generation(void){return owner_generation;}
unsigned long mac_native_bitmap_cache_requests(void){return successful_requests;}
const unsigned char *mac_native_bitmap_cache_rgba(const void *hardware,uint32_t mip,
    uint32_t *width,uint32_t *height,size_t *bytes)
{
    size_t i;const unsigned char *pixels;
    for(i=0;i<owner_count;++i){pixels=mac_native_bitmap_rgba_for_hardware(owners[i].owner,hardware,mip,NULL,width,height,bytes);if(pixels)return pixels;}
    return NULL;
}
const unsigned char *mac_native_bitmap_cache_rgba_face(const void *hardware,
    uint32_t face,uint32_t mip,uint32_t *width,uint32_t *height,size_t *bytes)
{
    size_t i;unsigned j;struct bitmap_data *bitmaps;
    const unsigned char *pixels;
    if(!hardware||face>=6)return NULL;
    for(i=0;i<owner_count;++i) {
        if(!owners[i].owner||!owners[i].root)continue;
        /* The converted tag's bitmap_data rows carry the exact D3D hardware
           identity assigned by the owner; do not infer identity from format. */
        if(owners[i].root->bitmaps.count<1||owners[i].root->bitmaps.count>2048||
           !owners[i].root->bitmaps.address)continue;
        bitmaps=(struct bitmap_data *)owners[i].root->bitmaps.address;
        for(j=0;j<(unsigned)owners[i].root->bitmaps.count;++j) {
            if(bitmaps[j].hardware_format!=hardware)continue;
            pixels=mac_native_bitmap_rgba_face(owners[i].owner,j,face,mip,width,height,bytes);
            if(pixels)return pixels;
        }
    }
    return NULL;
}
const unsigned char *mac_native_bitmap_cache_encoded_face(const void *hardware,
    uint32_t face,uint32_t mip,uint32_t *format,uint32_t *width,
    uint32_t *height,size_t *bytes)
{
    size_t i;unsigned j;struct bitmap_data *bitmaps;
    const unsigned char *encoded;
    if(format)*format=0;if(width)*width=0;if(height)*height=0;if(bytes)*bytes=0;
    if(!format||!width||!height||!bytes||!hardware||face>=6)return NULL;
    for(i=0;i<owner_count;++i) {
        if(!owners[i].owner||!owners[i].root)continue;
        if(owners[i].root->bitmaps.count<1||owners[i].root->bitmaps.count>2048||
           !owners[i].root->bitmaps.address)continue;
        bitmaps=(struct bitmap_data *)owners[i].root->bitmaps.address;
        for(j=0;j<(unsigned)owners[i].root->bitmaps.count;++j) {
            if(bitmaps[j].hardware_format!=hardware)continue;
            encoded=mac_native_bitmap_encoded_face(owners[i].owner,j,face,mip,
                format,width,height,bytes);
            return encoded;
        }
    }
    return NULL;
}
int mac_native_bitmap_cache_identify(const void *hardware,const char **name,
    uint32_t *index,uint32_t *width,uint32_t *height,int *cube)
{
    size_t i;unsigned j;struct bitmap_data *bitmaps;
    if(!hardware||!name||!index||!width||!height||!cube)return 0;
    for(i=0;i<owner_count;++i) {
        if(!owners[i].owner||!owners[i].root)continue;
        if(owners[i].root->bitmaps.count<1||owners[i].root->bitmaps.count>2048||
           !owners[i].root->bitmaps.address)continue;
        bitmaps=(struct bitmap_data *)owners[i].root->bitmaps.address;
        for(j=0;j<(unsigned)owners[i].root->bitmaps.count;++j) {
            if(bitmaps[j].hardware_format!=hardware)continue;
            if(bitmaps[j].type!=native_bitmap_type_2d&&bitmaps[j].type!=2)return 0;
            if(bitmaps[j].width<1||bitmaps[j].height<1)return 0;
            *name=mac_native_bitmap_owner_name(owners[i].owner);
            *index=j;*width=(uint32_t)bitmaps[j].width;*height=(uint32_t)bitmaps[j].height;
            *cube=bitmaps[j].type==2;
            return (*name)[0]!=0;
        }
    }
    return 0;
}
const unsigned char *mac_native_bitmap_cache_rgba_volume(const void *hardware,
    uint32_t mip,uint32_t *width,uint32_t *height,uint32_t *depth,size_t *bytes)
{
    size_t i;
    const unsigned char *pixels;
    if(!hardware)return NULL;
    for(i=0;i<owner_count;++i) {
        pixels=mac_native_bitmap_rgba_volume_for_hardware(owners[i].owner,
            hardware,mip,width,height,depth,bytes);
        if(pixels)return pixels;
    }
    return NULL;
}
int mac_native_bitmap_cache_is_bound(void){return owner_count!=0;}
int mac_native_bitmap_cache_contains(const void *hardware)
{
    size_t i;unsigned j;struct bitmap_data *bitmaps;
    if(!hardware)return 0;
    if(hardware_index) {
        size_t low=0,high=hardware_index_count;unsigned long key=(unsigned long)hardware;
        while(low<high){size_t middle=low+(high-low)/2;if(hardware_index[middle]<key)low=middle+1;else high=middle;}
        return low<hardware_index_count&&hardware_index[low]==key;
    }
    for(i=0;i<owner_count;++i) {
        if(!owners[i].owner||!owners[i].root||owners[i].root->bitmaps.count<1||
           owners[i].root->bitmaps.count>2048||!owners[i].root->bitmaps.address)continue;
        bitmaps=(struct bitmap_data *)owners[i].root->bitmaps.address;
        for(j=0;j<(unsigned)owners[i].root->bitmaps.count;++j)
            if(bitmaps[j].hardware_format==hardware)return 1;
    }
    return 0;
}
int mac_native_bitmap_cache_bind(struct mac_native_bitmap_tag *tags,size_t count)
{
    size_t i;struct native_bitmap_cache_owner *next;
    if(owner_count||owners||!tags||!count||count>65536)return 0;
    for(i=0;i<count;++i)if(!tags[i].owner||!tags[i].root)return 0;
    next=(struct native_bitmap_cache_owner *)malloc(count*sizeof(*next));if(!next)return 0;
    for(i=0;i<count;++i){next[i].owner=tags[i].owner;next[i].root=tags[i].root;}
    owners=next;owner_count=count;successful_requests=0;
    build_owner_spans();
    ++owner_generation;return 1;
}
void mac_native_bitmap_cache_unbind(void)
{free(owners);owners=NULL;owner_count=0;free(owner_spans);owner_spans=NULL;owner_span_count=0;
 free(hardware_index);hardware_index=NULL;hardware_index_count=0;++owner_generation;}
void *_texture_cache_bitmap_get_hardware_format(struct bitmap_data *bitmap,boolean block,boolean load)
{
    size_t i;void *resource;
    if(mac_xbox_mode())return xbox_texture_cache_bitmap_get_hardware_format(bitmap,block,load);
    /* the owner of the last bitmap found, asked first */
    static size_t last_owner;static unsigned long last_generation;
    /* bitmaps no owner held when every owner was asked (the interface's
       own), by address: asked of the owners again only when they change */
    static struct { const struct bitmap_data *bitmap;unsigned long generation; } unowned[64];
    size_t unowned_slot=((unsigned long)bitmap>>4)%64;
    if(!load&&block){fprintf(stderr,"native texture invalid blocking request\n");abort();}
    /* the menus' bitmaps (port/linux/game/menu_tags.c): textures the game made
       for them, whose pictures are the PNGs (engine_menu_art.c) */
    if(bitmap&&bitmap->hardware_format&&bitmap->tag_index!=NONE&&pc_menu_tag(bitmap->tag_index))
        return bitmap->hardware_format;
    if(unowned[unowned_slot].bitmap==bitmap&&unowned[unowned_slot].generation==owner_generation&&
       allocated_bitmap_hardware_format(bitmap,&resource)) {
        ++successful_requests;
        return resource;
    }
    if(last_generation==owner_generation&&last_owner<owner_count) {
        resource=mac_native_bitmap_hardware(owners[last_owner].owner,bitmap);
        if(resource){++successful_requests;return resource;}
    }
    if(owner_spans) {
        size_t low=0,high=owner_span_count;unsigned long address=(unsigned long)bitmap;
        while(low<high) {
            size_t middle=low+(high-low)/2;
            if(owner_spans[middle].end<=address)low=middle+1;
            else high=middle;
        }
        if(low<owner_span_count&&address>=owner_spans[low].start) {
            i=owner_spans[low].owner;
            resource=mac_native_bitmap_hardware(owners[i].owner,bitmap);
            if(resource){last_owner=i;last_generation=owner_generation;++successful_requests;return resource;}
        }
    }
    for(i=0;i<owner_count;++i){
        resource=mac_native_bitmap_hardware(owners[i].owner,bitmap);
        if(resource){last_owner=i;last_generation=owner_generation;++successful_requests;return resource;}
    }
    if(allocated_bitmap_hardware_format(bitmap,&resource)) {
        unowned[unowned_slot].bitmap=bitmap;unowned[unowned_slot].generation=owner_generation;
        ++successful_requests;
        return resource;
    }
    fprintf(stderr,"native texture requested outside resident cache\n");abort();return NULL;
}
