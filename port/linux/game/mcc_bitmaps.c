/* MCC bitmap resources. Convert and pack each resource in MCC-owned memory,
 * then stream its native hardware layout. The common rasterizer supplies
 * descriptor size limits; no CE bitmap state or conversion API is used. */
#include "cseries.h"
#include "errors.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "interface/unit_hud_interface_definition.h"
#include "rasterizer/rasterizer_swizzle.h"
#include "mcc_runtime.h"
#include <stdlib.h>
#include <string.h>
#define BCDEC_STATIC
#define BCDEC_IMPLEMENTATION
#include "../../third_party/bcdec/bcdec.h"

#define MCC_BITMAP_BASE 0x60000000u
#define MCC_BITMAP_LIMIT 0x1FFFFFFFu
#define MCC_BITMAP_ALLOCATION_LIMIT 0x08000000u

struct mcc_texture {
    struct bitmap_data *bitmap;
    uint32_t handle, offset, size;
    unsigned char *pixels;
    unsigned channels;
};
struct mcc_bitmap_clone { uint32_t source, target; unsigned channels; };
struct mcc_textures {
    struct mcc_texture *items;
    struct mcc_bitmap_clone *clones;
    uint32_t count, end, capacity, clone_count, tag_capacity;
};

static void *mcc_bitmap_tag(struct mcc_runtime *runtime,uint32_t handle,uint32_t group,uint32_t size)
{
    uint32_t *entry;
    if (handle==0xFFFFFFFFu || (handle&0xFFFF)>=runtime->report.tag_count) return NULL;
    entry=(uint32_t *)(runtime->tag_index+(handle&0xFFFF)*0x20);
    if (entry[0]!=group || entry[3]!=handle) return NULL;
    return mcc_runtime_pointer(runtime,entry[5],size);
}

static void *mcc_bitmap_block(struct mcc_runtime *runtime,struct tag_block const *block,uint32_t size)
{
    if (block->count<0 || (uint32_t)block->count>UINT32_MAX/size) return NULL;
    if (!block->count) return runtime->tags;
    return mcc_runtime_pointer(runtime,(uint32_t)block->address,(uint32_t)block->count*size);
}

static void mcc_bitmap_mark_one(struct mcc_textures *textures,struct bitmap_data *bitmap,unsigned channels)
{
    uint32_t index;
    for (index=0;index<textures->count;index++) {
        if (textures->items[index].bitmap==bitmap) textures->items[index].channels|=channels;
    }
}

static int mcc_bitmap_clone_block(struct mcc_runtime *runtime,struct tag_block *block,uint32_t stride)
{
    void *source,*copy;
    uint32_t bytes;
    if (!block->count) {block->address=NULL;return 1;}
    source=mcc_bitmap_block(runtime,block,stride);
    if (!source) return 0;
    bytes=(uint32_t)block->count*stride;
    copy=mcc_runtime_allocate(runtime,bytes);
    if (!copy) return 0;
    memcpy(copy,source,bytes);block->address=copy;
    return 1;
}

/* A bitmap can be used as both color and a packed material mask. Each
 * specialized consumer gets its own tag and descriptors, preserving the
 * ordinary source tag and every other consumer's channels. */
