/*
OPENCE_RENDER_WRITER.C

The guest's render batches (port/macos/include/opence_render_writer.h).
The words are written as the guest holds them, which is little-endian, as
the batches are.
*/

#include "opence_render.h"
#include "opence_render_writer.h"

#include <stdint.h>
#include <string.h>

#include "guest_host.h"

void opence_render_writer_init(struct opence_render_writer *writer, void *buffer, unsigned int size)
{
	memset(writer, 0, sizeof(*writer));
	if (size > OPENCE_RENDER_MAX_BATCH_LENGTH)
		size = OPENCE_RENDER_MAX_BATCH_LENGTH;
	writer->words = buffer;
	writer->capacity = size / 4;
	writer->length = OPENCE_RENDER_BATCH_HEADER_WORDS;
	/* (a buffer too small for a header holds no command) */
	if (writer->capacity < OPENCE_RENDER_BATCH_HEADER_WORDS)
		writer->capacity = 0;
}

unsigned int *opence_render_command(struct opence_render_writer *writer, unsigned int op, unsigned int words)
{
	unsigned int *command;

	if (!words || words > 0xffffu || writer->capacity < OPENCE_RENDER_BATCH_HEADER_WORDS ||
		words > writer->capacity - OPENCE_RENDER_BATCH_HEADER_WORDS)
		return NULL;
	if (words > writer->capacity - writer->length)
		opence_render_submit(writer);
	command = writer->words + writer->length;
	command[0] = OPENCE_RENDER_COMMAND(op, words);
	writer->length += words;
	writer->count++;
	return command + 1;
}

unsigned int opence_render_finish(struct opence_render_writer *writer)
{
	if (writer->capacity < OPENCE_RENDER_BATCH_HEADER_WORDS)
		return 0;
	writer->words[0] = OPENCE_RENDER_MAGIC;
	writer->words[1] = OPENCE_RENDER_VERSION;
	writer->words[2] = writer->length * 4;
	writer->words[3] = writer->count;
	return writer->length * 4;
}

int opence_render_submit(struct opence_render_writer *writer)
{
	unsigned int size;
	int refused;

	if (!writer->count)
		return 0;
	size = opence_render_finish(writer);
	refused = host_render_submit((unsigned int)(uintptr_t)writer->words, size);
	writer->submitted++;
	if (refused < 0)
		writer->failed++;
	else
		writer->refused += (unsigned int)refused;
	writer->length = OPENCE_RENDER_BATCH_HEADER_WORDS;
	writer->count = 0;
	return refused;
}

unsigned int opence_render_float(float value)
{
	unsigned int word;

	memcpy(&word, &value, sizeof(word));
	return word;
}
