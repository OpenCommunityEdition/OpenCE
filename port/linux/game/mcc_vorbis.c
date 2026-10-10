/* MCC's private instance of the unmodified generic Vorbis dependency. Every
 * public dependency symbol is namespaced so Xbox/CE keep their existing decoder.
 * Empty residue vector books add nothing in Xiph's reference decoder; stb treats
 * an attempted read from one as an error and abandons the remaining residue.
 * Normalize only those vector-book references before decoding any audio packet.
 * Reference: https://github.com/xiph/vorbis/blob/v1.3.7/lib/codebook.c
 * (vorbis_book_decodevs_add, vorbis_book_decodev_add, vorbis_book_decodevv_add).
 */
#include "mcc_vorbis.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#define stb_vorbis_get_info mcc_stb_get_info
#define stb_vorbis_get_comment mcc_stb_get_comment
#define stb_vorbis_get_error mcc_stb_get_error
#define stb_vorbis_close mcc_stb_close
#define stb_vorbis_get_sample_offset mcc_stb_get_sample_offset
#define stb_vorbis_get_file_offset mcc_stb_get_file_offset
#define stb_vorbis_decode_memory mcc_stb_decode_memory
#define stb_vorbis_open_memory mcc_stb_open_memory
#define stb_vorbis_seek_frame mcc_stb_seek_frame
#define stb_vorbis_seek mcc_stb_seek
#define stb_vorbis_seek_start mcc_stb_seek_start
#define stb_vorbis_stream_length_in_samples mcc_stb_stream_length_in_samples
#define stb_vorbis_stream_length_in_seconds mcc_stb_stream_length_in_seconds
#define stb_vorbis_get_frame_float mcc_stb_get_frame_float
#define stb_vorbis_get_frame_short_interleaved mcc_stb_get_frame_short_interleaved
#define stb_vorbis_get_frame_short mcc_stb_get_frame_short
#define stb_vorbis_get_samples_float_interleaved mcc_stb_get_samples_float_interleaved
#define stb_vorbis_get_samples_float mcc_stb_get_samples_float
#define stb_vorbis_get_samples_short_interleaved mcc_stb_get_samples_short_interleaved
#define stb_vorbis_get_samples_short mcc_stb_get_samples_short
#include "stb_vorbis.c"

static uint32_t mcc_ogg_u32(unsigned char const *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}

/* The pull decoder does not verify page checksums. Validate the complete bounded
 * permutation first, including EOS, so a truncated/damaged packet is not made
 * acceptable by the narrow empty-book compatibility rule. */
static int mcc_ogg_validate(unsigned char const *bytes, uint32_t size, uint32_t *frames,
    uint32_t *payload_end, uint32_t *final_page)
{
    uint32_t table[256], offset=0, serial=0, sequence=0, i, j;
    uint32_t last_page=0, last_end=0;
    int continued=0, ended=0;
    *payload_end=size;*final_page=0;
    for (i=0;i<256;i++) {
        uint32_t crc=i<<24;
        for (j=0;j<8;j++) crc=(crc<<1)^((crc&0x80000000u)?0x04C11DB7u:0);
        table[i]=crc;
    }
    while (offset<size) {
        unsigned char const *page=bytes+offset;
        uint32_t length=27, count, crc=0;
        if (ended || size-offset<27 || memcmp(page,"OggS",4) || page[4] || (page[5]&~7)) return 0;
        count=page[26];
        if (count>size-offset-length) return 0;
        length+=count;
        for (i=0;i<count;i++) length+=page[27+i];
        if (length>size-offset || (count && !!(page[5]&1)!=continued)) return 0;
        if (!offset) {
            if (!(page[5]&2)) return 0;
            serial=mcc_ogg_u32(page+14);sequence=mcc_ogg_u32(page+18);
        } else if ((page[5]&2) || mcc_ogg_u32(page+14)!=serial || mcc_ogg_u32(page+18)!=sequence) return 0;
        for (i=0;i<length;i++) {
            unsigned char value=i>=22 && i<26 ? 0 : page[i];
            crc=(crc<<8)^table[(crc>>24)^value];
        }
        if (crc!=mcc_ogg_u32(page+22)) return 0;
        /* Ogg permits zero-segment pages (including a separate EOS page). With
         * no lacing entries no packet begins or ends, so retain pending state.
         * https://xiph.org/ogg/doc/framing.html#page_segments */
        if (count) {
            continued=page[26+count]==255;
            last_page=offset;last_end=offset+length;
        }
        ended=(page[5]&4)!=0;
        if (ended) {
            if (continued || mcc_ogg_u32(page+10)) return 0;
            *frames=mcc_ogg_u32(page+6);
            if (!count) {
                if (!last_end) return 0;
                *payload_end=last_end;*final_page=last_page;
            }
        }
        offset+=length;sequence++;
    }
    return ended;
}

/* stb's page reader uses a stale lacing entry for an empty terminal page. Give
 * only this case a private equivalent framing: finish the last complete packet
 * page with the already-validated EOS granule. This also preserves final-block
 * trimming. Source bytes and the shared Xbox/CE decoder remain untouched. */
