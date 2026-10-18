#include <stdio.h>
#include <string.h>
#define PLATFORM_AUDIO_DEVICE_NAME_SIZE 128
int platform_audio_devices(int recording,char (*names)[PLATFORM_AUDIO_DEVICE_NAME_SIZE],int maximum);
int main(void)
{
	char names[2][PLATFORM_AUDIO_DEVICE_NAME_SIZE];
	int failures=0,checks=0,count,index;
	checks++;if(platform_audio_devices(0,NULL,2)!=0)failures++;
	checks++;if(platform_audio_devices(1,names,0)!=0)failures++;
	memset(names,0xa5,sizeof(names));
	count=platform_audio_devices(0,names,2);
	checks++;if(count<0||count>2)failures++;
	for(index=0;index<count;index++) {
		checks++;
		if(names[index][PLATFORM_AUDIO_DEVICE_NAME_SIZE-1]!='\0'||names[index][0]=='\0')failures++;
	}
	memset(names,0xa5,sizeof(names));
	count=platform_audio_devices(1,names,2);
	checks++;if(count<0||count>2)failures++;
	for(index=0;index<count;index++) {
		checks++;
		if(names[index][PLATFORM_AUDIO_DEVICE_NAME_SIZE-1]!='\0'||names[index][0]=='\0')failures++;
	}
	printf("mac audio-device enumeration: %d checks, %d failures (recording devices=%d)\n",checks,failures,count);
	return failures?1:0;
}
