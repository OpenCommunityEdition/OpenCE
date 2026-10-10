"""Asset-free Vorbis streams exercise MCC's isolated empty-residue adapter.

Generated streams have two residue passes: an empty vector book followed by a
nonzero vector book. This catches the original decoder's premature residue exit
as well as error suppression that would return the wrong PCM. No game audio is
included. The fixture also links the unchanged legacy decoder alongside MCC.
"""
from pathlib import Path
import os
import shutil
import struct
import subprocess
import re
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from harness import function, structure


class Bits:
    def __init__(self):
        self.bits = []

    def put(self, value, width):
        self.bits.extend((value >> i) & 1 for i in range(width))

    def bytes(self):
        out = bytearray((len(self.bits) + 7) // 8)
        for i, value in enumerate(self.bits):
            out[i // 8] |= value << (i % 8)
        return bytes(out)


def crc32(data):
    crc = 0
    for value in data:
        crc ^= value << 24
        for _ in range(8):
            crc = ((crc << 1) ^ (0x04C11DB7 if crc & 0x80000000 else 0)) & 0xFFFFFFFF
    return crc


def page(packet, sequence, flags, granule):
    lacing = [255] * (len(packet) // 255) + [len(packet) % 255]
    result = bytearray(b"OggS" + bytes([0, flags]) + struct.pack("<QIII", granule, 1234, sequence, 0))
    result += bytes([len(lacing), *lacing]) + packet
    struct.pack_into("<I", result, 22, crc32(result))
    return bytes(result)


def stream(channels=2, residue_type=2, empty=True, bad_class=False, bad_vector=False,
           bad_floor=False, empty_entries=1, transition=False, final_frames=280):
    identification = b"\x01vorbis" + struct.pack("<IBIiiiBB", 0, channels, 22050, 0, 0, 0, 0x86 if transition else 0x88, 1)
    comment = b"\x03vorbis" + struct.pack("<II", 0, 0) + b"\x01"
    setup = Bits()
    setup.put(2, 8)  # three codebooks
    for index in range(3):
        entries = empty_entries if index == 1 else 1
        setup.put(0x564342, 24)
        setup.put(1, 16)  # dimensions
        setup.put(entries, 24)
        setup.put(0, 1)  # unordered
        is_empty = index == 1 or (index == 0 and bad_class)
        setup.put(is_empty, 1)  # sparse
        setup.put(0, entries if is_empty else 5)  # absent, or length 1
        lookup = 0 if index == 0 or (index == 1 and bad_vector) else 1
        setup.put(lookup, 4)
        if lookup:
            setup.put(0x60100000 if index == 2 else 0, 32)  # nonzero later residue
            setup.put(0, 32)
            setup.put(0, 4)
            setup.put(0, 1)  # sequence flag
            setup.put(0, entries)  # one-bit multiplicands, dimensions1
    setup.put(0, 6)  # one time transform
    setup.put(0, 16)
    setup.put(0, 6)  # one floor
    setup.put(1, 16)  # floor1
    setup.put(1 if bad_floor else 0, 5)
    if bad_floor:
        setup.put(0, 4)  # partition class0
        setup.put(0, 3)  # dimension1
        setup.put(0, 2)  # no subclasses
        setup.put(2, 8)  # empty codebook1 is a scalar floor book
    setup.put(0, 2)  # multiplier1
    setup.put(5, 4)  # range32
    if bad_floor:
        setup.put(16, 5)  # extra floor coordinate
    setup.put(0, 6)  # one residue
    setup.put(residue_type, 16)
    setup.put(0, 24)
    setup.put(4, 24)
    setup.put(3, 24)  # partition size4
    setup.put(0, 6)  # one classification
    setup.put(0, 8)  # classbook0
    setup.put(3 if empty else 2, 3)  # passes0/1, or pass1 only
    setup.put(0, 1)
    if empty:
        setup.put(1, 8)
    setup.put(2, 8)
    setup.put(0, 6)  # one mapping
    setup.put(0, 16)
    setup.put(0, 1)  # no submaps
    setup.put(0, 1)  # no coupling
    setup.put(0, 2)
    setup.put(0, 8)
    setup.put(0, 8)
    setup.put(0, 8)
    setup.put(1 if transition else 0, 6)
    for mode in range(2 if transition else 1):
        setup.put(mode, 1)
        setup.put(0, 16)
        setup.put(0, 16)
        setup.put(0, 8)
    setup.put(1, 1)
    audio = Bits()
    audio.put(0, 1)
    for _ in range(channels):
        audio.put(1, 1)
        audio.put(220, 8)
        audio.put(220, 8)
    # The classbook consumes one bit per partition/channel; empty vectors consume
    # none. The later pass consumes four bits per partition/channel.
    vectors = 1 if residue_type == 2 and channels == 2 else channels
    audio.put(0, vectors + 4 * vectors)
    if transition:
        packets = [identification, comment, b"\x05vorbis" + setup.bytes()]
        for i in range(4):
            mixed = Bits()
            mixed.put(0, 1)
            mixed.put(i < 3, 1)
            if i < 3:
                mixed.put(1, 1)
                mixed.put(i < 2, 1)
            mixed.bits.extend(audio.bits[1:])
            packets.append(mixed.bytes())
        return b"".join(page(packet, i, 2 if i == 0 else 4 if i == 6 else 0,
                             final_frames if i == 6 else max(0, (i - 3) * 128))
                        for i, packet in enumerate(packets))
    packets = [identification, comment, b"\x05vorbis" + setup.bytes()] + [audio.bytes()] * 4
    return b"".join(page(packet, i, 2 if i == 0 else 4 if i == 6 else 0,
                         384 if i == 6 else max(0, (i - 3) * 128))
                    for i, packet in enumerate(packets))


@pytest.fixture(scope="module")
def decoder(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is needed for MCC Vorbis tests")
    work = tmp_path_factory.mktemp("mcc-vorbis")
    source = work / "decoder.c"
    source.write_text(r'''
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "mcc_vorbis.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
int main(int argc,char **argv) {
    FILE *file;unsigned char *bytes;long size;short *pcm=NULL;
    uint32_t frames=0,rate=0;int channels=0,ok;
    if(argc!=4)return 2;
    file=fopen(argv[1],"rb");if(!file)return 3;
    fseek(file,0,SEEK_END);size=ftell(file);rewind(file);
    bytes=malloc(size);if(!bytes || fread(bytes,1,size,file)!=(size_t)size)return 4;fclose(file);
    if(atoi(argv[3])) {
        int error=0;stb_vorbis *v=stb_vorbis_open_memory(bytes,size,&error,NULL);
        if(!v){free(bytes);return 1;}
        channels=stb_vorbis_get_info(v).channels;rate=stb_vorbis_get_info(v).sample_rate;
        pcm=calloc(512*channels,sizeof(short));
        frames=stb_vorbis_get_samples_short_interleaved(v,channels,pcm,512*channels);
        ok=frames && stb_vorbis_get_error(v)==0;stb_vorbis_close(v);
    } else ok=mcc_vorbis_decode(bytes,(uint32_t)size,1024,&pcm,&frames,&rate,&channels);
    if(ok) {file=fopen(argv[2],"wb");if(!file)return 5;
        fwrite(&frames,4,1,file);fwrite(&rate,4,1,file);fwrite(&channels,4,1,file);
        fwrite(pcm,2,frames*channels,file);fclose(file);}
    if(atoi(argv[3]))free(pcm);else mcc_vorbis_free(pcm);
    free(bytes);return ok?0:1;
}
''')
    exe = work / ("decoder.exe" if os.name == "nt" else "decoder")
    flags = ["-target", "i686-pc-windows-msvc"] if os.name == "nt" else []
    subprocess.run([compiler, *flags, "-O2", "-Wno-deprecated-declarations", "-Wno-tautological-compare",
                    "-I", str(ROOT / "port/linux/game"), str(source),
                    str(ROOT / "port/linux/game/mcc_vorbis.c"),
                    str(ROOT / "port/linux/game/stb_vorbis.c"),
                    *([] if os.name == "nt" else ["-lm"]), "-o", str(exe)], check=True, capture_output=True)
    return exe


def decode(decoder, tmp_path, data, legacy=False):
    source, target = tmp_path / "input.ogg", tmp_path / "out.pcm"
    source.write_bytes(data)
    result = subprocess.run([str(decoder), str(source), str(target), str(int(legacy))], capture_output=True)
    assert result.returncode in (0, 1), result.stderr
    return target.read_bytes() if result.returncode == 0 else None


@pytest.mark.parametrize("channels", [1, 2])
@pytest.mark.parametrize("residue_type", [0, 1, 2])
@pytest.mark.parametrize("empty_entries", [1, 625])
def test_empty_residue_preserves_later_nonzero_pass(decoder, tmp_path, channels, residue_type, empty_entries):
    expected = decode(decoder, tmp_path, stream(channels, residue_type, empty=False))
    assert expected is not None
    assert struct.unpack_from("<III", expected) == (384, 22050, channels)
    assert any(expected[12:])
    assert decode(decoder, tmp_path, stream(channels, residue_type, empty=False), legacy=True) == expected
    actual = decode(decoder, tmp_path, stream(channels, residue_type, empty_entries=empty_entries))
    assert actual == expected
    # The unaltered legacy decoder either rejects the empty book or mistakes it
    # for packet exhaustion (depending on prefetch), losing the nonzero pass.
    assert decode(decoder, tmp_path, stream(channels, residue_type, empty_entries=empty_entries), legacy=True) != expected


@pytest.mark.parametrize("mutation", ["crc", "truncated", "missing_eos", "extra_bytes", "classbook",
                                     "vector_lookup", "floorbook", "sequence", "frame_limit", "granule"])
def test_invalid_audio_still_rejected(decoder, tmp_path, mutation):
    data = stream(bad_class=mutation == "classbook", bad_vector=mutation == "vector_lookup",
                  bad_floor=mutation == "floorbook")
    if mutation == "crc":
        data = data[:-1] + bytes([data[-1] ^ 1])
    elif mutation == "truncated":
        data = data[:-3]
    elif mutation == "missing_eos":
        data = data[:data.rfind(b"OggS")]
    elif mutation == "extra_bytes":
        data += b"\0"
    elif mutation in ("sequence", "frame_limit", "granule"):
        start = data.rfind(b"OggS")
        last = bytearray(data[start:])
        if mutation == "sequence":
            struct.pack_into("<I", last, 18, 99)
        else:
            struct.pack_into("<Q", last, 6, 1025 if mutation == "frame_limit" else 512)
        struct.pack_into("<I", last, 22, 0)
        struct.pack_into("<I", last, 22, crc32(last))
        data = data[:start] + last
    assert decode(decoder, tmp_path, data) is None


@pytest.mark.parametrize("empty", [False, True])
@pytest.mark.parametrize("frames", [383, 384])
def test_separate_zero_segment_eos_page(decoder, tmp_path, empty, frames):
    data = stream(empty=empty)
    start = data.rfind(b"OggS")
    last = bytearray(data[start:])
    struct.pack_into("<Q", last, 6, frames)
    struct.pack_into("<I", last, 22, 0)
    struct.pack_into("<I", last, 22, crc32(last))
    expected = decode(decoder, tmp_path, data[:start] + last)
    assert expected is not None
    assert struct.unpack_from("<I", expected)[0] == frames
    last[5] &= ~4
    struct.pack_into("<I", last, 22, 0)
    struct.pack_into("<I", last, 22, crc32(last))
    eos = bytearray(b"OggS" + bytes([0, 4]) + struct.pack("<QIII", frames, 1234, 7, 0) + b"\0")
    struct.pack_into("<I", eos, 22, crc32(eos))
    data = data[:start] + last + eos
    assert decode(decoder, tmp_path, data) == expected
    if not empty:
        # Legacy accepts this framing but reads a stale lacing entry, producing
        # an extra frame. MCC must retain precisely the validated final granule.
        assert decode(decoder, tmp_path, data, legacy=True) is not None
    # An empty final page must not conceal an unfinished preceding packet.
    last[27] = 255
    last += bytes(255 - (len(last) - 28))
    struct.pack_into("<I", last, 22, 0)
    struct.pack_into("<I", last, 22, crc32(last))
    assert decode(decoder, tmp_path, data[:start] + last + eos) is None


@pytest.fixture(scope="module")
def allocator_decoder(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is needed for MCC Vorbis tests")
    work = tmp_path_factory.mktemp("mcc-vorbis-allocator")
    audio = (ROOT / "port/linux/game/mcc_audio.c").read_text()
    declarations = "\n".join(structure(audio, name) for name in ["mcc_pcm", "mcc_ima"])
    tables = "\n".join(re.search(r"static int const " + name + r"\[.*?\};", audio, re.S).group()
                       for name in ["mcc_ima_steps", "mcc_ima_changes"])
    routines = "\n".join(function(audio, name) for name in ["mcc_ima_advance", "mcc_pcm_open", "mcc_pcm_close"])
    source = work / "allocator.c"
    source.write_text(r'''
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "mcc_vorbis.h"
#define PIN(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define MCC_AUDIO_FRAME_LIMIT 0x1000000u
static void *tracked;
static int allocations,releases,fail_allocation;
static void *tracked_malloc(size_t size) {
    unsigned char *allocation;
    if(fail_allocation)return NULL;
    if(tracked)exit(70);
    allocation=malloc(size+16);if(!allocation)exit(71);
    tracked=allocation+16;allocations++;return tracked;
}
static void tracked_free(void *pointer) {
    /* A plain decoder pointer is rejected before any header is dereferenced. */
    if(pointer!=tracked || !tracked)exit(72);
    free((unsigned char *)pointer-16);tracked=NULL;releases++;
}
#define malloc tracked_malloc
#define free tracked_free
''' + declarations + "\n" + tables + "\n" + routines + r'''
#undef malloc
#undef free
int main(int argc,char **argv) {
    unsigned char raw[8]={0,0,1,0,2,0,3,0},bad_adpcm[36]={0};
    FILE *file;long size;unsigned char *bytes;struct mcc_pcm pcm;
    if(argc!=2)return 2;
    file=fopen(argv[1],"rb");if(!file)return 3;
    fseek(file,0,SEEK_END);size=ftell(file);rewind(file);bytes=malloc(size);
    if(!bytes || fread(bytes,1,size,file)!=(size_t)size)return 4;fclose(file);
    if(!mcc_pcm_open(bytes,size,3,2,22050,&pcm) || !pcm.samples || !pcm.vorbis_owned)return 5;
    /* This close must use the decoder's owner, with no tracked allocation. */
    mcc_pcm_close(&pcm);mcc_pcm_close(&pcm);
    if(pcm.samples || allocations || releases)return 6;
    if(!mcc_pcm_open(raw,8,0,1,22050,&pcm) || pcm.vorbis_owned || pcm.frames!=4)return 7;
    mcc_pcm_close(&pcm);mcc_pcm_close(&pcm);
    if(pcm.samples || allocations!=1 || releases!=1)return 8;
    /* A post-allocation ADPCM failure must also return to the tracked owner. */
    bad_adpcm[2]=89;
    if(mcc_pcm_open(bad_adpcm,36,1,1,22050,&pcm))return 9;
    mcc_pcm_close(&pcm);
    if(allocations!=2 || releases!=2 || tracked)return 10;
    if(mcc_pcm_open(raw,8,3,2,22050,&pcm))return 11;
    mcc_pcm_close(&pcm);
    fail_allocation=1;
    if(mcc_pcm_open(raw,8,0,1,22050,&pcm))return 12;
    mcc_pcm_close(&pcm);
    if(allocations!=2 || releases!=2 || tracked)return 13;
    free(bytes);return 0;
}
''')
    exe = work / ("allocator.exe" if os.name == "nt" else "allocator")
    flags = ["-target", "i686-pc-windows-msvc"] if os.name == "nt" else []
    subprocess.run([compiler, *flags, "-O2", "-Wno-deprecated-declarations", "-Wno-tautological-compare",
                    "-I", str(ROOT / "port/linux/game"), str(source),
                    str(ROOT / "port/linux/game/mcc_vorbis.c"),
                    *([] if os.name == "nt" else ["-lm"]), "-o", str(exe)], check=True, capture_output=True)
    return exe


def test_pcm_returns_to_its_own_allocator(allocator_decoder, tmp_path):
    source = tmp_path / "input.ogg"
    source.write_bytes(stream())
    subprocess.run([str(allocator_decoder), str(source)], check=True, capture_output=True)


@pytest.mark.parametrize("channels", [1, 2])
@pytest.mark.parametrize("frames", [256, 280, 304, 320])
def test_short_final_block_trims_preceding_overlap(decoder, tmp_path, channels, frames):
    complete = decode(decoder, tmp_path, stream(channels=channels, transition=True, final_frames=320))
    assert complete is not None
    actual = decode(decoder, tmp_path, stream(channels=channels, transition=True, final_frames=frames))
    assert actual is not None
    assert struct.unpack_from("<III", actual) == (frames, 22050, channels)
    assert actual[12:] == complete[12:12 + frames * channels * 2]


@pytest.mark.parametrize("mutation", ["past_end", "before_previous_page", "drifting_page", "truncated", "crc"])
def test_transition_validation_is_not_disabled(decoder, tmp_path, mutation):
    data = stream(transition=True)
    pages = [m.start() for m in re.finditer(b"OggS", data)]
    if mutation == "truncated":
        data = data[:-1]
    elif mutation == "crc":
        data = data[:-1] + bytes([data[-1] ^ 1])
    else:
        index = -2 if mutation == "drifting_page" else -1
        start = pages[index]
        end = pages[index + 1] if index == -2 else len(data)
        modified = bytearray(data[start:end])
        struct.pack_into("<Q", modified, 6, {"past_end": 800, "before_previous_page": 200, "drifting_page": 250}[mutation])
        struct.pack_into("<I", modified, 22, 0)
        struct.pack_into("<I", modified, 22, crc32(modified))
        data = data[:start] + modified + data[end:]
    assert decode(decoder, tmp_path, data) is None
