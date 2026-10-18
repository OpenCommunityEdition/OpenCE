/* Tiger's Component Manager + AudioUnit API; no AudioComponent/SDL3 dependency. */
#include "mac_audio_output.h"
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/AudioHardware.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
struct mac_audio_state {
    AudioUnit unit;
    mac_audio_fill_proc fill;
    void *context;
    pthread_mutex_t lock;
    struct mac_audio_output_stats stats;
    int initialized,running;
};
static OSStatus render(void *opaque,AudioUnitRenderActionFlags *flags,
    const AudioTimeStamp *time,UInt32 bus,UInt32 frames,AudioBufferList *buffers)
{
    struct mac_audio_state *state=(struct mac_audio_state *)opaque;
    unsigned i;uint32_t frame;int ok=0,format_ok;uint64_t nonzero_frames=0;
    (void)time;(void)bus;
    format_ok=buffers && buffers->mNumberBuffers==1 &&
        buffers->mBuffers[0].mNumberChannels==2 && buffers->mBuffers[0].mData &&
        frames<=0xffffffffU/8 && buffers->mBuffers[0].mDataByteSize>=frames*8;
    if(format_ok)ok=state->fill(state->context,(float *)buffers->mBuffers[0].mData,frames);
    /* HALO_MAC_AUDIO_CAPTURE=<file> (diagnostic): what the callback gave the
       output, native-endian float32 stereo at 48 kHz (not for release use) */
    if(format_ok && ok) {
        static FILE *capture;static int capture_checked;
        if(!capture_checked) {
            const char *path=getenv("HALO_MAC_AUDIO_CAPTURE");
            capture_checked=1;if(path)capture=fopen(path,"wb");
        }
        if(capture)fwrite(buffers->mBuffers[0].mData,8,frames,capture);
    }
    if(format_ok && ok) {
        const float *samples=(const float *)buffers->mBuffers[0].mData;
        for(frame=0;frame<frames;++frame)
            if(samples[frame*2]!=0.0f || samples[frame*2+1]!=0.0f)++nonzero_frames;
    }
    if(!ok && buffers) {
        for(i=0;i<buffers->mNumberBuffers;++i)
            if(buffers->mBuffers[i].mData)memset(buffers->mBuffers[i].mData,0,buffers->mBuffers[i].mDataByteSize);
        if(flags)*flags|=kAudioUnitRenderAction_OutputIsSilence;
    }
    pthread_mutex_lock(&state->lock);
    ++state->stats.callbacks;
    if(format_ok)state->stats.frames+=frames;
    else ++state->stats.format_errors;
    state->stats.nonzero_frames+=nonzero_frames;
    if(!ok)++state->stats.silent_failures;
    pthread_mutex_unlock(&state->lock);
    return format_ok?noErr:paramErr;
}
/* port: the output device called name (CoreAudio's own name for it, as
   engine_audio_devices.c lists them) */
