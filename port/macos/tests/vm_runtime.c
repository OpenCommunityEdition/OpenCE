/*
VM_RUNTIME.C

A guest program for tools/test_macos_vm.py: the guest runtime's services as
the macOS host gives them in its VM (threads, their thread pointer, futexes,
time, files, memory). It runs the case named by argv[1], prints "ok" and
returns 0 if it passes; else it prints what failed and returns 1. Some
cases take a folder of their own as argv[2].
*/

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "posix.h"

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

/* ---------- threads */

#define THREADS 8
#define ROUNDS 20000

static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
static long counter;
static __thread int thread_value;

struct worker
{
	int index;
	pthread_t self;
	int kept_value;
};

static void *work(void *context)
{
	struct worker *worker = context;
	int round;

	worker->self = pthread_self();
	thread_value = worker->index + 100;
	for (round = 0; round < ROUNDS; round++)
	{
		pthread_mutex_lock(&counter_lock);
		counter++;
		pthread_mutex_unlock(&counter_lock);
		if (round % 1000 == 0)
			sched_yield();
	}
	/* (the thread's own TLS, after yields and other threads' runs) */
	worker->kept_value = thread_value;
	return worker;
}

static void case_threads(void)
{
	struct worker workers[THREADS];
	pthread_t threads[THREADS];
	int index, other;

	thread_value = 7;
	for (index = 0; index < THREADS; index++)
	{
		workers[index].index = index;
		CHECK(pthread_create(&threads[index], NULL, work, &workers[index]) == 0);
	}
	for (index = 0; index < THREADS; index++)
	{
		void *result = NULL;

		CHECK(pthread_join(threads[index], &result) == 0);
		CHECK(result == &workers[index]);
		CHECK(workers[index].kept_value == index + 100);
		CHECK(!pthread_equal(workers[index].self, pthread_self()));
		for (other = 0; other < index; other++)
			CHECK(!pthread_equal(workers[index].self, workers[other].self));
	}
	CHECK(counter == (long)THREADS * ROUNDS);
	CHECK(thread_value == 7);
}

/* ---------- condition variables (futex waits, wakes and requeues) */

static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_ready = PTHREAD_COND_INITIALIZER;
static int queue[64];
static int queue_count, produced, consumed_total;

static void *consume(void *unused)
{
	int taken = 0;

	(void)unused;
	for (;;)
	{
		pthread_mutex_lock(&queue_lock);
		while (!queue_count && produced < 1000)
			pthread_cond_wait(&queue_ready, &queue_lock);
		if (!queue_count && produced >= 1000)
		{
			pthread_mutex_unlock(&queue_lock);
			break;
		}
		consumed_total += queue[--queue_count];
		taken++;
		pthread_mutex_unlock(&queue_lock);
	}
	return (void *)(long)taken;
}

static void case_condition(void)
{
	pthread_t consumers[3];
	long taken = 0;
	int index;

	for (index = 0; index < 3; index++)
		CHECK(pthread_create(&consumers[index], NULL, consume, NULL) == 0);
	for (index = 1; index <= 1000; index++)
	{
		pthread_mutex_lock(&queue_lock);
		while (queue_count == 64)
		{
			pthread_mutex_unlock(&queue_lock);
			sched_yield();
			pthread_mutex_lock(&queue_lock);
		}
		queue[queue_count++] = index;
		produced = index;
		pthread_cond_signal(&queue_ready);
		pthread_mutex_unlock(&queue_lock);
	}
	pthread_mutex_lock(&queue_lock);
	pthread_cond_broadcast(&queue_ready);
	pthread_mutex_unlock(&queue_lock);
	for (index = 0; index < 3; index++)
	{
		void *result;

		CHECK(pthread_join(consumers[index], &result) == 0);
		taken += (long)result;
	}
	CHECK(taken == 1000);
	CHECK(consumed_total == 1000 * 1001 / 2);
}