static int mcc_bitmap_specialize(struct mcc_runtime *runtime,struct mcc_textures *textures,
    uint32_t *handle,unsigned channels)
{
    struct bitmap_group *source,*copy;
    struct bitmap_group_sequence *sequences;
    struct bitmap_data *bitmaps;
    uint32_t i,*entry,new_handle;
    char *name;
    if (*handle==0xFFFFFFFFu || channels==1) return 1;
    for (i=0;i<textures->clone_count;i++) {
        struct mcc_bitmap_clone const *clone=&textures->clones[i];
        if (clone->source==*handle && clone->channels==channels) {*handle=clone->target;return 1;}
    }
    if (runtime->report.tag_count>=textures->tag_capacity) return 0;
    source=mcc_bitmap_tag(runtime,*handle,BITMAP_GROUP_TAG,sizeof(*source));
    if (!source) return 0;
    copy=mcc_runtime_allocate(runtime,sizeof(*copy));
    if (!copy) return 0;
    *copy=*source;
    if (!mcc_bitmap_clone_block(runtime,&copy->sequences,sizeof(*sequences)) ||
        !mcc_bitmap_clone_block(runtime,&copy->bitmaps,sizeof(*bitmaps))) return 0;
    sequences=copy->sequences.address;
    for (i=0;i<(uint32_t)copy->sequences.count;i++) {
        if (!mcc_bitmap_clone_block(runtime,&sequences[i].sprites,sizeof(struct bitmap_group_sprite))) return 0;
    }
    /* Import/pixel tag_data are editor storage. Runtime pixels use each
     * descriptor's file range, so these never alias a cloned tag allocation. */
    memset(&copy->import_bitmap,0,sizeof(copy->import_bitmap));
    memset(&copy->pixel_data,0,sizeof(copy->pixel_data));
    name=mcc_runtime_allocate(runtime,48);
    if (!name) return 0;
    snprintf(name,48,"mcc_runtime\\bitmap_%08lx_%u",(unsigned long)*handle,channels);
    entry=(uint32_t *)(runtime->tag_index+runtime->report.tag_count*32);
    memcpy(entry,runtime->tag_index+(*handle&0xFFFF)*32,32);
    new_handle=(*handle&0xFFFF0000u)|runtime->report.tag_count;
    entry[3]=new_handle;entry[4]=(uint32_t)name;entry[5]=(uint32_t)copy;
    bitmaps=copy->bitmaps.address;
    for (i=0;i<(uint32_t)copy->bitmaps.count;i++) {
        struct mcc_texture *texture;
        if (textures->count>=textures->capacity) return 0;
        texture=&textures->items[textures->count++];
        texture->bitmap=&bitmaps[i];texture->handle=new_handle;texture->channels=channels;
    }
    textures->clones[textures->clone_count].source=*handle;
    textures->clones[textures->clone_count].target=new_handle;
    textures->clones[textures->clone_count++].channels=channels;
    runtime->report.tag_count++;
    *handle=new_handle;
    return 1;
}

/* Channels is a usage mask: ordinary=1, multipurpose=2, HUD meter=4. */
static int mcc_bitmap_mark(struct mcc_runtime *runtime,struct mcc_textures *textures,
    uint32_t *reference,short sequence,unsigned channels)
{
    struct bitmap_group *group;
    struct bitmap_data *bitmaps;
    uint32_t index;
    uint32_t handle;
    if (!mcc_bitmap_specialize(runtime,textures,reference,channels)) return 0;
    handle=*reference;
    if (handle==0xFFFFFFFFu) return 1;
    group=mcc_bitmap_tag(runtime,handle,BITMAP_GROUP_TAG,sizeof(*group));
    if (!group) return 0;
    bitmaps=mcc_bitmap_block(runtime,&group->bitmaps,sizeof(*bitmaps));
    if (!bitmaps) return 0;
    if (sequence>=0 && sequence<group->sequences.count) {
        struct bitmap_group_sequence *sequences=mcc_bitmap_block(runtime,&group->sequences,sizeof(*sequences));
        struct bitmap_group_sequence *selected;
        if (!sequences) return 0;
        selected=&sequences[sequence];
        if (selected->sprites.count) {
            struct bitmap_group_sprite *sprites=mcc_bitmap_block(runtime,&selected->sprites,sizeof(*sprites));
            if (!sprites) return 0;
            for (index=0;index<(uint32_t)selected->sprites.count;index++) {
                short n=sprites[index].bitmap_index;
                if (n<0 || n>=group->bitmaps.count) return 0;
                mcc_bitmap_mark_one(textures,&bitmaps[n],channels);
            }
        } else {
            if (selected->first_bitmap_index<0 || selected->bitmap_count<0 ||
                selected->first_bitmap_index>group->bitmaps.count-selected->bitmap_count) return 0;
            for (index=0;index<(uint32_t)selected->bitmap_count;index++)
                mcc_bitmap_mark_one(textures,&bitmaps[selected->first_bitmap_index+index],channels);
        }
    } else {
        for (index=0;index<(uint32_t)group->bitmaps.count;index++) mcc_bitmap_mark_one(textures,&bitmaps[index],channels);
    }
    return 1;
}

static int mcc_meter_usage(struct mcc_runtime *runtime,struct mcc_textures *textures,
    struct meter_hud_element_definition *meter)
{
    /* MCC meter flag 5 keeps Xbox fill/shape channels. The native meter
     * already consumes that order; only Gearbox-style meters need a clone
     * with luminance and alpha exchanged. */
    unsigned channels=meter->meter_flags&0x20 ? 1 : 4;
    return mcc_bitmap_mark(runtime,textures,(uint32_t *)&meter->meter_bitmap.index,meter->sequence_index,channels);
}

