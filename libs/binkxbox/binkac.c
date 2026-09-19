/* January Bink decoder-initialization and audio lock/close packet. Transform and compression/decoding
 * implementation is not yet reconstructed; this is NOT a complete decoder.
 */
#include "binkaudio_internal.h"
#include "radmem.h"
#include <math.h>
#include <string.h>

/* Original frequency-band boundaries, BINKDATA + 0x68 (25 DWORDs).
 * Kept as a named table, not an alias to the splitter's anonymous block.
 */
#pragma data_seg("BINKDATA")
static unsigned long audio_band_frequencies[25]=
{
	0, 100, 200, 300, 400, 510, 630, 770, 920, 1080,
	1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700,
	4400, 5300, 6400, 7700, 9500, 12000, 15500
};
#pragma data_seg()

#pragma code_seg("BINK")
struct BINKAUDIO *__stdcall BinkAudioDecompressOpen(
	unsigned long frequency,
	unsigned long channels,
	unsigned long flags)
{
	/* A valid nonzero sample rate/channel count is required by the target.
	 * The low flag bit selects one rather than three 8K buffer units. */
	unsigned long nyquist= (frequency*channels+1)/2;
	struct BINKAUDIO *decoder= radmalloc(sizeof(*decoder)+
		8192*((flags&1) ? 1 : 3));
	if (!decoder)
	{
		return 0;
	}
	{
		unsigned long half;
		unsigned long band;
		memset(decoder, 0, sizeof(*decoder));
		decoder->samples= (short *)(decoder+1);
		decoder->compressed_buffer= (unsigned char *)decoder->samples+8192;
		decoder->compressed_buffer_bytes= 12288;
		if (frequency >= 44100)
		{
			decoder->transform_samples= 2048;
		}
		else if (frequency >= 22050)
		{
			decoder->transform_samples= 1024;
		}
		else
		{
			decoder->transform_samples= 512;
		}
		decoder->transform_samples*= channels;
		half= decoder->transform_samples/2;
		decoder->overlap_samples= decoder->transform_samples/16;
		decoder->transform_scale= (float)(2.0f/(float)sqrt((double)decoder->transform_samples));
		for (band= 0; band < 25 && audio_band_frequencies[band] < nyquist; ++band)
		{
			decoder->bands[band]= (audio_band_frequencies[band]*half)/nyquist;
			if (!decoder->bands[band])
			{
				decoder->bands[band]= 1;
			}
		}
		decoder->band_count= band;
		decoder->bands[band]= half;
		decoder->overlap_valid= 0;
		decoder->first_block= 1;
	}
	return decoder;
}

void __stdcall BinkAudioCompressLock(
	struct BINKAUDIOCOMP *compressor,
	short **samples,
	unsigned long *bytes)
{
	if (compressor->first_block)
	{
		if (samples)
		{
			*samples= compressor->samples;
		}
		if (bytes)
		{
			*bytes= compressor->transform_samples*2;
		}
	}
	else
	{
		if (samples)
		{
			*samples= compressor->samples+compressor->overlap_samples;
		}
		if (bytes)
		{
			*bytes= (compressor->transform_samples-compressor->overlap_samples)*2;
		}
	}
	return;
}

void __stdcall BinkAudioCompressClose(
	struct BINKAUDIOCOMP *compressor)
{
	radfree(compressor);
	return;
}

void __stdcall BinkAudioDecompressLock(
	struct BINKAUDIO *decoder,
	unsigned char **buffer,
	unsigned long *bytes)
{
	if (buffer)
	{
		*buffer= decoder->compressed_buffer;
	}
	if (bytes)
	{
		*bytes= decoder->compressed_buffer_bytes;
	}
	return;
}

void __stdcall BinkAudioDecompressClose(
	struct BINKAUDIO *decoder)
{
	radfree(decoder);
	return;
}

void __stdcall BinkAudioDecompressUnlock(
	struct BINKAUDIO *decoder,
	void **output,
	unsigned long *output_bytes)
{
	BinkAudioDecompress(decoder, output, output_bytes, decoder->compressed_buffer, 0);
	return;
}
