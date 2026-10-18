#include "mac_voice_dsp.h"
#include <math.h>
#include <string.h>
int mac_voice_ring_init(struct mac_voice_ring *ring)
{
    memset(ring->data,0,sizeof(ring->data));ring->read=ring->count=0;
    return pthread_mutex_init(&ring->lock,NULL)==0;
}
void mac_voice_ring_destroy(struct mac_voice_ring *ring){pthread_mutex_destroy(&ring->lock);}
void mac_voice_ring_clear(struct mac_voice_ring *ring)
{
    pthread_mutex_lock(&ring->lock);ring->read=ring->count=0;pthread_mutex_unlock(&ring->lock);
}
void mac_voice_ring_write(struct mac_voice_ring *ring,const float *samples,unsigned count)
{
    unsigned index;
    pthread_mutex_lock(&ring->lock);
    /* (more than the ring holds: only the latest) */
    if(count>MAC_VOICE_RING_SAMPLES){samples+=count-MAC_VOICE_RING_SAMPLES;count=MAC_VOICE_RING_SAMPLES;}
    for(index=0;index<count;++index) {
        if(ring->count==MAC_VOICE_RING_SAMPLES) {
            ring->read=(ring->read+1)%MAC_VOICE_RING_SAMPLES;--ring->count;
        }
        ring->data[(ring->read+ring->count)%MAC_VOICE_RING_SAMPLES]=samples[index];
        ++ring->count;
    }
    pthread_mutex_unlock(&ring->lock);
}
unsigned mac_voice_ring_read(struct mac_voice_ring *ring,float *samples,unsigned maximum)
{
    unsigned index,take;
    pthread_mutex_lock(&ring->lock);
    take=ring->count<maximum?ring->count:maximum;
    for(index=0;index<take;++index)
        samples[index]=ring->data[(ring->read+index)%MAC_VOICE_RING_SAMPLES];
    ring->read=(ring->read+take)%MAC_VOICE_RING_SAMPLES;ring->count-=take;
    pthread_mutex_unlock(&ring->lock);
    return take;
}
unsigned mac_voice_ring_trim(struct mac_voice_ring *ring,unsigned keep)
{
    unsigned drop=0;
    pthread_mutex_lock(&ring->lock);
    if(ring->count>keep) {
        drop=ring->count-keep;
        ring->read=(ring->read+drop)%MAC_VOICE_RING_SAMPLES;ring->count=keep;
    }
    pthread_mutex_unlock(&ring->lock);
    return drop;
}
unsigned mac_voice_ring_available(struct mac_voice_ring *ring)
{
    unsigned count;
    pthread_mutex_lock(&ring->lock);count=ring->count;pthread_mutex_unlock(&ring->lock);
    return count;
}
int mac_voice_resampler_init(struct mac_voice_resampler *resampler,double in_rate,double out_rate)
{
    memset(resampler,0,sizeof(*resampler));
    if(!(in_rate>=8000.0&&in_rate<=192000.0&&out_rate>=8000.0&&out_rate<=192000.0))return 0;
    resampler->step=in_rate/out_rate;
    resampler->averaging=resampler->step>1.0+1e-9;
    return 1;
}
unsigned mac_voice_resampler_process(struct mac_voice_resampler *resampler,const float *in,unsigned in_count,
    float *out,unsigned maximum)
{
    unsigned produced=0,index;
    if(resampler->averaging) {
        for(index=0;index<in_count;++index) {
            double remaining=1.0;
            while(remaining>1e-12) {
                double space=resampler->step-resampler->filled;
                double take=remaining<space?remaining:space;
                resampler->sum+=(double)in[index]*take;resampler->filled+=take;remaining-=take;
                if(resampler->filled>=resampler->step-1e-9) {
                    if(produced<maximum)out[produced++]=(float)(resampler->sum/resampler->step);
                    resampler->sum=0.0;resampler->filled=0.0;
                }
            }
        }
        return produced;
    }
    if(!in_count)return 0;
    for(;;) {
        double floor_position=floor(resampler->position);
        long base=(long)floor_position;
        float a,b,fraction;
        if(base+1>=(long)in_count||produced>=maximum)break;
        a=base<0?resampler->previous:in[base];b=in[base+1];
        fraction=(float)(resampler->position-floor_position);
        out[produced++]=a+(b-a)*fraction;
        resampler->position+=resampler->step;
    }
    resampler->position-=(double)in_count;
    if(resampler->position<-1.0)resampler->position=-1.0;
    resampler->previous=in[in_count-1];
    return produced;
}