static unsigned char *mcc_ogg_terminal_page(unsigned char const *bytes,
    uint32_t size, uint32_t offset, uint32_t frames)
{
    unsigned char *copy=malloc(size), *page;
    uint32_t i,j,crc=0;
    if (!copy) return NULL;
    memcpy(copy,bytes,size);page=copy+offset;
    page[5]|=4;
    for (i=0;i<4;i++) page[6+i]=(unsigned char)(frames>>(8*i));
    memset(page+10,0,4);memset(page+22,0,4);
    for (i=0;i<size-offset;i++) {
        crc^=(uint32_t)page[i]<<24;
        for (j=0;j<8;j++) crc=(crc<<1)^((crc&0x80000000u)?0x04C11DB7u:0);
    }
    for (i=0;i<4;i++) page[22+i]=(unsigned char)(crc>>(8*i));
    return copy;
}

static int mcc_vorbis_empty_book(Codebook const *book)
{
    int i;
    if (book->sparse) return !book->sorted_entries;
    /* stb expands very small sparse books, including an empty 1-3 entry book. */
    for (i=0;i<book->entries;i++) if (book->codeword_lengths[i]!=NO_CODE) return 0;
    return 1;
}

static int mcc_vorbis_residues(vorb *stream)
{
    int index, classification, pass;
    /* A floor book supplies scalar values, never an additive vector. Keep that
     * distinct from the zero-contribution residue case. */
    for (index=0;index<stream->floor_count;index++) {
        Floor1 const *floor=&stream->floor_config[index].floor1;
        int partition;
        if (stream->floor_types[index]!=1) return 0;
        for (partition=0;partition<floor->partitions;partition++) {
            int kind=floor->partition_class_list[partition];
            int subclasses=floor->class_subclasses[kind];
            if (subclasses && mcc_vorbis_empty_book(stream->codebooks+floor->class_masterbooks[kind])) return 0;
            for (classification=0;classification<(1<<subclasses);classification++) {
                int number=floor->subclass_books[kind][classification];
                if (number>=0 && mcc_vorbis_empty_book(stream->codebooks+number)) return 0;
            }
        }
    }
    for (index=0;index<stream->residue_count;index++) {
        Residue *residue=stream->residue_config+index;
        /* Empty scalar classbooks cannot supply classifications. This is not the
         * empty additive-vector case, and must not be silently accepted. */
        if (mcc_vorbis_empty_book(stream->codebooks+residue->classbook)) return 0;
        for (classification=0;classification<residue->classifications;classification++) {
            for (pass=0;pass<8;pass++) {
                int number=residue->residue_books[classification][pass];
                if (number>=0) {
                    Codebook const *book=stream->codebooks+number;
                    if (!book->lookup_type) return 0;
                    if (mcc_vorbis_empty_book(book)) residue->residue_books[classification][pass]=-1;
                }
            }
        }
    }
    return 1;
}

int mcc_vorbis_decode(unsigned char const *bytes, uint32_t size, uint32_t frame_limit,
    short **samples, uint32_t *frames, uint32_t *rate, int *channels)
{
    vorb stream;
    uint32_t expected=0, count=0, payload_end=0, final_page=0;
    unsigned char *normalized=NULL;
    short *pcm=NULL;
    int ok=0;
    *samples=NULL;*frames=0;*rate=0;*channels=0;
    if (!bytes || !size || size>INT_MAX || !mcc_ogg_validate(bytes,size,&expected,&payload_end,&final_page) ||
        !expected || expected>frame_limit || expected>INT_MAX/(2*sizeof(short))) return 0;
    if (payload_end<size) {
        normalized=mcc_ogg_terminal_page(bytes,payload_end,final_page,expected);
        if (!normalized) return 0;
        bytes=normalized;size=payload_end;
    }
    vorbis_init(&stream,NULL);
    stream.stream=(uint8 *)bytes;
    stream.stream_start=stream.stream;
    stream.stream_end=stream.stream+size;
    stream.stream_len=size;
    if (!start_decoder(&stream) || !mcc_vorbis_residues(&stream) ||
        stream.channels<1 || stream.channels>2 || stream.sample_rate<8000 || stream.sample_rate>192000) goto done;
    pcm=malloc((size_t)expected*stream.channels*sizeof(short));
    if (!pcm) goto done;
    /* Normalization precedes the discarded overlap packet, which can itself use
     * an empty residue vector book. No Xbox/CE decoder state is accessible here. */
    if (!vorbis_pump_first_frame(&stream) || stream.error) goto done;
    while (count<expected) {
        int got=stb_vorbis_get_samples_short_interleaved(&stream,stream.channels,
            pcm+count*stream.channels,(int)((expected-count)*stream.channels));
        if (got<=0 || stream.error) goto done;
        count+=(uint32_t)got;
    }
    {
        short extra[2];
        if (stb_vorbis_get_samples_short_interleaved(&stream,stream.channels,extra,stream.channels) || stream.error) goto done;
    }
    *samples=pcm;*frames=count;*rate=stream.sample_rate;*channels=stream.channels;
    pcm=NULL;ok=1;
done:
    free(pcm);
    vorbis_deinit(&stream);
    free(normalized);
    return ok;
}

void mcc_vorbis_free(short *samples)
{
    free(samples);
}
