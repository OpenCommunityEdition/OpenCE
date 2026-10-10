"""Asset-free codec vectors and stream ownership tests for MCC-only media.

Compile the actual private routines, not copies. Pixel vectors check channel
ordering, partial blocks and BC7 mode 6. PCM tests independently decode the
serialized Xbox ADPCM blocks in Python, including stereo interleave.
"""
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="session")
def media_tool(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is needed for the MCC media tests")
    work = tmp_path_factory.mktemp("mcc-media")
    audio = (ROOT / "port/linux/game/mcc_audio.c").read_text()
    bitmap = (ROOT / "port/linux/game/mcc_bitmaps.c").read_text()
    native_bitmap = (ROOT / "source/bitmaps/bitmaps.c").read_text()
    native_swizzle = (ROOT / "source/rasterizer/rasterizer_swizzle.c").read_text()
    hud = (ROOT / "port/linux/game/mcc_hud.c").read_text()
    declarations = "\n".join(structure(audio, name) for name in ["mcc_audio_storage", "mcc_pcm", "mcc_ima"])
    tables = "\n".join(re.search(r"static int const " + name + r"\[.*?\};", audio, re.S).group()
                       for name in ["mcc_ima_steps", "mcc_ima_changes"])
    functions = "\n".join(function(audio, name) for name in ["mcc_ima_advance", "mcc_ima_quantize",
        "mcc_pcm_sample", "mcc_resampled", "mcc_audio_store", "mcc_audio_contains"])
    functions += "\n" + function(native_swizzle, "rasterizer_xbox_bitmap_get_max_mipmap_count")
    functions += "\n" + function(native_bitmap, "bitmap_2d_address")
    functions += "\n" + "\n".join(function(bitmap, name) for name in ["mcc_bitmap_bytes", "mcc_pixel",
        "mcc_channels", "mcc_bitmap_decode", "mcc_morton_axis", "mcc_bitmap_pack"])
    functions += "\n" + function(hud, "mcc_hud_offset")
    functions += "\n" + function(hud, "mcc_hud_scale")
    functions += "\n" + function(hud, "mcc_hud_score_text")
    source = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define MCC_AUDIO_BASE 0x50000000u
#define MCC_AUDIO_LIMIT 0x10000000u
#define MCC_AUDIO_FRAME_LIMIT 0x1000000u
struct sound_permutation {struct {uint32_t file_offset,size;} samples; uint32_t sample_buffer_size; int compression;};
struct mcc_runtime {void *audio;};
struct hud_placement_definition {struct {short x,y;} offset;struct {float i,j;} scale; short multiplayer_scaling_flags;};
struct bitmap_data {short type,width,height,depth,format,mipmap_count;unsigned short flags;void *base_address;};
typedef unsigned char byte;
#define FALSE 0
#define _bitmap_type_2d 0
#define _bitmap_has_power_of_two_dimensions_bit 0
#define _bitmap_compressed_bit 1
#define _bitmap_swizzled_bit 3
#define _bitmap_linear_bit 4
#define TEST_FLAG(value,bit) ((value)&(1u<<(bit)))
#define match_assert(file,line,condition) do {if(!(condition))abort();} while(0)
#define match_vassert(file,line,condition,description) match_assert(file,line,condition)
static int bitmap_verify(struct bitmap_data *bitmap,int pixels) {(void)bitmap;(void)pixels;return 1;}
static short floor_log2(unsigned value) {short result=0;while(value>1){value>>=1;result++;}return result;}
static long bitmap_format_get_bits_per_pixel(short format) {return format==14 ? 4 : 8;}
uint32_t global_vector_palette[256];
#define BCDEC_STATIC
#define BCDEC_IMPLEMENTATION
#include "bcdec.h"
''' + declarations + "\n" + tables + "\n" + functions + r'''
int main(int argc,char **argv) {
    if (argc<2) return 2;
    if (!strcmp(argv[1],"audio")) {
        int channels=atoi(argv[2]), input_rate=atoi(argv[3]), output_rate=atoi(argv[4]);
        struct mcc_audio_storage output={0};
        struct sound_permutation permutation={0};
        struct mcc_pcm pcm;
        uint32_t i;
        pcm.frames=4096;pcm.channels=channels;pcm.rate=input_rate;
        pcm.samples=malloc(pcm.frames*channels*sizeof(short));
        for (i=0;i<pcm.frames;i++) {
            pcm.samples[i*channels]=(short)(sin(i*0.035)*14000);
            if(channels==2) pcm.samples[i*2+1]=(short)(cos(i*0.051)*9000);
        }
        if (!mcc_audio_store(&output,&pcm,channels,output_rate,&permutation)) return 3;
        {FILE *file=fopen(argv[5],"wb");if(!file)return 4;fwrite(output.bytes,1,output.used,file);fclose(file);}
        free(output.bytes);free(pcm.samples);return 0;
    }
    if (!strcmp(argv[1],"contains")) {
        struct mcc_audio_storage storage={0};
        struct mcc_runtime runtime;
        storage.used=72;runtime.audio=&storage;
        if (!mcc_audio_contains(&runtime,MCC_AUDIO_BASE,72) ||
            !mcc_audio_contains(&runtime,MCC_AUDIO_BASE+36,36) ||
            mcc_audio_contains(&runtime,MCC_AUDIO_BASE-1,1) ||
            mcc_audio_contains(&runtime,MCC_AUDIO_BASE+36,37) ||
            mcc_audio_contains(&runtime,0xFFFFFFFFu,2)) return 5;
        return 0;
    }
    if (!strcmp(argv[1],"hud")) {
        struct hud_placement_definition p={{20,-12},{2.0f,4.0f},4};
        mcc_hud_scale(&p,1);
        if(p.scale.i!=0.5f || p.scale.j!=1.0f || p.multiplayer_scaling_flags ||
            p.offset.x!=10 || p.offset.y!=-6) return 9;
        p.scale.i=2.0f;p.scale.j=4.0f;p.multiplayer_scaling_flags=4;
        mcc_hud_scale(&p,0);
        if(p.scale.i!=1.0f || p.scale.j!=2.0f || p.multiplayer_scaling_flags) return 10;
        p.scale.i=2.0f;p.scale.j=4.0f;p.multiplayer_scaling_flags=7;
        mcc_hud_scale(&p,0);
        if(p.scale.i!=1.0f || p.scale.j!=2.0f || p.multiplayer_scaling_flags!=3) return 11;
        return 0;
    }
    if (!strcmp(argv[1],"score-hint")) {
        uint16_t source[]={'H','o','l','d',' ','"','%','s','"',' ','f','o','r',' ','s','c','o','r','e',0};
        uint16_t out[80];unsigned i;int count;
        char const *expected="Hold \"Tab / BACK\" for score";
        for(i=0;i<80;i++)out[i]=0xABCD;
        count=mcc_hud_score_text(source,20," Tab , F1",NULL,0);
        if(count!=(int)strlen(expected)+1)return 24;
        if(mcc_hud_score_text(source,20," Tab , F1",out,count-1)!=-1 || out[0]!=0xABCD)return 25;
        if(mcc_hud_score_text(source,20," Tab , F1",out,count)!=count || out[count]!=0xABCD)return 26;
        for(i=0;i<(unsigned)count;i++)if(out[i]!=(unsigned char)expected[i])return 27;
        if(source[6]!='%' || source[7]!='s' || mcc_hud_score_text(out,count,"X",NULL,0))return 28;
        if(mcc_hud_score_text(source,19,"Tab",NULL,0)!=-1 || mcc_hud_score_text(NULL,20,"Tab",NULL,0)!=-1)return 29;
        if(mcc_hud_score_text(source,20,"",out,80)!=22 || out[6]!='B' || out[9]!='K' || out[10]!='"')return 30;
        source[0]=0x00D6;
        if(mcc_hud_score_text(source,20,"Mouse 4",out,80)<=0 || out[0]!=0x00D6)return 31;
        return 0;
    }
    if (!strcmp(argv[1],"pack")) {
        struct bitmap_data bitmap={0};unsigned char input[48],packed[770];unsigned i,f;
        static unsigned const order[16]={0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15};
        static unsigned const faces[6]={0,2,1,3,4,5};
        bitmap.width=4;bitmap.height=4;bitmap.depth=1;bitmap.format=1;
        for(i=0;i<48;i++)input[i]=(unsigned char)i;
        memset(packed,0xDD,sizeof(packed));
        if(!mcc_bitmap_pack(&bitmap,input,16,packed+1,128) || !(bitmap.flags&8))return 12;
        for(i=0;i<16;i++)if(packed[1+i]!=order[i])return 13;
        if(packed[0]!=0xDD || packed[129]!=0xDD)return 14;
        bitmap.width=3;bitmap.height=2;bitmap.flags=16;
        if(!mcc_bitmap_pack(&bitmap,input,6,packed+1,128))return 15;
        for(i=0;i<3;i++)if(packed[1+i]!=i || packed[65+i]!=i+3)return 16;
        for(i=4;i<65;i++)if(packed[i])return 17;
        bitmap.type=2;bitmap.width=bitmap.height=2;bitmap.flags=1;bitmap.mipmap_count=1;
        for(f=0;f<6;f++){memset(input+f*4,10+f,4);input[24+f]=(unsigned char)(20+f);}
        if(!mcc_bitmap_pack(&bitmap,input,30,packed+1,768))return 18;
        for(f=0;f<6;f++) {
            for(i=0;i<4;i++)if(packed[1+f*128+i]!=10+faces[f])return 19;
            if(packed[1+f*128+4]!=20+faces[f])return 20;
            for(i=5;i<128;i++)if(packed[1+f*128+i])return 21;
        }
        if(packed[0]!=0xDD || packed[769]!=0xDD)return 22;
        if(mcc_bitmap_pack(&bitmap,input,29,packed+1,768))return 23;
        return 0;
    }
    if (!strcmp(argv[1],"compressed-tail")) {
        struct bitmap_data bitmap={0};
        unsigned char input[144],packed[258];unsigned format,i,level,offset;
        bitmap.type=0;bitmap.width=16;bitmap.height=4;bitmap.depth=1;bitmap.mipmap_count=4;
        for(format=14;format<=16;format++) {
            unsigned block=format==14 ? 8 : 16,source_bytes=block*9,allocation=format==14 ? 128 : 256;
            bitmap.format=(short)format;bitmap.flags=3;bitmap.base_address=packed+1;
            if(rasterizer_xbox_bitmap_get_max_mipmap_count(&bitmap)!=2)return 32;
            for(i=0;i<source_bytes;i++)input[i]=(unsigned char)(i+1);
            memset(packed,0xDD,sizeof(packed));
            if(!mcc_bitmap_pack(&bitmap,input,source_bytes,packed+1,allocation))return 33;
            offset=0;
            for(level=0;level<=4;level++) {
                unsigned size=MAX((16u>>level)/4,1)*block;
                unsigned char *address=bitmap_2d_address(&bitmap,0,0,(short)level);
                if(address!=packed+1+offset || memcmp(address,input+offset,size))return 34;
                offset+=size;
            }
            if(offset!=source_bytes || packed[0]!=0xDD || packed[allocation+1]!=0xDD)return 35;
            for(i=source_bytes;i<allocation;i++)if(packed[1+i])return 36;
            if(mcc_bitmap_pack(&bitmap,input,source_bytes-1,packed+1,allocation))return 37;
            if(format!=14 && mcc_bitmap_pack(&bitmap,input,source_bytes,packed+1,128))return 38;
            /* Keeping CPU mips must not expand the GPU descriptor's count. */
            if(rasterizer_xbox_bitmap_get_max_mipmap_count(&bitmap)!=2)return 39;
        }
        return 0;
    }
    if (!strcmp(argv[1],"pixels")) {
        unsigned char source[16];
        uint32_t pixels[18];
        unsigned i;
        unsigned format=(unsigned)atoi(argv[2]), usage=(unsigned)atoi(argv[3]), side=(unsigned)atoi(argv[4]);
        FILE *file=fopen(argv[5],"rb");if(!file)return 6;
        memset(source,0,sizeof(source));fread(source,1,sizeof(source),file);fclose(file);
        for(i=0;i<18;i++)pixels[i]=0xF00DBAAD;
        mcc_bitmap_decode(source,pixels+1,side,side,(short)format,usage);
        if(pixels[0]!=0xF00DBAAD || pixels[side*side+1]!=0xF00DBAAD)return 7;
        for(i=0;i<side*side;i++)printf("%08x\n",pixels[i+1]);
        return 0;
    }
    return 8;
}
'''
    path = work / "media.c"
    path.write_text(source)
    output = work / ("media.exe" if sys.platform == "win32" else "media")
    command = [clang, "-std=gnu99", "-O2", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
               "-I", str(ROOT / "port/third_party/bcdec"), str(path), "-o", str(output)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    else:
        command += ["-lm", "-fsanitize=undefined", "-fsanitize-trap=undefined"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return output


def run(tool, *args):
    result = subprocess.run([str(tool), *map(str, args)], capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def decode_adpcm(data, channels):
    # Independent reference reconstruction from the serialized block format.
    steps = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,
        80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,
        544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,
        2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,
        10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
    adjustments = [-1,-1,-1,-1,2,4,6,8]
    samples = [[] for _ in range(channels)]
    for start in range(0, len(data), channels * 36):
        block = data[start:start + channels * 36]
        for channel in range(channels):
            predictor, step, reserved = struct.unpack_from("<hBB", block, channel * 4)
            assert reserved == 0 and 0 <= step <= 88
            for frame in range(64):
                packed = block[channels * 4 + (frame // 8 * channels + channel) * 4 + frame % 8 // 2]
                code = packed >> (frame % 2 * 4) & 15
                value = steps[step]
                delta = value // 8 + (value // 4 if code & 1 else 0) + (value // 2 if code & 2 else 0) + (value if code & 4 else 0)
                predictor = max(-32768, min(32767, predictor + (-delta if code & 8 else delta)))
                step = max(0, min(88, step + adjustments[code & 7]))
                samples[channel].append(predictor)
    return samples


@pytest.mark.parametrize("channels,source_rate,target_rate", [(1,22050,22050),(2,44100,44100),(1,44100,22050)])
def test_xbox_adpcm_waveform(media_tool, tmp_path, channels, source_rate, target_rate):
    target = tmp_path / "encoded.bin"
    run(media_tool, "audio", channels, source_rate, target_rate, target)
    data = target.read_bytes()
    frames = 4096 * target_rate // source_rate
    assert len(data) == ((frames + 63) // 64) * 36 * channels
    decoded = decode_adpcm(data, channels)
    for channel in range(channels):
        signal = [(math.sin(i * source_rate / target_rate * 0.035) * 14000 if channel == 0
                   else math.cos(i * source_rate / target_rate * 0.051) * 9000) for i in range(frames)]
        # Skip the initial codec predictor warmup; wrong stereo block order
        # or sample-rate conversion produces errors thousands of times larger.
        rms = math.sqrt(sum((decoded[channel][i] - signal[i]) ** 2 for i in range(64, frames)) / (frames - 64))
        assert rms < 160


def test_virtual_audio_ranges_do_not_alias_raw_file(media_tool):
    run(media_tool, "contains")


def test_hud_canvas_and_bitmap_scale_are_independent(media_tool):
    run(media_tool, "hud")


def test_mcc_score_hint_resolves_binding_without_mutating_source(media_tool):
    run(media_tool, "score-hint")


def test_hardware_packing_swizzle_rows_faces_and_mips(media_tool):
    run(media_tool, "pack")


def test_cpu_dxt_tiny_mips_preserve_source_at_native_addresses(media_tool):
    run(media_tool, "compressed-tail")


def test_shared_bitmap_consumers_get_independent_metadata(tmp_path):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is needed for the MCC bitmap ownership test")
    bitmap = (ROOT / "port/linux/game/mcc_bitmaps.c").read_text()
    header = (ROOT / "source/bitmaps/bitmap_group.h").read_text()
    declarations = "\n".join(structure(header, n) for n in ["bitmap_data", "bitmap_group_sprite",
        "bitmap_group_sequence", "bitmap_group"])
    declarations += "\n" + "\n".join(structure(bitmap, n) for n in ["mcc_texture", "mcc_bitmap_clone", "mcc_textures"])
    routines = "\n".join(function(bitmap, n) for n in ["mcc_bitmap_tag", "mcc_bitmap_block",
        "mcc_bitmap_clone_block", "mcc_bitmap_specialize", "mcc_bitmaps_contains", "mcc_bitmaps_read"])
    source = r'''
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef float real;
union point2d {struct {short x,y;};uint32_t n;};
typedef struct {float x0,x1,y0,y1;} real_rectangle2d;
typedef struct {float x,y;} real_point2d;
struct tag_block {long count;void *address,*definition;};
struct tag_data {long size,flags,file_offset;void *address,*definition;};
#define BITMAP_GROUP_TAG 'bitm'
#define MCC_BITMAP_BASE 0x60000000u
struct mcc_runtime {unsigned char *tags,*tag_index;uint32_t used;struct {uint32_t tag_count;} report;void *bitmaps;};
static void *mcc_runtime_pointer(struct mcc_runtime *r,uint32_t p,uint32_t size) {
    uintptr_t start=(uintptr_t)r->tags;
    return p>=start && p-start<=r->used && size<=r->used-(p-start) ? (void *)(uintptr_t)p : NULL;
}
static void *mcc_runtime_allocate(struct mcc_runtime *r,uint32_t size) {
    uint32_t at=(r->used+15)&~15u;
    if(size>65536-at)return NULL;
    r->used=at+size;return r->tags+at;
}
''' + declarations + '\n' + routines + r'''
int main(void) {
    struct mcc_runtime r={0};struct mcc_textures textures={0};
    struct bitmap_group *original,*copy,*meter;struct bitmap_data *pixels;
    struct bitmap_group_sequence *sequence;struct bitmap_group_sprite *sprite;
    uint32_t *entry,source=0xE1740000,material=source,meter_handle=source,repeated=source;
    r.tags=calloc(1,65536);r.tag_index=r.tags;r.used=512;r.report.tag_count=1;
    original=mcc_runtime_allocate(&r,sizeof(*original));pixels=mcc_runtime_allocate(&r,sizeof(*pixels));
    sequence=mcc_runtime_allocate(&r,sizeof(*sequence));sprite=mcc_runtime_allocate(&r,sizeof(*sprite));
    original->bitmaps.count=1;original->bitmaps.address=pixels;
    original->sequences.count=1;original->sequences.address=sequence;
    sequence->sprites.count=1;sequence->sprites.address=sprite;
    pixels->width=64;pixels->pixels_offset=2048;sprite->bitmap_index=0;
    entry=(uint32_t *)r.tag_index;entry[0]='bitm';entry[3]=source;entry[5]=(uint32_t)original;
    textures.capacity=8;textures.tag_capacity=8;
    textures.items=calloc(8,sizeof(*textures.items));textures.clones=calloc(8,sizeof(*textures.clones));
    if(!mcc_bitmap_specialize(&r,&textures,&material,2) || material==source || r.report.tag_count!=2)return 1;
    copy=mcc_bitmap_tag(&r,material,'bitm',sizeof(*copy));
    if(!copy || copy==original || copy->bitmaps.address==pixels || copy->sequences.address==sequence)return 2;
    if(((struct bitmap_group_sequence *)copy->sequences.address)->sprites.address==sprite)return 3;
    ((struct bitmap_data *)copy->bitmaps.address)->width=32;
    if(pixels->width!=64 || pixels->pixels_offset!=2048)return 4;
    if(!mcc_bitmap_specialize(&r,&textures,&repeated,2) || repeated!=material || r.report.tag_count!=2)return 5;
    if(!mcc_bitmap_specialize(&r,&textures,&meter_handle,4) || meter_handle==material || r.report.tag_count!=3)return 6;
    meter=mcc_bitmap_tag(&r,meter_handle,'bitm',sizeof(*meter));
    if(!meter || ((struct bitmap_data *)meter->bitmaps.address)->width!=64)return 7;
    if(textures.items[0].channels!=2 || textures.items[1].channels!=4)return 8;
    r.bitmaps=&textures;textures.items[0].offset=MCC_BITMAP_BASE;textures.items[0].size=128;
    textures.items[0].pixels=calloc(1,128);textures.items[0].pixels[127]=0xAB;
    {
        unsigned char out[3]={0xCC,0,0xDD};
        if(!mcc_bitmaps_contains(&r,MCC_BITMAP_BASE,128) || mcc_bitmaps_contains(&r,MCC_BITMAP_BASE,129))return 9;
        if(mcc_bitmaps_read(&r,material,MCC_BITMAP_BASE+127,1,out+1)!=1 || out[1]!=0xAB || out[0]!=0xCC || out[2]!=0xDD)return 10;
        if(mcc_bitmaps_read(&r,source,MCC_BITMAP_BASE,1,out+1)!=0 || mcc_bitmaps_read(&r,material,MCC_BITMAP_BASE+127,2,out+1)!=0)return 11;
        if(mcc_bitmaps_read(&r,material,2048,1,out+1)!=-1)return 12;
    }
    free(textures.items[0].pixels);
    free(textures.clones);free(textures.items);free(r.tags);return 0;
}
'''
    path = tmp_path / "ownership.c"
    path.write_text(source)
    output = tmp_path / ("ownership.exe" if sys.platform == "win32" else "ownership")
    command = [clang, "-std=gnu99", "-O2", "-Wall", "-Wextra", "-Werror", "-Wno-multichar", str(path), "-o", str(output)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    else:
        command[1:1] = ["-m32"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    run(output)


def test_mcc_owned_pixels_survive_pressure_without_legacy_cache(tmp_path):
    """Direct bindings retain original pixels above the legacy staging budget."""
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is needed for MCC bitmap ownership tests")
    bitmap = (ROOT / "port/linux/game/mcc_bitmaps.c").read_text()
    declarations = "\n".join(structure(bitmap, n) for n in ["mcc_texture", "mcc_bitmap_clone", "mcc_textures"])
    routines = "\n".join(function(bitmap, n) for n in ["mcc_bitmaps_valid", "mcc_bitmaps_pixels", "mcc_bitmaps_dispose"])
    source = r'''
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "halo_port_capacity.h"
#define FALSE 0
struct bitmap_data {
    uint32_t tag_index,pixels_offset,pixels_size;
    short width,height,depth,format,mipmap_count;
    void *base_address;
};
struct mcc_runtime {void *bitmaps;};
static int bitmap_verify(struct bitmap_data *bitmap,int import) {
    (void)import;
    return bitmap->width==2048 && bitmap->height==2048 && bitmap->depth==1 &&
        bitmap->format==11 && bitmap->mipmap_count==11;
}
static long rasterizer_xbox_bitmap_get_pixel_data_size(struct bitmap_data *bitmap) {
    (void)bitmap;return 22369664;
}
''' + declarations + '\n' + routines + r'''
int main(void) {
    enum {count=13,chain_bytes=22369664};
    struct bitmap_data bitmaps[count],copy;
    struct mcc_texture items[count];
    struct mcc_textures textures={0};
    struct mcc_runtime runtime;
    uint32_t bytes,i,frame,offset=0x60000000;
    memset(bitmaps,0,sizeof(bitmaps));memset(items,0,sizeof(items));
    textures.items=items;textures.count=count;runtime.bitmaps=&textures;
    /* Thirteen real 2K ARGB mip chains exceed the desktop cache's 256MiB.
     * Only boundary pages need touching: the API must return, never copy,
     * the owned immutable allocation, so no legacy allocator is linked. */
    if((uint64_t)count*chain_bytes<=HALO_PORT_TEXTURE_CACHE_SIZE)return 1;
    for(i=0;i<count;i++) {
        struct bitmap_data *bitmap=&bitmaps[i];struct mcc_texture *item=&items[i];
        bitmap->tag_index=0xE1000000+i;bitmap->pixels_offset=offset;bitmap->pixels_size=chain_bytes;
        bitmap->width=bitmap->height=2048;bitmap->depth=1;bitmap->format=11;bitmap->mipmap_count=11;
        item->bitmap=bitmap;item->handle=bitmap->tag_index;item->offset=offset;item->size=chain_bytes;
        item->pixels=malloc(chain_bytes);if(!item->pixels)return 2;
        item->pixels[0]=(unsigned char)i;item->pixels[chain_bytes-1]=(unsigned char)(255-i);
        offset+=chain_bytes;
    }
    for(frame=0;frame<3;frame++)for(i=0;i<count;i++) {
        unsigned n=frame&1 ? count-1-i : i;
        unsigned char const *pixels=mcc_bitmaps_pixels(&runtime,&bitmaps[n],&bytes);
        if(pixels!=items[n].pixels || bytes!=chain_bytes || pixels[0]!=n || pixels[bytes-1]!=255-n)return 3;
    }
    copy=bitmaps[0];bytes=123;
    if(mcc_bitmaps_pixels(&runtime,&copy,&bytes) || bytes)return 4;
    bitmaps[0].tag_index++;bytes=123;
    if(mcc_bitmaps_pixels(&runtime,&bitmaps[0],&bytes) || bytes)return 5;
    bitmaps[0]=copy;bitmaps[0].pixels_offset++;bytes=123;
    if(mcc_bitmaps_pixels(&runtime,&bitmaps[0],&bytes) || bytes)return 6;
    bitmaps[0]=copy;bitmaps[0].pixels_size--;bytes=123;
    if(mcc_bitmaps_pixels(&runtime,&bitmaps[0],&bytes) || bytes)return 7;
    bitmaps[0]=copy;bitmaps[0].width=0;bytes=123;
    if(mcc_bitmaps_pixels(&runtime,&bitmaps[0],&bytes) || bytes)return 8;
    bitmaps[0]=copy;items[0].size=bitmaps[0].pixels_size=chain_bytes-1;bytes=123;
    if(mcc_bitmaps_pixels(&runtime,&bitmaps[0],&bytes) || bytes)return 9;
    items[0].size=bitmaps[0].pixels_size=chain_bytes;
    if(mcc_bitmaps_pixels(&runtime,&bitmaps[0],NULL)!=items[0].pixels)return 10;
    bytes=123;if(mcc_bitmaps_pixels(NULL,&bitmaps[0],&bytes) || bytes)return 11;
    bytes=123;if(mcc_bitmaps_pixels(&runtime,NULL,&bytes) || bytes)return 12;
    runtime.bitmaps=NULL;bytes=123;
    if(mcc_bitmaps_pixels(&runtime,&bitmaps[0],&bytes) || bytes)return 13;
    for(i=0;i<count;i++)free(items[i].pixels);
    {
        struct mcc_textures *owned=calloc(1,sizeof(*owned));
        unsigned char foreign=0;
        owned->items=calloc(2,sizeof(*owned->items));owned->count=2;
        owned->items[0].pixels=malloc(128);owned->items[0].bitmap=&bitmaps[0];
        owned->items[1].pixels=malloc(128);owned->items[1].bitmap=&bitmaps[1];
        bitmaps[0].base_address=owned->items[0].pixels;bitmaps[1].base_address=&foreign;
        runtime.bitmaps=owned;mcc_bitmaps_dispose(&runtime);
        if(runtime.bitmaps || bitmaps[0].base_address || bitmaps[1].base_address!=&foreign)return 14;
        mcc_bitmaps_dispose(&runtime);
    }
    return 0;
}
'''
    path = tmp_path / "residency.c"
    path.write_text(source)
    output = tmp_path / ("residency.exe" if sys.platform == "win32" else "residency")
    command = [clang, "-std=gnu99", "-O2", "-Wall", "-Wextra", "-Werror",
               "-I", str(ROOT / "port/linux/include"), str(path), "-o", str(output)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    run(output)


def pixel_vector(media_tool, tmp_path, data, format, usage=0, side=4):
    path = tmp_path / "block.bin"
    path.write_bytes(data)
    return [int(line, 16) for line in run(media_tool, "pixels", format, usage, side, path).splitlines()]


def test_bc1_red_and_partial_block(media_tool, tmp_path):
    block = struct.pack("<HHI", 0xF800, 0, 0)
    assert pixel_vector(media_tool, tmp_path, block, 14) == [0xFFFF0000] * 16
    assert pixel_vector(media_tool, tmp_path, block, 14, side=1) == [0xFFFF0000]


def test_mcc_multipurpose_channel_order(media_tool, tmp_path):
    assert pixel_vector(media_tool, tmp_path, bytes([10,20,30,40]), 11, usage=2, side=1) == [0x1E0A1428]
    assert pixel_vector(media_tool, tmp_path, bytes([10,20,30,40]), 11, usage=4, side=1) == [0x1E282828]


def test_bc7_mode6_opaque_red(media_tool, tmp_path):
    # Microsoft BC7 mode-6 layout: unary mode, 2 RGBA endpoints at 7 bits per
    # component, one P bit per endpoint, then zero interpolation indices.
    bits = 1 << 6
    position = 7
    for component in [127,127,0,0,0,0,127,127]:
        bits |= component << position
        position += 7
    bits |= 3 << position
    block = bits.to_bytes(16, "little")
    assert pixel_vector(media_tool, tmp_path, block, 18) == [0xFFFF0101] * 16