static int mcc_static_usage(struct mcc_runtime *runtime,struct mcc_textures *textures,
    struct static_hud_element_definition *element)
{
    return mcc_bitmap_mark(runtime,textures,(uint32_t *)&element->interface_bitmap.index,element->sequence_index,1);
}

static int mcc_bitmap_model_usage(struct mcc_runtime *runtime,struct mcc_textures *textures,
    unsigned char *shader)
{
    static unsigned const offsets[]={0xB0,0xC8,0xE8,0x170};
    uint16_t flags;
    unsigned n;
    memcpy(&flags,shader+0x28,sizeof(flags));
    /* MCC's model flag 6 selects original Xbox mask channels. These masks
     * already have reflection in red and change color in blue; applying the
     * Gearbox swizzle would turn opaque DXT1 alpha into full-surface tint.
     * Select per reference: another shader may use this same bitmap with
     * the flag clear and must still receive its own reordered clone. */
    for (n=0;n<4;n++) {
        unsigned channels=n==1 && !(flags&0x40) ? 2 : 1;
        if (!mcc_bitmap_mark(runtime,textures,(uint32_t *)(shader+offsets[n]),NONE,channels)) return 0;
    }
    return 1;
}

static int mcc_bitmap_usages(struct mcc_runtime *runtime,struct mcc_textures *textures)
{
    uint32_t index;
    for (index=0;index<runtime->report.tag_count;index++) {
        uint32_t *entry=(uint32_t *)(runtime->tag_index+index*0x20);
        if (entry[0]=='soso') {
            unsigned char *shader=mcc_runtime_pointer(runtime,entry[5],0x174);
            if (!shader || !mcc_bitmap_model_usage(runtime,textures,shader)) return 0;
        } else if (entry[0]==UNIT_HUD_INTERFACE_DEFINITION_TAG) {
            struct unit_hud_interface_definition *hud=mcc_runtime_pointer(runtime,entry[5],sizeof(*hud));
            struct auxilary_meter_definition *meters;
            struct auxilary_overlay_definition *overlays;
            uint32_t n;
            if (!hud || !mcc_meter_usage(runtime,textures,&hud->shield_meter.meter) ||
                !mcc_meter_usage(runtime,textures,&hud->health_meter.meter) ||
                !mcc_static_usage(runtime,textures,&hud->background) ||
                !mcc_static_usage(runtime,textures,&hud->shield_meter.background) ||
                !mcc_static_usage(runtime,textures,&hud->health_meter.background) ||
                !mcc_static_usage(runtime,textures,&hud->motion_sensor.background) ||
                !mcc_static_usage(runtime,textures,&hud->motion_sensor.foreground)) return 0;
            meters=mcc_bitmap_block(runtime,&hud->auxilary_meters,sizeof(*meters));
            overlays=mcc_bitmap_block(runtime,&hud->auxilary_panel.auxilary_overlays,sizeof(*overlays));
            if (!meters || !overlays) return 0;
            for (n=0;n<(uint32_t)hud->auxilary_meters.count;n++) {
                if (!mcc_meter_usage(runtime,textures,&meters[n].panel.meter) ||
                    !mcc_static_usage(runtime,textures,&meters[n].panel.background)) return 0;
            }
            for (n=0;n<(uint32_t)hud->auxilary_panel.auxilary_overlays.count;n++)
                if (!mcc_static_usage(runtime,textures,&overlays[n].static_element)) return 0;
        } else if (entry[0]=='wphi') {
            unsigned char *hud=mcc_runtime_pointer(runtime,entry[5],0x78);
            unsigned which;
            if (!hud) return 0;
            for (which=0;which<2;which++) {
                struct tag_block *block=(struct tag_block *)(hud+0x60+which*12);
                unsigned char *elements=mcc_bitmap_block(runtime,block,0xB4);
                uint32_t n;
                if (!elements) return 0;
                for (n=0;n<(uint32_t)block->count;n++) {
                    void *element=elements+n*0xB4+0x24;
                    if (!(which ? mcc_meter_usage(runtime,textures,element) : mcc_static_usage(runtime,textures,element))) return 0;
                }
            }
        }
    }
    return 1;
}

