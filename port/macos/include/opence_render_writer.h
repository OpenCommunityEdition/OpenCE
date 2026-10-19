/* The guest's side of the render commands (opence_render.h): a writer that
builds batches in a buffer of the guest's own and gives each to the host
with one import, host_render_submit, its doorbell (docs/macos_host.md,
"The renderer"). The host copies a batch out before it returns, so the
buffer is the writer's again at once.

A writer is one thread's: it takes no locks. */

#ifndef OPENCE_RENDER_WRITER_H
#define OPENCE_RENDER_WRITER_H

struct opence_render_writer
{
	unsigned int *words;
	/* the buffer's size in words */
	unsigned int capacity;
	/* the words of the batch being written, its header included, and its
	commands */
	unsigned int length;
	unsigned int count;
	/* batches given to the host, commands it refused, and batches it
	refused whole, since the writer was made */
	unsigned int submitted;
	unsigned int refused;
	unsigned int failed;
};

/* a writer of batches in the `size` bytes at buffer (4-byte aligned; at
most OPENCE_RENDER_MAX_BATCH_LENGTH of them are used) */
void opence_render_writer_init(struct opence_render_writer *writer, void *buffer, unsigned int size);

/* a command of `op` and `words` words in all (its first included): the
place for its words - 1 fields, which the caller writes. A command that
does not fit after the batch's others submits them first. NULL, with
nothing written, for a command that no batch of the buffer holds */
unsigned int *opence_render_command(struct opence_render_writer *writer, unsigned int op, unsigned int words);

/* the header written for the batch so far; its size in bytes */
unsigned int opence_render_finish(struct opence_render_writer *writer);

/* gives the batch to the host (one exit) and starts the next: the number
of its commands the host refused, -1 if it refused the batch whole. An
empty batch is not given, and is 0 */
int opence_render_submit(struct opence_render_writer *writer);

/* a float field's word */
unsigned int opence_render_float(float value);

#endif
