#include "engine_xdk_prefix.h"
#include <stdio.h>
#include <string.h>

void __stdcall DirectSoundStopStream(LPDIRECTSOUNDSTREAM stream);
unsigned long __stdcall DirectSoundGetStreamVoiceStatus(LPDIRECTSOUNDSTREAM stream);

static unsigned checks;
static unsigned failures;
enum { PACKET_CAPACITY = 64 };

#define CHECK(expression) do { \
	++checks; \
	if (!(expression)) { \
		++failures; \
		fprintf(stderr, "FAIL %s at line %d\n", #expression, __LINE__); \
	} \
} while (0)

int main(void)
{
	WAVEFORMATEX format;
	DSSTREAMDESC description;
	IDirectSoundStream *stream;
	struct _XMEDIAINFO information;
	XMEDIAPACKET packets[PACKET_CAPACITY + 1];
	unsigned long statuses[PACKET_CAPACITY + 1], completed[PACKET_CAPACITY + 1], status;
	short pcm[8];
	unsigned i;
	HRESULT result;

	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = 22050;
	format.nAvgBytesPerSec = 44100;
	format.nBlockAlign = 2;
	format.wBitsPerSample = 16;
	memset(&description, 0, sizeof(description));
	description.dwMaxAttachedPackets = PACKET_CAPACITY;
	description.lpwfxFormat = &format;
	stream = NULL;
	result = IDirectSound_CreateSoundStream(NULL, &description, &stream, NULL);
	CHECK(result == DS_OK && stream != NULL);
	if (result != DS_OK || !stream)
		return 1;

	memset(&information, 0, sizeof(information));
	CHECK(stream->lpVtbl->GetInfo(stream, &information) == S_OK);
	CHECK(information.dwInputSize == 2);
	CHECK((information.dwFlags & XMO_STREAMF_FIXED_SAMPLE_SIZE) != 0);
	CHECK((information.dwFlags & XMO_STREAMF_INPUT_ASYNC) != 0);
	CHECK(stream->lpVtbl->GetStatus(stream, &status) == S_OK);
	CHECK((status & XMO_STATUSF_ACCEPT_INPUT_DATA) != 0);

	memset(pcm, 0, sizeof(pcm));
	memset(packets, 0, sizeof(packets));
	for (i = 0; i < PACKET_CAPACITY + 1; ++i) {
		statuses[i] = 0x12345678UL;
		completed[i] = 0x87654321UL;
		packets[i].pvBuffer = pcm;
		packets[i].dwMaxSize = sizeof(pcm);
		packets[i].pdwStatus = &statuses[i];
		packets[i].pdwCompletedSize = &completed[i];
	}
	for (i = 0; i < PACKET_CAPACITY; ++i) {
		CHECK(stream->lpVtbl->Process(stream, &packets[i], NULL) == S_OK);
		CHECK(statuses[i] == XMEDIAPACKET_STATUS_PENDING);
		CHECK(completed[i] == 0);
	}
	CHECK(DirectSoundGetStreamVoiceStatus(stream) != 0);
	CHECK(stream->lpVtbl->GetStatus(stream, &status) == S_OK);
	CHECK((status & XMO_STATUSF_ACCEPT_INPUT_DATA) == 0);
	CHECK(stream->lpVtbl->Process(stream, &packets[PACKET_CAPACITY], NULL) == E_OUTOFMEMORY);
	CHECK(statuses[PACKET_CAPACITY] == 0x12345678UL);
	CHECK(completed[PACKET_CAPACITY] == 0x87654321UL);

	DirectSoundStopStream(stream);
	for (i = 0; i < PACKET_CAPACITY; ++i) {
		CHECK(statuses[i] == XMEDIAPACKET_STATUS_FLUSHED);
		CHECK(completed[i] == 0);
	}
	CHECK(DirectSoundGetStreamVoiceStatus(stream) == 0);
	CHECK(stream->lpVtbl->GetStatus(stream, &status) == S_OK);
	CHECK((status & XMO_STATUSF_ACCEPT_INPUT_DATA) != 0);
	CHECK(stream->lpVtbl->Release(stream) == 0);
	printf("DirectSound packet lifecycle: %u checks, %u failures\n", checks, failures);
	return failures ? 1 : 0;
}
