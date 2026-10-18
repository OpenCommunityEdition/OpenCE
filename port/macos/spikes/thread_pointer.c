/* Spike: does macOS keep a per-thread value in TPIDR_EL0 for user code?

The guest's musl keeps its thread pointer in TPIDR_EL0. Darwin itself uses
TPIDRRO_EL0 for its own threads, so TPIDR_EL0 may be free for the guest,
or it may be cleared on a context switch. Two threads set different values,
yield and sleep many times, and check their own value each time. */

#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define ROUNDS 2000

static uint64_t read_tpidr(void)
{
	uint64_t value;

	__asm__ volatile("mrs %0, tpidr_el0" : "=r"(value));
	return value;
}

static void write_tpidr(uint64_t value)
{
	__asm__ volatile("msr tpidr_el0, %0" : : "r"(value));
}

static void *check(void *argument)
{
	uint64_t mine = (uint64_t)(uintptr_t)argument;
	int round;

	write_tpidr(mine);
	for (round = 0; round < ROUNDS; round++) {
		if (round % 2)
			sched_yield();
		else
			usleep(50);
		if (read_tpidr() != mine)
			return (void *)(uintptr_t)(round + 1);
	}
	return NULL;
}

int main(void)
{
	pthread_t threads[2];
	void *results[2];
	uint64_t before = read_tpidr();
	int i;

	printf("TPIDR_EL0 at start: 0x%llx\n", before);
	pthread_create(&threads[0], NULL, check, (void *)(uintptr_t)0x80001000u);
	pthread_create(&threads[1], NULL, check, (void *)(uintptr_t)0x80002000u);
	for (i = 0; i < 2; i++)
		pthread_join(threads[i], &results[i]);

	for (i = 0; i < 2; i++) {
		if (results[i]) {
			printf("FAIL: thread %d lost its TPIDR_EL0 at round %d\n", i, (int)(uintptr_t)results[i]);
			return 1;
		}
	}
	printf("both threads kept TPIDR_EL0 for %d rounds of yield and sleep\n", ROUNDS);
	printf("PASS\n");
	return 0;
}
