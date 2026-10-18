#include "engine_xdk_prefix.h"
#include "voice_audio.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#undef fprintf
#undef printf
#undef vfprintf
#undef memset

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); } } while (0)

/* The probe needs only an ordinary host diagnostic sink. */
void platform_log(const char *format,...)
{
	va_list args;
	va_start(args,format);
	vfprintf(stderr,format,args);
	va_end(args);
	fputc('\n',stderr);
}

const char *config_string(const char *name)
{
	(void)name;
	return "default";
}

int main(void)
{
	float frame[VOICE_FRAME_SAMPLES], output[VOICE_FRAME_SAMPLES * 6];
	unsigned char packet[VOICE_MAXIMUM_PACKET];
	int i, length;
	double left=0.0, right=0.0;
	for (i=0;i<VOICE_FRAME_SAMPLES;i++)
		frame[i]=0.3f*sinf((float)i*0.071f);
	/* (the microphone is never opened here: closed, it gives nothing) */
	CHECK(!voice_audio_microphone(0));
	CHECK(!voice_audio_read_frame(frame));
	length=voice_audio_encode(frame,24000,packet,sizeof(packet));
	CHECK(length>0 && length<=VOICE_MAXIMUM_PACKET);
	CHECK(!voice_audio_speaking(41));
	voice_audio_play(41,0,packet,length,1.0f,-1.0f);
	voice_audio_play(41,1,packet,length,1.0f,-1.0f);
	CHECK(voice_audio_speaking(41));
	voice_audio_play(41,2,packet,length,1.0f,-1.0f);
	CHECK(voice_audio_speaking(41));
	for (i=0;i<VOICE_FRAME_SAMPLES*6;i++) output[i]=0.0f;
	voice_audio_mix(output,VOICE_FRAME_SAMPLES*3);
	for(i=0;i<VOICE_FRAME_SAMPLES*3;i++)
	{
		left+=(double)output[i*2]*output[i*2];
		right+=(double)output[i*2+1]*output[i*2+1];
	}
	CHECK(left>0.001);
	CHECK(right<0.000001);
	CHECK(!voice_audio_speaking(41));
	voice_audio_forget(41);
	CHECK(!voice_audio_speaking(41));
	voice_audio_forget_all();
	printf("Mac voice audio: %d checks, %d failures\n",checks,failures);
	return failures?1:0;
}