static int mcc_bitmap_bytes(short format)
{
    switch (format) {
        case 0: case 1: case 2: case 17: return 1;
        case 3: case 6: case 8: case 9: return 2;
        case 10: case 11: return 4;
        default: return 0;
    }
}

static uint32_t mcc_pixel(unsigned char const *source,int format)
{
    unsigned v=source[0], a=255,r=v,g=v,b=v;
    if (format==0) {a=v; r=g=b=255;}
    else if (format==2) a=v;
    else if (format==3) a=source[1];
    else if (format==6 || format==8 || format==9) {
        v|=(unsigned)source[1]<<8;
        if (format==6) {r=(v>>11)*255/31; g=((v>>5)&63)*255/63; b=(v&31)*255/31;}
        else if (format==8) {a=(v&0x8000)?255:0; r=((v>>10)&31)*255/31; g=((v>>5)&31)*255/31; b=(v&31)*255/31;}
        else {a=(v>>12)*17; r=((v>>8)&15)*17; g=((v>>4)&15)*17; b=(v&15)*17;}
    } else if (format==10 || format==11) {
        b=source[0];g=source[1];r=source[2];a=format==10 ? 255 : source[3];
    } else if (format==17) return global_vector_palette[v];
    return b|(g<<8)|(r<<16)|(a<<24);
}

static uint32_t mcc_channels(uint32_t color,unsigned usage)
{
    unsigned b=color&255,g=(color>>8)&255,r=(color>>16)&255,a=color>>24;
    if (usage==2) return a|(g<<8)|(b<<16)|(r<<24);
    if (usage==4) return a|(a<<8)|(a<<16)|(r<<24);
    return color;
}

/* Decode one slice to BGRA. Compressed blocks remain 4x4 even for tiny mips. */
static void mcc_bitmap_decode(unsigned char const *input,uint32_t *output,
    unsigned width,unsigned height,short format,unsigned usage)
{
    unsigned x,y;
    if (format>=14 && format!=17) {
        unsigned step=format==14 ? 8 : 16;
        for (y=0;y<height;y+=4) for (x=0;x<width;x+=4) {
            unsigned char rgba[64];
            unsigned dx,dy;
            if (format==14) bcdec_bc1(input,rgba,16);
            else if (format==15) bcdec_bc2(input,rgba,16);
            else if (format==16) bcdec_bc3(input,rgba,16);
            else bcdec_bc7(input,rgba,16);
            input+=step;
            for (dy=0;dy<4 && y+dy<height;dy++) for (dx=0;dx<4 && x+dx<width;dx++) {
                unsigned char const *p=rgba+(dy*4+dx)*4;
                uint32_t color=p[2]|((uint32_t)p[1]<<8)|((uint32_t)p[0]<<16)|((uint32_t)p[3]<<24);
                output[(y+dy)*width+x+dx]=mcc_channels(color,usage);
            }
        }
    } else {
        unsigned step=mcc_bitmap_bytes(format);
        for (x=0;x<width*height;x++) output[x]=mcc_channels(mcc_pixel(input+x*step,format),usage);
    }
}

static uint32_t mcc_morton_axis(unsigned value,unsigned width,unsigned height,unsigned depth,unsigned axis)
{
    unsigned dimensions[3]={width,height,depth},bit,position=0,n;
    uint32_t address=0;
    for (bit=1;bit<width || bit<height || bit<depth;bit<<=1) {
        for (n=0;n<3;n++) if (bit<dimensions[n]) {
            if (n==axis && (value&bit)) address|=1u<<position;
            position++;
        }
    }
    return address;
}

/* MCC input is mip-major. Native textures have Morton-ordered uncompressed
 * texels, six face-major cube chains, and explicit row/face alignment. Pack
 * directly into an owned, zeroed buffer so allocation failure is observable. */
