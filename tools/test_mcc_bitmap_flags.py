"""Exercise MCC descriptor conversion with generated pixels, without map assets."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def bitmap_converter(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is needed for MCC bitmap conversion tests")
    bitmap = (ROOT / "port/linux/game/mcc_bitmaps.c").read_text()
    native = (ROOT / "source/bitmaps/bitmaps.c").read_text()
    swizzle = (ROOT / "source/rasterizer/rasterizer_swizzle.c").read_text()
    source = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define TRUE 1
#define FALSE 0
#define NONE -1
#define FLAG(bit) (1u<<(bit))
#define TEST_FLAG(value,bit) ((value)&FLAG(bit))
#define VALID_INDEX(index,count) ((index)>=0 && (index)<(count))
#define NUMBER_OF_BITMAP_FLAGS 9
#define NUMBER_OF_BITMAP_TYPES 3
#define NUMBER_OF_BITMAP_FORMATS 18
#define BITMAP_GROUP_TAG 0x6269746du
#define _bitmap_format_a8r8g8b8 11
#define _bitmap_has_power_of_two_dimensions_bit 0
#define _bitmap_compressed_bit 1
#define _bitmap_palettized_bit 2
#define _bitmap_swizzled_bit 3
#define _bitmap_linear_bit 4
#define _error_silent 0
#define error(...) ((void)0)
#define match_assert(file,line,condition) do {if(!(condition))abort();} while(0)
#define MCC_BITMAP_BASE 0x60000000u
#define MCC_BITMAP_LIMIT 0x1FFFFFFFu
#define MCC_BITMAP_ALLOCATION_LIMIT 0x08000000u
typedef unsigned char boolean;
struct bitmap_data {
    uint32_t signature;
    short width,height,depth,type,format;
    unsigned short flags;
    short mipmap_count;
    int32_t pixels_offset,pixels_size,tag_index,cache_block_index;
    void *hardware_format,*base_address;
};
struct tag_block {long count;void *address;};
struct bitmap_group_sprite {unsigned char data[32];};
struct bitmap_group_sequence {short first_bitmap_index,bitmap_count;struct tag_block sprites;};
struct bitmap_group {short usage;struct {long file_offset;} import_bitmap,pixel_data;struct tag_block bitmaps,sequences;};
struct mcc_runtime {
    struct bitmap_group group;
    unsigned char source[350000];
    uint32_t size,reads,read_fail_at,resource_reads;
    unsigned char *tag_index,*tags;
    uint32_t used;
    struct {uint32_t tag_count,bitmap_count;} report;
    void *bitmaps;
};
static void *mcc_runtime_pointer(struct mcc_runtime *r,uint32_t address,uint32_t size) {
    uintptr_t start=(uintptr_t)r->tags;
    if(address==(uintptr_t)&r->group && size<=sizeof(r->group))return &r->group;
    return r->tags && address>=start && address-start<=r->used && size<=r->used-(address-start) ?
        (void *)(uintptr_t)address : NULL;
}
static void *mcc_runtime_allocate(struct mcc_runtime *r,uint32_t size) {
    uint32_t at=(r->used+15)&~15u;
    if(size>65536-at)return NULL;
    r->used=at+size;return r->tags+at;
}
static int mcc_resources_read(struct mcc_runtime *r,unsigned type,char const *name,uint32_t offset,uint32_t bytes,void *out,char const *group) {
    (void)type;(void)name;(void)offset;(void)bytes;(void)out;(void)group;
    r->resource_reads++;return 0;
}
static short floor_log2(unsigned value) {short n=0;while(value>1){value>>=1;n++;}return n;}
static int bitmap_format_type_valid_width(short f,short t,short v) {(void)f;(void)t;return v>0 && v<=30000;}
static int bitmap_format_type_valid_height(short f,short t,short v) {return bitmap_format_type_valid_width(f,t,v);}
static int bitmap_format_type_valid_depth(short f,short t,short v) {(void)f;(void)t;return v==1;}
static int mcc_runtime_read(struct mcc_runtime *r,uint32_t offset,uint32_t size,void *out) {
    r->reads++;
    if(r->reads==r->read_fail_at || offset>r->size || size>r->size-offset)return 0;
    memcpy(out,r->source+offset,size);return 1;
}
static int test_hardware_calls,test_hardware_fail_at;
static long rasterizer_xbox_bitmap_get_pixel_data_size(struct bitmap_data *b) {
    uint32_t bytes=0;unsigned level;
    if(++test_hardware_calls==test_hardware_fail_at)return 64;
    if(b->flags&16)return (((b->width*(b->format==11 ? 4 : 2)+63)&~63u)*b->height+127u)&~127u;
    for(level=0;level<=(unsigned)b->mipmap_count;level++) {
        unsigned w=MAX(b->width>>level,1),h=MAX(b->height>>level,1);
        bytes+=b->format==14 ? ((w+3)/4)*((h+3)/4)*8 : w*h*(b->format==11 ? 4 : 2);
    }
    return (bytes+127u)&~127u;
}
static uint32_t global_vector_palette[256];
#define BCDEC_STATIC
#define BCDEC_IMPLEMENTATION
#include "bcdec.h"
static int test_allocations,test_fail_at,test_live;
static void *test_bitmap_malloc(size_t n) {
    void *p;
    if (++test_allocations==test_fail_at) return NULL;
    p=malloc(n);if(p)test_live++;return p;
}
static void test_bitmap_free(void *p) {if(!p)abort();test_live--;free(p);}
#define malloc test_bitmap_malloc
#define free test_bitmap_free
'''
    source += "\n" + next(line for line in bitmap.splitlines() if line.startswith("#define MCC_BITMAP_STORAGE_LIMIT "))
    source += "\n" + structure(bitmap, "mcc_texture")
    source += "\n" + structure(bitmap, "mcc_bitmap_clone")
    source += "\n" + structure(bitmap, "mcc_textures")
    source += "\n" + "\n".join(function(bitmap, name) for name in [
        "mcc_bitmap_tag", "mcc_bitmap_block", "mcc_bitmap_clone_block", "mcc_bitmap_specialize"])
    source += r'''
static int test_reverse,test_empty;
/* Consumer selection is outside this test. Actual clone construction must
 * append complete groups, and reuse must leave the constructed array alone. */
static int mcc_bitmap_usages(struct mcc_runtime *r,struct mcc_textures *t) {
    uint32_t a=0xE1110001u,b=0xE2220003u,empty=0xE3330000u,repeated=a;
    uint32_t count,tags,target;
    assert(t->count==(test_empty ? 0u : 5u));
    if(!test_empty) {
        assert(t->items[0].handle==a && t->items[1].handle==a && t->items[2].handle==a);
        assert(t->items[3].handle==b && t->items[4].handle==b);
    }
    if(test_reverse) {
        if(!mcc_bitmap_specialize(r,t,&b,4) || !mcc_bitmap_specialize(r,t,&a,2))return 0;
    } else if(!mcc_bitmap_specialize(r,t,&a,2) || !mcc_bitmap_specialize(r,t,&b,4))return 0;
    assert(a!=repeated && b!=0xE2220003u && a!=b);
    count=t->count;tags=r->report.tag_count;target=a;
    if(!mcc_bitmap_specialize(r,t,&repeated,2))return 0;
    assert(repeated==target && t->count==count && r->report.tag_count==tags);
    if(!mcc_bitmap_specialize(r,t,&empty,2))return 0;
    assert(t->count==count && r->report.tag_count==tags+1);
    assert(t->clone_count==3 && t->count==(test_empty ? 0u : 10u));
    return 1;
}
'''
    source += "\n" + function(native, "bitmap_verify")
    source += "\n" + function(swizzle, "rasterizer_xbox_bitmap_get_max_mipmap_count")
    source += "\n" + "\n".join(function(bitmap, name) for name in [
        "mcc_bitmap_bytes", "mcc_pixel", "mcc_channels", "mcc_bitmap_decode",
        "mcc_morton_axis", "mcc_bitmap_pack", "mcc_bitmap_lightmap_flags", "mcc_bitmap_complete_mips",
        "mcc_texture_prepare", "mcc_bitmaps_prepare", "mcc_bitmaps_read",
        "mcc_bitmap_valid_texture", "mcc_bitmaps_valid", "mcc_bitmaps_group_valid", "mcc_bitmaps_dispose"])
    source += r'''
#undef malloc
#undef free
static int test_construction(char const *mode) {
    static short const widths[5]={4,16,4,8,4},heights[5]={2,8,2,8,4};
    struct mcc_runtime r={0};
    struct bitmap_data originals[5],*descriptors[5];
    struct mcc_textures *t;
    uint32_t index,n,k=0,input=0,successful=0,used=0,stream_end=0;
    unsigned char out[512];
    int expected=!strcmp(mode,"construct") || !strcmp(mode,"construct-reverse") || !strcmp(mode,"construct-empty");
    test_reverse=!strcmp(mode,"construct-reverse");test_empty=!strcmp(mode,"construct-empty");
    r.tags=calloc(1,65536);assert(r.tags);
    r.report.tag_count=4;r.report.bitmap_count=test_empty ? 0 : 5;
    r.tag_index=mcc_runtime_allocate(&r,4*32);r.size=4096;
    for(index=0;index<r.size;index++)r.source[index]=(unsigned char)(index*13+7);
    for(index=0;index<4;index++) {
        uint32_t *entry=(uint32_t *)(r.tag_index+index*32);
        struct bitmap_group *group;
        struct bitmap_data *bitmaps;
        uint32_t count=test_empty || index==0 || index==2 ? 0 : index==1 ? 3 : 2;
        entry[0]=index==2 ? 0x74657374u : BITMAP_GROUP_TAG;
        entry[3]=(index==1 ? 0xE1110000u : index==3 ? 0xE2220000u : 0xE3330000u)|index;
        entry[4]=(uint32_t)mcc_runtime_allocate(&r,256);
        strcpy((char *)(uintptr_t)entry[4],"bitmap_group");
        group=mcc_runtime_allocate(&r,sizeof(*group));entry[5]=(uint32_t)group;
        bitmaps=mcc_runtime_allocate(&r,count*sizeof(*bitmaps));
        group->bitmaps.count=count;group->bitmaps.address=bitmaps;
        for(n=0;n<count;n++,k++) {
            struct bitmap_data *b=&bitmaps[n];
            b->signature=BITMAP_GROUP_TAG;b->width=widths[k];b->height=heights[k];b->depth=1;
            b->format=6;b->flags=0x81;b->pixels_offset=input;b->pixels_size=b->width*b->height*2;
            b->tag_index=entry[3];input+=b->pixels_size;
            if(!strcmp(mode,"construct-resource") && k==2)b->flags|=0x100;
            memcpy(&originals[k],b,sizeof(*b));descriptors[k]=b;
        }
    }
    if(!strcmp(mode,"construct-read"))r.read_fail_at=3;
    if(!strcmp(mode,"construct-pack"))test_hardware_fail_at=3;
    if(!strcmp(mode,"construct-allocation"))test_fail_at=8;
    assert(mcc_bitmaps_prepare(&r)==expected);
    t=r.bitmaps;assert(t && t->count==(test_empty ? 0u : 10u));
    for(index=0;index<t->count;index++) {
        struct mcc_texture const *item=&t->items[index];
        uint32_t original=(item->resource_handle==0xE1110001u ? 0 : 3)+item->resource_index;
        uint32_t offset,size,hash=2166136261u;
        if(item->handle==item->resource_handle)assert(item->bitmap==descriptors[original]);
        else assert(item->bitmap!=descriptors[original]);
        /* Each complete runtime-handle run occurs once, even though clones
         * retain their original resource handle for external resource reads. */
        if(index && item->handle!=t->items[index-1].handle)
            for(n=0;n<index;n++)assert(t->items[n].handle!=item->handle);
        if(!item->pixels) {
            assert(!memcmp(item->bitmap,&originals[original],sizeof(originals[original])));
            assert(!item->offset && !item->size);continue;
        }
        successful++;
        if(item->resource_handle==0xE1110001u)
            offset=item->resource_index==0 ? 0 : item->resource_index==1 ? 128 : item->channels ? 640 : 384;
        else offset=item->resource_index==0 ? 0 : item->channels ? 256 : 128;
        size=(widths[original]*heights[original]*(item->channels ? 4 : 2)+127u)&~127u;
        assert(item->offset==MCC_BITMAP_BASE+offset && item->size==size);
        assert(item->bitmap->pixels_offset==(int32_t)item->offset && item->bitmap->pixels_size==(int32_t)size);
        assert((uint32_t)item->bitmap->tag_index==item->handle && !item->bitmap->base_address && !item->bitmap->hardware_format);
        assert(mcc_bitmaps_read(&r,item->handle,item->offset,size,out)==1 && !memcmp(out,item->pixels,size));
        assert(mcc_bitmaps_read(&r,0xE999000Fu,item->offset,1,out)==0);
        for(n=0;n<size;n++)hash=(hash^item->pixels[n])*16777619u;
        printf("%08x %08x %u %u %u %u %u %u %08x\n",item->handle,item->resource_handle,
            item->resource_index,item->channels,item->bitmap->flags,item->bitmap->format,item->offset,size,hash);
        used+=size;stream_end=MAX(stream_end,offset+size);
    }
    assert(successful==(test_empty ? 0u : expected ? 10u : 2u));
    assert(t->end==used && t->stream_end==stream_end);
    assert(t->end==(test_empty ? 0u : expected ? 1920u : 384u));
    assert(t->stream_end==(test_empty ? 0u : expected ? 768u : 384u));
    assert(r.resource_reads==(!strcmp(mode,"construct-resource") ? 1u : 0u));
    assert(r.reads==(test_empty ? 0u : expected ? 10u :
        !strcmp(mode,"construct-read") || !strcmp(mode,"construct-pack") ? 3u : 2u));
    printf("end %u %u reads %u resources %u\n",t->end,t->stream_end,r.reads,r.resource_reads);
    mcc_bitmaps_dispose(&r);assert(!r.bitmaps && !test_live);
    free(r.tags);return 0;
}
int main(int argc,char **argv) {
    struct bitmap_data bitmap={0},original;
    struct mcc_textures textures={0};
    struct mcc_texture texture={0};
    struct mcc_runtime runtime={0};
    uint32_t tag_index[8*8]={0};
    unsigned i;int expect=1,converted,tail=0;
    static unsigned const order[8]={0,1,4,5,2,3,6,7};
    if(argc!=2)return 2;
    if(!strncmp(argv[1],"construct",9))return test_construction(argv[1]);
    runtime.group.usage=4;runtime.size=16;
    runtime.tag_index=(unsigned char *)tag_index;runtime.report.tag_count=8;
    tag_index[7*8]=BITMAP_GROUP_TAG;tag_index[7*8+3]=7;tag_index[7*8+5]=(uint32_t)&runtime.group;
    bitmap.signature=BITMAP_GROUP_TAG;bitmap.width=4;bitmap.height=2;bitmap.depth=1;
    bitmap.format=6;bitmap.flags=0x1281;bitmap.pixels_size=16;
    texture.bitmap=&bitmap;texture.handle=7;
    for(i=0;i<16;i++)runtime.source[i]=(unsigned char)i;
    if(!strncmp(argv[1],"stream-",7)) {
        uint32_t stream_offset=!strcmp(argv[1],"stream-last") ? MCC_BITMAP_LIMIT-255 :
            !strcmp(argv[1],"stream-full") ? MCC_BITMAP_LIMIT-127 :
            !strcmp(argv[1],"stream-limit") ? MCC_BITMAP_LIMIT : UINT32_MAX;
        int accept=!strcmp(argv[1],"stream-last");
        textures.end=128;textures.stream_end=stream_offset;
        memcpy(&original,&bitmap,sizeof(original));
        converted=mcc_texture_prepare(&runtime,&textures,&texture,stream_offset);
        assert(converted==accept);
        if(accept) {
            assert(texture.offset==MCC_BITMAP_BASE+stream_offset && texture.size==128);
            assert(textures.end==256 && textures.stream_end==stream_offset+128 && runtime.reads==1);
        } else {
            assert(!runtime.reads && !test_allocations && !texture.pixels && !texture.offset && !texture.size);
            assert(!memcmp(&bitmap,&original,sizeof(original)) && textures.end==128 && textures.stream_end==stream_offset);
        }
        free(texture.pixels);return 0;
    }
    if(!strncmp(argv[1],"group-",6)) {
        struct bitmap_group_sequence sequence={0};
        sequence.bitmap_count=1;
        if(!mcc_texture_prepare(&runtime,&textures,&texture,0))return 31;
        textures.count=1;textures.items=&texture;runtime.bitmaps=&textures;
        runtime.group.bitmaps.count=1;runtime.group.bitmaps.address=&bitmap;
        runtime.group.sequences.count=1;runtime.group.sequences.address=&sequence;
        if(!strcmp(argv[1],"group-large")) {
            bitmap.width=bitmap.height=4096;bitmap.pixels_size=texture.size=89478528;
        }
        if(!strcmp(argv[1],"group-unowned")) {texture.bitmap=NULL;expect=0;}
        if(!strcmp(argv[1],"group-offset")) {runtime.group.pixel_data.file_offset=1;expect=0;}
        if(!strcmp(argv[1],"group-sequence")) sequence.bitmap_count=2;
        if(mcc_bitmaps_group_valid(&runtime,&runtime.group)!=expect)return 32;
        if(!strcmp(argv[1],"group-sequence") && sequence.bitmap_count)return 33;
        free(texture.pixels);return 0;
    }
    if(!strncmp(argv[1],"allocation-",11)) {
        test_fail_at=atoi(argv[1]+11);texture.channels=2;
        if(mcc_texture_prepare(&runtime,&textures,&texture,0) || test_live || texture.pixels || textures.end)return 30;
        return 0;
    }
    if(!strncmp(argv[1],"storage-",8)) {
        int accept=!strcmp(argv[1],"storage-large");
        textures.end=accept ? 0x40000000u : !strcmp(argv[1],"storage-full") ? MCC_BITMAP_STORAGE_LIMIT-64 : 0xffffffffu;
        converted=mcc_texture_prepare(&runtime,&textures,&texture,0);
        if(converted!=accept || (!accept && runtime.reads))return 28;
        if(accept && (textures.end!=0x40000080u || memcmp(texture.pixels,runtime.source,4)))return 29;
        free(texture.pixels);return 0;
    }
    if(!strcmp(argv[1],"swizzle8192")) {
        bitmap.width=8192;bitmap.height=2;bitmap.format=6;bitmap.flags=0x81;
        bitmap.pixels_size=32768;runtime.size=32768;runtime.group.usage=0;
        for(i=0;i<16384;i++) {runtime.source[i*2]=(unsigned char)i;runtime.source[i*2+1]=(unsigned char)(i>>8);}
        if(!mcc_texture_prepare(&runtime,&textures,&texture,0) || bitmap.pixels_size!=32768)return 26;
        if(memcmp(texture.pixels+4,runtime.source+16384,2) ||
            memcmp(texture.pixels+16384,runtime.source+8192,2) ||
            memcmp(texture.pixels+32766,runtime.source+32766,2))return 27;
        free(texture.pixels);return 0;
    }
    if(!strncmp(argv[1],"wide",4)) {
        bitmap.width=15360;bitmap.height=4;bitmap.format=14;bitmap.flags=0x83;
        bitmap.pixels_size=30720;runtime.size=30720;runtime.group.usage=1;
        if(!strcmp(argv[1],"wide-short")) {bitmap.pixels_size--;return mcc_texture_prepare(&runtime,&textures,&texture,0)?24:0;}
        if(!strcmp(argv[1],"wide-limit")) {bitmap.width=16385;return mcc_texture_prepare(&runtime,&textures,&texture,0)?25:0;}
        if(!mcc_texture_prepare(&runtime,&textures,&texture,0) || bitmap.format!=11 ||
            !(bitmap.flags&16) || bitmap.width!=15360 || bitmap.pixels_size!=245760)return 22;
        if(texture.pixels[245759]!=255)return 23;
        free(texture.pixels);return 0;
    }
    if(!strcmp(argv[1],"tag-streams")) {
        struct mcc_texture items[3]={{0}};
        struct bitmap_data bitmaps[3];unsigned char out[16];
        bitmap.flags=0x81;textures.end=0x20000000u;
        textures.items=items;textures.count=3;runtime.bitmaps=&textures;
        for(i=0;i<3;i++) {
            bitmaps[i]=bitmap;items[i].bitmap=&bitmaps[i];items[i].handle=i==2 ? 8 : 7;
            if(!mcc_texture_prepare(&runtime,&textures,&items[i],i==1 ? 128 : 0))return 12;
        }
        if(items[0].offset!=MCC_BITMAP_BASE || items[1].offset!=MCC_BITMAP_BASE+128 ||
            items[2].offset!=MCC_BITMAP_BASE || textures.end!=0x20000180u)return 13;
        if(mcc_bitmaps_read(&runtime,7,MCC_BITMAP_BASE+128,16,out)!=1 ||
            mcc_bitmaps_read(&runtime,8,MCC_BITMAP_BASE,16,out)!=1 ||
            mcc_bitmaps_read(&runtime,8,MCC_BITMAP_BASE+128,1,out)!=0 ||
            mcc_bitmaps_read(&runtime,9,MCC_BITMAP_BASE,1,out)!=0)return 14;
        for(i=0;i<3;i++)free(items[i].pixels);
        return 0;
    }
    if(!strncmp(argv[1],"tail",4)) {
        tail=1;runtime.group.usage=0;runtime.size=349525;
        bitmap.width=1024;bitmap.height=512;bitmap.format=14;bitmap.flags=0x81;
        bitmap.mipmap_count=10;bitmap.pixels_size=349525;
        for(i=0;i<runtime.size;i++)runtime.source[i]=(unsigned char)i;
        if(!strcmp(argv[1],"tail-known")){}
        else if(!strcmp(argv[1],"tail-random-short")) {bitmap.pixels_size--;expect=0;}
        else if(!strcmp(argv[1],"tail-short-base")) {bitmap.pixels_size=262143;expect=0;}
        else if(!strcmp(argv[1],"tail-wrong-flags")) {bitmap.flags=0x83;expect=0;}
        else if(!strcmp(argv[1],"tail-wrong-format")) {bitmap.format=15;expect=0;}
        else return 3;
    }
    else if(!strncmp(argv[1],"npot",4)) {
        bitmap.width=6;bitmap.height=3;bitmap.format=3;bitmap.flags=0x81;bitmap.mipmap_count=1;
        bitmap.pixels_size=42;runtime.size=42;runtime.group.usage=1;
        if(!strcmp(argv[1],"npot-short")){bitmap.pixels_size=41;expect=0;}
    }
    else if(!strcmp(argv[1],"known")){}
    else if(!strcmp(argv[1],"ordinary")) {bitmap.flags=0x81;runtime.group.usage=0;}
    else if(!strcmp(argv[1],"wrong-group")) {runtime.group.usage=0;expect=0;}
    else if(!strcmp(argv[1],"missing-group")) {texture.handle=8;expect=0;}
    else if(!strcmp(argv[1],"missing-environment")) {bitmap.flags&=~0x200;expect=0;}
    else if(!strcmp(argv[1],"unknown-flag")) {bitmap.flags|=0x400;expect=0;}
    else if(!strcmp(argv[1],"extra-pixels")) {bitmap.pixels_size=17;expect=0;}
    else if(!strcmp(argv[1],"short-pixels")) {bitmap.pixels_size=15;expect=0;}
    else if(!strcmp(argv[1],"bad-signature")) {bitmap.signature=0;expect=0;}
    else if(!strcmp(argv[1],"wrong-format")) {bitmap.format=8;expect=0;}
    else if(!strcmp(argv[1],"non-power-two")) {bitmap.width=3;bitmap.pixels_size=12;expect=0;}
    else if(!strcmp(argv[1],"mipmaps")) {bitmap.mipmap_count=1;bitmap.pixels_size=20;expect=0;}
    else return 3;
    original=bitmap;
    converted=mcc_texture_prepare(&runtime,&textures,&texture,0);
    if(converted!=expect)return 4;
    if(!expect) {
        if(runtime.reads || memcmp(&bitmap,&original,sizeof(bitmap)))return 5;
        return 0;
    }
    if(!strncmp(argv[1],"npot",4)) {
        if(bitmap.width!=6 || bitmap.height!=3 || bitmap.format!=11 || bitmap.mipmap_count ||
            bitmap.flags!=0x90 || bitmap.pixels_size!=256 || runtime.reads!=1)return 20;
        for(i=0;i<18;i++) {
            unsigned char const *p=texture.pixels+(i/6)*64+(i%6)*4;
            if(p[0]!=runtime.source[i*2] || p[1]!=p[0] || p[2]!=p[0] || p[3]!=runtime.source[i*2+1])return 21;
        }
        free(texture.pixels);return 0;
    }
    if(tail) {
        if(bitmap.flags!=0x83 || bitmap.format!=14 || bitmap.mipmap_count!=7 ||
            bitmap.pixels_size!=349568 || runtime.reads!=1 || memcmp(texture.pixels,runtime.source,349520))return 10;
        for(i=349520;i<(unsigned)bitmap.pixels_size;i++)if(texture.pixels[i])return 11;
        free(texture.pixels);return 0;
    }
    if(bitmap.flags!=0x89 || bitmap.format!=6 || bitmap.width!=4 || bitmap.height!=2 ||
        bitmap.pixels_size!=128 || textures.end!=128 || runtime.reads!=1 ||
        (uint32_t)bitmap.pixels_offset!=MCC_BITMAP_BASE || bitmap.tag_index!=7)return 6;
    for(i=0;i<8;i++)if(memcmp(texture.pixels+i*2,runtime.source+order[i]*2,2))return 7;
    for(i=16;i<128;i++)if(texture.pixels[i])return 8;
    if(!bitmap_verify(&bitmap,FALSE))return 9;
    free(texture.pixels);return 0;
}
'''
    work = tmp_path_factory.mktemp("mcc-bitmap-converter")
    path = work / "bitmap.c"
    path.write_text(source)
    output = work / ("bitmap.exe" if sys.platform == "win32" else "bitmap")
    command = [compiler, "-include", str(ROOT / "port/linux/game/mcc_loading.h"), str(ROOT / "tools/mcc_loading_stub.c"), "-std=c99", "-O1", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
               "-I", str(ROOT / "port/third_party/bcdec"), str(path), "-o", str(output)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    else:
        command += ["-m32", "-fsanitize=undefined", "-fsanitize-trap=undefined"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return output


@pytest.mark.parametrize("case", ["known", "ordinary", "wrong-group", "missing-group", "missing-environment",
                                  "unknown-flag", "extra-pixels", "short-pixels", "bad-signature",
                                  "group-large", "group-unowned", "group-offset", "group-sequence",
                                  "wrong-format", "non-power-two", "mipmaps", "tag-streams", "npot", "npot-short", "wide", "wide-short", "wide-limit", "swizzle8192", "storage-large", "storage-full", "storage-overflow", "allocation-1", "allocation-2", "allocation-3"])
def test_mcc_lightmap_conversion(bitmap_converter, case):
    result = subprocess.run([str(bitmap_converter), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("case", ["tail-known", "tail-random-short", "tail-short-base",
                                  "tail-wrong-flags", "tail-wrong-format"])
def test_mcc_incomplete_dxt_tail(bitmap_converter, case):
    result = subprocess.run([str(bitmap_converter), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("case", ["construct", "construct-reverse", "construct-empty", "construct-read",
                                  "construct-resource", "construct-pack", "construct-allocation",
                                  "stream-last", "stream-full", "stream-limit", "stream-overflow"])
def test_mcc_bitmap_stream_construction(bitmap_converter, case):
    result = subprocess.run([str(bitmap_converter), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
