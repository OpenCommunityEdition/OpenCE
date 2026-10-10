/* MCC audio normalization. An MCC-owned Vorbis adapter handles codec differences;
 * no Custom Edition state or sound conversion routine is used. A damaged or
 * unsupported permutation rejects the map, rather than quietly silencing it. */
#include "cseries.h"
#include "errors.h"
#include "sound/sound_definitions.h"
#include "mcc_runtime.h"
#include "mcc_resources.h"
#include "mcc_vorbis.h"
#include <stdlib.h>
#include <string.h>

#define MCC_AUDIO_BASE 0x50000000u
#define MCC_AUDIO_LIMIT 0x10000000u
#define MCC_AUDIO_FRAME_LIMIT 0x1000000u

struct mcc_audio_storage { unsigned char *bytes; uint32_t used, capacity; };
struct mcc_pcm { short *samples; uint32_t frames, rate; int channels, vorbis_owned; };
struct mcc_ima { int value, step; };

/* The IMA ADPCM quantizer tables are part of the public codec format. */
static int const mcc_ima_steps[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,
    80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,
    494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,
    2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,
    8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,
    27086,29794,32767
};
static int const mcc_ima_changes[8] = {-1,-1,-1,-1,2,4,6,8};

static int mcc_ima_advance(struct mcc_ima *state, unsigned code)
{
    int quantizer=mcc_ima_steps[state->step];
    int delta=quantizer/8;
    if (code&1) delta+=quantizer/4;
    if (code&2) delta+=quantizer/2;
    if (code&4) delta+=quantizer;
    state->value+=(code&8) ? -delta : delta;
    state->value=PIN(state->value,-32768,32767);
    state->step=PIN(state->step+mcc_ima_changes[code&7],0,88);
    return state->value;
}

static unsigned mcc_ima_quantize(struct mcc_ima *state, int sample)
{
    int error=sample-state->value, divisor=mcc_ima_steps[state->step];
    unsigned code=error<0 ? 8 : 0, bit;
    if (error<0) error=-error;
    for (bit=4; bit; bit>>=1,divisor>>=1) {
        if (error>=divisor) {code|=bit; error-=divisor;}
    }
    mcc_ima_advance(state,code);
    return code;
}

static int mcc_pcm_open(unsigned char const *bytes, uint32_t size, int compression,
    int channels, uint32_t rate, struct mcc_pcm *pcm)
{
    uint32_t i;
    memset(pcm,0,sizeof(*pcm));
    pcm->channels=channels; pcm->rate=rate;
    if (compression==3) {
        pcm->vorbis_owned=1;
        return mcc_vorbis_decode(bytes,size,MCC_AUDIO_FRAME_LIMIT,
            &pcm->samples,&pcm->frames,&pcm->rate,&pcm->channels);
    }
    if (compression==0) {
        if (size%(2*channels)) return 0;
        pcm->frames=size/(2*channels);
    } else if (compression==1) {
        if (size%(36*channels)) return 0;
        pcm->frames=size/(36*channels)*64;
    } else return 0;
    if (!pcm->frames || pcm->frames>MCC_AUDIO_FRAME_LIMIT) return 0;
    pcm->samples=malloc(pcm->frames*channels*sizeof(short));
    if (!pcm->samples) return 0;
    if (!compression) {
        for (i=0;i<pcm->frames*(uint32_t)channels;i++)
            pcm->samples[i]=(short)(bytes[i*2]|bytes[i*2+1]<<8);
    } else {
        for (i=0;i<pcm->frames/64;i++) {
            int channel;
            unsigned char const *block=bytes+i*36*channels;
            for (channel=0;channel<channels;channel++) {
                struct mcc_ima state;
                uint32_t frame;
                state.value=(short)(block[channel*4]|block[channel*4+1]<<8);
                state.step=block[channel*4+2];
                if (state.step>88 || block[channel*4+3]) return 0;
                for (frame=0;frame<64;frame++) {
                    unsigned char packed=block[channels*4+(frame/8*channels+channel)*4+frame%8/2];
                    unsigned nibble=(packed>>(frame%2*4))&15;
                    pcm->samples[(i*64+frame)*channels+channel]=(short)mcc_ima_advance(&state,nibble);
                }
            }
        }
    }
    return 1;
}

static void mcc_pcm_close(struct mcc_pcm *pcm)
{
    /* mcc_vorbis uses the C runtime allocator; cseries remaps this module's free
     * to debug_free. Preserve ownership even after the permutation is converted
     * to ADPCM, so each allocation returns to the allocator that created it. */
    if (pcm->samples) {
        if (pcm->vorbis_owned) mcc_vorbis_free(pcm->samples);
        else free(pcm->samples);
    }
    pcm->samples=NULL;
}

static int mcc_pcm_sample(struct mcc_pcm const *pcm, uint32_t frame, int channel, int channels)
{
    if (pcm->channels==1) return pcm->samples[frame];
    if (channels==1) return ((int)pcm->samples[frame*2]+pcm->samples[frame*2+1])/2;
    return pcm->samples[frame*2+channel];
}

