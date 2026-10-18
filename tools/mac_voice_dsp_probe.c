/* Host unit tests of the microphone's ring and resampler (no audio device). */
#include "mac_voice_dsp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static int checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);}}while(0)
static struct mac_voice_ring ring;
static void *producer(void *unused)
{
    float block[100];unsigned sent=0,i;
    (void)unused;
    while(sent<200000) {
        for(i=0;i<100;++i)block[i]=(float)(sent+i);
        mac_voice_ring_write(&ring,block,100);sent+=100;
        /* (never more than a quarter second ahead of the reader) */
        while(mac_voice_ring_available(&ring)>12000)usleep(100);
    }
    return NULL;
}
/* a sine of freq at rate, count samples, starting at sample start */
static void sine(float *out,unsigned start,unsigned count,double freq,double rate)
{
    unsigned i;
    for(i=0;i<count;++i)out[i]=(float)(0.5*sin(2.0*3.14159265358979323846*freq*(double)(start+i)/rate));
}
static unsigned run(double in_rate,const float *in,unsigned n,unsigned chunk,float *out,unsigned max)
{
    struct mac_voice_resampler r;unsigned done=0,total=0;
    CHECK(mac_voice_resampler_init(&r,in_rate,48000.0));
    while(done<n) {
        unsigned take=n-done<chunk?n-done:chunk;
        total+=mac_voice_resampler_process(&r,in+done,take,out+total,max-total);
        done+=take;
    }
    return total;
}
/* (the position is carried in doubles: chunks differ by rounding only) */
static int near(const float *x,const float *y,unsigned n)
{
    unsigned i;
    for(i=0;i<n;++i)if(fabs(x[i]-y[i])>1e-4)return 0;
    return 1;
}
static unsigned crossings(const float *x,unsigned n)
{
    unsigned i,c=0;
    for(i=1;i<n;++i)if(x[i-1]<0.0f&&x[i]>=0.0f)++c;
    return c;
}
int main(void)
{
    static float in[200000],a[200000],b[200000],r[MAC_VOICE_RING_SAMPLES+8],w[MAC_VOICE_RING_SAMPLES+8];
    unsigned i,n,m;pthread_t thread;float last;double peak;
    /* ring */
    CHECK(mac_voice_ring_init(&ring));
    for(i=0;i<10;++i)w[i]=(float)i;
    mac_voice_ring_write(&ring,w,10);
    CHECK(mac_voice_ring_available(&ring)==10);
    CHECK(mac_voice_ring_read(&ring,r,4)==4&&r[0]==0.0f&&r[3]==3.0f);
    CHECK(mac_voice_ring_trim(&ring,2)==4);
    CHECK(mac_voice_ring_read(&ring,r,10)==2&&r[0]==8.0f&&r[1]==9.0f);
    CHECK(mac_voice_ring_read(&ring,r,10)==0);
    /* wrapping around the end, and overflow dropping the oldest */
    for(i=0;i<MAC_VOICE_RING_SAMPLES;++i)w[i]=(float)i;
    mac_voice_ring_write(&ring,w,MAC_VOICE_RING_SAMPLES-5);
    mac_voice_ring_read(&ring,r,MAC_VOICE_RING_SAMPLES-10);
    mac_voice_ring_write(&ring,w,20);
    CHECK(mac_voice_ring_available(&ring)==25);
    CHECK(mac_voice_ring_read(&ring,r,25)==25&&r[0]==(float)(MAC_VOICE_RING_SAMPLES-10)&&r[5]==0.0f&&r[24]==19.0f);
    mac_voice_ring_write(&ring,w,10);
    mac_voice_ring_write(&ring,w,MAC_VOICE_RING_SAMPLES);
    CHECK(mac_voice_ring_available(&ring)==MAC_VOICE_RING_SAMPLES);
    mac_voice_ring_write(&ring,w,3);
    CHECK(mac_voice_ring_read(&ring,r,MAC_VOICE_RING_SAMPLES)==MAC_VOICE_RING_SAMPLES);
    CHECK(r[0]==3.0f&&r[MAC_VOICE_RING_SAMPLES-4]==(float)(MAC_VOICE_RING_SAMPLES-1)&&r[MAC_VOICE_RING_SAMPLES-1]==2.0f);
    mac_voice_ring_clear(&ring);CHECK(mac_voice_ring_available(&ring)==0);
    /* a producer thread and a reader: every sample once, in order */
    CHECK(pthread_create(&thread,NULL,producer,NULL)==0);
    n=0;last=-1.0f;m=1;
    while(n<200000) {
        unsigned got=mac_voice_ring_read(&ring,r,512);
        for(i=0;i<got;++i){if(r[i]!=last+1.0f)m=0;last=r[i];}
        n+=got;if(!got)usleep(50);
    }
    pthread_join(thread,NULL);
    CHECK(m==1);
    mac_voice_ring_destroy(&ring);
    /* resampler: rejects rates it should */
    {struct mac_voice_resampler rs;CHECK(!mac_voice_resampler_init(&rs,0.0,48000.0));CHECK(!mac_voice_resampler_init(&rs,48000.0,1.0));}
    /* 44100 -> 48000: length, tone, amplitude, and the same in any chunking */
    sine(in,0,44100,1000.0,44100.0);
    n=run(44100.0,in,44100,44100,a,200000);
    CHECK(n>=47998&&n<=48000);
    CHECK(crossings(a,n)>=999&&crossings(a,n)<=1001);
    for(peak=0.0,i=0;i<n;++i)if(fabs(a[i])>peak)peak=fabs(a[i]);
    CHECK(peak>0.49&&peak<=0.5001);
    m=run(44100.0,in,44100,37,b,200000);
    CHECK(m==n&&near(a,b,m<n?m:n));
    m=run(44100.0,in,44100,512,b,200000);
    CHECK(m==n&&near(a,b,m<n?m:n));
    /* 48000 -> 48000 passes samples through (the last waits for the next chunk) */
    sine(in,0,4800,440.0,48000.0);
    n=run(48000.0,in,4800,1000,a,200000);
    CHECK(n==4799);
    for(m=1,i=0;i<n;++i)if(fabs(a[i]-in[i])>1e-6)m=0;
    CHECK(m==1);
    /* 96000 -> 48000 averages: a tone, and DC unchanged */
    sine(in,0,96000,2000.0,96000.0);
    n=run(96000.0,in,96000,1000,a,200000);
    CHECK(n==48000&&crossings(a,n)>=1999&&crossings(a,n)<=2001);
    for(i=0;i<96000;++i)in[i]=0.25f;
    n=run(96000.0,in,96000,333,a,200000);
    for(m=1,i=0;i<n;++i)if(fabs(a[i]-0.25f)>1e-5)m=0;
    CHECK(n==48000&&m==1);
    /* 44100 -> 48000 averaging not used; 192000 -> 48000 is; 11025 -> 48000 upsamples */
    n=run(192000.0,in,96000,4096,a,200000);
    CHECK(n==24000);
    sine(in,0,11025,500.0,11025.0);
    n=run(11025.0,in,11025,11025,a,200000);
    CHECK(n>=47995&&n<=48000&&crossings(a,n)>=499&&crossings(a,n)<=501);
    /* a silent device stays silent */
    memset(in,0,sizeof(in));
    n=run(44100.0,in,44100,441,a,200000);
    for(m=1,i=0;i<n;++i)if(a[i]!=0.0f)m=0;
    CHECK(m==1);
    printf("Mac voice dsp: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
