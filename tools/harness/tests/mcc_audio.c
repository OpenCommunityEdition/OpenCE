/* Exercise empty and nonempty MCC sound headers through the real converter. */
#include "cseries.h"
#include "errors.h"
#include "sound/sound_definitions.h"
#include "mcc_runtime.h"
#include "mcc_resources.h"
#include "mcc_loading.h"
#include "mcc_vorbis.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#undef malloc
#undef free
#undef realloc
#undef memcpy
#undef memset
#undef memcmp
static unsigned char tags[4096];
static int allocations,reads,attempts,fail_at;
void *debug_malloc(unsigned int n,boolean clear,char const *file,long line) {
    void *p; if(++attempts==fail_at)return NULL;p=malloc(n);(void)file;(void)line;if(p){allocations++;if(clear)memset(p,0,n);}return p;
}
void debug_free(void *p,char const *file,long line) {(void)file;(void)line;if(p){allocations--;free(p);}}
void *csmemcpy(void *a,void const *b,unsigned long n) {return memcpy(a,b,n);}
void *csmemset(void *a,long v,unsigned long n) {return memset(a,v,n);}
void error(short priority,char const *format,...) {(void)priority;(void)format;}
void mcc_loading_update(enum mcc_loading_stage stage,unsigned long a,unsigned long b) {(void)stage;(void)a;(void)b;}
void mcc_loading_pulse(void) {}
void *mcc_runtime_pointer(struct mcc_runtime *r,uint32_t p,uint32_t n) {
    uint32_t base=(uint32_t)r->tags;
    return p>=base && p-base<=r->used && n<=r->used-(p-base) ? (void *)p : NULL;
}
int mcc_runtime_read(struct mcc_runtime *r,uint32_t o,uint32_t n,void *out) {
    (void)r;reads++;memset(out,(int)o,n);return 1;
}
int mcc_resources_read(struct mcc_runtime *r,unsigned type,char const *name,uint32_t o,uint32_t n,void *out,char const *group) {
    (void)type;(void)name;(void)group;return mcc_runtime_read(r,o,n,out);
}
int mcc_vorbis_decode(unsigned char const *b,uint32_t n,uint32_t limit,short **samples,uint32_t *frames,uint32_t *rate,int *channels) {
    (void)b;(void)n;(void)limit;(void)samples;(void)frames;(void)rate;(void)channels;return 0;
}
void mcc_vorbis_free(short *p) {free(p);}
#define CHECK(c) do{if(!(c)){fprintf(stderr,"line %d\n",__LINE__);return 1;}}while(0)
int main(void) {
    struct mcc_runtime runtime;
    struct sound_definition *sound=(void *)(tags+512);
    struct sound_pitch_range *range=(void *)(tags+1024);
    struct sound_permutation *permutation=(void *)(tags+1536);
    uint32_t *entry=(void *)tags;
    int rate,nonempty,channels;
    for(rate=-2;rate<=2;rate++) for(nonempty=0;nonempty<3;nonempty++) for(channels=0;channels<2;channels++) {
        int expected=rate>=-1 && rate<=1 && (rate!=-1 || nonempty<2);
        memset(&runtime,0,sizeof(runtime));memset(tags,0,sizeof(tags));reads=0;
        runtime.tags=runtime.tag_index=tags;runtime.used=sizeof(tags);runtime.report.tag_count=1;
        entry[0]=SOUND_DEFINITION_TAG;entry[4]=(uint32_t)(tags+128);entry[5]=(uint32_t)sound;
        sound->sample_rate=(short)rate;sound->encoding=(short)channels;
        sound->pitch_ranges.count=1;sound->pitch_ranges.address=range;
        range->permutations.count=nonempty ? 1 : 0;range->permutations.address=permutation;
        permutation->samples.size=nonempty==2 ? 4 : 0;
        CHECK(mcc_audio_prepare(&runtime)==expected);
        if(expected) CHECK(sound->sample_rate>=0 && sound->sample_rate<=1 && sound->compression==1);
        if(rate<0) CHECK(!reads);
        mcc_audio_dispose(&runtime);CHECK(!runtime.audio && !allocations);
        mcc_audio_dispose(&runtime);CHECK(!allocations);
    }
    /* Identical offsets belong to different sound tags, and a reused
     * absolute index with the wrong salt cannot read another sound. */
    for(fail_at=0;fail_at<=8;fail_at++) {
        unsigned char a[36],b[36];
        int i,ok;
        memset(&runtime,0,sizeof(runtime));memset(tags,0,sizeof(tags));attempts=0;
        runtime.tags=runtime.tag_index=tags;runtime.used=sizeof(tags);runtime.report.tag_count=2;
        for(i=0;i<2;i++) {
            uint32_t *e=entry+i*8;
            sound=(void *)(tags+512+i*256);range=(void *)(tags+1024+i*96);
            permutation=(void *)(tags+1536+i*124);
            e[0]=SOUND_DEFINITION_TAG;e[3]=0xe0000000u+i*0x10001u;
            e[4]=(uint32_t)(tags+128);e[5]=(uint32_t)sound;
            sound->pitch_ranges.count=1;sound->pitch_ranges.address=range;
            range->permutations.count=1;range->permutations.address=permutation;
            permutation->samples.size=64;permutation->samples.file_offset=i+1;
        }
        ok=mcc_audio_prepare(&runtime);CHECK(ok==(fail_at==0));
        if(ok) {
            CHECK(mcc_audio_read(&runtime,(long)0xe0000000u,0x50000000u,36,a)==1);
            CHECK(mcc_audio_read(&runtime,(long)0xe0010001u,0x50000000u,36,b)==1);
            CHECK(memcmp(a,b,36));
            CHECK(!mcc_audio_read(&runtime,(long)0xe0020001u,0x50000000u,36,b));
            CHECK(!mcc_audio_read(&runtime,(long)0xe0020002u,0x50000000u,36,b));
            CHECK(!mcc_audio_contains(&runtime,(long)0xe0000000u,0x50000001u,36));
            CHECK(mcc_audio_read(&runtime,(long)0xe0000000u,0x50000024u,0,b)==1);
            CHECK(mcc_audio_read(&runtime,(long)0xe0000000u,16,1,b)==-1);
            CHECK(mcc_audio_stream_end(&runtime)==0x50000024u);
        }
        mcc_audio_dispose(&runtime);CHECK(!runtime.audio && !allocations);
        CHECK(!mcc_audio_contains(&runtime,(long)0xe0000000u,0x50000000u,1));
    }
    return 0;
}