static int mcc_bitmap_pack(struct bitmap_data *bitmap,unsigned char const *source,
    uint32_t source_size,unsigned char *target,uint32_t target_size)
{
    static unsigned const cube_face[6]={0,2,1,3,4,5};
    uint32_t x_address[4096],y_address[4096],z_address[512],out=0;
    unsigned face,faces=bitmap->type==2 ? 6 : 1;
    unsigned maximum=rasterizer_xbox_bitmap_get_max_mipmap_count(bitmap);
    unsigned bytes=mcc_bitmap_bytes(bitmap->format);
    /* CPU object lighting can select the final 2x2/1x1 DXT mips even though
     * the GPU descriptor stops at 4 texels. A 2D chain has identical CPU/GPU
     * prefix layout, so retain its complete source tail in owned storage. */
    if (bitmap->type==0 && (bitmap->flags&2)) maximum=bitmap->mipmap_count;
    memset(target,0,target_size);
    for (face=0;face<faces;face++) {
        unsigned level;
        uint32_t input=0;
        for (level=0;level<=maximum;level++) {
            unsigned width=MAX(bitmap->width>>level,1),height=MAX(bitmap->height>>level,1),depth=MAX(bitmap->depth>>level,1);
            uint32_t size=bytes ? width*height*depth*bytes : ((width+3)/4)*((height+3)/4)*depth*(bitmap->format==14 ? 8 : 16);
            uint32_t at=input+(faces==6 ? cube_face[face] : 0)*size;
            unsigned x,y,z;
            if (at>source_size || size>source_size-at || out>target_size) return 0;
            if (bitmap->flags&16) {
                unsigned pitch=(width*bytes+63)&~63u;
                if (pitch*height>target_size-out) return 0;
                for (y=0;y<height;y++) memcpy(target+out+y*pitch,source+at+y*width*bytes,width*bytes);
                out+=pitch*height;
            } else {
                if (size>target_size-out) return 0;
                if (!bytes) memcpy(target+out,source+at,size);
                else {
                    for (x=0;x<width;x++) x_address[x]=mcc_morton_axis(x,width,height,depth,0);
                    for (y=0;y<height;y++) y_address[y]=mcc_morton_axis(y,width,height,depth,1);
                    for (z=0;z<depth;z++) z_address[z]=mcc_morton_axis(z,width,height,depth,2);
                    for (z=0;z<depth;z++) for (y=0;y<height;y++) for (x=0;x<width;x++) {
                        uint32_t address=x_address[x]|y_address[y]|z_address[z];
                        memcpy(target+out+address*bytes,source+at+((z*height+y)*width+x)*bytes,bytes);
                    }
                }
                out+=size;
            }
            input+=size*faces;
        }
        out=(out+127)&~127u;
    }
    if (!(bitmap->flags&(2|16))) bitmap->flags|=8;
    return out==target_size;
}

/* Recent MCC campaign lightmaps carry bit 12 in addition to the environment
 * flag. Its wider MCC meaning is not assumed here: admit only the observed
 * lightmap-group RGB565 layout with a complete, tightly packed pixel range.
 * The normal descriptor verifier still checks every other bit and field. */
static int mcc_bitmap_lightmap_flags(struct bitmap_data *bitmap,short usage)
{
    if (!(bitmap->flags&0x1000)) return 1;
    if (usage!=4 || bitmap->flags!=0x1281 || bitmap->type!=0 || bitmap->format!=6 ||
        bitmap->width<=0 || bitmap->height<=0 || bitmap->depth!=1 || bitmap->mipmap_count ||
        (uint64_t)(unsigned)bitmap->width*(unsigned)bitmap->height*2!=(uint32_t)bitmap->pixels_size) return 0;
    bitmap->flags&=~0x1000;
    return 1;
}

/* Some MCC HUD textures declare a complete DXT1 chain but size its sub-4x4
 * tail as width*height/2 instead of whole compression blocks. Recognize only
 * that exact size formula and retain the complete prefix. Arbitrary short
 * ranges, missing base levels and other formats remain load failures. */
static void mcc_bitmap_complete_mips(struct bitmap_data *bitmap)
{
    uint64_t packed=0;
    unsigned level,maximum,complete=0;
    if (bitmap->type!=0 || bitmap->format!=14 || bitmap->flags!=0x81 || bitmap->depth!=1 ||
        bitmap->width<4 || bitmap->height<4 ||
        (bitmap->width&(bitmap->width-1)) || (bitmap->height&(bitmap->height-1))) return;
    maximum=(unsigned)floor_log2(MAX(bitmap->width,bitmap->height));
    if (bitmap->mipmap_count!=(short)maximum) return;
    for (level=0;level<=maximum;level++) {
        unsigned width=MAX(bitmap->width>>level,1),height=MAX(bitmap->height>>level,1);
        packed+=(uint64_t)width*height/2;
        if (width>=4 && height>=4) complete=level;
    }
    if (packed==(uint32_t)bitmap->pixels_size) bitmap->mipmap_count=(short)complete;
}

