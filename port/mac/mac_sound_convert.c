/* Xbox ADPCM conversion of sound permutations for the Mac session
   (mac_sound_convert.h). Registered permutations keep a pointer into the
   mapped map or sounds.map and the size their ADPCM will have. A low
   priority thread converts them ahead of need: the permutations after one
   the sound cache reads (a looping sound's next chunks), and the first chunk
   of each music sound when its tag is loaded, within a budget. The cache's
   read takes a converted chunk from memory; one not converted yet is waited
   for if the thread is converting it, or converted by the reader if it is
   only queued, so the reader waits for that one chunk and never for a
   queue. The conversion is custom_edition_sounds_encode (Ogg Vorbis with
   stb_vorbis); converted chunks are kept within a budget of the least
   recently read. */
#include "mac_sound_convert.h"
#include "engine_cache_files.h"
#include <mach/mach_time.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* port/linux/game/custom_edition_sounds.c (a buffer of the malloc heap) */
extern unsigned char *custom_edition_sounds_encode(const unsigned char *data,long data_bytes,
    short compression,unsigned char big_endian,long channels,long rate,long encoded_rate,
    unsigned long *encoded_bytes);
extern uint32_t (*mac_native_sound_convert_hook)(const unsigned char *,size_t,int,int,int,int,uint32_t,uint32_t *);
extern int (*mac_cache_files_converted_read_hook)(uint32_t,uint32_t,void *);
extern void (*mac_cache_files_converted_reset_hook)(void);

#define ADPCM_BLOCK_BYTES 36
#define ADPCM_BLOCK_SAMPLES 64
#define SPACE_BYTES (MAC_CACHE_BITMAP_BASE-MAC_CACHE_DECODED_BASE)
#define CACHE_BUDGET (12u<<20)        /* converted chunks kept */
#define LOAD_PREFETCH_BUDGET (4u<<20) /* first chunks of music queued at load */
#define PREFETCH_AHEAD 3              /* chunks after the one read */
#define MAXIMUM_FRAMES 0x1000000ul

enum { IDLE, QUEUED, RUNNING };

struct entry {
    const unsigned char *samples;
    size_t bytes;
    uint32_t offset,size;          /* in the converted space, from its start */
    uint32_t group;                /* the sound's tag handle */
    int compression,channels,source_rate,target_rate;
    unsigned char *kept;           /* converted */
    unsigned state;
    uint32_t tick;
};

static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t work=PTHREAD_COND_INITIALIZER,done=PTHREAD_COND_INITIALIZER;
static int thread_started,worker_busy;
static unsigned generation;
static struct entry *entries;
static unsigned long entry_count,entry_capacity;
static unsigned long *queue,queue_count,queue_capacity;
static uint32_t space_used,tick,kept_bytes,peak_kept,load_prefetch_bytes;
static unsigned long background_conversions,inline_conversions,conversion_failures,waits;
static uint64_t background_ticks,inline_ticks,wait_ticks;

/* frames an Ogg Vorbis stream has: the last page's granule position */
static int ogg_frames(const unsigned char *d,size_t n,unsigned long *frames,unsigned *rate)
{
    size_t id,p;uint64_t granule=0;unsigned i;
    if(n<64||memcmp(d,"OggS",4))return 0;
    for(id=0;id+16<n&&id<512;++id)if(d[id]==1&&!memcmp(d+id+1,"vorbis",6))break;
    if(id>=512||id+16>=n)return 0;
    *rate=d[id+12]|(unsigned)d[id+13]<<8|(unsigned)d[id+14]<<16|(unsigned)d[id+15]<<24;
    if(!*rate||*rate>192000)return 0;
    for(p=n-4;p>0;--p) {
        if(n-p>70000)return 0;
        if(!memcmp(d+p,"OggS",4)&&p+14<=n)break;
    }
    if(!p)return 0;
    for(i=0;i<8;++i)granule|=(uint64_t)d[p+6+i]<<(8*i);
    if(granule==0||granule>MAXIMUM_FRAMES)return 0;
    *frames=(unsigned long)granule;return 1;
}

/* converts a permutation to its advertised size; a failed one is silence.
   No lock is held and the entry is not touched: the arguments are copies. */