static int mcc_resampled(struct mcc_pcm const *pcm, uint32_t frame, int channel, int channels, uint32_t rate)
{
    uint64_t position=(uint64_t)frame*pcm->rate;
    uint32_t first=(uint32_t)(position/rate), fraction=(uint32_t)(position%rate), next;
    int a,b;
    if (first>=pcm->frames) first=pcm->frames-1;
    next=first+1<pcm->frames ? first+1 : first;
    a=mcc_pcm_sample(pcm,first,channel,channels);
    b=mcc_pcm_sample(pcm,next,channel,channels);
    return a+(int)((int64_t)(b-a)*fraction/rate);
}

static int mcc_audio_store(struct mcc_audio_storage *storage, struct mcc_pcm const *pcm,
    int channels, uint32_t rate, struct sound_permutation *permutation)
{
    uint64_t frames64=((uint64_t)pcm->frames*rate+pcm->rate-1)/pcm->rate;
    uint32_t frames,blocks,size,block;
    struct mcc_ima states[2]={{0,0},{0,0}};
    if (!frames64 || frames64>MCC_AUDIO_FRAME_LIMIT) return 0;
    frames=(uint32_t)frames64; blocks=(frames+63)/64; size=blocks*36*channels;
    if (size>MCC_AUDIO_LIMIT-storage->used) return 0;
    if (storage->used+size>storage->capacity) {
        uint32_t capacity=(storage->used+size+0xFFFFF)&~0xFFFFFu;
        unsigned char *larger=realloc(storage->bytes,capacity);
        if (!larger) return 0;
        storage->bytes=larger; storage->capacity=capacity;
    }
    for (block=0;block<blocks;block++) {
        unsigned char *out=storage->bytes+storage->used+block*36*channels;
        int channel;
        for (channel=0;channel<channels;channel++) {
            uint32_t pair;
            out[channel*4]=(unsigned char)states[channel].value;
            out[channel*4+1]=(unsigned char)(states[channel].value>>8);
            out[channel*4+2]=(unsigned char)states[channel].step;
            out[channel*4+3]=0;
            for (pair=0;pair<32;pair++) {
                uint32_t f=block*64+pair*2;
                unsigned lo=mcc_ima_quantize(&states[channel],mcc_resampled(pcm,MIN(f,frames-1),channel,channels,rate));
                unsigned hi=mcc_ima_quantize(&states[channel],mcc_resampled(pcm,MIN(f+1,frames-1),channel,channels,rate));
                out[channels*4+(pair/4*channels+channel)*4+pair%4]=(unsigned char)(lo|(hi<<4));
            }
        }
    }
    permutation->samples.file_offset=MCC_AUDIO_BASE+storage->used;
    permutation->samples.size=size;
    permutation->sample_buffer_size=0;
    permutation->compression=1;
    storage->used+=size;
    return 1;
}

/* Keep validated external ADPCM byte-for-byte, avoiding a lossy round trip. */
static int mcc_audio_store_raw(struct mcc_audio_storage *storage,
    unsigned char const *bytes,struct sound_permutation *permutation)
{
    uint32_t size=(uint32_t)permutation->samples.size;
    if (size>MCC_AUDIO_LIMIT-storage->used) return 0;
    if (storage->used+size>storage->capacity) {
        uint32_t capacity=(storage->used+size+0xFFFFF)&~0xFFFFFu;
        unsigned char *larger=realloc(storage->bytes,capacity);
        if (!larger) return 0;
        storage->bytes=larger;storage->capacity=capacity;
    }
    memcpy(storage->bytes+storage->used,bytes,size);
    permutation->samples.file_offset=MCC_AUDIO_BASE+storage->used;
    permutation->samples.pad&=~1;
    storage->used+=size;
    return 1;
}

