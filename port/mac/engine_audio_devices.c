/* CoreAudio's real device list for the menu's audio-device selectors. */
#include <CoreAudio/AudioHardware.h>
#include <stdlib.h>
#include <string.h>

#define PLATFORM_AUDIO_DEVICE_NAME_SIZE 128

int platform_audio_devices(int recording,char (*names)[PLATFORM_AUDIO_DEVICE_NAME_SIZE],int maximum)
{
	UInt32 bytes=0,count=0,index;
	AudioDeviceID *devices;
	int result=0;
	Boolean input=recording!=0;
	if(!names||maximum<=0)return 0;
	if(AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&bytes,NULL)!=noErr||
		bytes<sizeof(AudioDeviceID)||bytes%sizeof(AudioDeviceID))return 0;
	devices=(AudioDeviceID *)malloc(bytes);
	if(!devices)return 0;
	if(AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&bytes,devices)!=noErr) {
		free(devices);return 0;
	}
	count=bytes/(UInt32)sizeof(AudioDeviceID);
	for(index=0;index<count&&result<maximum;index++) {
		UInt32 stream_bytes=0,name_bytes=PLATFORM_AUDIO_DEVICE_NAME_SIZE;
		char name[PLATFORM_AUDIO_DEVICE_NAME_SIZE];
		if(AudioDeviceGetPropertyInfo(devices[index],0,input,kAudioDevicePropertyStreams,
			&stream_bytes,NULL)!=noErr||stream_bytes==0)continue;
		memset(name,0,sizeof(name));
		if(AudioDeviceGetProperty(devices[index],0,input,kAudioDevicePropertyDeviceName,
			&name_bytes,name)!=noErr)continue;
		name[sizeof(name)-1]='\0';
		if(!name[0]||strlen(name)>=sizeof(name)-1||strchr(name,'|'))continue;
		memcpy(names[result],name,sizeof(name));
		names[result][sizeof(name)-1]='\0';
		result++;
	}
	free(devices);
	return result;
}