static unsigned char *convert(const struct entry *e,int *failed)
{
    unsigned char *out=(unsigned char *)calloc(1,e->size?e->size:1),*encoded;
    unsigned long encoded_bytes=0;
    *failed=0;
    if(!out)return NULL;
    encoded=custom_edition_sounds_encode(e->samples,(long)e->bytes,(short)e->compression,0,
        e->channels,e->source_rate,e->target_rate,&encoded_bytes);
    if(encoded) {
        memcpy(out,encoded,encoded_bytes<e->size?encoded_bytes:e->size);
        free(encoded);
        {   /* HALO_MAC_SOUND_CONVERT_DUMP=<directory> (diagnostic) */
            const char *directory=getenv("HALO_MAC_SOUND_CONVERT_DUMP");
            static unsigned long dumped;
            if(directory&&dumped<40) {
                char path[1200];FILE *file;unsigned long n=++dumped;
                snprintf(path,sizeof(path),"%s/conv%03lu_c%d_ch%d_%d.src",directory,n,e->compression,e->channels,e->target_rate);
                if((file=fopen(path,"wb"))!=NULL){fwrite(e->samples,1,e->bytes,file);fclose(file);}
                snprintf(path,sizeof(path),"%s/conv%03lu.adpcm",directory,n);
                if((file=fopen(path,"wb"))!=NULL){fwrite(out,1,e->size,file);fclose(file);}
            }
        }
    } else *failed=1;
    return out;
}

/* (lock held) forget a queued entry */
static void unqueue(unsigned long index)
{
    unsigned long i;
    for(i=0;i<queue_count;++i)
        if(queue[i]==index){memmove(queue+i,queue+i+1,(queue_count-i-1)*sizeof(*queue));--queue_count;break;}
    if(entries[index].state==QUEUED)entries[index].state=IDLE;
}

/* (lock held) queue an entry: at the front for the chunks after one read, at
   the back for the first chunks of music at load */
static void enqueue(unsigned long index,int front)
{
    struct entry *e=&entries[index];
    if(e->kept||e->state!=IDLE)return;
    if(queue_count==queue_capacity) {
        unsigned long capacity=queue_capacity?queue_capacity*2:256;
        unsigned long *grown=(unsigned long *)realloc(queue,capacity*sizeof(*queue));
        if(!grown)return;
        queue=grown;queue_capacity=capacity;
    }
    if(front){memmove(queue+1,queue,queue_count*sizeof(*queue));queue[0]=index;}
    else queue[queue_count]=index;
    ++queue_count;e->state=QUEUED;
    pthread_cond_signal(&work);
}

/* (lock held) make room for `size` more kept bytes by dropping the least
   recently read chunks; FALSE if it cannot be made */
static int make_room(uint32_t size)
{
    while(kept_bytes+size>CACHE_BUDGET) {
        unsigned long i,oldest=entry_count;
        for(i=0;i<entry_count;++i)
            if(entries[i].kept&&(oldest==entry_count||entries[i].tick<entries[oldest].tick))oldest=i;
        if(oldest==entry_count)return 0;
        free(entries[oldest].kept);entries[oldest].kept=NULL;kept_bytes-=entries[oldest].size;
    }
    return 1;
}

/* (lock held) keep a converted chunk, which becomes the entry's; FALSE (and
   the caller keeps it) when the budget cannot hold it */
static int keep(struct entry *e,unsigned char *data,int evict)
{
    if(kept_bytes+e->size>CACHE_BUDGET&&(!evict||!make_room(e->size)))return 0;
    e->kept=data;kept_bytes+=e->size;e->tick=++tick;
    if(kept_bytes>peak_kept)peak_kept=kept_bytes;
    return 1;
}

