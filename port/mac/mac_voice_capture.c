/* Tiger's Component Manager + AudioUnit API (as mac_audio_output.c), the HAL
   output unit with its input bus on. */
#include "mac_voice_capture.h"
#include "mac_voice_dsp.h"
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/AudioHardware.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXIMUM_CHANNELS 8
/* the most frames one render may ask for; a larger one is dropped */
#define MAXIMUM_FRAMES 16384
struct capture_state {
    AudioUnit unit;
    int initialized,running;
    unsigned channels;
    double rate;
    struct mac_voice_ring ring;
    struct mac_voice_resampler resampler;
    float *input;   /* MAXIMUM_FRAMES * channels, the render's */
    float *mono;    /* MAXIMUM_FRAMES */
    float *resampled;
    unsigned resampled_size;
};
static OSStatus input_ready(void *opaque,AudioUnitRenderActionFlags *flags,const AudioTimeStamp *time,
    UInt32 bus,UInt32 frames,AudioBufferList *unused)
{
    struct capture_state *state=(struct capture_state *)opaque;
    AudioBufferList list;OSStatus error;UInt32 frame,channel,count;
    (void)unused;
    if(!frames||frames>MAXIMUM_FRAMES)return noErr;
    list.mNumberBuffers=1;list.mBuffers[0].mNumberChannels=state->channels;
    list.mBuffers[0].mDataByteSize=frames*state->channels*(UInt32)sizeof(float);
    list.mBuffers[0].mData=state->input;
    error=AudioUnitRender(state->unit,flags,time,bus,frames,&list);
    if(error)return error;
    /* (native-endian float, as asked of the unit: no byte swapping on PowerPC) */
    for(frame=0;frame<frames;++frame) {
        float sum=0.0f;
        for(channel=0;channel<state->channels;++channel)sum+=state->input[frame*state->channels+channel];
        state->mono[frame]=sum/(float)state->channels;
    }
    count=(UInt32)mac_voice_resampler_process(&state->resampler,state->mono,frames,state->resampled,
        state->resampled_size);
    mac_voice_ring_write(&state->ring,state->resampled,count);
    return noErr;
}
/* the input device called name (CoreAudio's own name for it) */
static int find_input_device(const char *name,AudioDeviceID *found)
{
    UInt32 bytes=0,count,index;AudioDeviceID *devices;int result=0;
    if(!name||!*name||!strcmp(name,"default"))return 0;
    if(AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&bytes,NULL)!=noErr||
        bytes<sizeof(AudioDeviceID)||bytes%sizeof(AudioDeviceID))return 0;
    devices=(AudioDeviceID *)malloc(bytes);
    if(!devices)return 0;
    if(AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&bytes,devices)!=noErr){free(devices);return 0;}
    count=bytes/(UInt32)sizeof(AudioDeviceID);
    for(index=0;index<count&&!result;index++) {
        UInt32 stream_bytes=0,name_bytes=128;char device_name[128];
        if(AudioDeviceGetPropertyInfo(devices[index],0,true,kAudioDevicePropertyStreams,&stream_bytes,NULL)!=noErr||
            stream_bytes==0)continue;
        memset(device_name,0,sizeof(device_name));
        if(AudioDeviceGetProperty(devices[index],0,true,kAudioDevicePropertyDeviceName,&name_bytes,device_name)!=noErr)continue;
        device_name[sizeof(device_name)-1]=0;
        if(!strcmp(device_name,name)){*found=devices[index];result=1;}
    }
    free(devices);
    return result;
}
static void release(struct capture_state *state)
{
    if(state->unit) {
        if(state->running)AudioOutputUnitStop(state->unit);
        if(state->initialized)AudioUnitUninitialize(state->unit);
        CloseComponent(state->unit);
    }
    mac_voice_ring_destroy(&state->ring);
    free(state->input);free(state->mono);free(state->resampled);free(state);
}
int mac_voice_capture_open(struct mac_voice_capture *capture,const char *device_name,long *status)
{
    ComponentDescription description;Component component;AudioDeviceID device=0;
    AudioStreamBasicDescription hardware,format;AURenderCallbackStruct callback;
    struct capture_state *state;OSStatus error=paramErr;UInt32 flag,size;
    if(status)*status=paramErr;
    if(!capture||capture->state)return 0;
    state=(struct capture_state *)calloc(1,sizeof(*state));
    if(!state){if(status)*status=memFullErr;return 0;}
    if(!mac_voice_ring_init(&state->ring)){free(state);if(status)*status=memFullErr;return 0;}
    memset(&description,0,sizeof(description));description.componentType=kAudioUnitType_Output;
    description.componentSubType=kAudioUnitSubType_HALOutput;description.componentManufacturer=kAudioUnitManufacturer_Apple;
    component=FindNextComponent(NULL,&description);
    if(!component){error=badComponentType;goto fail;}
    error=OpenAComponent(component,&state->unit);if(error)goto fail;
    /* (the input bus on, the output bus off, before the device is set) */
    flag=1;
    error=AudioUnitSetProperty(state->unit,kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Input,1,&flag,sizeof(flag));
    if(error)goto fail;
    flag=0;
    error=AudioUnitSetProperty(state->unit,kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Output,0,&flag,sizeof(flag));
    if(error)goto fail;
    if(!find_input_device(device_name,&device)) {
        size=sizeof(device);
        error=AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice,&size,&device);
        if(error)goto fail;
    } else fprintf(stderr,"audio: listening on %s\n",device_name);
    error=AudioUnitSetProperty(state->unit,kAudioOutputUnitProperty_CurrentDevice,kAudioUnitScope_Global,0,&device,sizeof(device));
    if(error)goto fail;
    /* (the device's own rate and channels: the unit converts neither rate nor
       channel count to ours on input, so this code does) */
    size=sizeof(hardware);memset(&hardware,0,sizeof(hardware));
    error=AudioUnitGetProperty(state->unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,1,&hardware,&size);
    if(error)goto fail;
    state->rate=hardware.mSampleRate;state->channels=(unsigned)hardware.mChannelsPerFrame;
    if(state->channels<1||state->channels>MAXIMUM_CHANNELS||
        !mac_voice_resampler_init(&state->resampler,state->rate,MAC_VOICE_RATE)){error=kAudioUnitErr_FormatNotSupported;goto fail;}
    memset(&format,0,sizeof(format));format.mSampleRate=state->rate;format.mFormatID=kAudioFormatLinearPCM;
    format.mFormatFlags=kAudioFormatFlagsNativeFloatPacked;format.mChannelsPerFrame=state->channels;
    format.mBitsPerChannel=32;format.mFramesPerPacket=1;
    format.mBytesPerFrame=format.mBytesPerPacket=4*state->channels;
    error=AudioUnitSetProperty(state->unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,1,&format,sizeof(format));
    if(error)goto fail;
    state->resampled_size=(unsigned)((double)MAXIMUM_FRAMES/state->resampler.step)+4;
    state->input=(float *)malloc((size_t)MAXIMUM_FRAMES*state->channels*sizeof(float));
    state->mono=(float *)malloc((size_t)MAXIMUM_FRAMES*sizeof(float));
    state->resampled=(float *)malloc((size_t)state->resampled_size*sizeof(float));
    if(!state->input||!state->mono||!state->resampled){error=memFullErr;goto fail;}
    callback.inputProc=input_ready;callback.inputProcRefCon=state;
    error=AudioUnitSetProperty(state->unit,kAudioOutputUnitProperty_SetInputCallback,kAudioUnitScope_Global,0,&callback,sizeof(callback));
    if(error)goto fail;
    error=AudioUnitInitialize(state->unit);if(error)goto fail;
    state->initialized=1;
    error=AudioOutputUnitStart(state->unit);if(error)goto fail;
    state->running=1;
    capture->state=state;if(status)*status=noErr;return 1;
