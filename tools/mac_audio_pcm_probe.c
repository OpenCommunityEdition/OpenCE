#include "mac_audio_pcm.h"
#include <stdio.h>
#include <string.h>
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"PCM FAIL %s\n",#x);}}while(0)
int main(void)
{
    /* Deliberately unaligned encoded source and signed extrema/interior values. */
    unsigned char raw[]={0x55,0,0x80,0xff,0x7f,0x34,0x12,0xcc,0xed,0xff,0xff,0,0};
    int16_t output[8],sentinel[8];size_t frames;unsigned i;
    const int16_t expected[]={-32768,32767,4660,-4660,-1,0};
    CHECK(sizeof(int16_t)==2);
    memset(output,0x5a,sizeof(output));memcpy(sentinel,output,sizeof(output));
    CHECK(mac_audio_pcm16le_decode(raw+1,12,1,output,8,&frames));CHECK(frames==6);
    for(i=0;i<6;++i)CHECK(output[i]==expected[i]);
    CHECK(output[6]==sentinel[6]&&output[7]==sentinel[7]);
    CHECK(mac_audio_pcm16le_decode(raw+1,12,2,output,8,&frames));CHECK(frames==3);
    for(i=0;i<6;++i)CHECK(output[i]==expected[i]);
    memcpy(sentinel,output,sizeof(output));
    CHECK(!mac_audio_pcm16le_decode(raw+1,11,1,output,8,&frames)&&frames==0);
    CHECK(!memcmp(output,sentinel,sizeof(output)));
    CHECK(!mac_audio_pcm16le_decode(raw+1,10,2,output,8,&frames)&&frames==0);
    CHECK(!memcmp(output,sentinel,sizeof(output)));
    CHECK(!mac_audio_pcm16le_decode(raw+1,12,0,output,8,&frames));
    CHECK(!mac_audio_pcm16le_decode(raw+1,12,3,output,8,&frames));
    CHECK(!mac_audio_pcm16le_decode(raw+1,12,2,output,5,&frames));
    CHECK(!memcmp(output,sentinel,sizeof(output)));
    CHECK(!mac_audio_pcm16le_decode(NULL,12,2,output,8,&frames));
    CHECK(!mac_audio_pcm16le_decode(raw+1,12,2,NULL,8,&frames));
    CHECK(!mac_audio_pcm16le_decode(raw+1,12,2,output,8,NULL));
    CHECK(!mac_audio_pcm16le_decode(output,12,2,output,8,&frames));
    CHECK(!mac_audio_pcm16le_decode(raw+1,12,2,(int16_t *)((unsigned char *)output+1),7,&frames));
    CHECK(!memcmp(output,sentinel,sizeof(output)));
    CHECK(mac_audio_pcm16le_decode(NULL,0,2,NULL,0,&frames)&&frames==0);
    printf("native PCM16: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