static void *worker(void *unused)
{
    (void)unused;
    pthread_mutex_lock(&lock);
    for(;;) {
        unsigned long index;struct entry copy;unsigned gen;unsigned char *out;int failed;uint64_t start;
        while(!queue_count)pthread_cond_wait(&work,&lock);
        index=queue[0];memmove(queue,queue+1,(queue_count-1)*sizeof(*queue));--queue_count;
        if(entries[index].state!=QUEUED)continue;
        if(entries[index].kept||kept_bytes+entries[index].size>CACHE_BUDGET){entries[index].state=IDLE;continue;}
        entries[index].state=RUNNING;copy=entries[index];gen=generation;worker_busy=1;
        pthread_mutex_unlock(&lock);
        start=mach_absolute_time();
        out=convert(&copy,&failed);
        start=mach_absolute_time()-start;
        pthread_mutex_lock(&lock);
        worker_busy=0;
        if(gen==generation) {
            struct entry *e=&entries[index];
            e->state=IDLE;background_ticks+=start;++background_conversions;conversion_failures+=failed;
            if(!out||!keep(e,out,0))free(out);
        } else free(out);
        pthread_cond_broadcast(&done);
    }
    return NULL;
}

static void start_thread(void)
{
    pthread_attr_t attr;pthread_t thread;struct sched_param param;
    if(thread_started)return;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr,2u<<20);
    if(!pthread_attr_getschedparam(&attr,&param)) {
        param.sched_priority=sched_get_priority_min(SCHED_OTHER);
        pthread_attr_setschedparam(&attr,&param);
    }
    if(!pthread_create(&thread,&attr,worker,NULL))thread_started=1;
    pthread_attr_destroy(&attr);
}

static uint32_t register_permutation(const unsigned char *samples,size_t bytes,int compression,
    int channels,int source_rate,int target_rate,uint32_t group,uint32_t *converted_bytes)
{
    unsigned long frames;unsigned rate=(unsigned)source_rate;struct entry *e;
    uint32_t size,result;
    if(!samples||!bytes||bytes>0x7fffffffu||channels<1||channels>2)return 0;
    if(compression==3) {
        if(!ogg_frames(samples,bytes,&frames,&rate))return 0;
    } else if(compression==0) {
        frames=(unsigned long)(bytes/(2*(size_t)channels));
    } else if(compression==1) {
        frames=(unsigned long)(bytes/(ADPCM_BLOCK_BYTES*(size_t)channels))*ADPCM_BLOCK_SAMPLES;
    } else return 0;
    if(!frames||frames>MAXIMUM_FRAMES)return 0;
    if((unsigned)target_rate!=rate)frames=(unsigned long)((double)frames*target_rate/rate);
    if(!frames)return 0;
    size=(uint32_t)((frames+ADPCM_BLOCK_SAMPLES-1)/ADPCM_BLOCK_SAMPLES)*ADPCM_BLOCK_BYTES*channels;
    pthread_mutex_lock(&lock);
    if(size>SPACE_BYTES-space_used){pthread_mutex_unlock(&lock);return 0;}
    if(entry_count==entry_capacity) {
        unsigned long capacity=entry_capacity?entry_capacity*2:1024;
        struct entry *grown=(struct entry *)realloc(entries,capacity*sizeof(*entries));
        if(!grown){pthread_mutex_unlock(&lock);return 0;}
        entries=grown;entry_capacity=capacity;
    }
    e=&entries[entry_count];memset(e,0,sizeof(*e));
    e->samples=samples;e->bytes=bytes;e->compression=compression;e->channels=channels;
    e->source_rate=(int)rate;e->target_rate=target_rate;e->group=group;
    e->offset=space_used;e->size=size;space_used+=(size+3u)&~3u;
    *converted_bytes=size;result=MAC_CACHE_DECODED_BASE+e->offset;
    /* the first chunk of a music sound (stereo at 44 kHz) is converted at load */
    if(compression==3&&channels==2&&target_rate==44100&&
       (!entry_count||entries[entry_count-1].group!=group)&&load_prefetch_bytes+size<=LOAD_PREFETCH_BUDGET) {
        load_prefetch_bytes+=size;start_thread();
        if(thread_started)enqueue(entry_count,0);
    }
    ++entry_count;
    pthread_mutex_unlock(&lock);
    return result;
}

/* (lock held) the chunks after entry `index` in its sound, looping round, are
   converted next */