static int mcc_texture_prepare(struct mcc_runtime *runtime,struct mcc_textures *textures,struct mcc_texture *texture)
{
    struct bitmap_data *bitmap=texture->bitmap, normalized=*bitmap;
    unsigned level,faces=bitmap->type==2 ? 6 : 1;
    uint64_t raw_size=0,decoded_size=0;
    unsigned char *raw=NULL,*pixels=NULL,*hardware=NULL;
    int convert=bitmap->format==18 || texture->channels==2 || texture->channels==4 ||
        ((bitmap->flags&16) && bitmap->format>=14 && bitmap->format<=17);
    int bytes_per_pixel=mcc_bitmap_bytes(bitmap->format);
    uint32_t input_offset=0,output_offset=0,hardware_size,allocation;
    if (bitmap->width<=0 || bitmap->height<=0 || bitmap->depth<=0 || bitmap->type<0 || bitmap->type>2 ||
        bitmap->width>4096 || bitmap->height>4096 || bitmap->depth>512 || bitmap->mipmap_count<0 || bitmap->mipmap_count>12 ||
        bitmap->pixels_size<=0 || (bitmap->flags&0x100) || (!bytes_per_pixel && (bitmap->format<14 || bitmap->format>18))) return 0;
    if ((bitmap->type==1 && (bitmap->width>512 || bitmap->height>512 || bitmap->depth>256)) ||
        (bitmap->type!=1 && bitmap->depth!=1)) return 0;
    if (texture->channels && (texture->channels&(texture->channels-1))) {
        error(_error_silent,"mcc bitmap: tag %08lx has conflicting pixel-channel uses",texture->handle);
        return 0;
    }
    mcc_bitmap_complete_mips(&normalized);
    for (level=0;level<=(unsigned)normalized.mipmap_count;level++) {
        unsigned w=MAX(bitmap->width>>level,1),h=MAX(bitmap->height>>level,1),d=MAX(bitmap->depth>>level,1);
        uint64_t slice=bytes_per_pixel ? (uint64_t)w*h*bytes_per_pixel :
            (uint64_t)((w+3)/4)*((h+3)/4)*(bitmap->format==14 ? 8 : 16);
        raw_size+=slice*d*faces;
        decoded_size+=(uint64_t)w*h*d*faces*4;
    }
    if (raw_size>(uint32_t)bitmap->pixels_size || raw_size>MCC_BITMAP_ALLOCATION_LIMIT ||
        (convert && decoded_size>MCC_BITMAP_ALLOCATION_LIMIT)) return 0;
    if (normalized.flags&0x1000) {
        struct bitmap_group *group=mcc_bitmap_tag(runtime,texture->handle,BITMAP_GROUP_TAG,sizeof(*group));
        if (!group || !mcc_bitmap_lightmap_flags(&normalized,group->usage)) return 0;
    }
    if (convert) {
        normalized.format=11;
        normalized.flags&=~(2|4|8|32|0x100);
    } else normalized.flags&=~(8|0x100);
    /* CEA bit 9 marks environment/lightmap use (Invader's public bitmap
     * schema). It is not a pixel-layout flag and has no Xbox counterpart. */
    normalized.flags&=~0x200;
    normalized.flags=(normalized.flags&~64)|128;
    /* MCC raw flags are not an Xbox hardware descriptor. Derive the
     * compressed/palette bits from the normalized pixel format. */
    normalized.flags&=~(2|4);
    if (normalized.format>=14 && normalized.format<=16) normalized.flags|=2;
    if (normalized.format==17) normalized.flags|=4;
    if (!bitmap_verify(&normalized,FALSE)) return 0;
    if (!(normalized.flags&16) &&
        ((normalized.width&(normalized.width-1)) || (normalized.height&(normalized.height-1)) ||
         (normalized.depth&(normalized.depth-1)))) return 0;
    if ((normalized.flags&16) && (normalized.type!=0 || normalized.mipmap_count || (normalized.flags&2))) return 0;
    hardware_size=(uint32_t)rasterizer_xbox_bitmap_get_pixel_data_size(&normalized);
    /* The generic bitmap verifier includes the tiny DXT mip tail, whereas
     * native hardware stops at a 4-texel dimension. Own enough storage for
     * both contracts, including the 2D tail sampled by CPU object lighting. */
    allocation=(MAX(hardware_size,(uint32_t)(convert ? decoded_size : raw_size))+127u)&~127u;
    if (!hardware_size || allocation>MCC_BITMAP_ALLOCATION_LIMIT || allocation>MCC_BITMAP_LIMIT-textures->end) return 0;
    raw=malloc((size_t)raw_size);
    pixels=malloc(allocation);
    if (pixels) memset(pixels,0,allocation);
    if (!raw || !pixels || !mcc_runtime_read(runtime,bitmap->pixels_offset,(uint32_t)raw_size,raw)) goto failed;
    if (convert) {
        for (level=0;level<=(unsigned)normalized.mipmap_count;level++) {
            unsigned w=MAX(bitmap->width>>level,1),h=MAX(bitmap->height>>level,1),d=MAX(bitmap->depth>>level,1),slice;
            unsigned source_bytes=bytes_per_pixel ? w*h*bytes_per_pixel : ((w+3)/4)*((h+3)/4)*(bitmap->format==14 ? 8 : 16);
            for (slice=0;slice<faces*d;slice++) {
                mcc_bitmap_decode(raw+input_offset,(uint32_t *)(pixels+output_offset),w,h,bitmap->format,texture->channels);
                input_offset+=source_bytes;output_offset+=w*h*4;
            }
        }
    } else memcpy(pixels,raw,(size_t)raw_size);
    free(raw);raw=NULL;
    hardware=malloc(allocation);
    if (hardware) memset(hardware,0,allocation);
    if (!hardware || !mcc_bitmap_pack(&normalized,pixels,(uint32_t)(convert ? decoded_size : raw_size),hardware,
        normalized.type==0 && (normalized.flags&2) ? allocation : hardware_size)) goto failed;
    free(pixels);pixels=hardware;hardware=NULL;
    if (bitmap->mipmap_count!=normalized.mipmap_count)
        error(_error_silent,"mcc bitmap: tag %08lx omitted incomplete DXT1 mip tail (levels %d through %d)",
            texture->handle,normalized.mipmap_count+1,bitmap->mipmap_count);
    bitmap->format=normalized.format;
    bitmap->flags=normalized.flags;
    bitmap->mipmap_count=normalized.mipmap_count;
    bitmap->pixels_offset=MCC_BITMAP_BASE+textures->end;
    bitmap->pixels_size=allocation;
    bitmap->tag_index=texture->handle;
    bitmap->cache_block_index=NONE;
    bitmap->hardware_format=NULL;
    bitmap->base_address=NULL;
    texture->offset=bitmap->pixels_offset;texture->size=allocation;texture->pixels=pixels;
    textures->end+=allocation;
    return 1;
failed:
    if (raw) free(raw);
    if (pixels) free(pixels);
    if (hardware) free(hardware);
    return 0;
}

