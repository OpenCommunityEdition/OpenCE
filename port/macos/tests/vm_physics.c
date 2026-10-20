/*
VM_PHYSICS.C

A guest program for tools/test_macos_physics_vm.py: physics batches
(port/macos/include/host_physics.h) given to the host with
host_physics_step, and the results it writes back. It runs the case named by
argv[1], prints what the test compares, then "ok", and returns 0; else it
prints what failed and returns 1.

"drop" sends a level (a floor at z 0, 100 units across) and a box, steps the
world 120 times at 30 a second and checks that the box comes to rest on the
floor; "refusals" sends batches the host must refuse.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_host.h"
#include "host_physics.h"

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

#define STEPS 120
#define HALF 0.25f

static unsigned int batch[1024];
static unsigned int results[1024];
static unsigned int length;
static unsigned int commands;
static float vertices[4 * 3] = {-50, -50, 0, 50, -50, 0, 50, 50, 0, -50, 50, 0};
/* counter-clockwise from above: the faces look up */
static unsigned int indices[2 * 3] = {0, 1, 2, 0, 2, 3};

static unsigned int word_of(float value)
{
	unsigned int word;

	memcpy(&word, &value, sizeof(word));
	return word;
}

static float float_of(unsigned int word)
{
	float value;

	memcpy(&value, &word, sizeof(value));
	return value;
}

static void begin(float seconds)
{
	batch[0] = HOST_PHYSICS_BATCH_MAGIC;
	batch[1] = HOST_PHYSICS_VERSION;
	batch[4] = word_of(seconds);
	length = HOST_PHYSICS_BATCH_HEADER_WORDS;
	commands = 0;
}

static void command(unsigned int op, unsigned int words)
{
	batch[length++] = HOST_PHYSICS_COMMAND(op, words);
	commands++;
}

static int step(void)
{
	batch[2] = length * 4;
	batch[3] = commands;
	return host_physics_step((unsigned int)(unsigned long)batch, length * 4, (unsigned int)(unsigned long)results,
		sizeof(results));
}

static void drop(void)
{
	unsigned int index;
	float z = 0;

	begin(0);
	command(HOST_PHYSICS_OP_CLEAR, HOST_PHYSICS_CLEAR_WORDS);
	command(HOST_PHYSICS_OP_LEVEL, HOST_PHYSICS_LEVEL_WORDS);
	batch[length++] = (unsigned int)(unsigned long)vertices;
	batch[length++] = 4;
	batch[length++] = (unsigned int)(unsigned long)indices;
	batch[length++] = 2;
	command(HOST_PHYSICS_OP_BODY_ADD, HOST_PHYSICS_BODY_ADD_WORDS);
	batch[length++] = 1;
	batch[length++] = HOST_PHYSICS_MOTION_DYNAMIC;
	batch[length++] = HOST_PHYSICS_SHAPE_BOX;
	for (index = 0; index < 3; index++)
		batch[length++] = word_of(HALF);
	batch[length++] = word_of(20); /* kg */
	batch[length++] = word_of(0.8f);
	batch[length++] = word_of(0);
	/* at (0, 0, 2), not turned, still */
	batch[length++] = word_of(0);
	batch[length++] = word_of(0);
	batch[length++] = word_of(2);
	batch[length++] = word_of(0);
	batch[length++] = word_of(0);
	batch[length++] = word_of(0);
	batch[length++] = word_of(1);
	for (index = 0; index < 6; index++)
		batch[length++] = word_of(0);
	CHECK(step() == 0);
	CHECK(results[0] == HOST_PHYSICS_RESULTS_MAGIC);
	CHECK(results[3] == 0 && results[4] == 1);
	for (index = 0; index < STEPS; index++)
	{
		begin(1.0f / 30.0f);
		CHECK(step() == 0);
		CHECK(results[0] == HOST_PHYSICS_RESULTS_MAGIC && results[4] == 1 && results[5] == 0);
		/* the body's entry: id, awake, x, y, z, ... */
		CHECK(results[HOST_PHYSICS_RESULTS_HEADER_WORDS] == 1);
		z = float_of(results[HOST_PHYSICS_RESULTS_HEADER_WORDS + 4]);
		CHECK(z == z && z > HALF - 0.05f && z < 2.05f);
	}
	printf("steps %u\n", STEPS);
	printf("rest z %.3f awake %u\n", z, results[HOST_PHYSICS_RESULTS_HEADER_WORDS + 1]);
	CHECK(fabsf(z - HALF) < 0.02f);
}

static void refusals(void)
{
	/* a batch outside guest memory, and one whose header is wrong */
	CHECK(host_physics_step(0xfffff000u, 64, (unsigned int)(unsigned long)results, sizeof(results)) == -1);
	begin(0);
	batch[0] = 0;
	CHECK(step() == -1);
	/* a level whose triangles name a vertex it does not have: refused, and
	   the next command still carried out */
	begin(0);
	command(HOST_PHYSICS_OP_LEVEL, HOST_PHYSICS_LEVEL_WORDS);
	batch[length++] = (unsigned int)(unsigned long)vertices;
	batch[length++] = 3;
	batch[length++] = (unsigned int)(unsigned long)indices;
	batch[length++] = 2;
	command(HOST_PHYSICS_OP_CLEAR, HOST_PHYSICS_CLEAR_WORDS);
	CHECK(step() == 1);
}

int main(int argc, char **argv)
{
	if (argc < 2)
		return 1;
	if (!strcmp(argv[1], "drop"))
		drop();
	else if (!strcmp(argv[1], "refusals"))
		refusals();
	else
		return 1;
	printf("ok\n");
	return 0;
}