/* a timed wait that times out */
static void case_timed_wait(void)
{
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t never = PTHREAD_COND_INITIALIZER;
	struct timespec until, before, after;
	long elapsed;

	clock_gettime(CLOCK_MONOTONIC, &before);
	clock_gettime(CLOCK_REALTIME, &until);
	until.tv_nsec += 200 * 1000000;
	if (until.tv_nsec >= 1000000000)
	{
		until.tv_nsec -= 1000000000;
		until.tv_sec++;
	}
	pthread_mutex_lock(&lock);
	CHECK(pthread_cond_timedwait(&never, &lock, &until) == ETIMEDOUT);
	pthread_mutex_unlock(&lock);
	clock_gettime(CLOCK_MONOTONIC, &after);
	elapsed = (after.tv_sec - before.tv_sec) * 1000 + (after.tv_nsec - before.tv_nsec) / 1000000;
	CHECK(elapsed >= 150 && elapsed < 2000);
}

/* ---------- time */

static void case_time(void)
{
	struct timespec before, after, nap = { 0, 50 * 1000000 };
	long elapsed;

	CHECK(clock_gettime(CLOCK_MONOTONIC, &before) == 0);
	CHECK(nanosleep(&nap, NULL) == 0);
	CHECK(clock_gettime(CLOCK_MONOTONIC, &after) == 0);
	elapsed = (after.tv_sec - before.tv_sec) * 1000 + (after.tv_nsec - before.tv_nsec) / 1000000;
	CHECK(elapsed >= 45 && elapsed < 1000);
	CHECK(time(NULL) > 1700000000);
}

/* ---------- files: Linux's flags and errno values, as the guest knows them */