static int find_output_device(const char *name,AudioDeviceID *found)
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
        if(AudioDeviceGetPropertyInfo(devices[index],0,false,kAudioDevicePropertyStreams,&stream_bytes,NULL)!=noErr||
            stream_bytes==0)continue;
        memset(device_name,0,sizeof(device_name));
        if(AudioDeviceGetProperty(devices[index],0,false,kAudioDevicePropertyDeviceName,&name_bytes,device_name)!=noErr)continue;
        device_name[sizeof(device_name)-1]=0;
        if(!strcmp(device_name,name)){*found=devices[index];result=1;}
    }
    free(devices);
    return result;
}
int mac_audio_output_open(struct mac_audio_output *output,mac_audio_fill_proc fill,void *context,long *status)
{
    return mac_audio_output_open_device(output,fill,context,status,NULL);
}
int mac_audio_output_open_device(struct mac_audio_output *output,mac_audio_fill_proc fill,void *context,long *status,const char *device_name)
{
    ComponentDescription description;Component component;AudioDeviceID device=0;int named;
    AudioStreamBasicDescription format;AURenderCallbackStruct callback;
    struct mac_audio_state *state;OSStatus error;
    if(status)*status=paramErr;
    if(!output||output->state||!fill)return 0;
    state=(struct mac_audio_state *)calloc(1,sizeof(*state));
    if(!state){if(status)*status=memFullErr;return 0;}
    if(pthread_mutex_init(&state->lock,NULL)) {free(state);if(status)*status=memFullErr;return 0;}
    state->fill=fill;state->context=context;
    memset(&description,0,sizeof(description));description.componentType=kAudioUnitType_Output;
    description.componentSubType=kAudioUnitSubType_DefaultOutput;description.componentManufacturer=kAudioUnitManufacturer_Apple;
    named=find_output_device(device_name,&device);
    if(named)description.componentSubType=kAudioUnitSubType_HALOutput;
    component=FindNextComponent(NULL,&description);
    if(!component){error=badComponentType;goto fail;}
    error=OpenAComponent(component,&state->unit);if(error)goto fail;
    if(named) {
        /* (the HAL output unit: its input bus off, then the device; one that
           will not take it plays on the default output instead) */
        UInt32 off=0;
        OSStatus bad=AudioUnitSetProperty(state->unit,kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Input,1,&off,sizeof(off));
        if(!bad)bad=AudioUnitSetProperty(state->unit,kAudioOutputUnitProperty_CurrentDevice,kAudioUnitScope_Global,0,&device,sizeof(device));
        if(bad) {
            fprintf(stderr,"audio: the output device \"%s\" could not be used (%ld); the default is\n",device_name,(long)bad);
            CloseComponent(state->unit);state->unit=NULL;
            description.componentSubType=kAudioUnitSubType_DefaultOutput;
            component=FindNextComponent(NULL,&description);
            if(!component){error=badComponentType;goto fail;}
            error=OpenAComponent(component,&state->unit);if(error)goto fail;
        } else fprintf(stderr,"audio: playing on %s\n",device_name);
    }
    memset(&format,0,sizeof(format));format.mSampleRate=48000;format.mFormatID=kAudioFormatLinearPCM;
    format.mFormatFlags=kAudioFormatFlagsNativeFloatPacked;format.mBytesPerPacket=8;
    format.mFramesPerPacket=1;format.mBytesPerFrame=8;format.mChannelsPerFrame=2;format.mBitsPerChannel=32;
    error=AudioUnitSetProperty(state->unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,0,&format,sizeof(format));
    if(error)goto fail;
    callback.inputProc=render;callback.inputProcRefCon=state;
    error=AudioUnitSetProperty(state->unit,kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&callback,sizeof(callback));
    if(error)goto fail;
    error=AudioUnitInitialize(state->unit);if(error)goto fail;
    state->initialized=1;output->state=state;if(status)*status=noErr;return 1;
fail:
    if(state->unit)CloseComponent(state->unit);
    pthread_mutex_destroy(&state->lock);free(state);if(status)*status=error;return 0;
}
int mac_audio_output_start(struct mac_audio_output *output,long *status)
{
    struct mac_audio_state *state;OSStatus error;
    if(status)*status=paramErr;
    if(!output||!output->state)return 0;state=(struct mac_audio_state *)output->state;
    if(state->running)return 0;
    error=AudioOutputUnitStart(state->unit);if(status)*status=error;
    if(error)return 0;state->running=1;return 1;
}
int mac_audio_output_stop(struct mac_audio_output *output,long *status)
{
    struct mac_audio_state *state;OSStatus error;
    if(status)*status=paramErr;
    if(!output||!output->state)return 0;state=(struct mac_audio_state *)output->state;
    if(!state->running){if(status)*status=noErr;return 1;}
    error=AudioOutputUnitStop(state->unit);if(status)*status=error;
    if(error)return 0;state->running=0;return 1;
}
int mac_audio_output_statistics(struct mac_audio_output *output,struct mac_audio_output_stats *stats)
{
    struct mac_audio_state *state;
    if(!output||!output->state||!stats)return 0;
    state=(struct mac_audio_state *)output->state;
    pthread_mutex_lock(&state->lock);*stats=state->stats;pthread_mutex_unlock(&state->lock);return 1;
}
int mac_audio_output_close(struct mac_audio_output *output,long *status)
{
    struct mac_audio_state *state;OSStatus error;
    if(status)*status=paramErr;
    if(!output)return 0;
    if(!output->state){if(status)*status=noErr;return 1;}
    state=(struct mac_audio_state *)output->state;
    if(!mac_audio_output_stop(output,status))return 0;
    if(state->initialized) {
        error=AudioUnitUninitialize(state->unit);
        if(error){if(status)*status=error;return 0;}
        state->initialized=0;
    }
    error=CloseComponent(state->unit);
    if(error){if(status)*status=error;return 0;}
    pthread_mutex_destroy(&state->lock);free(state);output->state=NULL;
    if(status)*status=noErr;return 1;
}
