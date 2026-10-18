/* Records the microphone through voice chat's own path (voice_audio_microphone
   and voice_audio_read_frame, the frames the Opus encoder is given), writing
   them to a file as native-endian float32, 960 samples each, and reports each
   second's level. Run: probe <seconds> <file>. HALO_AUDIO_INPUT_DEVICE names
   the device. Hidden: no window. */
#include "engine_xdk_prefix.h"
#include "voice_audio.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
int usleep(unsigned int);
#undef fprintf
#undef printf
#undef vfprintf
#undef memset

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
	const char *value=NULL;
	if(!strcmp(name,"audio.input_device"))value=getenv("HALO_AUDIO_INPUT_DEVICE");
	return value&&value[0]?value:"default";
}

int main(int argc,char **argv)
{
	float frame[VOICE_FRAME_SAMPLES];
	unsigned char packet[VOICE_MAXIMUM_PACKET];
	FILE *file;
	int seconds,frames=0,second_frames=0,total_bytes=0,nonzero=0,i;
	int ticks,opened=0;
	double second_sum=0.0,total_sum=0.0,second_peak=0.0,total_peak=0.0;
	if(argc!=3){fprintf(stderr,"usage: %s seconds file\n",argv[0]);return 2;}
	seconds=atoi(argv[1]);
	file=fopen(argv[2],"wb");
	if(!file){perror("fopen");return 2;}
	for(ticks=0;ticks<seconds*200;++ticks) {
		/* (as network_voice.c: asked each tick, read while it has frames) */
		if(!opened||ticks%100==0)opened=voice_audio_microphone(1);
		while(opened&&voice_audio_read_frame(frame)) {
			int length;
			for(i=0;i<VOICE_FRAME_SAMPLES;++i) {
				double magnitude=fabs((double)frame[i]);
				second_sum+=(double)frame[i]*frame[i];
				if(magnitude>second_peak)second_peak=magnitude;
				if(frame[i]!=0.0f)++nonzero;
			}
			fwrite(frame,sizeof(float),VOICE_FRAME_SAMPLES,file);
			length=voice_audio_encode(frame,24000,packet,sizeof(packet));
			total_bytes+=length;
			++frames;++second_frames;
			if(second_frames==50) {
				printf("second: rms %.6f peak %.6f\n",sqrt(second_sum/(50.0*VOICE_FRAME_SAMPLES)),second_peak);
				total_sum+=second_sum;if(second_peak>total_peak)total_peak=second_peak;
				second_sum=second_peak=0.0;second_frames=0;
			}
		}
		usleep(5000);
	}
	voice_audio_microphone(0);
	fclose(file);
	printf("frames %d (expected about %d) encoded bytes %d nonzero samples %d peak %.6f\n",frames,seconds*50,
		total_bytes,nonzero,total_peak);
	return frames>0?0:1;
}
