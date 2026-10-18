#include <mach/mach.h>
#include "cseries.h"
#include "native_resource_memory.h"
#include "bitmaps/bitmap_group.h"
#include "native_bitmap_tags.h"
#include "native_tags.h"
#include "retail_texture.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#include <stdlib.h>
#include <string.h>
#define BITMAP_LIMIT (32U*1024U*1024U)
struct bitmap_level { unsigned char *rgba;uint32_t width,height,depth,format;size_t bytes,encoded_offset,encoded_bytes; };
struct bitmap_image { D3DTexture texture;unsigned char *encoded,*sample_pixels;vm_size_t encoded_bytes;size_t encoded_payload_bytes,sample_bytes;struct bitmap_level levels[6][12];uint32_t count,face_count,volume_depth,is_volume; };
struct bitmap_owner { struct bitmap_group root;struct bitmap_image *images;size_t bytes;struct mac_native_tags registry;uint32_t handle;int accounting_bound;
    /* the tag's path, for texture packs (empty if it did not fit or was out of range) */
    char name[128]; };
typedef char bitmap_native_root_size[sizeof(struct bitmap_group)==108?1:-1];
typedef char bitmap_native_image_size[sizeof(struct bitmap_data)==48?1:-1];
static uint32_t r32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned r16(const unsigned char*p){return p[0]|(unsigned)p[1]<<8;}
static void short_at(void *out,const unsigned char *p){uint16_t v=(uint16_t)r16(p);memcpy(out,&v,2);}
static int real_at(void *out,const unsigned char*p){uint32_t v=r32(p);if((v&0x7f800000U)==0x7f800000U)return 0;memcpy(out,&v,4);return 1;}
static void *allocate(struct bitmap_owner *o,size_t n){void*p;if(!n)return NULL;if(n>BITMAP_LIMIT-o->bytes)return NULL;p=calloc(1,n);if(p)o->bytes+=n;return p;}
static uint32_t level_bytes(uint32_t format,uint32_t w,uint32_t h)
{return (format==6||format==8||format==9)?w*h*2:(format==10||format==11)?w*h*4:format==17?w*h:((w+3)/4)*((h+3)/4)*(format==14?8:16);}
#if defined(__ppc__)
static void swap_pair(unsigned char *p){unsigned char v=p[0];p[0]=p[1];p[1]=v;}
static void native_dxt_color(unsigned char *p)
{unsigned char v;swap_pair(p);swap_pair(p+2);v=p[4];p[4]=p[7];p[7]=v;v=p[5];p[5]=p[6];p[6]=v;}
static void native_sample_pixels(struct bitmap_owner *o,struct bitmap_image *image,
    const struct bitmap_data *bitmap,uint32_t total)
{
    size_t offset=0;uint32_t w=bitmap->width,h=bitmap->height,d=bitmap->depth,m,i,bytes,block_size,a,slices;
    image->sample_pixels=allocate(o,total);if(!image->sample_pixels)return;
    memcpy(image->sample_pixels,image->encoded,total);
    for(m=0;m<image->count;++m) {
        slices=bitmap->type==2?6U:bitmap->type==1?d:1U;
        bytes=level_bytes(bitmap->format,w,h)*slices;
        if(bitmap->format==17){}
        else if(bitmap->format==6||bitmap->format==8||bitmap->format==9)for(i=0;i<bytes;i+=2)swap_pair(image->sample_pixels+offset+i);
        else if(bitmap->format==10 || bitmap->format==11) {
            for(i=0;i<bytes;i+=4) {
                uint32_t pixel=r32(image->sample_pixels+offset+i);
                memcpy(image->sample_pixels+offset+i,&pixel,4);
            }
        } else {
            block_size=bitmap->format==14?8:16;
            for(i=0;i<bytes;i+=block_size) {
                unsigned char *block=image->sample_pixels+offset+i;
                if(bitmap->format==15)for(a=0;a<8;a+=2)swap_pair(block+a);
                native_dxt_color(block+(bitmap->format==14?0:8));
            }
        }
        offset+=bytes;w=w>1?w/2:1;h=h>1?h/2:1;d=d>1?d/2:1;
    }
}
#endif
void mac_native_bitmap_owner_free(void *v)
{
    struct bitmap_owner *o=v;unsigned i,j;struct bitmap_group_sequence *seq;
    if(!o)return;seq=o->root.sequences.address;
    if(seq)for(i=0;i<(unsigned)o->root.sequences.count;++i)free(seq[i].sprites.address);
    if(o->images)for(i=0;i<(unsigned)o->root.bitmaps.count;++i){unsigned f;if(o->images[i].encoded)vm_deallocate(mach_task_self(),(vm_address_t)o->images[i].encoded,o->images[i].encoded_bytes);if(o->images[i].sample_pixels&&o->images[i].sample_pixels!=o->images[i].encoded)free(o->images[i].sample_pixels);for(f=0;f<6;++f)for(j=0;j<12;++j)free(o->images[i].levels[f][j].rgba);}
    free(seq);free(o->root.bitmaps.address);free(o->images);free(o);
}
const char *mac_native_bitmap_owner_name(const void *owner){return owner?((const struct bitmap_owner*)owner)->name:"";}
void mac_native_bitmap_tag_free(struct mac_native_bitmap_tag *t){if(t){mac_native_bitmap_owner_free(t->owner);memset(t,0,sizeof(*t));}}
int mac_native_bitmap_owner_accounting_bind(void *owner,const struct mac_native_tags *tags,uint32_t handle)
{struct bitmap_owner*o=owner;if(!o||!tags||!tags->state||o->accounting_bound||handle==UINT32_MAX)return 0;o->registry=*tags;o->handle=handle;o->accounting_bound=1;return 1;}
int mac_native_bitmap_memory_usage_get(void *owner,struct mac_native_bitmap_memory_usage *out)
{
    struct bitmap_owner *o=owner;size_t encoded=0,payload=0,sample=0,rgba=0;
    unsigned i,f,m;
    if(!o||!out||!o->images||o->root.bitmaps.count<1||o->root.bitmaps.count>2048)return 0;
    for(i=0;i<(unsigned)o->root.bitmaps.count;++i) {
        const struct bitmap_image *image=&o->images[i];
        encoded+=image->encoded_bytes;payload+=image->encoded_payload_bytes;
        sample+=image->sample_bytes;
        for(f=0;f<image->face_count;++f)for(m=0;m<image->count;++m)
            if(image->levels[f][m].rgba)rgba+=image->levels[f][m].bytes;
    }
    if(encoded>o->bytes||sample>o->bytes-encoded||rgba>o->bytes-encoded-sample)return 0;
    out->total_owned_bytes=o->bytes;out->encoded_mapped_bytes=encoded;
    out->encoded_payload_bytes=payload;out->host_sample_bytes=sample;
    out->rgba_mip_bytes=rgba;out->auxiliary_bytes=o->bytes-encoded-sample-rgba;
    return 1;
}
int mac_native_bitmap_decode(const struct mac_retail_cache*c,uint32_t h,const void*external,size_t external_bytes,struct mac_native_bitmap_tag*out,const char**error)
{
    struct mac_retail_tag tag;struct bitmap_owner *o=NULL;const unsigned char *p,*q,*s,*pixels;
    struct bitmap_group_sequence *seq;struct bitmap_data *bitmaps;uint32_t n,ns,i,j,k,levels=0,digest=2166136261U;
    static const unsigned root_shorts[]={0,2,4,6,20,22,24,26,76,78,80,82};
    static const unsigned root_reals[]={8,12,16,68,72};
    if(error)*error="native bitmap argument/group";if(!out)return 0;memset(out,0,sizeof(*out));
    if(!c || !mac_retail_tag_get(c,h&65535,&tag) || tag.handle!=h || tag.group!=0x6269746dU)return 0;
    p=mac_retail_tag_span(c,tag.address,108);if(!p)return 0;
#define FAIL(msg) do{if(error)*error=msg;mac_native_bitmap_owner_free(o);return 0;}while(0)
    /* Retail retains editor size/offset metadata without runtime addresses.
       Preserve those scalars; nonempty addressed editor data is unsupported. */
    if(r32(p+40)||r32(p+60))FAIL("native bitmap addressed editor tag_data unsupported");
    ns=r32(p+84);n=r32(p+96);if(ns>256 || !n || n>2048)FAIL("native bitmap block count");
    o=calloc(1,sizeof(*o));if(!o)FAIL("native bitmap allocation");o->bytes=sizeof(*o);o->handle=h;
    /* port: the tag path is kept for texture packs (texture_pack.c); copied within the tag data */
    if(tag.name>=(const char*)c->tags&&tag.name<(const char*)c->tags+c->tag_size){
        size_t room=(size_t)((const char*)c->tags+c->tag_size-tag.name),n2=0;
        while(n2<room&&n2<sizeof(o->name)-1&&tag.name[n2])++n2;
        if(n2<room&&!tag.name[n2])memcpy(o->name,tag.name,n2);
    }
    for(i=0;i<sizeof(root_shorts)/sizeof(*root_shorts);++i)short_at((char*)&o->root+root_shorts[i],p+root_shorts[i]);
    for(i=0;i<sizeof(root_reals)/sizeof(*root_reals);++i)if(!real_at((char*)&o->root+root_reals[i],p+root_reals[i]))FAIL("native bitmap nonfinite root");
    /* Editor metadata is not a live imported-image buffer in a retail cache. */
    o->root.import_bitmap.size=r32(p+28);o->root.pixel_data.size=r32(p+48);
    o->root.import_bitmap.pad=r32(p+32);o->root.import_bitmap.file_offset=r32(p+36);
    o->root.pixel_data.pad=r32(p+52);o->root.pixel_data.file_offset=r32(p+56);
    o->root.sequences.count=ns;o->root.bitmaps.count=n;
    seq=allocate(o,ns*sizeof(*seq));o->root.sequences.address=seq;
    bitmaps=allocate(o,n*sizeof(*bitmaps));o->root.bitmaps.address=bitmaps;
    o->images=allocate(o,n*sizeof(*o->images));if((ns&&!seq)||!bitmaps||!o->images)FAIL("native bitmap allocation");
    q=mac_retail_tag_span(c,r32(p+88),ns*64);if(ns&&(!q||(r32(p+88)&3)))FAIL("native bitmap sequences");
    for(i=0;i<ns;++i){struct bitmap_group_sprite *sprites;uint32_t sn;const unsigned char *a=q+i*64;
        memcpy(seq[i].name,a,32);if(!memchr(a,0,32))FAIL("native bitmap sequence name");
        seq[i].first_bitmap_index=(short)r16(a+32);seq[i].bitmap_count=(short)r16(a+34);
        if(seq[i].first_bitmap_index<0||seq[i].bitmap_count<0||(uint32_t)seq[i].first_bitmap_index+seq[i].bitmap_count>n)FAIL("native bitmap sequence range");
        for(j=0;j<4;++j)seq[i].unused[j]=r32(a+36+j*4);
        sn=r32(a+52);if(sn>64)FAIL("native bitmap sprite count");seq[i].sprites.count=sn;
        sprites=allocate(o,sn*sizeof(*sprites));seq[i].sprites.address=sprites;
        s=mac_retail_tag_span(c,r32(a+56),sn*32);if(sn&&(!sprites||!s||(r32(a+56)&3)))FAIL("native bitmap sprites");
        for(j=0;j<sn;++j){sprites[j].bitmap_index=(short)r16(s+j*32);if(sprites[j].bitmap_index<0||(uint32_t)sprites[j].bitmap_index>=n)FAIL("native bitmap sprite index");
            sprites[j].bitmap_pad=(short)r16(s+j*32+2);sprites[j].unused=r32(s+j*32+4);
            for(k=8;k<32;k+=4)if(!real_at((char*)&sprites[j]+k,s+j*32+k))FAIL("native bitmap sprite float");}
    }
    q=mac_retail_tag_span(c,r32(p+100),n*48);if(!q||(r32(p+100)&3))FAIL("native bitmap images");
    for(i=0;i<n;++i){struct bitmap_data *b=&bitmaps[i];struct bitmap_image *image=&o->images[i];
        const unsigned char*a=q+i*48;uint32_t w,hh,dd,m,offset,stored,ulog=0,vlog=0,dlog=0,format,disk_flags,slices;size_t total=0,level,encoded_slice,file_bytes;
        b->signature=r32(a);if(b->signature!=0x6269746dU)FAIL("native bitmap signature");for(j=4;j<24;j+=2)short_at((char*)b+j,a+j);
        b->pixels_offset=r32(a+24);b->pixels_size=r32(a+28);b->tag_index=h;b->cache_block_index=NONE;
        disk_flags=b->flags;
        if(b->width<1||b->height<1||b->width>2048||b->height>2048||b->depth<1||b->depth>512||
           (b->type!=0&&b->type!=1&&b->type!=2)||(b->type!=1&&b->depth!=1)||
           (b->format!=6&&b->format!=8&&b->format!=9&&b->format!=10&&b->format!=11&&(b->format<14||b->format>17))||
           (b->type==1&&(b->format>=14||(b->flags&8)))||(b->flags&8)||((b->flags&4)&&b->format!=17)||
           ((b->format>=14&&b->format<=16)!=!!(b->flags&2))) {
            fprintf(stderr,"native bitmap rejected image %u: %ux%u depth=%u type=%u format=%u flags=%u (source %04lx)\n",
                (unsigned)i,(unsigned)b->width,(unsigned)b->height,(unsigned)b->depth,(unsigned)b->type,
                (unsigned)b->format,(unsigned)b->flags,(unsigned long)(h&65535));
            FAIL("native bitmap unsupported image family");
        }
        if(b->type==2&&b->width!=b->height)FAIL("native bitmap cubemap faces are not square");
        w=b->width;hh=b->height;dd=(uint32_t)b->depth;
        if((w&(w-1))||(hh&(hh-1))||(b->type==1&&(dd&(dd-1))))FAIL("native bitmap non-power-of-two image");
        while((1U<<ulog)<w)++ulog;while((1U<<vlog)<hh)++vlog;while((1U<<dlog)<dd)++dlog;
        if(b->mipmap_count<0||b->mipmap_count>(short)(ulog>vlog?(ulog>dlog?ulog:dlog):(vlog>dlog?vlog:dlog)))FAIL("native bitmap mip count");
        image->count=b->mipmap_count+1;image->face_count=b->type==2?6:1;levels+=image->count*image->face_count;
        image->is_volume=b->type==1;image->volume_depth=b->type==1?dd:0;
        for(m=0;m<image->count;++m){
            encoded_slice=level_bytes(b->format,w,hh);slices=b->type==2?6U:b->type==1?dd:1U;
            if(!encoded_slice||encoded_slice>BITMAP_LIMIT/slices)FAIL("native bitmap mip storage exceeds budget");
            level=encoded_slice*slices;if(level>BITMAP_LIMIT||total>BITMAP_LIMIT-level)FAIL("native bitmap mip chain exceeds budget");total+=level;
            w=w>1?w/2:1;hh=hh>1?hh/2:1;dd=dd>1?dd/2:1;
        }
        stored=r32(a+28);offset=r32(a+24);pixels=(disk_flags&256)?external:c->file;file_bytes=(disk_flags&256)?external_bytes:c->file_size;
        if(!pixels||stored<total||offset>file_bytes||stored>file_bytes-offset)FAIL("native bitmap pixel range");
        /* Retail's packed tag stores external-resource and cached metadata in
           the upper bits. External storage is not a runtime bitmap flag.
           Clear cached too: this resident owner has no Xbox cache block. */
        b->flags=(short)(disk_flags&0x7f);
        /* D3D texture Data stores an Xbox physical address. Reserve actual
           native high-window pages without replacing an existing mapping. */
        image->encoded_bytes=(total+vm_page_size-1)&~(vm_page_size-1);image->encoded_payload_bytes=total;
        if(image->encoded_bytes>BITMAP_LIMIT-o->bytes)FAIL("native bitmap resource budget");
        {unsigned long found=0;if(mac_resource_vm_allocate_next(PLATFORM_CONTIGUOUS_BASE+vm_page_size,PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE-image->encoded_bytes,vm_page_size,image->encoded_bytes,&found))image->encoded=(unsigned char*)found;}
        if(!image->encoded)FAIL("native bitmap resource mapping");o->bytes+=image->encoded_bytes;memcpy(image->encoded,pixels+offset,total);
        b->base_address=image->encoded;image->texture.Common=D3DCOMMON_TYPE_TEXTURE|1;
        image->texture.Data=(DWORD)image->encoded&0x7fffffffUL;
        format=b->format==6?D3DFMT_R5G6B5:b->format==8?D3DFMT_A1R5G5B5:b->format==9?D3DFMT_A4R4G4B4:b->format==10?D3DFMT_X8R8G8B8:(b->format==11||b->format==17)?D3DFMT_A8R8G8B8:b->format==14?D3DFMT_DXT1:b->format==15?D3DFMT_DXT3:D3DFMT_DXT5;
        image->texture.Format=D3DFORMAT_DMACHANNEL_A|D3DFORMAT_BORDERSOURCE_COLOR|((b->type==1?3U:2U)<<D3DFORMAT_DIMENSION_SHIFT)|(image->face_count==6?D3DFORMAT_CUBEMAP:0)|(format<<D3DFORMAT_FORMAT_SHIFT)|(image->count<<D3DFORMAT_MIPMAP_SHIFT)|(ulog<<D3DFORMAT_USIZE_SHIFT)|(vlog<<D3DFORMAT_VSIZE_SHIFT)|(b->type==1?(dlog<<D3DFORMAT_PSIZE_SHIFT):0);
        b->hardware_format=&image->texture;
#if defined(__ppc__)
        native_sample_pixels(o,image,b,total);if(!image->sample_pixels)FAIL("native bitmap sample allocation");
        image->sample_bytes=total;
        b->base_address=image->sample_pixels;
#else
        image->sample_pixels=image->encoded;b->base_address=image->encoded;
#endif
        w=b->width;hh=b->height;dd=(uint32_t)b->depth;offset=0;
        for(m=0;m<image->count;++m){uint32_t encoded=(uint32_t)level_bytes(b->format,w,hh),face;
            if(b->type==1){struct bitmap_level*l=&image->levels[0][m];
                l->width=w;l->height=hh;l->depth=dd;l->format=b->format;l->bytes=(size_t)w*hh*dd*4;
                l->encoded_offset=offset;l->encoded_bytes=(size_t)encoded*dd;
                if(l->bytes>BITMAP_LIMIT-o->bytes)FAIL("native bitmap volume RGBA budget");
                {unsigned z;unsigned char *scratch=malloc((size_t)w*hh*4);if(!scratch)FAIL("native bitmap volume digest allocation");
                 for(z=0;z<dd;++z){if(!mac_retail_texture_decode(b->format,w,hh,image->encoded+offset+(size_t)z*encoded,encoded,scratch,(size_t)w*hh*4)){free(scratch);FAIL("native bitmap volume mip decode");}
                    for(j=0;j<(size_t)w*hh*4;++j)digest=(digest^scratch[j])*16777619U;}
                 free(scratch);}
            } else for(face=0;face<image->face_count;++face){struct bitmap_level*l=&image->levels[face][m];
                unsigned char *scratch;
                l->width=w;l->height=hh;l->depth=1;l->format=b->format;l->bytes=(size_t)w*hh*4;
                l->encoded_offset=(size_t)offset+(size_t)face*encoded;l->encoded_bytes=encoded;
                scratch=malloc(l->bytes);if(!scratch)FAIL("native bitmap digest scratch allocation");
                if(!mac_retail_texture_decode(b->format,w,hh,image->encoded+l->encoded_offset,encoded,scratch,l->bytes)) {free(scratch);FAIL("native bitmap mip decode");}
                for(j=0;j<l->bytes;++j)digest=(digest^scratch[j])*16777619U;
                free(scratch);}
            offset+=encoded*(b->type==1?dd:image->face_count);w=w>1?w/2:1;hh=hh>1?hh/2:1;dd=dd>1?dd/2:1;}
    }
    out->root=&o->root;out->owner=o;out->handle=h;out->group=0x6269746dU;out->image_count=n;out->level_count=levels;out->pixel_digest=digest;out->root_bytes=sizeof(o->root);out->bytes_owned=o->bytes;if(error)*error=NULL;return 1;
#undef FAIL
}
void *mac_native_bitmap_hardware(void *owner,const struct bitmap_data *bitmap)
{
    /* (by position in the owner's array: the game asks once per texture set,
       of every owner in turn) */
    struct bitmap_owner*o=owner;unsigned long base,address,offset,index;
    if(!o||!bitmap||o->root.bitmaps.count<=0)return NULL;
    base=(unsigned long)o->root.bitmaps.address;address=(unsigned long)bitmap;
    if(address<base)return NULL;
    offset=address-base;
    if(offset%sizeof(struct bitmap_data))return NULL;
    index=offset/sizeof(struct bitmap_data);
    if(index>=(unsigned long)o->root.bitmaps.count)return NULL;
    return &o->images[index].texture;
}
const unsigned char *mac_native_bitmap_rgba(void *owner,uint32_t image,uint32_t mip,uint32_t*w,uint32_t*h,size_t*bytes)
{return mac_native_bitmap_rgba_face(owner,image,0,mip,w,h,bytes);}
static const unsigned char *materialize_rgba(struct bitmap_owner *o,struct bitmap_image *image,
    struct bitmap_level *l)
{
    unsigned char *pixels;
    int reserved=0;
    if(l->rgba)return l->rgba;
    if(!o||!l||!l->bytes||l->bytes>BITMAP_LIMIT-o->bytes)return NULL;
    if(o->accounting_bound) {
        if(!mac_native_tags_owner_bytes_reserve(&o->registry,o->handle,o,l->bytes,NULL))return NULL;
        reserved=1;
    }
    pixels=malloc(l->bytes);
    if(!pixels) {
        if(reserved)mac_native_tags_owner_bytes_release(&o->registry,o->handle,o,l->bytes);
        return NULL;
    }
    if(!mac_retail_texture_decode(l->format,l->width,l->height,
        image->encoded+l->encoded_offset,l->encoded_bytes,pixels,l->bytes)) {
        free(pixels);
        if(reserved)mac_native_tags_owner_bytes_release(&o->registry,o->handle,o,l->bytes);
        return NULL;
    }
    o->bytes+=l->bytes;l->rgba=pixels;return pixels;
}
static const unsigned char *materialize_volume_rgba(struct bitmap_owner *o,
    struct bitmap_image *image,struct bitmap_level *l)
{
    unsigned char *pixels,*slice;
    size_t slice_bytes;
    uint32_t z;
    int reserved=0;
    if(l->rgba)return l->rgba;
    if(!o||!image||!l||!image->volume_depth||!l->depth||
       l->depth>512||!l->bytes||l->bytes>BITMAP_LIMIT-o->bytes)return NULL;
    slice_bytes=(size_t)l->width*l->height*4;
    if(slice_bytes>l->bytes||l->depth>l->bytes/slice_bytes||
       l->bytes!=(size_t)l->depth*slice_bytes||
       l->encoded_bytes%l->depth!=0||l->encoded_offset>image->encoded_payload_bytes||
       l->encoded_bytes>image->encoded_payload_bytes-l->encoded_offset)return NULL;
    if(o->accounting_bound) {
        if(!mac_native_tags_owner_bytes_reserve(&o->registry,o->handle,o,l->bytes,NULL))return NULL;
        reserved=1;
    }
    pixels=malloc(l->bytes);slice=malloc(slice_bytes);
    if(!pixels||!slice) {
        free(pixels);free(slice);
        if(reserved)mac_native_tags_owner_bytes_release(&o->registry,o->handle,o,l->bytes);
        return NULL;
    }
    for(z=0;z<l->depth;++z) {
        size_t source_offset=l->encoded_offset+(size_t)z*l->encoded_bytes/l->depth;
        size_t source_bytes=l->encoded_bytes/l->depth;
        if(!mac_retail_texture_decode(l->format,l->width,l->height,
            image->encoded+source_offset,source_bytes,slice,slice_bytes)) {
            free(pixels);free(slice);
            if(reserved)mac_native_tags_owner_bytes_release(&o->registry,o->handle,o,l->bytes);
            return NULL;
        }
        memcpy(pixels+(size_t)z*slice_bytes,slice,slice_bytes);
    }
    free(slice);o->bytes+=l->bytes;l->rgba=pixels;return pixels;
}
const unsigned char *mac_native_bitmap_rgba_face(void *owner,uint32_t image,uint32_t face,uint32_t mip,uint32_t*w,uint32_t*h,size_t*bytes)
{struct bitmap_owner*o=owner;struct bitmap_level*l;if(!o||image>=(uint32_t)o->root.bitmaps.count||o->images[image].is_volume||face>=o->images[image].face_count||mip>=o->images[image].count)return NULL;l=&o->images[image].levels[face][mip];if(w)*w=l->width;if(h)*h=l->height;if(bytes)*bytes=l->bytes;return materialize_rgba(o,&o->images[image],l);}
const unsigned char *mac_native_bitmap_encoded_face(void *owner,uint32_t image,
    uint32_t face,uint32_t mip,uint32_t *format,uint32_t *w,uint32_t *h,
    size_t *bytes)
{
    struct bitmap_owner *o=owner;
    struct bitmap_image *bitmap;
    struct bitmap_level *level;
    size_t blocks_w,blocks_h,expected;
    unsigned block_bytes;
    if(format)*format=0;if(w)*w=0;if(h)*h=0;if(bytes)*bytes=0;
    if(!format||!w||!h||!bytes||!o||!o->images||
       o->root.bitmaps.count<1||o->root.bitmaps.count>2048||
       image>=(uint32_t)o->root.bitmaps.count)return NULL;
    bitmap=&o->images[image];
    if(!bitmap->encoded||bitmap->is_volume||
       (bitmap->face_count!=1&&bitmap->face_count!=6)||
       bitmap->count<1||bitmap->count>12||face>=bitmap->face_count||
       mip>=bitmap->count||bitmap->encoded_payload_bytes>bitmap->encoded_bytes)
        return NULL;
    level=&bitmap->levels[face][mip];
    if(level->format<14||level->format>16||!level->width||!level->height||
       level->width>2048||level->height>2048||level->depth!=1)
        return NULL;
    blocks_w=((size_t)level->width+3)/4;
    blocks_h=((size_t)level->height+3)/4;
    block_bytes=level->format==14?8U:16U;
    if(!blocks_w||!blocks_h||blocks_w>(size_t)-1/blocks_h/block_bytes)
        return NULL;
    expected=blocks_w*blocks_h*block_bytes;
    if(level->encoded_bytes!=expected||level->encoded_offset>bitmap->encoded_payload_bytes||
       expected>bitmap->encoded_payload_bytes-level->encoded_offset||
       level->bytes!=(size_t)level->width*level->height*4)
        return NULL;
    *format=level->format;*w=level->width;*h=level->height;*bytes=expected;
    return bitmap->encoded+level->encoded_offset;
}
const unsigned char *mac_native_bitmap_rgba_volume(void *owner,uint32_t image,uint32_t mip,
    uint32_t*w,uint32_t*h,uint32_t*d,size_t*bytes)
{struct bitmap_owner*o=owner;struct bitmap_level*l;if(!o||image>=(uint32_t)o->root.bitmaps.count||!o->images[image].is_volume||mip>=o->images[image].count)return NULL;l=&o->images[image].levels[0][mip];if(w)*w=l->width;if(h)*h=l->height;if(d)*d=l->depth;if(bytes)*bytes=l->bytes;return materialize_volume_rgba(o,&o->images[image],l);}
const unsigned char *mac_native_bitmap_rgba_for_hardware(void *owner,const void *hardware,
    uint32_t mip,uint32_t *image,uint32_t *w,uint32_t *h,size_t *bytes)
{struct bitmap_owner*o=owner;unsigned i;if(!o||!hardware)return NULL;
 for(i=0;i<(unsigned)o->root.bitmaps.count;++i)if(hardware==&o->images[i].texture){
     if(image)*image=i;return mac_native_bitmap_rgba(owner,i,mip,w,h,bytes);}
 return NULL;}
const unsigned char *mac_native_bitmap_rgba_volume_for_hardware(void *owner,const void *hardware,
    uint32_t mip,uint32_t*w,uint32_t*h,uint32_t*d,size_t*bytes)
{struct bitmap_owner*o=owner;unsigned i;if(!o||!hardware)return NULL;
 for(i=0;i<(unsigned)o->root.bitmaps.count;++i)if(hardware==&o->images[i].texture)
     return mac_native_bitmap_rgba_volume(owner,i,mip,w,h,d,bytes);
 return NULL;}