int mcc_bitmaps_prepare(struct mcc_runtime *runtime)
{
    struct mcc_textures *textures;
    uint32_t index,original_count=runtime->report.tag_count;
    unsigned char *new_index;
    textures=malloc(sizeof(*textures));
    if (!textures) return 0;
    memset(textures,0,sizeof(*textures));
    runtime->bitmaps=textures;
    textures->capacity=runtime->report.bitmap_count*3;
    textures->tag_capacity=MIN(original_count*3,0xFFFFu);
    new_index=mcc_runtime_allocate(runtime,textures->tag_capacity*32);
    if (!new_index) return 0;
    memcpy(new_index,runtime->tag_index,original_count*32);
    runtime->tag_index=new_index;
    textures->clones=malloc(textures->tag_capacity*sizeof(*textures->clones));
    if (!textures->clones) return 0;
    if (runtime->report.bitmap_count) {
        textures->items=malloc(textures->capacity*sizeof(*textures->items));
        if (!textures->items) return 0;
        memset(textures->items,0,textures->capacity*sizeof(*textures->items));
    }
    for (index=0;index<original_count;index++) {
        uint32_t *entry=(uint32_t *)(runtime->tag_index+index*0x20), n;
        struct bitmap_group *group;
        struct bitmap_data *bitmaps;
        if (entry[0]!=BITMAP_GROUP_TAG) continue;
        group=mcc_runtime_pointer(runtime,entry[5],sizeof(*group));
        if (!group || !(bitmaps=mcc_bitmap_block(runtime,&group->bitmaps,sizeof(*bitmaps)))) return 0;
        for (n=0;n<(uint32_t)group->bitmaps.count;n++) {
            struct mcc_texture *texture;
            if (textures->count>=runtime->report.bitmap_count) return 0;
            texture=&textures->items[textures->count++];
            texture->bitmap=&bitmaps[n];texture->handle=entry[3];
        }
    }
    if (!mcc_bitmap_usages(runtime,textures)) {
        error(_error_silent,"mcc bitmaps: invalid bitmap reference in shader or HUD"); return 0;
    }
    for (index=0;index<textures->count;index++) {
        if (!mcc_texture_prepare(runtime,textures,&textures->items[index])) {
            error(_error_silent,"mcc bitmaps: cannot convert texture %lu of tag %08lx; map refused",index,textures->items[index].handle);
            return 0;
        }
    }
    error(_error_silent,"mcc bitmaps: %lu textures converted to %lu bytes",textures->count,textures->end);
    return 1;
}