static void case_files(const char *folder)
{
	char path[512], other[512];
	char buffer[64];
	struct stat information;
	int fd, found = 0, pipe_fds[2];
	DIR *directory;
	struct dirent *entry;

	snprintf(path, sizeof(path), "%s/a_file.txt", folder);
	snprintf(other, sizeof(other), "%s/renamed.txt", folder);
	fd = open(path, O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC, 0644);
	CHECK(fd >= 0);
	CHECK(write(fd, "hello, mac", 10) == 10);
	CHECK(close(fd) == 0);
	CHECK(open(path, O_CREAT | O_EXCL | O_WRONLY, 0644) == -1 && errno == EEXIST);
	fd = open(path, O_RDWR | O_APPEND);
	CHECK(fd >= 0);
	CHECK(write(fd, "!", 1) == 1);
	CHECK(lseek(fd, 0, SEEK_SET) == 0);
	CHECK(read(fd, buffer, sizeof(buffer)) == 11 && !memcmp(buffer, "hello, mac!", 11));
	CHECK((fcntl(fd, F_GETFL) & O_ACCMODE) == O_RDWR && (fcntl(fd, F_GETFL) & O_APPEND));
	CHECK(fstat(fd, &information) == 0 && information.st_size == 11 && S_ISREG(information.st_mode));
	CHECK(close(fd) == 0);
	CHECK(stat(folder, &information) == 0 && S_ISDIR(information.st_mode));
	CHECK(rename(path, other) == 0);
	CHECK(open(path, O_RDONLY) == -1 && errno == ENOENT);
	directory = opendir(folder);
	CHECK(directory != NULL);
	while ((entry = readdir(directory)) != NULL)
		found += !strcmp(entry->d_name, "renamed.txt");
	CHECK(closedir(directory) == 0);
	CHECK(found == 1);
	CHECK(unlink(other) == 0);
	CHECK(mkdir(path, 0755) == 0 && rmdir(path) == 0);
	/* an empty non-blocking pipe: EAGAIN, which is 11 to Linux and 35 to
	Darwin */
	CHECK(pipe2(pipe_fds, O_NONBLOCK) == 0);
	CHECK(read(pipe_fds[0], buffer, 1) == -1 && errno == EAGAIN && EAGAIN == 11);
	CHECK(write(pipe_fds[1], "x", 1) == 1 && read(pipe_fds[0], buffer, 1) == 1 && buffer[0] == 'x');
	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

/* ---------- memory */

static void case_memory(void)
{
	size_t size = 3 * 1024 * 1024 + 123, index;
	unsigned char *block = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	unsigned char *again;
	char *large;

	CHECK(block != MAP_FAILED);
	for (index = 0; index < size; index += 4096)
		CHECK(block[index] == 0);
	memset(block, 0xab, size);
	CHECK(munmap(block, size) == 0);
	again = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	CHECK(again != MAP_FAILED);
	/* (memory given back comes back zero) */
	for (index = 0; index < size; index += 4096)
		CHECK(again[index] == 0);
	CHECK(munmap(again, size) == 0);
	large = malloc(64 * 1024 * 1024);
	CHECK(large != NULL);
	memset(large, 1, 64 * 1024 * 1024);
	free(large);
	CHECK((unsigned long)&size < 0x100000000UL);
}

/* ---------- sockets: the platform layer's (port/linux/src/posix.h), which
the host's posix_net.c carries out */

static void case_sockets(void)
{
	unsigned char address[16], from[16];
	int receiver, sender, length = sizeof(from);
	char buffer[16];

	/* struct sockaddr_in for 127.0.0.1, port 0 (any) in the guest's (Linux's)
	layout, a 16-bit family of 2 first; Darwin's has a length byte (16)
	before an 8-bit family. posix_net.c converts both ways */
	memset(address, 0, sizeof(address));
	address[0] = 2;
	address[4] = 127;
	address[7] = 1;
	receiver = posix_socket(2, 2, 0);
	CHECK(receiver >= 0);
	CHECK(posix_socket_bind(receiver, address, sizeof(address)) == 0);
	CHECK(posix_socket_getsockname(receiver, address, &length) == 0);
	CHECK(address[0] == 2 && address[1] == 0 && address[4] == 127 && (address[2] | address[3]));
	CHECK(posix_socket_set_nonblocking(receiver, 1) == 0);
	/* nothing waits: WSAEWOULDBLOCK (10035), the Windows Sockets code
	posix_net.c gives the platform layer for EAGAIN */
	CHECK(posix_socket_recv(receiver, buffer, sizeof(buffer), 0) == -1);
	CHECK(posix_socket_last_error() == 10035);
	sender = posix_socket(2, 2, 0);
	CHECK(sender >= 0);
	CHECK(posix_socket_sendto(sender, "ping", 4, 0, address, sizeof(address)) == 4);
	length = sizeof(from);
	{
		int tries;
		int received = -1;

		for (tries = 0; tries < 100 && received < 0; tries++)
		{
			received = posix_socket_recvfrom(receiver, buffer, sizeof(buffer), 0, from, &length);
			if (received < 0)
				usleep(10000);
		}
		CHECK(received == 4 && !memcmp(buffer, "ping", 4));
		CHECK(from[0] == 2 && from[1] == 0 && from[4] == 127);
	}
	CHECK(posix_socket_close(sender) == 0);
	CHECK(posix_socket_close(receiver) == 0);
}

int main(int argc, char **argv)
{
	const char *name = argc > 1 ? argv[1] : "";

	if (!strcmp(name, "threads"))
		case_threads();
	else if (!strcmp(name, "condition"))
		case_condition();
	else if (!strcmp(name, "timed_wait"))
		case_timed_wait();
	else if (!strcmp(name, "time"))
		case_time();
	else if (!strcmp(name, "files") && argc > 2)
		case_files(argv[2]);
	else if (!strcmp(name, "memory"))
		case_memory();
	else if (!strcmp(name, "sockets"))
		case_sockets();
	else if (!strcmp(name, "exit"))
		exit(42);
	else
	{
		printf("no case %s\n", name);
		return 2;
	}
	printf("ok\n");
	return 0;
}
