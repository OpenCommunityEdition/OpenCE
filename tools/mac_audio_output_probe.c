/* Hardware output is deliberately silent; no microphone or audible tone. */
#include "mac_audio_output.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"audio FAIL %s status=%ld\n",#x,status);}}while(0)
static int silence(void *context,float *samples,uint32_t frames)
{
    (void)context;memset(samples,0,(size_t)frames*2*sizeof(float));return 1;
}
static int reject(void *context,float *samples,uint32_t frames)
{(void)context;(void)samples;(void)frames;return 0;}
int main(void)
{
    struct mac_audio_output output={0};struct mac_audio_output_stats before,after;
    long status=0;unsigned cycle;
    CHECK(!mac_audio_output_start(&output,&status));
    CHECK(!mac_audio_output_open(&output,NULL,NULL,&status));
    for(cycle=0;cycle<2;++cycle) {
        int opened=mac_audio_output_open(&output,cycle?reject:silence,NULL,&status);
        CHECK(opened);if(!opened)break;
        CHECK(!mac_audio_output_open(&output,silence,NULL,&status));
        CHECK(mac_audio_output_statistics(&output,&before));CHECK(before.callbacks==0);
        CHECK(mac_audio_output_start(&output,&status));
        usleep(250000);
        CHECK(mac_audio_output_stop(&output,&status));
        CHECK(mac_audio_output_statistics(&output,&after));
        CHECK(after.callbacks>0 && after.frames>0 && after.format_errors==0);
        CHECK(cycle?after.silent_failures==after.callbacks:after.silent_failures==0);
        CHECK(after.nonzero_frames==0);
        before=after;usleep(30000);CHECK(mac_audio_output_statistics(&output,&after));
        CHECK(before.callbacks==after.callbacks && before.frames==after.frames);
        printf("CoreAudio cycle=%u callbacks=%llu frames=%llu nonzero_frames=%llu silence_fallbacks=%llu format_errors=%u\n",
            cycle,(unsigned long long)after.callbacks,(unsigned long long)after.frames,
            (unsigned long long)after.nonzero_frames,
            (unsigned long long)after.silent_failures,after.format_errors);
        CHECK(mac_audio_output_close(&output,&status));CHECK(output.state==NULL);
    }
    CHECK(mac_audio_output_close(&output,&status));
    printf("native audio output: %u checks, %u failures; silent default-output test only\n",checks,failures);
    return failures?1:0;
}