int mcc_bitmaps_read(struct mcc_runtime *runtime,long tag,uint32_t offset,uint32_t bytes,void *out)
{
    struct mcc_textures *textures=runtime->bitmaps;
    uint32_t index;
    if (offset<MCC_BITMAP_BASE) return -1;
    for (index=0;textures && index<textures->count;index++) {
        struct mcc_texture *texture=&textures->items[index];
        uint32_t position=offset-texture->offset;
        if ((uint32_t)tag==texture->handle && offset>=texture->offset && position<=texture->size && bytes<=texture->size-position) {
            memcpy(out,texture->pixels+position,bytes); return 1;
        }
    }
    return 0;
}

uint32_t mcc_bitmaps_stream_end(struct mcc_runtime *runtime)
{
    struct mcc_textures *textures=runtime->bitmaps;
    return textures && textures->end ? MCC_BITMAP_BASE+textures->end : (uint32_t)runtime->source.size;
}

int mcc_bitmaps_contains(struct mcc_runtime *runtime,uint32_t offset,uint32_t bytes)
{
    struct mcc_textures *textures=runtime->bitmaps;
    uint32_t index;
    for (index=0;textures && index<textures->count;index++) {
        struct mcc_texture const *texture=&textures->items[index];
        if (texture->pixels && offset>=texture->offset && offset-texture->offset<=texture->size &&
            bytes<=texture->size-(offset-texture->offset)) return 1;
    }
    return 0;
}

int mcc_bitmaps_valid(struct mcc_runtime *runtime,struct bitmap_data *bitmap)
{
    struct mcc_textures *textures=runtime->bitmaps;
    uint32_t index;
    for (index=0;textures && index<textures->count;index++) {
        struct mcc_texture const *texture=&textures->items[index];
        if (texture->bitmap!=bitmap) continue;
        return texture->pixels && (uint32_t)bitmap->tag_index==texture->handle &&
            (uint32_t)bitmap->pixels_offset==texture->offset && (uint32_t)bitmap->pixels_size==texture->size &&
            bitmap_verify(bitmap,FALSE) && rasterizer_xbox_bitmap_get_pixel_data_size(bitmap)<=(long)texture->size;
    }
    return 0;
}

/* The renderer may bind the immutable MCC allocation directly. A copied
 * descriptor, another map's bitmap, or a changed stream range cannot borrow
 * this storage. The caller receives bytes only after ownership validation. */
void const *mcc_bitmaps_pixels(struct mcc_runtime *runtime,struct bitmap_data const *bitmap,uint32_t *bytes)
{
    struct mcc_textures *textures=runtime ? runtime->bitmaps : NULL;
    uint32_t index;
    if (bytes) *bytes=0;
    if (!textures || !bitmap || !mcc_bitmaps_valid(runtime,(struct bitmap_data *)bitmap)) return NULL;
    for (index=0;index<textures->count;index++) {
        struct mcc_texture const *texture=&textures->items[index];
        if (texture->bitmap!=bitmap) continue;
        if (bytes) *bytes=texture->size;
        return texture->pixels;
    }
    return NULL;
}

void mcc_bitmaps_dispose(struct mcc_runtime *runtime)
{
    struct mcc_textures *textures=runtime->bitmaps;
    uint32_t index;
    if (!textures) return;
    for (index=0;index<textures->count;index++) {
        struct mcc_texture *texture=&textures->items[index];
        if (texture->bitmap && texture->bitmap->base_address==texture->pixels)
            texture->bitmap->base_address=NULL;
        if (texture->pixels) free(texture->pixels);
    }
    if (textures->items) free(textures->items);
    if (textures->clones) free(textures->clones);
    free(textures);runtime->bitmaps=NULL;
}
