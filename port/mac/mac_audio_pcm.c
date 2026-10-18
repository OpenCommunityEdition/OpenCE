#include "mac_audio_pcm.h"
int mac_audio_pcm16le_decode(const void *source,size_t bytes,unsigned channels,
    int16_t *output,size_t output_samples,size_t *frames)
{
    const unsigned char *input=(const unsigned char *)source;
    uintptr_t input_address=(uintptr_t)source,output_address=(uintptr_t)output;
    size_t sample,samples=bytes/2;
    if(!frames)return 0;*frames=0;
    if((channels!=1&&channels!=2)||bytes%(2*channels)||samples>output_samples)return 0;
    if(bytes) {
        if(!source||!output||output_address%sizeof(*output)||
            input_address>UINTPTR_MAX-bytes||output_address>UINTPTR_MAX-bytes)return 0;
        if(input_address<output_address+bytes && output_address<input_address+bytes)return 0;
    }
    for(sample=0;sample<samples;++sample) {
        unsigned value=(unsigned)input[sample*2]|((unsigned)input[sample*2+1]<<8);
        output[sample]=(int16_t)(value>=32768U?(int)value-65536:(int)value);
    }
    *frames=samples/channels;return 1;
}
