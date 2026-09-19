#ifndef BINK_AUDIO_H
#define BINK_AUDIO_H

struct BINKAUDIO;
struct BINKAUDIO *__stdcall BinkAudioDecompressOpen(
	unsigned long frequency,
	unsigned long channels,
	unsigned long flags);
void __stdcall BinkAudioDecompressClose(
	struct BINKAUDIO *decoder);

void __stdcall BinkAudioDecompress(
	struct BINKAUDIO *decoder,
	void **output,
	unsigned long *output_bytes,
	const unsigned char *input,
	const unsigned char **next_input);

struct BINKAUDIOCOMP;
void __stdcall BinkAudioCompressLock(
	struct BINKAUDIOCOMP *compressor,
	short **samples,
	unsigned long *bytes);
void __stdcall BinkAudioCompressClose(
	struct BINKAUDIOCOMP *compressor);
void __stdcall BinkAudioDecompressLock(
	struct BINKAUDIO *decoder,
	unsigned char **buffer,
	unsigned long *bytes);
void __stdcall BinkAudioDecompressUnlock(
	struct BINKAUDIO *decoder,
	void **output,
	unsigned long *output_bytes);

#endif