static void prefetch_after(unsigned long index)
{
    unsigned long low=index,high=index,k;
    uint32_t group=entries[index].group;
    while(low>0&&index-low<64&&entries[low-1].group==group)--low;
    while(high+1<entry_count&&high-index<64&&entries[high+1].group==group)++high;
    if(high==low)return;
    start_thread();
    if(!thread_started)return;
    for(k=PREFETCH_AHEAD;k>=1;--k) {
        unsigned long next=index+k;
        if(next>high)next=low+(next-high-1);
        if(next==index||next<low||next>high)continue;
        if(!entries[next].kept&&entries[next].state==IDLE&&kept_bytes+entries[next].size<=CACHE_BUDGET)
            enqueue(next,1);
    }
}

static int converted_read(uint32_t offset,uint32_t size,void *destination)
{
    unsigned long low=0,high,index;struct entry *e;int result=0;
    pthread_mutex_lock(&lock);
    high=entry_count;
    while(low<high) {
        unsigned long mid=(low+high)/2;
        if(entries[mid].offset+entries[mid].size<=offset)low=mid+1;else high=mid;
    }
    if(low>=entry_count){pthread_mutex_unlock(&lock);return 0;}
    index=low;e=&entries[index];
    if(offset<e->offset||size>e->size||offset-e->offset>e->size-size){pthread_mutex_unlock(&lock);return 0;}
    for(;;) {
        if(e->kept) {
            memcpy(destination,e->kept+(offset-e->offset),size);
            e->tick=++tick;result=1;
            prefetch_after(index);
            break;
        }
        if(e->state==RUNNING) {
            /* the thread has begun this chunk: wait for it (only it) */
            uint64_t start=mach_absolute_time();
            ++waits;
            pthread_cond_wait(&done,&lock);
            wait_ticks+=mach_absolute_time()-start;
            e=&entries[index];
            continue;
        } else {
            /* not begun: converted here, and the thread keeps to the rest */
            struct entry copy;unsigned char *out;int failed;uint64_t start;unsigned gen=generation;
            if(e->state==QUEUED)unqueue(index);
            e->state=RUNNING;copy=*e;
            pthread_mutex_unlock(&lock);
            start=mach_absolute_time();
            out=convert(&copy,&failed);
            start=mach_absolute_time()-start;
            pthread_mutex_lock(&lock);
            e=&entries[index];e->state=IDLE;
            if(gen!=generation||!out){free(out);break;}
            inline_ticks+=start;++inline_conversions;conversion_failures+=failed;
            memcpy(destination,out+(offset-e->offset),size);result=1;
            if(!keep(e,out,1))free(out);
            prefetch_after(index);
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    return result;
}

static void converted_reset(void)
{
    unsigned long i;
    pthread_mutex_lock(&lock);
    ++generation;queue_count=0;
    while(worker_busy)pthread_cond_wait(&done,&lock);
    if(entry_count) {
        mach_timebase_info_data_t base;double scale;
        mach_timebase_info(&base);scale=(double)base.numer/base.denom/1e6;
        fprintf(stderr,"sound convert: %lu permutations registered (%lu bytes of ADPCM when read, table %lu bytes), "
            "%lu in the background (%.0f ms), %lu by the reader (%.0f ms), reader waited for the thread %lu times (%.0f ms), "
            "%lu failed, at most %lu bytes kept\n",entry_count,(unsigned long)space_used,
            (unsigned long)(entry_capacity*sizeof(*entries)),background_conversions,background_ticks*scale,
            inline_conversions,inline_ticks*scale,waits,wait_ticks*scale,conversion_failures,(unsigned long)peak_kept);
    }
    for(i=0;i<entry_count;++i)free(entries[i].kept);
    free(entries);entries=NULL;entry_count=entry_capacity=0;
    space_used=tick=kept_bytes=peak_kept=load_prefetch_bytes=0;
    background_conversions=inline_conversions=conversion_failures=waits=0;
    background_ticks=inline_ticks=wait_ticks=0;
    pthread_mutex_unlock(&lock);
}

void mac_sound_convert_install(void)
{
    const char *disabled=getenv("HALO_MAC_SOUND_CONVERT");
    if(disabled&&disabled[0]=='0'&&!disabled[1])return; /* A/B: sounds as the map has them */
    mac_native_sound_convert_hook=register_permutation;
    mac_cache_files_converted_read_hook=converted_read;
    mac_cache_files_converted_reset_hook=converted_reset;
}