int mcc_audio_prepare(struct mcc_runtime *runtime)
{
    struct mcc_audio_storage *storage;
    uint32_t index,converted=0;
    long failed_range=NONE,failed_permutation=NONE,failed_codec=NONE;
    char const *stage="sound header", *tag_name="(unavailable)";
    if (runtime->source.size>=MCC_AUDIO_BASE) return 0;
    storage=malloc(sizeof(*storage));
    if (!storage) return 0;
    memset(storage,0,sizeof(*storage));
    runtime->audio=storage;
    for (index=0;index<runtime->report.tag_count;index++) {
        uint32_t *entry=(uint32_t *)(runtime->tag_index+index*0x20);
        struct sound_definition *sound;
        uint32_t range_index,rate,output_rate;
        int channels;
        if (entry[0]!=(uint32_t)SOUND_DEFINITION_TAG) continue;
        failed_range=failed_permutation=failed_codec=NONE;
        stage="sound header";
        tag_name=mcc_runtime_pointer(runtime,entry[4],256);
        if (!tag_name || !memchr(tag_name,0,256)) tag_name="(unavailable)";
        sound=mcc_runtime_pointer(runtime,entry[5],sizeof(*sound));
        if (!sound || sound->encoding<0 || sound->encoding>1 || sound->sample_rate<0 || sound->sample_rate>1) goto failed;
        channels=sound->encoding+1; rate=sound->sample_rate ? 44100 : 22050;
        output_rate=channels==1 ? 22050 : rate;
        for (range_index=0;range_index<(uint32_t)sound->pitch_ranges.count;range_index++) {
            struct sound_pitch_range *range=mcc_runtime_pointer(runtime,
                (uint32_t)sound->pitch_ranges.address+range_index*sizeof(*range),sizeof(*range));
            uint32_t permutation_index;
            failed_range=range_index;stage="pitch range";
            if (!range) goto failed;
            for (permutation_index=0;permutation_index<(uint32_t)range->permutations.count;permutation_index++) {
                struct sound_permutation *permutation=mcc_runtime_pointer(runtime,
                    (uint32_t)range->permutations.address+permutation_index*sizeof(*permutation),sizeof(*permutation));
                unsigned char *data;
                struct mcc_pcm pcm;
                int ok,external;
                char resource_name[288];
                failed_permutation=permutation_index;stage="permutation header";
                failed_codec=permutation ? permutation->compression : NONE;
                if (!permutation || permutation->samples.size<0 || permutation->samples.size>0x4000000) goto failed;
                external=(permutation->samples.pad&1)!=0;
                permutation->unknown0=NONE;
                permutation->unknown1=0;
                permutation->unknown2=entry[3];
                permutation->unknown3=entry[3];
                permutation->samples.address=NULL;
                permutation->sample_buffer_size=0;
                if (!permutation->samples.size) continue;
                data=malloc(permutation->samples.size);
                stage="input allocation";
                if (!data) goto failed;
                stage="sample read";
                if (external) {
                    stage="external samples (matching mcc_maps/sounds.map required)";
                    snprintf(resource_name,sizeof(resource_name),"%s__%lu__%lu",tag_name,range_index,permutation_index);
                    ok=mcc_resources_read(runtime,2,resource_name,permutation->samples.file_offset,permutation->samples.size,data);
                } else ok=mcc_runtime_read(runtime,permutation->samples.file_offset,permutation->samples.size,data);
                if (!ok) {
                    free(data); goto failed;
                }
                stage="codec decode or sample format";
                ok=mcc_pcm_open(data,permutation->samples.size,permutation->compression,channels,rate,&pcm);
                if (ok && (permutation->compression!=1 || rate!=output_rate)) {
                    stage="ADPCM output allocation or encoding";
                    ok=mcc_audio_store(storage,&pcm,channels,output_rate,permutation);
                    if (ok) converted++;
                } else if (ok && external) {
                    stage="external ADPCM allocation";
                    ok=mcc_audio_store_raw(storage,data,permutation);
                    if (ok) converted++;
                }
                free(data);
                if (ok) permutation->samples.pad&=~1;
                mcc_pcm_close(&pcm);
                if (!ok) goto failed;
            }
        }
        sound->compression=1;
        sound->sample_rate=output_rate==44100 ? 1 : 0;
    }
    error(_error_silent,"mcc audio: %lu permutations converted, %lu ADPCM bytes",converted,storage->used);
    return 1;
failed:
    error(_error_silent,"mcc audio: sound tag #%lu '%s', range %ld, permutation %ld, codec %ld: %s; map refused",
        index,tag_name,failed_range,failed_permutation,failed_codec,stage);
    return 0;
}

int mcc_audio_read(struct mcc_runtime *runtime,uint32_t offset,uint32_t bytes,void *out)
{
    struct mcc_audio_storage *storage=runtime->audio;
    if (offset<MCC_AUDIO_BASE || offset>=MCC_AUDIO_BASE+MCC_AUDIO_LIMIT) return -1;
    offset-=MCC_AUDIO_BASE;
    if (!storage || offset>storage->used || bytes>storage->used-offset) return 0;
    memcpy(out,storage->bytes+offset,bytes);
    return 1;
}

int mcc_audio_contains(struct mcc_runtime *runtime,uint32_t offset,uint32_t bytes)
{
    struct mcc_audio_storage *storage=runtime->audio;
    return storage && offset>=MCC_AUDIO_BASE && offset-MCC_AUDIO_BASE<=storage->used &&
        bytes<=storage->used-(offset-MCC_AUDIO_BASE);
}

uint32_t mcc_audio_stream_end(struct mcc_runtime *runtime)
{
    struct mcc_audio_storage *storage=runtime->audio;
    return storage && storage->used ? MCC_AUDIO_BASE+storage->used : (uint32_t)runtime->source.size;
}

void mcc_audio_dispose(struct mcc_runtime *runtime)
{
    struct mcc_audio_storage *storage=runtime->audio;
    if (storage) {
        if (storage->bytes) free(storage->bytes);
        free(storage); runtime->audio=NULL;
    }
}