fail:
    release(state);if(status)*status=error;return 0;
}
unsigned mac_voice_capture_read(struct mac_voice_capture *capture,float *samples,unsigned maximum)
{
    if(!capture||!capture->state)return 0;
    return mac_voice_ring_read(&((struct capture_state *)capture->state)->ring,samples,maximum);
}
unsigned mac_voice_capture_available(struct mac_voice_capture *capture)
{
    if(!capture||!capture->state)return 0;
    return mac_voice_ring_available(&((struct capture_state *)capture->state)->ring);
}
unsigned mac_voice_capture_trim(struct mac_voice_capture *capture,unsigned keep)
{
    if(!capture||!capture->state)return 0;
    return mac_voice_ring_trim(&((struct capture_state *)capture->state)->ring,keep);
}
int mac_voice_capture_format(struct mac_voice_capture *capture,double *rate,unsigned *channels)
{
    struct capture_state *state;
    if(!capture||!capture->state)return 0;
    state=(struct capture_state *)capture->state;
    if(rate)*rate=state->rate;
    if(channels)*channels=state->channels;
    return 1;
}
void mac_voice_capture_close(struct mac_voice_capture *capture)
{
    if(!capture||!capture->state)return;
    release((struct capture_state *)capture->state);capture->state=NULL;
}
